/* arg_dir.c - set of dirfunc routines for globbing against the file system */

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


/* @(#)arg_dir.c	1.12 17 Apr 1994 (UKC) */
char arg_dir_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include <sys/stat.h>
#include <local/ukcprog.h>

#include "arg.h"

static int fs_isdir PROTO((const char *path));
static int fs_opendir PROTO((const char *path, long *p_dirid));
static const char *fs_readdir PROTO((long dirid));
static void fs_closedir PROTO((long dirid));

static int
fs_isdir(path)
const char *path;
{
	struct stat stbuf;

	return stat(path, &stbuf) == 0 && S_ISDIR(stbuf.st_mode);
}

static int
fs_opendir(path, p_dirid)
const char *path;
long *p_dirid;
{
	DIR *dirp;

	if ((dirp = opendir(path)) == NULL)
		return -1;
	*p_dirid = (long) dirp;
	return 0;
}

static const char *
fs_readdir(dirid)
long dirid;
{
	struct dirent *d;

	d = readdir((DIR *)dirid);
	return (d != NULL) ? d->d_name : NULL;
}

static void
fs_closedir(dirid)
long dirid;
{
	closedir((DIR *)dirid);
}

static dirfuncs_t Fs_df = {
	fs_isdir,
	fs_opendir,
	fs_readdir,
	fs_closedir
};

dvec_t
arg_glob_cpat(cpat, buf, buflen)
const char *cpat;
char *buf;
int buflen;
{
	return arg_gen_glob_cpat(cpat, buf, buflen, &Fs_df);
}

dvec_t
arg_glob(p_pat)
const char **p_pat;
{
	return arg_gen_glob(p_pat, &Fs_df);
}
