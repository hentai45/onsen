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
#include "api.h"
#include "color.h"
#include "debug.h"
#include "graphic.h"
#include "memory.h"
#include "msg.h"
#include "str.h"
#include "task.h"
#include "timer.h"


void api_exit_app(int exit_status);

static int l_timer_map[TIMER_MAX];

static void conv_window_cord(int *x, int *y);

#define HRB_CLIENT_X     (4)
#define HRB_CLIENT_Y     (24)
#define HRB_WINDOW_EXT_WIDTH  (8)
#define HRB_WINDOW_EXT_HEIGHT (28)

//=============================================================================
// 公開関数

int hrb_api(API_REGISTERS regs)
{
    int ret = 0;

    /*
    int cs, ds;
    __asm__ __volatile__ ("mov %%cs, %0" : "=r" (cs));
    __asm__ __volatile__ ("mov %%ds, %0" : "=r" (ds));
    dbgf("hrb_api: cs = %X, ds = %X\n", cs, ds);
    */

    if (regs.edx == API_PUTCHAR) {
        printf("%c", regs.eax);
    } else if (regs.edx == API_PUTSTR0) {
        printf("%s", regs.ebx);
    } else if (regs.edx == API_PUTSTR1) {
        printf("%.*s", regs.ecx, regs.ebx);
    } else if (regs.edx == API_END) {
        api_exit_app(0);
    } else if (regs.edx == API_OPENWIN) {
        int w = regs.esi - HRB_WINDOW_EXT_WIDTH;
        int h = regs.edi - HRB_WINDOW_EXT_HEIGHT;

        /* ***** 注意：バッファはAPI_MALLOCで取得したものでないといけない。
         *             ユーザ領域のバッファを使うとページフォルトになる ***** */
        int sid = new_window_from_buf(10, 20, w, h, (char *) regs.ecx, (void *) regs.ebx, 8);
        ret = hrb_sid2addr(sid);

        set_colorkey(ret, RGB8_TO_32(regs.eax));
    } else if (regs.edx == API_PUTSTRWIN) {
        bool update = regs.ebx & 1;
        regs.ebx &= ~1;
        int sid = hrb_addr2sid(regs.ebx);

        char str[4096];
        snprintf(str, regs.ecx + 1, "%s", regs.ebp);

        int x = regs.esi;
        int y = regs.edi;
        conv_window_cord(&x, &y);

        COLOR32 color = RGB8_TO_32(regs.eax);

        draw_text(sid, x, y, color, str);

        update = 1;
        if (update) {
            int len = strlen(str);
            update_text(sid, x, y, len);
        }
    } else if (regs.edx == API_BOXFILWIN) {
        bool update = regs.ebx & 1;
        regs.ebx &= ~1;
        int sid = hrb_addr2sid(regs.ebx);

        int x = regs.eax;
        int y = regs.ecx;
        conv_window_cord(&x, &y);

        int w = regs.esi - regs.eax;
        int h = regs.edi - regs.ecx;

        fill_rect(sid, x, y, w, h, RGB8_TO_32(regs.ebp));

        update = 1;
        if (update)
            update_rect(sid, x, y, w, h);
    } else if (regs.edx == API_INITMALLOC) {
    } else if (regs.edx == API_MALLOC) {
        ret = (int) mem_alloc(regs.ecx);
    } else if (regs.edx == API_FREE) {
        mem_free((void *) regs.eax);
    } else if (regs.edx == API_POINT) {
        bool update = regs.ebx & 1;
        regs.ebx &= ~1;
        int sid = hrb_addr2sid(regs.ebx);

        int x = regs.esi;
        int y = regs.edi;
        conv_window_cord(&x, &y);

        draw_pixel(sid, x, y, RGB8_TO_32(regs.eax));

        if (update)
            update_rect(sid, x, y, 1, 1);
    } else if (regs.edx == API_REFRESHWIN) {
        int sid = hrb_addr2sid(regs.ebx);

        int x = regs.eax;
        int y = regs.ecx;
        conv_window_cord(&x, &y);

        int w = regs.esi - regs.eax;
        int h = regs.edi - regs.ecx;
        update_rect(sid, x, y, w, h);
    } else if (regs.edx == API_LINEWIN) {
        bool update = regs.ebx & 1;
        regs.ebx &= ~1;
        int sid = hrb_addr2sid(regs.ebx);

        int x0 = regs.eax;
        int y0 = regs.ecx;
        conv_window_cord(&x0, &y0);

        int x1 = regs.esi;
        int y1 = regs.edi;
        conv_window_cord(&x1, &y1);

        draw_line(sid, x0, y0, x1, y1, RGB8_TO_32(regs.ebp));

        update = 1;
        if (update)
            update_surface(sid);
    } else if (regs.edx == API_CLOSEWIN) {
        int sid = hrb_addr2sid(regs.ebx);

        free_surface(sid);
    } else if (regs.edx == API_GETKEY) {
        MSG msg;

        if (regs.eax == 0) {
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
    } else if (regs.edx == API_ALLOCTIMER) {
        ret = timer_new();
    } else if (regs.edx == API_INITTIMER) {
        l_timer_map[regs.ebx] = regs.eax;
    } else if (regs.edx == API_SETTIMER) {
        timer_start(regs.ebx, regs.eax * 10);
    } else if (regs.edx == API_FREETIMER) {
        timer_free(regs.ebx);
    } else if (regs.edx == API_PUTINTX) {
        printf("%X", regs.eax);
    }

    return ret;
}


static void conv_window_cord(int *x, int *y)
{
    *x -= HRB_CLIENT_X;
    *y -= HRB_CLIENT_Y;
}
