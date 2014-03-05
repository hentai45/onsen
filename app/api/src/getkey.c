#include "apino.h"
#include "utils.h"

int getkey(int wait)
{
    return api1(API_GETKEY, wait);
}

