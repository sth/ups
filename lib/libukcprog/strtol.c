/* strtol.c - strtol, for machines that don't have it */

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


/* @(#)strtol.c	1.4 19/7/92 UKC */
char ukcprog_strtol_rcsid[] = "$Id$";

#if defined(clipper) || (defined(vax) && !defined(ultrix))

#include <ctype.h>
#include <stdlib.h>

#define NO_STRTOL
#endif

#ifdef NO_STRTOL
long
strtol(nptr, eptr, base)
const char *nptr;
char **eptr;
int base;
{
	const char *s, *save_s;
	long val;

	for (s = nptr; *s != '\0' && isspace(*s); ++s)
		;
	
	if (base > 36) {
		base = 10;
	}
	else if (base == 16) {
		if (*s == '0' && (s[1] == 'x' || s[1] == 'X'))
			s += 2;
	}
	else if (base == 0) {
		if (*s == '0') {
			++s;
			if (*s == 'x' || *s == 'X') {
				++s;
				base = 16;
			}
			else if (isdigit(*s))
				base = 8;
			else {
				--s;
				base = 10;
			}
		}
		else
			base = 10;
	}

	val = 0;
	for (save_s = s; *s != '\0'; ++s) {
		int digit;

		if (isdigit(*s))
			digit = *s - '0';
		else if (isupper(*s))
			digit = *s - 'A' + 10;
		else if (islower(*s))
			digit = *s - 'a' + 10;
		else
			break;
		if (digit >= base)
			break;
		
		val = val * base + digit;
	}
	if (s == save_s)
		s = nptr;

	if (eptr != NULL)
		*eptr = (char *)((s > save_s) ? s : nptr);
	
	return val;
}
#endif /* clipper || vax */
