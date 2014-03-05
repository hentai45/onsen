#include "apino.h"
#include "utils.h"

void fill_surface(int sid, unsigned int color)
{
    api2(API_FILL_SURFACE, sid, color);
}

