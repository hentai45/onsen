#include "apino.h"
#include "utils.h"

int execve(const char *fname, char *const argv[], char *const envp[])
{
    return api3(API_EXECVE, (int) fname, (int) argv, (int) envp);
}

