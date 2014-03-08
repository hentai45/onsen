/**
 * メモリ管理
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_MEMORY
#define HEADER_MEMORY

#include "sysinfo.h"

//-----------------------------------------------------------------------------
// メモリマップ

#define SZ4MB  4 * 1024 * 1024  // 4MB のサイズ

#define VADDR_BASE          (0xC0000000)  // 論理アドレスのベースアドレス

#define VADDR_USER_ESP      (0xBFFFF000)

#define VADDR_SYS_INFO      (0xC0000A00)  // システム情報が格納されているアドレス
/* FREE START               (0x00001000) */
/* OS_PDTはCR3レジスタに設定するので物理アドレスでなければいけない */
#define MADDR_OS_PDT        (0x00001000)  // 4KB(0x1000)境界であること
#define VADDR_OS_PDT        (0xC0001000)  // 4KB(0x1000)境界であること
#define VADDR_BITMAP_START  (0xC0010000)  // 物理アドレス管理用ビットマップ
#define VADDR_BITMAP_END    (0xC0030000)
#define VADDR_BMEM_MNG      (VADDR_BITMAP_END)
#define VADDR_VMEM_MNG      (VADDR_BMEM_MNG + 0x10000)
/* FREE END                 (0x0009FFFF) */
#define VADDR_DISK_IMG      (0xC0100000)
#define VADDR_IDT           (0xC026F800)
#define LIMIT_IDT           (0x000007FF)
#define VADDR_GDT           (0xC0270000)
#define LIMIT_GDT           (0x0000FFFF)
#define VADDR_OS            (0xC0280000)  // 最大 1,152 KB
#define VADDR_MEM_START     (0xC0400000)
#define VADDR_VRAM          (0xE0000000)
#define VADDR_MEM_END       (VADDR_VRAM)
#define VADDR_PD_SELF       (0xFFFFF000)

/* 0x4000境界であること。bitmapの初期化がそのことを前提に書かれているから */
#define MADDR_FREE_START    (0x00400000)


extern SYSTEM_INFO *g_sys_info;


//-----------------------------------------------------------------------------
// メモリ管理

void  mem_init(void);
void *mem_alloc(unsigned int size_B);
void *mem_alloc_str(const char *s);
void *mem_alloc_user_page(unsigned long vaddr, int size_B, int flags);
unsigned long mem_expand_stack(unsigned long old_stack, unsigned long new_stack);
void *mem_alloc_maddr(void);
int   mem_free(void *vp_vaddr);
int   mem_free_user(void *vp_vaddr);
int   mem_free_maddr(void *vp_maddr);
void  mem_dbg(void);

//-----------------------------------------------------------------------------
// メモリ容量確認

unsigned int mem_total_B(void);
unsigned int mem_total_mfree_B(void);
unsigned int mem_total_vfree_B(void);

#endif


//=============================================================================
// 非公開ヘッダ

#include <stdbool.h>

#include "asmfunc.h"
#include "debug.h"
#include "paging.h"
#include "stacktrace.h"
#include "str.h"
#include "sysinfo.h"


SYSTEM_INFO *g_sys_info = (SYSTEM_INFO *) VADDR_SYS_INFO;

//-----------------------------------------------------------------------------
// メモリ管理

typedef struct _MEMORY {
    void *addr;
    unsigned int size;
} MEMORY;


typedef struct _MEM_MNG {
    // 記憶領域の確保用。
    // free.addrの昇順でなければならない
    MEMORY *free;

    int max_free;    // freeの最大数
    int num_free;    // 空き情報の数
    int total_free;  // free.sizeの合計
    int unit;        // sizeの単位
    int info_size;   // 管理情報のサイズ
} MEM_MNG;


static int mem_set_free(MEM_MNG *mng, void *vp_addr, unsigned int size);
static void init_mem_mng(MEM_MNG *mng, int max_free, int unit, int info_size);
static void *get_free_addr(MEM_MNG *mng, unsigned int size);
static int page_free_maddr(void *vp_vaddr);


