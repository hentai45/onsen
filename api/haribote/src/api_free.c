#include "apino.h"

void api_free(char *addr, int size)
{
    __asm__ __volatile__ (
        "int   $0x40"

        :
        : "d" (API_FREE), "a" (addr), "c" (size)
    );
}
