/* edit.c - do emacs like editing */

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


/* @(#)edit.c	1.8 09 Apr 1995 (UKC) */
char edit_edit_c_rcsid[] = "$Id$";

#include <time.h>
#include <ctype.h>
#include <local/ukcprog.h>
#include <local/wn.h>

#include "edit.h"
#include "lbuf.h"
#include "textbuf.h"
#include "display.h"
#include "win.h"

typedef struct {
	Text_buffer textbuf;
	size_t start;
	size_t lim;
	const char *buf;
	size_t pos;
} Epos;

static void init_epos PROTO((Epos *ep, Edit_buffer *buffer, size_t pos));
static bool nextc PROTO((Epos *ep, int *p_ch));
static bool prevc PROTO((Epos *ep, int *p_ch));
static size_t start_of_line PROTO((Edit_buffer *buffer, size_t start, 
                                   size_t lim, size_t pos, int offset));
static size_t end_of_line PROTO((Edit_buffer *buffer, size_t lim, size_t pos));
static bool check_not_at_end PROTO((Edit_buffer *buffer, size_t lim,
				    size_t point));
static bool check_not_at_start PROTO((size_t start, size_t point));
static size_t to_goal PROTO((Edit_buffer *buffer, size_t lim, size_t pos, 
                             size_t goal_column));
static size_t get_kill_line_endpos PROTO((Edit_buffer *buffer, size_t lim,
					  size_t pos));
static size_t get_kill_word_endpos PROTO((Edit_buffer *buffer, size_t lim, 
                                          size_t pos));
static size_t get_backward_kill_word_pos PROTO((Edit_buffer *buffer, 
                                                size_t start, size_t pos));
static size_t lim_point PROTO((Edit_display *d));
static size_t start_point PROTO((Edit_display *d));
static void move_point PROTO((Edit_display *d, size_t point));
static void insert PROTO((Edit_display *d, size_t point, const char *text, 
                          size_t len));
static void delete PROTO((Edit_display *d, size_t point, size_t len));

static void
init_epos(ep, buffer, pos)
Epos *ep;
Edit_buffer *buffer;
size_t pos;
{
	ep->textbuf = *buffer->textbuf;
	ep->start = 0;
	ep->lim = 0;
	ep->buf = NULL;
	ep->pos = pos;
}

static bool
nextc(ep, p_ch)
Epos *ep;
int *p_ch;
{
	if (ep->pos < ep->start || ep->pos >= ep->lim) {
		const char *ilim;
		
		if (!(ep->textbuf.ops->get_bytes)(ep->textbuf.bufdata, ep->pos,
						  &ep->buf, &ilim))
			return FALSE;

		ep->start = ep->pos;
		ep->lim = ep->pos + (ilim - ep->buf);

	}
							 
	*p_ch = ep->buf[ep->pos++ - ep->start];
	return TRUE;
}

static bool
prevc(ep, p_ch)
Epos *ep;
int *p_ch;
{
	if (ep->pos <= ep->start || ep->pos > ep->lim) {
		const char *ilim;
		
		if (!(ep->textbuf.ops->get_bytes_before)(ep->textbuf.bufdata,
							 ep->pos,
							 &ep->buf, &ilim))
			return FALSE;

		ep->start = ep->pos - (ilim - ep->buf);
		ep->lim = ep->pos;

	}
							 
	*p_ch = ep->buf[--ep->pos - ep->start];
	return TRUE;
}

/*  Return the position of the start of the line COUNT lines before or
 *  after the one at pos.
 */
static size_t
start_of_line(buffer, start, lim, pos, offset)
Edit_buffer *buffer;
size_t start, lim, pos;
int offset;
{
	int i;

	if (offset <= 0) {
		for (i = 0; i <= -offset; ++i) {
			if (!edit_find_char_backwards(buffer, pos, start,
						      "\n", &pos))
				return pos;
		}
		++pos;
	}
	else {
		for (i = 0; i < offset; ++i) {
			if (!edit_find_char_forwards(buffer, pos,
						     lim, "\n", &pos))
				break;
			++pos;
		}
	}
	
	return pos;
}

void
edit_get_buffer_modtime(buffer, mtime)
     Edit_buffer *buffer;
     long *mtime;
{
  text_get_buffer_modtime(buffer->textbuf, mtime);
}

