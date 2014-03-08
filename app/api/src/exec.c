#include "apino.h"
#include "utils.h"

int exec(const char *fname)
{
    return api1(API_EXEC, (int) fname);
}

