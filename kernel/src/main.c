/**
 * メイン
 *
 * ブートしたらまず OnSenMain が呼ばれる
 */

#include <stdbool.h>

#include "asmfunc.h"
#include "ata/common.h"
#include "console.h"
#include "debug.h"
#include "ext2fs.h"
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
#include "str.h"
#include "sysinfo.h"
#include "task.h"
#include "timer.h"


#define UPDATE_INTERVAL_MS  1000

#define INFO_X  930
#define INFO_Y  10
#define INFO_W  (10 * HANKAKU_W)
#define INFO_H  (5 * HANKAKU_H)
#define INFO_BG COL_BLACK

static void init_onsen(void);
static void init_gui(void);
static void main_proc(unsigned int message, unsigned long u_param,
        long l_param);

static void timer_handler(void);
static void mouse_handler(unsigned long data);
static void keydown_handler(unsigned long keycode);


static int active_win_pid = ERROR_PID;

static bool is_shift_on = false;
static bool is_ctrl_on  = false;

static COLOR32 bg = 0x008484;
static int l_info_bg_sid;
static int l_info_sid;
static int l_update_tid;


void OnSenMain(void)
{
    init_onsen();

    struct MSG msg;

    while (get_message(&msg)) {
        dispatch_message(&msg, main_proc);
    }
}


static void init_onsen(void)
{
    void *vram = (void *) VADDR_VRAM;
    minimal_graphic_init(vram, g_sys_info->w, g_sys_info->h, g_sys_info->color_width);

    mem_init();     // メモリ初期化
    paging_init();
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
    graphic_init();  // 画面初期化
    mouse_init();
    set_mouse_pos(get_screen_w() / 2, get_screen_h() / 2);
    ata_init();
    ext2_init();

    init_gui();

    l_update_tid = timer_new();
    timer_handler();

    // OSタスクを起動
    kernel_thread(debug_main, 0);
    kernel_thread(console_main, 0);
}


static void init_gui(void)
{
    // desk top
    fill_surface(g_dt_sid, bg);
    update_surface(g_dt_sid);

    l_info_bg_sid = new_surface(NO_PARENT_SID, INFO_W, INFO_H);
    set_alpha(l_info_bg_sid, 60);
    fill_surface(l_info_bg_sid, INFO_BG);

    l_info_sid = new_surface(NO_PARENT_SID, INFO_W, INFO_H);
    set_colorkey(l_info_sid, COL_BLUE);
    fill_surface(l_info_sid, COL_BLUE);
}


static void main_proc(unsigned int message, unsigned long u_param, long l_param)
{
    switch (message) {
    case MSG_TIMER:
        timer_handler();
        break;

    case MSG_RAW_MOUSE:
        mouse_handler(/* data */ u_param);
        break;

    case MSG_KEYDOWN:
        keydown_handler(/* keycode = */ u_param);
        break;

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
    }
}


#define L_TIMER_TMP_SIZE  32
static char l_timer_tmp[L_TIMER_TMP_SIZE];

static void timer_handler(void)
{
    fill_surface(l_info_bg_sid, INFO_BG);
    fill_surface(l_info_sid, COL_BLUE);

    // userd memory
    int used_mem_percent = (mem_total_mfree_B() * 100) /  mem_total_B();
    snprintf(l_timer_tmp, L_TIMER_TMP_SIZE, "mem: %d%%", used_mem_percent);
    draw_text(l_info_sid, 5, 5, COL_WHITE, l_timer_tmp);

    fill_rect(g_dt_sid, INFO_X, INFO_Y, INFO_W, INFO_H, bg);
    draw_surface(l_info_bg_sid, g_dt_sid, INFO_X, INFO_Y, OP_SRC_COPY);
    draw_surface(l_info_sid, g_dt_sid, INFO_X, INFO_Y, OP_SRC_COPY);

    update_rect(g_dt_sid, INFO_X, INFO_Y, INFO_W, INFO_H);

    timer_start(l_update_tid, UPDATE_INTERVAL_MS);
}


static bool l_left_down   = false;
static bool l_right_down  = false;
static bool l_center_down = false;

static void mouse_handler(unsigned long data)
{
    if (active_win_pid <= 0)
        return;

    struct MOUSE_DECODE *mdec = mouse_decode(data);

    if (mdec == 0) {
        return;
    }

    struct MSG msg;
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
    struct MSG msg;
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

