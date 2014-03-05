#include "apino.h"
#include "utils.h"

void update_surface(int sid)
{
    api1(API_UPDATE_SURFACE, sid);
}

