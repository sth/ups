/*
 *  'sleep()' to test auto raise/lower features in UPS
 */

#include <stdio.h>

int main (int argc, char **argv)
{
    int i = 1, s;

    setbuf (stdout, NULL);
    while (i < argc)
    {
	s = atoi (argv[i]);
	printf ("Sleeping for %d seconds ", s);
	sleep (s);
	printf ("\n");
	i++;
    }
    exit (0);
}

