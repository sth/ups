/* util.c - convenience routines, implemented using public interfaces only */

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


/* @(#)util.c	1.4 16 Sep 1994 (UKC) */
char edit_util_c_rcsid[] = "$Id$";

#include <stdlib.h>
#include <string.h>

#ifndef __STDC__
char *memchr();
#endif

#include <local/ukcprog.h>

#include "edit.h"

struct Edit_lineinfo {
	Edit_buffer *buffer;
	size_t pos;
	const char *iptr;
	const char *ilim;
	int endc;
	size_t linelen;
	size_t linebuf_size;
	char *linebuf;
};

static void make_charmap PROTO((const char *pat, unsigned char *map));
static void add_bytes PROTO((Edit_lineinfo *li, const char *buf, size_t len));
static bool nextline PROTO((Edit_lineinfo *li, size_t lim, const char **p_line, 
                            size_t *p_linelen, bool *p_seen_eol));

Edit_lineinfo *
edit_make_lineinfo(buffer, endc)
Edit_buffer *buffer;
int endc;
{
	Edit_lineinfo *li;

	li = (Edit_lineinfo *)e_malloc(sizeof(Edit_lineinfo));
	li->buffer = buffer;
	li->pos = 0;
	li->iptr = NULL;
	li->ilim = NULL;
	li->endc = endc;
	li->linelen = 0;	/* for neatness only */
	li->linebuf_size = 100;
	li->linebuf = e_malloc(li->linebuf_size + 1);

	return li;
}

void
edit_set_lineinfo_pos(li, pos)
Edit_lineinfo *li;
size_t pos;
{
	li->pos = pos;
	li->iptr = NULL;
	li->ilim = NULL;
}

size_t
edit_get_lineinfo_pos(li)
Edit_lineinfo *li;
{
	return li->pos;
}

void
edit_free_lineinfo(li)
Edit_lineinfo *li;
{
	free(li->linebuf);
	free((char *)li);
}

bool
edit_nextline(li, p_line, p_linelen, p_seen_eol)
Edit_lineinfo *li;
const char **p_line;
size_t *p_linelen;
bool *p_seen_eol;
{
	return nextline(li, EDIT_MAX_POINT, p_line, p_linelen, p_seen_eol);
}

static bool
nextline(li, lim, p_line, p_linelen, p_seen_eol)
Edit_lineinfo *li;
size_t lim;
const char **p_line;
size_t *p_linelen;
bool *p_seen_eol;
{
	const char *endstr, *old_iptr;
	size_t len, linelen;
	bool seen_eol;
	
	endstr = old_iptr = NULL;
	li->linelen = linelen = len = 0;

	for (;;) {
		if (li->iptr == li->ilim) {
			if (li->pos == lim)
				break;
			
			if (!edit_get_bytes(li->buffer, li->pos,
					    &li->iptr, &li->ilim))
				break;

			if (li->ilim - li->iptr > lim - li->pos)
				li->ilim = li->iptr + (lim - li->pos);
		}
	
		endstr = memchr(li->iptr, li->endc,
				(size_t)(li->ilim - li->iptr));

		old_iptr = li->iptr;
	
		li->iptr = (endstr != NULL) ? endstr + 1 : li->ilim;
		len = li->iptr - old_iptr;
		li->pos += len;

		linelen += len;
		
		if (endstr != NULL)
			break;
		
		add_bytes(li, li->iptr - len, len);
	}

	seen_eol = endstr != NULL;

	if (linelen > len && li->iptr != NULL) { /* RGA */
		add_bytes(li, li->iptr - len, len);
		*p_line = li->linebuf;
	}
	else {
		*p_line = old_iptr;
	}
	
	*p_linelen = linelen - (seen_eol ? 1 : 0);
	*p_seen_eol = seen_eol;

	return linelen != 0 || seen_eol;
}

static void
add_bytes(li, buf, len)
Edit_lineinfo *li;
const char *buf;
size_t len;
{
	if (li->linelen + len > li->linebuf_size) {
		li->linebuf_size = li->linelen + len + 50;
		li->linebuf = e_realloc(li->linebuf, li->linebuf_size + 1);
	}

	memcpy(li->linebuf + li->linelen, buf, len);
	li->linelen += len;
}

char *
edit_get_writable_copy(li, line, len)
Edit_lineinfo *li;
const char *line;
size_t len;
{
	if (line != li->linebuf) {
		li->linelen = 0;
		add_bytes(li, line, len);
	}

	return li->linebuf;
}

bool
edit_prevline(li, p_line, p_linelen, p_seen_eol)
Edit_lineinfo *li;
const char **p_line;
size_t *p_linelen;
bool *p_seen_eol;
{
	size_t save_pos, lim;
	bool more;
	char nl[2];
	
	if (li->pos == 0)
		return FALSE;

	li->iptr = li->ilim = NULL;

	nl[0] = li->endc;
	nl[1] = '\0';
	
	lim = li->pos;

	--li->pos;
	
	if (edit_find_char_backwards(li->buffer, li->pos, 0, nl, &li->pos))
		++li->pos;

	save_pos = li->pos;
	more = nextline(li, lim, p_line, p_linelen, p_seen_eol);
	li->pos = save_pos;

	return more;
}

