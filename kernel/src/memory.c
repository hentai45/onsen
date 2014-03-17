/**
 * メモリ管理
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_MEMORY
#define HEADER_MEMORY

#include <stdint.h>
#include "sysinfo.h"

//-----------------------------------------------------------------------------
// メモリマップ

#define SZ4MB  4 * 1024 * 1024  // 4MB のサイズ

#define VADDR_BASE          (0xC0000000)  // 論理アドレスのベースアドレス

#define VADDR_USER_ESP      (0xBFFFF000)

/* FREE LOW ADDR START      (0x00000500) */
#define VADDR_MBR           (0xC0000500)
#define VADDR_PARTITION_TBL (0xC0000500 + 0x1BE)  // パーティションテーブル
/* OS_PDTはCR3レジスタに設定するので物理アドレスでなければいけない */
#define MADDR_OS_PDT        (0x00001000)  // 4KB(0x1000)境界であること
#define VADDR_OS_PDT        (0xC0001000)  // 4KB(0x1000)境界であること
#define VADDR_SYS_INFO      (0xC0002000)  // システム情報が格納されているアドレス
#define VADDR_MMAP_TBL      (0xC0003000)  // 使用可能メモリ情報のテーブル
#define VADDR_VBR           (0xC0007C00)
#define VADDR_BITMAP_START  (0xC0010000)  // 物理アドレス管理用ビットマップ
#define VADDR_BITMAP_END    (0xC0030000)
#define VADDR_GDT           (0xC0030000)
#define LIMIT_GDT           (0x0000FFFF)
#define VADDR_IDT           (0xC0040000)
#define LIMIT_IDT           (0x000007FF)
#define VADDR_BMEM_MNG      (0xC0050000)
#define VADDR_VMEM_MNG      (0xC0060000)
/* FREE LOW ADDR END        (0x0009FFFF) */

#define VADDR_DISK_IMG      (0xC0100000)
#define VADDR_OS_TEXT       (0xC0100000)
#define VADDR_OS_STACK      (0xC0200000)

// FREE 2MB

#define MADDR_FREE_START    (0x00400000)
#define VADDR_MEM_START     (0xC0400000)
#define VADDR_VRAM          (0xE0000000)
#define VADDR_MEM_END       (VADDR_VRAM)
#define VADDR_PD_SELF       (0xFFFFF000)


extern struct SYSTEM_INFO *g_sys_info;


//-----------------------------------------------------------------------------
// メモリ管理

struct USER_PAGE {
    unsigned long vaddr;
    unsigned long end_vaddr;  // end_vaddr自体は含まれない
    int refs;
};


void  mem_init(void);

void *mem_alloc(unsigned int size_B);
void *mem_alloc_str(const char *s);
struct USER_PAGE *mem_alloc_user_page(unsigned long vaddr, int size_B, int flags);
int   mem_expand_user_page(struct USER_PAGE *page, unsigned long new_end);
int   mem_expand_stack(struct USER_PAGE *stack, unsigned long new_stack);
void *mem_alloc_maddr(void);

int mem_free(void *vp_vaddr);
int mem_free_user(struct USER_PAGE *page);
int mem_free_maddr(void *vp_maddr);

void  mem_dbg(void);

//-----------------------------------------------------------------------------
// メモリ容量確認

struct MEMORY_MAP_ENTRY {
    uint32_t base_low;     // base address QWORD
    uint32_t base_high;
    uint32_t length_low;   // length QWORD
    uint32_t length_high;
    uint16_t type;         // entry Ttpe
    uint16_t acpi;         // exteded
} __attribute__ ((packed));


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
#include "str.h"
#include "sysinfo.h"


struct SYSTEM_INFO *g_sys_info = (struct SYSTEM_INFO *) VADDR_SYS_INFO;

//-----------------------------------------------------------------------------
// メモリ管理

struct MEMORY {
    void *addr;
    unsigned int size;
};


struct MEM_MNG {
    // 記憶領域の確保用。
    // free.addrの昇順でなければならない
    struct MEMORY *free;

    int max_free;    // freeの最大数
    int num_free;    // 空き情報の数
    int total_free;  // free.sizeの合計
    int unit;        // sizeの単位
    int info_size;   // 管理情報のサイズ
};


static int   mem_set_free(struct MEM_MNG *mng, void *vp_addr, unsigned int size);
static void  init_mem_mng(struct MEM_MNG *mng, int max_free, int unit, int info_size);
static void *get_free_addr(struct MEM_MNG *mng, unsigned int size);
static int   page_free_maddr(void *vp_vaddr);


