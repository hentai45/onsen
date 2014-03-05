#include "apino.h"
#include "utils.h"

void exit(int status)
{
    api1(API_EXIT, status);
}