//-----------------------------------------------------------------------------
// 8バイト単位メモリ管理（4096-8バイト以下のメモリ割り当てを管理する）

#define BYTE_MEM_MNG_MAX  (4096)
#define MEM_INFO_B        (8)                // メモリの先頭に置く管理情報のサイズ
#define BYTE_MEM_BYTES    (1 * 1024 * 1024)  // バイト管理する容量
#define MM_SIG            (0xBAD41BAD)       // メモリの先頭に置くシグネチャ
#define BYTES_TO_8BYTES(b)   (((b) + 7) >> 3)  // バイトを8バイト単位に変換する

typedef struct _INFO_8BYTES {
    unsigned long signature;
    unsigned long size;
} INFO_8BYTES;

static MEM_MNG *l_mng_b = (MEM_MNG *) VADDR_BMEM_MNG;


//-----------------------------------------------------------------------------
// ページ単位メモリ管理


#define PAGE_MEM_MNG_MAX   (4096)

#define SET_MNG_FLG  (true)
#define NO_MNG_FLG   (false)

/**
 * 物理アドレスからビットマップのインデックスに変換
 * 4096バイト(0x1000)単位で管理しているので12ビット右にシフト
 * さらに、4バイトで32管理できるので5ビット右にシフト
 * 合計17ビット右にシフトする。
 */
#define MADDR2IDX(maddr)  (((unsigned long) maddr) >> 17)

// 物理アドレスからビットマップのビット位置に変換
#define MADDR2BIT(maddr)  (1 << (31 - ((((unsigned long) maddr) >> 12) & 0x1F)))

// インデックスとビット番号から物理アドレスに変換
#define IDX2MADDR(idx, bits)  ((void *) (((idx) << 17) | ((31 - (bits)) << 12)))

// 指定した物理アドレスが空いているなら0以外
#define IS_FREE_MADDR(maddr) (l_bitmap[MADDR2IDX(maddr)] & MADDR2BIT(maddr))

// 指定した物理アドレスを空きにする
#define SET_FREE_MADDR(maddr) (l_bitmap[MADDR2IDX(maddr)] |= MADDR2BIT(maddr))

// 指定した物理アドレスを使用中にする
#define SET_USED_MADDR(maddr) (l_bitmap[MADDR2IDX(maddr)] &= ~MADDR2BIT(maddr))

#define BITMAP_ST MADDR2IDX(MADDR_FREE_START)

static MEM_MNG *l_mng_v = (MEM_MNG *) VADDR_VMEM_MNG;

// 物理アドレスを管理するためのビットマップ。使用中なら0、空きなら1
static unsigned long *l_bitmap = (unsigned long *) VADDR_BITMAP_START;

// ビットマップの最後のインデックス
static int l_bitmap_end;

static unsigned long l_mfree_B;


static void *mem_alloc_kernel_page(unsigned int num_pages, bool set_mng_flg);
static int page_free(void *maddr);

//=============================================================================
// 関数

//-----------------------------------------------------------------------------
// メモリ管理

static void init_mpage_mem(void);
static void init_vpage_mem(void);
static void init_byte_mem(void);

/**
 * メモリ管理の初期化
 */
void mem_init(void)
{
    init_mpage_mem();
    init_vpage_mem();
    init_byte_mem();
}

/**
 * ページ単位メモリ管理（物理アドレス）の初期化
 */
static void init_mpage_mem(void)
{
    /* ビットマップを使用中で初期化 */
    unsigned long size_B = VADDR_BITMAP_END - VADDR_BITMAP_START;
    memset(l_bitmap, 0, size_B);

    /* 空き領域を設定 */
    l_bitmap_end = MADDR2IDX(g_sys_info->end_free_maddr) + 1;
    size_B = (unsigned long) &l_bitmap[l_bitmap_end - 1] - (unsigned long) &l_bitmap[BITMAP_ST];
    memset(&l_bitmap[BITMAP_ST], 0xFF, size_B);

    l_mfree_B = g_sys_info->end_free_maddr - MADDR_FREE_START;
}


