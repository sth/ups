#include <stdio.h>
#include <stdarg.h>

int variadic_string(int count, ...)
{
	int i;
	va_list ap;

	va_start(ap, count);

	for (i = 0; i < count; i++) {
		char *s;

		s = va_arg(ap, char *);
		printf("%d: %s\n", i, s);
	}

	va_end(ap);
}

int variadic_double(int count, ...)
{
	int i;
	va_list ap;

	va_start(ap, count);

	for (i = 0; i < count; i++) {
		double d;

		d = va_arg(ap, double);
		printf("%d: %f\n", i, d);
	}

	va_end(ap);
}

int main()
{
	double d1 = 7.67;
	double d2 = -67.1005;
	double d3 = -590.01;
	variadic_string(3, "hello", "dibyendu", "majumdar");
	variadic_double(3, d1, d2, d3);

	return 0;
}
