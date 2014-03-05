#include "apino.h"
#include "utils.h"

struct MSG;

int get_message(struct MSG *msg)
{
    return api1(API_GET_MESSAGE, (int) msg);
}

