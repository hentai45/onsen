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

#define PAGE_SIZE_B (4096)  ///< ページサイズ

#define IS_4KB_ALIGN(byte) (((byte) & 0xFFF) == 0)  ///< 4KB 境界であるか確認
#define CEIL_4KB(byte)  (((byte) + 0xFFF) & ~0xFFF) ///< 4KB 単位で切り上げ
#define FLOOR_4KB(byte) ((byte) & ~0xFFF)           ///< 4KB 単位で切り捨て

#define BYTE_TO_PAGE(byte) ((CEIL_4KB(byte)) >> 12)


void paging_init(void);
unsigned long *get_os_pd(void);

void paging_dbg(void);

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

#define NUM_PDE     (1024)  /* １つの PD 内の PDE の数 */
#define NUM_PTE     (1024)  /* １つの PT 内の PTE の数 */

#define PTE_PRESENT (0x01)  /* 1ならページがメモリ上に存在する */
#define PTE_RW      (0x02)  /* 0なら特権レベル3では書き込めない */
#define PTE_US      (0x04)  /* 0なら特権レベル3ではアクセスできない */
#define PTE_ACCESS  (0x20)  /* このエントリのページをアクセスするとCPUが1にする */
#define PTE_DIRTY   (0x40)  /* このエントリのページに書き込むとCPUが1にする */
#define PTE_4MB     (0x80)  /* 4MBページ */


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


static PDE *l_os_pd = (PDE *) ADDR_OS_PDT;


static void create_os_pd(void);


//=============================================================================
// 公開関数

//-----------------------------------------------------------------------------
// ページング

static void disable_8MB_page(void);

void paging_init(void)
{
    //disable_8MB_page();
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
    dbg_addr(pd);
    dbg_newline();

    for (int i_pd = 0; i_pd < NUM_PDE; i_pd++) {
        if (pd[i_pd] & PTE_4MB) {
            dbg_str("    4MB Page = 0x");
            dbg_intx(i_pd * (4 * 1024 * 1024));
            dbg_str(" (");
            dbg_int(i_pd);
            dbg_str(")");
            dbg_newline();
            continue;
        }

        PTE *pt = get_pt(pd, i_pd);

        if (pt == 0) {
            continue;
        }

        dbg_str("    PD[");
        dbg_int(i_pd);
        dbg_str("] = ");
        dbg_addr(pt);

        int num_pages = 0;

        dbg_str("        ");
        for (int i_pt = 0; i_pt < NUM_PTE; i_pt++) {
            PTE pte = pt[i_pt];

            if (i_pt < 3) {
                if (i_pt != 0) {
                    dbg_str(", ");
                }

                dbg_str("PT[");
                dbg_int(i_pt);
                dbg_str("] = 0x");
                dbg_intx(pte);
            }

            if (pte != 0) {
                num_pages++;
            }
        }

        dbg_str(",   page = ");
        dbg_intln(num_pages);
    }

    dbg_str("\n");
}

//=============================================================================
// 非公開関数

//-----------------------------------------------------------------------------
// ページング


/**
 * 論理アドレスの先頭8MBを無効にする
 */
static void disable_8MB_page(void)
{
    l_os_pd[0] = 0;
    l_os_pd[1] = 0;
    flush_tlb();
}


static void create_os_pd(void)
{
    // ---- バイト単位メモリ管理の作成

    /*
    void *mem_mng = page_alloc(4);
    mem_init_mng(mem_mng, 4 * PAGE_SIZE_B);
    int *vaddr = 0; //TODO
    set_pte(pd, vaddr, mem_mng, PTE_RW | PTE_US | PTE_PRESENT);
    */
}

