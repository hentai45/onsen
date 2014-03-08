/**
 * メイン
 *
 * ブートしたらまず OnSenMain が呼ばれる
 */

#include <stdbool.h>

#include "asmfunc.h"
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
#include "paging.h"
#include "stacktrace.h"
#include "sysinfo.h"
#include "task.h"
#include "timer.h"


static int active_win_pid = ERROR_PID;

static bool is_shift_on = false;
static bool is_ctrl_on  = false;


static void init_onsen(void);
static void init_gui(void);
static void main_proc(unsigned int message, unsigned long u_param,
        long l_param);

static void mouse_handler(unsigned long data);
static void keydown_handler(unsigned long keycode);

void (*g_func)(void);

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
    init_func_names();
    gdt_init();
    idt_init();
    intr_init();   // 割り込み初期化
    sti();         // 割り込み許可
    timer_init();
    set_pic0_mask(0xF8);  // PITとPIC1とキーボードの割り込みを許可
    set_pic1_mask(0xEF);  // マウスの割り込みを許可
    task_init();
    // タスク初期化のあとでする必要がある
    void *vram = (void *) VADDR_VRAM;
    graphic_init(vram);  // 画面初期化
    mouse_init();
    set_mouse_pos(get_screen_w() / 2, get_screen_h() / 2);

    init_gui();

    task_run_os("debug", debug_main);
    task_run_os("console", console_main);
}


static void init_gui(void)
{
    fill_surface(g_dt_sid, 0x008484);

    update_surface(g_dt_sid);
}


static void main_proc(unsigned int message, unsigned long u_param, long l_param)
{
    switch (message) {
    case MSG_REQUEST_EXIT:
        //dbgf("request exit: pid=%d %s\n", u_param, task_get_name(u_param));
        task_free(/* exit app pid =  */ u_param, /* exit status =  */ l_param);
        break;

    case MSG_WINDOW_ACTIVE:
        //dbgf("window active: %s\n", task_get_name(u_param));

        if (u_param != g_root_pid) {
            active_win_pid = u_param;
            send_window_active_msg(u_param, u_param);
            update_window(u_param);
        }
        break;

    case MSG_WINDOW_DEACTIVE:
        //dbgf("window deactive: %s\n", task_get_name(u_param));

        if (u_param != g_root_pid) {
            send_window_deactive_msg(u_param, u_param);
            update_window(u_param);
        }
        break;

    case MSG_RAW_MOUSE:
        mouse_handler(/* data */ u_param);
        break;

    case MSG_KEYDOWN:
        keydown_handler(/* keycode = */ u_param);
        break;
    }
}


static bool l_left_down   = false;
static bool l_right_down  = false;
static bool l_center_down = false;

static void mouse_handler(unsigned long data)
{
    if (active_win_pid <= 0)
        return;

    MOUSE_DECODE *mdec = mouse_decode(data);

    if (mdec == 0) {
        return;
    }

    MSG msg;
    msg.l_param = mdec->y << 16 | mdec->x;

    if (mdec->change_pos) {
        set_mouse_pos(mdec->x, mdec->y);
    }

    if (mdec->btn_left && ! l_left_down) {
        // left button down
        l_left_down = true;
        graphic_left_down(mdec->x, mdec->y);

        msg.message = MSG_LEFT_DOWN;
        msg_q_put(active_win_pid, &msg);
    } else if ( ! mdec->btn_left && l_left_down) {
        // left button up
        l_left_down = false;
        graphic_left_up(mdec->x, mdec->y);

        msg.message = MSG_LEFT_UP;
        msg_q_put(active_win_pid, &msg);
    } else {
        // left button drag
        graphic_left_drag(mdec->x, mdec->y);
    }

    if (mdec->btn_right && ! l_right_down) {
        // right button down
        l_right_down = true;

        msg.message = MSG_RIGHT_DOWN;
        msg_q_put(active_win_pid, &msg);
    } else if ( ! mdec->btn_right && l_right_down) {
        // right button up
        l_right_down = false;

        msg.message = MSG_RIGHT_UP;
        msg_q_put(active_win_pid, &msg);
    } else {
        // right button drag
    }

    if (mdec->btn_center && ! l_center_down) {
        // middle button down
        l_center_down = true;

        msg.message = MSG_CENTER_DOWN;
        msg_q_put(active_win_pid, &msg);
    } else if ( ! mdec->btn_center && l_center_down) {
        // middle button up
        l_center_down = false;

        msg.message = MSG_CENTER_UP;
        msg_q_put(active_win_pid, &msg);
    } else {
        // middle button drag
    }
}


static void keydown_handler(unsigned long keycode)
{
    if (active_win_pid <= 0)
        return;

    if (keycode == KC_TAB) {
        switch_window();
        return;
    }

    switch (keycode) {
    case KC_F5:
        update_from_buf();
        break;

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

    if (keycode == KC_F1 && is_shift_on) {
        if ( ! is_os_task(active_win_pid)) {
            task_free(active_win_pid, -1);
        }

        return;
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

