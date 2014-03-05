#include "apino.h"
#include "utils.h"

void dbg_str(const char *s)
{
    api1(API_DBG_STR, (int) s);
}


