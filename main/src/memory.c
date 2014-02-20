/**
 * メモリ管理
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_MEMORY
#define HEADER_MEMORY

//-----------------------------------------------------------------------------
// メモリマップ

#define SZ4MB  4 * 1024 * 1024  /* 4MB のサイズ */

#define VADDR_BASE        (0xC0000000)  /* 論理アドレスのベースアドレス */

#define ADDR_SYS_INFO     (0x00000A00)  /* システム情報が格納されているアドレス */
/* FREE START             (0x00001000) */
#define ADDR_OS_PDT       (0x00001000)
#define VADDR_MAP_START   (0x00010000)  /* 物理アドレスを管理するためのビットマップ */
#define VADDR_MAP_END     (0x00030000)
#define VADDR_BMEM_MNG    (VADDR_BASE + VADDR_MAP_END)
#define VADDR_VMEM_MNG    (VADDR_BMEM_MNG + 0x10000)
/* FREE END               (0x0009FFFF) */
#define ADDR_DISK_IMG     (0x00100000)
#define ADDR_IDT          (0x0026F800)
#define LIMIT_IDT         (0x000007FF)
#define ADDR_GDT          (0x00270000)
#define LIMIT_GDT         (0x0000FFFF)
#define ADDR_OS           (0x00280000)  /* 最大 1,152 KB */
#define MADDR_FREE_START  (0x00400000)
#define VADDR_VRAM        (0xE0000000)

#define VADDR_MEM_START   (VADDR_BASE + SZ4MB * 2)
#define VADDR_MEM_END     (VADDR_VRAM)


//-----------------------------------------------------------------------------
// バイト単位メモリ管理

void  mem_init(void);
unsigned int mem_total_free(void);
void *mem_alloc(unsigned int size_B);
int   mem_free(void *vp_vaddr);

void mem_dbg(void);


//-----------------------------------------------------------------------------
// ページ単位メモリ管理

void *mem_alloc_page(unsigned int num_pages);
int page_free(void *maddr);


//-----------------------------------------------------------------------------
// メモリ容量確認

unsigned int mem_total_B(void);

#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "debug.h"
#include "paging.h"
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
#define MEM_INFO_B        (4)                /* メモリの先頭に置く管理情報のサイズ */
#define BYTE_MEM_BYTES    (1 * 1024 * 1024)  /* バイト管理する容量 */
#define MM_SIG            (0xBAD41000)       /* メモリの先頭に置くシグネチャ */

static MEM_MNG *l_mng_b = (MEM_MNG *) VADDR_BMEM_MNG;


//-----------------------------------------------------------------------------
// ページ単位メモリ管理


#define PAGE_MEM_MNG_MAX   (4096)

static MEM_MNG *l_mng = (MEM_MNG *) VADDR_VMEM_MNG;

static void *get_free_maddr(unsigned int num_pages);


//=============================================================================
// 公開関数

//-----------------------------------------------------------------------------
// バイト単位メモリ管理

void mem_init(void)
{
    int num_pages;
    unsigned long size_B;
    void *addr;

    init_mem_mng(l_mng, PAGE_MEM_MNG_MAX, PAGE_SIZE_B, 0);

    // ---- 空きメモリをページ単位メモリ管理に登録

    size_B = g_sys_info->mem_total_B - MADDR_FREE_START;
    num_pages = BYTE_TO_PAGE(size_B);
    mem_set_free(l_mng, (void *) MADDR_FREE_START, num_pages);


    // ---- バイト単位メモリ管理の初期化

    init_mem_mng(l_mng_b, BYTE_MEM_MNG_MAX, 1, MEM_INFO_B);

    // バイト単位でメモリ管理する
    size_B = BYTE_MEM_BYTES;
    num_pages = BYTE_TO_PAGE(size_B);
    addr = mem_alloc_page(num_pages);
    mem_set_free(l_mng_b, addr, size_B);
}

static unsigned int total_free_pages(void);

/// 空き容量を取得
unsigned int mem_total_free(void)
{
    return total_free_pages() * PAGE_SIZE_B;
}

static void *mem_alloc_byte(unsigned int size_B);

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


void mem_dbg(void)
{
    DBG_STR("DEBUG BYTE UNIT MEMORY MANAGE");
    dbg_mem_mng(l_mng_b);

    DBG_STR("DEBUG PAGE UNIT MEMORY MANAGE");
    dbg_mem_mng(l_mng);
}


//-----------------------------------------------------------------------------
// ページ単位メモリ管理



void *alloc_os_page_vaddr(unsigned int num_pages)
{
    // TODO
    return 0;
}


void *alloc_user_page_vaddr(unsigned int num_pages)
{
    // TODO
    return 0;
}


/**
 * @return リニアアドレス。空き容量が足りなければ 0 が返ってくる
 */
void *mem_alloc_page(unsigned int num_pages)
{
    void *maddr;

    if (num_pages == 0) {
        return 0;
    }

    maddr = get_free_addr(l_mng, num_pages);

    if (maddr == 0) {
        return 0;
    }

    return maddr;
}


int page_free(void *vp_vaddr)
{
    /*TODO
    PDE *pd = get_pd();
    void *vp_maddr = vaddr2maddr(pd, vp_vaddr);
    */
    void *vp_maddr = vp_vaddr;

    if (vp_maddr == 0) {
        return -1;
    }

    // 割り当てサイズの取得
    unsigned int num_pages = 1; // TODO : どうにかして得る

    return mem_set_free(l_mng, vp_maddr, num_pages);
}

//-----------------------------------------------------------------------------
// メモリ容量確認

unsigned int mem_total_B(void)
{
    return g_sys_info->mem_total_B;
}


//=============================================================================
// 非公開関数

static void init_mem_mng(MEM_MNG *mng, int max_free, int unit, int info_size_b)
{
    mng->free = (MEMORY *) (mng + 1);
    mng->max_free = max_free;
    mng->num_free = 0;
    mng->total_free = 0;
    mng->unit = unit;
    mng->info_size_b = info_size_b;
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
        dbg_int(mem->size);

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
            l_mng->total_free += size;
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
// ページ単位メモリ管理


static unsigned int total_free_pages(void)
{
    return l_mng->total_free;
}

