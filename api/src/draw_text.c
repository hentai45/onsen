#include "onsen.h"
#include "utils.h"

void draw_text(int sid, int x, int y, unsigned int color, const char *s)
{
    api5(API_DRAW_TEXT, sid, x, y, (int) color, (int) s);
}

