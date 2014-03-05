#ifndef HEADER_ONSEN_API_NO
#define HEADER_ONSEN_API_NO

#define API_EXIT_APP      1
#define API_GET_MESSAGE   9

#define API_WRITE       10

//-----------------------------------------------------------------------------
// タイマ
//-----------------------------------------------------------------------------

#define API_TIMER_NEW     100
#define API_TIMER_FREE    101
#define API_TIMER_START   102


//-----------------------------------------------------------------------------
// グラフィック
//-----------------------------------------------------------------------------

#define API_CREATE_WINDOW  1000
#define API_UPDATE_SCREEN  1001
#define API_FILL_SURFACE  1002
#define API_DRAW_TEXT     1005


//-----------------------------------------------------------------------------
// デバッグ
//-----------------------------------------------------------------------------

#define API_DBG_STR     9000
#define API_DBG_INT     9001
#define API_DBG_INTX    9002
#define API_DBG_NEWLINE 9003


#endif

