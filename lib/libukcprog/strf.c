/* strf.c -- formatted strings, storage allocated from malloc */

/*  Copyright 1992  Godfrey Paul, University of Kent at Canterbury.
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


/* @(#)strf.c	1.7 30/5/93 UKC */
char ukcprog_strf_rcsid[] = "$Id$";

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include "ukcprog.h"


#ifdef __STDC__
char *
strf(const char *fmt, ...)
{

#else /* !__STDC__ */
char *
strf(va_alist)
va_dcl
{
	char *fmt;
#endif /* !__STDC__ */
	va_list args;
	char buffer[100];
	char *s;

#ifdef __STDC__
	va_start(args, fmt);
#else
	va_start(args);
	fmt = va_arg(args, char *);
#endif

	s = formf(buffer, sizeof(buffer), fmt, args);

	va_end(args);

	if (s == buffer)	/* not obtained from malloc */
		s = strsave(s);

	return s;
}
