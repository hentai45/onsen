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
void dbg_uint(unsigned int n);
void dbg_intx(unsigned int n);
void dbg_intxln(unsigned int n);
void dbg_bits(unsigned int n);
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

static unsigned short fg = COL_WHITE;
static unsigned short bg = COL_BLACK;

//-----------------------------------------------------------------------------
// メイン

static void debug_proc(unsigned int msg, unsigned long u_param, long l_param);


//-----------------------------------------------------------------------------
// 画面出力

// 例外発生時に CPU が自動で push するレジスタ
typedef struct REGISTER_FAULT {
    int err_code, eip, cs, eflags, app_esp, app_ss;
} REGISTER_FAULT;


#define WIDTH  (80)
#define HEIGHT (30)

static int is_initialized = 0;

static char l_buf[HEIGHT][WIDTH + 1];
static int l_x = 0;
static int l_y = 0;


static void dbg_fault_reg(const REGISTER_FAULT *r);


//=============================================================================
// 公開関数

//-----------------------------------------------------------------------------
// メイン

static void update_all(void);

void debug_main(void)
{
    /*
    fill_surface(g_dbg_sid, bg);
    update_screen(g_dbg_sid);
    */
    is_initialized = 1;
    update_all();

    MSG msg;

    while (get_message(&msg)) {
        dispatch_message(&msg, debug_proc);
    }
}

//-----------------------------------------------------------------------------
// バッファ処理

static void update(void);
static void scroll(int n);

static void update_all(void)
{
    if (! is_initialized) {
        return;
    }

    fill_surface(g_dbg_sid, bg);

    for (int y = 0; y < HEIGHT; y++) {
        draw_text_bg(g_dbg_sid, 0, y * 16, fg, bg, l_buf[y]);
    }

    update_screen(g_dbg_sid);
}

static void update(void)
{
    if (! is_initialized) {
        return;
    }

    char s[2];
    s[0] = l_buf[l_y][l_x];
    s[1] = 0;

    draw_text_bg(g_dbg_sid, l_x * 8, l_y * 16, fg, bg, s);
    update_rect(g_dbg_sid, l_x * 8, l_y * 16, 8, 16);
}

static void scroll(int n)
{
    for (int y = 0; y < HEIGHT - n; y++) {
        memcpy(l_buf[y], l_buf[y + n], WIDTH + 1);
    }

    for (int y = HEIGHT - n; y < HEIGHT; y++) {
        l_buf[y][0] = 0;
    }

    update_all();
}


static void buf_newline(void)
{
    l_y++;
    l_x = 0;

    if (l_y >= HEIGHT) {
        scroll(5);

        l_y -= 5;
    }
}

static void buf_ch(char ch)
{
    if (ch == '\n') {
        buf_newline();
        return;
    }

    l_buf[l_y][l_x] = ch;
    update();
    l_x++;

    l_buf[l_y][l_x] = 0;

    if (l_x >= WIDTH) {
        buf_newline();
    }
}

static void buf_str(char *s)
{
    for (int i = 0; i < s_len(s); i++) {
        buf_ch(s[i]);
    }
}


//-----------------------------------------------------------------------------
// 画面出力

void dbg_newline(void)
{
    buf_newline();
}


void dbg_char(char ch)
{
    buf_ch(ch);
}


void dbg_str(const char *s)
{
    buf_str(s);
}


void dbg_strln(const char *s)
{
    buf_str(s);
    buf_newline();
}


void dbg_addr(void *p)
{
    buf_str("0x");
    dbg_intx((unsigned int) p);
}


void dbg_int(int n)
{
    char s[32];
    s_itoa(n, s);
    buf_str(s);
}


void dbg_intln(int n)
{
    dbg_int(n);
    buf_newline();
}


void dbg_intx(unsigned int n)
{
    char s[9];
    s_itox(n, s, 8);
    buf_str(s);
}


void dbg_intxln(unsigned int n)
{
    dbg_intx(n);
    buf_newline();
}


void dbg_uint(unsigned int n)
{
    char s[32];
    s_uitoa(n, s);
    buf_str(s);
}


void dbg_bits(unsigned int n)
{
    int i;

    for (i = 31; i >= 0; i--) {
        if (n & (1 << i)) {
            dbg_int(1);
        } else {
            dbg_int(0);
        }

        if ((i & 0x7) == 0) {
            buf_str(" ");
        }

        if ((i & 0x3) == 0) {
            buf_str(" ");
        }
    }
}


void dbg_reg(const REGISTER *r)
{
    buf_str("EAX = ");
    dbg_intx(r->eax);

    buf_str(", EBX = ");
    dbg_intx(r->ebx);

    buf_str(", ECX = ");
    dbg_intx(r->ecx);

    buf_str(", EDX = ");
    dbg_intx(r->edx);
    buf_newline();


    buf_str("EBP = ");
    dbg_intx(r->ebp);

    buf_str(", ESI = ");
    dbg_intx(r->esi);

    buf_str(", EDI = ");
    dbg_intx(r->edi);
    buf_newline();
}


void dbg_seg(void)
{
    unsigned short cs, ds, ss;
    unsigned long esp;

    __asm__ __volatile__ ("movw %%cs, %0" : "=m" (cs));
    __asm__ __volatile__ ("movw %%ds, %0" : "=m" (ds));
    __asm__ __volatile__ ("movw %%ss, %0" : "=m" (ss));
    __asm__ __volatile__ ("movl %%esp, %0" : "=m" (esp));

    buf_str("cs = ");
    dbg_seg_reg(cs);

    buf_str(", ds = ");
    dbg_seg_reg(ds);

    buf_str(", ss = ");
    dbg_seg_reg(ss);

    buf_str(", esp = ");
    dbg_intx(esp);
    buf_newline();
}


/// セグメントレジスタを "ds = 1003 * 8 + 3" のような形式で表示する
void dbg_seg_reg(unsigned short seg_reg)
{
    dbg_int(seg_reg >> 3);
    buf_str(" * 8");

    if (seg_reg & 0x07) {
        buf_str(" + ");
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

    buf_str("DS = ");
    dbg_seg_reg(esp[8]);
    buf_str(", ES = ");
    dbg_seg_reg(esp[9]);
    buf_newline();

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
    buf_str("ERROR CODE = ");
    dbg_intx(r->err_code);
    buf_newline();


    buf_str("EIP = ");
    dbg_intx(r->eip);
    buf_str(", CS = ");
    dbg_seg_reg(r->cs);
    buf_newline();


    buf_str("EFLAGS = ");
    dbg_intx(r->eflags);
    buf_str("  IF = ");
    int intr_flg = (r->eflags & 0x0200) ? 1 : 0;
    dbg_int(intr_flg);
    buf_newline();


    buf_str("APP ESP = ");
    dbg_intx(r->app_esp);
    buf_str(", APP SS = ");
    dbg_seg_reg(r->app_ss);
    buf_newline();
}

