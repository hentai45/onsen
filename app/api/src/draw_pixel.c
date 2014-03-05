#include "apino.h"
#include "utils.h"

void draw_pixel(int sid, int x, int y, unsigned int color)
{
    api4(API_DRAW_PIXEL, sid, x, y, (int) color);
}

