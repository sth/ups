/*
 *  Instructions
 *  ------------
 *  1) Run this program to get a core dump.
 *  2) Run 'ups' on the program and the coredump and look at the stack.
 */

#include <assert.h>

void one (int a);
void two (int b);
void three (int c);

void
three (int c)
{
    assert (0);
}

void
two (int b)
{
    three (b);
}

void
one (int a)
{
    two (a);
}

int
main (int argc, char **argv)
{
    one (argc);
    exit (0);
}

