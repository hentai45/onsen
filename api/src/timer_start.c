#include "onsen.h"
#include "utils.h"

void timer_start(int tid, unsigned int timeout_ms)
{
    api2(API_TIMER_START, tid, timeout_ms);
}

