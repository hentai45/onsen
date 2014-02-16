/**
 * メモリ管理
 *
 * @file memory.c
 * @author Ivan Ivanovich Ivanov
 */

/*
 * ＜目次＞
 * ・バイト単位メモリ管理
 * ・メモリ容量確認
 */

// TODO : サイズの前にシグネチャを書き込む

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_MEMORY
#define HEADER_MEMORY

//-----------------------------------------------------------------------------
// メモリマップ

#define ADDR_VRAM        0x00000FF0  ///< VRAMのアドレスが格納されているアドレス

#define ADDR_DISK_IMG    0x00100000

#define ADDR_IDT         0x0026F800
#define LIMIT_IDT        0x000007FF

#define ADDR_GDT         0x00270000
#define LIMIT_GDT        0x0000FFFF

#define MADDR_PAGE_MEM_MNG 0x003C0000

#define ADDR_MEMORY_MNG  0x003C0000

#define ADDR_FREE1_START 0x00001000
#define ADDR_FREE1_SIZE  0x0009E000

#define ADDR_FREE2_START 0x00400000

#define ADDR_MEMTEST_START ADDR_FREE2_START
#define ADDR_MEMTEST_END 0xBFFFFFFF


//-----------------------------------------------------------------------------
// バイト単位メモリ管理

void  mem_init(void);
unsigned int mem_total_free(void);
void *mem_alloc(unsigned int size_B);
int   mem_free(void *vp_vaddr);

void mem_dbg(void);

//-----------------------------------------------------------------------------
// メモリ容量確認

unsigned int mem_total_size_B(void);

#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "debug.h"
#include "paging.h"
#include "str.h"

//-----------------------------------------------------------------------------
// バイト単位メモリ管理

#define MEMORY_MNG_MAX     4096
#define MEM_INFO_B  4

#define MM_SIG 0xBAD41000

typedef struct MEMORY {
    unsigned int vaddr;
    unsigned int size_B;
} MEMORY;


typedef struct MEMORY_MNG {
    /// 記憶領域の確保用。
    /// free.vaddrの昇順でなければならない
    MEMORY *free;

    int max_free;
    int num_free;   ///< 空き情報の個数
} MEMORY_MNG;


static MEMORY_MNG mng;

static int mem_set_free(void *vp_vaddr, unsigned int size_B);


//-----------------------------------------------------------------------------
// メモリ容量確認

#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000


static unsigned int s_memtotal;


static unsigned int memtest(unsigned int start, unsigned int end);
static unsigned int memtest_sub(unsigned int start, unsigned int end);
static char is486(void);
static void enable_cache(void);
static void disable_cache(void);


//=============================================================================
// 公開関数

//-----------------------------------------------------------------------------
// バイト単位メモリ管理

void mem_init(void)
{
    page_mem_init();

    // ---- 空きメモリをページ単位メモリ管理に登録

    page_set_free((void *) ADDR_FREE1_START, BYTE_TO_PAGE(ADDR_FREE1_SIZE));

    s_memtotal = memtest(ADDR_MEMTEST_START, ADDR_MEMTEST_END);
    unsigned int size_B = s_memtotal - ADDR_FREE2_START;
    page_set_free((void *) ADDR_FREE2_START, BYTE_TO_PAGE(size_B));


    // ---- バイト単位メモリ管理の初期化

    // MEMORY_MNG の初期化
    size_B = MEMORY_MNG_MAX * sizeof (MEMORY);
    mng.free = (MEMORY *) page_alloc(BYTE_TO_PAGE(size_B));
    mng.max_free = MEMORY_MNG_MAX;
    mng.num_free = 0;

    // 1MB をバイト単位でメモリ管理する
    size_B = 1 * 1024 * 1024; // 1MB
    void *vaddr = page_alloc(BYTE_TO_PAGE(size_B));
    mem_set_free(vaddr, size_B);
}


/// 空き容量を取得
unsigned int mem_total_free(void)
{
    return total_free_page() * PAGE_SIZE_B;
}


/**
 * @note 割り当てられたメモリの前に割り当てサイズが置かれる。
 *
 * @note 4KB以上確保するならページ単位メモリ管理のほうで管理させる。
 */
void *mem_alloc(unsigned int size_B)
{
    if (size_B == 0) {
        return 0;
    }

    if (size_B >= PAGE_SIZE_B) {
        return page_alloc(BYTE_TO_PAGE(size_B));
    }

    for (int i = 0; i < mng.num_free; i++) {
        if (mng.free[i].size_B >= size_B + MEM_INFO_B) {
            // 空きが見つかった

            MEMORY *mem = &mng.free[i];

            unsigned int vaddr = mem->vaddr;

            // 割り当てサイズの記録。
            // ここで管理していることを表す印も追加する。
            *((unsigned int *) vaddr) = MM_SIG | size_B;
            vaddr += MEM_INFO_B;

            // 空きアドレスをずらす
            mem->vaddr += (size_B + MEM_INFO_B);
            mem->size_B -= (size_B + MEM_INFO_B);

            if (mem->size_B <= MEM_INFO_B) {
                // 空いた隙間をずらす
                mng.num_free--;
                for ( ; i < mng.num_free; i++) {
                    mng.free[i] = mng.free[i + 1];
                }
            }

            return (void *) vaddr;
        }
    }

    return 0;
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

    return mem_set_free((void *) size_B_vaddr, size_B);
}


