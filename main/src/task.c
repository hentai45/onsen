/**
 * タスク
 *
 * @note
 * タスクの優先度機能はない。タイムスライスは設定できる。
 *
 * @file task.c
 * @author Ivan Ivanovich Ivanov
 */

//=============================================================================
// 公開ヘッダ

#ifndef HEADER_TASK
#define HEADER_TASK

#include <stdbool.h>
#include "file.h"

#define ERROR_PID        (-1)

#define TASK_MAX         (32)  ///< 最大タスク数
#define FILE_TABLE_SIZE  (16)


extern int g_root_pid;
extern int g_idle_pid;


void task_init(void);
int  task_new(char *name);
int  task_free(int pid, int exit_status);
void task_run(int pid, int timeslice_ms);
int  task_run_app(void *p, unsigned int size, const char *name);
void task_switch(int ts_tid);
void task_sleep(int pid);
void task_wakeup(int pid);
int run_os_task(char *name, void (*main)(void), int timeslice_ms, bool create_esp);
int  get_pid(void);
const char *task_get_name(int pid);
void task_set_pt(int i_pd, unsigned long pt);

void task_dbg(void);

int get_free_fd(void);
FILE_T *task_get_file(int fd);
int task_set_file(int fd, FILE_T *f);

int is_os_task(int pid);


#endif


//=============================================================================
// 非公開ヘッダ

#include <stdbool.h>

#include "asmfunc.h"
#include "console.h"
#include "debug.h"
#include "gdt.h"
#include "graphic.h"
#include "hrbbin.h"
#include "memory.h"
#include "msg_q.h"
#include "paging.h"
#include "str.h"
#include "timer.h"


#define TASK_FLG_FREE     0   ///< 割り当てなし
#define TASK_FLG_ALLOC    1   ///< 割り当て済み
#define TASK_FLG_RUNNING  2   ///< 動作中

#define TSS_REG_SIZE 104  ///< TSS のレジスタ保存部のサイズ

#define EFLAGS_INT_ENABLE   0x0202  ///< 割り込みが有効
#define EFLAGS_INT_DISABLE  0x0002  ///< 割り込みが無効

#define TASK_NAME_MAX  16   ///< タスク名の最大長 + '\0'

typedef struct TSS {
    // ---- レジスタ保存部

    short backlink; short f1;
    long esp0; unsigned short ss0; short f2;
    long esp1; unsigned short ss1; short f3;
    long esp2; unsigned short ss2; short f4;
    unsigned long cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    unsigned short es, f5, cs, f6, ss, f7, ds, f8, fs, f9, gs, f10;
    unsigned short ldt, f11, t, iobase;


    // ---- OS 用タスク管理情報部

    int pid;  ///< プロセス ID
    int flags;

    int ppid;  ///< 親 PID

    /* ページディレクトリ。CR3に入っているのは
     * 物理アドレスなのでアクセスできない。こっちは論理アドレス */
    PDE *pd;

    char name[TASK_NAME_MAX];
    int timeslice_ms;

    // メモリ解放用
    void *code;
    void *data;
    void *stack0;

    FTE file_table[FILE_TABLE_SIZE];
} __attribute__ ((__packed__)) TSS;


typedef struct TASK_MNG {
    /// 記憶領域の確保用。
    /// tss のインデックスと PID は同じ
    TSS tss[TASK_MAX];

    int num_running;
    int cur_run;  ///< 現在実行しているタスクの run でのインデックス
    TSS *run[TASK_MAX];
} TASK_MNG;


int g_root_pid;
int g_idle_pid;
int g_dbg_pid;
int g_con_pid;


static TASK_MNG l_mng;

static TSS *pid2tss(int pid);
static int pid2tss_sel(int pid);


extern int timer_ts_tid(void);

static void idle_main(void);

static void init_tss_seg(void);
static void set_os_tss(int pid, void (*f)(void), void *esp);
static void set_app_tss(int pid, PDE maddr_pd, PDE vaddr_pd, void (*f)(void), void *esp, void *esp0);
static TSS *set_tss(int pid, int cs, int ds, PDE cr3, PDE pd,
        void (*f)(void), unsigned long eflags, void *esp,
        int ss, void *esp0, int ss0);

