/* props.c - maintain the edit buffer property change list */

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


/* @(#)props.c	1.4 09 Apr 1995 (UKC) */
char edit_props_c_rcsid[] = "$Id$";

#include <stdlib.h>
#include <time.h>

#include <local/ukcprog.h>

#include "edit.h"
#include "textbuf.h"
#include "display.h"
#include "props.h"

struct Proplist {
	Edit_propchange *pclist;
	
	Edit_propchange *new_propchanges[2];
	bool all_changes_are_additions;
	
	size_t start;
	size_t lim;
	bool updating;
	bool num_pending_changes;
};

typedef enum { EARLIER, SAME, LATER } Pcrel;

static Edit_propchange *make_pc PROTO((size_t point, char *user_data,
				       bool new_font, Edit_fontinfo *fi,
				       Edit_flags flagmask,
				       Edit_flags flags,
				       bool backsliding,
				       Edit_propchange *next));
static void handle_prop_change PROTO((Edit_buffer *buffer));
static void note_propchange PROTO((Edit_buffer *buffer, Edit_propchange *newpc, 
                                   size_t start, size_t lim));
static Edit_propchange *get_prev_propchange PROTO((Proplist *pl, 
                                                   Edit_propchange *pc));
static void redo_fontinfo_and_flags PROTO((Proplist *pl, 
					   Edit_propchange *prev));
static void free_pclist PROTO((Edit_propchange *list));
static void check_proplist_order PROTO((Proplist *pl));
static Pcrel relation_to_pc PROTO((size_t point, bool backsliding, 
                                   Edit_propchange *pc));


static Edit_propchange *
make_pc(point, user_data, new_font, fi, flagmask, flags, backsliding, next)
size_t point;
char *user_data;
bool new_font;
Edit_fontinfo *fi;
Edit_flags flagmask, flags;
bool backsliding;
Edit_propchange *next;
{
	Edit_propchange *pc;
	
	pc = (Edit_propchange *)e_malloc(sizeof(Edit_propchange));
	
	pc->point = point;
	pc->user_data = user_data;
	pc->new_font = new_font;
	pc->fi = fi;
	pc->flagmask = flagmask;
	pc->flags = flags;
	pc->backsliding = backsliding;
	pc->next = next;

	return pc;
}
	
Edit_buffer *
edit_create_buffer(tb, fi, flags)
Text_buffer *tb;
Edit_fontinfo *fi;
Edit_flags flags;
{
	Edit_buffer *buffer;
	Proplist *pl;
	Edit_propchange *pc;

	pc = make_pc(0, (char *)NULL, TRUE, fi, ~(Edit_flags)0, flags,
		     TRUE, (Edit_propchange *)NULL);

	pl = (Proplist *)e_malloc(sizeof(Proplist));

	pl->pclist = pc;
	pl->start = 0;		/* for neatness only */
	pl->lim = 0;		/* for neatness only */
	pl->updating = TRUE;
	pl->num_pending_changes = 0;
	pl->all_changes_are_additions = TRUE;

	buffer = (Edit_buffer *)e_malloc(sizeof(Edit_buffer));
	buffer->textbuf = tb;
	buffer->proplist = pl;

	return buffer;
}

void
edit_destroy_buffer(buffer)
Edit_buffer *buffer;
{
/*	Edit_display *d;*/

/* RGA comment out at present.
   When debugging a SunOS target without a PID, and having
   opened shared library files (for viewing or breakpoints), this is 
   called on the first attach from free_symtab_cache(). Change
   in lbuf_destroy() allows this to be OK 
*/

/*	for (d = edit__get_display_list(); d != NULL; d = d->next) {
		if (d->buffer == buffer)
			panic("edit_destroy_buffer called on displayed buffer");
	}*/

	(*buffer->textbuf->ops->destroy)(buffer->textbuf->bufdata);
	
	free_pclist(buffer->proplist->pclist);
	
	free((char *)buffer->proplist);

	free((char *)buffer);
}

