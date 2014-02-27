#include "apino.h"

void api_freetimer(int timer)
{
    __asm__ __volatile__ (
        "int   $0x40"

        :
        : "d" (API_FREETIMER), "b" (timer)
    );
}

