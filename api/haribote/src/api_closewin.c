#include "apino.h"

void api_closewin(int win)
{
    __asm__ __volatile__ (
        "int   $0x40"

        :
        : "d" (API_CLOSEWIN), "b" (win)
    );
}