Edit_fontinfo *
edit_make_fontinfo(font_handle, char_width_tab, line_height, baseline)
char *font_handle;
short *char_width_tab;
int line_height, baseline;
{
	Edit_fontinfo *fi;

	fi = (Edit_fontinfo *)e_malloc(sizeof(Edit_fontinfo));
	fi->font_handle = font_handle;
	fi->char_width_tab = char_width_tab;
	fi->height = line_height;
	fi->baseline = baseline;

	return fi;
}

bool
edit_set_propchange_updates(buffer, on)
Edit_buffer *buffer;
bool on;
{
	bool was_on;
	Proplist *pl;

	pl = buffer->proplist;
	
	was_on = pl->updating;

	if (on && !was_on) {
		if (pl->num_pending_changes != 0)
			handle_prop_change(buffer);
	}

	pl->updating = on;

	return was_on;
}

Edit_propchange *
edit_add_propchange(buffer, point, user_data, fi, flagmask, flags, backsliding)
Edit_buffer *buffer;
size_t point;
char *user_data;
Edit_fontinfo *fi;
Edit_flags flagmask, flags;
bool backsliding;
{
	Edit_propchange *pc, *last;
	
	last = NULL;		/* to satisfy gcc */

	pc = buffer->proplist->pclist;

	/* RGA For optimization, put relation_to_pc() logic in line here.
	   this used to be:

	while (pc != NULL &&
	        relation_to_pc(point, backsliding, pc) != EARLIER) {
	*/

	while (pc != NULL &&
	       (point > pc->point ||
		(point == pc->point && !(backsliding && !pc->backsliding)))) {
		last = pc;
		pc = pc->next;
	}

	last->next = make_pc(point, user_data,
			     fi != NULL,
			     (fi != NULL) ? fi : last->fi,
			     flagmask,
			     (last->flags & ~flagmask) | (flags & flagmask),
			     backsliding,
			     pc);
	pc = last->next;

	note_propchange(buffer, pc, pc->point,
			(pc->next != NULL) ? pc->next->point : EDIT_MAX_POINT);

	return pc;
}

static Edit_propchange *
get_prev_propchange(pl, pc)
Proplist *pl;
Edit_propchange *pc;
{
	Edit_propchange *last, *pc2;

	last = NULL;
	
	for (pc2 = pl->pclist; pc2 != pc; pc2 = pc2->next) {
		if (pc2 == NULL)
/*	RGA		panic("prop not found in edp");*/
			break;	/* RGA can happen when running with changed target */
		last = pc2;
	}

	if (last == NULL)
		panic("attempt to reference initial prop in edp");

	return last;
}
	
void
edit_delete_propchange(buffer, pc)
Edit_buffer *buffer;
Edit_propchange *pc;
{
	Proplist *pl;
	Edit_propchange *prev, *next;
	size_t point;

	pl = buffer->proplist;
	
	prev = get_prev_propchange(pl, pc);
	point = pc->point;

	next = pc->next;
	prev->next = next;

	free((char *)pc);
	
	note_propchange(buffer, (Edit_propchange *)NULL,
			point, (next != NULL) ? next->point : EDIT_MAX_POINT);
}

