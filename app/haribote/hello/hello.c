#include "haribote.h"

static char buf[150 * 50];

void HariMain(void)
{
    int i;

    api_putchar('H');
    api_putchar('a');
    api_putchar('r');
    api_putchar('i');
    api_putchar('\n');

    api_putstr1("abcdefg", 3);
    api_putstr0("\nhello!\n");

    int win = api_openwin(0, 150, 50, -1, "Hello");

    api_end();
}

