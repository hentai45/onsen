#include "apino.h"

int api_linewin(int win, int x0, int y0, int x1, int y1, int col)
{
    __asm__ __volatile__ (
        "movl %0, %%ebp\n"
        "int   $0x40"

        :
        : "g" (col), "d" (API_LINEWIN), "b" (win), "a" (x0), "c" (y0),
          "S" (x1), "D" (y1)
    );
}
