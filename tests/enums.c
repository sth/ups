/* Rod Armstrong <rod@san-jose.tt.slb.com> 16th June 2000*/
/*
 *  A handy trick to look at enum values is to use an add expr,
 *  in this case (Pat_execution_mode)1, and bump
 *  the number to see all the values. Try this program:
 */
/*
 *  'gcc' may optimise this away with "-g -O2" in compiler flags so
 *  you may have to manually build this without the "-O2".
 */

typedef enum { INDIVIDUAL = 0x0001, 
	       BURST      = 0x0002, 
	       CHAIN      = 0x0004} Pat_execution_mode;
main()
{
    Pat_execution_mode p;

    p = INDIVIDUAL;
    p = BURST;
    p = CHAIN;
    p = 0;
}

/*
Doing a 'Dup' and editing the value you should get:

      main                                             enum.c:9
         (Pat_execution_mode)1              INDIVIDUAL
         (Pat_execution_mode)2              BURST
         (Pat_execution_mode)3              <3>
         (Pat_execution_mode)4              CHAIN
*/
