/*
 *  Test display of register variables.
 *  Also tests registers in core files.
 */

#include <stdio.h>
#include <string.h>

int one (int n, int a, int b);

int one (int n, int a, int b)
{
    register unsigned int d, e, f, g, h, i;

    d = 0x123400;
    e = 0x567800;
    f = 0x9abc00;
    g = 0xdef000;
    h = 0x135700;
    i = 0x9bce00;

    if (n > 1) strcat (NULL, NULL);

    return (d);
}

int main (int argc, char **argv)
{
    register unsigned int d, e, f, g, h, i;

/*
 *  Instructions
 *  ------------
 *  1) Put a breakpoints on the 'return()' call above.
 *  2) Press 'Start' and check all the variables.
 *     Use the 'Format' button to show as hex.
 *     Press 'Continue'
 *  3) Quit from 'ups' and just run the program with a parameter, and it
 *     should core dump.  If you don't get a core dump they may be disabled,
 *     this may fix it :
 *         ulimit -c unlimited
 *     and try again.
 *  4) Run 'ups' on the program and the core file.
 *     Look at the stack trace and the variables,
 */

    d = 0x431200;
    e = 0x876500;
    f = 0xcba900;
    g = 0xffed00;
    h = 0x753100;
    i = 0xecb900;

    i = one (argc, f, g);

    exit (0);
}

