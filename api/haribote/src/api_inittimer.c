#include "apino.h"

void api_inittimer(int timer, int data)
{
    __asm__ __volatile__ (
        "int   $0x40"

        :
        : "d" (API_INITTIMER), "b" (timer), "a" (data)
    );
}

