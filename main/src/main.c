/**
 * メイン
 *
 * ブートしたらまず OnSenMain が呼ばれる
 *
 * @file _main.c
 * @author Ivan Ivanovich Ivanov
 */

#include <stdbool.h>

#include "asmfunc.h"
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

static int screen_pid;

static bool is_shift_on = false;
static bool is_ctrl_on  = false;


static void onsen_init(void);
static void main_proc(unsigned int message, unsigned long u_param,
        long l_param);

static void mouse_handler(unsigned long data);
static void keydown_handler(unsigned long keycode);


void OnSenMain(void)
{
    onsen_init();

    MSG msg;

    while (get_message(&msg)) {
        dispatch_message(&msg, main_proc);
    }
}


static void onsen_init(void)
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

    screen_pid = get_screen_pid();
}


static void main_proc(unsigned int message, unsigned long u_param, long l_param)
{
    switch (message) {
    case MSG_REQUEST_EXIT:
        task_free(/* exit app pid =  */ u_param, /* exit status =  */ l_param);
        break;

    case MSG_SCREEN_SWITCHED:
        screen_pid = u_param;
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

    MSG msg;
    msg.l_param = mdec->y << 16 | mdec->x;

    if (mdec->btn_left) {
        msg.message = MSG_LEFT_CLICK;

        msg_q_put(screen_pid, &msg);
    }

    if (mdec->btn_right) {
        msg.message = MSG_RIGHT_CLICK;

        msg_q_put(screen_pid, &msg);
    }

    if (mdec->btn_center) {
        msg.message = MSG_CENTER_CLICK;

        msg_q_put(screen_pid, &msg);
    }
}


static void keydown_handler(unsigned long keycode)
{
    if (keycode == KC_TAB) {
        switch_screen();
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

    MSG msg;
    msg.message = MSG_KEYDOWN;
    msg.u_param = keycode;

    msg_q_put(screen_pid, &msg);

    char ch = keycode2char(keycode, is_shift_on);

    if (is_ctrl_on) {
        if (ch == 'c' || ch == 'C') {
            msg.message = MSG_INTR;

            msg_q_put(screen_pid, &msg);
        }
    } else {
        if (ch != 0) {
            msg.message = MSG_CHAR;
            msg.u_param = ch;

            msg_q_put(screen_pid, &msg);
        }
    }
}

