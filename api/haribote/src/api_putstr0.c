#include "apino.h"

void api_putstr0(const char *s)
{
    __asm__ __volatile__ (
        "int   $0x40"

        :
        : "d" (API_PUTSTR0), "b" (s)
    );
}

