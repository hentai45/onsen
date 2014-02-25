#include "apino.h"
#include "utils.h"

void api_putchar(char ch)
{
    api1(API_PUTCHAR, ch);
}

