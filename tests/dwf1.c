/*
gcc31 -g -c junk.c ; gcc31 -g -c junk2.c ; gcc -o ~/bin/junk3 -g junk.o junk2.o
*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int global_int = 0;
static int static_int;

int main(int argc, char **argv)
{
    long l;
    unsigned long ul = 0;

    ul = 0xabcdef;
    tell("Hello "); tell("Ian\n");
    if (argc <= 1)
    {
	enum { yes, no, maybe } ynm;
	char *msg = "block block\n";

	ynm = maybe;
	tell(strchr(msg, ' '));
    }
    else
    {
	int i;
	for (i = 1; i < argc; i++) {
	    int k = i + atoi(argv[i]);
	    tell(argv[i]);
	    tell(" ");
	}
	tell ("\n");
    }
    exit (1);
}

