#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{

	double d ;
	int i;

	if (argc != 2) {
		return 1;
	}

	d = atof(argv[1]);		/* BUG */
	i = (int)d;			/* BUG */
	printf("atof(%s) = %g\n", argv[1], (double)d);
	printf("(int)atof(%s) = %d\n", argv[1], i);

	return 0;
}
	
