/**
 * OS 側 はりぼてAPI 処理
 */

#ifndef HEADER_HRBAPI
#define HEADER_HRBAPI

void haribote_init(void);

#endif

//=============================================================================
// 非公開ヘッダ

// API 番号と機能の対応などが書かれたヘッダ。/api/haribote/include
#include "haribote/apino.h"

#include <stdbool.h>
#include "color.h"
#include "debug.h"
#include "graphic.h"
#include "memory.h"
#include "msg.h"
#include "str.h"
#include "task.h"
#include "timer.h"


void api_exit_app(int exit_status);

typedef struct _PUSHAL_REGISTERS {
    int edi, esi, ebp, esp, ebx, edx, ecx, eax;
} PUSHAL_REGISTERS;

static int l_timer_map[TIMER_MAX];

static void conv_window_cord(int *x, int *y);

#define HRB_CLIENT_X     (4)
#define HRB_CLIENT_Y     (24)
#define HRB_WINDOW_EXT_WIDTH  (8)
#define HRB_WINDOW_EXT_HEIGHT (28)

//=============================================================================
// 公開関数


int hrb_api(PUSHAL_REGISTERS reg)
{
    int ret = 0;

    short cs, ds;
    __asm__ __volatile__ ("mov %%cs, %0" : "=r" (cs));
    __asm__ __volatile__ ("mov %%ds, %0" : "=r" (ds));
    //dbgf("hrb_api: cs = %X, ds = %X\n", cs, ds);

    if (reg.edx == API_PUTCHAR) {
        s_printf("%c", reg.eax);
    } else if (reg.edx == API_PUTSTR0) {
        s_printf("%s", reg.ebx);
    } else if (reg.edx == API_PUTSTR1) {
        s_printf("%.*s", reg.ecx, reg.ebx);
    } else if (reg.edx == API_END) {
        api_exit_app(0);
    } else if (reg.edx == API_OPENWIN) {
        int w = reg.esi - HRB_WINDOW_EXT_WIDTH;
        int h = reg.edi - HRB_WINDOW_EXT_HEIGHT;

        /* ***** 注意：バッファはAPI_MALLOCで取得したものでないといけない。
         *             ユーザ領域のバッファを使うとページフォルトになる ***** */
        int sid = new_window_from_buf(10, 20, w, h, (char *) reg.ecx, (void *) reg.ebx, 8);
        ret = hrb_sid2addr(sid);

        set_colorkey(ret, RGB8_TO_32(reg.eax));
    } else if (reg.edx == API_PUTSTRWIN) {
        bool update = reg.ebx & 1;
        reg.ebx &= ~1;
        int sid = hrb_addr2sid(reg.ebx);

        char str[4096];
        s_snprintf(str, reg.ecx + 1, "%s", reg.ebp);

        int x = reg.esi;
        int y = reg.edi;
        conv_window_cord(&x, &y);

        COLOR32 color = RGB8_TO_32(reg.eax);

        draw_text(sid, x, y, color, str);

        update = 1;
        if (update) {
            int len = s_len(str);
            update_text(sid, x, y, len);
        }
    } else if (reg.edx == API_BOXFILWIN) {
        bool update = reg.ebx & 1;
        reg.ebx &= ~1;
        int sid = hrb_addr2sid(reg.ebx);

        int x = reg.eax;
        int y = reg.ecx;
        conv_window_cord(&x, &y);

        int w = reg.esi - reg.eax;
        int h = reg.edi - reg.ecx;

        fill_rect(sid, x, y, w, h, RGB8_TO_32(reg.ebp));

        update = 1;
        if (update)
            update_rect(sid, x, y, w, h);
    } else if (reg.edx == API_INITMALLOC) {
    } else if (reg.edx == API_MALLOC) {
        ret = (int) mem_alloc(reg.ecx);
    } else if (reg.edx == API_FREE) {
        mem_free((void *) reg.eax);
    } else if (reg.edx == API_POINT) {
        bool update = reg.ebx & 1;
        reg.ebx &= ~1;
        int sid = hrb_addr2sid(reg.ebx);

        int x = reg.esi;
        int y = reg.edi;
        conv_window_cord(&x, &y);

        draw_pixel(sid, x, y, RGB8_TO_32(reg.eax));

        if (update)
            update_rect(sid, x, y, 1, 1);
    } else if (reg.edx == API_REFRESHWIN) {
        int sid = hrb_addr2sid(reg.ebx);

        int x = reg.eax;
        int y = reg.ecx;
        conv_window_cord(&x, &y);

        int w = reg.esi - reg.eax;
        int h = reg.edi - reg.ecx;
        update_rect(sid, x, y, w, h);
    } else if (reg.edx == API_LINEWIN) {
        bool update = reg.ebx & 1;
        reg.ebx &= ~1;
        int sid = hrb_addr2sid(reg.ebx);

        int x0 = reg.eax;
        int y0 = reg.ecx;
        conv_window_cord(&x0, &y0);

        int x1 = reg.esi;
        int y1 = reg.edi;
        conv_window_cord(&x1, &y1);

        draw_line(sid, x0, y0, x1, y1, RGB8_TO_32(reg.ebp));

        update = 1;
        if (update)
            update_surface(sid);
    } else if (reg.edx == API_CLOSEWIN) {
        int sid = hrb_addr2sid(reg.ebx);

        free_surface(sid);
    } else if (reg.edx == API_GETKEY) {
        MSG msg;

        if (reg.eax == 0) {
            while (peek_message(&msg)) {
                if (msg.message == MSG_KEYDOWN && msg.u_param == 0x1C) {
                    ret = '\n';
                    break;
                } else if (msg.message == MSG_CHAR) {
                    ret = msg.u_param;
                    break;
                } else if (msg.message == MSG_TIMER) {
                    ret = l_timer_map[msg.u_param];
                    break;
                }
            }
        } else {
            while (get_message(&msg)) {
                if (msg.message == MSG_KEYDOWN && msg.u_param == 0x1C) {
                    ret = '\n';
                    break;
                } else if (msg.message == MSG_CHAR) {
                    ret = msg.u_param;
                    break;
                } else if (msg.message == MSG_TIMER) {
                    ret = l_timer_map[msg.u_param];
                    break;
                }
            }
        }
    } else if (reg.edx == API_ALLOCTIMER) {
        ret = timer_new();
    } else if (reg.edx == API_INITTIMER) {
        l_timer_map[reg.ebx] = reg.eax;
    } else if (reg.edx == API_SETTIMER) {
        timer_start(reg.ebx, reg.eax * 10);
    } else if (reg.edx == API_FREETIMER) {
        timer_free(reg.ebx);
    } else if (reg.edx == API_PUTINTX) {
        s_printf("%X", reg.eax);
    }

    return ret;
}


static void conv_window_cord(int *x, int *y)
{
    *x -= HRB_CLIENT_X;
    *y -= HRB_CLIENT_Y;
}
