/* sline.c - the EDIT_FOLDED_LINES display type */

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


/* @(#)sline.c	1.2 09 Sep 1994 (UKC) */
char edit_sline_c_rcsid[] = "$Id$";

#include <stdlib.h>
#include <local/ukcprog.h>

#include "edit.h"
#include "display.h"
#include "props.h"
#include "render.h"

typedef struct {
	Lmap start;
	Lmap lim;
	int ystart;
} Sl_state;

#define MORE_WIDTH           2	/* pixels */
#define MORE_LMARGIN         2	/* pixels */
#define MORE_VMARGIN         1	/* pixels */

static void sl_redraw_display PROTO((Edit_display *d));
static void sl_display_from PROTO((Edit_display *d,
				   int y_offset, size_t point));
static bool sl_visible PROTO((Edit_display *d, size_t point));
static bool sl_pixel_to_point PROTO((Edit_display *d, int x, int y, 
                                     size_t *p_point));
static void sl_draw_cursor PROTO((Edit_display *d, size_t point, bool on));
static void sl_redraw_with_point_visible PROTO((Edit_display *d, size_t point));
static bool sl_update_display_size PROTO((Edit_display *d, int width,
					  int height));
static void sl_get_display_info PROTO((Edit_display *d, int *p_pixel_offset, 
				       size_t *p_start_point, 
				       size_t *p_lim_point));
static bool sl_point_to_pixel PROTO((Edit_display *d, size_t point, int *p_x, 
                                     int *p_y, int *p_width, int *p_height,
				     int *p_baseline));
static void sl_note_new_buffer PROTO((Edit_display *d));
static void forget_render_state PROTO((Sl_state *sl));
static void set_lmap PROTO((Lmap *lm, size_t point));
static bool visible PROTO((Edit_display *d, size_t point,
			   Lmap *start, Lmap *lim));
static void sl_handle_change PROTO((Edit_display *d, size_t pos, size_t len));
static void sl_destroy PROTO((Edit_display *d));

static void
sl_redraw_display(d)
Edit_display *d;
{
	Sl_state *sl = (Sl_state *)d->display_data;

	edit__free_rdesc(&sl->start);
	edit__free_rdesc(&sl->lim);

	sl_display_from(d, 0, sl->start.point);
}

static void
sl_get_display_info(d, p_pixel_offset, p_start_point, p_lim_point)
Edit_display *d;
int *p_pixel_offset;
size_t *p_start_point, *p_lim_point;
{
	Sl_state *sl = (Sl_state *)d->display_data;

	*p_pixel_offset = 0;
	*p_start_point = sl->start.point;
	*p_lim_point = sl->lim.point;
}

static void
sl_display_from(d, x_offset, point)
Edit_display *d;
int x_offset;
size_t point;
{
	Sl_state *sl = (Sl_state *)d->display_data;
	Lmap old_start;
	int new_ystart;

	edit__init_rdesc(&old_start);
	edit__copy_lmap(&old_start, &sl->start);
	
	set_lmap(&sl->start, point);
	edit__convert_line(d, &sl->start, &sl->lim, (Edit_propchange **)NULL);

	new_ystart = (d->display_height - sl->start.height) / 2;

	edit__render_line(d, &sl->start, new_ystart,
			  (new_ystart == sl->ystart) ? &old_start : NULL);

	sl->ystart = new_ystart;
	edit__free_rdesc(&old_start);
}

static void
set_lmap(lm, point)
Lmap *lm;
size_t point;
{
	edit__set_lmap(lm, point);

	/*  We don't expand tabs, so set column to let convert_line() know
	 *  whether or not we need a start of line continuation marker.
	 */
	lm->column = lm->point;
}

static bool
sl_visible(d, point)
Edit_display *d;
size_t point;
{
	Sl_state *sl = (Sl_state *)d->display_data;

	return visible(d, point, &sl->start, &sl->lim);
}

static bool
visible(d, point, start, lim)
Edit_display *d;
size_t point;
Lmap *start, *lim;
{
	size_t buflen;

	buflen = edit_get_buffer_length(d->buffer);

	/*  Nothing is visible if we have not done a render yet.
	 */
	if (start->rdesc == NULL)
		return FALSE;

	if (point != buflen || lim->point != buflen)
		return point >= start->point && point < lim->point;

	return edit__extra_char_would_fit(d, start, ' ');
}

