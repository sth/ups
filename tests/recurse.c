/* recurse.c */
/* From : Bob Carragher <bob@fla.fujitsu.com> */

void recurse(int x);

/*
 *  This is just to test what happens when you do infinite recursion,
 *  and how ups copes, either running the program or examining the core
 *  file.
 */

void
recurse(int x)
{
    recurse(x+1);
}

int
main()
{
    recurse(0);
    exit (0);
}