//-----------------------------------------------------------------------------
// 8バイト単位メモリ管理（4096-8バイト以下のメモリ割り当てを管理する）

#define BYTE_MEM_MNG_MAX  4096
#define MEM_INFO_B        8                // メモリの先頭に置く管理情報のサイズ
#define BYTE_MEM_BYTES    (1 * 1024 * 1024)  // バイト管理する容量
#define MM_SIG            (0xBAD41BAD)       // メモリの先頭に置くシグネチャ
#define BYTES_TO_8BYTES(b)   (((b) + 7) >> 3)  // バイトを8バイト単位に変換する

struct INFO_8BYTES {
    unsigned long signature;
    unsigned long size;
};

static struct MEM_MNG *l_mng_b = (struct MEM_MNG *) VADDR_BMEM_MNG;


//-----------------------------------------------------------------------------
// ページ単位メモリ管理

#define MAX(x,y)  (((x) > (y)) ? (x) : (y))

#define PAGE_MEM_MNG_MAX   4096

#define SET_MNG_FLG  true
#define NO_MNG_FLG   false

/**
 * 物理アドレスからビットマップのインデックスに変換
 * 4096バイト(0x1000)単位で管理しているので12ビット右にシフト
 * さらに、4バイトで32管理できるので5ビット右にシフト
 * 合計17ビット右にシフトする。
 */
#define MADDR2IDX(maddr)  (((unsigned long) maddr) >> 17)

// 物理アドレスからビットマップのビット位置に変換
#define MADDR2BIT(maddr)  (1 << (31 - ((((unsigned long) maddr) >> 12) & 0x1F)))

// 物理アドレスからビットマップのビット番号に変換
#define MADDR2BIT_NO(maddr)  (31 - ((((unsigned long) maddr) >> 12) & 0x1F))

// インデックスとビット番号から物理アドレスに変換
#define IDX2MADDR(idx, bits)  ((void *) (((idx) << 17) | ((31 - (bits)) << 12)))

// 指定した物理アドレスが空いているなら0以外
#define IS_FREE_MADDR(maddr) (l_bitmap[MADDR2IDX(maddr)] & MADDR2BIT(maddr))

// 指定した物理アドレスが使用中なら0以外
#define IS_USED_MADDR(maddr) ( ! (l_bitmap[MADDR2IDX(maddr)] & MADDR2BIT(maddr)))

// 指定した物理アドレスを空きにする
#define SET_FREE_MADDR(maddr) (l_bitmap[MADDR2IDX(maddr)] |= MADDR2BIT(maddr))

// 指定した物理アドレスを使用中にする
#define SET_USED_MADDR(maddr) (l_bitmap[MADDR2IDX(maddr)] &= ~MADDR2BIT(maddr))


static void *mem_alloc_kernel_page(unsigned int num_pages, bool set_mng_flg);
static int page_free(void *vp_vaddr, bool is_only_maddr);


static struct MEM_MNG *l_mng_v = (struct MEM_MNG *) VADDR_VMEM_MNG;

// 物理アドレスを管理するためのビットマップ。使用中なら0、空きなら1
static unsigned long *l_bitmap = (unsigned long *) VADDR_BITMAP_START;

// ビットマップの最初のインデックス
static int l_bitmap_start_i;

// ビットマップの最後のインデックス
static int l_bitmap_end_i;

static unsigned long l_mfree_B;

static unsigned long l_end_free_maddr;


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
 * BIOSで得た情報をもとに空き領域を設定する。
 */
