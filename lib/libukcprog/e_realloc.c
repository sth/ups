/*  e_realloc() -- Error checking realloc. */

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


/* @(#)e_realloc.c	1.8 30/5/93 UKC */
char ukcprog_realloc_rcsid[] = "$Id$";

#ifndef __STDC__
#include <sys/types.h>	/* for size_t */
#endif

#include <stdio.h>	/* for NULL */
#include <stdlib.h>

#include "ukcprog.h"


voidptr
e_realloc(old, size)
voidptr old;
size_t size;
{
	char *new;

	if (old == NULL)
		return e_malloc(size);

	if (size == 0) {
		free(old);
		return NULL;
	}
		
	if ((new = realloc(old, (size_t)size)) == NULL)
		panic("realloc failed in e_realloc");

	return new;
}