void
edit_move_propchange(buffer, pc, newpoint)
Edit_buffer *buffer;
Edit_propchange *pc;
size_t newpoint;
{
	size_t oldpoint;
	Edit_propchange *oldprev, *newprev, *last, *pc2;
	bool moving_forwards;

	oldpoint = pc->point;
	
	if (newpoint == oldpoint)
		return;

	moving_forwards = newpoint > oldpoint;

	oldprev = newprev = last = NULL;
	
	for (pc2 = buffer->proplist->pclist; pc2 != NULL; pc2 = pc2->next) {
		Pcrel rel;
		
		if (pc2 == pc) {
			oldprev = last;

			if (newprev != NULL)
				break;
		}

		rel = relation_to_pc(newpoint, pc->backsliding, pc2);

		/*  When moving a pc backwards it must land after any pc
		 *  at new location.  When going forwards it must end up
		 *  before any other pcs.  I.e., pcs only move past each
		 *  other if they have to.
		 */
		if (newprev == NULL &&
		    (rel == EARLIER || (rel == SAME && moving_forwards))) {
			newprev = last;

			if (oldprev != NULL)
				break;
		}
		
		last = pc2;
	}

	if (newprev == NULL)
		newprev = last;
	if (newprev == pc)
		newprev = oldprev;

	if (oldprev == NULL)
		panic("pc not found in empc");

	pc->point = newpoint;

	if (oldprev != newprev) {
		size_t old_nextpoint, new_nextpoint;
		
		old_nextpoint = (pc->next != NULL) ? pc->next->point
						   : EDIT_MAX_POINT;

		oldprev->next = pc->next;

		pc->next = newprev->next;
		newprev->next = pc;

		new_nextpoint = (pc->next != NULL) ? pc->next->point
						   : EDIT_MAX_POINT;

		if (newpoint < oldpoint) {
			note_propchange(buffer, (Edit_propchange *)NULL,
					newpoint, old_nextpoint);
		}
		else {
			note_propchange(buffer, (Edit_propchange *)NULL,
					oldpoint, new_nextpoint);
		}
	}
	else {
		if (newpoint < oldpoint) {
			note_propchange(buffer, (Edit_propchange *)NULL,
					newpoint, oldpoint);
		}
		else {
			note_propchange(buffer, (Edit_propchange *)NULL,
					oldpoint, newpoint);
		}
	}
}

static void
redo_fontinfo_and_flags(pl, prev)
Proplist *pl;
Edit_propchange *prev;
{
	Edit_propchange *pc;
	
	for (pc = prev->next; pc != NULL; pc = pc->next) {
		pc->fi = pc->new_font ? pc->fi : prev->fi;
		
		pc->flags = (prev->flags & ~pc->flagmask) |
			    (pc->flags & pc->flagmask);

		prev = pc;
	}
}

size_t
edit_get_propchange_point(pc)
Edit_propchange *pc;
{
	return pc->point;
}

void edit_move_propchange_point(pc, delta)
Edit_propchange *pc;
int delta;
{
    pc->point += delta;
}


char *
edit_get_propchange_data(pc)
Edit_propchange *pc;
{
	return pc->user_data;
}

void
edit_set_propchange_data(pc, data)
Edit_propchange *pc;
char *data;
{
	pc->user_data = data;
}

void
edit_get_propchange_info(pc, p_point, p_user_data, p_fontinfo, p_flags)
Edit_propchange *pc;
size_t *p_point;
char **p_user_data;
Edit_fontinfo **p_fontinfo;
Edit_flags *p_flags;
{
    /* RCB: Test for  NULL pointers */
    if ( p_point )
	*p_point = pc->point;
    if ( p_user_data )
	*p_user_data = pc->user_data;
    if (p_fontinfo)
	*p_fontinfo = pc->fi;
    if ( p_flags)
	*p_flags = pc->flags;
}

void
edit_change_propchange_info(pc, p_point, p_user_data, p_fontinfo, p_flags)
Edit_propchange *pc;
size_t *p_point;
char **p_user_data;
Edit_fontinfo **p_fontinfo;
Edit_flags *p_flags;
{
    if ( p_point )
	pc->point = *p_point;
    if ( p_user_data )
	pc->user_data = *p_user_data;
    if (p_fontinfo)
	pc->fi = *p_fontinfo;
    if ( p_flags)
	pc->flags = *p_flags;
}

static void
handle_prop_change(buffer)
Edit_buffer *buffer;
{
	Proplist *pl;
	Edit_display *d;

	pl = buffer->proplist;
	
	if (pl->num_pending_changes == 2 && pl->all_changes_are_additions) {
		Edit_propchange *pc1, *pc2;

		pc1 = pl->new_propchanges[0];
		pc2 = pl->new_propchanges[1];

		if (pc1->point > pc2->point) {
			Edit_propchange *tmp;

			tmp = pc1;
			pc1 = pc2;
			pc2 = tmp;
		}

		if (pc1->next == pc2) {
			Edit_propchange *last;
			
			last = get_prev_propchange(pl, pc1);

			if (pc2->fi == last->fi && pc2->flags == last->flags) {
				pl->start = pc1->point;
				pl->lim = pc2->point;
			}
		}
	}

	redo_fontinfo_and_flags(pl, pl->pclist);
	
	for (d = edit__get_display_list(); d != NULL; d = d->next) {
		if (d->buffer == buffer)
			(*d->display_ops->handle_prop_change)(d,
							      pl->start,
							      pl->lim);
	}

	pl->all_changes_are_additions = TRUE;
	pl->num_pending_changes = 0;

/*	check_proplist_order(pl);*/
}

