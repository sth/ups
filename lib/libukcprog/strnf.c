/* strnf.c -- like sprintf, but with a buffer size argument */

/*  Copyright 1992  Mark Russell, University of Kent at Canterbury.
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


/* @(#)strnf.c	1.7 25/10/93 UKC */
char ukcprog_strnf_rcsid[] = "$Id$";

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __STDC__
#include <memory.h>
#endif

#include "ukcprog.h"


#ifdef __STDC__
void
strnf(char *buf, size_t bufsize, const char *fmt, ...)
{

#else /* !__STDC__ */
void
strnf(va_alist)
va_dcl
{
	char *buf;
	int bufsize;
	char *fmt;
#endif /* !__STDC__ */
	va_list args;
	char *s;

#ifdef __STDC__
	va_start(args, fmt);
#else
	va_start(args);
	buf = va_arg(args, char *);
	bufsize = va_arg(args, size_t);
	fmt = va_arg(args, char *);
#endif

	s = formf(buf, (int)bufsize, fmt, args);

	va_end(args);

	/*  If formf had to allocate a buffer then the supplied buf
	 *  was too small.  Copy what will fit and free the formf buffer.
	 */
	if (s != buf) {
		memcpy(buf, s, (size_t)(bufsize - 1));
		buf[bufsize - 1] = '\0';
		free(s);
	}
}
