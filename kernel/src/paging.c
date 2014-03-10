/**
 * ページング
 *
 * ## 用語定義
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
 * ## ページングとは
 * ページ単位を管理する仕組み。
 * 仮想記憶などで使われる。
 *
 * ## 論理アドレスが物理アドレスに変換されるまでの流れ
 * - セグメント機構によって論理アドレスからリニアアドレスに変換される。
 * - ページング機構によってリニアアドレスから物理アドレスに変換される。
 *
 * ## OnSen OS でのセグメント
 * OnSen OS では 「論理アドレス = リニアアドレス」 になるようにセグメントを
 * 設定している。これは Linux と同じ。ページングは有効・無効を設定できるが、
 * セグメントはオフにできない。だから、最低限の設定をしている(gdt.c 参照)。
 *
 * ## リニアアドレス（32ビット）の構成
 * - PD インデックス（上位 10 ビット）
 * - PT インデックス（10 ビット）
 * - ページ内オフセット（下位 12 ビット）
 *
 * ## リニアアドレスを物理アドレスに変換する仕組み
 * - CR3 レジスタから PD アドレスを取得する。
 * - PD アドレス + (PD インデックス * 4) = PT アドレス
 * - PT アドレス + (PT インデックス * 4) = ページアドレス
 * - ページアドレス + ページ内オフセット = 物理アドレス
 *
 * ## 各タスクでのメモリマップ（アドレスはリニアアドレス）
 * - 0x00000000 - 0xBFFFFFFF : ユーザ空間
 * - 0xC0000000 - 0xFFFFFFFF : カーネル空間（特権レベル0）
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_PAGING
#define HEADER_PAGING


//-----------------------------------------------------------------------------
// ページング

#define PAGE_SIZE_B  4096  // ページサイズ
#define CUR_PD       (VADDR_PD_SELF)

#define PTE_PRESENT  (0x001)  // 1ならページがメモリ上に存在する
#define PTE_RW       (0x002)  // 0なら特権レベル3では書き込めない
#define PTE_US       (0x004)  // 0なら特権レベル3ではアクセスできない
#define PTE_ACCESS   (0x020)  // このエントリのページをアクセスするとCPUが1にする
#define PTE_DIRTY    (0x040)  // このエントリのページに書き込むとCPUが1にする
#define PTE_4MB      (0x080)  // 4MBページ

/* メモリ管理用 */
#define PTE_START    (0x200)  // 連続領域開始。カスタムフラグ
#define PTE_CONT     (0x400)  // 連続領域続く。カスタムフラグ
#define PTE_END      (0x800)  // 連続領域終了。カスタムフラグ
#define PTE_MNG_MASK (0xE00)  // カスタムフラグを取得するためのマスク

#define IS_4KB_ALIGN(byte) (((unsigned long) (byte) & 0xFFF) == 0)  // 4KB 境界であるか確認
#define CEIL_4KB(byte)  (((byte) + 0xFFF) & ~0xFFF) // 4KB 単位で切り上げ
#define FLOOR_4KB(byte) ((byte) & ~0xFFF)           // 4KB 単位で切り捨て

#define BYTE_TO_PAGE(byte) ((CEIL_4KB(byte)) >> 12)

#define VADDR_TO_PD_INDEX(vaddr)  (((unsigned long) vaddr) >> 22)
#define VADDR_TO_PT_INDEX(vaddr)  ((((unsigned long) vaddr) >> 12) & 0x3FF)

#define BASE_PD_I  VADDR_TO_PD_INDEX(VADDR_BASE)


typedef unsigned long PDE;
typedef unsigned long PTE;

void paging_init(void);
void paging_map(void *vp_vaddr, void *vp_maddr, int flg);
void *paging_get_maddr(void *vp_vaddr);
PDE *create_user_pd(void);
PDE *copy_pd(void);
void paging_clear_pd_range(unsigned long start_vaddr, unsigned long end_vaddr);
int  paging_get_flags(void *vp_vaddr);
int  paging_set_flags(void *vp_vaddr, int flags);

void app_area_copy(PDE *pd);
void app_area_clear(void);

void paging_dbg(void);

#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "debug.h"
#include "graphic.h"
#include "memory.h"
#include "str.h"
#include "task.h"


//-----------------------------------------------------------------------------
// ページング

#define NUM_PDE     (1024)  // １つの PD 内の PDE の数
#define NUM_PTE     (1024)  // １つの PT 内の PTE の数

#define MAKE_PTE(maddr, flg)  (((unsigned long) (maddr) & ~0xFFF) | (flg & 0xFFF))

static PDE *get_pt(int i_pd);
static PTE *get_pte(void *vp_vaddr);


static PDE *l_pd = (PDE *) VADDR_PD_SELF;


//=============================================================================
// 関数

void paging_init(void)
{
    // 仮につくっておいた先頭のページングを無効にする
    l_pd[0] = 0;

    flush_tlb();
}


/**
 * ページの物理アドレスをリニアアドレスに対応づける。
 * PT がなければ自動でつくる
 */
