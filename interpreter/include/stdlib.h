#ifndef __UPS_STDLIB_H
#define __UPS_STDLIB_H

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

/* MB_CUR_MAX unsupported */

#include <stddef.h>	/* for NULL */

#undef  RAND_MAX
#define	RAND_MAX	2147483647

/* div_t unsupported */
/* ldiv_t unsupported */

/* size_t defined in stddef.h */
/* wchar_t unsupported */

extern double atof  (const char *)  ;
extern int atoi  (const char *)  ;
extern long int atol  (const char *)  ;
extern double strtod  (const char *, char **)  ;
extern long int strtol  (const char *, char **, int)  ;
extern unsigned long int strtoul  (const char *, char **, int)  ;
extern int rand  (void)  ;
extern void srand  (unsigned int)  ;
extern void *calloc  (size_t, size_t)  ;
extern void free  (void *)  ;
extern void *malloc  (size_t)  ;
extern void *realloc  (void *, size_t)  ;
extern void abort  (void) ;
/* atexit() unsupported */
extern void exit  (int) ;
extern char *getenv  (const char *)  ;
extern int system  (const char *)  ;
extern void *  bsearch  (const void *, const void *,
			     size_t, size_t,
			     int (*)(const void *, const void *))  ;
extern void    qsort  (void *, size_t, size_t,
			int (*)(const void *, const void *))  ;

extern int      abs  (int)  ;
/* div() unsupported */
extern long int labs  (long int) ;
/* ldiv() unsupported */
/* mblen() unsupported */
/* mbtowc() unsupported */
/* wctomb() unsupported */
/* mbstowcs() unsupported */
/* wcstombs() unsupported */

#endif
