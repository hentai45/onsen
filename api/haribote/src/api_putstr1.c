#include "apino.h"

void api_putstr1(const char *s, int cnt)
{
    __asm__ __volatile__ (
        "int   $0x40"

        :
        : "d" (API_PUTSTR1), "b" (s), "c" (cnt)
    );
}