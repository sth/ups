/*
 *	Testing handling of underscores in Fortran names; also C subroutines
 *	Contributed by Satish Balay <balay@mcs.anl.gov>
 *	Comprises : test2_main.F test2_sub.F test2_foo.c
 *
 *	For GNU f77 (and others ?) symbols for routines have an underscore
 *	appended to the name.  So for a C routine to be callable it needs one.
 */
#include <stdio.h>
void foo_(int* a)
{
	printf(" print from c = %d\n",*a);
}

