/*
 *	Testing mixed C and Fortran programs.
 *	Comprises : test2_c.c test2_sub.F test2_foo.c
 *
 *	For GNU f77 (and others ?) symbols for routines have an underscore
 *	appended to the name.
 */
#define b_r(x) b_r__(x)
void main ()
{
    int i = 98;
    b_r(&i);
    i++;
    b_r__(&i);
    exit (0);
}
