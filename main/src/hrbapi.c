/**
 * OS 側 はりぼてAPI 処理
 */


//=============================================================================
// 非公開ヘッダ

// API 番号と機能の対応などが書かれたヘッダ。/api/haribote/include
#include "haribote/apino.h"

#include <stdbool.h>
#include "debug.h"
#include "graphic.h"
#include "msg.h"
#include "str.h"
#include "task.h"


void api_exit_app(int exit_status);

typedef struct _PUSHAL_REGISTERS {
    int edi, esi, ebp, esp, ebx, edx, ecx, eax;
} PUSHAL_REGISTERS;



//=============================================================================
// 公開関数

int hrb_api(PUSHAL_REGISTERS reg)
{
    int ret = 0;

    if (reg.edx == API_PUTCHAR) {
        s_printf("%c", reg.eax);
    } else if (reg.edx == API_PUTSTR0) {
        s_printf("%s", reg.ebx);
    } else if (reg.edx == API_PUTSTR1) {
        s_printf("%.*s", reg.ecx, reg.ebx);
    } else if (reg.edx == API_END) {
        api_exit_app(0);
    } else if (reg.edx == API_OPENWIN) {
        int sid = new_window(10, 20, reg.esi, reg.edi, (char *) reg.ecx);
        ret = hrb_sid2addr(sid);
        set_colorkey(ret, reg.eax);
    } else if (reg.edx == API_PUTSTRWIN) {
        bool update = reg.ebx & 1;
        reg.ebx &= ~1;
        int sid = hrb_addr2sid(reg.ebx);

        char str[4096];
        s_snprintf(str, reg.ecx + 1, "%s", reg.ebp);

        draw_text(sid, reg.esi, reg.edi, reg.eax, str);

        if (update) {
            int len = s_len(str);
            update_text(sid, reg.esi, reg.edi, len);
        }
    } else if (reg.edx == API_BOXFILWIN) {
        bool update = reg.ebx & 1;
        reg.ebx &= ~1;
        int sid = hrb_addr2sid(reg.ebx);

        int w = reg.esi - reg.eax;
        int h = reg.edi - reg.ecx;

        fill_rect(sid, reg.eax, reg.ecx, w, h, reg.ebp);

        if (update)
            update_rect(sid, reg.eax, reg.ecx, w, h);
    } else if (reg.edx == API_POINT) {
        bool update = reg.ebx & 1;
        reg.ebx &= ~1;
        int sid = hrb_addr2sid(reg.ebx);

        draw_pixel(sid, reg.esi, reg.edi, reg.eax);

        if (update)
            update_rect(sid, reg.esi, reg.edi, 1, 1);
    } else if (reg.edx == API_REFRESHWIN) {
        int sid = hrb_addr2sid(reg.ebx);

        int w = reg.esi - reg.eax;
        int h = reg.edi - reg.ecx;
        update_rect(sid, reg.eax, reg.ecx, w, h);
    } else if (reg.edx == API_LINEWIN) {
        bool update = reg.ebx & 1;
        reg.ebx &= ~1;
        int sid = hrb_addr2sid(reg.ebx);

        draw_line(sid, reg.eax, reg.ecx, reg.esi, reg.edi, reg.ebp);

        if (update)
            update_surface(sid);
    } else if (reg.edx == API_CLOSEWIN) {
        int sid = hrb_addr2sid(reg.ebx);

        free_surface(sid);
    } else if (reg.edx == API_GETKEY) {
        MSG msg;

        if (reg.eax == 0) {
            while (peek_message(&msg)) {
                if (msg.message == MSG_KEYDOWN) {
                    ret = msg.u_param;
                    break;
                }
            }
        } else {
            while (get_message(&msg)) {
                if (msg.message == MSG_KEYDOWN) {
                    ret = msg.u_param;
                    break;
                }
            }
        }
    } else if (reg.edx == API_PUTINTX) {
        s_printf("%X", reg.eax);
    }

    return ret;
}

