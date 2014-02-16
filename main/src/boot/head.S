# マルチブート
#define MB_MAGIC  0x1BADB002
#define MB_FLAGS  0x00000000
#define MB_CHKSUM -(MB_MAGIC + MB_FLAGS)

    .text
    .global _start
    .extern OnSenMain

_start:
    jmp entry
    .align 4
    .long MB_MAGIC
    .long MB_FLAGS
    .long MB_CHKSUM

entry:
    push %ebx

    jmp OnSenMain

