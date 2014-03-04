/**
 * マウス
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_MOUSE
#define HEADER_MOUSE

#include <stdbool.h>


typedef struct MOUSE_DECODE {
    // ---- デコード結果
    int x;   ///< マウス位置X
    int y;   ///< マウス位置Y
    bool btn_left;
    bool btn_right;
    bool btn_center;
    bool change_pos;

    // ---- デコード処理用
    int phase;
    unsigned char buf[3];
    int dx;  ///< マウス位置の変化分X
    int dy;  ///< マウス位置の変化分Y
} MOUSE_DECODE;


void mouse_init(void);
void int2C_handler(int *esp);
MOUSE_DECODE *mouse_decode(unsigned char data);

#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "graphic.h"
#include "intr.h"
#include "keyboard.h"
#include "msg_q.h"
#include "task.h"
#include "timer.h"


#define KBC_DATA_ENABLE_MOUSE    0xF4


#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAXMIN(A, B, C) MAX(A, MIN(B, C))


static MOUSE_DECODE l_mdec;

static unsigned int l_w;
static unsigned int l_h;

//=============================================================================
// 公開関数

void mouse_init(void)
{
    l_w = get_screen_w();
    l_h = get_screen_h();

    l_mdec.x = l_w / 2;
    l_mdec.y = l_h / 2;

    l_mdec.phase = 0;

    // ---- キーボードコントローラがマウスデータを送信するように設定する

    wait_kbc_sendready();
    outb(PORT_W_KBC_CMD, KBC_CMD_WRITE_MODE);
    wait_kbc_sendready();
    outb(PORT_RW_KBC_DATA, KBC_DATA_ENABLE_MOUSE_MODE);


    // ---- マウスがデータを送信するように設定する

    wait_kbc_sendready();
    outb(PORT_W_KBC_CMD, KBC_CMD_SENDTO_MOUSE);
    wait_kbc_sendready();
    outb(PORT_RW_KBC_DATA, KBC_DATA_ENABLE_MOUSE);
    // うまくいくとACK(0xFA)がIRQ-12で送信されてくる
}


// マウス割り込み(IRQ-12)を処理する
void int2C_handler(int *esp)
{
    notify_intr_end(/* IRQ = */ 12);  // 割り込み完了通知

    // マウスデータの取得
    unsigned char data = inb(PORT_RW_KBC_DATA);

    // データをメッセージキューに入れる
    MSG msg;
    msg.message = MSG_RAW_MOUSE;
    msg.u_param = data;

    msg_q_put(g_root_pid, &msg);
}


MOUSE_DECODE *mouse_decode(unsigned char data)
{
    // マウスデータは３バイトで１つのデータになる

    switch (l_mdec.phase) {
    case 0:
        // マウスの 0xFA を待っている段階
        if (data == 0xFA) {
            l_mdec.phase = 1;
        }
        break;

    case 1:
        if ((data & 0xC8) == 0x08) {  // 正しい１バイト目か判断
            l_mdec.buf[0] = data;
            l_mdec.phase = 2;
        }
        break;

    case 2:
        l_mdec.buf[1] = data;
        l_mdec.phase = 3;
        break;

    case 3:
        l_mdec.buf[2] = data;
        l_mdec.phase = 1;

        l_mdec.btn_left   = l_mdec.buf[0] & 0x01;
        l_mdec.btn_right  = l_mdec.buf[0] & 0x02;
        l_mdec.btn_center = l_mdec.buf[0] & 0x04;

        l_mdec.dx = l_mdec.buf[1];
        l_mdec.dy = l_mdec.buf[2];

        if ((l_mdec.buf[0] & 0x10) != 0) {
            l_mdec.dx |= 0xFFFFFF00;
        }

        if ((l_mdec.buf[0] & 0x20) != 0) {
            l_mdec.dy |= 0xFFFFFF00;
        }

        l_mdec.dy = -l_mdec.dy;

        bool change_pos = false;

        if (l_mdec.dx != 0) {
            l_mdec.x = MAXMIN(0, l_mdec.x + l_mdec.dx, l_w);
            change_pos = true;
        }

        if (l_mdec.dy != 0) {
            l_mdec.y = MAXMIN(0, l_mdec.y + l_mdec.dy, l_h);
            change_pos = true;
        }

        l_mdec.change_pos = change_pos;

        return &l_mdec;
    }

    return 0;
}


//=============================================================================
// 非公開関数



