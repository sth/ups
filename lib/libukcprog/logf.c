/* logf.c -- Log file and error reporting routines. */

/* Copyright 1992 Godfrey Paul, University of Kent of Canterbury.
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


/* @(#)logf.c	1.20 10/1/95 UKC */
char ukcprog_logf_rcsid[] = "$Id$";

#ifndef MSDOS
#include <sys/types.h>
#else
#include <sys/tk_types.h>
#endif

#ifdef VMS
#ifndef __CADDR_T
#define __CADDR_T
typedef char *caddr_t;
#endif /* __CADDR_T */
struct iovec {
	caddr_t iov_base;
	int iov_len;
};
#else
#include <sys/uio.h>
#endif
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#ifdef __STDC__
#ifndef VMS
#include <unistd.h>
#endif /* VMS */
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifndef MSDOS
#ifdef VMS

#include <ssdef.h>
#include <descrip.h>

extern volatile int noshare sys_nerr;
extern const char noshare *sys_errlist[];

#else /* !VMS */

#endif /* !VMS */
#endif /* !MSDOS */

#if !defined(__STDC__)
extern char *sys_errlist[];
extern int sys_nerr;
#define strerror(n) \
    (((n) >= 0 && (n) < sys_nerr) ? sys_errlist[n] : "unknown error")
#endif

#include "ukcprog.h"

static void write_logstr PROTO((const char *str, bool err));
static const char *get_errno_str PROTO((void));

static int Log_fd = 2;		/* just print to stderr by default */
static int Log_level = LG_INFO;	/* by default */
static char *Log_prefix;	/* identification tag */
static int Log_prefix_len = 0;

#if defined(VMS) || defined(MSDOS)
/*  writev()
 *  Writev() in terms of write().  Not sure about the behaviour of
 *  the real writev() in the error cases: this version can return -1
 *  after having successfully written some output.
 */
static int
writev(int fd, struct iovec *iov, int iovcnt)
{
	int i, n, total;

	total = 0;
	for (i = 0; i < iovcnt; ++i) {
		n = write(fd, iov[i].iov_base, (size_t)iov[i].iov_len);
		if (n < 0)
			return n;
		total += n;
	}
	return total;
}

#endif

#define DATE_LEN	(sizeof("Wed Jul 22 15:30:45 1992") - 1)

/*  This is to avoid using %m in logf_set_ofile.  We should really make
 *  this public and use it in formf().
 */
static const char *
get_errno_str()
{
	return (strerror (errno));
}

int
logf_set_ofile(filename, prefix)
const char *filename, *prefix;
{
	static bool first_time = TRUE;
	int fd;

	if (filename != NULL) {
		if ((fd = open(filename, O_RDWR|O_APPEND|O_CREAT, 0666)) < 0) {
			errf("Can't open logfile %s (%s)",
			     filename, get_errno_str());
			return -1;
		}

		if (!first_time)
			(void)close(Log_fd);
		else
			first_time = FALSE;

		Log_fd = fd;
	}

	if (prefix != NULL) {
		if (Log_prefix != NULL)
			free(Log_prefix);
		Log_prefix = strsave(prefix);
		Log_prefix_len = strlen(Log_prefix);
	}

	return 0;
}


static void
write_logstr(str, err)
const char *str;
bool err;
{
	time_t now;
	struct iovec iov[5];

	if (Log_fd < 0) {
		fprintf(stderr, "%s: %s\n", errf_get_progname(), str);
		return;
	}

	now = time((time_t *)NULL);

	iov[0].iov_base = (caddr_t)ctime(&now);
	iov[0].iov_len = sizeof("Fri Sep  9 15:10:00 ") - 1;

	iov[1].iov_base = (caddr_t)Log_prefix;
	iov[1].iov_len = Log_prefix_len;

	if (err)
		iov[2].iov_base = (caddr_t)" ERROR: ";
	else
		iov[2].iov_base = (caddr_t)" ";
	iov[2].iov_len = strlen(iov[2].iov_base);

	iov[3].iov_base = (caddr_t)str;
	iov[3].iov_len = strlen(str);

	iov[4].iov_base = (caddr_t)"\n";
	iov[4].iov_len = 1;

	writev(Log_fd, iov, (int)(sizeof(iov) / sizeof(*iov)));
}


#ifdef __STDC__
void
log_f(int level, const char *fmt, ...)
{

#else /* !__STDC__ */
void
log_f(va_alist)
va_dcl
{
	int level;
	char *fmt;
#endif /* !__STDC__ */
	va_list args;
	char buffer[100];
	char *s;

#ifdef __STDC__
	va_start(args, fmt);
#else
	va_start(args);
	level = va_arg(args, int);
	fmt = va_arg(args, char *);
#endif
	if (level < Log_level) {
		va_end(args);
		return;
	}

	s = formf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	write_logstr(s, FALSE);

	if (s != buffer)
		free(s);	/* allocated by formf() */
}

int
logf_set_level(new_level)
int new_level;
{
	int old;

	old = Log_level;
	Log_level = new_level;
	log_f(LG_LOG, "Log level changed from %d to %d", old, new_level);

	return old;
}


void
logf_errf_ofunc(str)
const char *str;
{
	write_logstr(str, TRUE);
}
