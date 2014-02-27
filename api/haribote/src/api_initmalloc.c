#include "apino.h"

void api_initmalloc(void)
{
    __asm__ __volatile__ (
        "int   $0x40"

        :
        : "d" (API_INITMALLOC)
    );
}
