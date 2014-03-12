#include "myc.h"
#include "onsen.h"

void main(void)
{
    int pid = fork();

    if (pid == 0) {
        printf("I am a child. pid = %d\n", getpid());
        execve("walk", 0, 0);
        printf("hi\n");
    } else {
        printf("I am a parent. My child's pid is %d. pid = %d\n", pid, getpid());
    }

    exit(0);
}

