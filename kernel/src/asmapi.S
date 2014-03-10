/**
 * API呼び出され処理
 */

/*
#ifndef HEADER_ASMAPI
#define HEADER_ASMAPI

void asm_api(void);
void new_task_start(void);

#endif
*/


    .globl asm_api
    .globl new_task_start

    .extern onsen_api

    .text


# API(システムコール)を処理するトラップゲートから呼ばれる
# C の onsen_api 関数を呼び出す。
# トラップゲートなので割り込み禁止に自動でならない。
asm_api:
    pushl %es
    pushl %ds
    pushal
    movw  %ss, %ax
    movw  %ax, %ds         # OS用のセグメントをdsとesに設定する
    movw  %ax, %es
    call   onsen_api
    movl   %eax, 28(%esp)  # 戻り値をこの関数の入り口の pushal で積んだ
                           # eax レジスタに格納する。
    popal
    popl  %ds
    popl  %es
    iret

new_task_start:
    popal
    popl  %ds
    popl  %es
    iret

