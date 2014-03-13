// Generated by mkhdr

#ifndef HEADER_TASK
#define HEADER_TASK

#include <stdbool.h>
#include "api.h"
#include "file.h"
#include "memory.h"
#include "paging.h"

#define ERROR_PID        (-1)

#define TASK_MAX         (32)  // 最大タスク数

#define TASK_FLG_FREE     (0)   // 割り当てなし
#define TASK_FLG_ALLOC    (1)   // 割り当て済み
#define TASK_FLG_RUNNING  (2)   // 動作中
#define TASK_FLG_KERNEL   (16)  // カーネルタスク

#define TSS_REG_SIZE (104)  // TSS のレジスタ保存部のサイズ

#define EFLAGS_INT_ENABLE   (0x0202)  // 割り込みが有効
#define EFLAGS_INT_DISABLE  (0x0002)  // 割り込みが無効

#define TASK_NAME_MAX  (16)   // タスク名の最大長 + '\0'

#define DEFAULT_STACK0_SIZE  (8 * 1024)
#define DEFAULT_TIMESLICE_MS  (20)


struct TSS {
    // ---- レジスタ保存部

    short backlink; short f1;
    long esp0; unsigned short ss0; short f2;
    long esp1; unsigned short ss1; short f3;
    long esp2; unsigned short ss2; short f4;
    unsigned long cr3, eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    unsigned short es, f5, cs, f6, ss, f7, ds, f8, fs, f9, gs, f10;
    unsigned short ldt, f11, t, iobase;


    // ---- OS 用タスク管理情報部

    int pid;   // プロセス ID
    int ppid;  // 親 PID

    unsigned int flags;

    /* ページディレクトリ。CR3に入っているのは
     * 物理アドレスなのでアクセスできない。こっちは論理アドレス */
    PDE *pd;

    char name[TASK_NAME_MAX];
    int timeslice_ms;

    // メモリ
    struct USER_PAGE *code;
    struct USER_PAGE *data;
    unsigned long brk;
    struct USER_PAGE *stack;
    // OSタスクなら普通のスタック。アプリならOS権限時のスタック
    unsigned long stack0;

    // ファイルテーブル
    struct FILE_TABLE_ENTRY *file_tbl;
} __attribute__ ((__packed__));


struct TASK_MNG {
    // 記憶領域の確保用。
    // tss のインデックスと PID は同じ
    struct TSS tss[TASK_MAX];

    int num_running;
    int cur_run;  // 現在実行しているタスクの run でのインデックス
    struct TSS *run[TASK_MAX];
};


void task_init(void);
int  task_new(const char *name);
int  task_free(int pid, int exit_status);
int  task_copy(struct API_REGISTERS *regs, int flg);
int  kernel_thread(int (*fn)(void), int flg);
int  task_exec(struct API_REGISTERS *regs, const char *fname);
void task_run(int pid);
void task_switch(void);
void task_sleep(int pid);
void task_wakeup(int pid);
void task_set_pt(int i_pd, unsigned long pt);
int task_set_brk(unsigned long brk);

const char *task_get_name(int pid);
void task_set_name(const char *name);

void task_dbg(void);

bool is_os_task(int pid);

void set_app_tss(int pid, PDE vaddr_pd, void (*f)(void), unsigned long esp, unsigned long esp0);
struct TSS *pid2tss(int pid);

extern struct TSS *g_cur;
extern int g_pid;

extern int g_root_pid;
extern int g_idle_pid;

#endif
