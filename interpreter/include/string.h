#ifndef __UPS_STRING_H
#define __UPS_STRING_H

#include <stddef.h>		/* for NULL and size_t */

extern char *strchr  (const char *, int)  ;
extern char *strcpy  (char *, const char *)  ;
extern char *strcat  (char *, const char *)  ;
extern int strcmp  (const char *, const char *)  ;
/* strcoll() not supported */
extern size_t strcspn  (const char *, const char *)  ;
extern char *strerror(int);
extern size_t strlen  (const char *)  ;
extern char *strncat  (char *, const char *, size_t)  ;
extern int strncmp  (const char *, const char *, size_t)  ;
extern char *strncpy  (char *, const char *, size_t)  ;
extern char *strpbrk  (const char *, const char *)  ;
extern char *strrchr  (const char *, int)  ;
extern size_t strspn  (const char *, const char *)  ;
extern char *strstr  (const char *, const char *)  ;
extern char *strtok  (char *, const char *)  ;
/* strxfrm() not supported */

extern void *memchr  (const void *, int, size_t)  ;
extern int   memcmp  (const void *, const void *, size_t)  ;
extern void *memcpy  (void *, const void *, size_t)  ;
extern void *memmove  (void *, const void *, size_t)  ;
extern void *memset  (void *, int, size_t)  ;

#endif
