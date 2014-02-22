#include "onsen.h"
#include "utils.h"

void timer_free(int tid)
{
    api1(API_TIMER_FREE, tid);
}

