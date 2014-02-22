/**
 * メモリ管理
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_MEMORY
#define HEADER_MEMORY

//-----------------------------------------------------------------------------
// メモリマップ

#define SZ4MB  4 * 1024 * 1024  // 4MB のサイズ

#define VADDR_BASE          (0xC0000000)  // 論理アドレスのベースアドレス

#define VADDR_SYS_INFO      (0xC0000A00)  // システム情報が格納されているアドレス
/* FREE START               (0x00001000) */
/* OS_PDTはCR3レジスタに設定するので物理アドレスでなければいけない */
#define MADDR_OS_PDT        (0x00001000)  // 4KB(0x1000)境界であること
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



//-----------------------------------------------------------------------------
// メモリ管理

void  mem_init(void);
void *mem_alloc(unsigned int size_B);
void *mem_alloc_user(void *vp_vaddr, int size_B);
void *mem_alloc_maddr(void);
int   mem_free(void *vp_vaddr);
void  mem_dbg(void);

//-----------------------------------------------------------------------------
// メモリ容量確認

unsigned int mem_total_B(void);
unsigned int mem_total_mfree_B(void);
unsigned int mem_total_vfree_B(void);

#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "debug.h"
#include "paging.h"
#include "stdbool.h"
#include "str.h"
#include "sysinfo.h"

//-----------------------------------------------------------------------------
// メモリ管理

typedef struct _MEMORY {
    void *addr;
    unsigned int size;
} MEMORY;


typedef struct _MEM_MNG {
    /// 記憶領域の確保用。
    /// free.addrの昇順でなければならない
    MEMORY *free;

    int max_free;    ///< freeの最大数
    int num_free;    ///< 空き情報の数
    int total_free;  ///< free.sizeの合計
    int unit;        ///< sizeの単位
    int info_size_b; ///< 管理情報のサイズ
} MEM_MNG;


static int mem_set_free(MEM_MNG *mng, void *vp_addr, unsigned int size);
static void dbg_mem_mng(MEM_MNG *mng);
static void init_mem_mng(MEM_MNG *mng, int max_free, int unit, int info_size_b);
static void *get_free_addr(MEM_MNG *mng, unsigned int size);


//-----------------------------------------------------------------------------
// バイト単位メモリ管理

#define BYTE_MEM_MNG_MAX  (4096)
#define MEM_INFO_B        (4)                // メモリの先頭に置く管理情報のサイズ
#define BYTE_MEM_BYTES    (1 * 1024 * 1024)  // バイト管理する容量
#define MM_SIG            (0xBAD41000)       // メモリの先頭に置くシグネチャ

static MEM_MNG *l_mng_b = (MEM_MNG *) VADDR_BMEM_MNG;


//-----------------------------------------------------------------------------
// ページ単位メモリ管理


#define PAGE_MEM_MNG_MAX   (4096)

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

static void *get_free_maddr(unsigned int num_pages);

// 物理アドレスを管理するためのビットマップ。使用中なら0、空きなら1
static unsigned long *l_bitmap = (unsigned long *) VADDR_BITMAP_START;

// ビットマップの最後のインデックス
static int l_bitmap_end;

static unsigned long l_mfree_B;


static void *mem_alloc_page(unsigned int num_pages);
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
    init_mem_mng(l_mng_b, BYTE_MEM_MNG_MAX, 1, MEM_INFO_B);

    unsigned long size_B = BYTE_MEM_BYTES;
    int num_pages = BYTE_TO_PAGE(size_B);
    void *addr = mem_alloc_page(num_pages);
    mem_set_free(l_mng_b, addr, size_B);
}

static void init_mem_mng(MEM_MNG *mng, int max_free, int unit, int info_size_b)
{
    mng->free = (MEMORY *) (mng + 1);
    mng->max_free = max_free;
    mng->num_free = 0;
    mng->total_free = 0;
    mng->unit = unit;
    mng->info_size_b = info_size_b;
}


/**
 * @note 割り当てられたメモリの前に割り当てサイズが置かれる。
 *
 * @note 4KB以上確保するならページ単位メモリ管理のほうで管理させる。
 */
void *mem_alloc(unsigned int size_B)
{
    // TODO: align
 
    if (size_B == 0) {
        return 0;  // TODO: debug 出力
    }

    if (size_B >= PAGE_SIZE_B) {
        return mem_alloc_page(BYTE_TO_PAGE(size_B));
    }

    return get_free_addr(l_mng_b, size_B);  // FIXME: 0ならmem_alloc_pageを使う
}


