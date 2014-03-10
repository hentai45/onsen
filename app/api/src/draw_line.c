#include "apino.h"
#include "onsen.h"
#include "utils.h"

void draw_line(int sid, int x0, int y0, int x1, int y1, unsigned int color)
{
    struct POINT pt0 = { .x = x0, .y = y0 };
    struct POINT pt1 = { .x = x1, .y = y1 };

    api4(API_DRAW_LINE, sid, (int) &pt0, (int) &pt1, (int) color);
}

