#include "apino.h"

void api_putstrwin(int win, int x, int y, int col, int len, char *str)
{
    __asm__ __volatile__ (
        "movl %0, %%ebp\n"
        "int   $0x40"

        :
        : "g" (str), "d" (API_PUTSTRWIN), "b" (win), "S" (x), "D" (y),
          "a" (col), "c" (len)
    );
}
