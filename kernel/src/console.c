/**
 * コンソール
 */


//=============================================================================
// ヘッダ

#ifndef HEADER_CONSOLE
#define HEADER_CONSOLE

#include "file.h"

int console_main(void);

extern struct FILE_T *f_console;

#endif


#include <stdbool.h>

#include "asmfunc.h"
#include "debug.h"
#include "elf.h"
#include "ext2fs.h"
#include "gdt.h"
#include "graphic.h"
#include "keycode.h"
#include "memory.h"
#include "msg_q.h"
#include "paging.h"
#include "stdarg.h"
#include "str.h"
#include "sysinfo.h"
#include "task.h"
#include "time.h"
#include "timer.h"


#define CURSOR_INTERVAL_MS 500


static COLOR32 fg = COL_WHITE;
static COLOR32 bg = COL_BLACK;

static int cursor_tid;
static int cursor_on = 0;
static bool l_active;

int cons_write(void *self, const void *buf, int cnt);

struct FILE_T l_f_console = { .write = cons_write };
struct FILE_T *f_console = &l_f_console;


//-----------------------------------------------------------------------------
// メイン

static void console_proc(unsigned int msg, unsigned long u_param, long l_param);


//-----------------------------------------------------------------------------
// メッセージ処理ハンドラ

static void cursor_timer_handler(int tid);
static void keydown_handler(int keycode);
static void keychar_handler(int ch);


//-----------------------------------------------------------------------------
// 画面出力

#define MAX_LINE_CHAR 1024

#define WIDTH_CH   39
#define HEIGHT_CH  28

#define SCROLL_LINES  5

static int l_sid = ERROR_SID;

static int x_ch = 0;
static int y_ch = 0;
static char line[MAX_LINE_CHAR + 1];
static int i_line = 0;


static void newline(void);
static void put_char(char ch);
static void putf(const char *fmt, ...);
static void del_char(void);
static void put_prompt(void);


//-----------------------------------------------------------------------------
// コマンド

static void run_cmd(char *cmd_name);

static void cmd_ls(void);
static void cmd_clear(void);
static void cmd_cat(char *fname);
static void cmd_ps(void);
static void cmd_mem(void);
static void cmd_date(void);
static void cmd_kill(int pid);
static void cmd_dbg(char *name);
static int  cmd_app(char *cmd_name, int bgp);


static int child_pid = 0;


//=============================================================================
// 関数

//-----------------------------------------------------------------------------
// メイン

int console_main(void)
{
    task_set_name("console");

    cursor_tid = timer_new();
    timer_start(cursor_tid, CURSOR_INTERVAL_MS);

    int w = WIDTH_CH * HANKAKU_W;
    int h = HEIGHT_CH * HANKAKU_H;
    l_sid = new_window(700, 280, w, h, "console");
    fill_surface(l_sid, bg);
    update_surface(l_sid);

    put_prompt();

    struct MSG msg;

    while (get_message(&msg)) {
        dispatch_message(&msg, console_proc);
    }

    return 0;
}


static void console_proc(unsigned int msg, unsigned long u_param, long l_param)
{
    if (child_pid != 0) {
        if (msg == MSG_INTR) {
            task_free(child_pid, -1);
        }

        if (msg == MSG_INTR || msg == MSG_NOTIFY_CHILD_EXIT) {
            child_pid = 0;
            timer_start(cursor_tid, CURSOR_INTERVAL_MS);
            newline();
            put_prompt();
        }
        return;
    }

    if (msg == MSG_TIMER) {
        int tid = u_param;
        if (tid == cursor_tid) {
            cursor_timer_handler(cursor_tid);
        }
    } else if (msg == MSG_WINDOW_ACTIVE) {
        l_active = true;
    } else if (msg == MSG_WINDOW_DEACTIVE) {
        l_active = false;
        erase_char(l_sid, x_ch * HANKAKU_W, y_ch * HANKAKU_H, bg, true);
    } else if (msg == MSG_KEYDOWN) {
        keydown_handler(u_param);
    } else if (msg == MSG_CHAR) {
        char ch = u_param;

        keychar_handler(ch);

        if (i_line < MAX_LINE_CHAR) {
            line[i_line++] = ch;
        } else {
            newline();
            i_line = 0;
        }
    }
}


//-----------------------------------------------------------------------------
// メッセージ処理ハンドラ

