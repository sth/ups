/* strtod.c - strtod, for machines that don't have it */

/*  Copyright 1991 Mark Russell, University of Kent at Canterbury.
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


/* @(#)strtod.c	1.11 15/9/92 (UKC) */
char utils_strtod_rcsid[] = "$Id$";

#include <ifdefs.h>

#if HAVE_STRTOD
/* Have it. */
#else

#include <ctype.h>
#include <stdlib.h>
#include <local/ukcprog.h>

double
strtod(start, endp)
const char *start;
char **endp;
{
	double val;
	const char *s;
	
	val = atof(start);

	/* Now skip the characters that atof looked at.
	 */
	for (s = start; *s != '\0' && isspace(*s); ++s)
		;
	if (*s == '+' || *s == '-')
		++s;

	while (isdigit(*s))
		++s;
	if (*s == '.')
		++s;
	while (isdigit(*s))
		++s;
	
	if (*s == 'e' || *s == 'E')
		++s;
	while (isdigit(*s))
		++s;
	
	*endp = (char *)s;	/* ugh! */

	return val;
}

#endif /* ! HAVE_STRTOD */