//=============================================================================
// 公開関数

void task_init(void)
{
    // ---- タスクの初期化

    l_mng.num_running = 0;
    l_mng.cur_run = 0;

    for (int pid = 0; pid < TASK_MAX; pid++) {
        TSS *t = &l_mng.tss[pid];

        t->pid = pid;
        t->flags = TASK_FLG_FREE;
        t->ppid = 0;
        memset(t->name, 0, TASK_NAME_MAX);
        memset(t->file_table, 0, FILE_TABLE_SIZE * sizeof(FTE));
    }


    // ---- タスクを作成

    init_tss_seg();

    g_root_pid = run_os_task("root",    0,            20, false);
    g_idle_pid = run_os_task("idle",    idle_main,    10, true);

    ltr(pid2tss_sel(g_root_pid));


    // ---- タスク切り替え用タイマをスタート

    int ts_tid = timer_ts_tid();
    timer_start(ts_tid, 20);
}


int task_run_app(void *p, unsigned int size, const char *name)
{
    HRB_HEADER *hdr = (HRB_HEADER *) p;

    int stack_and_data_size  = hdr->stack_and_data_size;
    int esp                  = hdr->dst_data;
    int bss_size             = hdr->bss_size;
    int data_size            = hdr->data_size;
    int data_addr            = hdr->src_data;

    char app_name[9];
    memcpy(app_name, name, 8);
    app_name[8] = 0;

    int pid = task_new(app_name);

    /* .text */
    char *p_code = mem_alloc_user(0, size);
    memcpy(p_code, p, size);

    /* .data */
    char *p_data = (char *) mem_alloc_user((void *) esp, stack_and_data_size);
    memcpy(p_data, p + data_addr, data_size);

    /* .bss */
    memset(p_data + data_size, 0, bss_size);

    /* stack */
    unsigned char *stack0 = mem_alloc(64 * 1024);
    unsigned char *esp0 = stack0 + (64 * 1023);

    PDE *pd = create_user_pd();
    set_app_tss(pid, (PDE) paging_get_maddr(pd), (PDE) pd, (void (*)(void)) 0x1B, (void *) esp + stack_and_data_size, (void *) esp0);

    TSS *t = pid2tss(pid);
    t->code   = p_code;
    t->data   = p_data;
    t->stack0 = stack0;

    task_run(pid, 20);

    return pid;
}


int task_new(char *name)
{
    for (int pid = 0; pid < TASK_MAX; pid++) {
        TSS *t = &l_mng.tss[pid];

        if (t->flags == TASK_FLG_FREE) { // 未割り当て領域を発見
            msg_q_init(pid);

            memcpy(t->name, name, TASK_NAME_MAX - 1);
            t->flags = TASK_FLG_ALLOC;
            t->ppid = get_pid();

            return pid;
        }
    }

    // もう全部使用中
    DBGF("could not allocate new task.");
    return ERROR_PID;
}


int task_free(int pid, int exit_status)
{
    if (is_os_task(pid)) {
        return -1;
    }

    TSS *t = pid2tss(pid);

    if (t == 0) {
        return -2;
    }

    task_sleep(pid);

    app_area_copy(t->pd);

    mem_free_user(t->code);
    mem_free_user(t->data);
    mem_free_user(t->stack0);
    mem_free(t->pd);

    app_area_clear();

    timer_task_free(pid);

    free_surface_task(pid);

    t->flags = TASK_FLG_FREE;

    // 親タスクにメッセージで終了を通知
    TSS *parent = pid2tss(t->ppid);
    if (parent != 0) {
        MSG msg;
        msg.message = MSG_NOTIFY_CHILD_EXIT;
        msg.u_param = pid;
        msg.l_param = exit_status;

        msg_q_put(t->ppid, &msg);
    }

    return 0;
}


