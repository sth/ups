#ifndef __UPS_STDARG_H
#define __UPS_STDARG_H

#ifndef __UPS_VA_LIST_DEFINED
	typedef char *va_list;
#	define __UPS_VA_LIST_DEFINED
#endif

/* PORTABILITY */
#undef __ups_roundup
#define __ups_roundup(x,n) (((x)+((n)-1))&(~((n)-1)))

#undef va_arg
#undef va_end
#undef va_start

/* The 4 used below represents the size of each stack word - which is
 * 4 bytes (unsigned long) in UPS C Interpreter.
 */
#define va_arg(ap, T)	\
	(*(T *)(((ap) +=__ups_roundup(sizeof(T),4)) - \
			__ups_roundup(sizeof(T),4)))
#define va_end(ap)		(void)0
#define va_start(ap, A)	\
	(void)((ap) = (char *)&(A) + __ups_roundup(sizeof(A),4))

#endif
