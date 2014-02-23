/**
 * OS 側 はりぼてAPI 処理
 */


//=============================================================================
// 非公開ヘッダ

// API 番号と機能の対応などが書かれたヘッダ。/api/haribote/include
#include "haribote.h"

#include "debug.h"


void api_exit_app(int exit_status);

typedef struct _PUSHAL_REGISTERS {
    int edi, esi, ebp, esp, ebx, edx, ecx, eax;
} PUSHAL_REGISTERS;



//=============================================================================
// 公開関数

int hrb_api(PUSHAL_REGISTERS reg)
{
    if (reg.edx == 1) {
        dbgf("%c", reg.eax);
    } else if (reg.edx == 2) {
        dbgf("%s", reg.ebx);
    } else if (reg.edx == 4) {
        api_exit_app(0);
    }

    return 0;
}