bool
edit_get_bytes(buffer, pos, p_line, p_lim)
Edit_buffer *buffer;
size_t pos;
const char **p_line, **p_lim;
{
	return (*buffer->textbuf->ops->get_bytes)(buffer->textbuf->bufdata,
						  pos, p_line, p_lim);
}

bool
edit_get_bytes_before(buffer, pos, p_line, p_lim)
Edit_buffer *buffer;
size_t pos;
const char **p_line, **p_lim;
{
	return (*buffer->textbuf->ops->get_bytes_before)(
						  buffer->textbuf->bufdata,
						  pos, p_line, p_lim);
}

size_t
edit_get_buffer_length(buffer)
Edit_buffer *buffer;
{
	return (*buffer->textbuf->ops->get_length)(buffer->textbuf->bufdata);
}

static size_t
end_of_line(buffer, lim, pos)
Edit_buffer *buffer;
size_t lim, pos;
{
	edit_find_char_forwards(buffer, pos, lim, "\n", &pos);
	return pos;
}

static bool
check_not_at_end(buffer, lim, point)
Edit_buffer *buffer;
size_t lim, point;
{
	if (point >= lim) {
		if (lim == edit_get_buffer_length(buffer))
			errf("End of buffer");
		else
			errf("End of editable text");
	
		return FALSE;
	}

	return TRUE;
}

static bool
check_not_at_start(start, point)
size_t start, point;
{
	if (point == start) {
		errf("Start of %s", (start == 0) ? "buffer" : "editable text");
		return FALSE;
	}

	return TRUE;
}

static size_t
to_goal(buffer, lim, pos, goal_column)
Edit_buffer *buffer;
size_t lim, pos, goal_column;
{
	size_t maxpos;

	maxpos = end_of_line(buffer, lim, pos);
	pos += goal_column;

	return (pos > maxpos) ? maxpos : pos;
}

static size_t
get_kill_line_endpos(buffer, lim, pos)
Edit_buffer *buffer;
size_t lim, pos;
{
	Epos epos;
	int ch;
	bool seen_nonspace;
	
	init_epos(&epos, buffer, pos);
	seen_nonspace = FALSE;

	while (epos.pos < lim && nextc(&epos, &ch)) {
		if (!isspace(ch))
			seen_nonspace = TRUE;

		if (ch == '\n') {
			if (seen_nonspace)
				--epos.pos;
			break;
		}
	}

	return epos.pos;
}
			
static size_t
get_kill_word_endpos(buffer, lim, pos)
Edit_buffer *buffer;
size_t lim, pos;
{
	Epos epos;
	int ch;
	bool seen_wordc;
	
	init_epos(&epos, buffer, pos);
	seen_wordc = FALSE;

	while (epos.pos < lim && nextc(&epos, &ch)) {
		bool isword;
		
		isword = isalnum(ch) || ch == '_';

		if (isword && !seen_wordc) {
			seen_wordc = TRUE;
		}
		else if (!isword && seen_wordc) {
			break;
		}
	}

	return epos.pos;
}
			
static size_t
get_backward_kill_word_pos(buffer, start, pos)
Edit_buffer *buffer;
size_t start, pos;
{
	Epos epos;
	int ch;
	bool seen_wordc;
	
	init_epos(&epos, buffer, pos);
	seen_wordc = FALSE;

	while (epos.pos > start && prevc(&epos, &ch)) {
		bool isword;
		
		isword = isalnum(ch) || ch == '_';

		if (isword && !seen_wordc) {
			seen_wordc = TRUE;
		}
		else if (!isword && seen_wordc) {
			++epos.pos;
			break;
		}
	}

	return epos.pos;
}
			
void
edit_handle_key_event(d, key, modifiers)
Edit_display *d;
int key;
unsigned modifiers;
{
	d->last_modifiers = modifiers;
	d->last_key = key;
	d->quit_requested = FALSE;
	
	(*edit_lookup_key(d->keymap, modifiers, key))(d);
}

bool
edit_quit_requested(d)
Edit_display *d;
{
	return d->quit_requested;
}

size_t
edit_restrict_point(d, point)
Edit_display *d;
size_t point;
{
	if (point < d->start)
		point = d->start;

	if (point > d->lim)
		point = d->lim;

	return point;
}

static size_t
lim_point(d)
Edit_display *d;
{
	return edit_restrict_point(d, EDIT_MAX_POINT);
}

