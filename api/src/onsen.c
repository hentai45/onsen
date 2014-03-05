#ifndef HEADER_ONSEN
#define HEADER_ONSEN

#include "keycode.h"
#include "msg.h"

struct MSG;

void exit_app(int status);
int get_message(struct MSG *msg);


//-----------------------------------------------------------------------------
// タイマ
//-----------------------------------------------------------------------------

int  timer_new(void);
void timer_free(int tid);
void timer_start(int tid, unsigned int timeout_ms);


//-----------------------------------------------------------------------------
// グラフィック
//-----------------------------------------------------------------------------

int  create_window(int w, int h, const char *name);
void update_screen(int sid);
void fill_surface(int sid, unsigned int color);
void draw_text(int sid, int x, int y, unsigned int color, const char *s);


//-----------------------------------------------------------------------------
// デバッグ
//-----------------------------------------------------------------------------

void dbg_str(const char *s);
void dbg_int(int val);
void dbg_intx(unsigned int val);
void dbg_newline(void);

#endif