static void cursor_timer_handler(int tid)
{
    if (l_active) {
        COLOR32 color = (cursor_on) ? bg : fg;

        erase_char(l_sid, x_ch * HANKAKU_W, y_ch * HANKAKU_H, color, true);

        cursor_on = ! cursor_on;
    }

    timer_start(tid, CURSOR_INTERVAL_MS);
}


static void keydown_handler(int keycode)
{
    // バックスペース
    if (keycode == KC_BACKSPACE) {
        del_char();
        return;
    }

    // Enter
    if (keycode == KC_ENTER) {
        if (cursor_on) {
            erase_char(l_sid, x_ch * HANKAKU_W, y_ch * HANKAKU_H, bg, true);
        }

        newline();

        line[i_line] = 0;

        if (i_line > 0) {
            run_cmd(line);

            if (child_pid == 0) {
                put_prompt();
            }
        } else {
            put_prompt();
        }

        return;
    }

}


static void keychar_handler(int ch)
{
    // 通常文字
    put_char(ch);
}


//-----------------------------------------------------------------------------
// 画面出力

static void scroll(int n)
{
    int cy = HANKAKU_H * SCROLL_LINES;
    scroll_surface(l_sid, 0, -cy);
    int y = HEIGHT_CH * HANKAKU_H - cy;
    fill_rect(l_sid, 0, y, WIDTH_CH * HANKAKU_W, cy, bg);

    update_surface(l_sid);
}


static void newline(void)
{
    y_ch++;
    x_ch = 0;

    if (y_ch >= HEIGHT_CH) {
        scroll(SCROLL_LINES);

        y_ch -= SCROLL_LINES;
    }
}


static void put_char(char ch)
{
    if (ch == '\n') {
        newline();
        return;
    }

    char s[2];
    s[0] = ch;
    s[1] = 0;

    draw_text_bg(l_sid, x_ch * HANKAKU_W, y_ch * HANKAKU_H, fg, bg, s);
    update_char(l_sid, x_ch * HANKAKU_W, y_ch * HANKAKU_H);

    x_ch++;
    if (x_ch >= WIDTH_CH) {
        newline();
    }
}

#define TMP_SIZE  (4096)
static char l_tmp[TMP_SIZE];

static void putf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(l_tmp, TMP_SIZE, fmt, ap);
    va_end(ap);

    for (char *s = l_tmp; *s; s++) {
        put_char(*s);
    }
}


static void del_char(void)
{
    if (x_ch > 0 && i_line > 0) {
        erase_char(l_sid, x_ch * HANKAKU_W, y_ch * HANKAKU_H, bg, true);
        x_ch--;
        i_line--;
        erase_char(l_sid, x_ch * HANKAKU_W, y_ch * HANKAKU_H, bg, true);
    }
}


static void put_prompt(void)
{
    putf("$ ");
    i_line = 0;
}


//-----------------------------------------------------------------------------
// コマンド

static void run_cmd(char *cmd_name)
{
    int run_status = 0;

    if (STRCMP(cmd_name, ==, "ls")) {
        cmd_ls();
    } else if (STRCMP(cmd_name, ==, "clear")) {
        cmd_clear();
    } else if (STRNCMP(cmd_name, ==, "cat ", 4)) {
        cmd_cat(&cmd_name[4]);
    } else if (STRCMP(cmd_name, ==, "ps")) {
        cmd_ps();
    } else if (STRCMP(cmd_name, ==, "mem")) {
        cmd_mem();
    } else if (STRCMP(cmd_name, ==, "date")) {
        cmd_date();
    } else if (STRNCMP(cmd_name, ==, "kill ", 5)) {
        if (cmd_name[5] < '0' || '9' < cmd_name[5]) {
            putf("Usage: kill pid\n\n");
        } else {
            cmd_kill(atoi(&cmd_name[5]));
        }
    } else if (STRNCMP(cmd_name, ==, "run ", 4)) {
        run_status = cmd_app(&cmd_name[4], 1);
    } else if (STRCMP(cmd_name, ==, "dbg")) {
        cmd_dbg("all");
    } else if (STRNCMP(cmd_name, ==, "dbg ", 4)) {
        cmd_dbg(&cmd_name[4]);
    } else {
        run_status = cmd_app(cmd_name, 0);
    }

    if (run_status == -1) {
        putf("format error.\n\n");
    } else if (run_status == -2) {
        putf("Command not found.\n\n");
    }
}


