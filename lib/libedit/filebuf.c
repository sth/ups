/* filebuf.c - make an lbuf referring to a file */

/*  Copyright 1994 Mark Russell, University of Kent at Canterbury.
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


/* @(#)filebuf.c	1.6 09 Apr 1995 (UKC) */
char edit_filebuf_c_rcsid[] = "$Id$";

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>	/* FIXME: goes when ukcprog fixed */

#include "textbuf.h"
#include "lbuf.h"
#include "filebuf.h"

typedef struct {
	const char *what;
	const char *path;
	int fd;
} Fhandle;

static bool file_info PROTO((char *handle, size_t *p_size, time_t *p_mtime));
static bool file_read PROTO((char *handle, char *buf, long pos, size_t count));
static void file_close PROTO((char *handle));
static bool open_if_necessary PROTO((Fhandle *fh));

static Fhandle *Lastfh = NULL;

static bool
open_if_necessary(fh)
Fhandle *fh;
{
	if (fh->fd != -1)
		return TRUE;

	if (Lastfh != NULL && Lastfh != fh && Lastfh->fd != -1) {
		close(Lastfh->fd);
		Lastfh->fd = -1;
	}

	if ((fh->fd = open(fh->path, O_RDONLY)) == -1) {
		failmesg("Can't open", fh->what, fh->path);
		return FALSE;
	}

	Lastfh = fh;

	return TRUE;
}

static bool
file_info(handle, p_size, p_mtime)
char *handle;
size_t *p_size;
time_t *p_mtime;
{
	Fhandle *fh;
	struct stat stbuf;

	fh = (Fhandle *)handle;

	if (!open_if_necessary(fh))
		return FALSE;
	
	if (fstat(fh->fd, &stbuf) != 0) {
		failmesg("Can't fstat", fh->what, fh->path);
		return FALSE;
	}

	*p_size = stbuf.st_size;
	*p_mtime = stbuf.st_mtime;
	return TRUE;
}

static bool
file_read(handle, buf, pos, count)
char *handle;
char *buf;
long pos;
size_t count;
{
	Fhandle *fh;
	long nread;

	fh = (Fhandle *)handle;

	if (!open_if_necessary(fh))
		return FALSE;
	
	if (lseek(fh->fd, (off_t)pos, SEEK_SET) == -1) {
		errf("Can't lseek to pos %ld in %s%s: %s",
		     pos, fh->what, fh->path, get_errno_str());
		return FALSE;
	}

	if ((nread = read(fh->fd, buf, count)) != count) {
		if (nread < 0)
			failmesg("Error reading", fh->what, fh->path);
		else
			errf("Unexpected EOF in %s%s", fh->what, fh->path);

		return FALSE;
	}

	return TRUE;
}

static void
file_close(handle)
char *handle;
{
	Fhandle *fh;

	fh = (Fhandle *)handle;

	if (fh->fd != -1 && close(fh->fd) != 0)
		failmesg("Warning: error closing", fh->what, fh->path);

	if (fh == Lastfh)
		Lastfh = NULL;
}

bool
edit_filebuf_visit_file(ap, what, path, maxblocks, blocksize, p_buffer)
alloc_pool_t *ap;
const char *what, *path;
size_t maxblocks, blocksize;
Text_buffer **p_buffer;
{
	Fhandle *fh;

	fh = (Fhandle *)alloc(ap, sizeof(Fhandle));
	fh->fd = -1;
	fh->what = what;
	fh->path = alloc_strdup(ap, path);

	if (!open_if_necessary(fh))
		return FALSE;
	
	*p_buffer = text_create_lbuf_buffer(ap, maxblocks, blocksize,
					    (char *)fh,
					    file_info, file_read, file_close, FALSE);
	return *p_buffer != NULL;
}

void
edit_reset_file_handle()
{
	Lastfh = NULL;
}

