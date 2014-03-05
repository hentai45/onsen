#include "apino.h"
#include "onsen.h"
#include "utils.h"

void draw_line(int sid, POINT *pt0, POINT *pt1, unsigned int color)
{
    api4(API_DRAW_LINE, sid, (int) pt0, (int) pt1, (int) color);
}

