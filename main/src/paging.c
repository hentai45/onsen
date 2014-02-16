/**
 * ページング
 *
 * @note
 * @par 用語定義
 * - ページ : 4KB 単位のメモリ
 * - 論理アドレス : セグメントアドレスとオフセットアドレスの組
 * - リニアアドレス : セグメント機構により論理アドレスから変換されたアドレス。
 *                    プログラム中では名前に vaddr を使用する。
 * - 物理アドレス : 物理的なメモリのアドレス。
 *                  プログラム中では名前に maddr を使用する。
 * - PD : ページディレクトリ。
 *        ページテーブルのアドレスを取得するためのテーブル。
 * - PDE : PD のエントリ。上位20ビットは１つのページテーブルのアドレスを表す。
 * - PT : ページテーブル。
 *        ページのアドレスを取得するためのテーブル。
 * - PTE : PT のエントリ。上位20ビットは１つのページのアドレスを表す。
 *
 * @par ページングとは
 * ページ単位を管理する仕組み。
 * 仮想記憶などで使われる。
 *
 * @par 論理アドレスが物理アドレスに変換されるまでの流れ
 * -# セグメント機構によって論理アドレスからリニアアドレスに変換される。
 * -# ページング機構によってリニアアドレスから物理アドレスに変換される。
 *
 * @par OnSen OS でのセグメント
 * OnSen OS では 「論理アドレス = リニアアドレス」 になるようにセグメントを
 * 設定している。これは Linux と同じ。ページングは有効・無効を設定できるが、
 * セグメントはオフにできない。だから、最低限の設定をしている(gdt.c 参照)。
 *
 * @par リニアアドレス（32ビット）の構成
 * - PD インデックス（上位 10 ビット）
 * - PT インデックス（10 ビット）
 * - ページ内オフセット（下位 12 ビット）
 *
 * @par リニアアドレスを物理アドレスに変換する仕組み
 * -# CR3 レジスタから PD アドレスを取得する。
 * -# PD アドレス + (PD インデックス * 4) = PT アドレス
 * -# PT アドレス + (PT インデックス * 4) = ページアドレス
 * -# ページアドレス + ページ内オフセット = 物理アドレス
 *
 * @par 各タスクでのメモリマップ（アドレスはリニアアドレス）
 * - 0x00000000 - 0xBFFFFFFF : ユーザ空間
 * - 0xC0000000 - 0xFFFFFFFF : カーネル空間（特権レベル0）
 *
 * @file paging.c
 * @author Ivan Ivanovich Ivanov
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_PAGING
#define HEADER_PAGING


//-----------------------------------------------------------------------------
// ページング

#define PAGE_SIZE_B 4096  ///< ページサイズ

#define IS_4KB_ALIGN(byte) (((byte) & 0xFFF) == 0)  ///< 4KB 境界であるか確認
#define CEIL_4KB(byte)  (((byte) + 0xFFF) & ~0xFFF) ///< 4KB 単位で切り上げ
#define FLOOR_4KB(byte) ((byte) & ~0xFFF)           ///< 4KB 単位で切り捨て

#define BYTE_TO_PAGE(byte) ((CEIL_4KB(byte)) >> 12)


void paging_init(void);
unsigned long *get_os_pd(void);

void paging_dbg(void);


//-----------------------------------------------------------------------------
// ページ単位メモリ管理

void page_mem_init(void);
unsigned int total_free_page(void);
void *page_alloc(unsigned int num_page);
int page_free(void *maddr);
int page_set_free(void *vp_vaddr, unsigned int num_page);
void *get_mem_mng(void);

void page_mem_dbg();

#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "debug.h"
#include "graphic.h"
#include "memory.h"
#include "str.h"


//-----------------------------------------------------------------------------
// ページング

#define NUM_PDE     1024  ///< １つの PD 内の PDE の数
#define NUM_PTE     1024  ///< １つの PT 内の PTE の数

#define PTE_PRESENT 0x01  ///< 1ならページがメモリ上に存在する
#define PTE_RW      0x02  ///< 0なら特権レベル3では書き込めない
#define PTE_US      0x04  ///< 0なら特権レベル3ではアクセスできない
#define PTE_ACCESS  0x20  ///< このエントリのページをアクセスするとCPUが1にする
#define PTE_DIRTY   0x40  ///< このエントリのページに書き込むとCPUが1にする


#define VADDR_TO_PD_INDEX(vaddr)  (((unsigned long) vaddr) >> 22)
#define VADDR_TO_PT_INDEX(vaddr)  ((((unsigned long) vaddr) >> 12) & 0x3FF)
#define PTE_TO_MADDR(pte) ((void *) ((pte) & ~0x000))


typedef unsigned long PDE;
typedef unsigned long PTE;


inline __attribute__ ((always_inline))
static PDE *get_pd(void)
{
    return (PDE *) (load_cr3() & ~0xFFF);
}


inline __attribute__ ((always_inline))
static PTE *get_pt(PDE *pd, int i_pd)
{
    return (PTE *) (pd[i_pd] & ~0xFFF);
}


inline __attribute__ ((always_inline))
static PTE *get_pte_maddr(PDE *pd, void *vp_vaddr)
{
    int i_pd = VADDR_TO_PD_INDEX(vp_vaddr);
    PTE *pt = get_pt(pd, i_pd);

    if (pt == 0) {
        return 0;
    }

    int i_pt  = VADDR_TO_PT_INDEX(vp_vaddr);
    return &pt[i_pt];
}


inline __attribute__ ((always_inline))
static PTE make_pte(void *vp_maddr, unsigned long flg)
{
    unsigned long maddr = (unsigned long) vp_maddr;

    return (maddr & ~0xFFF) | flg;
}


/**
 * ページの物理アドレスをリニアアドレスに対応づける。
 * PT がなければ自動でつくる
 */
