/* ci_libvars.h - C interpreter builtin variables */

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


/* @(#)cx_libvars.h	1.4 21 Apr 1994 (UKC) */

#undef V
#undef V2
#undef VA

/*
 * One of WANT_ADDRS, WANT_DECLS or WANT_NAMES will be defined before
 * this file is included.
 */

#ifdef WANT_DECLS

#if HAVE_SYS_ERRLIST && !HAVE_SYS_ERRLIST_DECL
extern char *	sys_errlist[];
#endif
#if HAVE_SYS_NERR && !HAVE_SYS_NERR_DECL
extern int	sys_nerr;
#endif

#endif

#ifdef WANT_ADDRS
#	define V(var)		(char *)&var,
#	define V2(var, name)	(char *)&var,
#	define VA(var, name)	(char *)var,
#endif

#ifdef WANT_DECLS
#	define V(var)
#	define V2(var, name)
#	define VA(var, name)
#endif

#ifdef WANT_NAMES
#	ifdef __STDC__
#		define V(var)		#var,
#	else
#		define V(var)		"var",
#	endif
#	define V2(var, name)	name,
#	define VA(var, name)	name,
#endif

#if !defined errno
	V(errno)
#endif

#if HAVE_SYS_NERR
	V(sys_nerr)
#endif
#if HAVE_SYS_ERRLIST
#if HAVE_SYS_ERRLIST_DECL
	V2(Builtin_sys_errlist, "sys_errlist")
#else
	VA(sys_errlist, "sys_errlist")
#endif
#endif
	V(optind)
	V(optarg)
	V2(Builtin_environ, "environ")
#if HAVE_GLOBAL_IOB
	VA(GLOBAL_IOB, NAME_IOB)
	VA(GLOBAL_IOB, "_iob")
#endif
#if HAVE_GLOBAL_CTYPE
	V2(Builtin_ctype, NAME_CTYPE)
	V2(Builtin_ctype, "_ctype")
/*
	VA(GLOBAL_CTYPE, NAME_CTYPE)
	VA(GLOBAL_CTYPE, "_ctype")
*/
#endif
/* GNU implementation of standard I/O. */
#if defined(__GLIBC__)  && (__GLIBC__ >= 2)
#if defined(__GLIBC_MINOR__) && (__GLIBC_MINOR__ >= 1)
	/* glibc 2.1 */
	V(_IO_2_1_stdin_)
	V(_IO_2_1_stdout_)
	V(_IO_2_1_stderr_)
#else
	/* glibc 2.0 */
	V(_IO_stdin_)
	V(_IO_stdout_)
	V(_IO_stderr_)
#endif
#elif defined(_IO_stdin)
	/* glibc 1.x (and other early Linux ?) */
	V(_IO_stdin_)
	V(_IO_stdout_)
	V(_IO_stderr_)
#endif
#ifdef OS_ULTRIX
	V(_pctype)
#endif
#if HAVE_X_WINDOWS
	V(_Xdebug)
#endif

