/* filetype_to_string.c - convert a file mode to an ls type string */

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


/* @(#)fttostr.c	1.5 17 Apr 1994 (UKC) */
char utils_filetype_to_string_rcsid[] = "$Id$";

#define UKC_WANT_COMMON_UNIX_EXTENSIONS

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <local/ukcprog.h>

#include "utils.h"

const char *
filetype_to_string(mode)
int mode;
{
	static char buf[100];

	if (S_ISREG(mode))
		return "regular file";
	if (S_ISDIR(mode))
		return "directory";
	if (S_ISCHR(mode))
		return "character special file";
	if (S_ISBLK(mode))
		return "block special file";
	if (S_ISFIFO(mode))
		return "named pipe";
#ifdef S_ISSOCK
	if (S_ISSOCK(mode))
		return "socket";
#endif
#ifdef S_ISLNK
	if (S_ISLNK(mode))
		return "symbolic link";
#endif
	
	sprintf(buf, "file with unknown mode 0%o", mode);
	return buf;
}
