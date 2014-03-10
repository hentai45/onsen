/**
 * デバッグ
 */


//=============================================================================
// 公開ヘッダ

#ifndef HEADER_DEBUG
#define HEADER_DEBUG

#include <stdarg.h>
#include "asmfunc.h"
#include "file.h"
#include "stacktrace.h"

//-----------------------------------------------------------------------------
// メイン

void debug_main(void);


//-----------------------------------------------------------------------------
// 画面出力

struct INT_REGISTERS {
    // asm_inthander.S で積まれたスタックの内容
    unsigned int edi, esi, ebp, esp, ebx, edx, ecx, eax;  // pushal
    unsigned int ds, es;

    // 以下は、例外発生時にCPUが自動でpushしたもの
    unsigned int err_code;
    unsigned int eip, cs, eflags, app_esp, app_ss;
};


void temp_dbgf(const char *fmt, ...);
void dbgf(const char *fmt, ...);
void dbg_clear(void);
void dbg_seg(void);

void blue_screen(void);
void blue_screen_f(int line_no, const char *fmt, ...);

void dbg_fault(const char *msg, int no, struct INT_REGISTERS *regs);

extern struct FILE_T *f_debug;
extern struct FILE_T *f_dbg_temp;

extern int g_dbg_temp_flg;


#define ASSERT(cond, fmt, ...) do {                                           \
    if (!(cond)) {                                                            \
        dbgf("**** ASSERT ****\n");                                           \
        dbgf("FILE: %s, FUNC: %s, LINE: %d\n", __FILE__, __func__, __LINE__); \
        dbgf("COND: %s\n", #cond);                                            \
        dbgf(fmt "\n", ##__VA_ARGS__);                                        \
        stacktrace(5, f_debug);                                               \
        for (;;) { hlt(); }                                                   \
    }                                                                         \
} while (0)


// graphic_init前でも使えるASSERT
#define ASSERT2(cond, fmt, ...) do {                                                    \
    if (!(cond)) {                                                                      \
        blue_screen();                                                                  \
        blue_screen_f(0, "**** ASSERT ****");                                           \
        blue_screen_f(1, "FILE: %s, FUNC: %s, LINE: %d", __FILE__, __func__, __LINE__); \
        blue_screen_f(2, "COND: %s", #cond);                                            \
        blue_screen_f(3, fmt, ##__VA_ARGS__);                                           \
        for (;;) { hlt(); }                                                             \
    }                                                                                   \
} while (0)


#define ERROR(fmt, ...) do {                                              \
    dbgf("**** ERROR ****\n");                                            \
    dbgf("FILE: %s, FUNC: %s, LINE: %d\n", __FILE__, __func__, __LINE__); \
    dbgf(fmt "\n", ##__VA_ARGS__);                                        \
    stacktrace(5, f_debug);                                               \
    for (;;) { hlt(); }                                                   \
} while (0)


#define DBGF(fmt, ...)  dbgf("%s %d %s : " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)


#endif


//=============================================================================
// 非公開ヘッダ

#include "graphic.h"
#include "msg_q.h"
#include "str.h"
#include "task.h"

static COLOR32 fg = COL_WHITE;
static COLOR32 bg = COL_BLACK;

//-----------------------------------------------------------------------------
// メイン

static void debug_proc(unsigned int msg, unsigned long u_param, long l_param);


//-----------------------------------------------------------------------------
// 画面出力


#define WIDTH_CH   (80)
#define HEIGHT_CH  (28)

static int l_sid = ERROR_SID;

static int is_initialized = 0;

static char l_buf[HEIGHT_CH][WIDTH_CH + 1];
static int l_x = 0;
static int l_y = 0;

#define TMP_SIZE  (4096)
static char l_tmp[TMP_SIZE];

static int dbg_write(void *self, const void *buf, int cnt);

struct FILE_T l_f_debug = { .write = dbg_write };
struct FILE_T *f_debug = &l_f_debug;

static int dbg_temp_read(void *self, void *buf, int cnt);
static int dbg_temp_write(void *self, const void *buf, int cnt);

struct FILE_T l_f_dbg_temp = { .read = dbg_temp_read, .write = dbg_temp_write };
struct FILE_T *f_dbg_temp = &l_f_dbg_temp;


//=============================================================================
// 公開関数

//-----------------------------------------------------------------------------
// メイン

static void update_all(void);

void debug_main(void)
{
    int w = WIDTH_CH * HANKAKU_W;
    int h = HEIGHT_CH * HANKAKU_H;
    l_sid = new_window(20, 280, w, h, "debug");

    is_initialized = 1;

    update_all();

    struct MSG msg;

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

    fill_surface(l_sid, bg);

    for (int y = 0; y < HEIGHT_CH; y++) {
        draw_text_bg(l_sid, 0, y * 16, fg, bg, l_buf[y]);
    }

    update_surface(l_sid);
}

static void update(void)
{
    if (! is_initialized) {
        return;
    }

    char s[2];
    s[0] = l_buf[l_y][l_x];
    s[1] = 0;

    draw_text_bg(l_sid, l_x * 8, l_y * 16, fg, bg, s);
    update_rect(l_sid, l_x * 8, l_y * 16, 8, 16);
}

static void scroll(int n)
{
    for (int y = 0; y < HEIGHT_CH - n; y++) {
        memcpy(l_buf[y], l_buf[y + n], WIDTH_CH + 1);
    }

    for (int y = HEIGHT_CH - n; y < HEIGHT_CH; y++) {
        l_buf[y][0] = 0;
    }

    update_all();
}


static void buf_newline(void)
{
    l_y++;
    l_x = 0;

    if (l_y >= HEIGHT_CH) {
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

    if (l_x >= WIDTH_CH) {
        buf_newline();
    }
}

static void buf_str(char *s)
{
    for (int i = 0; i < strlen(s); i++) {
        buf_ch(s[i]);
    }
}


//-----------------------------------------------------------------------------
// 画面出力

// 画面更新関数の中でも使える
void temp_dbgf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f_dbg_temp, fmt, ap);
    va_end(ap);
}


void dbgf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(l_tmp, TMP_SIZE, fmt, ap);
    va_end(ap);

    buf_str(l_tmp);
}


