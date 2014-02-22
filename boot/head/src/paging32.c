/**
 * ページングの仮設定
 *
 * @par 各タスクでのメモリマップ（アドレスはリニアアドレス）
 * - 0x00000000 - 0xBFFFFFFF : ユーザ空間
 * - 0xC0000000 - 0xFFFFFFFF : カーネル空間（特権レベル0）
 */

#include "asmfunc.h"
#include "memory.h"
#include "sysinfo.h"

#define NUM_PDE     (1024)  /* 1つの PD 内の PTE の数 */

#define PTE_PRESENT (0x01)  /* 1ならページがメモリ上に存在する */
#define PTE_RW      (0x02)  /* 0なら特権レベル3では書き込めない */
#define PTE_US      (0x04)  /* 0なら特権レベル3ではアクセスできない */
#define PTE_4MB     (0x80)  /* 4MBページ */


#define VADDR_TO_PD_INDEX(vaddr)  (((unsigned long) vaddr) >> 22)


typedef unsigned long PDE;


static PDE *create_os_pd(void);

void paging_init(void)
{
    enable_4MB_page();

    PDE *pd = create_os_pd();

    store_cr3((unsigned long) pd);

    enable_paging();
}

#define SIZE_4MB (4 * 1024 * 1024)

static void zero_clear_pde(PDE *pd);
static void map_4MB_page(PDE *pd, void *vp_vaddr, void *vp_maddr);
static void map_self(PDE *pd);

static PDE *create_os_pd(void)
{
    PDE *pd = (PDE *) MADDR_OS_PDT;
    zero_clear_pde(pd);

    // 最初の物理メモリ4MBを論理アドレスにそのまま対応させる
    map_4MB_page(pd, (void *) 0, (void *) 0);

    // 最初の物理メモリ4MBを論理アドレスVADDR_BASEに対応させる
    map_4MB_page(pd, (void *) VADDR_BASE, (void *) 0);

    // VRAMを論理アドレスVADDR_VRAMから8MBを対応させる
    unsigned short *vram = (unsigned short *) g_sys_info->vram;
    map_4MB_page(pd, (void *) VADDR_VRAM, vram);
    map_4MB_page(pd, (void *) (VADDR_VRAM + SIZE_4MB), ((char *) vram) + SIZE_4MB);

    map_self(pd);

    return pd;
}


#define FLG_4MB (PTE_4MB | PTE_RW | PTE_US | PTE_PRESENT)

/**
 * 4MBページの物理アドレスをリニアアドレスに対応づける。
 * @pd ページディレクトリテーブル
 * @vp_vaddr 論理アドレス
 * @vp_maddr 物理アドレス
 */
static void map_4MB_page(PDE *pd, void *vp_vaddr, void *vp_maddr)
{
    int i_pd = VADDR_TO_PD_INDEX(vp_vaddr);
    unsigned long maddr = (unsigned long) vp_maddr;
    pd[i_pd] = (maddr & ~0xFFF) | FLG_4MB;
}


static void map_self(PDE *pd)
{
    int i_pd = VADDR_TO_PD_INDEX(VADDR_PD_SELF);
    pd[i_pd] = ((unsigned long) pd & ~0xFFF) | (PTE_RW | PTE_US | PTE_PRESENT);
}


static void zero_clear_pde(PDE *pd)
{
    int i;

    for (i = 0; i < NUM_PDE; i++) {
        pd[i] = 0;
    }
}
