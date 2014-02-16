/**
 * アセンブラ関数
 *
 * C 言語で書けない命令を、インラインアセンブラで記述
 *
 * @file asmfunc.c
 * @author Ivan Ivanovich Ivanov
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_ASMFUNC
#define HEADER_ASMFUNC

inline __attribute__ ((always_inline)) void hlt(void);
inline __attribute__ ((always_inline)) void stihlt(void);
inline __attribute__ ((always_inline)) void cli(void);
inline __attribute__ ((always_inline)) void sti(void);
inline __attribute__ ((always_inline)) void outb(unsigned short port,
        unsigned char data);
inline __attribute__ ((always_inline)) unsigned char inb(unsigned short port);
inline __attribute__ ((always_inline)) void outw(unsigned short port,
        unsigned short data);
inline __attribute__ ((always_inline)) int load_cr0(void);
inline __attribute__ ((always_inline)) void store_cr0(int cr0);
inline __attribute__ ((always_inline)) unsigned long load_cr3(void);
inline __attribute__ ((always_inline)) void store_cr3(unsigned long cr3);
inline __attribute__ ((always_inline)) void enable_paging(void);
inline __attribute__ ((always_inline)) void flush_tlb(unsigned long vaddr);
inline __attribute__ ((always_inline)) int load_eflags(void);
inline __attribute__ ((always_inline)) void store_eflags(int eflags);
inline __attribute__ ((always_inline)) void ltr(int tr);
inline __attribute__ ((always_inline)) void far_jmp(unsigned short cs,
        unsigned long eip);
inline __attribute__ ((always_inline)) void far_call(unsigned short cs,
        unsigned long eip);
inline __attribute__ ((always_inline)) void reload_segments(unsigned short cs,
        unsigned short ds);


inline __attribute__ ((always_inline))
void hlt(void)
{
    __asm__ __volatile__ ("hlt");
}


inline __attribute__ ((always_inline))
void stihlt(void)
{
    __asm__ __volatile__ (
            "sti        \n\t"
            "hlt");
}


inline __attribute__ ((always_inline))
void cli(void)
{
    __asm__ __volatile__ ("cli");
}


inline __attribute__ ((always_inline))
void sti(void)
{
    __asm__ __volatile__ ("sti");
}


inline __attribute__ ((always_inline))
void outb(unsigned short port, unsigned char data)
{
    __asm__ __volatile__ (
            "outb %0, %1"

            :
            : "a" (data), "Nd" (port)  // N : 0 から 255 の範囲の定数
            );
}


inline __attribute__ ((always_inline))
unsigned char inb(unsigned short port)
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
void outw(unsigned short port, unsigned short data)
{
    __asm__ __volatile__ (
            "outw %0, %1"

            :
            : "q" (data), "q" (port)
            );
}

inline __attribute__ ((always_inline))
int load_cr0(void)
{
    int cr0;

    __asm__ __volatile__ ("movl %%cr0, %0" : "=r" (cr0));

    return cr0;
}


inline __attribute__ ((always_inline))
void store_cr0(int cr0)
{
    __asm__ __volatile__ ("movl %0, %%cr0" : : "r" (cr0));
}


inline __attribute__ ((always_inline))
unsigned long load_cr3(void)
{
    unsigned long cr3;

    __asm__ __volatile__("movl %%cr3, %0" : "=r" (cr3));

    return cr3;
}


inline __attribute__ ((always_inline))
void store_cr3(unsigned long cr3)
{
    __asm__ __volatile__("movl %0, %%cr3" : : "r" (cr3));
}


inline __attribute__ ((always_inline))
void enable_paging(void)
{
    unsigned long temp;

    __asm__ __volatile__(
            "movl  %%cr0, %0                \n\t"
            "orl   $0x80000000, %0          \n\t"
            "movl  %0, %%cr0"

            : "=r" (temp)
            );
}


inline __attribute__ ((always_inline))
void flush_tlb(unsigned long vaddr)
{
    __asm__ __volatile__(
            "cli              \n\t"
            "invlpg %0        \n\t"
            "sti"

            :
            : "m" (vaddr)
            : "memory");
}


inline __attribute__ ((always_inline))
int load_eflags(void)
{
    int eflags = 0;

    __asm__ __volatile__ (
            "pushf            \n\t"
            "popl %0"

            : "=r" (eflags)
           );

    return eflags;
}


inline __attribute__ ((always_inline))
void store_eflags(int eflags)
{
    __asm__ __volatile__ (
            "pushl %0         \n\t"
            "popf"

            :
            : "a" (eflags));
}


inline __attribute__ ((always_inline))
void ltr(int tr)
{
    __asm__ __volatile__("ltr %0" : : "m" (tr));
}


inline __attribute__ ((always_inline))
void far_jmp(unsigned short cs, unsigned long eip)
{
    struct {
        unsigned long eip;
        unsigned short cs;
    } __attribute__((__packed__)) FARJMP;

    FARJMP.cs = cs;
    FARJMP.eip = eip;

    __asm__ __volatile__ ("ljmp *(%0)" : : "q" (&FARJMP) : "memory");
}


inline __attribute__ ((always_inline))
void far_call(unsigned short cs, unsigned long eip)
{
    struct {
        unsigned long eip;
        unsigned short cs;
    } __attribute__((__packed__)) FARCALL;

    FARCALL.cs = cs;
    FARCALL.eip = eip;

    __asm__ __volatile__ ("lcall *(%0)" : : "q" (&FARCALL) : "memory");
}


// GDT を変更したあとにセグメントを読みなおすのに使用する
inline __attribute__ ((always_inline))
void reload_segments(unsigned short cs, unsigned short ds)
{
    // 「&&ラベル名」 はラベルのアドレスを取得できるgcc拡張
    far_jmp(cs, (unsigned long) &&reload_cs);

reload_cs:

    __asm__ __volatile__ (
            "movw %0, %%ds           \n\t"
            "movw %0, %%es           \n\t"
            "movw %0, %%fs           \n\t"
            "movw %0, %%gs           \n\t"
            "movw %0, %%ss"
 
            :
            : "q" (ds)
            );
}


#endif


//=============================================================================
// 非公開ヘッダ


//=============================================================================
// 公開関数


//=============================================================================
// 非公開関数



