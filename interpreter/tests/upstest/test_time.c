#include <time.h>
#include <stdio.h>

int main()
{
	time_t t1, t2;
	struct tm *tmptr;
	double d;
	int i;
	extern unsigned int sleep(unsigned int);

	printf("Clock time = %ld\n", clock());

	t1 = time(NULL);
	sleep(2);
	t2 = time(NULL);

	printf("Time 1 = %ld, Time 2 = %ld\n", t1, t2);
	d = difftime(t2,t1);
	i = (int) d;				/* BUG */
	printf("Difference (double) = %g secs\n", d);
	printf("Difference (int) = %d secs\n", i);

	tmptr = localtime(&t1);
	printf("Current date/time = %s\n", asctime(tmptr));

	return 0;
}	
	
