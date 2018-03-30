/* apply_to_files_in.c - apply a function to all files in a directory */

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


/* @(#)appltofl.c	1.9 16 Sep 1994 (UKC) */
char utils_apply_to_files_in_rcsid[] = "$Id$";

#include "ifdefs.h"
#include <local/ukcprog.h>

#include "utils.h"

/*  Apply func to all the files in directory dirname.  Return -1 and print an
 *  error message on stderr if there was an error reading the directory.
 *
 *  Return -1 and stop reading the directory if func returns non-zero.
 *
 *  Return 0 if we get the end of the directory without errors.
 */
int
apply_to_files_in(dirname, func, arg)
const char *dirname;
int (*func)PROTO((const char *func_dirname, const char *filename, char *arg));
char *arg;
{
	DIR *dirp;
	struct dirent *dp;
	bool readdir_error;

	if ((dirp = opendir(dirname)) == NULL) {
		failmesg("Can't open directory", "", dirname);
		return -1;
	}

	for (;;) {
		errno = 0;
		dp = readdir(dirp);
		readdir_error = errno != 0;
		if (dp == NULL)
			break;
		if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
			if ((*func)(dirname, dp->d_name, arg) != 0)
				break;
		}
	}

	/*  We would like to check whether closedir succeeded, but it
	 *  is declared as void on some systems.
	 */
	closedir(dirp);

	if (dp != NULL)
		return -1;
	if (readdir_error) {
		failmesg("Error reading directory", "", dirname);
		return -1;
	}
	return 0;
}
