/* cx_libfuncs.h - C interpreter builtin functions */

/*  Copyright 1992 Mark Russell, University of Kent at Canterbury.
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


/* @(#)cx_libfuncs.h	1.6 09 Apr 1995 (UKC) */

#undef F
#undef F2

#ifdef OS_SUNOS_4
#undef clearerr
#undef feof
#undef ferror
#undef isalnum
#undef isalpha
#undef iscntrl
#undef isdigit
#undef isgraph
#undef islower
#undef isprint
#undef ispunct
#undef isspace
#undef isupper
#undef isxdigit
#endif /* OS_SUNOS_4 */

/*
 * One of WANT_ADDRS, WANT_DECLS or WANT_NAMES will be defined before
 * this file is included.
 */

#ifdef WANT_DECLS

#include <stdlib.h>
#include <assert.h>

#if WANT_MATH
#include <math.h>
#endif
#if STDC_HEADERS
#include <stdlib.h>
#endif
#if HAVE_GRP_H
#include <grp.h>
#endif
#if HAVE_PWD_H
#include <pwd.h>
#endif
#if HAVE_NETDB_H
#include <netdb.h>
#endif
#include <signal.h>
#if HAVE_SYS_PTRACE_H
#include <sys/ptrace.h>
#endif
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif


#if HAVE_GETDTABLESIZE && !HAVE_GETDTABLESIZE_DECL
int	getdtablesize PROTO((void));
#endif
#if HAVE_GETRUSAGE && !HAVE_GETRUSAGE_DECL
int	getrusage PROTO((int, struct rusage *));
#endif
#if HAVE_KILLPG && !HAVE_KILLPG_DECL
int	killpg PROTO((pid_t pgrp, int sig));
#endif
#if HAVE_SRANDOM && !HAVE_SRANDOM_DECL
void	srandom PROTO((unsigned long));
long	random PROTO((void));
#endif
#if HAVE_SETLINEBUF && !HAVE_SETLINEBUF_DECL
int	setlinebuf PROTO((char *file, struct timeval *tvp));
#endif
#if HAVE_UTIMES && !HAVE_UTIMES_DECL
int	utimes PROTO((char *file, struct timeval *tvp));
#endif
#if HAVE_WAIT3 && !HAVE_WAIT3_DECL
pid_t	wait3 PROTO((int *status, int options, struct rusage *rusage));
#endif


#if OS_SUNOS_4
/* SunOS 4.x cc/acc/gcc don't have these in header files. */
#if !defined(__STDC__)
int   vprintf(), vfprintf(), vsprintf();
#endif
int   adjtime(), gettimeofday(), settimeofday();
int   getpgrp(), setpgrp(), getgroups(), setgroups(), setgrent();
double strtod();
long  strtol(), clock();
int   lstat(), ptrace(), putenv(), readlink(), system(), select(), symlink();
int   send(), recv(), sendto(), recvfrom(), sendmsg(), recvmsg();
int   accept(), bind(), connect(), listen(), shutdown();
int   socket(), socketpair(), getsockopt(), setsockopt();
int   getpeername(), getsockname();
#if defined(__STDC__)
int   clearerr(), feof(), ferror();
#endif
int   fflush(), remove(), rename();
int   isalnum(), isalpha(), iscntrl(), isdigit(), isgraph(), islower(),
      isprint(), ispunct(), isspace(), isupper(), isxdigit();
#if !defined(__STDC__)
/* Actually only needed by /usr/ucb/cc and not /usr/5bin/cc. */ 
int   fprintf(), printf(), fscanf(), scanf(), sscanf();
int   fseek(), fgetc(), fputc(), fputs(), puts(), ungetc();
int   toupper(), tolower();
size_t fread(), fwrite(), fseek();
void  rewind(), perror(), setbuf();
int   strftime();
time_t mktime(), time();
#endif
#if HAVE_SETVBUF
int   setvbuf();
#endif
#if HAVE_GETC_HELPER
int   GETC_HELPER();
#endif
#if HAVE_PUTC_HELPER
int   PUTC_HELPER();
#endif
#endif /* OS_SUNOS_4 */

#if OS_SUNOS
#if HAVE_DOPRNT
int _doprnt(void);
#endif
#endif /* OS_SUNOS */

#endif /* WANT_DECLS */


#ifdef WANT_ADDRS
#	define F(func)		(libfunc_addr_t)func,
#	define F2(func, name)	(libfunc_addr_t)func,
#endif

#ifdef WANT_DECLS
#	define F(func)
#	define F2(func, name)
#endif

