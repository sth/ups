/* ebuf.c - an expanding buffer */

/*  Copyright 1993 Mark Russell, University of Kent at Canterbury
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


/* @(#)ebuf.c	1.6 25/10/93 UKC */
char ukcprog_ebuf_rcsid[] = "$Id$";

#include <stdlib.h>
#include <string.h>

#include "ukcprog.h"

struct ebuf_s {
	bool eb_errors_are_fatal;
	char *eb_buf;
	size_t eb_bufsize;
	size_t eb_pos;
};

ebuf_t *
ebuf_create(errors_are_fatal)
bool errors_are_fatal;
{
	ebuf_t *eb;
	char *buf;
	size_t bufsize;

	bufsize = 100;

	if (errors_are_fatal) {
		eb = (ebuf_t *)e_malloc(sizeof(ebuf_t));
		buf = e_malloc(bufsize);
	}
	else {
		if ((eb = (ebuf_t *)malloc(sizeof(ebuf_t))) == NULL)
			return NULL;
		if ((buf = malloc(bufsize)) == NULL) {
			free((char *)eb);
			return NULL;
		}
	}

	eb->eb_errors_are_fatal = errors_are_fatal;
	eb->eb_buf = buf;
	eb->eb_bufsize = bufsize;
	eb->eb_pos = 0;

	return eb;
}

void
ebuf_reset(eb)
ebuf_t *eb;
{
	eb->eb_pos = 0;
}

/*  We would like this to take a pointer to a size_t, but that would
 *  break existing code (when compiling with gcc).  Maybe later.
 */
voidptr
ebuf_get(eb, p_len)
ebuf_t *eb;
int *p_len;
{
	if (p_len != NULL)
		*p_len = eb->eb_pos;
	return eb->eb_buf;
}

ebuf_t *
ebuf_start(eb, errors_are_fatal)
ebuf_t *eb;
bool errors_are_fatal;
{
	if (eb == NULL)
		eb = ebuf_create(errors_are_fatal);
	else
		ebuf_reset(eb);

	return eb;
}

void
ebuf_free(eb)
ebuf_t *eb;
{
	free(eb->eb_buf);
	free((char *)eb);
}

int
ebuf_add(eb, buf, count)
ebuf_t *eb;
constvoidptr buf;
size_t count;
{
	size_t size;

	for (size = eb->eb_bufsize; eb->eb_pos + count > size; size *= 2)
		;

	if (size != eb->eb_bufsize) {
		if ((eb->eb_buf = realloc(eb->eb_buf, size)) == NULL) {
			if (eb->eb_errors_are_fatal)
				panic("realloc failed in ebuf_add");
			return -1;
		}
		eb->eb_bufsize = size;
	}

	memcpy(eb->eb_buf + eb->eb_pos, buf, count);
	eb->eb_pos += count;

	return 0;
}

int
ebuf_addstr(eb, str)
ebuf_t *eb;
const char *str;
{
	return ebuf_add(eb, str, strlen(str));
}
