/**
 * コンソール
 */

/*
 * ＜目次＞
 * ・メイン
 * ・メッセージ処理ハンドラ
 * ・画面出力
 * ・コマンド
 */


//=============================================================================
// ヘッダ

#ifndef HEADER_CONSOLE
#define HEADER_CONSOLE

#include "file.h"

void console_main(void);

extern FILE_T *f_console;

#endif


#include <stdbool.h>

#include "asmfunc.h"
#include "bitmap.h"
#include "debug.h"
#include "elf.h"
#include "fat12.h"
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
#include "timer.h"


#define CURSOR_INTERVAL_MS 500


static COLOR fg = COL_WHITE;
static COLOR bg = COL_BLACK;

static int cursor_tid;
static int cursor_on = 0;
static bool l_active;

int cons_write(void *self, const void *buf, int cnt);

FILE_T l_f_console = { .write = cons_write };
FILE_T *f_console = &l_f_console;


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

#define WIDTH_CH   (39)
#define HEIGHT_CH  (28)

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

static int child_pid = 0;


static void run_cmd(char *cmd_name);

static void cmd_ls(void);
static void cmd_clear(void);
static void cmd_cat(char *fname);
static void cmd_ps(void);
static void cmd_mem(void);
static void cmd_kill(int pid);
static void cmd_dbg(char *name);
static int  cmd_app(char *cmd_name, int bgp);


//=============================================================================
// 関数

//-----------------------------------------------------------------------------
// メイン

void console_main(void)
{
    cursor_tid = timer_new();
    timer_start(cursor_tid, CURSOR_INTERVAL_MS);

    int w = WIDTH_CH * HANKAKU_W;
    int h = HEIGHT_CH * HANKAKU_H;
    l_sid = new_window(320, 0, w, h, "console");
    fill_surface(l_sid, bg);
    update_surface(l_sid);

    put_prompt();

    MSG msg;

    while (get_message(&msg)) {
        dispatch_message(&msg, console_proc);
    }
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
        COLOR color = (cursor_on) ? bg : fg;

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
    s_vsnprintf(l_tmp, TMP_SIZE, fmt, ap);
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

    if (s_cmp(cmd_name, "ls") == 0) {
        cmd_ls();
    } else if (s_cmp(cmd_name, "clear") == 0) {
        cmd_clear();
    } else if (s_ncmp(cmd_name, "cat ", 4) == 0) {
        cmd_cat(&cmd_name[4]);
    } else if (s_cmp(cmd_name, "ps") == 0) {
        cmd_ps();
    } else if (s_cmp(cmd_name, "mem") == 0) {
        cmd_mem();
    } else if (s_ncmp(cmd_name, "kill ", 5) == 0) {
        cmd_kill(s_atoi(&cmd_name[5]));
    } else if (s_ncmp(cmd_name, "run ", 4) == 0) {
        run_status = cmd_app(&cmd_name[4], 1);
    } else if (s_cmp(cmd_name, "dbg") == 0) {
        cmd_dbg("all");
    } else if (s_ncmp(cmd_name, "dbg ", 4) == 0) {
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
    FILEINFO *finfo = fat12_get_file_info();
    char s[30];

    for (int i = 0; i < 224; i++) {
        // ここで終了か？
        if (finfo[i].name[0] == 0x00)
            break;

        // 削除されたファイルは飛ばす
        if (finfo[i].name[0] == 0xE5)
            continue;

        // 隠しファイルかディレクトリなら次へ
        if ((finfo[i].type & 0x18) != 0)
            continue;

        for (int y = 0; y < 8; y++) {
            s[y] = finfo[i].name[y];
        }
        s[8] = '.';
        s[9] = finfo[i].ext[0];
        s[10] = finfo[i].ext[1];
        s[11] = finfo[i].ext[2];
        s[12] = 0;

        putf("%s   %d\n", s, finfo[i].size);
    }

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
    FILEINFO *finfo = fat12_get_file_info();
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


static void cmd_kill(int pid)
{
    int status = task_free(pid, -1);

    if (status == 0) {
        // そういうものだ
    } else {
        putf("could not kill pid %d\n", pid);
    }

    newline();
}


static void cmd_dbg(char *name)
{
    if (s_cmp(name, "all") == 0) {
        graphic_dbg();
        mem_dbg();
        paging_dbg();
        task_dbg();
        timer_dbg();
    } else if (s_cmp(name, "sysinfo") == 0) {
        dbgf("\nvram = %X\n", g_sys_info->vram);
        dbgf("width = %d\n", g_sys_info->w);
        dbgf("height = %d\n", g_sys_info->h);
        dbgf("color width = %d\n", g_sys_info->color_width);
        dbgf("end_free_maddr = %#X\n\n", g_sys_info->end_free_maddr);
    } else if (s_cmp(name, "task") == 0) {
        task_dbg();
    } else if (s_cmp(name, "timer") == 0) {
        timer_dbg();
    } else if (s_cmp(name, "mem") == 0) {
        mem_dbg();
    } else if (s_cmp(name, "paging") == 0) {
        paging_dbg();
    } else if (s_cmp(name, "graphic") == 0) {
        graphic_dbg();
    } else if (s_cmp(name, "temp") == 0) {
        g_dbg_temp_flg = 1;
        int fd = f_open("/debug/temp", O_RDONLY);
        char buf[4096];
        int ret = f_read(fd, buf, 4096);
        dbgf("%.*s\n", 4096, buf);
        g_dbg_temp_flg = 0;
    } else {
        putf("invalid dbg argment\n\n");
    }
}


static int cmd_app(char *cmd_name, int bgp)
{
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

    FILEINFO *finfo = fat12_get_file_info();
    int i_fi = fat12_search_file(finfo, name);

    // 見つからなかったら、後ろに.HRBをつけてもう１度検索する
    if (i_fi < 0) {
        name[i_name    ] = '.';
        name[i_name + 1] = 'H';
        name[i_name + 2] = 'R';
        name[i_name + 3] = 'B';
        name[i_name + 4] = 0;

        i_fi = fat12_search_file(finfo, name);
    }

    if (i_fi < 0) {  // ファイルが見つからなかった
        return -2;
    }

    FILEINFO *fi = &finfo[i_fi];

    char *p_code = (char *) mem_alloc(fi->size);
    fat12_load_file(fi->clustno, fi->size, p_code);

    if (fi->size < 36 || s_ncmp(p_code + 4, "Hari", 4) != 0 ||
            *p_code != 0) {
        return -1;
    }

    child_pid = task_run_app(p_code, fi->size, fi->name);

    if (bgp) {
        child_pid = 0;
    } else {
        timer_stop(cursor_tid);
    }

    return 0;
}


int cons_write(void *self, const void *buf, int cnt)
{
    putf("%.*s", cnt, (char *) buf);
    return 0;
}


