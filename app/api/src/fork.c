#include "apino.h"
#include "utils.h"

int fork(void)
{
    return api0(API_FORK);
}

