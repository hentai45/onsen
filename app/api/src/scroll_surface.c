#include "apino.h"
#include "utils.h"

int scroll_surface(int sid, int cx, int cy)
{
    return api3(API_SCROLL_SURFACE, sid, cx, cy);
}

