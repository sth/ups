/* srcbuf.c - maintain the buffers on source files */

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


/* @(#)srcbuf.c	1.3 09 Apr 1995 (UKC) */
char ups_srcbuf_c_rcsid[] = "$Id$";

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <local/ukcprog.h>
#include <local/edit/textbuf.h>
#include <local/edit/filebuf.h>
#include <local/edit/edit.h>

#include "srcbuf.h"

/*  Element in the list of editable lines for a file.
 *  These are pseudo source lines added by ups for things like breakpoints.
 *
 *  Each fil_t structure has a list of these blocks, which is kept sorted
 *  by real source line number.
 */
struct Editblock {
	Srcbuf *srcbuf;
	Edit_propchange *startpc;
	Edit_propchange *limpc;
	char *text;
	char *user_data;			
	Editblock_callback callback;
	Editblock_change_callback change_callback;
	bool removing;
	Editblock *next;
};

struct Srcbuf {
	const char *what;
	const char *path;
	Edit_buffer *buffer;
	size_t *linetab;
	size_t num_file_lines;
	Editblock *eblist;
  	ebuf_t *ebuf;
};

static void make_linetab PROTO((alloc_pool_t *ap, Edit_buffer *lb,
				size_t **p_linetab, size_t *p_num_file_lines,
				ebuf_t **p_ebuf));
static Editblock *get_editblock_before_point PROTO((Srcbuf *sb, size_t point));
static size_t startpos PROTO((Editblock *eb));
static Editblock *get_prev_editblock PROTO((Srcbuf *sb, Editblock *match_eb));

void
srcbuf_get_buffer_modtime(sb,  mtime)
     Srcbuf *sb;
     long *mtime;
{
    edit_get_buffer_modtime(sb->buffer, mtime);
}

Srcbuf *
srcbuf_visit_file(ap, what, path, fi, flags)
alloc_pool_t *ap;
const char *what, *path;
Edit_fontinfo *fi;
Edit_flags flags;
{
	Srcbuf *sb;
	Text_buffer *textbuf;
	Edit_buffer *buffer;
	size_t *linetab, num_file_lines;
	ebuf_t *ebuf;

	if (!edit_filebuf_visit_file(ap, what, path, 4, 1024, &textbuf))
		return NULL;
	buffer = edit_create_buffer(textbuf, fi, flags);

	make_linetab(ap, buffer, &linetab, &num_file_lines, &ebuf);
			       
	sb = (Srcbuf *)alloc(ap, sizeof(Srcbuf));
	sb->what = what;
	sb->path = alloc_strdup(ap, path);
	sb->buffer = buffer;
	sb->linetab = linetab;
	sb->num_file_lines = num_file_lines;
	sb->eblist = NULL;
	sb->ebuf = ebuf;
	return sb;
}

static void
make_linetab(ap, buffer, p_linetab, p_num_file_lines, p_ebuf)
alloc_pool_t *ap;
Edit_buffer *buffer;
size_t **p_linetab;
size_t *p_num_file_lines;
ebuf_t **p_ebuf;
{
	size_t pos, nextpos, buflen, nbytes;
	const char *buf;
	int int_nbytes;
	static ebuf_t *eb = NULL;

	eb = ebuf_create(TRUE);
	buflen = edit_get_buffer_length(buffer);

	for (pos = 0; pos < buflen; pos = nextpos) {
		ebuf_add(eb, (char *)&pos, sizeof(pos));
		
		if (edit_find_char_forwards(buffer, pos, buflen, "\n",
					    &nextpos))
			++nextpos;
	}

	/*  Add an extra entry for eof.
	 */
	ebuf_add(eb, (char *)&pos, sizeof(pos));

	buf = ebuf_get(eb, &int_nbytes);
	nbytes = (size_t)int_nbytes;
	
	*p_linetab = (size_t *)memcpy(alloc(ap, nbytes), buf, nbytes);
	*p_num_file_lines = nbytes / sizeof(size_t) - 1;
	*p_ebuf = eb;
}

bool
srcbuf_file_lnum_to_point_range(sb, lnum, p_start, p_lim)
Srcbuf *sb;
int lnum;
size_t *p_start, *p_lim;
{
	size_t start, orig_start;
	Editblock *eb;
	
	if (lnum < 0 || lnum >= sb->num_file_lines)
		return FALSE;
	
	start = orig_start = sb->linetab[lnum];

	for (eb = sb->eblist; eb != NULL; eb = eb->next) {
		size_t ebstart;
		
		ebstart = edit_get_propchange_point(eb->startpc);

		if (ebstart > start)
			break;

		/* RCB: Add one for newline at end of edit block. */
		start += edit_get_propchange_point(eb->limpc) - ebstart+1;
	}

	*p_start = start;
	*p_lim = sb->linetab[lnum + 1] + (start - orig_start);
	
	return TRUE;
}

