#include "apino.h"
#include "utils.h"

void erase_char(int sid, int x, int y, unsigned long color, int update)
{
    api5(API_ERASE_CHAR, sid, x, y, color, update);
}

