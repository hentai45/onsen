#include "apino.h"

void api_settimer(int timer, int time)
{
    __asm__ __volatile__ (
        "int   $0x40"

        :
        : "d" (API_SETTIMER), "b" (timer), "a" (time)
    );
}

