/* memxxx.c - memxxx routines for machines that don't have them */

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


/* @(#)memxxx.c	1.6 26/4/92 (UKC) */
char utils_memxxx_rcsid[] = "$Id$";

#include <ifdefs.h>
#include <stdlib.h>


#ifdef ARCH_CLIPPER
#define NO_MEMXXX
#endif

#ifdef NO_MEMXXX
char *
memcpy(dst, src, nbytes)
register char *dst;
register const char *src;
int nbytes;
{
	char *res;

	res = dst;
	while (--nbytes >= 0)
		*dst++ = *src++;
	return res;
}

int
memcmp(m1, m2, nbytes)
register char *m1, *m2;
int nbytes;
{
	for (; --nbytes >= 0; ++m1, ++m2)
		if (*m1 != *m2)
			return m1 - m2;
	return 0;
}

char *
memset(m, c, nbytes)
register char *m;
int c, nbytes;
{
	char *res;

	res = m;
	while (--nbytes >= 0)
		*m++ = c;
	return res;
}

char *
memchr(m, c, nbytes)
register char *m;
int c, nbytes;
{
	register char *lim;

	for (lim = m + nbytes; m < lim; ++m)
		if (*m == c)
			return m;
	return NULL;
}
#endif
