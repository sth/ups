/* is_number.c - return TRUE is a string is a decimal number */

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


/* @(#)is_number.c	1.4 26/4/92 (UKC) */
char utils_is_number_rcsid[] = "$Id$";

#include <ctype.h>
#include <local/ukcprog.h>

#include "utils.h"

int
is_number(s)
const char *s;
{
	for (; *s != '\0'; s++)
		if (!isdigit(*s))
			return FALSE;
	return TRUE;
}