static size_t
start_point(d)
Edit_display *d;
{
	return edit_restrict_point(d, 0);
}

static void
move_point(d, point)
Edit_display *d;
size_t point;
{
	edit_move_point(d, point);
	edit_set_goal_column(d, d->point);
}

static void
insert(d, point, text, len)
Edit_display *d;
size_t point;
const char *text;
size_t len;
{
	if (edit_insert(d->buffer, point, text, len, TRUE))
		edit_set_goal_column(d, d->point);
}

static void
delete(d, point, len)
Edit_display *d;
size_t point;
size_t len;
{
	if (edit_delete(d->buffer, point, len))
		edit_set_goal_column(d, d->point);
}

void
edit_set_goal_column(d, point)
Edit_display *d;
size_t point;
{
	d->goal_column = d->point - start_of_line(d->buffer, start_point(d),
						  lim_point(d), d->point, 0);
}

void
edit_f_end_of_buffer(d)
Edit_display *d;
{
	move_point(d, lim_point(d));
}

void
edit_f_beginning_of_buffer(d)
Edit_display *d;
{
	move_point(d, start_point(d));
}

void
edit_f_beginning_of_line(d)
Edit_display *d;
{
	move_point(d,
		   start_of_line(d->buffer, start_point(d), lim_point(d),
				 d->point, 0));
}

void
edit_f_end_of_line(d)
Edit_display *d;
{
	move_point(d, end_of_line(d->buffer, lim_point(d), d->point));
}

void
edit_f_forward_char(d)
Edit_display *d;
{
	if (check_not_at_end(d->buffer, lim_point(d), d->point))
		move_point(d, d->point + 1);
}

void
edit_f_backward_char(d)
Edit_display *d;
{
	if (check_not_at_start(start_point(d), d->point))
		move_point(d, d->point - 1);
}

void
edit_f_delete_char(d)
Edit_display *d;
{
	if (check_not_at_end(d->buffer, lim_point(d), d->point))
		delete(d, d->point, 1);
}

void
edit_f_kill_line(d)
Edit_display *d;
{
	delete(d, d->point,
	       get_kill_line_endpos(d->buffer, lim_point(d), d->point) -
	       d->point);
}

void
edit_f_backward_kill_line(d)
Edit_display *d;
{
	size_t sol;

	sol = start_of_line(d->buffer, start_point(d), lim_point(d),
			    d->point, 0);

	delete(d, sol, d->point - sol);
}

void
edit_f_kill_word(d)
Edit_display *d;
{
	size_t endpos;

	endpos = get_kill_word_endpos(d->buffer, lim_point(d), d->point);
	delete(d, d->point, endpos - d->point);
}

void
edit_f_backward_word(d)
Edit_display *d;
{
	move_point(d,
		   get_backward_kill_word_pos(d->buffer, start_point(d),
					      d->point));
}

void
edit_f_back_to_indentation(d)
Edit_display *d;
{
	size_t point, sol, lim;

	lim = lim_point(d);
	sol = start_of_line(d->buffer, start_point(d), lim, d->point, 0);
	edit_find_char_forwards(d->buffer, sol, lim, "! \t", &point);
	move_point(d, point);
}

void
edit_f_delete_horizontal_space(d)
Edit_display *d;
{
	size_t start, lim;
	
	if (edit_find_char_backwards(d->buffer, d->point, start_point(d),
				     "! \t", &start))
		++start;
	
	edit_find_char_forwards(d->buffer, d->point, lim_point(d), "! \t",
				&lim);

	if (start != lim)
		delete(d, start, lim - start);
}

void
edit_f_forward_word(d)
Edit_display *d;
{
	move_point(d, get_kill_word_endpos(d->buffer, lim_point(d), d->point));
}

void
edit_f_backward_kill_word(d)
Edit_display *d;
{
	size_t point;
	
	point = get_backward_kill_word_pos(d->buffer, start_point(d), d->point);
	delete(d, point, d->point - point);
}

void
edit_f_delete_backward_char(d)
Edit_display *d;
{
	if (check_not_at_start(start_point(d), d->point))
		delete(d, d->point - 1, 1);
}

void
edit_f_previous_line(d)
Edit_display *d;
{
	size_t solpos;
	
	solpos = start_of_line(d->buffer, start_point(d), lim_point(d),
			       d->point, -1);
	edit_move_point(d, to_goal(d->buffer, lim_point(d),
				   solpos, d->goal_column));
}

