/* failmesg.c - produce error messages for system call failures */

/*  Copyright 1994 Mark Russell, University of Kent at Canterbury.
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


/* @(#)failmesg.c	1.2 9/4/95 (UKC) */
char utils_failmesg_rcsid[] = "$Id$";

#include "ifdefs.h"
#include <local/ukcprog.h>

#include "utils.h"

void
failmesg(mesg, what, path)
const char *mesg, *what, *path;
{
	const char *errno_str;

	errno_str = get_errno_str();
	
	if (what == NULL || *what == '\0')
		errf("%s %s: %s", mesg, path, errno_str);
	else
		errf("%s %s %s: %s", mesg, what, path, errno_str);
}

const char *
get_errno_str()
{
	return (strerror (errno));
}

/* Create an 'strerror()' routine to work round a problem in libukcprog.a
   with GCC 2.8.1 on SunOS 4.1.1 */
#if !HAVE_STRERROR && defined(__STDC__)
#undef strerror
char *strerror(n)
int n;
{
	return (((n) >= 0 && (n) < sys_nerr) ? sys_errlist[n] : "unknown error");
}
#endif

