
#include <stdio.h>
#include <stdlib.h>

int another_global;
static int another_static;
extern int global_int;

static tell_static(char *msg)
{
    fprintf(stdout, msg);
}

void tell(char *msg)
{
    const char *constant_x;
    const int constant_i = 42;

    constant_x = msg;
    tell_static(msg);
}

