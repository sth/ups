#include <stdio.h>

int main()
{

/*	int ch = getchar(); */
	int ch = 'e';

	switch (ch) {

	case 'a':
	case 'b':
	case 'c':
	case 'd':
		printf("abcd\n");
		break;

	case 'e':
	case 'f':
		printf("ef\n");

	case 'g':
		printf("g\n");
		break;

	default:
		break;
	}

	return 0;
}
	
