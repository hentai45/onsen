#include "haribote.h"

void HariMain(void)
{
    api_putchar('H');
    api_putchar('a');
    api_putchar('r');
    api_putchar('i');
    api_putchar('\n');

    api_putstr1("abcdefg", 3);
    api_putstr0("\nhello!\n");

    api_end();
}

