/**
 * OS 側 API 処理
 *
 * API が処理される流れ
 * 1. アプリケーションがAPIを呼び出す(INT 0x44)
 * 2. トラップゲートが呼び出される
 * 3. asmapi.S の asm_api 関数が呼び出される
 * 4. 本ファイルの onsen_api 関数が呼び出される
 * 5. onsen_api がシステムコールを処理する関数を呼び出す
 */


//=============================================================================
// ヘッダ

#ifndef HEADER_API
#define HEADER_API

void api_exit_app(int exit_status);

typedef struct _API_REGISTERS {
    // asmapi.S で積まれたスタックの内容
    unsigned int edi, esi, ebp, esp0, ebx, edx, ecx, eax;  // pushal
    unsigned int ds, es;

    // 以下は、ソフトウェア読み込み時にCPUが自動でpushしたもの
    unsigned int eip, cs, eflags, esp, ss;
} API_REGISTERS;

#endif


// API 番号と機能の対応などが書かれたヘッダ。/api/include
#include "onsen/apino.h"

#include "asmfunc.h"
#include "debug.h"
#include "gdt.h"
#include "graphic.h"
#include "msg.h"
#include "msg_q.h"
#include "str.h"
#include "task.h"
#include "timer.h"



//=============================================================================
// 関数

/**
 * OS 側 API 処理メイン（API 番号により処理を振り分ける）
 *
 * asmapi.S から呼ばれる
 */
int onsen_api(API_REGISTERS regs)
{
    int ret = 0;

    int api_no = regs.eax;
    int arg1 = regs.ebx;
    int arg2 = regs.ecx;
    int arg3 = regs.edx;
    int arg4 = regs.esi;
    int arg5 = regs.edi;

    switch (api_no) {
    case API_EXIT_APP:
        api_exit_app(arg1);
        break;

    case API_GET_MESSAGE:
        ret = get_message((MSG *) arg1);
        break;

    case API_WRITE:
        ret = f_write(arg1, (const void *) arg2, arg3);
        break;

    case API_TIMER_NEW:
        ret = timer_new();
        break;

    case API_TIMER_FREE:
        timer_free(arg1);
        break;

    case API_TIMER_START:
        timer_start(arg1, arg2);
        break;

    case API_DBG_STR:
        printf((char *) arg1);
        break;

    case API_DBG_INT:
        printf("%d", arg1);
        break;

    case API_DBG_INTX:
        printf("%X", arg1);
        break;

    case API_DBG_NEWLINE:
        printf("\n");
        break;

    case API_CREATE_WINDOW:
        ret = new_window(0, 0, arg1, arg2, (char *) arg3);
        break;

    case API_UPDATE_SCREEN:
        update_surface(arg1);
        break;

    case API_FILL_SURFACE:
        fill_surface(arg1, arg2);
        break;

    case API_DRAW_TEXT:
        draw_text(arg1, arg2, arg3, arg4, (char *) arg5);
        break;
    }

    return ret;
}


void api_exit_app(int exit_status)
{
    MSG msg;
    msg.message = MSG_REQUEST_EXIT;
    msg.u_param = g_pid;
    msg.l_param = exit_status;
    msg_q_put(g_root_pid, &msg);

    for (;;) {
        // メッセージが処理されるのをここで待つ
        // これがないとアプリケーションが return でおかしなところに飛んでいく
    }
}

