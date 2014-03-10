#include "apino.h"
#include "onsen.h"
#include "utils.h"

void draw_text_bg(int sid, int x, int y, unsigned int fg_color, unsigned int bg_color, const char *s)
{
    struct POINT pt = { .x = x, .y = y };

    api5(API_DRAW_TEXT_BG, sid, (int) &pt, (int) fg_color, (int) bg_color, (int) s);
}