inline __attribute__ ((always_inline))
static void map_page(PDE *pd, void *vp_vaddr, void *vp_maddr, int flg)
{
    PTE *pte = get_pte_maddr(pd, vp_vaddr);

    if (pte == 0) {
        // PT がなかったのでつくる

        PTE *pt = page_alloc(1);
        memset(pt, 0, PAGE_SIZE_B);

        int i_pd = VADDR_TO_PD_INDEX(vp_vaddr);
        pd[i_pd] = make_pte(pt, PTE_RW | PTE_US | PTE_PRESENT);

        int i_pt  = VADDR_TO_PT_INDEX(vp_vaddr);
        pte = &pt[i_pt];
    }

    *pte = make_pte(vp_maddr, flg);
}


inline __attribute__ ((always_inline))
static void *vaddr2maddr(PDE *pd, void *vp_vaddr)
{
    PTE *pte = get_pte_maddr(pd, vp_vaddr);

    if (pte == 0) {
        return 0;
    }

    return PTE_TO_MADDR(*pte);
}


static PDE *l_os_pd;


static unsigned long *create_os_pd(void);


//-----------------------------------------------------------------------------
// ページ単位メモリ管理

typedef struct PAGE_MEM {
    unsigned int maddr;
    unsigned int num_page;
} PAGE_MEM;


#define PAGE_MEM_MNG_MAX   4096

typedef struct PAGE_MEM_MNG {
    /// 記憶領域の確保用。
    /// free.maddrの昇順でなければならない
    PAGE_MEM free[PAGE_MEM_MNG_MAX];

    int num_free;       ///< 空き情報の数
    int num_free_page;  ///< 空きページの数
} PAGE_MEM_MNG;


PAGE_MEM_MNG *l_mng = (PAGE_MEM_MNG *) MADDR_PAGE_MEM_MNG;


static void *alloc_page_maddr(unsigned int num_page);


//=============================================================================
// 公開関数

//-----------------------------------------------------------------------------
// ページング

void paging_init(void)
{
    l_os_pd = create_os_pd();

    store_cr3((unsigned long) l_os_pd);

    enable_paging();
}