bool
srcbuf_point_to_file_lnum(sb, point, p_lnum)
Srcbuf *sb;
size_t point;
int *p_lnum;
{
	size_t *lineptr, *linelim;
	size_t delta;
	Editblock *eb;
	
	delta = 0;
	
	for (eb = sb->eblist; eb != NULL; eb = eb->next) {
		size_t lim;
		
		lim = edit_get_propchange_point(eb->limpc);

		if (lim > point)
			break;

		/* RCB: Add one for newline at end of edit block. */
		delta += lim - edit_get_propchange_point(eb->startpc)+1;
	}

	point -= delta;
	
	linelim = sb->linetab + sb->num_file_lines;

	for (lineptr = sb->linetab + 1; lineptr < linelim; ++lineptr) {
		if (*lineptr > point) {
			*p_lnum = (lineptr - sb->linetab) - 1;
			return TRUE;
		}
	}

	return FALSE;
}

Edit_propchange*
srcbuf_get_propchange_at_point(sb, point)
Srcbuf *sb;
size_t point;
{
        Editblock *eb;
        
        for (eb = sb->eblist; eb != NULL; eb = eb->next) {
                size_t ebstart;
                
                ebstart = edit_get_propchange_point(eb->startpc);
                if (    ebstart <= point
                     && point <= edit_get_propchange_point(eb->limpc))
                    return eb->startpc;
        }
        
        return NULL;
}


Edit_buffer *
srcbuf_get_buffer(sb)
Srcbuf *sb;
{
	return sb->buffer;
}

size_t
srcbuf_nlines_in_file(sb)
Srcbuf *sb;
{
	return sb->num_file_lines;
}

const char *
srcbuf_get_path(sb)
Srcbuf *sb;
{
	return sb->path;
}

static size_t
startpos(eb)
Editblock *eb;
{
	if (eb == NULL)
		return EDIT_MAX_POINT;

	return edit_get_propchange_point(eb->startpc);
}

void
srcbuf_count_pixels(sb, lim_point, lim_ypos, editline_height, srcline_height,
		    p_lnum, p_ypos)
Srcbuf *sb;
size_t lim_point;
int lim_ypos, editline_height, srcline_height, *p_lnum, *p_ypos;
{
	int ypos, mixedline_height;
	size_t offset, ebstart, *lptr, *llim;
	Editblock *eb;
	
	mixedline_height = (editline_height > srcline_height) ?
					    editline_height : srcline_height;

	offset = 0;
	ypos = 0;
	lptr = sb->linetab;
	llim = lptr + sb->num_file_lines;
	
	eb = sb->eblist;
	ebstart = startpos(eb);

	for (;;) {
		if (*lptr + offset >= ebstart) {
			size_t eblen;
			
			ypos += srcbuf_get_editblock_height(eb, lim_point,
							    editline_height,
							    srcline_height);

			eblen = edit_get_propchange_point(eb->limpc) - ebstart;
			offset += eblen;

			eb = eb->next;
			ebstart = startpos(eb);
		}
		
		for (; lptr < llim; ++lptr) {
			size_t point;

			point = *lptr + offset;
			
			if (ypos >= lim_ypos || point >= lim_point)
				goto quit;
			
			if (point > ebstart)
				break;
			
			ypos += srcline_height;
		}

		if (lptr == llim)
			break;
	}

 quit:
	*p_lnum = lptr - sb->linetab;
	*p_ypos = ypos;
}

void
srcbuf_destroy(sb)
Srcbuf *sb;
{
	Editblock *eb, *next;
	
	for (eb = sb->eblist; eb != NULL; eb = next) {
		next = eb->next;
		srcbuf_remove_editblock(eb, FALSE);
	}

	ebuf_free(sb->ebuf);
	edit_destroy_buffer(sb->buffer);
}

char *
srcbuf_get_editblock_data(eb)
Editblock *eb;
{
	return eb->user_data;
}

const char *
srcbuf_get_editblock_text(eb)
Editblock *eb;
{
	return eb->text;
}

void
srcbuf_get_editblock_point_range(eb, p_start, p_lim)
Editblock *eb;
size_t *p_start, *p_lim;
{
	*p_start = edit_get_propchange_point(eb->startpc);
	*p_lim = edit_get_propchange_point(eb->limpc);
}

/* RGA use this for keeping editblocks in duplicate functions the same.
 *
 *  Used for dynamically relinked functions.
 */

