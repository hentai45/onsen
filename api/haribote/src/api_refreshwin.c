#include "apino.h"

void api_refreshwin(int win, int x0, int y0, int x1, int y1)
{
    __asm__ __volatile__ (
        "int   $0x40"

        :
        : "d" (API_REFRESHWIN), "b" (win), "a" (x0), "c" (y0),
          "S" (x1), "D" (y1)
    );
}
