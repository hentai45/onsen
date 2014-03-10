#include "apino.h"
#include "onsen.h"
#include "utils.h"

void fill_rect(int sid, int x, int y, int w, int h, unsigned int color)
{
    struct RECT rect = { .x = x, .y = y, .w = w, .h = h };

    api3(API_FILL_RECT, sid, (int) &rect, (int) color);
}

