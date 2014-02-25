#include "haribote.h"

static char buf[150 * 50];

void HariMain(void)
{
    api_putchar('H');
    api_putchar('a');
    api_putchar('r');
    api_putchar('i');
    api_putchar('\n');

    api_putstr1("abcdefg", 3);
    api_putstr0("\nhello!\n");

    int win = api_openwin(0, 150, 50, -1, "Hello");

    api_putintx(win);
    api_putchar('\n');

    api_boxfilwin(win, 10, 10, 100, 30, 0xF0);
    api_putstrwin(win, 20, 20, 0x0F, 4, "test");

    for (int i = 0; i < 50; i++) {
        api_point(win, i, i, 0x23);
    }

    api_linewin(win, 150, 0, 0, 50, 0x4B);

    api_refreshwin(win, 0, 0, 150, 50);

    api_getkey(1);

    api_closewin(win);

    api_end();
}