void cmd_ls(void)
{
    struct DIRECTORY *dir = ext2_open_dir(2);

    ASSERT(dir, "");

    struct DIRECTORY_ENTRY *ent;

    while ((ent = ext2_read_dir(dir)) != 0) {
        putf("%.*s\n", ent->name_len, ent->name);
    }

    ext2_close_dir(dir);

    newline();
}


static void cmd_clear(void)
{
    fill_surface(l_sid, bg);
    update_surface(l_sid);
    x_ch = 0;
    y_ch = 0;
}


static void cmd_cat(char *fname)
{
    /*
    struct FILEINFO *finfo = fat12_get_file_info();
    int i = fat12_search_file(finfo, fname);

    if (i >= 0) {  // ファイルが見つかった
        char *p = (char *) mem_alloc(finfo[i].size + 1);
        fat12_load_file(finfo[i].clustno, finfo[i].size, p);
        p[finfo[i].size] = 0;
        putf("%s", p);
        mem_free(p);
    } else {  // ファイルが見つからなかった
        putf("File not found.\n");
    }

    newline();
    */
}


static void cmd_ps(void)
{
    for (int i = 0; i < TASK_MAX; i++) {
        const char *name = task_get_name(i);

        if (name != 0) {
            putf("%d : %s\n", i, name);
        }
    }

    newline();
}


static void cmd_mem(void)
{
    putf("memory:\n");
    putf("total  : %z\n", mem_total_B());
    putf("mfree  : %z\n", mem_total_mfree_B());
    putf("vfree  : %z\n\n", mem_total_vfree_B());
}


static void cmd_date(void)
{
    char s[32];
    struct DATETIME t;
    now(&t);
    dt_str(s, 32, &t);
    putf("%s\n\n", s);
}


static void cmd_kill(int pid)
{
    int status = task_free(pid, -1);

    if (status == 0) {
        // そういうものだ
        putf("so it goes.\n", pid);
    } else {
        putf("could not kill pid %d\n", pid);
    }

    newline();
}


static void cmd_dbg(char *name)
{
    if (STRCMP(name, ==, "all")) {
        graphic_dbg();
        mem_dbg();
        paging_dbg();
        task_dbg();
        timer_dbg();
    } else if (STRCMP(name, ==, "sysinfo")) {
        dbgf("\nvram = %X\n", g_sys_info->vram);
        dbgf("width = %d\n", g_sys_info->w);
        dbgf("height = %d\n", g_sys_info->h);
        dbgf("color width = %d\n", g_sys_info->color_width);
        dbgf("mmap entries = %d\n", g_sys_info->mmap_entries);
    } else if (STRCMP(name, ==, "task")) {
        task_dbg();
    } else if (STRCMP(name, ==, "timer")) {
        timer_dbg();
    } else if (STRCMP(name, ==, "mem")) {
        mem_dbg();
    } else if (STRCMP(name, ==, "paging")) {
        paging_dbg();
    } else if (STRCMP(name, ==, "graphic")) {
        graphic_dbg();
    } else if (STRCMP(name, ==, "temp")) {
        g_dbg_temp_flg = 1;
        int fd = f_open("/debug/temp", O_RDONLY);
        char buf[4096];
        f_read(fd, buf, 4096);
        dbgf("%.*s\n", 4096, buf);
        g_dbg_temp_flg = 0;
    } else {
        putf("invalid dbg argment\n\n");
    }
}


static int cmd_app(char *cmd_name, int bgp)
{
    /*
    // コマンドラインからファイル名を生成
    char name[32];

    int i_name;
    for (i_name = 0; i_name < 13; i_name++) {
        if (cmd_name[i_name] <= ' ') {
            break;
        }

        name[i_name] = cmd_name[i_name];
    }
    name[i_name] = 0;

    struct FILEINFO *finfo = fat12_get_file_info();
    int i_fi = fat12_search_file(finfo, name);

    if (i_fi < 0) {  // ファイルが見つからなかった
        return -2;
    }

    struct FILEINFO *fi = &finfo[i_fi];

    char *p = (char *) mem_alloc(fi->size);
    fat12_load_file(fi->clustno, fi->size, p);

    if (is_elf((struct Elf_Ehdr *) p)) {
        child_pid = elf_load(p, fi->size, fi->name);
        //mem_free(p);
    } else {
        //mem_free(p);
        return -1;
    }


    if (bgp) {
        child_pid = 0;
    } else {
        timer_stop(cursor_tid);
    }
    */

    return 0;
}


int cons_write(void *self, const void *buf, int cnt)
{
    putf("%.*s", cnt, (char *) buf);
    return 0;
}


