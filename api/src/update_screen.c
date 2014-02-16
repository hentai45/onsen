#include "onsen.h"
#include "utils.h"

void update_screen(int sid)
{
    api1(API_UPDATE_SCREEN, sid);
}

