#include <stdio.h>

#if 0
Hi Rod & Ian.

I found a problem with the display of local variables declared to be of type
"register long long".  The problem is illustrated by the following test
program.  Commentary shows where variables displayed correctly or incorrectly
for version 3.33.  I got the same results compiling with gcc and SC4.

=============        reg_ll.c    =================
#endif
/*  Observed problems with display of long longs when they
**  were declared register
**
**  This proved to be a problem with types of size > 4 declared register
*/
typedef struct TwoLongs
{
   long l1;
   long l2;
} TwoLongs;
void ptwolongs(TwoLongs tl)
{
    printf("TwoLongs: %x %x\n", tl.l1,tl.l2);  /* tl displays OK here */
}

void plonglong( unsigned long long ll)
{
    printf("long long is %qx\n", ll);        /* ll displays OK here */
}
main()
{
    register unsigned long long ll = 0;
    register TwoLongs twolongs;

/*
 *  Instructions
 *  ------------
 *  1) Put breakpoints on the two 'printf()' calls above.
 *  2) Press the 'Start' button to get to the first breakpoint and display
 *     the all the variables.  Use the 'Format' button to show as hex.
 *  3) Press 'Continue' and display "tl".
 */

    ll = (long long)0x123456789abc;
    plonglong(ll);			     /* ll displays WRONG here */
    twolongs.l1 = 0x13579bdf;
    twolongs.l2 = 0x2468ace0;		     /* twolongs displays OK here */
    ptwolongs(twolongs);

}
