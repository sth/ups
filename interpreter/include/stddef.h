/***
* stddef.h for UPS C Interpreter
*/
#ifndef __UPS_STDDEF_H
#define __UPS_STDDEF_H

#undef NULL
#define NULL ((void *)0)

#ifndef __UPS_PTRDIFF_T_DEFINED
	typedef int ptrdiff_t;
#	define __UPS_PTRDIFF_T_DEFINED
#endif

#undef  offsetof
#define offsetof(T, member)	((size_t)&((T *)0)->member)

#ifndef __UPS_SIZE_T_DEFINED
	typedef unsigned int size_t;
#	define __UPS_SIZE_T_DEFINED
#endif

/* wchar_t unsupported */

#endif
