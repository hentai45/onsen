#include "onsen.h"

void main(void)
{
    int pid = chopsticks();

    if (pid == 0) {
        printf("I am a child. pid = %d\n", getpid());
        exec("walk");
        printf("hi\n");
    } else {
        printf("I am a parent. My child's pid is %d. pid = %d\n", pid, getpid());
    }

    exit(0);
}

