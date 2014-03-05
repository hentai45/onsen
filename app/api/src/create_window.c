#include "apino.h"
#include "utils.h"

int create_window(int w, int h, const char *name)
{
    return api3(API_CREATE_WINDOW, w, h, (int) name);
}

