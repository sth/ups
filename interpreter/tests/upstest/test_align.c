#define test_align(type) \
	{ \
		struct _st { char dummy; type me; }; \
		printf("Sizeof %20s = %d,  ", #type, \
			sizeof(type)); \
		printf("Alignment = %d\n", \
			offsetof(struct _st, me)); \
	}

#include <stddef.h>
#include <stdio.h>

int main()
{
	struct structure { int dummy; };

	test_align(char);
	test_align(signed char);
	test_align(unsigned char);
	test_align(short);
	test_align(unsigned short);
	test_align(int);
	test_align(unsigned);
	test_align(long);
	test_align(unsigned long);
	test_align(float);
	test_align(double);
	test_align(long double);

	test_align(void*);
	test_align(char*);
	test_align(signed char*);
	test_align(unsigned char*);
	test_align(short*);
	test_align(unsigned short*);
	test_align(int*);
	test_align(unsigned*);
	test_align(long*);
	test_align(unsigned long*);
	test_align(float*);
	test_align(double*);
	test_align(long double*);

	test_align(struct structure);
	test_align(struct structure*);
	return 0;
}