#ifdef WANT_NAMES
#	ifdef __STDC__
#		define F(func)		#func,
#		define F2(func, name)	#name,
#	else
#		define F(func)		"func",
#		define F2(func, name)	"name",
#	endif
#endif

	/* Globals often now implemented as routines. */
	/* The _xxx_ builtins support the set of standard headers supplied
	   in the UPS distribution. */
	F2(builtin_stdin, _stdin_)
	F2(builtin_stdout, _stdout_)
	F2(builtin_stderr, _stderr_)
	F2(builtin_errno, _errno_)
#if WANT_CLIB
#if HAVE___ERRNO_LOCATION && HAVE_ERRNO_LOCATION_DECL
	F(__errno_location)
#endif
#if HAVE___ERROR && HAVE___ERRNO_DECL
	F(__error)
#endif

	/* B1.1 File operations */
	F2(builtin_fopen, fopen)
	F2(builtin_freopen, freopen)
	F(fflush)
	F2(builtin_fclose, fclose)
#if HAVE_REMOVE
	/** FIX: should this be a built-in ? file handles opened by
	 ** by built-ins are closed when the interpreter finishes
	 ** executing a program - but in this case, the file created
	 ** must also be automatically removed when the file is closed.
	 **/
	F(remove)
#endif
	F(rename)
#if HAVE_TMPFILE
	F(tmpfile)
#endif
#if HAVE_SETVBUF
	F(setvbuf)
#endif
	F(setbuf)

	/* B1.2 Formatted output */
	F(fprintf)
	F(printf)
	F(sprintf)
#if HAVE_VPRINTF
	F(vprintf)
	F(vfprintf)
	F(vsprintf)
#endif
#if HAVE_DOPRNT
	F(_doprnt)
#endif

	/* B1.3 Formatted input */
	F(fscanf)
	F(scanf)
	F(sscanf)

	/* B1.4 Character input and output functions */
	F(fgetc)
	F(fgets)
	F(fputc)
	F(fputs)
#if HAVE_GETS && WANT_GETS
	F(gets)
#endif
	F(puts)
	F(ungetc)

	/* Helper routines used by stdio getc/putc macro implementations */
#if HAVE_GETC_HELPER
# ifdef WANT_NAMES
	GETC_H_NAME,
# else
	F(GETC_HELPER)
# endif
#endif
#if HAVE_PUTC_HELPER
# ifdef WANT_NAMES
	PUTC_H_NAME,
# else
	F(PUTC_HELPER)
# endif
#endif
#if HAVE_PEEKC_HELPER
# ifdef WANT_NAMES
	PEEKC_H_NAME,
# else
	F(PEEKC_HELPER)
# endif
#endif

	/* B1.5 Direct input and output functions */
	F(fread)
	F(fwrite)

	/* B1.6 File positioning functions */
	F(fseek)
	F(ftell)
	F(rewind)
#if HAVE_FGETPOS
	F(fgetpos)
#endif
#if HAVE_FSETPOS
	F(fsetpos)
#endif

	/* B1.7 Error functions */
#if HAVE_FEOF_FUNCTIONS
	F(clearerr)
	F(feof)
	F(ferror)
#endif
	F(perror)

	/* B2 Character class tests */
	/* Often were macros, can still be but should also be
	   available as functions with ANSI C. */
#if HAVE_CTYPE_FUNCTIONS
	F(isalnum)
	F(isalpha)
	F(iscntrl)
	F(isdigit)
	F(isgraph)
	F(islower)
	F(isprint)
	F(ispunct)
	F(isspace)
	F(isupper)
	F(isxdigit)
#endif
	/* toupper/tolower are usually functions */
	F(toupper)
	F(tolower)

	/* B3 - String and memory functions */
	F(strcpy)
	F(strncpy)
	F(strcat)
	F(strncat)
	F(strcmp)
	F(strncmp)
	F(strchr)
	F(strrchr)
#if HAVE_STRING_H
	F(strspn)
	F(strcspn)
	F(strpbrk)
	F(strstr)
#endif
	F(strlen)
#if HAVE_STRERROR
	F(strerror)
#endif
	F(strtok)
#if HAVE_MEMCPY
	F(memcpy)
#endif
#if HAVE_MEMMOVE
	F(memmove)
#endif
#if HAVE_MEMCMP
	F(memcmp)
#endif
	F(memchr)
	F(memset)

	/* B4 Math */
	F(abs)
#if WANT_MATH && HAVE_LIBM
	F(sin)
	F(cos)
	F(tan)
	F(asin)
	F(acos)
	F(atan)
	F(atan2)
	F(sinh)
	F(cosh)
	F(tanh)
	F(exp)
	F(log)
	F(log10)
	F(pow)
	F(sqrt)
	F(ceil)
	F(floor)
	F(fabs)
	F(ldexp)
	F(frexp)
	F(modf)
	F(fmod)
