/* swap_longs.c - byte swap an array of longs */

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


/* @(#)swap_longs.c	1.5 30 May 1994 (UKC) */
char utils_swap_longs_rcsid[] = "$Id$";

#include <local/ukcprog.h>

#include "utils.h"

void
swap_longs(base, count)
long *base;
size_t count;
{
	unsigned char *s, *lim;

	lim = (unsigned char *)(base + count);
	for (s = (unsigned char *)base; s < lim; s += sizeof(int)) {
		unsigned char c;

		c = s[0];
		s[0] = s[3];
		s[3] = c;

		c = s[1];
		s[1] = s[2];
		s[2] = c;
	}
}
