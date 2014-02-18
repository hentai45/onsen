/**
 * アセンブラ関数
 *
 * C 言語で書けない命令を、インラインアセンブラで記述
 *
 * @file asmfunc.c
 * @author Ivan Ivanovich Ivanov
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


#endif

