#include "apino.h"

void api_point(int win, int x, int y, int col)
{
    __asm__ __volatile__ (
        "int   $0x40"

        :
        : "d" (API_POINT), "b" (win), "S" (x), "D" (y), "a" (col)
    );
}
