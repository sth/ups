/* mperror.c - Mperror code */

/*  Copyright 1991 John Bovey, University of Kent at Canterbury.
 *  All Rights Reserved.
 *
 *  This file is part of MED3.
 *
 *  MED3 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  MED3 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with UPS; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* @(#)mperror.c	1.6 4/7/91 (UKC) */
char Men3_mperror_c_rcsid[] = "$Id$";

#include <stdio.h>

#include <local/wn.h>
#include "menu3.h"
#include "menu_priv.h"

static const char * merrmes[] = {
	"no error",
	"menu table full",
	"menu descriptor not of an open menu",
	"menu descriptor out of range",
	"can't read menu from menu file",
	"can't open menu file",
	"bad window file descriptor",
	"menu not displayed",
	"bad menu function argument",
	"message area error",
	"menerr out of range",
	NULL
};

/*  print out a menu error message
 */
int
Mperror(str)
const char * str;
{
	register int i;

	/* protect against out of range error number.
	 */
	menerr = menerr < 0 ? 1000 : menerr;
	for (i = 0; i <= menerr; i++)
		if (merrmes[i] == NULL)
			break;
	fprintf(stderr,"%s: %s\7\n",str,merrmes[i - 1]);
	return(0);
}
