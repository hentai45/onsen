/**
 * デバッグ
 *
 * @file debug.c
 * @author Ivan Ivanovich Ivanov
 */

/*
 * ＜目次＞
 * ・メイン
 * ・画面出力
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_DEBUG
#define HEADER_DEBUG


//-----------------------------------------------------------------------------
// メイン

void debug_main(void);


//-----------------------------------------------------------------------------
// 画面出力

// pushal でスタックに格納されるレジスタ
typedef struct REGISTER {
    int edi, esi, ebp, ebx, edx, ecx, eax;
} REGISTER;


#define DBG_HEADER do {                \
    dbg_str(__FILE__ " ");             \
    dbg_int(__LINE__);                 \
    dbg_str(" ");                      \
    dbg_str(__func__);                 \
    dbg_str(" : ");                    \
} while (0)


#define DBG_STR(str) do {              \
    DBG_HEADER;                        \
    dbg_str(str);                      \
    dbg_str("\n");                     \
} while (0)


#define DBG_INT(var) do {              \
    DBG_HEADER;                        \
    dbg_str(#var " = ");               \
    dbg_int(var);                      \
    dbg_str("\n");                     \
} while (0)


#define DBG_INTX(var) do {             \
    DBG_HEADER;                        \
    dbg_str(#var " = 0x");             \
    dbg_intx(var);                     \
    dbg_str("\n");                     \
} while (0)



void dbg_newline(void);
void dbg_char(char ch);
void dbg_str(const char *s);
void dbg_strln(const char *s);
void dbg_addr(void *p);
void dbg_int(int n);
void dbg_intln(int n);
void dbg_intx(unsigned int n);
void dbg_intxln(unsigned int n);
void dbg_reg(const REGISTER *r);
void dbg_seg(void);

void dbg_seg_reg(unsigned short seg_reg);

void dbg_fault(const char *msg, int *esp);

#endif


//=============================================================================
// 非公開ヘッダ

#include "graphic.h"
#include "msg_q.h"
#include "str.h"
#include "task.h"


unsigned short fg = COL_WHITE;
unsigned short bg = COL_BLACK;

//-----------------------------------------------------------------------------
// メイン

static void debug_proc(unsigned int msg, unsigned long u_param, long l_param);


//-----------------------------------------------------------------------------
// 画面出力

// 例外発生時に CPU が自動で push するレジスタ
typedef struct REGISTER_FAULT {
    int err_code, eip, cs, eflags, app_esp, app_ss;
} REGISTER_FAULT;


static int x_ch = 0;
static int y_ch = 0;


static void dbg_fault_reg(const REGISTER_FAULT *r);


//=============================================================================
// 公開関数

//-----------------------------------------------------------------------------
// メイン

void debug_main(void)
{
    fill_surface(g_dbg_sid, bg);
    update_screen(g_dbg_sid);

    dbg_str("DEBUG SCREEN\n");

    MSG msg;

    while (get_message(&msg)) {
        dispatch_message(&msg, debug_proc);
    }
}


//-----------------------------------------------------------------------------
// 画面出力

void dbg_newline(void)
{
    y_ch++;
    x_ch = 0;

    if (y_ch >= 30) {
        scroll_surface(g_dbg_sid, 0, -16);
        fill_rect(g_dbg_sid, 0, g_h - 16, g_w, 16, COL_BLACK);
        update_screen(g_dbg_sid);

        y_ch = 29;
    }
}


void dbg_char(char ch)
{
    if (ch == '\n') {
        dbg_newline();
        return;
    }

    char s[2];
    s[0] = ch;
    s[1] = 0;

    draw_text_bg(g_dbg_sid, x_ch * 8, y_ch * 16, fg, bg, s);
    update_rect(g_dbg_sid, x_ch * 8, y_ch * 16, 8, 16);

    x_ch++;
    if (x_ch >= 80) {
        dbg_newline();
    }
}


void dbg_str(const char *s)
{
    for (int i = 0; i < s_len(s); i++) {
        dbg_char(s[i]);
    }
}


void dbg_strln(const char *s)
{
    dbg_str(s);
    dbg_newline();
}


void dbg_addr(void *p)
{
    dbg_str("0x");
    dbg_intx((unsigned int) p);
}


void dbg_int(int n)
{
    char s[32];
    s_itoa(n, s);
    dbg_str(s);
}


void dbg_intln(int n)
{
    dbg_int(n);
    dbg_newline();
}


void dbg_intx(unsigned int n)
{
    char s[9];
    s_itox(n, s, 8);
    dbg_str(s);
}


void dbg_intxln(unsigned int n)
{
    dbg_intx(n);
    dbg_newline();
}


void dbg_reg(const REGISTER *r)
{
    dbg_str("EAX = ");
    dbg_intx(r->eax);

    dbg_str(", EBX = ");
    dbg_intx(r->ebx);

    dbg_str(", ECX = ");
    dbg_intx(r->ecx);

    dbg_str(", EDX = ");
    dbg_intx(r->edx);
    dbg_newline();


    dbg_str("EBP = ");
    dbg_intx(r->ebp);

    dbg_str(", ESI = ");
    dbg_intx(r->esi);

    dbg_str(", EDI = ");
    dbg_intx(r->edi);
    dbg_newline();
}


void dbg_seg(void)
{
    unsigned short cs, ds, ss;
    unsigned long esp;

    __asm__ __volatile__ ("movw %%cs, %0" : "=m" (cs));
    __asm__ __volatile__ ("movw %%ds, %0" : "=m" (ds));
    __asm__ __volatile__ ("movw %%ss, %0" : "=m" (ss));
    __asm__ __volatile__ ("movl %%esp, %0" : "=m" (esp));

    dbg_str("cs = ");
    dbg_seg_reg(cs);

    dbg_str(", ds = ");
    dbg_seg_reg(ds);

    dbg_str(", ss = ");
    dbg_seg_reg(ss);

    dbg_str(", esp = ");
    dbg_intx(esp);
    dbg_newline();
}


/// セグメントレジスタを "ds = 1003 * 8 + 3" のような形式で表示する
void dbg_seg_reg(unsigned short seg_reg)
{
    dbg_int(seg_reg >> 3);
    dbg_str(" * 8");

    if (seg_reg & 0x07) {
        dbg_str(" + ");
        dbg_int(seg_reg & 0x07);
    }
}


/// 例外（フォールト）が発生したときにデバッグ表示する用の関数
void dbg_fault(const char *msg, int *esp)
{
    unsigned short bk_fg = fg;
    unsigned short bk_bg = bg;
    fg = COL_RED;
    bg = COL_WHITE;

    dbg_str("\n\n");

    dbg_str(msg);
    dbg_str("\n\n");

    // ---- PID とプロセス名を表示

    int pid = get_pid();
    dbg_str("PID = ");
    dbg_int(pid);
    dbg_str(", name = ");
    dbg_str(task_get_name(pid));
    dbg_str("\n\n");


    // ---- レジスタの内容を表示

    dbg_reg((REGISTER *) esp);
    dbg_fault_reg((REGISTER_FAULT *) (esp + 9));

    dbg_str("DS = ");
    dbg_seg_reg(esp[8]);
    dbg_str(", ES = ");
    dbg_seg_reg(esp[9]);
    dbg_newline();

    fg = bk_fg;
    bg = bk_bg;
}


//=============================================================================
// 非公開関数

//-----------------------------------------------------------------------------
// メイン

static void debug_proc(unsigned int msg, unsigned long u_param, long l_param)
{
}


//-----------------------------------------------------------------------------
// 画面出力

static void dbg_fault_reg(const REGISTER_FAULT *r)
{
    dbg_str("ERROR CODE = ");
    dbg_intx(r->err_code);
    dbg_newline();


    dbg_str("EIP = ");
    dbg_intx(r->eip);
    dbg_str(", CS = ");
    dbg_seg_reg(r->cs);
    dbg_newline();


    dbg_str("EFLAGS = ");
    dbg_intx(r->eflags);
    dbg_str("  IF = ");
    int intr_flg = (r->eflags & 0x0200) ? 1 : 0;
    dbg_int(intr_flg);
    dbg_newline();


    dbg_str("APP ESP = ");
    dbg_intx(r->app_esp);
    dbg_str(", APP SS = ");
    dbg_seg_reg(r->app_ss);
    dbg_newline();
}