int mem_free(void *vp_vaddr)
{
    if (vp_vaddr == 0) {
        return -1;
    }

    unsigned int vaddr = (unsigned int) vp_vaddr;

    if (IS_4KB_ALIGN(vaddr)) {
        // アドレスが4KB境界なのでページ単位メモリ管理のほうのメモリ。
        // バイト単位メモリ管理ではアドレスの前にサイズを置くので
        // 4KB境界になることはない。
        // FIXME: 上に書いたことはウソ。信じるな
        return page_free(vp_vaddr);
    }

    // 割り当てサイズの取得
    unsigned int size_B_vaddr = vaddr - MEM_INFO_B;
    unsigned int size_B = *((int *) size_B_vaddr);

    // 「ここで管理されてます」という印はあるか？
    if ((size_B & ~0xFFF) != MM_SIG) {
        return -1;
    }

    size_B &= 0xFFF;  // 印を取り除く
    size_B += MEM_INFO_B;

    return mem_set_free(l_mng_b, (void *) size_B_vaddr, size_B);
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

void mem_dbg(void)
{
    DBG_STR("DEBUG BYTE UNIT MEMORY MANAGE");
    dbg_mem_mng(l_mng_b);

    DBG_STR("DEBUG PAGE UNIT MEMORY MANAGE");
    dbg_mem_mng(l_mng_v);
}


//-----------------------------------------------------------------------------
// ページ単位メモリ管理


static int mem_alloc_page_sub(void *vp_vaddr, int num_pages);

void *mem_alloc_user(void *vp_vaddr, int size_B)
{
    int num_pages = BYTE_TO_PAGE(size_B);

    if (mem_alloc_page_sub(vp_vaddr, num_pages) < 0) {
        return 0;
    } else {
        return vp_vaddr;
    }
}


/**
 * @return リニアアドレス。空き容量が足りなければ 0 が返ってくる
 */
void *mem_alloc_page(unsigned int num_pages)
{
    if (num_pages == 0) {
        return 0;
    }

    void *vaddr = get_free_addr(l_mng_v, num_pages);

    /* 論理アドレスが足りない */
    if (vaddr == 0) {
        return 0;
    }

    if (mem_alloc_page_sub(vaddr, num_pages) < 0) {
        return 0;
    } else {
        return vaddr;
    }
}


static int mem_alloc_page_sub(void *vp_vaddr, int num_pages)
{
    unsigned long vaddr = (unsigned long) vp_vaddr;
    void *vp_maddr;
    bool first = true;

    for (int i = BITMAP_ST; i < l_bitmap_end; i++) {
        if (l_bitmap[i] != 0) {
            for (int j = 31; j >= 0; j--) {
                if (l_bitmap[i] & (1 << j)) {
                    vp_maddr = IDX2MADDR(i, j);
                    SET_USED_MADDR(vp_maddr);
                    l_mfree_B -= PAGE_SIZE_B;

                    int flg = PTE_RW | PTE_US | PTE_PRESENT;

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

                    paging_map2((void *) vaddr, vp_maddr, flg);
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

static int page_free_maddr(void *vp_vaddr);

static int page_free(void *vp_vaddr)
{
    int flg = get_page_flags(vp_vaddr);
    if ((flg & PTE_START) == 0) {
        return -2;
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
    flg = get_page_flags((void *) vaddr);

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
        flg = get_page_flags((void *) vaddr);
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


static void dbg_mem_mng(MEM_MNG *mng)
{
    int i;

    for (i = 0; i < mng->num_free; i++) {
        MEMORY *mem = &mng->free[i];

        dbg_int(i);
        dbg_str(" : addr = ");
        dbg_addr(mem->addr);

        dbg_str(", size = ");
        dbg_int((mem->size * mng->unit) / (1024));
        dbg_str("KB");

        dbg_newline();
    }

    dbg_newline();
}


static int mem_set_free(MEM_MNG *mng, void *vp_addr, unsigned int size)
{
    char *addr = (char *) vp_addr;

    // 4KB 境界でなかったらエラー
    if (size == 0 || ! IS_4KB_ALIGN((unsigned int) addr)) {
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
        char *prev_end_addr = (char *) prev->addr + prev->size * mng->unit;

        // 前とまとめれるか？
        if (prev_end_addr == addr) {
            prev->size += size;

            // 後ろがあるか？
            if (next != 0) {

                // 後ろとまとめれるか？
                if (end_addr == next->addr) {
                    prev->size += next->size;

                    mng->num_free--; // 空き１つ追加されて前後２つ削除された
                    mng->total_free += size;

                    for (; i < mng->num_free; i++) {
                        mng->free[i] = mng->free[i + 1];
                    }
                }
            }

            return 0;
        }
    }

    // 前とはまとめれなかった

    // 後ろがあるか？
    if (next != 0) {

        // 後ろとまとめれるか？
        if (end_addr == next->addr) {
            next->addr = addr;
            next->size += size;

            // 空きが１つ追加されて１つ削除されたのでmng->num_freeは変わらない
            mng->total_free += size;
            return 0;
        }
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
        if (mng->free[i].size >= size + mng->info_size_b) {
            // 空きが見つかった

            MEMORY *mem = &mng->free[i];

            unsigned int addr = (unsigned int) mem->addr;

            if (mng->info_size_b > 0) {
                // 割り当てサイズの記録。
                // ここで管理していることを表す印も追加する。
                *((unsigned int *) addr) = MM_SIG | size;
                addr += mng->info_size_b;
            }

            // 空きアドレスをずらす
            mem->addr += (size * mng->unit + mng->info_size_b);
            mem->size -= (size + mng->info_size_b);
            mng->total_free -= size + mng->info_size_b;

            if (mem->size <= mng->info_size_b) {
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

/// 空き容量を取得
unsigned int mem_total_vfree_B(void)
{
    return l_mng_v->total_free * l_mng_v->unit;
}