static void init_mpage_mem(void)
{
    /* ビットマップを使用中で初期化 */

    unsigned long size_B = VADDR_BITMAP_END - VADDR_BITMAP_START;
    memset(l_bitmap, 0, size_B);

    /* 空き領域を設定 */

    struct MEMORY_MAP_ENTRY *ent = (struct MEMORY_MAP_ENTRY *) VADDR_MMAP_TBL;

    unsigned long min_start = 0xFFFFFFFF;
    unsigned long max_end   = 0;

    l_mfree_B = 0;

    for (int i = 0; i < g_sys_info->mmap_entries; i++, ent++) {
        if (ent->type != 1)  // 使用可能じゃなければ飛ばす
            continue;

        if (ent->base_high != 0) {
            dbgf("ignore above 4GB");
            continue;
        }

        //dbgf("addr = %X, len = %X\n", ent->base_low, ent->length_low);

        unsigned long start = MAX(ent->base_low, MADDR_FREE_START);
        unsigned long end   = ent->base_low + ent->length_low;

        // 4KB単位で管理するための調整
        if (start & 0xFFF)
            start = (start + 0xFFF) & ~0xFFF;  // 切り上げ

        if (end & 0xFFF)
            end = (end - 0xFFF) & ~0xFFF;  // 切り下げ

        if (ent->length_high != 0) {
            end = 0xFFFFF000;
        }

        if (start >= end)
            continue;

        int i_start = MADDR2IDX(start);
        int i_end   = MADDR2IDX(end);

        int b_start = MADDR2BIT_NO(start);
        int b_end   = MADDR2BIT_NO(end);

        //dbgf("start %#X (%d,%d), end %#X (%d,%d)\n", start, i_start, b_start, end, i_end, b_end);

        if (start < min_start) {
            min_start = start;
        }

        if (max_end < end) {
            max_end = end;
        }

        // intに綺麗に収まらない部分のビットを設定しておく
        for (int b = b_start; b >= 0; b--) {
            unsigned long maddr = (unsigned long) IDX2MADDR(i_start, b);

            if (maddr > end)
                break;

            SET_FREE_MADDR(maddr);
        }

        i_start++;

        // intに綺麗に収まらない部分のビットを設定しておく
        for (int b = 31; b >= b_end; b--) {
            unsigned long maddr = (unsigned long) IDX2MADDR(i_end, b);

            if (maddr < start)
                continue;

            SET_FREE_MADDR(maddr);
        }

        i_end--;

        // 残りは一気にやっちゃいましょう
        size_B = (unsigned long) (&l_bitmap[i_end] - &l_bitmap[i_start]);
        size_B = (size_B + 1) * 4;  // int(4バイト)で管理しているので4倍する
        memset(&l_bitmap[i_start], 0xFF, size_B);

        l_mfree_B += end - start;
    }

    l_end_free_maddr = max_end;
    l_bitmap_start_i = MADDR2IDX(min_start);
    l_bitmap_end_i = MADDR2IDX(max_end);
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
static void init_mem_mng(struct MEM_MNG *mng, int max_free, int unit, int info_size)
{
    mng->free = (struct MEMORY *) (mng + 1);
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
    ASSERT(size_B, "attempted to allocate 0 bytes memory");

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
    ASSERT(vp_vaddr != 0, "attempted to free address 0");

    unsigned int vaddr = (unsigned int) vp_vaddr;

    ASSERT(VADDR_MEM_START <= vaddr && vaddr <= VADDR_MEM_END,
            "vaddr < VADDR_MEM_START || VADDR_MEM_END < vaddr: %p", vp_vaddr);

    // メモリ管理が返すメモリは、下位3ビットが0であるはず
    ASSERT((vaddr & 0x7) == 0, "(vaddr(%p) & 0x7) != 0", vp_vaddr);

    int flg = paging_get_flags(vp_vaddr);

    if (flg & 0xE00) {  // ページ単位メモリ管理が使うフラグが立っている
        /* ページ単位メモリ管理 */

        ASSERT(IS_4KB_ALIGN(vp_vaddr), "vaddr(%p) is not 4KB align", vp_vaddr);

        return page_free(vp_vaddr, false);
    }

    // ---- 8バイト単位メモリ管理

    // 割り当てサイズの取得
    struct INFO_8BYTES *info = (struct INFO_8BYTES *) (vaddr - MEM_INFO_B);

    // 「ここで管理されてます」という印はあるか？
    ASSERT(info->signature == MM_SIG, "not found the memory management signature");

    return mem_set_free(l_mng_b, (void *) info, info->size);
}


int mem_free_user(struct USER_PAGE *page)
{
    ASSERT(page, "");
    ASSERT(page->vaddr, "");
    ASSERT(page->vaddr < VADDR_BASE, "attempted to free kernel address");

    if (page->refs > 1) {
        page->refs--;
        return 0;
    }

    ASSERT(page->refs == 1, "invalid page refs");

    int ret = page_free((void *) page->vaddr, true);

    mem_free(page);

    return ret;
}


/**
 * mem_alloc_maddrで取得した物理アドレスを解放する
 */
int mem_free_maddr(void *vp_maddr)
{
    unsigned long maddr = (unsigned long) vp_maddr;
    ASSERT(maddr >= 0x1000, "maddr = %p", vp_maddr);
    ASSERT(maddr <= l_end_free_maddr, "maddr = %p", vp_maddr);
    ASSERT(IS_USED_MADDR(vp_maddr), "vp_maddr = %p", vp_maddr);

    SET_FREE_MADDR(vp_maddr & ~0xFFF);
    l_mfree_B += PAGE_SIZE_B;
    return 0;
}


int mem_expand_user_page(struct USER_PAGE *page, unsigned long new_end)
{
    ASSERT(page, "");

    if (page->end_vaddr > new_end)
        return 0;

    int size_B = new_end - page->end_vaddr;

    struct USER_PAGE *new_page = mem_alloc_user_page(page->end_vaddr, size_B, PTE_RW);

    ASSERT(page->end_vaddr == new_page->vaddr, "");

    // 古いページと新しいページを１つにまとめる

    void *last_page = (void *) (page->end_vaddr - 0x1000);
    int flg = paging_get_flags(last_page);
    ASSERT(flg & PTE_END, "not found end flag");
    flg &= ~PTE_END;
    if ((flg & PTE_START) == 0) {
        flg |= PTE_CONT;
    }
    paging_set_flags(last_page, flg);

    flg = paging_get_flags((void *) new_page->vaddr);
    ASSERT(flg & PTE_START, "not found start flag");
    flg &= ~PTE_START;
    if ((flg & PTE_END) == 0) {
        flg |= PTE_CONT;
    }
    paging_set_flags((void *) new_page->vaddr, flg);

    dbgf("expanded user page. %#X => %#X\n", page->end_vaddr, new_page->end_vaddr);

    // 更新
    page->end_vaddr = new_page->end_vaddr;

    return 0;
}


int mem_expand_stack(struct USER_PAGE *stack, unsigned long new_stack)
{
    ASSERT(stack, "");

    new_stack &= ~0xFFF;
    new_stack -= 0x1000;  // ちょっと余裕をもたせる

    ASSERT(stack->vaddr > new_stack, "");

    int size_B = stack->vaddr - new_stack;

    struct USER_PAGE *new_stack_page = mem_alloc_user_page(new_stack, size_B, PTE_RW);
    if (new_stack_page == 0) {
        ERROR("could not allocate pages");
        return -1;
    }

    ASSERT(new_stack == new_stack_page->vaddr, "");
    ASSERT(new_stack_page->end_vaddr == stack->vaddr, "");

    // 古いスタックと新しいスタックを１つにまとめる

    void *new_stack_last = (void *) (stack->vaddr - 0x1000);
    int flg = paging_get_flags(new_stack_last);
    ASSERT(flg & PTE_END, "not found end flag");
    flg &= ~PTE_END;
    if ((flg & PTE_START) == 0) {
        flg |= PTE_CONT;
    }
    paging_set_flags(new_stack_last, flg);

    flg = paging_get_flags((void *) stack->vaddr);
    ASSERT(flg & PTE_START, "not found start flag");
    flg &= ~PTE_START;
    if ((flg & PTE_END) == 0) {
        flg |= PTE_CONT;
    }
    paging_set_flags((void *) stack->vaddr, flg);

    dbgf("expanded stack. %#X => %#X\n", stack->vaddr, new_stack);

    // 更新
    stack->vaddr = new_stack;

    return 0;
}


/**
 * 1ページ分の物理アドレスを割り当てる
 */
void *mem_alloc_maddr(void)
{
    int i, j;
    void *maddr;
    for (i = l_bitmap_start_i; i < l_bitmap_end_i; i++) {
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
    ERROR("no remaining maddr");
    return 0;
}

// TODO
#include "ata/common.h"
#include "ata/cmd.h"

static void dbg_mem_mng(struct MEM_MNG *mng);

void mem_dbg(void)
{
    dbgf("\n");
    DBGF("DEBUG BYTE UNIT MEMORY MANAGE");
    dbg_mem_mng(l_mng_b);

    DBGF("DEBUG PAGE UNIT MEMORY MANAGE");
    dbg_mem_mng(l_mng_v);
    dbgf("\n");

    int buf[128];
    ata_init();
    ata_cmd_read_sectors(g_ata0, 1, buf, 1);
    dbgf("read: %X %X %X %X\n", buf[0], buf[1], buf[2], buf[3]);
}

static void dbg_mem_mng(struct MEM_MNG *mng)
{
    int i;

    for (i = 0; i < mng->num_free; i++) {
        struct MEMORY *mem = &mng->free[i];

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

struct USER_PAGE *mem_alloc_user_page(unsigned long vaddr, int size_B, int flags)
{
    struct USER_PAGE *page = (struct USER_PAGE *) mem_alloc(sizeof(struct USER_PAGE));
    page->refs = 1;

    int fraction = vaddr & 0xFFF;
    vaddr &= ~0xFFF;

    int num_pages = BYTE_TO_PAGE(size_B + fraction);

    if (mem_alloc_page_sub(vaddr, num_pages, flags | PTE_US, SET_MNG_FLG) < 0) {
        ERROR("could not allocate user pages: vaddr=%#X, size=%Z", vaddr, size_B);
        return 0;
    } else {
        page->vaddr = vaddr;
        page->end_vaddr = vaddr + (num_pages * PAGE_SIZE_B);

        return page;
    }
}


/**
 * @return リニアアドレス。空き容量が足りなければ 0 が返ってくる
 */
void *mem_alloc_kernel_page(unsigned int num_pages, bool set_mng_flg)
{
    ASSERT(num_pages, "attempted to allocate 0 pages");

    void *vaddr = get_free_addr(l_mng_v, num_pages);

    /* 論理アドレスが足りない */
    if (vaddr == 0) {
        ERROR("no remaining vaddr");
        return 0;
    }

    if (mem_alloc_page_sub((unsigned long) vaddr, num_pages, PTE_RW, set_mng_flg) < 0) {
        ERROR("could not allocate pages: vaddr=%#X, num pages=%d", vaddr, num_pages);
        return 0;
    } else {
        return vaddr;
    }
}


static int mem_alloc_page_sub(unsigned long vaddr, int num_pages, int flags, bool set_mng_flg)
{
    void *vp_maddr;
    bool first = true;

    for (int i = l_bitmap_start_i; i < l_bitmap_end_i; i++) {
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
    ERROR("no remaining maddr");
    return -1;
}


static int page_free(void *vp_vaddr, bool is_only_maddr)
{
    ASSERT(vp_vaddr, "vaddr == 0");

    ASSERT(IS_4KB_ALIGN(vp_vaddr), "vaddr(%p) is not 4KB align", vp_vaddr);

    int flg = paging_get_flags(vp_vaddr);
    ASSERT(flg & PTE_START, "not found a start flag");

    unsigned int num_pages = 1;

    if (page_free_maddr(vp_vaddr) < 0) {
        ERROR("could not free maddr");
        return -1;
    }

    if (flg & PTE_END) {
        if (is_only_maddr) {
            return 0;
        } else {
            return mem_set_free(l_mng_v, vp_vaddr, num_pages);
        }
    }

    unsigned long vaddr = (unsigned long) vp_vaddr;
    vaddr += PAGE_SIZE_B;
    num_pages++;
    flg = paging_get_flags((void *) vaddr);

    while ((flg & PTE_END) == 0) {
        ASSERT(flg & PTE_CONT, "not found continue flag");
        ASSERT((flg & PTE_START) == 0, "invalid start flag");

        if (page_free_maddr((void *) vaddr) < 0) {
            ERROR("could not free maddr");
            return -1;
        }

        vaddr += PAGE_SIZE_B;
        num_pages++;
        flg = paging_get_flags((void *) vaddr);
    }

    if (page_free_maddr((void *) vaddr) < 0) {
        ERROR("could not free maddr");
        return -1;
    }

    if (is_only_maddr) {
        return 0;
    } else {
        return mem_set_free(l_mng_v, vp_vaddr, num_pages);
    }
}


static int page_free_maddr(void *vp_vaddr)
{
    void *vp_maddr = paging_get_maddr(vp_vaddr);

    return mem_free_maddr(vp_maddr);
}


static int mem_set_free(struct MEM_MNG *mng, void *vp_addr, unsigned int size)
{
    char *addr = (char *) vp_addr;

    ASSERT(size, "");

    // ---- 管理している空きメモリとまとめれるならまとめる

    int i;
    struct MEMORY *next = 0;

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
        struct MEMORY *prev = &mng->free[i - 1];
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
static void *get_free_addr(struct MEM_MNG *mng, unsigned int size)
{
    if (mng->total_free < size) {
        return 0;
    }

    for (int i = 0; i < mng->num_free; i++) {
        if (mng->free[i].size >= size + mng->info_size) {
            // 空きが見つかった

            struct MEMORY *mem = &mng->free[i];

            unsigned int addr = (unsigned int) mem->addr;

            if (mng->info_size > 0) {
                // 割り当てサイズの記録。
                // ここで管理していることを表す印も追加する。
                struct INFO_8BYTES *info = (struct INFO_8BYTES *) addr;
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
    return l_end_free_maddr;
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