int *
srcbuf_update_tracking_editblock(eb, sb, lines, nlines)
Editblock *eb;
Srcbuf *sb;
const char **lines;
int nlines;
{
  Editblock *eb1;
  int i, j;

  for (i = 0, eb1 = sb->eblist; eb1 != NULL; eb1 = eb1->next, i++)
    if (eb1 == eb)
      break;

  if (eb1)
  {
    for (j = 0, eb1 = sb->eblist; eb1 != NULL; eb1 = eb1->next, j++)
      if (i == j)
	break;
    if (eb1)
    {
/*      free_lines(eb1->lines, eb1->nlines);
      eb1->lines = dup_lines(lines, nlines);
      eb1->nlines = nlines;*/
      return (int *)eb1->user_data;
    }
  }
  return 0;
}

int *
srcbuf_get_editblock_in_tracking_file(eb, sb, sb1)
Editblock *eb;
Srcbuf *sb, *sb1;
{
  Editblock *eb1;
  int i, j;

  if (eb && sb)
  {
    for (i = 0, eb1 = sb->eblist; eb1 != NULL; eb1 = eb1->next, i++)
      if (eb1 == eb)
	break;
    
    if (eb1)
    {
      for (j = 0, eb1 = sb1->eblist; eb1 != NULL; eb1 = eb1->next, j++)
	if (i == j)
	  return (int *)eb1->user_data;
    }
  }
  return 0;
}

Editblock *
srcbuf_add_editblock(sb, fil_lnum, flagmask, flags,
		     start_fontinfo, end_fontinfo, text,
		     callback, change_callback, user_data)
Srcbuf *sb;
int fil_lnum;
Edit_flags flagmask, flags;
Edit_fontinfo *start_fontinfo, *end_fontinfo;
char *text;
Editblock_callback callback;
Editblock_change_callback change_callback;
char *user_data;
{
	bool oldval;
	Edit_propchange *startpc, *limpc;
	size_t start, lim;
	Editblock *eb, *prev;
	
	if (!srcbuf_file_lnum_to_point_range(sb, fil_lnum, &start, &lim))
	{
	  if (fil_lnum < 0)
	    errf("Line number %d less than one for %s", fil_lnum, sb->path);
	  else
	    errf("Can't display line %d of %s - it only has %ld lines",
		 fil_lnum, sb->path, sb->num_file_lines);
	  return NULL;
	}
	
	eb = (Editblock *)e_malloc(sizeof(Editblock));

	oldval = edit_set_propchange_updates(sb->buffer, FALSE);
	startpc = edit_add_propchange(sb->buffer, start, (char *)eb,
				      start_fontinfo, flagmask, flags, TRUE);
	limpc = edit_add_propchange(sb->buffer, start, (char *)NULL,
				    end_fontinfo, flagmask, 0, FALSE);
	edit_set_propchange_updates(sb->buffer, oldval);

	edit_insert(sb->buffer, start, text, strlen(text), TRUE);

	/* RCB: Back up over the newline at the end of the text
	** This is to prevent consecutive property changes when
	** there is more than one breakpoint on a single source line.
	*/
	edit_move_propchange_point(limpc,-1);

	eb->srcbuf = sb;
	eb->startpc = startpc;
	eb->limpc = limpc;
	eb->text = text;
	eb->user_data = user_data;
	eb->callback = callback;
	eb->change_callback = change_callback;
	eb->removing = FALSE;

	prev = get_editblock_before_point(sb, start);
	
	if (prev == NULL) {
		eb->next = sb->eblist;
		sb->eblist = eb;
	}
	else {
		eb->next = prev->next;
		prev->next = eb;
	}

	if (eb->next == eb)
		panic("editblock list botch in sae");

	return eb;
}

void srcbuf_editblock_change_flags(eb, flags)
Editblock *eb;
Edit_flags flags;
{
	edit_change_propchange_info(eb->startpc
				    , NULL /* p_point */
				    , NULL /* p_user_data */
				    , NULL /* p_fontinfo */
				    , &flags );
}

/*  Get the height in pixels added by the existence of eb.
 */
