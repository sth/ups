#include <errno.h>
#include <stdio.h>

int main()
{
	errno = EDOM;
	perror("Testing");
	errno = ERANGE;
	perror("Testing");
	return 0;
}
