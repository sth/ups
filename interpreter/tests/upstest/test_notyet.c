#include <stdio.h>

int main()
{
	remove("xyz");
	perror("remove");
	return 0;
}
