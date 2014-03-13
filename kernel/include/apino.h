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

#define API_BRK          45

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
#define API_UPDATE_TEXT     1003
#define API_UPDATE_CHAR     1004
#define API_FILL_SURFACE    1005
#define API_FILL_RECT       1006
#define API_SCROLL_SURFACE  1007
#define API_DRAW_PIXEL      1008
#define API_DRAW_LINE       1009
#define API_DRAW_TEXT       1010
#define API_DRAW_TEXT_BG    1011
#define API_ERASE_CHAR      1012

#endif

