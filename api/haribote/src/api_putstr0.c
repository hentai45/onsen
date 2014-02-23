#include "haribote.h"

inline __attribute__ ((always_inline))
static int api1(int api_no, char *arg1)
{
    int ret = 0;

    __asm__ volatile(
        "int   $0x40\n"
        "movl  %%eax, %0"

        : "=q" (ret)
        : "d" (api_no), "b" (arg1)
    );

    return ret;
}

void api_putstr0(char *s)
{
    api1(API_PUTSTR0, s);
}

