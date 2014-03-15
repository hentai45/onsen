/**
 * ブートローダ
 */

#include "memory.h"
#include "sysinfo.h"


struct SYSTEM_INFO *g_sys_info = (struct SYSTEM_INFO *) ADDR_SYS_INFO;


void paging_init(void);
void run_elf(void *p);

extern int OnSenElfStart;

/**
 * 32ビット処理のメイン関数
 */
void head32_main(void)
{
    paging_init();            /* 仮のページングを設定 */
    run_elf(&OnSenElfStart);  /* onsenを実行 */
}


void memory_copy32(char *dst, char *src, int num_bytes)
{
    int i;
    for (i = 0; i < num_bytes; i++) {
        *dst++ = *src++;
    }
}


void memory_set32(char *p, char val, int num_bytes)
{
    int i;
    for (i = 0; i < num_bytes; i++) {
        *p++ = val;
    }
}
