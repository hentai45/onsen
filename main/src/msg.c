/**
 * メッセージ
 *
 * @file msg.c
 * @author Ivan Ivanovich Ivanov
 */

/*
 * ＜目次＞
 * ・メッセージ処理
 * ・メッセージ定義
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_MSG
#define HEADER_MSG

typedef struct MSG {
    unsigned int message;
    unsigned long u_param;
    long l_param;
} MSG;


//-----------------------------------------------------------------------------
// メッセージ処理

typedef void (*ONSEN_PROC) (unsigned int, unsigned long, long);

int get_message(MSG *msg);
void dispatch_message(const MSG *msg, ONSEN_PROC proc);


//-----------------------------------------------------------------------------
// メッセージ定義


// ---- アプリケーション終了依頼メッセージ
// u_param : 終了させるアプリケーションの PID
// l_param : 終了コード
#define MSG_REQUEST_EXIT  0


// ---- タイマメッセージ
// u_param : タイマID
#define MSG_TIMER    1


// ---- 割り込みメッセージ（Ctrl + C により発生する）
#define MSG_INTR     2


// ---- キーコードメッセージ
// u_param : キーコード
#define MSG_KEYDOWN  3


// ---- 文字メッセージ
// u_param : 文字
#define MSG_CHAR     4


// ---- 生マウスメッセージ
// u_param : データ
#define MSG_RAW_MOUSE  5


// ---- 左クリックメッセージ
// l_param : 下位16ビット：x, 上位16ビット：y
#define MSG_LEFT_CLICK  6


// ---- 右クリックメッセージ
// l_param : 下位16ビット：x, 上位16ビット：y
#define MSG_RIGHT_CLICK  7


// ---- 中央クリックメッセージ
// l_param : 下位16ビット：x, 上位16ビット：y
#define MSG_CENTER_CLICK  8


// ---- 子タスクの終了通知メッセージ
// u_param : 子タスクの PID
// l_param : 終了ステータス
#define MSG_NOTIFY_CHILD_EXIT   9


// ---- 画面切り替わりメッセージ
// u_param : 切り替わった画面の PID
#define MSG_SCREEN_SWITCHED    10


#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "msg_q.h"
#include "task.h"


//=============================================================================
// 公開関数

//-----------------------------------------------------------------------------
// メッセージ処理

int get_message(MSG *msg)
{
    int pid = get_pid();

    for (;;) {
        cli();

        if (msg_q_size(pid) == 0) {
            task_sleep(pid);
            sti();
        } else {
            msg_q_get(pid, msg);
            sti();
            return 1;
        }
    }
}

void dispatch_message(const MSG *msg, ONSEN_PROC proc)
{
    proc(msg->message, msg->u_param, msg->l_param);
}


//=============================================================================
// 非公開関数



