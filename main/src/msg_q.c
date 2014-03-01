/**
 * メッセージキュー
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_MSG_Q
#define HEADER_MSG_Q

#include "msg.h"

void msg_q_init(int pid);
int  msg_q_put(int pid, struct MSG *msg);
int  msg_q_get(int pid, struct MSG *msg);
int  msg_q_size(int pid);

#endif


//=============================================================================
// 非公開ヘッダ

#include "asmfunc.h"
#include "task.h"


#define MSG_Q_MAX 128  ///< メッセージキューの最大容量


// メッセージキュー
typedef struct MSG_Q {
    int capacity;
    MSG *buf;
    int free;
    int i_w;
    int i_r;
} MSG_Q;


typedef struct MSG_Q_MNG {
    MSG_Q msg_q[TASK_MAX];
    MSG msg_q_buf[TASK_MAX][MSG_Q_MAX];
} MSG_Q_MNG;


static MSG_Q_MNG s_mng;


static MSG_Q *get_msg_q(int pid);


//=============================================================================
// 公開関数

void msg_q_init(int pid)
{
    MSG_Q *q = get_msg_q(pid);

    if (q == 0) {
        return;
    }

    q->buf = s_mng.msg_q_buf[pid];
    q->capacity = MSG_Q_MAX;
    q->free = MSG_Q_MAX;
    q->i_w = 0;
    q->i_r = 0;
}


int msg_q_put(int pid, MSG *msg)
{
    MSG_Q *q = get_msg_q(pid);

    if (q == 0) {
        return -1;
    }

    if (q->free == 0) {
        return -1;
    }

    q->buf[q->i_w++] = *msg;
    if (q->i_w >= q->capacity) {
        q->i_w = 0;
    }
    q->free--;

    // タスクが寝てたら起こす
    task_wakeup(pid);

    return 0;
}


int msg_q_get(int pid, MSG *msg)
{
    MSG_Q *q = get_msg_q(pid);

    if (q == 0) {
        return -1;
    }

    if (q->free == q->capacity) {
        return -1;
    }

    *msg = q->buf[q->i_r++];
    if (q->i_r == q->capacity) {
        q->i_r = 0;
    }
    q->free++;

    return 0;
}


int msg_q_size(int pid)
{
    MSG_Q *q = get_msg_q(pid);

    if (q == 0) {
        return 0;
    }

    return q->capacity - q->free;
}


//=============================================================================
// 非公開関数

static MSG_Q *get_msg_q(int pid)
{
    if (pid < 0 || TASK_MAX <= pid) {
        return 0;
    }

    return &(s_mng.msg_q[pid]);
}


