#include "onsen.h"
#include "msg.h"

void OnSenMain(void)
{
    int sid = get_screen();

    fill_surface(sid, COL_RED);
    update_screen(sid);

    char s[2];
    char tt;

    asm volatile (
            "movw $1132*8, %%ax\n\t"
            "movw %%ax, %%ds\n\t"
            "movb $65, %%ds:(0)\n\t"
            "movb %%ds:(1), %0\n\t"
            "movw $1133*8, %%ax\n\t"
            "movw %%ax, %%ds\n\t"
            : "=q" (tt)
            );

    s[0] = tt;
    s[1] = 0;

    dbg_str(s);

    exit_app(0);
}

