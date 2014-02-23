/**
 * コンソール
 *
 * @file console.c
 * @author Ivan Ivanovich Ivanov
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


#define fg COL_WHITE
#define bg COL_BLACK

#define CURSOR_INTERVAL_MS 500


static int cursor_tid;
static int cursor_on = 0;

int cons_write(void *self, const void *buf, int cnt);

FILE_T l_f_console = { .write = cons_write };
FILE_T *f_console = &l_f_console;


//-----------------------------------------------------------------------------
// メイン

static void console_proc(unsigned int msg, unsigned long u_param, long l_param);


//-----------------------------------------------------------------------------
// メッセージ処理ハンドラ

static void cursor_timer_handler(int id);
static void keydown_handler(int keycode);
static void keychar_handler(int ch);


//-----------------------------------------------------------------------------
// 画面出力

#define MAX_LINE_CHAR 1024

#define SCROLL_LINES  15


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

void test_draw_rainbow(void);
void test_draw_bitmap(void);
void test_draw_textbox(void);

void console_main(void)
{
    test_draw_rainbow();
    test_draw_bitmap();
    test_draw_textbox();

    update_screen(g_con_sid);

    cursor_tid = timer_new();
    timer_start(cursor_tid, CURSOR_INTERVAL_MS);

    put_prompt();

    MSG msg;

    while (get_message(&msg)) {
        dispatch_message(&msg, console_proc);
    }
}

void test_draw_rainbow(void)
{
    for (int y = 0; y < g_h; y++) {
        for (int x = 0; x < g_w; x++) {
            draw_pixel(g_con_sid, x, y, RGB(x % 256, y % 256, 255 - (x % 256)));
        }
    }
}

void test_draw_bitmap(void)
{
    FILEINFO *finfo = fat12_get_file_info();
    int i_fi = fat12_search_file(finfo, "test.bmp");

    if (i_fi >= 0) {
        FILEINFO *fi = &finfo[i_fi];

        char *p = (char *) mem_alloc(fi->size);
        fat12_load_file(fi->clustno, fi->size, p);

        int bmp_sid = load_bmp(p, fi->size);
        set_sprite_pos(bmp_sid, 250, 250);
        draw_sprite(bmp_sid, g_con_sid, OP_SRC_COPY);

        mem_free(p);
    }
}

void test_draw_textbox(void)
{
    int sid = surface_new(g_w / 4, g_h / 4);
    fill_surface(sid, COL_BLACK);
    set_alpha(sid, 50);
    set_sprite_pos(sid, 150, 30);
    draw_sprite(sid, g_con_sid, OP_SRC_COPY);

    draw_text(g_con_sid, 155, 35, COL_WHITE, "HELLO");
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

static void cursor_timer_handler(int id)
{
    int x = x_ch * 8;
    int y = y_ch * 16;

    if (cursor_on == 0) {
        cursor_on = 1;

        fill_rect(g_con_sid, x, y, 8, 16, fg);
        update_rect(g_con_sid, x, y, 8, 16);

        timer_start(id, CURSOR_INTERVAL_MS);
    } else {
        cursor_on = 0;
        fill_rect(g_con_sid, x, y, 8, 16, bg);
        update_rect(g_con_sid, x, y, 8, 16);
        timer_start(id, CURSOR_INTERVAL_MS);
    }
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
            fill_rect(g_con_sid, x_ch * 8, y_ch * 16, 8, 16, bg);
            update_rect(g_con_sid, x_ch * 8, y_ch * 16, 8, 16);
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

static void newline(void)
{
    y_ch++;
    x_ch = 0;

    if (y_ch >= 30) {
        int cy = 16 * SCROLL_LINES;
        scroll_surface(g_con_sid, 0, -cy);
        fill_rect(g_con_sid, 0, g_h - cy, g_w, cy, COL_BLACK);
        update_screen(g_con_sid);

        y_ch = 30 - SCROLL_LINES;
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

    draw_text_bg(g_con_sid, x_ch * 8, y_ch * 16, fg, bg, s);
    update_rect(g_con_sid, x_ch * 8, y_ch * 16, 8, 16);

    x_ch++;
    if (x_ch >= 80) {
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
        fill_rect(g_con_sid, x_ch * 8, y_ch * 16, 8, 16, bg);
        update_rect(g_con_sid, x_ch * 8, y_ch * 16, 8, 16);
        x_ch--;
        i_line--;
        fill_rect(g_con_sid, x_ch * 8, y_ch * 16, 8, 16, bg);
        update_rect(g_con_sid, x_ch * 8, y_ch * 16, 8, 16);
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
    fill_surface(g_con_sid, bg);
    update_screen(g_con_sid);
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
        dbg_clear();
        graphic_dbg();
        mem_dbg();
        paging_dbg();
        task_dbg();
        timer_dbg();
        switch_debug_screen();
    } else if (s_cmp(name, "sysinfo") == 0) {
        dbg_clear();
        dbgf("vram = %X\n", g_sys_info->vram);
        dbgf("width = %d\n", g_sys_info->w);
        dbgf("height = %d\n", g_sys_info->h);
        dbgf("end_free_maddr = %X\n", g_sys_info->end_free_maddr);

        switch_debug_screen();
    } else if (s_cmp(name, "task") == 0) {
        dbg_clear();
        task_dbg();
        switch_debug_screen();
    } else if (s_cmp(name, "timer") == 0) {
        dbg_clear();
        timer_dbg();
        switch_debug_screen();
    } else if (s_cmp(name, "mem") == 0) {
        dbg_clear();
        mem_dbg();
        switch_debug_screen();
    } else if (s_cmp(name, "paging") == 0) {
        dbg_clear();
        paging_dbg();
        switch_debug_screen();
    } else if (s_cmp(name, "graphic") == 0) {
        dbg_clear();
        graphic_dbg();
        switch_debug_screen();
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


