/*
 *  Testing conditional breakpoints using interpreted code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char **argv)
{
    int i;

/*
 *  Instructions
 *  ------------
 *  1) Supply <n> strings as command line parameters.
 *     Either do this when you run the program or add them now by a middle
 *     mouse click at the end of the line in the information window that just
 *     reads "interp_stop", type some words, then press ESC key. 
 *  2) Put a breakpoint on the 'printf()' line.
 *     Edit the breakpoint to make it conditional on "i" e.g.
 *         if (i == 3) #stop;
 *  3) Press 'Run' and check 'ups' stops when expected.
 *     Press 'Continue'
 *  4) Edit the breakpoint to make it conditional on a string e.g.
 *         if (strcmp(argv[i],"abc") == 0) #stop;
 *  5) Press 'Run' and check 'ups' stops when expected.
 */

    for (i = 1; i < argc; i++)
    {
	printf ("[%d] %s\n", i, argv[i]);
    }
    exit (0);
}

