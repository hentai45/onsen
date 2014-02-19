/**
 * アセンブラ関数
 *
 * C 言語で書けない命令を、インラインアセンブラで記述
 */

#ifndef HEADER_ASMFUNC
#define HEADER_ASMFUNC

inline __attribute__ ((always_inline))
static void cli(void)
{
    __asm__ __volatile__ ("cli");
}


inline __attribute__ ((always_inline))
static void nop(void)
{
    __asm__ __volatile__ ("nop");
}


inline __attribute__ ((always_inline))
static void outb(unsigned short port, unsigned char data)
{
    __asm__ __volatile__ (
        "outb %0, %1"

        :
        : "a" (data), "Nd" (port)  // N : 0 から 255 の範囲の定数
    );
}


inline __attribute__ ((always_inline))
static unsigned char inb(unsigned short port)
{
    unsigned char ret;

    __asm__ __volatile__ (
        "inb %1, %0"

        : "=a" (ret)
        : "Nd" (port)  // N : 0 から 255 の範囲の定数
    );

    return ret;
}


inline __attribute__ ((always_inline))
static int load_cr0(void)
{
    int cr0;

    __asm__ __volatile__ ("movl %%cr0, %0" : "=r" (cr0));

    return cr0;
}


inline __attribute__ ((always_inline))
static void store_cr0(int cr0)
{
    __asm__ __volatile__ ("movl %0, %%cr0" : : "r" (cr0));
}


inline __attribute__ ((always_inline))
static void store_cr3(unsigned long cr3)
{
    __asm__ __volatile__ ("movl %0, %%cr3" : : "r" (cr3));
}


inline __attribute__ ((always_inline))
static int load_eflags(void)
{
    int eflags = 0;

    __asm__ __volatile__ (
        "pushf\n"
        "popl %0"

        : "=r" (eflags)
    );

    return eflags;
}


inline __attribute__ ((always_inline))
static void store_eflags(int eflags)
{
    __asm__ __volatile__ (
        "pushl %0\n"
        "popf"

        :
        : "a" (eflags)
    );
}


inline __attribute__ ((always_inline))
static void enable_4MB_page(void)
{
    unsigned long temp;

    __asm__ __volatile__ (
        "movl %%cr4, %0\n"
        "orl  $0x00000010, %0\n"  /* PSEフラグON */
        "movl %0, %%cr4"

        : "=r" (temp)
    );
}


inline __attribute__ ((always_inline))
static void enable_paging(void)
{
    unsigned long temp;

    __asm__ __volatile__ (
        "movl  %%cr0, %0\n"
        "orl   $0x80000000, %0\n"
        "movl  %0, %%cr0"

        : "=r" (temp)
    );
}

#endif

