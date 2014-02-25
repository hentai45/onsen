/**
 * OS 側 はりぼてAPI 処理
 */


//=============================================================================
// 非公開ヘッダ

// API 番号と機能の対応などが書かれたヘッダ。/api/haribote/include
#include "haribote/apino.h"

#include "debug.h"
#include "graphic.h"
#include "str.h"


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
        ret = new_window(0, 0, reg.esi, reg.edi, reg.ecx);
        set_colorkey(ret, reg.eax);
    } else if (reg.edx == API_PUTINTX) {
        s_printf("%X", reg.eax);
    }

    return ret;
}