static void
free_pclist(list)
Edit_propchange *list;
{
	Edit_propchange *pc, *next;
	
	for (pc = list; pc != NULL; pc = next) {
		next = pc->next;
		pc->fi = NULL;	    /* cause SEGV on attempt to use freed pc */
		free((char *)pc);
	}
}

static void
note_propchange(buffer, newpc, start, lim)
Edit_buffer *buffer;
Edit_propchange *newpc;
size_t start, lim;
{
	Proplist *pl;

	pl = buffer->proplist;
	
	if (newpc == NULL) {
		pl->all_changes_are_additions = FALSE;
	}
	else {
		if (pl->num_pending_changes < 2)
			pl->new_propchanges[pl->num_pending_changes] = newpc;
	}

	if (pl->num_pending_changes != 0) {
		if (start < pl->start)
			pl->start = start;
		if (lim > pl->lim)
			pl->lim = lim;
	}
	else {
		pl->start = start;
		pl->lim = lim;
	}

	++pl->num_pending_changes;
	
	if (pl->updating)
		handle_prop_change(buffer);
}

void
edit__update_proplist_points(pl, pos, delta)
Proplist *pl;
size_t pos;
long delta;
{
	Edit_propchange *pc;
	
	for (pc = pl->pclist->next; pc != NULL; pc = pc->next) {
		if (pc->backsliding && pos == pc->point && delta > 0)
			continue;
		
		if (pc->point >= pos) {
			if (delta < 0 && pc->point < pos + -delta)
				pc->point = pos;
			else
				pc->point += delta;
		}
	}

/*	check_proplist_order(pl);*/
}

static Pcrel
relation_to_pc(point, backsliding, pc)
size_t point;
bool backsliding;
Edit_propchange *pc;
{
	if (point < pc->point)
		return EARLIER;
	if (point > pc->point)
		return LATER;

	if (backsliding && !pc->backsliding)
		return EARLIER;
	if (!backsliding && pc->backsliding)
		return LATER;

	return SAME;
}
	
static void
check_proplist_order(pl)
Proplist *pl;
{
	size_t last_point;
	bool last_backsliding;
	Edit_propchange *pc;

	last_point = 0;
	last_backsliding = TRUE;

	for (pc = pl->pclist; pc != NULL; pc = pc->next) {
/*		if (relation_to_pc(last_point, last_backsliding, pc) == LATER)
			panic("proplist order botch in cpo");
 RGA seems OK when called from display_area_overlay_delete_all_text */

		last_point = pc->point;
		last_backsliding = pc->backsliding;
	}
}

Edit_propchange *
edit_get_propchange_at_point(buffer, point)
Edit_buffer *buffer;
size_t point;
{
	size_t junk_next_pc_point;
	
	return edit_get_next_propchange(buffer,
					(Edit_propchange *)NULL, point,
					&junk_next_pc_point);
}

Edit_propchange *
edit_get_next_propchange(buffer, pc, point, p_next_pc_point)
Edit_buffer *buffer;
size_t point;
Edit_propchange *pc;
size_t *p_next_pc_point;
{
	Edit_propchange *nextpc;

	if (pc == NULL)
		pc = buffer->proplist->pclist;
	
	for (;;) {
		nextpc = pc->next;

		if (nextpc == NULL || nextpc->point > point)
			break;

		pc = nextpc;
	}
	
	*p_next_pc_point = (pc->next != NULL) ? pc->next->point : EDIT_MAX_POINT;

	return pc;
}
