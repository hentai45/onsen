/**
 * タイマ
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_TIMER
#define HEADER_TIMER

#define ERROR_TID  -1

#define TIMER_MAX 512

void timer_init(void);
int  timer_new(void);
void timer_free(int tid);
void free_task_timer(int pid);
void timer_start(int tid, unsigned int timeout_ms);
int  timer_stop(int tid);
unsigned int timer_get_count_10ms(void);
void int20_handler(int *esp);
void timer_dbg(void);

#endif


//=============================================================================
// 非公開ヘッダ

#include <stdbool.h>

#include "asmfunc.h"
#include "debug.h"
#include "intr.h"
#include "msg_q.h"
#include "task.h"


#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040

#define TIMER_FLG_FREE   0   // 未使用
#define TIMER_FLG_ALLOC  1   // 確保した状態
#define TIMER_FLG_USING  2   // タイマ作動中


struct TIMER {
    int tid;    // TIMER ID
    int flags;
    int pid;    // このタイマを持っているプロセス ID

    unsigned int timeout_10ms;

    struct TIMER *next_use;
};


struct TIMER_MNG {
    // 記憶領域の確保用。
    // timers のインデックスと TID は同じ
    struct TIMER timers[TIMER_MAX];

    struct TIMER *head_use;  // 使用中のタイマを timeout_10ms の昇順に並べたリスト

    unsigned int count_10ms;
    unsigned int next_timeout_10ms;
};


static struct TIMER_MNG l_mng;
static struct TIMER *ts_timer;  // タスク切り替え用タイマ
static int ts_tid;       // タスク切り替え用タイマの ID


inline __attribute__ ((always_inline))
static struct TIMER *tid2timer(int tid)
{
    if (tid < 0 || TIMER_MAX <= tid) {
        return 0;
    }

    return &(l_mng.timers[tid]);
}


//=============================================================================
// 関数

void timer_init(void)
{
    // ---- IRQ-00 の割り込み周期を約10msに設定する

    outb(PIT_CTRL, 0x34);
    outb(PIT_CNT0, 0x9C);
    outb(PIT_CNT0, 0x2E);


    // ---- タイマ初期化

    l_mng.count_10ms = 0;

    for (int tid = 0; tid < TIMER_MAX; tid++) {
        struct TIMER *t = &(l_mng.timers[tid]);

        t->tid = tid;
        t->flags = TIMER_FLG_FREE;
    }


    // ---- 番兵を追加

    int tid = timer_new();
    struct TIMER *t = tid2timer(tid);
    t->timeout_10ms = 0xFFFFFFFF;
    t->flags = TIMER_FLG_USING;
    t->next_use = 0;
    l_mng.head_use = t;
    l_mng.next_timeout_10ms = t->timeout_10ms;


    // ---- タスク切り替え用タイマの割当て

    ts_tid = timer_new();
    ts_timer = tid2timer(ts_tid);
}


int timer_new(void)
{
    for (int tid = 0; tid < TIMER_MAX; tid++) {
        if (l_mng.timers[tid].flags == TIMER_FLG_FREE) {
            l_mng.timers[tid].flags = TIMER_FLG_ALLOC;
            l_mng.timers[tid].pid = g_pid;

            return tid;
        }
    }

    return 0;
}


// タスク切り替え用タイマ取得
int timer_ts_tid(void)
{
    return ts_tid;
}


void timer_free(int tid)
{
    timer_stop(tid);

    struct TIMER *t = tid2timer(tid);

    if (t == 0) {
        return;
    }

    t->flags = TIMER_FLG_FREE;
}


void free_task_timer(int pid)
{
    for (int tid = 0; tid < TIMER_MAX; tid++) {
        struct TIMER *t = tid2timer(tid);

        if (t->pid != pid || t->flags == TIMER_FLG_FREE) {
            continue;
        }

        timer_stop(tid);
        t->flags = TIMER_FLG_FREE;
    }
}


// タイムアウトになるとタイマは自動で止まる。
// つまり、繰り返しタイマイベントは発生しない。
// 必要ならまた timer_start しないといけない。
void timer_start(int tid, unsigned int timeout_ms)
{
    struct TIMER *t = tid2timer(tid);

    if (t == 0) {
        return;
    }

    t->timeout_10ms = (timeout_ms / 10) + l_mng.count_10ms;
    t->flags = TIMER_FLG_USING;

    int e = load_eflags();
    cli();

    // ---- 昇順にならんだリストに timer を挿入する

    struct TIMER *list = l_mng.head_use;

    // 先頭に入れる場合の処理
    if (t->timeout_10ms <= list->timeout_10ms) {
        l_mng.head_use = t;
        t->next_use = list;
        l_mng.next_timeout_10ms = t->timeout_10ms;
        store_eflags(e);
        return;
    }

    // prev と list の間に入れる場合の処理
    struct TIMER *prev;
    for (;;) {
        prev = list;
        list = list->next_use;

        if (t->timeout_10ms <= list->timeout_10ms) {
            prev->next_use = t;
            t->next_use = list;
            store_eflags(e);
            return;
        }
    }

    // 一番うしろに入れる場合の処理
    prev->next_use = t;
    t->next_use = 0;
    store_eflags(e);
}


int timer_stop(int tid)
{
    struct TIMER *t = tid2timer(tid);

    if (t == 0) {
        return 0;
    }

    if ((t->flags & TIMER_FLG_USING) == 0) {
        return 0;
    }

    int e = load_eflags();
    cli();

    if (t == l_mng.head_use) {
        // 先頭だった場合の取り消し処理
        struct TIMER *tmp = t->next_use;
        l_mng.head_use = tmp;
        l_mng.next_timeout_10ms = tmp->timeout_10ms;
    } else {
        struct TIMER *list;

        for (list = l_mng.head_use; list->next_use != t; list = list->next_use) {
        }

        list->next_use = t->next_use;
    }

    t->flags = TIMER_FLG_ALLOC;

    store_eflags(e);

    return 1;
}


unsigned int timer_get_count_10ms(void)
{
    return l_mng.count_10ms;
}


// タイマ割り込み(IRQ-00)を処理
void int20_handler(int *esp)
{
    notify_intr_end(/* IRQ = */ 0);  // 受付完了をPICに通知

    l_mng.count_10ms++;

    if (l_mng.next_timeout_10ms > l_mng.count_10ms) {
        return;
    }

    bool ts = false;

    struct TIMER *t;
    for (t = l_mng.head_use; t->timeout_10ms <= l_mng.count_10ms; t = t->next_use) {
        // タイムアウト
        t->flags = TIMER_FLG_ALLOC;

        if (t == ts_timer) {
            // ここで task_switch してはダメ
            // task_switchするとIF(割り込み許可フラグ)が変化して、
            // 割込み処理が終わってないうちに次の割込みが来るかもしれないから
            ts = true;
        } else {
            // タイマメッセージをメッセージキューに入れる
            struct MSG msg;
            msg.message = MSG_TIMER;
            msg.u_param = t->tid;

            msg_q_put(t->pid, &msg);
        }

    }

    l_mng.head_use = t;
    l_mng.next_timeout_10ms = t->timeout_10ms;

    if (ts) {
        task_switch();
    }
}


static void print_timer(struct TIMER *t);

void timer_dbg(void)
{
    dbgf("\n");
    DBGF("TIMER DEBUG");

    dbgf("count_10ms = %u\n\n", l_mng.count_10ms);

    struct TIMER *t;

    dbgf("ALL TIMERS:\n");
    for (int i = 0; i < TIMER_MAX; i++) {
        t = &l_mng.timers[i];

        if (t->flags != TIMER_FLG_FREE) {
            print_timer(t);
        }
    }

    dbgf("\nUSED TIMERS:\n");
    for (t = l_mng.head_use; t; t = t->next_use) {
        print_timer(t);
    }

    dbgf("\n");
}

static char *status[] = { "free", "alloc", "using" };

static void print_timer(struct TIMER *t)
{
    dbgf("tid = %d, pid = %d, timeout_10ms = %u, status = %s\n",
            t->tid, t->pid, t->timeout_10ms, status[t->flags]);
}
