/* recurse.c */
/* From : Bob Carragher <bob@fla.fujitsu.com> */

/*
 *  This is just to test what happens when you do infinite recursion,
 *  and how ups copes, either running the program or examining the core
 *  file.
 */

recurse(int x)
{
    recurse(x+1);
}

main()
{
    recurse(0);
}