void
edit_f_next_line(d)
Edit_display *d;
{
	size_t solpos;
	
	solpos = start_of_line(d->buffer, start_point(d), lim_point(d),
			       d->point, 1);
	edit_move_point(d, to_goal(d->buffer, lim_point(d),
				   solpos, d->goal_column));
}

void
edit_f_previous_line_or_history_item(d)
Edit_display *d;
{
	/* Determine if there is more than one line */
	size_t start = start_point(d);
	size_t sol = start_of_line(d->buffer, start, lim_point(d), d->point, 0);
	size_t lim = lim_point(d);
	size_t solnext = start_of_line(d->buffer, start_point(d), lim, d->point, 1);
	if ( sol != start || solnext < lim )
	    edit_f_previous_line(d);
	else
	    edit_scroll_history(d, 1);
}

void
edit_f_next_line_or_history_item(d)
Edit_display *d;
{
	/* Determine if there is more than one line */
	size_t start = start_point(d);
	size_t sol = start_of_line(d->buffer, start, lim_point(d), d->point, 0);
	size_t lim = lim_point(d);
	size_t solnext = start_of_line(d->buffer, start_point(d), lim, d->point, 1);
	if ( sol != start || solnext < lim )
	    edit_f_next_line(d);
	else
	    edit_scroll_history(d, -1);
}

void
edit_f_newline(d)
Edit_display *d;
{
	insert(d, d->point, "\n", 1);
}

void
edit_f_indent_for_tab_command(d)
Edit_display *d;
{
	insert(d, d->point, "\t", 1);
}
		
void
edit_f_scroll_up(d)
Edit_display *d;
{
	edit_scroll_display(d, d->display_height / 2);
}

void
edit_f_scroll_down(d)
Edit_display *d;
{
	edit_scroll_display(d, -(d->display_height / 2));
}

void
edit_f_set_mark_command(d)
Edit_display *d;
{
	edit_set_mark(d, d->point);
}

void
edit_f_kill_region(d)
Edit_display *d;
{
	size_t start, lim;

	start = d->point;
	lim = d->mark;

	if (start > lim) {
		size_t tmp;

		tmp = start;
		start = lim;
		lim = tmp;
	}

	start = edit_restrict_point(d, start);
	lim = edit_restrict_point(d, lim);
	
	delete(d, start, lim - start);
}

void
edit_f_request_quit(d)
Edit_display *d;
{
	d->quit_requested = TRUE;
}
		
void
edit_f_self_insert(d)
Edit_display *d;
{
	char c;

	c = d->last_key;

	/*  Can't self insert characters which don't fit in a byte.
	 *  We should probably have explicit keymap entries for the
	 *  characters which do self-insert, but this way is less work.
	 */
	if (c != d->last_key) {
		edit_f_undef(d);
	}
	else {
		/*  This stuff is shamelessly ASCII dependent.
		 */
		
		if ((d->last_modifiers & EDIT_SHIFT) != 0)
			c = toupper(c);
		
		if ((d->last_modifiers & EDIT_CONTROL) != 0)
			c &= ~('A' - 1);

		if ((d->last_modifiers & EDIT_ALT) != 0)
			c |= 0x80;
		
		insert(d, d->point, &c, 1);
	}
}

void
edit_f_yank(d)
Edit_display *d;
{
	if (d->render_ops->get_selection != NULL) {
		const char *selbytes;
		size_t selcount;
			
		if ((*d->render_ops->get_selection)(d->render_data,
						    &selbytes,
						    &selcount)) {
			/***
			insert(d, d->point, selbytes, selcount);
			***/
			/*  RCB: Pass the characters one at a time
			**  for proper processing.  In particular, if
			**  a newline is pasted, it should normally
			**  terminate the edit.  Pasting a newline is
			**  usually accidental, and results in great
			**  confusion in one line displays.
			*/
			static event_t evbuf;
			evbuf.ev_type = EV_KEY;
			while (!d->quit_requested && selcount--)
			{
			    evbuf.ev_char = *selbytes++;
			    edit_handle_wn_key_event(d, &evbuf);
			}
		}
	}
}

void
edit_f_undef(d)
Edit_display *d;
{
	errf("Undefined key");
}
