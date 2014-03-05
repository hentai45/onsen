#include "apilib.h"

void HariMain(void)
{
    short cs, ds;
    __asm__ __volatile__ ("mov %%cs, %0" : "=r" (cs));
    __asm__ __volatile__ ("mov %%ds, %0" : "=r" (ds));

    api_putstr0("cs = ");
    api_putintx(cs);
    api_putstr0("\nds = ");
    api_putintx(ds);
    api_putstr0("\n\n");

    // コード領域への書き込み
    unsigned int *code_p = (unsigned int *) 0;
    *code_p = 1;

    // OS領域の読み込み
    unsigned int *os_p = (unsigned int *) 0xC0280000;
    for (int i = 0; i < 12; i++) {
        api_putintx(os_p[i]);
        if (i % 4 == 3)
            api_putstr0("\n");
        else
            api_putstr0(" ");
    }

    api_end();
}