static bool
sl_pixel_to_point(d, x, y, p_point)
Edit_display *d;
int x, y;
size_t *p_point;
{
	Sl_state *sl = (Sl_state *)d->display_data;

	return y > 0 && y <= d->display_height &&
	       edit__xpos_to_point(d, &sl->start, x, p_point);
}

static bool
sl_point_to_pixel(d, point, p_x, p_y, p_width, p_height, p_baseline)
Edit_display *d;
size_t point;
int *p_x, *p_y, *p_width, *p_height, *p_baseline;
{
	Sl_state *sl = (Sl_state *)d->display_data;

	if (edit__lmap_point_to_pixel(d, &sl->start, point, p_x, p_width,
				      p_height, p_baseline)) {
		*p_y = sl->ystart;
		return TRUE;
	}

	return FALSE;
}

static void
sl_draw_cursor(d, point, on)
Edit_display *d;
size_t point;
bool on;
{
	Sl_state *sl = (Sl_state *)d->display_data;
	
	edit__draw_lmap_cursor(d, &sl->start, point, sl->ystart, on);
}

static void
sl_handle_change(d, pos, len)
Edit_display *d;
size_t pos, len;
{
	sl_display_from(d, 0, ((Sl_state *)d->display_data)->start.point);
}
	
static void
sl_note_new_buffer(d)
Edit_display *d;
{
	forget_render_state((Sl_state *)d->display_data);
}

static void
sl_redraw_with_point_visible(d, point)
Edit_display *d;
size_t point;
{
	size_t start;
	Lmap lmap, next_lmap;
	
	/*  First see if we can fit the whole string in the display area.
	 */
	edit__init_rdesc(&lmap);
	edit__init_rdesc(&next_lmap);
	set_lmap(&lmap, 0);
	edit__convert_line(d, &lmap, &next_lmap, (Edit_propchange **)NULL);

	if (visible(d, point, &lmap, &next_lmap)) {
		start = 0;
	}
	else {
		int old_display_width;
	
		old_display_width = d->display_width;
		d->display_width /= 2;

		for (start = point; start > 0; --start) {
			set_lmap(&lmap, start);
		
			edit__convert_line(d, &lmap, &next_lmap,
					   (Edit_propchange **)NULL);

			if (next_lmap.point < point &&
			    !next_lmap.line_continues)
				break;
		}

		d->display_width = old_display_width;
	}

	edit__free_rdesc(&lmap);
	edit__free_rdesc(&next_lmap);

	sl_display_from(d, 0, start);
}

static bool
sl_update_display_size(d, width, height)
Edit_display *d;
int width, height;
{
	forget_render_state((Sl_state *)d->display_data);
	return TRUE;
}

static void
forget_render_state(sl)
Sl_state *sl;
{
	edit__free_rdesc(&sl->start);
	edit__free_rdesc(&sl->lim);
	sl->ystart = 0;
}

static void
sl_destroy(d)
Edit_display *d;
{
	Sl_state *sl = (Sl_state *)d->display_data;

	forget_render_state(sl);
	free((char *)d->render_context);
	free((char *)sl);
}

void
edit_use_single_line(d)
Edit_display *d;
{
	static Display_ops sl_display_ops = {
		sl_visible,
		sl_redraw_with_point_visible,
		sl_draw_cursor,
		
		sl_get_display_info,
		
		NULL,
		sl_redraw_display,
		sl_display_from,
		
		NULL,
		sl_handle_change,
		NULL,
		sl_handle_change,
		
		sl_update_display_size,
		sl_note_new_buffer,
		
		sl_handle_change,
		
		sl_point_to_pixel,
		sl_pixel_to_point,

		sl_destroy,
	};
	Sl_state *sl;
	
	sl = (Sl_state *)e_malloc(sizeof(Sl_state));
	edit__init_rdesc(&sl->start);
	edit__init_rdesc(&sl->lim);
	
	d->display_ops = &sl_display_ops;
	d->display_data = (char *)sl;
	
	d->render_context = edit__make_render_context(0, FALSE, TRUE,
						      MORE_WIDTH,
						      MORE_LMARGIN,
						      MORE_WIDTH,
						      MORE_LMARGIN,
						      MORE_VMARGIN);
}