void paging_map(void *vp_vaddr, void *vp_maddr, int flg)
{
    PTE *pte = get_pte(vp_vaddr);

    if (pte == 0) {
        // PT がなかったのでつくる

        PTE *pt_maddr = (PTE *) mem_alloc_maddr();

        int i_pd = VADDR_TO_PD_INDEX(vp_vaddr);

        int flags = PTE_RW | PTE_PRESENT;
        if (i_pd < BASE_PD_I)
            flags |= PTE_US;

        PDE pt = MAKE_PTE(pt_maddr, flags);
        l_pd[i_pd] = pt;

        /* OS領域なら全プロセスのPDに作成したPTを設定する */
        if (i_pd >= BASE_PD_I) {
            task_set_pt(i_pd, pt);
        }

        PTE *pt_vaddr = (PTE *) (0xFFC00000 | (i_pd << 12));

        memset(pt_vaddr, 0, PAGE_SIZE_B);

        int i_pt  = VADDR_TO_PT_INDEX(vp_vaddr);
        pte = &pt_vaddr[i_pt];
    }

    *pte = MAKE_PTE(vp_maddr, (flg | PTE_PRESENT));
}


int paging_get_flags(void *vp_vaddr)
{
    PTE *pte = get_pte(vp_vaddr);

    if (pte == 0) {
        return 0;
    }

    return *pte & 0xFFF;
}


int paging_set_flags(void *vp_vaddr, int flags)
{
    PTE *pte = get_pte(vp_vaddr);

    if (pte == 0) {
        return -1;
    }

    flags &= 0xFFF;

    *pte &= ~0xFFF;
    *pte |= flags;
    return 0;
}


void *paging_get_maddr(void *vp_vaddr)
{
    int i_pd = VADDR_TO_PD_INDEX(vp_vaddr);
    int i_pt = VADDR_TO_PT_INDEX(vp_vaddr);
    PTE *pt = (PTE *) (0xFFC00000 | (i_pd << 12));

    return (void *) ((pt[i_pt] & ~0xFFF) + ((unsigned long) vp_vaddr & 0xFFF));
}


// stack領域とdata領域が同じpdeを使わないことが前提
void paging_clear_pd_range(unsigned long start_vaddr, unsigned long end_vaddr)
{
    if (start_vaddr > end_vaddr)
        return;

    int i_start_pd = VADDR_TO_PD_INDEX(start_vaddr);
    int i_end_pd   = VADDR_TO_PD_INDEX(end_vaddr);

    for (int i_pd = i_start_pd; i_pd <= i_end_pd; i_pd++) {
        l_pd[i_pd] = 0;
    }

    flush_tlb();
}


PDE *create_user_pd(void)
{
    PDE *pd = mem_alloc(PAGE_SIZE_B);

    memcpy(pd, l_pd, PAGE_SIZE_B);

    app_area_clear();

    // 自分自身を参照できるようにする
    int i_pd = VADDR_TO_PD_INDEX(VADDR_PD_SELF);
    void *p = paging_get_maddr(pd);
    pd[i_pd] = ((unsigned long) p & ~0xFFF) | (PTE_RW | PTE_PRESENT);

    return pd;
}


PDE *copy_pd(void)
{
    PDE *pd = mem_alloc(PAGE_SIZE_B);

    memcpy(pd, l_pd, PAGE_SIZE_B);

    // 自分自身を参照できるようにする
    int i_pd = VADDR_TO_PD_INDEX(VADDR_PD_SELF);
    void *p = paging_get_maddr(pd);
    pd[i_pd] = ((unsigned long) p & ~0xFFF) | (PTE_RW | PTE_PRESENT);

    return pd;
}


static PDE *get_pt(int i_pd)
{
    PDE *pt = (PDE *) (l_pd[i_pd] & ~0xFFF);

    if (pt == 0) {
        return 0;
    }

    return (PTE *) (0xFFC00000 | (i_pd << 12));
}


static PTE *get_pte(void *vp_vaddr)
{
    int i_pd = VADDR_TO_PD_INDEX(vp_vaddr);
    PTE *pt = get_pt(i_pd);

    if (pt == 0) {
        return 0;
    }

    int i_pt = VADDR_TO_PT_INDEX(vp_vaddr);

    return &pt[i_pt];
}


void app_area_copy(PDE *pd)
{
    memcpy(l_pd, pd, BASE_PD_I * 4);
}


void app_area_clear(void)
{
    memset(l_pd, 0, BASE_PD_I * 4);
    flush_tlb();
}


void paging_dbg(void)
{
    dbgf("\n");
    DBGF("DEBUG PAGING");

    for (int i_pd = 0; i_pd < NUM_PDE; i_pd++) {
        if (l_pd[i_pd] & PTE_4MB) {
            dbgf("%d : 4MB Page = %#X\n", i_pd, i_pd * (4 * 1024 * 1024));
            continue;
        }

        PTE *pt = get_pt(i_pd);

        if (pt == 0) {
            continue;
        }

        dbgf("%d : %#X:\n", i_pd, pt);

        int num_pages = 0;

        pt = (PTE *) (0xFFC00000 | (i_pd << 12));

        dbgf("    ");
        for (int i_pt = 0; i_pt < NUM_PTE; i_pt++) {
            PTE pte = pt[i_pt];

            if (pte == 0) {
                continue;
            }

            if (num_pages < 3) {
                if (num_pages != 0) {
                    dbgf(", ");
                }

                dbgf("%d : %#X", i_pt, pte);
            }

            num_pages++;
        }

        dbgf(", page = %d\n", num_pages);
    }

    dbgf("\n");
}