PDE *get_os_pd(void)
{
    return l_os_pd;
}


void paging_dbg(void)
{
    PDE *pd = get_pd();

    if (pd == 0) {
        return;
    }

    DBG_STR("DEBUG PAGING");

    dbg_str("PD : maddr = ");
    dbg_intxln((int) pd);

    for (int i_pd = 0; i_pd < NUM_PDE; i_pd++) {
        PTE *pt = get_pt(pd, i_pd);

        if (pt == 0) {
            continue;
        }

        dbg_str("    PD[");
        dbg_int(i_pd);
        dbg_str("] = ");
        dbg_intxln((int) pt);

        int num_page = 0;

        dbg_str("        ");
        for (int i_pt = 0; i_pt < NUM_PTE; i_pt++) {
            PTE pte = pt[i_pt];

            if (i_pt < 3) {
                if (i_pt != 0) {
                    dbg_str(", ");
                }

                dbg_str("PT[");
                dbg_int(i_pt);
                dbg_str("] = ");
                dbg_intx(pte);
            }

            if (pte != 0) {
                num_page++;
            }
        }

        dbg_str(",   page = ");
        dbg_intln(num_page);
    }

    dbg_str("\n");
}


//-----------------------------------------------------------------------------
// ページ単位メモリ管理

void page_mem_init(void)
{
    l_mng->num_free = 0;
    l_mng->num_free_page = 0;
}


unsigned int total_free_page(void)
{
    return l_mng->num_free_page;
}


void *alloc_os_page_vaddr(unsigned int num_page)
{
    return 0;
}


void *alloc_user_page_vaddr(unsigned int num_page)
{
    return 0;
}


/**
 * @return リニアアドレス。空き容量が足りなければ 0 が返ってくる
 */
void *page_alloc(unsigned int num_page)
{
    if (num_page == 0) {
        return 0;
    }

    return alloc_page_maddr(num_page);
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
    unsigned int num_page = 1; // TODO : どうにかして得る

    return page_set_free(vp_maddr, num_page);
}


