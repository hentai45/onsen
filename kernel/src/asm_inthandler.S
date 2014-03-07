/**
 * アセンブラで書かれた割り込みハンドラ
 *
 * C 言語の割り込みハンドラを呼び出す。
 *
 * ## なぜアセンブラで書くのか？
 * C 言語のインラインアセンブラでは iret がうまく動作しないから。
 * スタックが原因。
 * コンパイラは関数の前後に自動的にスタックの処理を追加する。
 * でも iret すると、関数の前に追加されたスタック処理は実行するけど、
 * 関数の後に追加されたスタック処理は実行されない。
 * だから、アセンブラで間接的に C の割り込みハンドラを呼び出している。
 */


/*
#ifndef HEADER_INTHANDLER
#define HEADER_INTHANDLER

void asm_int00_handler(void);
void asm_int05_handler(void);
void asm_int06_handler(void);
void asm_int07_handler(void);
void asm_int0A_handler(void);
void asm_int0B_handler(void);
void asm_int0C_handler(void);
void asm_int0D_handler(void);
void asm_int0E_handler(void);

void asm_int20_handler(void);
void asm_int21_handler(void);
void asm_int2C_handler(void);

#endif
*/


    .globl asm_int00_handler
    .globl asm_int05_handler
    .globl asm_int06_handler
    .globl asm_int07_handler
    .globl asm_int0A_handler
    .globl asm_int0B_handler
    .globl asm_int0C_handler
    .globl asm_int0D_handler
    .globl asm_int0E_handler

    .globl asm_int20_handler
    .globl asm_int21_handler
    .globl asm_int2C_handler


    .extern int00_handler
    .extern int05_handler
    .extern int06_handler
    .extern int07_handler
    .extern int0A_handler
    .extern int0B_handler
    .extern int0C_handler
    .extern int0D_handler
    .extern int0E_handler

    .extern int20_handler
    .extern int21_handler


# 特権レベル３（アプリ）から呼び出される場合、cs レジスタは特権レベル０(OS)に
# 変わるけど、ds, es はそのままになっている。
# なので、ss（OS のデータセグメントになっている）を ds, es に代入している。
.macro asm_int_handler  c_int_handler
    pushl %es
    pushl %ds
    pushal
    movw  %ss, %ax
    movw  %ax, %ds
    movw  %ax, %es
    call  \c_int_handler
    popal
    popl  %ds
    popl  %es
    iret
.endm

    .text


asm_int00_handler:
    asm_int_handler int00_handler

asm_int05_handler:
    asm_int_handler int05_handler

asm_int06_handler:
    asm_int_handler int06_handler

asm_int07_handler:
    asm_int_handler int07_handler

asm_int0A_handler:
    asm_int_handler int0A_handler

asm_int0B_handler:
    asm_int_handler int0B_handler

asm_int0C_handler:
    asm_int_handler int0C_handler

asm_int0D_handler:
    asm_int_handler int0D_handler

asm_int0E_handler:
    pushl %es
    pushl %ds
    pushal
    movw  %ss, %ax
    movw  %ax, %ds
    movw  %ax, %es
    call  int0E_handler
    popal
    popl  %ds
    popl  %es
    addl  $4, %esp  # エラーコードを取り除く
    iret

asm_int20_handler:
    asm_int_handler int20_handler

asm_int21_handler:
    asm_int_handler int21_handler

asm_int2C_handler:
    asm_int_handler int2C_handler

