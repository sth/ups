/*
 *  Instructions
 *  ------------
 *  1) Run this program to get a core dump.
 *  2) Run 'ups' on the program and the coredump and look at the stack.
 *     The call stack is wrong on some systems.
 */

#include <string.h>

void
three (int c)
{
    strcat (0,0);
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

