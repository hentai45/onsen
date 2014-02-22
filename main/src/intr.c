/**
 * 割り込み関連
 *
 * @note
 * 割り込みデータはメッセージキューに格納される
 *
 * @file intr.c
 * @author Ivan Ivanovich Ivanov
 */

/*
 * ＜目次＞
 * ・割り込み
 * ・フォールト
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_INTR
#define HEADER_INTR

//-----------------------------------------------------------------------------
// 割り込み

void intr_init(void);

void notify_intr_end(unsigned char irq);

void set_pic0_mask(unsigned char mask);
void set_pic1_mask(unsigned char mask);

//-----------------------------------------------------------------------------
// フォールト


#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "debug.h"
#include "graphic.h"
#include "msg_q.h"
#include "str.h"
#include "task.h"


//-----------------------------------------------------------------------------
// 割り込み

#define PIC0_ICW1  0x0020
#define PIC0_OCW2  0x0020
#define PIC0_IMR   0x0021
#define PIC0_ICW2  0x0021
#define PIC0_ICW3  0x0021
#define PIC0_ICW4  0x0021

#define PIC1_ICW1  0x00A0
#define PIC1_OCW2  0x00A0
#define PIC1_IMR   0x00A1
#define PIC1_ICW2  0x00A1
#define PIC1_ICW3  0x00A1
#define PIC1_ICW4  0x00A1


//-----------------------------------------------------------------------------
// フォールト

static void fault_handler(const char *msg, int *esp);

#define INT_HANDLER(NO, MESSAGE) void int ## NO ## _handler(int *esp) \
{                                                                     \
    fault_handler("INT 0x" #NO " : " MESSAGE, esp);                   \
}

//=============================================================================
// 公開関数

//-----------------------------------------------------------------------------
// 割り込み

void intr_init(void)
{
    // ---- PICの初期化

    set_pic0_mask(0xFF);      // すべての割り込みを受け付けない
    set_pic1_mask(0xFF);      // すべての割り込みを受け付けない

    outb(PIC0_ICW1, 0x11);    // エッジトリガモード
    outb(PIC0_ICW2, 0x20);    // IRQ0-7は、INT20-27で受ける
    outb(PIC0_ICW3, 1 << 2);  // PIC1はIRQ2にて接続
    outb(PIC0_ICW4, 0x01);    // ノンバッファモード

    outb(PIC1_ICW1, 0x11);    // エッジトリガモード
    outb(PIC1_ICW2, 0x28);    // IRQ8-15は、INT28-2Fで受ける
    outb(PIC1_ICW3, 2);       // PIC1はIRQ2にて接続
    outb(PIC1_ICW4, 0x01);    // ノンバッファモード

    set_pic0_mask(0xFB);      // 11111011 PIC1以外はすべての割り込みを禁止
    set_pic1_mask(0xFF);      // 11111111 すべての割り込みを受け付けない
}


/// 割り込みマスクを設定する。
/// mask = 0xFF でPIC0すべての割り込みを受け付けない
void set_pic0_mask(unsigned char mask)
{
    outb(PIC0_IMR, mask);
}


void set_pic1_mask(unsigned char mask)
{
    outb(PIC1_IMR, mask);
}


/**
 * 割込完了通知
 *
 * PICのOCW2に「0x60 + IRQ番号」をoutすることで割り込み完了通知できる。
 * これで次の割り込みもPICから通知されるようになる。
 */
void notify_intr_end(unsigned char irq)
{
    if (0 <= irq && irq <= 7) {
        outb(PIC0_OCW2, 0x60 + irq);
    } else if (8 <= irq && irq <= 15) {
        outb(PIC1_OCW2, 0x60 + irq - 8);
        outb(PIC0_OCW2, 0x60 + 2);
    }
}


//-----------------------------------------------------------------------------
// フォールト

/// 除算エラー
INT_HANDLER(00, "divide error exception")

/// 配列境界違反
INT_HANDLER(05, "bounds exception")

/// 無効命令
INT_HANDLER(06, "invalid opcode exception")

/// コプロセッサ無効
INT_HANDLER(07, "coprocessor not available")

/// 無効TSS
INT_HANDLER(0A, "invalid tss exception")

/// セグメント不在
INT_HANDLER(0B, "segment not exsist exception")

/// スタック例外
INT_HANDLER(0C, "stack fault")

/// 一般保護例外
INT_HANDLER(0D, "general protection exception")

/// ページフォルト
INT_HANDLER(0E, "page fault")


//=============================================================================
// 非公開関数

static void fault_handler(const char *message, int *esp)
{
    if (is_os_task(get_pid())) {
        dbg_fault(message, esp);

        switch_debug_screen();

        for (;;) {
            hlt();
        }
    } else {
        MSG msg;
        msg.message = MSG_REQUEST_EXIT;
        msg.u_param = get_pid();
        msg.l_param = -666;
        msg_q_put(g_root_pid, &msg);

        dbg_fault(message, esp);

        switch_debug_screen();

        sti();

        for (;;) {
        }
    }
}