void mem_dbg(void)
{
    char s[32];

    DBG_STR("DEBUG BYTE UNIT MEMORY MANAGE");

    for (int i = 0; i < mng.num_free; i++) {
        MEMORY *mem = &mng.free[i];

        dbg_int(i);

        dbg_str(" : addr = ");
        dbg_intx(mem->vaddr);

        dbg_str(", size = ");
        s_size(mem->size_B, s);
        dbg_strln(s);
    }

    dbg_newline();
}


//-----------------------------------------------------------------------------
// メモリ容量確認

unsigned int mem_total_size_B(void)
{
    return s_memtotal;
}


//=============================================================================
// 非公開関数

//-----------------------------------------------------------------------------
// バイト単位メモリ管理

static int mem_set_free(void *vp_vaddr, unsigned int size_B)
{
    unsigned int vaddr = (unsigned int) vp_vaddr;

    // ---- 管理している空きメモリとまとめれるならまとめる

    int i;
    MEMORY *next = 0;

    // 挿入位置または結合位置を検索
    for (i = 0; i < mng.num_free; i++) {
        if (mng.free[i].vaddr > vaddr) {
            next = &mng.free[i];
            break;
        }
    }

    // うしろに空きがなければ末尾に追加
    if (next == 0) {
        if (mng.num_free >= mng.max_free) {
            // TODO : 自動で増やす
            return -1;
        }

        mng.free[i].vaddr = vaddr;
        mng.free[i].size_B = size_B;

        mng.num_free++;  // 空きが１つ追加

        return 0;
    }

    unsigned int end_vaddr = vaddr + size_B;

    // 前があるか？
    if (i > 0) {
        MEMORY *prev = &mng.free[i - 1];
        unsigned int prev_end_vaddr = prev->vaddr + prev->size_B;

        // 前とまとめれるか？
        if (prev_end_vaddr == vaddr) {
            prev->size_B += size_B;

            // 後ろがあるか？
            if (next != 0) {

                // 後ろとまとめれるか？
                if (end_vaddr == next->vaddr) {
                    prev->size_B += next->size_B;

                    mng.num_free--;  // 空き１つ追加されて前後２つ削除された

                    for (; i < mng.num_free; i++) {
                        mng.free[i] = mng.free[i + 1];
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
        if (end_vaddr == next->vaddr) {
            next->vaddr = vaddr;
            next->size_B += size_B;

            // 空きが１つ追加されて１つ削除されたのでmng->num_freeは変わらない
            return 0;
        }
    }

    // 前にも後ろにもまとめれなかった

    if (mng.num_free < mng.max_free) {
        // free[i]より後ろを、後ろへずらして、すきまをつくる
        for (int j = mng.num_free; j > i; j--) {
            mng.free[j] = mng.free[j - 1];
        }

        next->vaddr = vaddr;
        next->size_B = size_B;

        mng.num_free++;  // 空きが１つ追加
        return 0;
    }

    // 失敗

    return -1;
}


//-----------------------------------------------------------------------------
// メモリ容量確認

/// @return 末尾の有効なアドレス
static unsigned int memtest(unsigned int start, unsigned int end)
{
    char flag486 = is486();

    if (flag486 != 0) {
        disable_cache();
    }

    unsigned int i = memtest_sub(start, end);

    if (flag486 != 0) {
        enable_cache();
    }

    return i;
}


static unsigned int memtest_sub(unsigned int start, unsigned int end)
{
    unsigned int i, old, pattern0 = 0xAA55AA55, pattern1 = 0x55AA55AA;
    volatile unsigned int *p;

    // 4KBごとにチェック
    for (i = start; i <= end; i += 0x1000) {
        p = (unsigned int *) (i + 0xFFC);
        old = *p;
        *p = pattern0;

        *p ^= 0xFFFFFFFF;  // 反転
        if (*p != pattern1) {
            // メモリでない
            *p = old;
            break;
        }

        *p ^= 0xFFFFFFFF;  // 反転して元に戻す
        if (*p != pattern0) {
            // メモリでない
            *p = old;
            break;
        }

        *p = old;
    }

    return i;
}


/// 386 では AC-bit を 1 にしても自動で 0 に戻ることを利用して
/// 486 であるかどうかを確認する
static char is486(void)
{
    unsigned int eflg = load_eflags();
    eflg |= EFLAGS_AC_BIT;  // AC-bit = 1
    store_eflags(eflg);

    char flag486 = 0;
    if ((eflg & EFLAGS_AC_BIT) != 0) {
        flag486 = 1;
    }
    eflg &= ~EFLAGS_AC_BIT;  // AC-bit = 0
    store_eflags(eflg);

    return flag486;
}


static void enable_cache(void)
{
    unsigned int cr0 = load_cr0();
    cr0 &= ~CR0_CACHE_DISABLE;  // キャッシュ許可
    store_cr0(cr0);
}


static void disable_cache(void)
{
    unsigned int cr0 = load_cr0();
    cr0 |= CR0_CACHE_DISABLE;  // キャッシュ禁止
    store_cr0(cr0);
}