/**
 * ページ単位メモリ管理（論理アドレス）の初期化
 */
static void init_vpage_mem(void)
{
    init_mem_mng(l_mng_v, PAGE_MEM_MNG_MAX, PAGE_SIZE_B, 0);

    unsigned long size_B = VADDR_MEM_END - VADDR_MEM_START;
    int num_pages = BYTE_TO_PAGE(size_B);
    mem_set_free(l_mng_v, (void *) VADDR_MEM_START, num_pages);
}

/**
 * バイト単位メモリ管理の初期化
 */
static void init_byte_mem(void)
{
    init_mem_mng(l_mng_b, BYTE_MEM_MNG_MAX, /* unit = */ 8, BYTES_TO_8BYTES(MEM_INFO_B));

    unsigned long size_B = BYTE_MEM_BYTES;
    int num_pages = BYTE_TO_PAGE(size_B);
    void *addr = mem_alloc_kernel_page(num_pages, NO_MNG_FLG);
    mem_set_free(l_mng_b, addr, BYTES_TO_8BYTES(size_B));
}

/**
 * メモリ管理構造体の初期化
 */
static void init_mem_mng(MEM_MNG *mng, int max_free, int unit, int info_size)
{
    mng->free = (MEMORY *) (mng + 1);
    mng->max_free = max_free;
    mng->num_free = 0;
    mng->total_free = 0;
    mng->unit = unit;
    mng->info_size = info_size;
}


/**
 * @note 8バイト単位メモリ管理の場合は、割り当てられたメモリの前に割り当てサイズが置かれる。
 *
 * @note 4KB以上確保するならページ単位メモリ管理のほうで管理させる。
 */
void *mem_alloc(unsigned int size_B)
{
    if (size_B == 0) {
        return 0;
    }

    if (size_B >= PAGE_SIZE_B - (l_mng_b->info_size * l_mng_b->unit)) {
        return mem_alloc_kernel_page(BYTE_TO_PAGE(size_B), SET_MNG_FLG);
    }

    void *p = get_free_addr(l_mng_b, BYTES_TO_8BYTES(size_B));

    if (p == 0) {
        return mem_alloc_kernel_page(BYTE_TO_PAGE(size_B), SET_MNG_FLG);
    }

    return p;
}


int mem_free(void *vp_vaddr)
{
    if (vp_vaddr == 0) {
        return -1;
    }

    if (vp_vaddr < VADDR_MEM_START) {
        DBGF("vaddr < VADDR_MEM_START\n");
        stacktrace(3, f_debug);
        return -1;
    }

    if ((unsigned long) vp_vaddr & 0x7) {
        // メモリ管理が返すメモリは、下位3ビットが0であるはず
        return -1;
    }

    int flg = paging_get_flags(vp_vaddr);

    if (flg & 0xE00) {  // ページ単位メモリ管理が使うフラグが立っている
        /* ページ単位メモリ管理 */

        if (IS_4KB_ALIGN(vp_vaddr)) {
            return page_free(vp_vaddr);
        } else {
            return -1;
        }
    }

    /* 8バイト単位メモリ管理 */

    unsigned int vaddr = (unsigned int) vp_vaddr;

    // 割り当てサイズの取得
    INFO_8BYTES *info = (INFO_8BYTES *) (vaddr - MEM_INFO_B);

    // 「ここで管理されてます」という印はあるか？
    if (info->signature != MM_SIG) {
        return -1;
    }

    return mem_set_free(l_mng_b, (void *) info, info->size);
}


