#include "apino.h"
#include "utils.h"

int api_getkey(int mode)
{
    return api1(API_GETKEY, mode);
}

