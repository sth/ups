/* ukcprog.h -- Declarations for UKC programmers' library routines. */

/*  Copyright 1993  Godfrey Paul, University of Kent at Canterbury.
 *  All Rights Reserved.
 *
 *  This file is part of UPS.
 *
 *  UPS is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  UPS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with UPS; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* @(#)ukcprog.h	1.34 16 Sep 1994 UKC */

#ifndef UKCPROG_H_DEFINED
#define UKCPROG_H_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __STDC__
#ifndef PROTO
#define PROTO(a)	a
#endif	/* !PROTO */

typedef void *voidptr;
typedef const void *constvoidptr;

#include <stddef.h>	/* needed for size_t */

#else /* !__STDC__ */

#include <sys/types.h>	/* size_t for old C */

#ifndef PROTO
#define PROTO(a)	()
#endif	/* !PROTO */

/*  Patch up for things that are missing without ANSI C. */
#ifndef const
#define const
#endif
#ifndef volatile
#define volatile
#endif
#ifndef signed
#define signed
#endif

typedef char *voidptr;
typedef char *constvoidptr;

#endif /* !__STDC__ */

/*  Defining boolean type.  This might cause problems for some ...  */
#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	1
#endif
#ifndef bool
#define bool int
#endif

/*  Macro to get control characters (works for ASCII only).  */
#define CONTROL(c)	((c) & 0x1f)

/*  Define NULL - this avoids having the whole of stdio.h */
#ifndef NULL
#define NULL 0
#endif

/*  Macro to concatenate two or three names.  */
#ifdef CAT
#undef CAT
#endif /* CAT */

#if defined(__STDC__) && !defined(VMS)
#define CAT(a,b)	a ## b
#define CAT3(a,b,c)	a ## b ## c
#else
#define _IDENT(a) a
#define CAT(a,b) _IDENT(a)b
#define CAT3(a,b,c) CAT(a,b)c
#endif

/* from panic.c */
typedef void (*panic_handler_t) PROTO((const char *message));

panic_handler_t install_panic_handler PROTO((panic_handler_t handler));
void panic PROTO((const char *message));


/* from e_malloc.c */
voidptr e_malloc PROTO((size_t size));


/* from e_realloc.c */
voidptr e_realloc PROTO((voidptr old, size_t size));


/* from strsave.c */
char *strsave PROTO((const char *s));


/* from config.c */
char *config_trim_line PROTO((char *line));


#ifdef UKC_WANT_FORMF_CHECKS
#define FORMF_ARGS(fpos, argpos)   __attribute__((format(printf, fpos, argpos)))
#else

#ifdef VMS
/*  The VMS C compiler with /STANDARD=PORTABLE complains about unused
 *  arguments in macros.  This grossness is to shut it up.
 */
#define FORMF_ARGS(fpos, apos) ; extern int CAT(__ukc,fpos), CAT(__ukc,apos)
#else
#define FORMF_ARGS(fpos, argpos)	/* nothing */
#endif

#endif

/* from formf.c */
#ifdef __STDC__
#include <stdarg.h>	/* nasty, but needed for prototype */
#endif

char *formf PROTO((char *buffer_space, int buffer_size,
			const char *format, va_list args));


/* from errf.c */
typedef void (*errf_ofunc_t) PROTO((const char *string));

errf_ofunc_t errf_set_ofunc PROTO((errf_ofunc_t func));
errf_ofunc_t errf_set_ofunc1 PROTO((errf_ofunc_t func));
const char *errf_set_prefix PROTO((const char *prefix));
const char *errf_get_prefix PROTO((void));
void errf_set_progname PROTO((const char *progname));
const char *errf_get_progname PROTO((void));
void errf_usage PROTO((const char *usage));

void errf PROTO((const char *fmt, ...)) FORMF_ARGS(1, 2);
char *strf PROTO((const char *fmt, ...)) FORMF_ARGS(1, 2);
void strnf PROTO((char *buf, size_t bufsize, const char *fmt, ...))
							FORMF_ARGS(3, 4);