int mem_free_user(void *vp_vaddr)
{
    if (vp_vaddr == 0) {
        return -1;
    }

    if ( ! IS_4KB_ALIGN(vp_vaddr)) {
        return -1;
    }

    int flg = paging_get_flags(vp_vaddr);
    if ((flg & PTE_START) == 0) {
        return -1;
    }

    if (page_free_maddr(vp_vaddr) < 0) {
        return -1;
    }

    if (flg & PTE_END) {
        return 0;
    }

    unsigned long vaddr = (unsigned long) vp_vaddr;
    vaddr += PAGE_SIZE_B;
    flg = paging_get_flags((void *) vaddr);

    while ((flg & PTE_END) == 0) {
        if ((flg & PTE_CONT) == 0) {
            return -1;
        }

        /* ASSERT */
        if (flg & PTE_START) {
            return -1;
        }

        if (page_free_maddr((void *) vaddr) < 0) {
            return -1;
        }

        vaddr += PAGE_SIZE_B;
        flg = paging_get_flags((void *) vaddr);
    }

    return page_free_maddr((void *) vaddr);
}


unsigned long mem_expand_stack(unsigned long old_stack, unsigned long new_stack)
{
    new_stack &= ~0xFFF;
    new_stack -= 0x1000;  // ちょっと余裕をもたせる

    if (mem_alloc_user_page(new_stack, old_stack - new_stack, PTE_RW) < 0) {
        return 0;
    }

    // 古いスタックと新しいスタックを１つにまとめる

    void *new_stack_last = (void *) (old_stack - 0x1000);
    int flg = paging_get_flags(new_stack_last);
    if ((flg & PTE_END) == 0) {
        DBGF("bug!");
    }
    flg &= ~PTE_END;
    paging_set_flags(new_stack_last, flg);
    if ((flg & PTE_START) == 0) {
        flg |= PTE_CONT;
    }

    flg = paging_get_flags((void *) old_stack);
    if ((flg & PTE_START) == 0) {
        DBGF("bug!");
    }
    flg &= ~PTE_START;
    if ((flg & PTE_END) == 0) {
        flg |= PTE_CONT;
    }
    paging_set_flags((void *) old_stack, flg);

    return new_stack;
}


/**
 * 1ページ分の物理アドレスを割り当てる
 */
void *mem_alloc_maddr(void)
{
    int i, j;
    void *maddr;
    for (i = BITMAP_ST; i < l_bitmap_end; i++) {
        if (l_bitmap[i] != 0) {
            for (j = 31; j >= 0; j--) {
                if (l_bitmap[i] & (1 << j)) {
                    maddr = IDX2MADDR(i, j);
                    SET_USED_MADDR(maddr);
                    l_mfree_B -= PAGE_SIZE_B;
                    return maddr;
                }
            }
        }
    }

    /* 物理アドレスが足りない */
    return 0;
}


/**
 * mem_alloc_maddrで取得した物理アドレスを解放する
 */
int mem_free_maddr(void *vp_maddr)
{
    SET_FREE_MADDR(vp_maddr & ~0xFFF);
    return 0;
}


static void dbg_mem_mng(MEM_MNG *mng);

void mem_dbg(void)
{
    dbgf("\n");
    DBGF("DEBUG BYTE UNIT MEMORY MANAGE");
    dbg_mem_mng(l_mng_b);

    DBGF("DEBUG PAGE UNIT MEMORY MANAGE");
    dbg_mem_mng(l_mng_v);
    dbgf("\n");
}

static void dbg_mem_mng(MEM_MNG *mng)
{
    int i;

    for (i = 0; i < mng->num_free; i++) {
        MEMORY *mem = &mng->free[i];

        dbgf("%d : addr = %#X, size = %Z\n", i, mem->addr, mem->size * mng->unit);
    }

    dbgf("\n");
}


void *mem_alloc_str(const char *s)
{
    void *p = mem_alloc(strlen(s));
    strcpy(p, s);
    return p;
}


//-----------------------------------------------------------------------------
// ページ単位メモリ管理


