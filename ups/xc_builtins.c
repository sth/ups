/* xc_builtins.c - builtin function stubs for the standalone C interpreter */

/*  Copyright 1993 Mark Russell, University of Kent at Canterbury.
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


/* @(#)xc_builtins.c	1.11 04 Jun 1995 (UKC) */
char ups_xc_builtins_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "xc_opcodes.h"
#include "xc_machine.h"
#include "xc_builtins.h"

#if HAVE_X_WINDOWS
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
extern int _Xdebug;
#endif
#if STDC_HEADERS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

/*
 * This should be kept in step with that in cx_libfuncs.h.
 */

typedef int (*qsort_func_t)PROTO((const void *, const void *));
static void builtin_qsort PROTO((char *base, size_t nel, size_t width,
						textaddr_t cmp_func));
static voidptr builtin_bsearch PROTO((const void *key, const void *base,
			size_t nmemb, size_t size, textaddr_t cmp_func));
#if HAVE_X_WINDOWS
static int builtin_XIfEvent PROTO((Display *display, XEvent *xev,
					textaddr_t wanted_func, char *arg));
#endif
static textaddr_t builtin_signal PROTO((int sig, textaddr_t func));

static int builtin__exit PROTO((int status));
static int builtin_exit PROTO((int status));

static int builtin_creat PROTO((const char *path, int mode));
static int builtin_open PROTO((const char *path, int flags, int mode));
static int builtin_close PROTO((int fd));

static int builtin_read PROTO((int fd, void *buf, size_t nbytes));
static int builtin_write PROTO((int fd, const void *buf, size_t nbytes));
static int builtin_pipe PROTO((int *fds));
static int builtin_lseek PROTO((int fd, off_t offset, int whence));
static int builtin_fcntl PROTO((int fd, int cmd, char *arg));
static int builtin_dup2 PROTO((int fd, int newfd));
static int builtin_dup PROTO((int fd));
static int builtin_fsync PROTO((int fd));
static int builtin_fchown PROTO((int fd, int uid, int gid));
static int builtin_fchmod PROTO((int fd, int mode));
#if HAVE_FLOCK
static int builtin_flock PROTO((int fd, int op));
#endif
static int builtin_ftruncate PROTO((int fd, long length));
static int builtin_ioctl PROTO((int fd, unsigned cmd, caddr_t arg));
static int builtin_fstat PROTO((int fd, struct stat *stbuf));
#if HAVE_X_WINDOWS
static Display *builtin_XOpenDisplay PROTO((const char *display_name));
static int builtin_XCloseDisplay PROTO(( Display *dpy));
#endif
static FILE *builtin_fopen PROTO((const char *path, const char *mode));
static int builtin_fclose PROTO(( FILE *fp));
static FILE *builtin_popen PROTO((const char *cmd, const char *mode));
static int builtin_pclose PROTO(( FILE *fp));
static FILE *builtin_freopen PROTO((const char *path, const char *mode, FILE *fp));
#if HAVE___BUILTIN_NEXT_ARG
#endif

static voidptr builtin_malloc PROTO((size_t size));
static voidptr builtin_calloc PROTO((size_t count, size_t size));
static voidptr builtin_realloc PROTO((voidptr p, size_t size));
static void builtin_free PROTO((voidptr p));

static bool fd_ok PROTO((int fd));

static FILE *builtin_stdin PROTO((void));
static FILE *builtin_stderr PROTO((void));
static FILE *builtin_stdout PROTO((void));
static int *builtin_errno PROTO((void));

extern int optind;
extern char *optarg;

static char **Builtin_environ = NULL;
static char **Builtin_sys_errlist = NULL;
static char **Builtin_ctype = NULL;

/* ---------------------------------------------------------------------------
 */

#define WANT_DECLS
#include "cx_libfuncs.h"
#include "cx_libvars.h"
#undef  WANT_DECLS

static libfunc_addr_t Libfunc_addrs[] = {
#define WANT_ADDRS
#include "cx_libfuncs.h"
#undef  WANT_ADDRS
};

static char *Libvar_addrs[] = {
#define WANT_ADDRS
#include "cx_libvars.h"
#undef  WANT_ADDRS
};

#define N_LIBFUNCS	(sizeof Libfunc_addrs / sizeof *Libfunc_addrs)
#define N_LIBVARS	(sizeof Libvar_addrs / sizeof *Libvar_addrs)

