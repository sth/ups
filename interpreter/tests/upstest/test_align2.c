/*
 *             Testfile for certain system features
 *                         Joerg Schoen 1996
 *
 *  Check imposed and required alignments of data types:
 *  1. "Imposed" means alignment used in struct's. Are there any padding
 *     bytes inserted in structure definitions to force alignment of types
 *     on multiples of a certain alignment value? If so, it prints
 *       -DALGN_TYPE=<alignment>
 *  2. "Required" means necessary to avoid bus errors. A variable can only
 *     be accessed at multiples of an alignment value, if not, usually
 *     a bus error results. If this is detected, the program prints
 *       -DNEED_ALGN_TYPE=<alignment>
 *
 *  The basic data types short, int, long, double and long double are tested.
 *
 *  The following table shows the results for some platforms I have access to
 *   (values are imposed/required. If none is given for required, the machine
 *   can access values at any address):
 *
 *============================================================================
 *                |             |        |        |        |        |  LONG  |
 * System         |   Compiler  | SHORT  |  INT   |  LONG  | DOUBLE | DOUBLE |
 *                |             |        |        |        |        |        |
 *============================================================================
 * AIX 3.2, 4.1   | xlc         |  2     |  4     |  4     |   4    |  4     |
 *----------------------------------------------------------------------------
 * GNU 2.7.2      | gcc         |  2     |  4     |  4     |   4    |  4     |
 *----------------------------------------------------------------------------
 * SUN 5.5        | cc          |  2/2   |  4/4   |  4/4   |   8/4  |  8/4   |
 *----------------------------------------------------------------------------
 * HP A.09.05     | c89         |  2/2   |  4/4   |  4/4   |   8/8  |  8/4   |
 *----------------------------------------------------------------------------
 * SGI 5.2        | cc          |  2/2   |  4/4   |  4/4   |   8/4  |  8/4   |
 *                | cc -mips2   |  2/2   |  4/4   |  4/4   |   8/8  |  8/8   |
 *----------------------------------------------------------------------------
 * DEC V3.2       | cc          |  2     |  4     |  8     |   8    |  8     |
 *----------------------------------------------------------------------------
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int testalign(void); /*  prototype  */

int main(int argc,char *argv[])
{
  int ret = 0;

  /*  Check for required alignments  */
  if(testalign()) ret = 10;

  return(ret);
}

int testalign(void)
{
  volatile char *a;
  volatile int i,what;

  /*  Assume long double is biggest type  */
  if((a = (volatile char *)malloc(2 * sizeof(long double))) == NULL)
    goto error;

  for(what = 1 ; what <= 5 ; what++) {
    volatile char *name;
    volatile int align,len;

#define TESTALGN1(TYPE,NAME) \
      name = NAME; \
      { \
	struct { \
	  char c; \
	  TYPE var; \
        } sTest; \
\
        if(((char*)&(sTest.var) - (char*)&sTest) != sizeof(sTest.c)) \
          printf("-DALGN_%s=%d",name,(char*)&(sTest.var) - (char*)&sTest); \
      } \
      len = sizeof(TYPE)

    switch(what) {
    case 1:
      TESTALGN1(short,"SHORT");
      break;
    case 2:
      TESTALGN1(int,"INT");
      break;
    case 3:
      TESTALGN1(long,"LONG");
      break;
    case 4:
      TESTALGN1(double,"DOUBLE");
      break;
    case 5:
      TESTALGN1(long double,"LONGDOUBLE");
      break;
    }
#undef TESTALGN1

    align = len; /*  assume worst case for alignment  */

    for(i = -1 ; i < len ; i++) {
      if(i == -1) {
	int sig;

	/*  Is executed when jump buffer is initialized  */
	if(i == -1) continue;
      }

#define TESTALGN2(TYPE) *(TYPE *)&a[i] = 11
      switch(what) {
      case 1: TESTALGN2(short);       break;
      case 2: TESTALGN2(int);         break;
      case 3: TESTALGN2(long);        break;
      case 4: TESTALGN2(double);      break;
      case 5: TESTALGN2(long double); break;
      }
#undef TESTALGN2

      /*  If we are here, the access succeeded  */
      if(i && i < align) align = i;
    }

    /*  Print message  */
    if(align != 1) printf(" -DNEED_ALGN_%s=%d",name,align);

    printf("\n");
  }

  free((void *)a);

  return(0);
error:
  fprintf(stderr,"WARNING: Error in testalign \"%d\"!\n",errno);
  return(-1);
}