static int mem_alloc_page_sub(unsigned long vaddr, int num_pages, int flags, bool set_mng_flg);

void *mem_alloc_user_page(unsigned long vaddr, int size_B, int flags)
{
    int fraction = vaddr & 0xFFF;
    vaddr &= ~0xFFF;

    int num_pages = BYTE_TO_PAGE(size_B + fraction);

    if (mem_alloc_page_sub(vaddr, num_pages, flags | PTE_US, SET_MNG_FLG) < 0) {
        return 0;
    } else {
        return (void *) vaddr;
    }
}


/**
 * @return リニアアドレス。空き容量が足りなければ 0 が返ってくる
 */
void *mem_alloc_kernel_page(unsigned int num_pages, bool set_mng_flg)
{
    if (num_pages == 0) {
        return 0;
    }

    void *vaddr = get_free_addr(l_mng_v, num_pages);

    /* 論理アドレスが足りない */
    if (vaddr == 0) {
        return 0;
    }

    if (mem_alloc_page_sub((unsigned long) vaddr, num_pages, PTE_RW, set_mng_flg) < 0) {
        return 0;
    } else {
        return vaddr;
    }
}


static int mem_alloc_page_sub(unsigned long vaddr, int num_pages, int flags, bool set_mng_flg)
{
    void *vp_maddr;
    bool first = true;

    for (int i = BITMAP_ST; i < l_bitmap_end; i++) {
        if (l_bitmap[i] != 0) {
            for (int j = 31; j >= 0; j--) {
                if (l_bitmap[i] & (1 << j)) {
                    vp_maddr = IDX2MADDR(i, j);
                    SET_USED_MADDR(vp_maddr);
                    l_mfree_B -= PAGE_SIZE_B;

                    int flg = flags | PTE_PRESENT;

                    if (set_mng_flg) {
                        if (first == false && num_pages != 1) {
                            flg |= PTE_CONT;
                        }

                        if (first) {
                            flg |= PTE_START;
                            first = false;
                        }

                        if (num_pages == 1) {
                            flg |= PTE_END;
                        }
                    }

                    paging_map((void *) vaddr, vp_maddr, flg);
                    vaddr += PAGE_SIZE_B;

                    if (--num_pages == 0) {
                        return 0;
                    }
                }
            }
        }
    }

    /* 物理アドレスが足りない */
    return -1;
}


static int page_free(void *vp_vaddr)
{
    int flg = paging_get_flags(vp_vaddr);
    if ((flg & PTE_START) == 0) {
        return -1;
    }

    unsigned int num_pages = 1;

    if (page_free_maddr(vp_vaddr) < 0) {
        return -1;
    }

    if (flg & PTE_END) {
        return mem_set_free(l_mng_v, vp_vaddr, num_pages);
    }

    unsigned long vaddr = (unsigned long) vp_vaddr;
    vaddr += PAGE_SIZE_B;
    num_pages++;
    flg = paging_get_flags((void *) vaddr);

    while ((flg & PTE_END) == 0) {
        if ((flg & PTE_CONT) == 0) {
            return -1;
        }

        /* ASSERT */
        if (flg & PTE_START) {
            return -1;
        }

        if (page_free_maddr((void *) vaddr) < 0) {
            return -1;
        }

        vaddr += PAGE_SIZE_B;
        num_pages++;
        flg = paging_get_flags((void *) vaddr);
    }

    if (page_free_maddr((void *) vaddr) < 0) {
        return -1;
    }

    return mem_set_free(l_mng_v, vp_vaddr, num_pages);
}

static int page_free_maddr(void *vp_vaddr)
{
    void *vp_maddr = paging_get_maddr(vp_vaddr);

    if (vp_maddr == 0) {
        return -1;
    }

    if (IS_FREE_MADDR(vp_maddr)) {
        return -1;
    }

    SET_FREE_MADDR(vp_maddr);

    return 0;
}