#endif

	/* B5 Utility functions */
	F(atof)
	F(atoi)
	F(atol)
	F(strtod)
	F(strtol)
#if HAVE_STRTOUL
	F(strtoul)
#endif
#if HAVE_SRANDOM
	F(srandom)
	F(random)
#endif
	F(rand)
	F(srand)
	F2(builtin_calloc, calloc)
	F2(builtin_malloc, malloc)
	F2(builtin_realloc, realloc)
	F2(builtin_free, free)
	F(abort)
	F2(builtin_exit, exit)
#if HAVE_ATEXIT
	F(atexit)
#endif
	F(system)
	F(getenv)
#if HAVE_BSEARCH
	F2(builtin_bsearch, bsearch)
#endif
	F2(builtin_qsort, qsort)
#if HAVE_LABS
	F(labs)
#endif
#ifdef STRUCTRET
	/** no support for external routines that return structures **/
	F(div)
	F(ldiv)
#endif

	/* B6 Diagnostics */
	/** assert is a macro */
#if HAVE___ASSERT
	F(__assert)
#endif
#if HAVE___ASSERT_FAIL
	F(__assert_fail)
#endif

	/* B7 Variable Argument Lists */
	/** implemented as macros */

	/* B8 Non-local jumps */
	/** setjmp and longjmp implemented elsewhere */

	/* B9 Signals */
	F2(builtin_signal, signal)

	/* B10 Date and time functions */
#if HAVE_CLOCK
	F(clock)
#endif
	F(time)
	F(ctime)
	F(mktime)
	F(asctime)
	F(localtime)
#if HAVE_DIFFTIME
	F(difftime)
#endif
#if HAVE_ADJTIME
	F(adjtime)
#endif
	F(gmtime)
	F(localtime)
	F(strftime)
#if HAVE_GETTIMEOFDAY
	F(gettimeofday)
	F(settimeofday)
#endif

	/******** K&RII C functions end here *********/

	/* Directory routines */
	F(opendir)
	F(readdir)
	F(closedir)
#if HAVE_SEEKDIR
	F(seekdir)
	F(telldir)
#endif

#if HAVE_POPEN
	F2(builtin_pclose, pclose)
	F2(builtin_popen, popen)
#endif

	F(alarm)
	F(sleep)

	/* Old (non-ANSI) library routines */
#if HAVE_SETLINEBUF
	F(setlinebuf)
#endif
#if HAVE_UTIMES
	F(utimes)
#endif
#if HAVE_MKSTEMP
	F(mkstemp)
#endif
#if HAVE_GETWD
     	F(getwd)
#endif
#if HAVE_GETCWD
	F(getcwd)
#endif
	F(fdopen)

	/* Authorisation system calls */
	F(getuid)
	F(geteuid)
	F(getgid)
	F(getegid)
	F(getgroups)
#if HAVE_SETGROUPS
	F(setgroups)
#endif
#if HAVE_SETGRENT
	F(setgrent)
#endif
#if HAVE_GETGRGID
	F(getgrgid)
	F(setreuid)
	F(setregid)
#endif
#if HAVE_GETPWNAM
	F(setpwent)
	F(getpwent)
	F(getpwnam)
	F(getpwuid)
#endif

	/* Pathname and file descriptor type system calls */
	F2(builtin_read, read)
	F2(builtin_write, write)
	F2(builtin_pipe, pipe)
	F2(builtin_lseek, lseek)
	F2(builtin_fcntl, fcntl)
	F2(builtin_close, close)
	F2(builtin_dup, dup)
	F2(builtin_dup2, dup2)
#if HAVE_GETDTABLESIZE
	F(getdtablesize)
#endif
	F2(builtin_fchown, fchown)
	F2(builtin_fchmod, fchmod)
#if HAVE_FLOCK
	F2(builtin_flock, flock)
#endif
	F2(builtin_fsync, fsync)
	F2(builtin_ftruncate, ftruncate)
	F2(builtin_ioctl, ioctl)
#if HAVE_SELECT
	F(select)
#endif
	F2(builtin_open, open)
	F2(builtin_creat, creat)
	F(link)
#if HAVE_SYMLINK
	F(symlink)
	F(readlink)
	F(lstat)
#endif
#ifdef NOTYET
	F(chroot)
	F(truncate)
	F(sync)
#endif
	F(unlink)
	F(chown)
	F(chmod)
	F(mkdir)
	F(rmdir)
	F(access)
	F(chdir)
	F(umask)
	F(stat)
	F2(builtin_fstat, fstat)

	/* Process handling system calls */
	F(getpid)
	F(getpgrp)
