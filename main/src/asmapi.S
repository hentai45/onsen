/*
#ifndef HEADER_ASMAPI
#define HEADER_ASMAPI

void asm_api(void);

#endif
*/


.globl asm_api

.extern onsen_api

.text


# API(システムコール)を処理するトラップゲートから呼ばれる
# C の onsen_api 関数を呼び出す
asm_api:
    pushal
    pushl  %edi          # 引数５
    pushl  %esi          # 引数４
    pushl  %edx          # 引数３
    pushl  %ecx          # 引数２
    pushl  %ebx          # 引数１
    pushl  %eax          # API 番号
    call   onsen_api
    movl   %eax, 52(%esp)  # 戻り値をこの関数の入り口の pushal で積んだ
                           # eax レジスタに格納する。
    addl   $24, %esp
    popal
    iret

