/* read_chunk.c - chained read_chunk table */

/* @(#)read_chunk.c	1.1 24/5/95 (UKC) */
char mtrprog_read_chunk_rcsid[] = "$Id$";

/* Copyright 1995 Mark Russell, University of Kent at Canterbury */

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <local/ukcprog.h>

#include "utils.h"
#include "io.h"

bool
open_for_reading(path, what, p_fd)
const char *path, *what;
int *p_fd;
{
	int fd;
	
	if ((fd = open(path, O_RDONLY)) == -1) {
		failmesg("Can't open", what, path);
		return FALSE;
	}

	*p_fd = fd;
	return TRUE;
}

bool
read_chunk(path, what, fd, chunkname, offset, buf, nbytes)
const char *path, *what;
int fd;
const char *chunkname;
off_t offset;
voidptr buf;
size_t nbytes;
{
#ifdef _POSIX_SOURCE
	ssize_t nread;
#else
	int nread;
#endif
	const char *wpad;
	wpad = (*what != '\0') ? " " : "";
	
	if (lseek(fd, offset, SEEK_SET) == -1) {
		errf("Can't seek to offset %ld in %s%s%s to read %s: %s",
		     offset, what, wpad, path, chunkname, get_errno_str());
		return FALSE;
	}
	
	if ((nread = read(fd, buf, nbytes)) != nbytes) {
		if (nread == -1) {
			errf("Error reading %s from %s%s%s: %s",
			     chunkname, what, wpad, path, get_errno_str());
		}
		else {
			errf("Unexpected EOF reading %s (%lu bytes) from %s%s%s",
			     chunkname, (unsigned long)nbytes,
			     what, wpad, path);
		}
		return FALSE;
	}

	return TRUE;
}