void
edit_add_keymap_control_entry(keymap, key, function)
Edit_keymap *keymap;
int key;
Edit_function function;
{
	edit_add_keymap_entry(keymap, EDIT_CONTROL, key, function);
	edit_add_keymap_entry(keymap, EDIT_CONTROL | EDIT_SHIFT, key, function);
}

void
edit_update_propchange_pair_pos(buffer, point, startpc, endpc, p_movepc)
Edit_buffer *buffer;
size_t point;
Edit_propchange *startpc, *endpc, **p_movepc;
{
	size_t start_point, end_point;
	bool oldval;

	start_point = edit_get_propchange_point(startpc);
	end_point = edit_get_propchange_point(endpc);

	if (*p_movepc == NULL) {
		size_t start_dist, end_dist;

		start_dist = (point > start_point) ? point - start_point
						   : start_point - point;
		end_dist = (point > end_point) ? point - end_point
					       : end_point - point;

		*p_movepc = (start_dist < end_dist) ? startpc : endpc;
	}
	
	if (*p_movepc != startpc && *p_movepc != endpc)
		panic("movepc botch in eupp");
		
	oldval = edit_set_propchange_updates(buffer, FALSE);

	if (*p_movepc == endpc && point < start_point) {
		edit_move_propchange(buffer, endpc, start_point);
		*p_movepc = startpc;
	}
	else if (*p_movepc == startpc && point > end_point) {
		edit_move_propchange(buffer, startpc, end_point);
		*p_movepc = endpc;
	}

	edit_move_propchange(buffer, *p_movepc, point);

	edit_set_propchange_updates(buffer, oldval);
}

char *
edit_copy_bytes(buffer, start, lim, orig_obuf)
Edit_buffer *buffer;
size_t start, lim;
char *orig_obuf;
{
	const char *iptr, *ilim;
	char *optr, *obuf;
	
	obuf = (orig_obuf != NULL) ? orig_obuf : e_malloc(lim - start + 1);
	optr = obuf;
	
	while (start < lim && edit_get_bytes(buffer, start, &iptr, &ilim)) {
		size_t len;
		
		len = ilim - iptr;
		if (start + len > lim)
			len = lim - start;
		
		memcpy(optr, iptr, len);

		optr += len;
		start += len;
	}

	if (orig_obuf == NULL)
		*optr = '\0';
	
	return obuf;
}

#define CHARMAP_SIZE	((size_t)(1 << (sizeof(char) * 8)))

bool
edit_find_char_forwards(buffer, pos, lim, pat, p_pos)
Edit_buffer *buffer;
size_t pos, lim;
const char *pat;
size_t *p_pos;
{
	unsigned char map[CHARMAP_SIZE];
	const char *ibuf, *ilim;

	make_charmap(pat, map);
	
	while (pos < lim && edit_get_bytes(buffer, pos, &ibuf, &ilim)) {
		const char *iptr;

		if (ilim - ibuf > lim - pos)
			ilim = ibuf + (lim - pos);

		for (iptr = ibuf; iptr < ilim; ++iptr) {
			if (map[(unsigned char)*iptr] != 0) {
				*p_pos = pos + (iptr - ibuf);
				return TRUE;
			}
		}

		pos += ilim - ibuf;
	}

	*p_pos = pos;
	return FALSE;
}

bool
edit_find_char_backwards(buffer, pos, minpos, pat, p_pos)
Edit_buffer *buffer;
size_t pos, minpos;
const char *pat;
size_t *p_pos;
{
	unsigned char map[CHARMAP_SIZE];
	const char *ibuf, *ilim;

	make_charmap(pat, map);
	
	while (pos > minpos &&
	       edit_get_bytes_before(buffer, pos, &ibuf, &ilim)) {
		const char *iptr;

		pos -= ilim - ibuf;

		if (pos < minpos) {
			ibuf += minpos - pos;
			pos = minpos;
		}
		
		iptr = ilim;

		while (iptr != ibuf) {
			if (map[(unsigned char)*--iptr] != 0) {
				*p_pos = pos + (iptr - ibuf);
				return TRUE;
			}
		}

		if (pos == minpos)
			break;
	}

	*p_pos = pos;
	return FALSE;
}

static void
make_charmap(pat, map)
const char *pat;
unsigned char *map;
{
	const char *patstart;
	bool invert;
	
	memset(map, '\0', CHARMAP_SIZE);

	/*  Fast track for single character searches.  This also handles
	 *  the "!" special case.
	 */
	if (pat[0] != '\0' && pat[1] == '\0') {
		map[*pat] = 1;
		return;
	}

	invert = *pat == '!';
	if (invert)
		++pat;

	for (patstart = pat; *pat != '\0'; ++pat) {
		if (*pat == '-' && pat != patstart && pat[1] != '\0') {
			int i, lastch;
			
			lastch = *++pat;

			map[pat[-2]] = 0; /* Might be an empty range */
			
			for (i = pat[-2]; i <= lastch; ++i)
				map[i] = 1;
		}
		else {
			map[*pat] = 1;
		}
	}

	if (invert) {
		int i;
		
		for (i = 0; i < CHARMAP_SIZE; ++i)
			map[i] = !map[i];
	}
}