/* ---------------------------------------------------------------------------
 */

static int
builtin__exit(status)
int status;
{
	ci_cause_exit((machine_t *)NULL);
	return status;
}

static int
builtin_exit(status)
int status;
{
	ci_cause_exit((machine_t *)NULL);
	return status;
}

static textaddr_t
builtin_signal(sig, func)
int sig;
textaddr_t func;
{
	return ci_set_sig_handler(sig, func);
}

static void
builtin_qsort(base, nel, width, cmp_func)
char *base;
size_t nel, width;
textaddr_t cmp_func;
{

	qsort(base, nel, width, (qsort_func_t)ci_make_callback(cmp_func, 2));
}

#if HAVE_BSEARCH
static voidptr
builtin_bsearch(key, base, nmemb, size, cmp_func)
const void *key;
const void *base;
size_t nmemb, size;
textaddr_t cmp_func;
{
	return bsearch(key, base, nmemb, size,
		(qsort_func_t)ci_make_callback(cmp_func, 2));
}
#endif

#if HAVE_X_WINDOWS
static int
builtin_XIfEvent(display, xev, wanted_func, arg)
Display *display;
XEvent *xev;
textaddr_t wanted_func;
char *arg;
{
	typedef int (*xie_func_t)PROTO((Display *, XEvent *, char *));

	return XIfEvent(display, xev,
			(xie_func_t)ci_make_callback(wanted_func, 3), arg);
}
#endif

static bool
fd_ok(fd)
int fd;
{
	if (ci_fd_is_registered((machine_t *)NULL, fd))
		return TRUE;

	errno = EBADF;
	return FALSE;
}

static int
builtin_creat(path, mode)
const char *path;
int mode;
{
	return builtin_open(path, O_WRONLY | O_CREAT | O_TRUNC, mode);
}

static int
builtin_open(path, flags, mode)
const char *path;
int flags, mode;
{
	int fd;

	if ((fd = open(path, flags, mode)) != -1)
		ci_register_fd((machine_t *)NULL, fd);

	return fd;
}

static int
builtin_close(fd)
int fd;
{
	int res;

	if (!fd_ok(fd))
		return -1;

	if ((res = close(fd)) == 0)
		ci_unregister_fd((machine_t *)NULL, fd);

	return res;
}

static int
builtin_read(fd, buf, nbytes)
int fd;
void *buf;
size_t nbytes;
{
	return fd_ok(fd) ? read(fd, buf, nbytes) : -1;
}

static int
builtin_ioctl(fd, cmd, arg)
int fd;
unsigned cmd;
char *arg;
{
	return fd_ok(fd) ? ioctl(fd, (int)cmd, arg) : -1;
}

#if HAVE_FLOCK
static int
builtin_flock(fd, op)
int fd, op;
{
	return fd_ok(fd) ? flock(fd, op) : -1;
}
#endif

static int
builtin_write(fd, buf, nbytes)
int fd;
constvoidptr buf;
size_t nbytes;
{
	return fd_ok(fd) ? write(fd, buf, nbytes) : -1;
}

static int
builtin_fchmod(fd, mode)
int fd, mode;
{
	return fd_ok(fd) ? fchmod(fd, (mode_t)mode) : -1;
}

static int
builtin_fchown(fd, uid, gid)
int fd, uid, gid;
{
	return fd_ok(fd) ? fchown(fd, uid, gid) : -1;
}

static int
builtin_fstat(fd, st)
int fd;
struct stat *st;
{
	return fd_ok(fd) ? fstat(fd, st) : -1;
}

static int
builtin_fsync(fd)
int fd;
{
	return fd_ok(fd) ? fsync(fd) : -1;
}

static int
builtin_ftruncate(fd, length)
int fd;
long length;
{
       return fd_ok(fd) ? ftruncate(fd, (off_t)length) : -1;
}

static int
builtin_fcntl(fd, cmd, arg)
int fd, cmd;
char *arg;
{
	int res;

	if (!fd_ok(fd))
		return -1;

	res = fcntl(fd, cmd, arg);

	if (cmd == F_DUPFD && res != -1)
		ci_register_fd((machine_t *)NULL, fd);

	return res;
}

static int
builtin_lseek(fd, offset, whence)
int fd;
off_t offset;
int whence;
{
	return fd_ok(fd) ? lseek(fd, offset, whence) : -1;
}

