/* strsave.c -- allocate memory and save a copy of a string */

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


/* @(#)strsave.c	1.7 30/5/93 UKC */
char ukcprog_strsave_rcsid[] = "$Id$";

#include <string.h>
#include <stdlib.h>	/* for malloc() */

#include "ukcprog.h"


char *
strsave(s)
const char *s;
{
	char *str;

	str = e_malloc(strlen(s) + 1);

	return strcpy(str, s);
}
