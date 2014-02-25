#include "apino.h"

int api_openwin(char *buf, int w, int h, int col_inv, char *title)
{
    int sid;

    __asm__ __volatile__ (
        "int   $0x40\n"
        "movl  %%eax, %0"

        : "=q" (sid)
        : "d" (API_OPENWIN), "b" (buf), "S" (w), "D" (h),
          "a" (col_inv), "c" (title)
    );

    return sid;
}