void dbg_clear(void)
{
    for (int y = 0; y < HEIGHT_CH; y++) {
        l_buf[y][0] = 0;
    }

    l_x = l_y = 0;

    update_all();
}


void blue_screen(void)
{
    fill_surface(g_vram_sid, 0x0000B0);
}


void blue_screen_f(int line_no, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(l_tmp, TMP_SIZE, fmt, ap);
    va_end(ap);

    draw_text(g_vram_sid, 8, 8 + (line_no * HANKAKU_H), COL_WHITE, l_tmp);
}


void dbg_seg(void)
{
    unsigned short cs, ds, ss;
    unsigned long esp;

    __asm__ __volatile__ ("movw %%cs, %0" : "=m" (cs));
    __asm__ __volatile__ ("movw %%ds, %0" : "=m" (ds));
    __asm__ __volatile__ ("movw %%ss, %0" : "=m" (ss));
    __asm__ __volatile__ ("movl %%esp, %0" : "=m" (esp));

    dbgf("CS = %d * 8 + %d", cs >> 3, cs & 0x07);
    dbgf(", DS = %d * 8 + %d", ds >> 3, ds & 0x07);
    dbgf(", SS = %d * 8 + %d", ss >> 3, ss & 0x07);
    dbgf(", ESP = %X\n", esp);
}


// 例外（フォールト）が発生したときにデバッグ表示する用の関数
void dbg_fault(const char *msg, int no, struct INT_REGISTERS *regs)
{
    unsigned short bk_fg = fg;
    unsigned short bk_bg = bg;
    fg = COL_RED;
    bg = COL_WHITE;

    dbgf("\n\n%s\n\n", msg);

    // ---- PID とプロセス名を表示

    dbgf("PID = %d, name = %s\n\n", g_pid, g_cur->name);

    // ---- レジスタの内容を表示

    dbgf("EAX = %X, EBX = %X, ECX = %X, EDX = %X\n",
            regs->eax, regs->ebx, regs->ecx, regs->edx);

    dbgf("EBP = %X, ESI = %X, EDI = %X, ESP = %X\n\n",
            regs->ebp, regs->esi, regs->edi, regs->esp);

    dbgf("ERROR CODE = %X", regs->err_code);
    if (no == /* page fault */ 0x0E) {
        if (regs->err_code & 16)
            dbgf(" [Present]");

        if (regs->err_code & 8)
            dbgf(" [Write]");

        if (regs->err_code & 4)
            dbgf(" [User]");

        if (regs->err_code & 2)
            dbgf(" [Reserved write]");

        if (regs->err_code & 1)
            dbgf(" [Instruction Fetch]");

        unsigned long cr2, cr3;
        __asm__ __volatile__ ("movl %%cr2, %0" : "=r" (cr2));
        __asm__ __volatile__ ("movl %%cr3, %0" : "=r" (cr3));
        dbgf("\nCR2 = %X, CR3 = %X\n", cr2, cr3);
    } else {
        dbgf("\n");
    }

    dbgf("EIP = %X", regs->eip);
    char *name = get_func_name(regs->eip);
    if (name) {
        dbgf(", %s\n", name);
    } else {
        dbgf("\n");
    }

    dbgf("CS = %d * 8 + %d", regs->cs >> 3, regs->cs & 0x07);
    dbgf(", DS = %d * 8 + %d", regs->ds >> 3, regs->ds & 0x07);
    dbgf(", ES = %d * 8 + %d\n", regs->es >> 3, regs->es & 0x07);

    dbgf("EFLAGS = %X", regs->eflags);
    int intr_flg = (regs->eflags & 0x0200) ? 1 : 0;
    dbgf("  IF = %d\n", intr_flg);

    dbgf("APP ESP = %X", regs->app_esp);
    dbgf(", APP SS = %d * 8 + %d\n\n", regs->app_ss >> 3, regs->app_ss & 0x07);

    if (g_cur->is_os_task) {
        stacktrace2(5, f_debug, (unsigned int *) regs->ebp);
    }

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

#include "asmfunc.h"

static int dbg_write(void *self, const void *buf, int cnt)
{
    dbgf("%.*s", cnt, (char *) buf);
    return cnt;
}

static int l_temp_buf_i = 0;
static char l_temp_buf[4096];
int g_dbg_temp_flg = 0;

static int dbg_temp_read(void *self, void *buf, int cnt)
{
    int num_read = (l_temp_buf_i < cnt) ? l_temp_buf_i : cnt;

    num_read = snprintf((char *) buf, cnt, "%.*s", num_read, l_temp_buf);

    int from = num_read;
    int to   = 0;

    while (from < 4096) {
        l_temp_buf[to++] = l_temp_buf[from++];
    }

    l_temp_buf_i -= num_read;

    return num_read;
}


static int dbg_temp_write(void *self, const void *buf, int cnt)
{
    if (g_dbg_temp_flg)
        return 0;

    int num_write = snprintf(l_temp_buf + l_temp_buf_i, 4096 - l_temp_buf_i,
            "%.*s", cnt, (char *) buf);
    l_temp_buf_i += num_write;
    return num_write;
}