int
srcbuf_get_editblock_height(eb, lim_point, editline_height, srcline_height)
Editblock *eb;
size_t lim_point;
int editline_height, srcline_height;
{
	size_t start, lim;
	const char *iptr, *ilim;
	int num_mixed_lines, num_edit_lines, num_file_lines;
	size_t last_nl_point, point;
	int mixedline_height;

	start = edit_get_propchange_point(eb->startpc);
	lim = edit_get_propchange_point(eb->limpc);

	if (lim_point < lim)
		lim = lim_point;

	iptr = ilim = NULL;

	if (start > 0 &&
	    edit_get_bytes(eb->srcbuf->buffer, start - 1, &iptr, &ilim)) {
		last_nl_point = (*iptr == '\n') ? start : lim;
		++iptr;
	}
	else {
		last_nl_point = lim;
	}
	
	num_mixed_lines = num_edit_lines = 0;
	
	for (point = start; point < lim; ++point, ++iptr) {
		if (iptr == ilim &&
		    !edit_get_bytes(eb->srcbuf->buffer, point, &iptr, &ilim))
			break;

		if (*iptr == '\n') {
			if (last_nl_point != lim && last_nl_point >= start)
				++num_edit_lines;
			else
				++num_mixed_lines;

			last_nl_point = point;
		}
	}

	if (last_nl_point + 1 != lim) {
		++num_mixed_lines;
		num_file_lines = -1;
	}
	else {
		num_file_lines = 0;
	}

	mixedline_height = (editline_height > srcline_height) ?
					      editline_height : srcline_height;

	return num_file_lines * srcline_height +
	       num_edit_lines * editline_height +
	       num_mixed_lines * mixedline_height;
}

bool
srcbuf_change_editblock(eb, force, p_error_point)
Editblock *eb;
bool force;
size_t *p_error_point;
{
	size_t start, lim, point, error_pos;
	Edit_buffer *buffer;
	char *new_text;
	
	buffer = eb->srcbuf->buffer;

	start = edit_get_propchange_point(eb->startpc);
	lim = edit_get_propchange_point(eb->limpc);

	if (!edit_find_char_forwards(buffer, start, lim, "! \t\n", &point)) {
		srcbuf_remove_editblock(eb, TRUE);
		return TRUE;
	}
	
	new_text = edit_copy_bytes(buffer, start, lim, (char *)NULL);
	
	if (strcmp(new_text, eb->text) == 0)
		return TRUE;

	if (eb->change_callback == NULL ||
	    (*eb->change_callback)(eb, new_text, force, &error_pos)) {
		free(eb->text);
		eb->text = new_text;
		return TRUE;
	}

	*p_error_point = start + error_pos;
	return FALSE;
}
	
void
srcbuf_remove_editblock(eb, call_callback)
Editblock *eb;
bool call_callback;
{
	bool oldval;
	Srcbuf *sb;
	size_t start, lim;
	Editblock *prev;

	if (eb->removing)
		panic("dup remove in sre");
	
	sb = eb->srcbuf;
	start = edit_get_propchange_point(eb->startpc);
	lim = edit_get_propchange_point(eb->limpc);

	oldval = edit_set_propchange_updates(sb->buffer, FALSE);
	
	edit_delete_propchange(sb->buffer, eb->startpc);
	edit_delete_propchange(sb->buffer, eb->limpc);

	edit_set_propchange_updates(sb->buffer, oldval);

	edit_delete(sb->buffer, start, lim - start + 1);

	eb->removing = TRUE;
	
	if (call_callback && eb->callback != NULL)
		(*eb->callback)(eb, EDL_REMOVE);
	
	prev = get_prev_editblock(sb, eb);

	if (prev == NULL)
		sb->eblist = eb->next;
	else
		prev->next = eb->next;

	free(eb->text);
	free((char *)eb);
}

void
srcbuf_select_editblock(eb, selected)
Editblock *eb;
bool selected;
{
	if (eb->callback != NULL)
		(*eb->callback)(eb, selected ? EDL_SELECT : EDL_DESELECT);
}

void
srcbuf_accel_editblock_change(eb)
Editblock *eb;
{
	if (eb->callback != NULL)
		(*eb->callback)(eb, EDL_ACCELERATOR_ACTION );
}
	
	
static Editblock *
get_prev_editblock(sb, match_eb)
Srcbuf *sb;
Editblock *match_eb;
{
	Editblock *prev, *eb;

	prev = NULL;
	
	for (eb = sb->eblist; eb != match_eb; eb = eb->next) {
		if (eb == NULL)
			panic("eb list botch in gpe");
		prev = eb;
	}

	return prev;
}

static Editblock *
get_editblock_before_point(sb, point)
Srcbuf *sb;
size_t point;
{
	Editblock *eb, *prev;

	prev = NULL;
	
	for (eb = sb->eblist; eb != NULL; eb = eb->next) {
		if (edit_get_propchange_point(eb->startpc) >= point)
			break;
		prev = eb;
	}

	return prev;
}

const char *
srcbuf_get_file_line(sb, lnum)
Srcbuf *sb;
int lnum;
{
	static char *last = NULL;
	size_t start, lim;
	
	if (!srcbuf_file_lnum_to_point_range(sb, lnum - 1, &start, &lim))
		return "";
	
	if (last != NULL)
		free(last);

	last = edit_copy_bytes(srcbuf_get_buffer(sb), start, lim, (char *)NULL);

	return last;
}

void 
srcbuf_reset_file_handle()
{
  edit_reset_file_handle();
}
