#include <stdio.h>
int main()
{
	int i = -1;
	unsigned long ul = ~0;

	if (i < ul)
		printf("test failed\n");
	else
		printf("test ok\n");
	return 0;
}
