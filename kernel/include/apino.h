#ifndef HEADER_ONSEN_API_NO
#define HEADER_ONSEN_API_NO

#define API_EXIT          1
#define API_FORK          2
#define API_READ          3
#define API_WRITE         4
#define API_OPEN          5
#define API_CLOSE         6
#define API_EXECVE       11
#define API_GETPID       20

#define API_GET_MESSAGE  200
#define API_GETKEY       201


//-----------------------------------------------------------------------------
// タイマ
//-----------------------------------------------------------------------------

#define API_TIMER_NEW     100
#define API_TIMER_FREE    101
#define API_TIMER_START   102


//-----------------------------------------------------------------------------
// グラフィック
//-----------------------------------------------------------------------------

#define API_CREATE_WINDOW   1000
#define API_UPDATE_SURFACE  1002
#define API_FILL_SURFACE    1004
#define API_DRAW_PIXEL      1005
#define API_DRAW_LINE       1006
#define API_DRAW_TEXT       1008


#endif

