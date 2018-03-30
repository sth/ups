/* onpath.c - search for a file on a path */

/*  Copyright 1995 Mark Russell, University of Kent at Canterbury.
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


/* @(#)onpath.c	1.7 30/5/93 UKC */
char utils_onpath_rcsid[] = "$Id$";

#include "ifdefs.h"
#include <stdlib.h>
#include <local/ukcprog.h>
#include "utils.h"

/*  Returns the path to a file by looking in a list of directories.
 *  The name of the file is in the 'name' parameter.
 *  The list of directories is in 'dirs', seperated by 'seps',  
 *  e,g. "/lib:/usr/lib" and ":".
 *
 *  Returns NULL if the file was not found in any directory.
 *
 *  The returned string is malloc'ed storage - when it is finished with
 *  it should be passed to free() to release its storage.
 */
char *
onpath(dirs, seps, name)
const char *dirs, *seps, *name;
{
	char **dirvec, **dp;

	if ((dirs == NULL) || (name == NULL))
		return NULL;

	dirvec = ssplit(dirs, seps);

	for (dp = dirvec; *dp != NULL; ++dp) {
		char *path;
		
		path = strf("%s/%s", *dp, name);

		if (access(path, F_OK) == 0) {
			free(dirvec);
			return path;
		}

		free(path);
	}

	free(dirvec);
	return NULL;
}

