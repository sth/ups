/* e_close.c - error checking interface to close */

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


/* @(#)e_close.c	1.8 16 Sep 1994 (UKC) */
char utils_e_close_rcsid[] = "$Id$";

#include <sys/types.h>
#include <sys/file.h>

#ifdef __STDC__
#include <unistd.h>
#endif

#include <local/ukcprog.h>

#include "utils.h"

int
e_close(fd, filename)
int fd;
const char *filename;
{
	int res;

	if ((res = close(fd)) != 0) {
		errf("Close of %s (fd %d) failed: %s",
		     filename, fd, get_errno_str());
	}
	
	return res;
}
