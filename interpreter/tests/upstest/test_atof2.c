#include <stdlib.h>
#include <stdio.h>

int isdigit(int c) { return c >= '0' && c <= '9'; }
int isspace(int c) { return c == ' ' || c == '\t'; }

double atof(char s[])
{
	double val, power;
	int i, sign;

	for (i = 0; isspace(s[i]); i++)
		;
	sign = (s[i] == '-') ? -1 : 1;
	if (s[i] == '+' || s[i] == '-')
		i++;
	for (val = 0.0; isdigit(s[i]); i++)
		val = 10.0 * val + (s[i] - '0');
	if (s[i] == '.')
		i++;
	for (power = 1.0; isdigit(s[i]); i++) {
		val = 10.0 * val + (s[i] - '0');
		power *= 10.0;
	}
	return sign * val / power;
}	


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
	