void task_run(int pid, int timeslice_ms)
{
    TSS *t = pid2tss(pid);

    if (t == 0 || t->flags == TASK_FLG_RUNNING) {
        return;
    }

    t->flags = TASK_FLG_RUNNING;
    t->timeslice_ms = timeslice_ms;
    l_mng.run[l_mng.num_running] = t;
    l_mng.num_running++;
}


void task_switch(int ts_tid)
{
    if (l_mng.num_running <= 1) {
        // idle プロセスのみ
        timer_start(ts_tid, 20);
    } else {
        l_mng.cur_run++;
        if (l_mng.cur_run >= l_mng.num_running) {
            l_mng.cur_run = 0;
        }

        TSS *t = l_mng.run[l_mng.cur_run];

        timer_start(ts_tid, t->timeslice_ms);

        far_jmp(pid2tss_sel(t->pid), 0);
    }
}


/// 実行中タスクリスト(run)からタスクをはずす
void task_sleep(int pid)
{
    TSS *t = pid2tss(pid);

    if (t == 0) {
        return;
    }

    if (t->flags != TASK_FLG_RUNNING) {
        return;
    }

    char ts = 0;  // タスクスイッチフラグ
    if (t == l_mng.run[l_mng.cur_run]) {
        // 現在実行中のタスクをスリープさせるので、あとでタスクスイッチする
        ts = 1;
    }

    int i_rt;  // l_mng.run での t のインデックス
    for (i_rt = 0; i_rt < l_mng.num_running; i_rt++) {
        if (l_mng.run[i_rt] == t) {
            break;
        }
    }

    if (i_rt >= l_mng.num_running) {
        // ここに来たらOSのバグ
        DBGF("[task_sleep] FOUND BUG!!");
        i_rt = l_mng.num_running - 1;
    }

    // cur_run をずらす
    if (i_rt < l_mng.cur_run) {
        l_mng.cur_run--;
    }

    l_mng.num_running--;

    // run をずらす
    for ( ; i_rt < l_mng.num_running; i_rt++) {
        l_mng.run[i_rt] = l_mng.run[i_rt + 1];
    }

    t->flags = TASK_FLG_ALLOC;

    if (ts == 1) {
        // **** タスクスイッチする

        if (l_mng.cur_run >= l_mng.num_running) {
            l_mng.cur_run = 0;
        }

        TSS *t = l_mng.run[l_mng.cur_run];
        far_jmp(pid2tss_sel(t->pid), 0);
    }
}


/// タスクが寝てたら起こす
void task_wakeup(int pid)
{
    TSS *t = pid2tss(pid);

    if (t == 0) {
        return;
    }

    if (t->flags != TASK_FLG_ALLOC) {
        return;
    }

    task_run(pid, t->timeslice_ms);
}


int get_pid(void)
{
    TSS *t = l_mng.run[l_mng.cur_run];

    /* task_init が呼ばれる前はpid=0とする */
    if (t == 0) {
        return 0;
    }

    return t->pid;
}


int get_free_fd(void)
{
    TSS *t = l_mng.run[l_mng.cur_run];

    for (int fd = 0; fd < FILE_TABLE_SIZE; fd++) {
        if (t->file_table[fd].flags == FILE_FLG_FREE) {
            return fd;
        }
    }

    return -1;
}


FILE_T *task_get_file(int fd)
{
    TSS *t = l_mng.run[l_mng.cur_run];
    return t->file_table[fd].file;
}


int task_set_file(int fd, FILE_T *f)
{
    TSS *t = l_mng.run[l_mng.cur_run];
    t->file_table[fd].file = f;

    return 0;
}


const char *task_get_name(int pid)
{
    TSS *t = pid2tss(pid);

    if (t == 0) {
        return "ERROR";
    }

    if (t->flags == TASK_FLG_FREE) {
        return "ERROR";
    }

    return t->name;
}