#if HAVE_SETPGRP
	F(setpgrp)
#endif
	F(fork)
	F2(fork, vfork)
#if HAVE_WAIT3
	F(wait3)
#endif
#if HAVE_KILLPG
	F(killpg)
#endif
#if HAVE_GETRUSAGE
	F(getrusage)
#endif
	F(execve)
	F(wait)
	F(kill)
	F2(builtin__exit, _exit)
	F(execl)
	F(execv)
	F(execle)
	F(execlp)
	F(execvp)

	/* Socket system calls */
#if HAVE_SOCKET
	F(send)
	F(sendto)
	F(sendmsg)
	F(recv)
	F(recvfrom)
	F(recvmsg)
	F(socket)
	F(socketpair)
	F(shutdown)
	F(getsockopt)
	F(setsockopt)
	F(getsockname)
	F(bind)
	F(connect)
	F(listen)
	F(accept)
	F(getpeername)
#endif

	/* Miscellaneous library functions */
	F(isatty)
#if HAVE_PTRACE
	F(ptrace)
#endif
#if HAVE_PUTENV
	F(putenv)
#endif

	/* BSD and Posix signals */
#if HAVE_SIGADDSET && !defined(sigaddset)
	F(sigaddset)
	F(sigdelset)
#endif
#if HAVE_SIGEMPTYSET && !defined(sigemptyset)
	F(sigemptyset)
	F(sigfillset)
#endif
	F(sigprocmask)

	/* Linux weirdness */
#if HAVE__XUSTAT
	F(_xustat)
#endif

	/* Varargs support (some can do without this). */
#if HAVE___BUILTIN_NEXT_ARG
	/*F(__builtin_next_arg)*/ /* FIX: */
#endif

	/* netdb.h stuff */
#if HAVE_GETHOSTBYNAME
	F(gethostbyname)
	F(gethostbyaddr)
	F(getservbyname)
	F(getservbyport)
#endif

#if HAVE_X_WINDOWS
	F(XUngrabServer)
	F(XUnmapWindow)
	F(XGrabServer)
	F(XMoveResizeWindow)
	F(XDestroyWindow)
	F(XrmGetResource)
	F(XGetDefault)
	F(XrmGetFileDatabase)
	F(XStoreColor)
	F(XAllocColor)
	F2(builtin_XCloseDisplay, XCloseDisplay)
	F(XSynchronize)
	F2(builtin_XOpenDisplay, XOpenDisplay)
	F(XCreateGC)
	F(XGetWindowAttributes)
	F(XSetStandardProperties)
	F(XCreateSimpleWindow)
	F(XSetClassHint)
	F(XSetWMHints)
	F(XParseGeometry)
	F(XMapWindow)
	F(XChangeWindowAttributes)
	F(XBell)
	F(XStoreBytes)
	F(XSetSelectionOwner)
	F(XGetWindowProperty)
	F(XFetchBytes)
	F(XPending)
	F(XConvertSelection)
	F(XSendEvent)
	F(XChangeProperty)
	F(XGetImage)
	F(XPutImage)
	F(XSetFillStyle)
	F(XSetTile)
	F(XCreatePixmap)
	F(XCopyArea)
	F(XSetGraphicsExposures)
	F(XDrawLine)
	F(XFillRectangle)
	F(XLookupKeysym)
	F(XLookupString)
	F(XPeekEvent)
	F(XRaiseWindow)
	F(XNextEvent)
	F2(builtin_XIfEvent, XIfEvent)
	F(XCheckWindowEvent)
	F(XWarpPointer)
	F(XSync)
	F(XFlush)
	F(XSelectInput)
	F(XLoadQueryFont)
	F(XListFonts)
	F(XFreeFontNames)
	F(XUnloadFont)
	F(XDrawImageString)
	F(XDrawString)
	F(XSetFont)
	F(XSetState)
	F(XCreateBitmapFromData)
	F(XFreePixmap)
	F(XCreatePixmapCursor)
	F(XFreeCursor)
	F(XDefineCursor)
	F(XFreeColors)
	F(XAllocColorCells)
	F(XQueryColor)
	F(XParseColor)
	F(XStoreColors)
	F(XSetBackground)
	F(XSetForeground)
	F(XDisplayName)
	F(XFree)
	F(XSetWMProperties)
	F(XAllocClassHint)
	F(XAllocWMHints)
	F(XStringListToTextProperty)
	F(XAllocSizeHints)
	F(XClearWindow)
	F(XDrawRectangle)
	F(XTextWidth)
	F(XCheckTypedEvent)
	F(XClearArea)
#endif
#endif /* WANT_CLIB */

#undef F
#undef F2

