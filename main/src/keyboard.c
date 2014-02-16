/**
 * キーボード
 *
 * @file keyboard.c
 * @author Ivan Ivanovich Ivanov
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_KEYBOARD
#define HEADER_KEYBOARD

#include <stdbool.h>


#define PORT_RW_KBC_DATA            0x0060
#define KBC_DATA_ENABLE_MOUSE_MODE    0x47

#define PORT_R_KBC_STATE            0x0064
#define KBC_STATE_SEND_NOT_READY      0x02

#define PORT_W_KBC_CMD              0x0064
#define KBC_CMD_WRITE_MODE            0x60
#define KBC_CMD_SENDTO_MOUSE          0xD4


void int21_handler(int *esp);
void wait_kbc_sendready(void);
char keycode2char(int keycode, bool is_shift_on);

#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "intr.h"
#include "msg_q.h"
#include "task.h"


//=============================================================================
// 公開関数

/// キーボード割り込み(IRQ-01)を処理する
void int21_handler(int *esp)
{
    notify_intr_end(/* IRQ = */ 1);  // 割り込み完了通知

    // キーデータの取得
    unsigned char data = inb(PORT_RW_KBC_DATA);

    // データをメッセージキューに入れる
    MSG msg;
    msg.message = MSG_KEYDOWN;
    msg.u_param = data;

    msg_q_put(g_root_pid, &msg);
}


/// キーボードコントローラがデータ送信可能になるのを待つ
void wait_kbc_sendready(void)
{
    for (;;) {
        if ((inb(PORT_R_KBC_STATE) & KBC_STATE_SEND_NOT_READY) == 0) {
            return;
        }
    }
}


char keycode2char(int keycode, bool is_shift_on)
{
    // [キーコード => 文字コード] 変換テーブル
    static char keytbl[0x80] = {
          0,   0, '1', '2', '3', '4', '5', '6', '7', '8',
        '9', '0', '-', '^',   0,   0, 'Q', 'W', 'E', 'R',
        'T', 'Y', 'U', 'I', 'O', 'P', '@', '[',   0,   0,
        'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';',
        ':',   0,   0, ']', 'Z', 'X', 'C', 'V', 'B', 'N',
        'M', ',', '.', '/',   0, '*',   0, ' ',   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.'
    };

    // SHIFT ON 時の [キーコード => 文字コード] 変換テーブル
    static char keytbl_shift[0x80] = {
          0,   0, '!',0x22, '#', '$', '%', '&',0x27, '(',
        ')', '~', '=', '~',   0,   0, 'Q', 'W', 'E', 'R',
        'T', 'Y', 'U', 'I', 'O', 'P', '`', '{',   0,   0,
        'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', '+',
        '*',   0,   0, '}', 'Z', 'X', 'C', 'V', 'B', 'N',
        'M', '<', '>', '?',   0, '*',   0, ' ',   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.',   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0, '_',   0,   0,   0,   0,
          0,   0,   0,   0,   0, '|',   0,   0
    };


    // **** [キーコード => 文字コード] 変換

    char ch;
    if (keycode < 0x80) {
        if (is_shift_on) {
            ch = keytbl_shift[keycode];
        } else {
            ch = keytbl[keycode];
        }
    } else {
        ch = 0;
    }


    // **** 大文字・小文字変換

    if ('A' <= ch && ch <= 'Z') {
        if (! is_shift_on) {
            ch += 0x20;  // 大文字 => 小文字
        }
    }


    // 通常文字
    if (ch != 0) {
        return ch;
    }

    // その他
    return 0;
}


//=============================================================================
// 非公開関数