static int
builtin_pipe(fds)
int *fds;
{
	int res;

	if ((res = pipe(fds)) == 0) {
		ci_register_fd((machine_t *)NULL, fds[0]);
		ci_register_fd((machine_t *)NULL, fds[1]);
	}

	return res;
}

static int
builtin_dup(fd)
int fd;
{
	int res;

	if ((res = dup(fd)) != -1)
		ci_register_fd((machine_t *)NULL, res);

	return res;
}

static int
builtin_dup2(fd, newfd)
int fd, newfd;
{
	int res;

	if ((res = dup2(fd, newfd)) != -1) {
		if (!ci_fd_is_registered((machine_t *)NULL, newfd))
			ci_register_fd((machine_t *)NULL, newfd);
	}

	return res;
}

static FILE *
builtin_fopen(path, mode)
const char *path, *mode;
{
	FILE *fp;

	fp = fopen(path, mode);

	if (fp != NULL)
		ci_register_fd((machine_t *)NULL, fileno(fp));

	return fp;
}

static int
builtin_fclose(fp)
FILE *fp;
{
	ci_unregister_fd((machine_t *)NULL, fileno(fp));
	return fclose(fp);
}

static FILE *
builtin_freopen(path, mode, fp)
const char *path, *mode;
FILE *fp;
{
	ci_unregister_fd((machine_t *)NULL, fileno(fp));

	fp = freopen(path, mode, fp);

	if (fp != NULL)
		ci_register_fd((machine_t *)NULL, fileno(fp));

	return fp;
}

static FILE *
builtin_popen(cmd, mode)
const char *cmd, *mode;
{
	FILE *fp;

	fp = (FILE *)popen(cmd, mode);

	if (fp != NULL)
		ci_register_fd((machine_t *)NULL, fileno(fp));

	return fp;
}

static int
builtin_pclose(fp)
FILE *fp;
{
	ci_unregister_fd((machine_t *)NULL, fileno(fp));
	return pclose(fp);
}

#if HAVE_X_WINDOWS
static Display *
builtin_XOpenDisplay(display_name)
const char *display_name;
{
	Display *dpy;

	dpy = XOpenDisplay(display_name);

	if (dpy != NULL)
		ci_register_fd((machine_t *)NULL, ConnectionNumber(dpy));

	return dpy;
}

static int
builtin_XCloseDisplay(dpy)
Display *dpy;
{
	ci_unregister_fd((machine_t *)NULL, ConnectionNumber(dpy));
	return XCloseDisplay(dpy);
}
#endif

static voidptr
builtin_calloc(count, size)
size_t count, size;
{
	size_t nbytes;
	voidptr p;

	nbytes = count * size;

	p = ci_mem_malloc((machine_t *)NULL, nbytes);

	if (p != NULL)
		memset(p, '\0', nbytes);

	return p;
}

static voidptr
builtin_malloc(size)
size_t size;
{
	return ci_mem_malloc((machine_t *)NULL, size);
}

static voidptr
builtin_realloc(p, size)
voidptr p;
size_t size;
{
	return ci_mem_realloc((machine_t *)NULL, p, size);
}

static void
builtin_free(p)
voidptr p;
{
	ci_mem_free((machine_t *)NULL, p);
}

/* Dibyendu - hack to get standard streams. */
static FILE *
builtin_stdin()
{
	return stdin;
}

static FILE *
builtin_stdout()
{
	return stdout;
}

static FILE *
builtin_stderr()
{
	return stderr;
}

static int *
builtin_errno()
{
#if HAVE___ERRNO_LOCATION && HAVE_ERRNO_LOCATION_DECL
	return __errno_location();
#else
	return &errno;
#endif
}

/*  BUG: this sucks.
 */
void
cx_set_environ(envp)
char **envp;
{
	Builtin_environ = envp;
#if HAVE_GLOBAL_CTYPE
	Builtin_ctype = (char **)(GLOBAL_CTYPE);
#endif
#if HAVE_SYS_ERRLIST && HAVE_SYS_ERRLIST_DECL
	Builtin_sys_errlist = (char **)sys_errlist;
#endif
}

libfunc_addr_t *
cx_get_libfuncs(count)
size_t count;
{
	if (count != N_LIBFUNCS)
		panic("libfuncs count botch");

	return Libfunc_addrs;
}

char **
cx_get_libvars(count)
size_t count;
{
	if (count != N_LIBVARS)
		panic("libvar count botch");

	return Libvar_addrs;
}
