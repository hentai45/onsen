#include "apino.h"
#include "utils.h"

void update_char(int sid, int x, int y)
{
    api3(API_UPDATE_CHAR, sid, x, y);
}

