#ifndef __UPS_ASSERT_H
#define __UPS_ASSERT_H

#include <stdio.h>

#ifdef NDEBUG
#define assert(x) ((void) 0)
#else
#define assert(x) __ups_assert(__FILE__, __LINE__, x)
static void __ups_assert(const char *__file, int __line, int __true) { 
	extern void exit(int);
	if (!__true) { 
		printf("assertion failed at %s(%d)\n", __file, __line); 
		exit(1); 
	}
}
#endif

#endif
