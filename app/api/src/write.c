#include "apino.h"
#include "utils.h"

int write(int fd, const void *buf, int cnt)
{
    return api3(API_WRITE, fd, (int) buf, cnt);
}


