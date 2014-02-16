#include "onsen.h"
#include "utils.h"

void exit_app(int status)
{
    api1(API_EXIT_APP, status);
}

