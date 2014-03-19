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

static int l_info_sid;
static int l_update_tid;

static char *l_dt_txt[] = {
"                    ..,,;cdooddkkO000K00KKKKKKKKKKKKKKKXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXKKKKKKKKKKKKKKKKKKK0o.                   ",
"                   ....,;ldlooxOOO000KKKKKKKKKKKKKKKKKKXXKXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXKKKKKKKKKKKKKKKKK0O:                   ",
"                   ....';cllloxkO0000KKKKKKKKKKKKKKKKKKKKKXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXKKKKKKKKKKKKKKKKKKKKO;                  ",
"                   ......:::coxOO0000KKKKKKKKKKKKKKKKKKKKKXXXXXXXXXXXXXXXXXXXXXXKXXXXXXXXXKKKKKKXXXXKKKKKKKKKk'                 ",
"                     ....,:;:odkO00000KKKKKKKKKKKKKKKKKKKKXXXXXXXXXXXXXXXXXXXXXXXXXXXXXKKXKKKKKKXXXXXKKKKKKXKKd.                ",
"                     .'..,:,;coxOO0000KKKKKKKKKKKKKKKKKKKKXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXKKKKKKXXXXXXXXXXXXXXXKO;                ",
"                     ....;;;,,:ldkOOO00KK0KKKKKKKKKKKKKXXXXXXXXXXXXXXXXXXNXXXXXXXXXXXXXKKKKKKKXXXXXXXXXXXXXXXXKo.               ",
"                     ......'..':c:cccccllodkO0KKKKKKKKKXXXXXXXXXXXXXXXXXNNNXXXXXXXXXXXXKKKKKKKKXXXXXXXXXXXXXXXKk,               ",
"                     ..,. .   ..'...      ..;lokO0KKKKKKXXXXXXXXXXXXXXXXXXXXXXXXXXXKKKKKKKKKKKXXXXXXXXXXXXXXXXK0l.              ",
"                    .....                     .'ldO0KKKKXKXXXXXXXXXXXXXXXK00OkxxxxdxkOOOOO000KKXXXXXXXXXXXXXXXXKx.              ",
"                     .          ....,;;;'..    .'cxO00KKKKKKKXXXXXXK0O0KK0OxdollodxdxOOOkkkOOOKKXXXXXXXXXXXXXXXKO'              ",
"                            .'':odxxkkOOkxol:'...,coxOOO000KKKXXXXKK000000OOOOOOOOOOkkkkOOOkxxOKKXXXXXXXXXXXXXXKO,              ",
"                       ....;cloxkO00000000000Odc'..':odkOO000KKKKKK00000000000000KKKKKKKK000OxxO0KXXXXXXXXXXXXXKO'              ",
"                       ;cllooodxk00000KKKKXXXXKOxl,'':odxkOO00KKKKKK0000000KKKKKXXXXXXXKKKKKK0OOkO0KXXXXXXXXXXXKO,              ",
"                    ..';cc:;;:ooc;,',,;:cox0KKKK0xc,,:coxkOO00KKKKKK000KKKKKKKKKKKXXXXXXXXXKXXKK00KXXXXXXXXXXXXKO'              ",
"                  .'''cxOko,..           .  .:kKK0kl;;;cx0KKKXXXXKKKK0000000OO00KKXXXXXXXXXXXXXKKKXXXXXXXXXXXXXX0'              ",
"                  .,,....'..      .        ':. 'd00d:::cd0KKKXXXKKK000OOkkkxdloddxkOKXXXXXXXXXXXXXKXXKKKXXXXXXXKO.              ",
"                  'cccooolcc,.;odlc:'...'..x0O:  l0O:;old0KKKXXKKKK000OOkxl,.       ..;lOKXXXXXXXKKKKKXXXXXXXXXKd  .':;'        ",
"                  .;:lddxxxdc'':::clloddkkkOOOOxod00dddox0KKKKXKKKKK000Okx:,:;        ...':xKXXXXXKKKKKXXXXXXXXKkoxO000Od.      ",
"                  .;coxkkkkkkxollll:;::cooxkOOO00K0Odolok00KKKKKKKKKKKK0OOkddo:;;:cxdkOOOOOxkOO0000KK0KKXXXXXXXK0KK000Oxx;      ",
"                  'coxkOOkkOOOkxdxkkkxxxkkkOOO0000OkdooxO00KKKKKKKKKKKKK0OkkxxxkkkOO00000KKKKKKK000KKKKXXXXXXXKK000000Oxkc      ",
"                  ,cdkO0OOOOOOOOOOOOOOOOOOOO0000OOkxdodk000KKKKKKKKKKKKKKK00OkkkkkOOOOOO0KKKXXXKKKK0000KXXXXXXKK00KKK00Okl      ",
"                  ,cdO00OOOOOOOOOOOOOOOOOOOOO0OOOxddoodO000KKKKKKKKKKKKKKKXXKKKKKKXXXXKKKXXXXXXKKKKKK00KKXXXXXKKKKKKK00OOl      ",
"                  .:ok00OOOOOkOOOOOOOOOOOOOOOOOkxooolldkO0000000000KKKKKKKXXXKKKKKKXXKXXXXXXXXXXKKKKKK00KXXXXXKKKKKKKK0OOc      ",
"                  .;lx0K0OOOOOOOOOOOOOOOOOkOOOkkdollloxkOO00000000000000KKKXXKKKKKKKKKKKKKKXXXXXKKKKKKKKKXXXXK00000KKK0Ok,      ",
"                   'cdO00OOOOOOOO0000OOOOOOOOOkdolcldkOO00000K000000000KKKKKKKKKKKKKKKKKKKKKXXXXXKKKKKKKKXXXKK0000000K0Ox.      ",
"                   .:dkOOkkkkkOOOOO00OOOOOOkkxxl:::dkO000000000KKKK0000KKKKKKKKKKK0KKKKKKKKXXXXKKKKKKKKKXXXKKK000000000Oc       ",
"                   .,ldxxkkkkkkOOOOOOOOkkkkxxddo;;:xO000000KKK0KKK00000KKKKXKKKKKKKKKKKKXXXXXXXKKKKKKXXXXKKKKKKKKK00000x.       ",
"                    .;loddxkkxkkkkkkkkkxdlllodxxolodkO0000KKKKKKKK000000000K0000KKKKKKKKXXXXXXXKKKKKXXXXXKKKKKKKK00000O:        ",
"                     ';looddxxxxxxxxdol;. .'coxxddlcloxOO000KKKK0000000000000OkkO000KKKKXXXXXXKKKKKKXXXXKKKKKKKK000000x.        ",
"                      ';clloooddoolc,.      ..,:;,'..,cdkOOO00000000K000000OkxxoldkO000OKKKKKKKKKKKKXXXKK0000KK000000Ol         ",
"                       .';:ccllc:,..                   ':ldxkOOdllokO000OOOkxxdl,.:dkOOOKKKKKKKKKKKKXKKK0000000000000k,         ",
"                         .',,,'....                        ...      .'cx0KK0OOkxc. .:oxk00K0000000KKKKK000000KKK0000Ol          ",
"                        ...''.':c:,. ..,,,;,...         .,:lodxdollcldO0KKKKKK00Od,  .;oxO0000000000K00000O00000000Od.          ",
"                         .'..;lc:,.':odxxxxdoll;,,;;::cokOO00000KKKK0000KKKKKKKK00Ox,  .cdxkOO00O00000000OOO0000000x.           ",
"                         .'.;ool;..,coodxxxkxxxxxxxkOkOOOOOO0000KKKK000000KKK000K0OOkl. 'ldxkOOOO0OO00000OOOO000O0k'            ",
"                        .:;,lxdl:'  .,coodxkkkkkOOOOOOO000000000KKKKK00000K0K00000Okkxo.,dxxxkOOOOOO00000OOO000OOd'             ",
"                        ,:;;ldxl:;     ..';;:::ccloddxkkO00000000000000KKKKKK00000Okkxx''xkOkkkOOOOO0000OOkOOOkd;               ",
"                        .:c:ldxxo:.                ...,,;clooddxdddddxxxkkOO00OOOOOOOOx'.oOOOkkOOOOO0OOOOkxo:,.                 ",
"                        .;clodxxxxdl.          .:c:..''. .,:l:;okxl;:c,.. ...';:loddkkx,,xOOOOOOOOO00OOOkx:                     ",
"                         .:llodxxxxkkd:.    'o;xKKK00KKKOkKKXKKXXXKKKK00xo.    .cxxxkOxoxO0OOOOOOOOOOOOkxd.                     ",
"                          ,clodxxkkkkkkxd:,..ccOKKK0KKKKk0KXXKKXXXXKXKKXKKk'.;oO00OOOkkOOOOOOOOOOOOOOOOkx;                      ",
"                          .;clodxkkkkxdddddoll::ok:,dO0d;OKKK0kKKKKKK0K0OOkk0KKKK00OOOOO0OOOOOOOOOOOOOkko.                      ",
"                           '::codxkkkkxocldxxkxxddl:cod:,okxxoxkkdkOkxkkkO00KKKKKK0OOO000OOOOOOOOOOOOkkx,                       ",
"                            ,c,cdxxkkkxoc::cloddxxxkkkkkkkkxxxkOOOOOOOOO0000KKKKKKK0O00OOOOOOOOOOOOOkkx:                        ",
"                            'cc:ldxxxkkxoc::;;;;:loxxxkkOOOOOO0000000000000KKKKKKKK0000OOOOOOOOOOOOkkxl                         ",
"                            .:ccclodxxkxddoollcc::::;clooooddxxkkOOO00000KKKKKKKKKK00000OOOOOOOOOOkkxc                          ",
"                            .;:c:codddxkxxdoooodddoollollcllodxxkOO0000KKKKKKKKKKK000000OOOOOOOOOkkkd.                          ",
};

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
    init_func_names();

    init_gui();

    l_update_tid = timer_new();
    timer_handler();

    // OSタスクを起動
    kernel_thread(debug_main, 0);
    kernel_thread(console_main, 0);
}


static void init_gui(void)
{
    // ---- desk top

    for (int y = 0; y < 48; y++) {
        draw_text_bg(g_dt_sid, 0, y * HANKAKU_H, COL_WHITE, COL_BLACK, l_dt_txt[y]);
    }

    update_surface(g_dt_sid);

    // ---- info

    l_info_sid = new_surface(NO_PARENT_SID, INFO_W, INFO_H);
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
    fill_surface(l_info_sid, COL_BLACK);

    // used memory
    int used_mem_percent = (mem_total_mfree_B() * 100) /  mem_total_B();
    snprintf(l_timer_tmp, L_TIMER_TMP_SIZE, "mem: %d%%", used_mem_percent);
    draw_text(l_info_sid, 5, 5, COL_WHITE, l_timer_tmp);

    fill_rect(g_dt_sid, INFO_X, INFO_Y, INFO_W, INFO_H, COL_BLACK);
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

    if (mdec->change_pos) {
        set_mouse_pos(mdec->x, mdec->y);
    }

    struct MSG msg;
    msg.l_param = mdec->y << 16 | mdec->x;

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

