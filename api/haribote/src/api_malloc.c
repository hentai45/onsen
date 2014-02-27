#include "apino.h"

char *api_malloc(int size)
{
    char *ret;

    __asm__ __volatile__ (
        "int   $0x40\n"
        "movl  %%eax, %0"

        : "=r" (ret)
        : "d" (API_MALLOC), "c" (size)
    );

    return ret;
}
