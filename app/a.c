#include "onsen.h"

void main(void)
{
    int pid = chopsticks();

    if (pid == 0) {
        printf("I am a child.\n");
    } else {
        printf("I am a parent. My child's pid is %d.\n", pid);
    }

    exit(0);
}