/* from log.c */
#define LG_ALL	0	/* lowest priority */
#define LG_DEBUG 1	/* debugging messages */
#define LG_INFO	5	/* information messages */
#define LG_ERR	9	/* error messages */
#define LG_LOG	10	/* highest priority; messages from the logger */

int logf_set_ofile PROTO((const char *filename, const char *prefix));
void log_f PROTO((int level, const char *fmt, ...)) FORMF_ARGS(2, 3);
int logf_set_level PROTO((int new_level));
void logf_errf_ofunc PROTO((const char *str));

/* from fpgetline.c
 *
 * Only include this prototype if stdio.h has been #included already.
 * This is to mandating the inclusion of stdio.h unless fpgetline()
 * is required.
 */
#ifdef EOF
char *fpgetline PROTO((FILE *fp));
#endif


/* from alloc.c */

typedef struct alloc_pool_s alloc_pool_t;
typedef struct alloc_mark_s alloc_mark_t;

/*  Backwards compatibility.
 */
typedef alloc_pool_t *alloc_id_t;
typedef alloc_mark_t *alloc_mark_id_t;

alloc_pool_t *alloc_create_pool PROTO((void));
void alloc_free_pool PROTO((alloc_pool_t *ap));
void alloc_reset_pool PROTO((alloc_pool_t *ap));

/*  Various forms of allocation.  alloc() aligns like malloc,
 *  allocstr() doesn't.  alloc_strdup() is like strdup() but
 *  used allocstr() rather than malloc().  All of the preceding
 *  calls panic if they run out memory.  alloc_ck() and allocstr_ck()
 *  are like alloc() and allocstr() except that they return NULL
 *  rather than panicking if memory runs out.
 */
voidptr alloc PROTO((alloc_pool_t *ap, size_t nbytes));
char *allocstr PROTO((alloc_pool_t *ap, size_t nbytes));
voidptr alloc_ck PROTO((alloc_pool_t *ap, size_t nbytes));
char *allocstr_ck PROTO((alloc_pool_t *ap, size_t nbytes));
char *alloc_strdup PROTO((alloc_pool_t *ap, const char *s));

/*  Control - set and clear debug flags both globally and per-pool.
 *  If the debug flag is set new memory is initialised to garbage
 *  and set to (different) garbage when a pool is freed.
 */
bool alloc_set_default_debug_flag PROTO((bool val));
bool alloc_set_debug_flag PROTO((alloc_pool_t *ap, bool val));

/*  alloc_mark() returns an alloc_mark_id that represents the current
 *  state of the alloc pool.  alloc_release() releases any memory
 *  allocated since the alloc_mark() call.
 */
alloc_mark_t *alloc_mark PROTO((alloc_pool_t *ap));
void alloc_release PROTO((alloc_pool_t *ap, alloc_mark_t *am));


/* from ssplit.c */

char **ssplit PROTO((const char *line, const char *delimiters));

/* from ip.c */

#ifdef IPPROTO_TCP
int get_host_addr PROTO((const char *hostname, struct in_addr *p_addr));
int get_service_port PROTO((const char *servname, int *p_port));
#endif

/* from ebuf.c */

typedef struct ebuf_s ebuf_t;

void ebuf_reset PROTO((ebuf_t *eb));
ebuf_t *ebuf_create PROTO((bool errors_are_fatal));
ebuf_t *ebuf_start PROTO((ebuf_t *ebuf, bool errors_are_fatal));
int ebuf_add PROTO((ebuf_t *eb, constvoidptr buf, size_t count));
int ebuf_addstr PROTO((ebuf_t *eb, const char *str));
voidptr ebuf_get PROTO((ebuf_t *eb, int *p_len));
void ebuf_free PROTO((ebuf_t *eb));

/* from sccsdata.c */
const char *ukcprog_version PROTO((void));

#ifdef __cplusplus
}
#endif

#endif	/* !UKCPROG_H_DEFINED */
