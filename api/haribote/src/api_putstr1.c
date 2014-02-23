#include "haribote.h"

void api_putstr1(const char *s, int cnt)
{
    __asm__ volatile(
        "int   $0x40\n"

        :
        : "d" (API_PUTSTR1), "b" (s), "c" (cnt)
    );
}
