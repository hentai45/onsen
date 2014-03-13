#include "apino.h"
#include "utils.h"

int brk(void *new_brk)
{
    return api1(API_BRK, (int) new_brk);
}

