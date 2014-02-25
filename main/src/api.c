/**
 * OS 側 API 処理
 *
 * @note
 * @par API が処理される流れ
 * -# アプリケーションがAPIを呼び出す(INT 0x44)
 * -# トラップゲートが呼び出される
 * -# asmapi.S の asm_api 関数が呼び出される
 * -# 本ファイルの onsen_api 関数が呼び出される
 * -# onsen_api がシステムコールを処理する関数を呼び出す
 */


//=============================================================================
// 公開ヘッダ

// onsen_api 関数は asmapi.S から呼ばれる


//=============================================================================
// 非公開ヘッダ

// API 番号と機能の対応などが書かれたヘッダ。/api/include
#include "onsen/apino.h"

#include "asmfunc.h"
#include "debug.h"
#include "gdt.h"
#include "graphic.h"
#include "msg.h"
#include "msg_q.h"
#include "task.h"
#include "timer.h"


void api_exit_app(int exit_status);


//=============================================================================
// 公開関数

/**
 * OS 側 API 処理メイン（API 番号により処理を振り分ける）
 *
 * @note
 * @par ds, es レジスタ
 * この時点で ds, es レジスタは API 呼び出し側の ds, es のまま。
 * この関数の始めで OS 用のセグメントに変更して、終わりで元に戻している。
 *
 * @par アプリケーションから渡されたポインタからデータを取得する場合
 * -# ポインタがセグメントのリミットを超えた値でないことを調べる(今は省略)
 *    TODO
 * -# 渡されたポインタに addr_app_ds 変数を足して OS からアクセスできる
 *    ポインタに変換する
 */
int onsen_api(int api_no, int arg1, int arg2, int arg3, int arg4, int arg5)
{
    switch (api_no) {
    case API_EXIT_APP:
        api_exit_app(arg1);
        break;

    case API_GET_MESSAGE:
        return get_message((MSG *) arg1);

    case API_TIMER_NEW:
        return timer_new();

    case API_TIMER_FREE:
        timer_free(arg1);
        break;

    case API_TIMER_START:
        timer_start(arg1, arg2);
        break;

    case API_DBG_STR:
        dbgf((char *) arg1);
        break;

    case API_DBG_INT:
        dbgf("%d", arg1);
        break;

    case API_DBG_INTX:
        dbgf("%X", arg1);
        break;

    case API_DBG_NEWLINE:
        dbgf("\n");
        break;

    case API_GET_SCREEN:
        return get_screen();

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

    return 0;
}


//=============================================================================
// 非公開関数

void api_exit_app(int exit_status)
{
    MSG msg;
    msg.message = MSG_REQUEST_EXIT;
    msg.u_param = get_pid();
    msg.l_param = exit_status;
    msg_q_put(g_root_pid, &msg);

    for (;;) {
        // メッセージが処理されるのをここで待つ
        // これがないとアプリケーションが return でおかしなところに飛んでいく
    }
}

