/**
 * メイン
 *
 * ブートしたらまず OnSenMain が呼ばれる
 */

#include <stdbool.h>

#include "asmfunc.h"
#include "bitmap.h"
#include "console.h"
#include "debug.h"
#include "fat12.h"
#include "gdt.h"
#include "graphic.h"
#include "idt.h"
#include "intr.h"
#include "keyboard.h"
#include "keycode.h"
#include "mouse.h"
#include "memory.h"
#include "msg_q.h"
#include "multiboot.h"
#include "paging.h"
#include "sysinfo.h"
#include "task.h"
#include "timer.h"


SYSTEM_INFO *g_sys_info = (SYSTEM_INFO *) VADDR_SYS_INFO;

static int active_win_pid = ERROR_PID;

static bool is_shift_on = false;
static bool is_ctrl_on  = false;


static void init_onsen(void);
static void init_gui(void);
static void run_console(void);
static void run_debug(void);
static void main_proc(unsigned int message, unsigned long u_param,
        long l_param);

static void mouse_handler(unsigned long data);
static void keydown_handler(unsigned long keycode);


void OnSenMain(void)
{
    init_onsen();

    MSG msg;

    while (get_message(&msg)) {
        dispatch_message(&msg, main_proc);
    }
}


static void init_onsen(void)
{
    mem_init();     // メモリ初期化
    paging_init();
    fat12_init();
    gdt_init();
    idt_init();
    intr_init();   // 割り込み初期化
    sti();         // 割り込み許可
    timer_init();
    set_pic0_mask(0xF8);  // PITとPIC1とキーボードの割り込みを許可
    set_pic1_mask(0xEF);  // マウスの割り込みを許可
    task_init();
    // タスク初期化のあとでする必要がある
    COLOR *vram = (COLOR *) VADDR_VRAM;
    graphic_init(vram);  // 画面初期化
    mouse_init();
    set_mouse_pos(g_w / 2, g_h / 2);

    init_gui();

    run_debug();
    run_console();

    active_win_pid = get_active_win_pid();
}


static void test_draw_rainbow(void);
static void test_draw_bitmap(void);
static void test_draw_textbox(void);
static void test_draw_window(void);

static void init_gui(void)
{
    fill_surface(g_dt_sid, RGB2(0x008484));

    test_draw_rainbow();
    test_draw_bitmap();
    test_draw_textbox();

    update_surface(g_dt_sid);
}

static void test_draw_rainbow(void)
{
    for (int y = 0; y < g_h; y++) {
        for (int x = 0; x < g_w; x++) {
            draw_pixel(g_dt_sid, x, y, RGB(x % 256, y % 256, 255 - (x % 256)));
        }
    }
}

static void test_draw_bitmap(void)
{
    FILEINFO *finfo = fat12_get_file_info();
    int i_fi = fat12_search_file(finfo, "test.bmp");

    if (i_fi >= 0) {
        FILEINFO *fi = &finfo[i_fi];

        char *p = (char *) mem_alloc(fi->size);
        fat12_load_file(fi->clustno, fi->size, p);

        int bmp_sid = load_bmp(p, fi->size);
        set_sprite_pos(bmp_sid, 250, 250);
        draw_sprite(bmp_sid, g_dt_sid, OP_SRC_COPY);

        mem_free(p);
    }
}

static void test_draw_textbox(void)
{
    int sid = new_surface(NO_PARENT_SID, g_w / 4, g_h / 4);
    fill_surface(sid, COL_BLACK);
    set_alpha(sid, 50);
    set_sprite_pos(sid, 150, 30);
    draw_sprite(sid, g_dt_sid, OP_SRC_COPY);

    draw_text(g_dt_sid, 155, 35, COL_WHITE, "HELLO");
}


static void run_console(void)
{
    run_os_task("console", console_main, 20, true);
}


static void run_debug(void)
{
    run_os_task("debug", debug_main, 20, true);
}


static void main_proc(unsigned int message, unsigned long u_param, long l_param)
{
    switch (message) {
    case MSG_REQUEST_EXIT:
        dbgf("request exit: %s\n", task_get_name(u_param));
        task_free(/* exit app pid =  */ u_param, /* exit status =  */ l_param);
        break;

    case MSG_WINDOW_SWITCHED:
        dbgf("change active window: %s\n", task_get_name(u_param));
        active_win_pid = u_param;
        break;

    case MSG_RAW_MOUSE:
        mouse_handler(/* data */ u_param);
        break;

    case MSG_KEYDOWN:
        keydown_handler(/* keycode = */ u_param);
        break;
    }
}


static void mouse_handler(unsigned long data)
{
    MOUSE_DECODE *mdec = mouse_decode(data);

    if (mdec == 0) {
        return;
    }

    if (mdec->change_pos) {
        set_mouse_pos(mdec->x, mdec->y);
    }

    if (active_win_pid == ERROR_PID)
        return;

    MSG msg;
    msg.l_param = mdec->y << 16 | mdec->x;

    if (mdec->btn_left) {
        msg.message = MSG_LEFT_CLICK;

        msg_q_put(active_win_pid, &msg);
    }

    if (mdec->btn_right) {
        msg.message = MSG_RIGHT_CLICK;

        msg_q_put(active_win_pid, &msg);
    }

    if (mdec->btn_center) {
        msg.message = MSG_CENTER_CLICK;

        msg_q_put(active_win_pid, &msg);
    }
}


static void keydown_handler(unsigned long keycode)
{
    if (active_win_pid == ERROR_PID)
        return;

    if (keycode == KC_TAB) {
        switch_window();
        return;
    }

    switch (keycode) {
    case KC_CTRL_ON:
        is_ctrl_on = true;
        break;

    case KC_CTRL_OFF:
        is_ctrl_on = false;
        break;

    case KC_LEFT_SHIFT_ON:
    case KC_RIGHT_SHIFT_ON:
        is_shift_on = true;
        break;

    case KC_LEFT_SHIFT_OFF:
    case KC_RIGHT_SHIFT_OFF:
        is_shift_on = false;
        break;
    }

    // 押下時だけメッセージを送る（離したときは0x80がプラスされた値になる）
    MSG msg;
    if (keycode < 0x80) {
        msg.message = MSG_KEYDOWN;
        msg.u_param = keycode;
    }

    msg_q_put(active_win_pid, &msg);

    char ch = keycode2char(keycode, is_shift_on);

    if (is_ctrl_on) {
        if (ch == 'c' || ch == 'C') {
            msg.message = MSG_INTR;

            msg_q_put(active_win_pid, &msg);
        }
    } else {
        if (ch != 0) {
            msg.message = MSG_CHAR;
            msg.u_param = ch;

            msg_q_put(active_win_pid, &msg);
        }
    }
}

