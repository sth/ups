/*
 *  Basic test that registers are being displayed.
 */

#include <stdio.h>

int
main (int argc, char **argv)
{
    register float f;
    register unsigned long long ll = 0;
    register long t = 0xfedc, u = 0xabcd;
    register double d;

/*
 *  Instructions
 *  ------------
 *  1) Put a breakpoint on the call to 'exit()' at the end of the code
 *     then press the 'Start' button.
 *  2) Display each of the variables.
 */

    ll = (long long)0x123456789abc;
    f = 9.87654321;
    d = 1.23456e101;

    exit (0);

}
