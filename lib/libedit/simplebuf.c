/* simplebuf.c - simplest possible Edit_buffer implementation */

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


/* @(#)simplebuf.c	1.4 25 May 1995 (UKC) */
char edit_simplebuf_c_rcsid[] = "$Id$";

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <local/ukcprog.h>
#include <mtrprog/ifdefs.h>

#include "textbuf.h"
#include "simplebuf.h"

struct Text_bufdata {
	char *text;
	size_t len;
	size_t maxsize;
};
	
static void simplebuf_destroy PROTO((Text_bufdata *b));

static void simplebuf_insert PROTO((Text_bufdata *b, size_t pos,
				    const char *text, size_t len));
static void simplebuf_delete PROTO((Text_bufdata *b, size_t pos, size_t len));

static bool simplebuf_get_bytes PROTO((Text_bufdata *b, size_t pos,
				       const char **p_start,
				       const char **p_lim));
static bool simplebuf_get_bytes_before PROTO((Text_bufdata *b, size_t pos,
					      const char **p_start,
					      const char **p_lim));
static void simplebuf_set_debug_flag PROTO((Text_bufdata *b));

static size_t simplebuf_get_length PROTO((Text_bufdata *b));

#define INITIAL_MAXSIZE		100

#if HAVE_MEMMOVE || defined(memmove)
/* Have 'memmove()' or an alternative. */
#else
voidptr
memmove(vdst, vsrc, len)
voidptr vdst;
constvoidptr vsrc;
size_t len;
{
	const char *src, *lim;
	char *dst;

	src = vsrc;
	dst = vdst;
	
	if (dst < src) {
		lim = src + len;
		while (src < lim)
			*dst++ = *src++;
	}
	else {
		lim = src;
		src += len;
		dst += len;

		while (src > lim)
			*--dst = *--src;
	}

	return vdst;
}
#endif

Text_buffer *
text_create_simple_buffer()
{
	static Text_buffer_ops simplebuf_ops = {
		simplebuf_destroy,
		simplebuf_insert,
		simplebuf_delete,
		simplebuf_get_bytes,
		simplebuf_get_bytes_before,
		simplebuf_get_length,
		simplebuf_set_debug_flag
	};
	Text_bufdata *b;
	Text_buffer *tb;
	
	b = (Text_bufdata *)e_malloc(sizeof(Text_bufdata));
	
	b->text = e_malloc(INITIAL_MAXSIZE);
	b->len = 0;
	b->maxsize = INITIAL_MAXSIZE;

	tb = (Text_buffer *)e_malloc(sizeof(Text_buffer));
	tb->bufdata = b;
	tb->ops = &simplebuf_ops;

	return tb;
}

Text_buffer *
text_create_readonly_buffer(text, len)
const char *text;
size_t len;
{
	static Text_buffer_ops readonly_ops = {
		simplebuf_destroy,
		NULL,
		NULL,
		simplebuf_get_bytes,
		simplebuf_get_bytes_before,
		simplebuf_get_length,
		simplebuf_set_debug_flag
	};
	Text_bufdata *b;
	Text_buffer *tb;
	
	b = (Text_bufdata *)e_malloc(sizeof(Text_bufdata));
	
	b->text = (char *)text;
	b->len = len;
	b->maxsize = 0;

	tb = (Text_buffer *)e_malloc(sizeof(Text_buffer));
	tb->bufdata = b;
	tb->ops = &readonly_ops;

	return tb;
}

static void
simplebuf_set_debug_flag(b)
Text_bufdata *b;
{
	/*  No debugging stuff for simplebuf */
}

static void
simplebuf_destroy(b)
Text_bufdata *b;
{
	if (b->maxsize != 0)
		free(b->text);
	free((char *)b);
}

static void
simplebuf_insert(b, pos, text, len)
Text_bufdata *b;
size_t pos;
const char *text;
size_t len;
{
	size_t maxsize;

	if (pos > b->len)
		pos = b->len;
	
	maxsize = b->maxsize;
	while (b->len + len > maxsize)
		maxsize = maxsize * 3 / 2;

	if (maxsize > b->maxsize) {
		b->maxsize = maxsize;
		b->text = e_realloc(b->text, b->maxsize);
	}

	memmove(b->text + pos + len, b->text + pos, b->len - pos);
	memcpy(b->text + pos, text, len);

	b->len += len;
}

static void
simplebuf_delete(b, pos, len)
Text_bufdata *b;
size_t pos, len;
{
	if (pos + len > b->len)
		panic("pos out of range in simplebuf_delete");

	memmove(b->text + pos, b->text + pos + len, b->len - (pos + len));
	b->len -= len;
}

static size_t
simplebuf_get_length(b)
Text_bufdata *b;
{
	return b->len;
}

static bool
simplebuf_get_bytes(b, pos, p_start, p_lim)
Text_bufdata *b;
size_t pos;
const char **p_start, **p_lim;
{
	if (pos >= b->len) {
		*p_start = *p_lim = NULL;
		return FALSE;
	}
	
	*p_start = b->text + pos;
	*p_lim = b->text + b->len;
	return TRUE;
}

static bool
simplebuf_get_bytes_before(b, pos, p_start, p_lim)
Text_bufdata *b;
size_t pos;
const char **p_start, **p_lim;
{
	if (pos > b->len)
		pos = b->len;

	if (pos == 0) {
		*p_start = *p_lim = NULL;
		return FALSE;
	}
	
	*p_start = b->text;
	*p_lim = b->text + pos;
	
	return TRUE;
}
