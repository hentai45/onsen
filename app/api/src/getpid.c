#include "apino.h"
#include "utils.h"

int getpid(void)
{
    return api0(API_GETPID);
}