/// OS 起動時に空いているメモリを設定するのに使う。
/// 通常のメモリ解放は、page_free のほうを使う。
int page_set_free(void *vp_maddr, unsigned int num_page)
{
    unsigned int maddr = (unsigned int) vp_maddr;

    // 4KB 境界でなかったらエラー
    if (num_page == 0 || ! IS_4KB_ALIGN(maddr)) {
        return -1;
    }

    // ---- 管理している空きメモリとまとめれるならまとめる

    int i;
    PAGE_MEM *next = 0;

    // 挿入位置または結合位置を検索
    for (i = 0; i < l_mng->num_free; i++) {
        if (l_mng->free[i].maddr > maddr) {
            next = &l_mng->free[i];
            break;
        }
    }

    // うしろに空きがなければ末尾に追加
    if (next == 0) {
        if (l_mng->num_free >= PAGE_MEM_MNG_MAX) {
            return -1;
        }

        l_mng->free[i].maddr = maddr;
        l_mng->free[i].num_page = num_page;

        l_mng->num_free++;  // 空きが１つ追加
        l_mng->num_free_page += num_page;

        return 0;
    }

    unsigned int end_maddr = maddr + (num_page * PAGE_SIZE_B);

    // 前があるか？
    if (i > 0) {
        PAGE_MEM *prev = &l_mng->free[i - 1];
        unsigned int prev_end_maddr = prev->maddr + prev->num_page*PAGE_SIZE_B;

        // 前とまとめれるか？
        if (prev_end_maddr == maddr) {
            prev->num_page += num_page;

            // 後ろがあるか？
            if (next != 0) {

                // 後ろとまとめれるか？
                if (end_maddr == next->maddr) {
                    prev->num_page += next->num_page;

                    l_mng->num_free--; // 空き１つ追加されて前後２つ削除された
                    l_mng->num_free_page += num_page;

                    for (; i < l_mng->num_free; i++) {
                        l_mng->free[i] = l_mng->free[i + 1];
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
        if (end_maddr == next->maddr) {
            next->maddr = maddr;
            next->num_page += num_page;

            // 空きが１つ追加されて１つ削除されたのでl_mng->num_freeは変わらない
            l_mng->num_free_page += num_page;
            return 0;
        }
    }

    // 前にも後ろにもまとめれなかった

    if (l_mng->num_free < PAGE_MEM_MNG_MAX) {
        // free[i]より後ろを、後ろへずらして、すきまをつくる
        for (int j = l_mng->num_free; j > i; j--) {
            l_mng->free[j] = l_mng->free[j - 1];
        }

        next->maddr = maddr;
        next->num_page = num_page;

        l_mng->num_free++;  // 空きが１つ追加
        l_mng->num_free_page += num_page;
        return 0;
    }


    // 失敗

    return -1;
}


void *get_mem_mng(void)
{
    return 0;
}


void page_mem_dbg(void)
{
    char s[32];

    DBG_STR("DEBUG PAGE UNIT MEMORY MANAGE");

    for (int i = 0; i < l_mng->num_free; i++) {
        PAGE_MEM *mem = &l_mng->free[i];

        dbg_int(i);
        dbg_str(" : addr = ");
        dbg_intx(mem->maddr);

        dbg_str(", page = ");
        dbg_int(mem->num_page);

        dbg_str(", size = ");
        s_size(mem->num_page * PAGE_SIZE_B, s);
        dbg_strln(s);
    }

    dbg_newline();
}


//=============================================================================
// 非公開関数

//-----------------------------------------------------------------------------
// ページング

static PDE *create_os_pd(void)
{
    // ---- ページディレクトリの作成

    PDE *pd = page_alloc(1);
    memset(pd, 0, PAGE_SIZE_B);


    // ---- カーネル空間の設定

    // FIXME
    // 最初の物理メモリ8MBをリニアアドレスにそのまま対応させる
    int flg = PTE_RW | PTE_US | PTE_PRESENT;
    char *max_p = (char *) (8 * 1024 * 1024);
    for (char *p = 0; p < max_p; p += PAGE_SIZE_B) {
        map_page(pd, p, p, flg);
    }

    // VRAM
    unsigned short *vram = (unsigned short *) *((int *) ADDR_VRAM);
    char *vaddr = (char *) vram;
    max_p = vaddr + (g_w * g_h * 2);

    for (char *p = (char *) vram; p < max_p; p += PAGE_SIZE_B) {
        map_page(pd, vaddr, p, flg);
        vaddr += PAGE_SIZE_B;
    }


    // ---- バイト単位メモリ管理の作成

    /*
    void *mem_mng = page_alloc(4);
    mem_init_mng(mem_mng, 4 * PAGE_SIZE_B);
    int *vaddr = 0; //TODO
    set_pte(pd, vaddr, mem_mng, PTE_RW | PTE_US | PTE_PRESENT);
    */

    return (PDE *) (((unsigned long) pd) | flg);
}


//-----------------------------------------------------------------------------
// ページ単位メモリ管理

/**
 * 空きページを割り当てる。ページは物理的に連続したメモリアドレスになっている。
 *
 * @return ページの物理アドレス。空きがなければ0。
 */
static void *alloc_page_maddr(unsigned int num_page)
{
    for (unsigned int i = 0; i < l_mng->num_free; i++) {
        if (l_mng->free[i].num_page >= num_page) {
            // 空きが見つかった

            PAGE_MEM *mem = &l_mng->free[i];

            unsigned int maddr = mem->maddr;

            // 空きアドレスを減らす
            mem->maddr += (num_page * PAGE_SIZE_B);
            mem->num_page -= num_page;
            l_mng->num_free_page -= num_page;

            if (mem->num_page <= 0) {
                // 空いた隙間をずらす
                l_mng->num_free--;
                for ( ; i < l_mng->num_free; i++) {
                    l_mng->free[i] = l_mng->free[i + 1];
                }
            }

            return (void *) maddr;
        }
    }

    return 0;
}