void task_set_pt(int i_pd, PDE pt)
{
    for (int pid = 0; pid < TASK_MAX; pid++) {
        TSS *t = &l_mng.tss[pid];

        if (t->flags != TASK_FLG_FREE && t->pd != 0) {
            t->pd[i_pd] = pt;
        }
    }
}


void task_dbg(void)
{
    dbgf("\nnum running : %d\n", l_mng.num_running);

    for (int pid = 0; pid < TASK_MAX; pid++) {
        TSS *t = &l_mng.tss[pid];

        if (t->flags != TASK_FLG_FREE) {
            dbgf("%d %s, pd : %p, cs : %X, ds : %X, ss : %X\n",
                    pid, t->name, t->pd, t->cs, t->ds, t->ss);
        }
    }

    dbgf("\n");
}


int is_os_task(int pid)
{
    TSS *t = pid2tss(pid);

    if (t->cr3 == MADDR_OS_PDT) {
        return 1;
    } else {
        return 0;
    }
}


int run_os_task(char *name,
        void (*main)(void), int timeslice_ms, bool create_esp)
{
    int pid = task_new(name);

    unsigned char *esp = 0;

    if (create_esp) {
        esp = mem_alloc(64 * 1024);
        esp += 64 * 1024;
    }

    set_os_tss(pid, main, esp);

    task_run(pid, timeslice_ms);

    return pid;
}



//=============================================================================
// 非公開関数

static void idle_main(void)
{
    for (;;) {
        hlt();
    }
}


static void init_tss_seg(void)
{
    for (int pid = 0; pid < TASK_MAX; pid++) {
        unsigned long tss = (unsigned long) &l_mng.tss[pid];

        set_seg_desc(SEG_TSS + pid, tss, TSS_REG_SIZE,
                SEG_TYPE_TSS, 0, /* dpl = */ 0);
    }
}


static void set_os_tss(int pid, void (*f)(void), void *esp)
{
    TSS *tss = set_tss(pid, KERNEL_CS, KERNEL_DS, MADDR_OS_PDT, VADDR_OS_PDT, f,
            EFLAGS_INT_ENABLE, esp, KERNEL_DS, 0, 0);
}


static void set_app_tss(int pid, PDE maddr_pd, PDE vaddr_pd, void (*f)(void), void *esp, void *esp0)
{
    // | 3 は要求者特権レベルを3にするため
    TSS *tss = set_tss(pid, USER_CS | 3, USER_DS, maddr_pd, vaddr_pd, f,
            EFLAGS_INT_ENABLE, esp, USER_DS | 3, esp0, KERNEL_DS);

    tss->file_table[0].flags = FILE_FLG_USED;
    tss->file_table[0].file  = f_keyboard;
    tss->file_table[1].flags = FILE_FLG_USED;
    tss->file_table[1].file  = f_console;
    tss->file_table[2].flags = FILE_FLG_USED;
    tss->file_table[2].file  = f_console;
}



static TSS *set_tss(int pid, int cs, int ds, PDE cr3, PDE pd,
        void (*f)(void), unsigned long eflags, void *esp,
        int ss, void *esp0, int ss0)
{
    if (pid < 0 || TASK_MAX <= pid) {
        return;
    }

    TSS *tss = &l_mng.tss[pid];
    memset(tss, 0, TSS_REG_SIZE);

    tss->cr3 = cr3;
    tss->pd = (PDE *) pd;

    // タスク実行開始時のレジスタ内容を設定する
    tss->cs = cs;
    tss->eip = (unsigned long) f;
    tss->eflags = eflags;
    tss->esp = (unsigned long) esp;
    tss->ds = tss->es = tss->fs = tss->gs = ds;
    tss->ss = ss;

    // 特権レベル0に移行したときのスタック領域を設定する
    tss->esp0 = (unsigned long) esp0;
    tss->ss0 = ss0;

    return tss;
}


static TSS *pid2tss(int pid)
{
    if (pid < 0 || TASK_MAX <= pid) {
        return 0;
    }

    return &l_mng.tss[pid];
}


static int pid2tss_sel(int pid)
{
    return (SEG_TSS + pid) << 3;
}
