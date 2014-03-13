#include "myc.h"
#include "onsen.h"

void main(void)
{
    char *p = (char *) malloc(256);
    printf("p = %p\n", p);

    snprintf(p, 256, "test malloc");
    printf("%s\n", p);

    char *p2 = (char *) malloc(256);
    printf("p2 = %p\n", p2);

    free(p);

    char *p3 = (char *) malloc(256);
    printf("p3 = %p\n", p3);

    char *p4 = (char *) malloc(0x4000);
    printf("p4 = %p\n", p4);

    exit(0);
}

