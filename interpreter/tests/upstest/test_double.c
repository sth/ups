#include <stdio.h>

int main()
{
	double a, b;
	int x,y;

	a = 55.55;
	b = 65.87;

	if (a == 55.55 && b == 65.87 && (int)a == 55 && (int)b == 65)
		printf("test ok\n");
	else
		printf("test failed\n");
	return 0;
}	
