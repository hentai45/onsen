/**
 * ブートローダ
 */

__asm__ (".code16gcc\n");

#include "asmfunc.h"


void set_video_mode(void);
static void disable_interrupts(void);
void detect_memory(void);
static void enable_a20(void);

/**
 * 16ビット処理のメイン関数
 */
void head16_main(void)
{
    disable_interrupts();  /* 割り込みを受け付けないようにする */
    set_video_mode();      /* 画面モードの設定 */
    detect_memory();       /* 使用可能メモリ領域を調べる */
    enable_a20();          /* 1MB以上のメモリにアクセスできるようにする */
}


/**
 * 一切の割り込みを受け付けないようにする
 * AT互換機の仕様では、
 * CLI前にこれをやっておかないと、たまにハングアップする
 */
static void disable_interrupts(void)
{
    outb(0x21, 0xFF);  /* 全マスタの割り込みを禁止 */
    nop();             /* OUT命令を連続させるとうまくいかない機種があるらしい */
    outb(0xA1, 0xFF);  /* 全スレーブの割り込みを禁止 */
    cli();             /* CPUレベルでも割り込み禁止 */
}


static void wait_kbc_sendready(void);

/**
 * CPUから1MB以上のメモリにアクセスできるように、A20GATEを設定する
 */
static void enable_a20(void)
{
    wait_kbc_sendready();
    outb(0x64, 0xD1);
    wait_kbc_sendready();
    outb(0x60, 0xDF);  /* enable A20 */
    wait_kbc_sendready();
}

#define PORT_R_KBC_STATE            0x0064
#define KBC_STATE_SEND_NOT_READY      0x02

/**
 * キーボードコントローラがデータ送信可能になるのを待つ
 */
static void wait_kbc_sendready(void)
{
    for (;;) {
        if ((inb(PORT_R_KBC_STATE) & KBC_STATE_SEND_NOT_READY) == 0) {
            return;
        }
    }
}