static int mem_set_free(MEM_MNG *mng, void *vp_addr, unsigned int size)
{
    char *addr = (char *) vp_addr;

    if (size == 0) {
        return -1;
    }

    // ---- 管理している空きメモリとまとめれるならまとめる

    int i;
    MEMORY *next = 0;

    // 挿入位置または結合位置を検索
    for (i = 0; i < mng->num_free; i++) {
        if (mng->free[i].addr > vp_addr) {
            next = &mng->free[i];
            break;
        }
    }

    // うしろに空きがなければ末尾に追加
    if (next == 0) {
        if (mng->num_free >= mng->max_free) {
            return -1;
        }

        mng->free[i].addr = vp_addr;
        mng->free[i].size = size;

        mng->num_free++;  // 空きを１つ追加
        mng->total_free += size;

        return 0;
    }

    char *end_addr = addr + (size * mng->unit);

    // 前があるか？
    if (i > 0) {
        MEMORY *prev = &mng->free[i - 1];
        char *prev_end_addr = (char *) prev->addr + (prev->size * mng->unit);

        // 前とまとめれるか？
        if (prev_end_addr == addr) {
            prev->size += size;
            mng->total_free += size;

            // 後ろとまとめれるか？
            if (next != 0 && end_addr == next->addr) {
                prev->size += next->size;

                mng->num_free--; // 空き１つ追加されて前後２つ削除された

                for (; i < mng->num_free; i++) {
                    mng->free[i] = mng->free[i + 1];
                }
            }

            return 0;
        }
    }

    // 前とはまとめれなかった

    // 後ろとまとめれるか？
    if (next != 0 && end_addr == next->addr) {
        next->addr = addr;
        next->size += size;
        mng->total_free += size;
        // 空きが１つ追加されて１つ削除されたのでmng->num_freeは変わらない
        return 0;
    }

    // 前にも後ろにもまとめれなかった

    if (mng->num_free < mng->max_free) {
        // free[i]より後ろを、後ろへずらして、すきまをつくる
        for (int j = mng->num_free; j > i; j--) {
            mng->free[j] = mng->free[j - 1];
        }

        next->addr = addr;
        next->size = size;

        mng->num_free++;  // 空きが１つ追加
        mng->total_free += size;
        return 0;
    }


    // 失敗

    return -1;
}


// 連続した空き領域を取得する
static void *get_free_addr(MEM_MNG *mng, unsigned int size)
{
    if (mng->total_free < size) {
        return 0;
    }

    for (int i = 0; i < mng->num_free; i++) {
        if (mng->free[i].size >= size + mng->info_size) {
            // 空きが見つかった

            MEMORY *mem = &mng->free[i];

            unsigned int addr = (unsigned int) mem->addr;

            if (mng->info_size > 0) {
                // 割り当てサイズの記録。
                // ここで管理していることを表す印も追加する。
                INFO_8BYTES *info = (INFO_8BYTES *) addr;
                info->signature = MM_SIG;
                info->size = size + mng->info_size;
                addr += mng->info_size * mng->unit;
            }

            // 空きアドレスをずらす
            mem->addr += ((size + mng->info_size) * mng->unit);
            mem->size -= (size + mng->info_size);
            mng->total_free -= size + mng->info_size;

            if (mem->size <= mng->info_size) {
                // 空いた隙間をずらす
                mng->num_free--;
                for ( ; i < mng->num_free; i++) {
                    mng->free[i] = mng->free[i + 1];
                }
            }

            return (void *) addr;
        }
    }

    return 0;
}


//-----------------------------------------------------------------------------
// メモリ容量確認

unsigned int mem_total_B(void)
{
    return g_sys_info->end_free_maddr;
}


unsigned int mem_total_mfree_B(void)
{
    return l_mfree_B;
}

// 空き容量を取得
unsigned int mem_total_vfree_B(void)
{
    return l_mng_v->total_free * l_mng_v->unit;
}

