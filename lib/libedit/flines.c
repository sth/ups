/* flines.c - the EDIT_FOLDED_LINES display type */

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


/* @(#)flines.c	1.5 09 Apr 1995 (UKC) */
char edit_flines_c_rcsid[] = "$Id$";

#include <stdlib.h>
#include <string.h>

#include <local/ukcprog.h>

#include "edit.h"
#include "display.h"
#include "props.h"
#include "render.h"

typedef struct {
	Lmap *lmap;		/* Display line table: 0..num_text_lines */
	int lmap_size;
	int y_offset;		/* Height of blank strip at top of display */
	int num_text_lines;	/* Number lines of display containing text */

	bool change_affects_display;
	bool do_full_repaint;
	bool cursor_was_visible;
	int start_lnum;
	int start_ypos;

	bool want_check_after_updates;
} Fl_state;

#define RMORE_WIDTH           2	/* pixels */
#define RMORE_LMARGIN         2	/* pixels */
#define RMORE_VMARGIN         1	/* pixels */
#define TABSPACES		8	/* characters */

static void fl_redraw_display PROTO((Edit_display *d));
static void fl_display_from PROTO((Edit_display *d,
				   int y_offset, size_t point));
static void fill_lines PROTO((Edit_display *d, Lmap *lm, int start_lnum,
			      int ypos, int ylim, size_t endpos,
			      int *p_lnum, int *p_ypos));
static void go_backwards PROTO((Edit_display *d, Lmap *lm, int *p_nlines, 
                                int *p_height));
static void back_up PROTO((Edit_display *d, Lmap *lm, int delta, int *p_delta, 
                           int *p_nlines, int *p_new_y_offset));
static int scroll_backwards PROTO((Edit_display *d, int delta));
static void fill_display PROTO((Edit_display *d, Lmap *lm, int start_lnum, 
                                int ypos));
static int scroll_forwards PROTO((Edit_display *d, int delta));
static void change_y_offset PROTO((Edit_display *d, int new_y_offset));
static bool fl_visible PROTO((Edit_display *d, size_t point));
static bool point_before_lmap PROTO((size_t point, Lmap *lm));
static bool in_line_range PROTO((Edit_display *d, size_t point, Lmap *start, 
                                 Lmap *lim));
static bool point_to_lnum PROTO((Edit_display *d, size_t point, int *p_lnum, 
                                 int *p_ypos));
static bool fl_pixel_to_point PROTO((Edit_display *d, int x, int y, 
                                     size_t *p_point));
static void fl_draw_cursor PROTO((Edit_display *d, size_t point, bool on));
static int fl_scroll_display PROTO((Edit_display *d, int delta));
static void fl_about_to_delete PROTO((Edit_display *d, size_t pos, size_t len));
static void fl_about_to_insert PROTO((Edit_display *d, size_t pos, size_t len));
static void prepare_for_change PROTO((Edit_display *d, size_t startpos, 
                                      size_t limpos));
static void fl_done_delete PROTO((Edit_display *d, size_t startpos, 
                                  size_t len));
static void fl_done_insert PROTO((Edit_display *d, size_t pos, size_t len));
static void handle_change PROTO((Edit_display *d, size_t pos, size_t oldlen, 
                                 size_t newlen));
static void fl_handle_prop_change PROTO((Edit_display *d, size_t start, 
                                         size_t lim));
static void check_lmap PROTO((Edit_display *d));
static void check_lmaps_same PROTO((Lmap *lm1, Lmap *lm2, bool is_last_lmap));
static void note_change PROTO((Edit_display *d, size_t pos, long delta));
static void expand_lmap PROTO((Fl_state *fs, int count));
static void make_lmap PROTO((Edit_display *d, size_t point, Lmap *lm));
static void fl_redraw_with_point_visible PROTO((Edit_display *d, size_t point));
static bool fl_update_display_size PROTO((Edit_display *d, int width, 
                                          int height));
static void scroll_lines PROTO((Edit_display *d,
				int old_scroll_start_lnum, 
				int new_scroll_start_lnum, 
				int old_scroll_start_ypos, 
				int new_scroll_start_ypos, 
				Lmap *repaint_start_lm,
				int repaint_start_lnum,
				int repaint_start_ypos));
static void fl_handle_prop_change PROTO((Edit_display *d, size_t start, 
                                         size_t lim));
static void fl_get_display_info PROTO((Edit_display *d, int *p_pixel_offset, 
				       size_t *p_start_point, 
				       size_t *p_lim_point));
static bool fl_point_to_pixel PROTO((Edit_display *d, size_t point, int *p_x, 
                                     int *p_y, int *p_width, int *p_height,
				     int *p_baseline));
static void fl_note_new_buffer PROTO((Edit_display *d));
static bool no_final_newline PROTO((Edit_buffer *buffer, size_t buflen));
static void force_redraw PROTO((Fl_state *fs, int start_lnum, int lim_lnum));
static void fl_destroy PROTO((Edit_display *d));

static void
fl_redraw_display(d)
Edit_display *d;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	Lmap lmap;
		
	force_redraw(fs, 0, fs->num_text_lines + 1);
		
	edit__init_rdesc(&lmap);
	edit__copy_lmap(&lmap, &fs->lmap[0]);
	fill_display(d, &lmap, 0, fs->y_offset);
	edit__free_rdesc(&lmap);
	
}

static void
fl_get_display_info(d, p_pixel_offset, p_start_point, p_lim_point)
Edit_display *d;
int *p_pixel_offset;
size_t *p_start_point, *p_lim_point;
{
	Fl_state *fs = (Fl_state *)d->display_data;

	*p_pixel_offset = fs->y_offset;
	*p_start_point= fs->lmap[0].point;
	*p_lim_point = fs->lmap[fs->num_text_lines].point;
}

static void
fl_display_from(d, y_offset, point)
Edit_display *d;
int y_offset;
size_t point;
{
	Lmap lmap;

	edit__init_rdesc(&lmap);
	make_lmap(d, point, &lmap);

	fill_display(d, &lmap, 0, y_offset);
	edit__free_rdesc(&lmap);

	change_y_offset(d, y_offset);
}

static void
force_redraw(fs, start_lnum, lim_lnum)
Fl_state *fs;
int start_lnum, lim_lnum;
{
	int lnum;
	
	for (lnum = start_lnum; lnum < lim_lnum; ++lnum)
		edit__free_rdesc(&fs->lmap[lnum]);
}
		
static void
fill_lines(d, lm, start_lnum, ypos, ylim, endpos, p_lnum, p_ypos)
Edit_display *d;
Lmap *lm;
int start_lnum, ypos, ylim;
size_t endpos;
int *p_lnum, *p_ypos;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	int lnum, old_ypos;
	Edit_propchange *pc;

	old_ypos = fs->y_offset;
	for (lnum = 0; lnum < start_lnum; ++lnum)
		old_ypos += fs->lmap[lnum].height;
	
	pc = NULL;
	
	while (lm->point < endpos) {
		Lmap next_lmap, old_lmap;
		Lmap *oldlm;

		if (lnum >= fs->lmap_size)
			expand_lmap(fs, lnum);
		
		edit__init_rdesc(&next_lmap);

		edit__init_rdesc(&old_lmap);
		edit__copy_lmap(&old_lmap, &fs->lmap[lnum]);
				
		edit__convert_line(d, lm, &next_lmap, &pc);

		if (ypos + lm->height > ylim) {
			edit__free_rdesc(&next_lmap);
			edit__free_rdesc(&old_lmap);
			break;
		}

		if (old_lmap.rdesc != NULL &&
		    old_ypos == ypos &&
		    old_lmap.height == lm->height &&
		    old_lmap.baseline == lm->baseline) {
			oldlm = &old_lmap;
		}
		else {
			oldlm = NULL;
		}
		
		edit__render_line(d, lm, ypos, oldlm);

		old_ypos += fs->lmap[lnum].height;
		ypos += lm->height;
		
		edit__copy_lmap(&fs->lmap[lnum], lm);
		
		edit__copy_lmap(lm, &next_lmap);
		edit__free_rdesc(&next_lmap);

		edit__free_rdesc(&old_lmap);
		
		++lnum;
	}

	if (endpos == edit_get_buffer_length(d->buffer) && ypos < ylim) {
		(*d->render_ops->set_area)(d->render_data, 0, ypos,
					   d->display_width, ylim - ypos,
					   FALSE);
	}
	
	*p_lnum = lnum;
	*p_ypos = ypos;
}

static void
go_backwards(d, lm, p_nlines, p_height)
Edit_display *d;
Lmap *lm;
int *p_nlines, *p_height;
{
	Lmap lmap;
	int lnum, height;
	size_t point;
	
	if (edit_find_char_backwards(d->buffer, lm->point - 1, 0, "\n", &point))
		++point;

	edit__init_rdesc(&lmap);
	edit__set_lmap(&lmap, point);

	height = 0;
	lnum = 0;
	
	while (lmap.point != lm->point || lmap.column != lm->column) {
		Lmap next_lmap;
		
		edit__init_rdesc(&next_lmap);
		edit__convert_line(d, &lmap, &next_lmap,
				   (Edit_propchange **)NULL);
		height += lmap.height;
		++lnum;
		
		edit__copy_lmap(&lmap, &next_lmap);
		edit__free_rdesc(&next_lmap);
	}

	edit__set_lmap(lm, point);
	*p_nlines = lnum;
	*p_height = height;
}

static void
back_up(d, lm, delta, p_delta, p_nlines, p_new_y_offset)
Edit_display *d;
Lmap *lm;
int delta;
int *p_delta, *p_nlines, *p_new_y_offset;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	int nlines, height, excess_pixels, new_y_offset, thisline_height;
	
	height = -fs->y_offset;
	nlines = 0;
	thisline_height = 0;	/* to satisfy gcc */

	while (height < delta && (lm->point != 0 || lm->column != 0)) {
		int thisline_nlines;

		go_backwards(d, lm, &thisline_nlines, &thisline_height);
		
		nlines += thisline_nlines;
		height += thisline_height;
	}

	if (height < delta)
		delta = height;
	       
	excess_pixels = height - delta;

	if (excess_pixels == 0) {
		new_y_offset = 0;
	}
	else {
		int line_height;
		Edit_propchange *pc;

		pc = NULL;
		
		for (;;) {
			Lmap next_lmap;
			
			edit__init_rdesc(&next_lmap);
			edit__convert_line(d, lm, &next_lmap, &pc);
			line_height = lm->height;
			
			edit__copy_lmap(lm, &next_lmap);
			edit__free_rdesc(&next_lmap);
			
			--nlines;
		
			if (excess_pixels - line_height <= 0)
				break;
			excess_pixels -= line_height;
		}

		new_y_offset = line_height - excess_pixels;
	}
		
	*p_delta = delta;
	*p_nlines = nlines;
	*p_new_y_offset = new_y_offset;
}

static int
scroll_backwards(d, delta)
Edit_display *d;
int delta;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	Lmap lmap;
	int nlines, new_y_offset;
	bool cursor_was_visible;

	edit__init_rdesc(&lmap);
	edit__copy_lmap(&lmap, &fs->lmap[0]);

	back_up(d, &lmap, delta, &delta, &nlines, &new_y_offset);

	if (delta == 0) {
		edit__free_rdesc(&lmap);
		return 0;
	}

	cursor_was_visible = d->keep_cursor_visible || fl_visible(d, d->point);

	scroll_lines(d,
		     0, nlines,
		     fs->y_offset, fs->y_offset + delta,
		     &lmap, 0, new_y_offset);

	edit__free_rdesc(&lmap);

	change_y_offset(d, new_y_offset);

	if (d->want_cursor &&
	    d->keep_cursor_visible && !fl_visible(d, d->point)) {
		d->point = fs->lmap[fs->num_text_lines].point;

		if (d->point != 0)
			--d->point;
	}

	return delta;
}

static void
scroll_lines(d,
	     old_scroll_start_lnum, new_scroll_start_lnum,
	     old_scroll_start_ypos, new_scroll_start_ypos,
	     repaint_start_lm, repaint_start_lnum, repaint_start_ypos)
Edit_display *d;
int old_scroll_start_lnum, new_scroll_start_lnum;
int old_scroll_start_ypos, new_scroll_start_ypos;
Lmap *repaint_start_lm;
int repaint_start_lnum, repaint_start_ypos;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	Lmap repaint_start_lmap;
	int lnum, scroll_nlines;
	bool scroll_ok;
	int scroll_height, scroll_end_ypos, ypos, ylim;
	int junk_lnum, junk_ypos;
	
	/*  Find out how many common lines there are.
	 */
	ypos = old_scroll_start_ypos;
	ylim = d->display_height - (new_scroll_start_ypos - old_scroll_start_ypos);

	for (lnum = old_scroll_start_lnum; lnum < fs->num_text_lines; ++lnum) {
		if (ypos + fs->lmap[lnum].height > ylim)
			break;
		ypos += fs->lmap[lnum].height;
	}

	scroll_nlines = lnum - old_scroll_start_lnum;
	scroll_height = ypos - old_scroll_start_ypos;
	scroll_end_ypos = new_scroll_start_ypos + scroll_height;

	edit__init_rdesc(&repaint_start_lmap);
	
	if (old_scroll_start_ypos >= new_scroll_start_ypos) {
		edit__copy_lmap(&repaint_start_lmap, &fs->lmap[lnum]);
		
		repaint_start_lnum = lnum - (old_scroll_start_lnum -
					     new_scroll_start_lnum);
	}
	else{
		edit__copy_lmap(&repaint_start_lmap, repaint_start_lm);
	}
		
	if (scroll_height == 0) {
		fill_display(d, &repaint_start_lmap,
			     repaint_start_lnum, repaint_start_ypos);
		edit__free_rdesc(&repaint_start_lmap);
		return;
	}

	if (old_scroll_start_ypos != new_scroll_start_ypos) {
		scroll_ok = (*d->render_ops->copy_area)(d->render_data,
					    0, old_scroll_start_ypos,
					    0, new_scroll_start_ypos,
					    d->display_width, scroll_height);
	}
	else {
		scroll_ok = TRUE;
	}
	
	
	if (fs->lmap_size < new_scroll_start_lnum + scroll_nlines + 1)
		expand_lmap(fs, new_scroll_start_lnum + scroll_nlines + 1);
		
	if (new_scroll_start_lnum < old_scroll_start_lnum) {
		int i;
		
		for (i = 0; i <= scroll_nlines; ++i) {
			edit__copy_lmap(&fs->lmap[new_scroll_start_lnum + i],
				  &fs->lmap[old_scroll_start_lnum + i]);
			edit__free_rdesc(&fs->lmap[old_scroll_start_lnum + i]);
		}
	}
	else if (new_scroll_start_lnum > old_scroll_start_lnum) {
		int i;
		
		for (i = scroll_nlines; i >= 0; --i) {
			edit__copy_lmap(&fs->lmap[new_scroll_start_lnum + i],
				  &fs->lmap[old_scroll_start_lnum + i]);
			edit__free_rdesc(&fs->lmap[old_scroll_start_lnum + i]);
		}
	}

	if (!scroll_ok) {
		force_redraw(fs, new_scroll_start_lnum,
			     new_scroll_start_lnum + scroll_nlines);
	}
	
	if (new_scroll_start_ypos > old_scroll_start_ypos) {
		force_redraw(fs, old_scroll_start_lnum,
			     new_scroll_start_lnum);
		
		fill_lines(d, repaint_start_lm, repaint_start_lnum,
			   repaint_start_ypos,
			   scroll_ok ? new_scroll_start_ypos : scroll_end_ypos,
			   edit_get_buffer_length(d->buffer),
			   &junk_lnum, &junk_ypos);

		force_redraw(fs, new_scroll_start_lnum + scroll_nlines,
			     fs->num_text_lines + 1);
		fs->num_text_lines = new_scroll_start_lnum + scroll_nlines;
		
		if (scroll_end_ypos < d->display_height) {
			(*d->render_ops->set_area)(d->render_data,
						   0, scroll_end_ypos,
						   d->display_width,
						   d->display_height
						           - scroll_end_ypos,
						   FALSE);
		}
	}
	else {
		int fill_start_ypos;
		
		/*  Force complete redraw of lines beyond the ones that
		 *  were scrolled.
		 */
		force_redraw(fs, new_scroll_start_lnum + scroll_nlines,
			     fs->num_text_lines + 1);
		
		if (scroll_ok) {
			fill_start_ypos = scroll_end_ypos;
		}
		else {
			repaint_start_lnum = new_scroll_start_lnum;
			edit__copy_lmap(&repaint_start_lmap,
					 &fs->lmap[repaint_start_lnum]);
			fill_start_ypos = new_scroll_start_ypos;
		}
		
		fill_display(d, &repaint_start_lmap, repaint_start_lnum,
			     fill_start_ypos);
	}

	edit__free_rdesc(&repaint_start_lmap);
}

static void
fill_display(d, lm, start_lnum, ypos)
Edit_display *d;
Lmap *lm;
int start_lnum, ypos;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	int junk_ypos, num_text_lines;

	fill_lines(d, lm, start_lnum, ypos,
		   d->display_height, edit_get_buffer_length(d->buffer),
		   &num_text_lines, &junk_ypos);

	if (num_text_lines != fs->num_text_lines) {
		if (num_text_lines >= fs->lmap_size)
			expand_lmap(fs, num_text_lines);
	
		force_redraw(fs, num_text_lines, fs->num_text_lines + 1);
		fs->num_text_lines = num_text_lines;
	}

	edit__copy_lmap(&fs->lmap[fs->num_text_lines], lm);
}

static int
scroll_forwards(d, delta)
Edit_display *d;
int delta;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	Lmap lmap, new_topline_lmap, repaint_start_lmap;
	int lnum, old_scroll_start_lnum, num_common_lines;
	int ypos, old_scroll_start_ypos;
	bool hit_eof, found_ypos_line, want_scroll;
	size_t buflen;

	if (fs->num_text_lines == 0)
		return 0;

	found_ypos_line = hit_eof = FALSE;
	ypos = fs->y_offset;
	buflen = edit_get_buffer_length(d->buffer);

	edit__init_rdesc(&lmap);
	edit__init_rdesc(&new_topline_lmap);
	edit__init_rdesc(&repaint_start_lmap);
	
	for (lnum = 0; lnum < fs->num_text_lines; ++lnum) {
		edit__copy_lmap(&lmap, &fs->lmap[lnum]);

		if (fs->lmap[lnum + 1].point == buflen) {
			hit_eof = TRUE;
			break;
		}
		
		if (ypos + lmap.height > delta) {
			found_ypos_line = TRUE;
			edit__copy_lmap(&new_topline_lmap, &fs->lmap[lnum]);
			break;
		}
			
		ypos += lmap.height;
	}

	old_scroll_start_lnum = lnum;
	old_scroll_start_ypos = ypos;
	num_common_lines = fs->num_text_lines - lnum;

	want_scroll = found_ypos_line || hit_eof;

	if (!want_scroll) {
		/*  The scroll distance is more than a screenfull, so
		 *  we have to start doing convert_line()s beyond the
		 *  end of the display.
		 */
		Lmap next_lmap;
		Edit_propchange *pc;

		edit__init_rdesc(&next_lmap);
		edit__copy_lmap(&lmap, &fs->lmap[lnum]);

		pc = NULL;
			
		for (;;) {
			edit__convert_line(d, &lmap, &next_lmap, &pc);

			if (next_lmap.point == buflen)
				break;
			
			if (ypos + lmap.height > delta) {
				found_ypos_line = TRUE;
				break;
			}

			ypos += lmap.height;
			edit__copy_lmap(&lmap, &next_lmap);
		}

		edit__copy_lmap(&repaint_start_lmap, &lmap);
		edit__free_rdesc(&next_lmap);
	}

	if (!found_ypos_line && delta > ypos)
		delta = ypos;

	if (delta != 0) {
		bool cursor_was_visible;
		int new_y_offset;
	
		cursor_was_visible = d->keep_cursor_visible ||
			             fl_visible(d, d->point);
	
		if (ypos > delta) {
			new_y_offset = ypos - delta;
		}
		else if (ypos == delta) {
			new_y_offset = 0;
		}
		else {
			if (want_scroll) {
				new_y_offset = new_topline_lmap.height -
					   		        (delta - ypos);

				++old_scroll_start_lnum;
				old_scroll_start_ypos +=new_topline_lmap.height;
			}
			else {
				new_y_offset = repaint_start_lmap.height -
					                        (delta - ypos);
				
				edit__convert_line(d, &repaint_start_lmap,
						   &repaint_start_lmap,
						   (Edit_propchange **)NULL);
			}
		}

		/*  At this point lmap points to the line that should appear at
		 *  the top of the display, and new_y_offset is the new initial
		 *  y offset.
		 */

		if (want_scroll) {
			scroll_lines(d,
				     old_scroll_start_lnum, 0,
				     old_scroll_start_ypos, new_y_offset,
				     (Lmap *)NULL, 0, 0);
		}
		else {
			fill_display(d, &repaint_start_lmap, 0, new_y_offset);
		}

		change_y_offset(d, new_y_offset);

		if (d->want_cursor &&
		    d->keep_cursor_visible && !fl_visible(d, d->point)) {
			d->point = fs->lmap[0].point;
		
			if (fs->lmap[0].ch_offset != 0)
				++d->point;
		}
	}

	edit__free_rdesc(&lmap);
	edit__free_rdesc(&new_topline_lmap);
	edit__free_rdesc(&repaint_start_lmap);
	
	return delta;
}

static void
change_y_offset(d, new_y_offset)
Edit_display *d;
int new_y_offset;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	
	if (new_y_offset > fs->y_offset) {
		(*d->render_ops->set_area)(d->render_data, 0, fs->y_offset,
					   d->display_width,
					   new_y_offset - fs->y_offset, FALSE);
	}

	fs->y_offset = new_y_offset;
}
	
static bool
fl_visible(d, point)
Edit_display *d;
size_t point;
{
	Fl_state *fs = (Fl_state *)d->display_data;

	return fs->lmap != NULL &&
	       in_line_range(d, point, fs->lmap, fs->lmap + fs->num_text_lines);
}

static bool
point_before_lmap(point, lm)
size_t point;
Lmap *lm;
{
	return point < lm->point ||
	       (point == lm->point && lm->ch_offset != 0);
}

static bool
in_line_range(d, point, start, lim)
Edit_display *d;
size_t point;
Lmap *start, *lim;
{
	if (point == edit_get_buffer_length(d->buffer)) {
		int junk_lnum, junk_ypos;
		
		return point_to_lnum(d, point, &junk_lnum, &junk_ypos);
	}
	
	return !point_before_lmap(point, start) &&
	        point_before_lmap(point, lim);
}

static bool
point_to_lnum(d, point, p_lnum, p_ypos)
Edit_display *d;
size_t point;
int *p_lnum, *p_ypos;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	int lnum, ypos;
	size_t buflen;

	ypos = fs->y_offset;
	buflen = edit_get_buffer_length(d->buffer);
	
	if (point == buflen) {
		int n;
		Edit_propchange *pc;

		if (buflen == 0) {
			*p_lnum = 0;
			*p_ypos = ypos;
			return TRUE;
		}

		lnum = fs->num_text_lines;
		
		if (fs->lmap[lnum].point != point)
			return FALSE;

		if (no_final_newline(d->buffer, buflen))
			--lnum;

		for (n = 0; n < lnum; ++n)
			ypos += fs->lmap[n].height;

		pc = edit_get_propchange_at_point(d->buffer, point);

		if (ypos + pc->fi->height > d->display_height)
			return FALSE;
		
		*p_lnum = lnum;
		*p_ypos = ypos;
		return TRUE;
	}
	
	if (point_before_lmap(point, fs->lmap))
		return FALSE;
	
	for (lnum = 0; lnum < fs->num_text_lines; ++lnum) {
		if (point_before_lmap(point, &fs->lmap[lnum + 1])) {
			*p_lnum = lnum;
			*p_ypos = ypos;
			return TRUE;
		}
		ypos += fs->lmap[lnum].height;
	}

	return FALSE;
}

static bool
fl_pixel_to_point(d, x, y, p_point)
Edit_display *d;
int x, y;
size_t *p_point;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	int ypos, lnum;

	if (y < fs->y_offset)
		return FALSE;

	ypos = fs->y_offset;
	
	for (lnum = 0; lnum < fs->num_text_lines; ++lnum) {
		if (ypos + fs->lmap[lnum].height > y)
			break;
		ypos += fs->lmap[lnum].height;
	}

	if (lnum == fs->num_text_lines)
		return FALSE;

	return edit__xpos_to_point(d, &fs->lmap[lnum], x, p_point);
}

static bool
fl_point_to_pixel(d, point, p_x, p_y, p_width, p_height, p_baseline)
Edit_display *d;
size_t point;
int *p_x, *p_y, *p_width, *p_height, *p_baseline;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	int lnum;
	
	return point_to_lnum(d, point, &lnum, p_y) &&
	       edit__lmap_point_to_pixel(d, &fs->lmap[lnum], point,
					 p_x, p_width, p_height, p_baseline);
}

static void
fl_draw_cursor(d, point, on)
Edit_display *d;
size_t point;
bool on;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	int lnum, ypos;

	if (point_to_lnum(d, point, &lnum, &ypos))
		edit__draw_lmap_cursor(d, &fs->lmap[lnum], point, ypos, on);
}

static int
fl_scroll_display(d, delta)
Edit_display *d;
int delta;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	int res;
	
	if (delta > 0)
		res = scroll_forwards(d, delta);
	else
		res = -scroll_backwards(d, -delta);

	if (fs->want_check_after_updates)
		check_lmap(d);

	return res;
}

static void
fl_about_to_delete(d, pos, len)
Edit_display *d;
size_t pos, len;
{
	prepare_for_change(d, pos, pos + len);
}

static void
fl_about_to_insert(d, pos, len)
Edit_display *d;
size_t pos, len;
{
	prepare_for_change(d, pos, pos);
}

static void
prepare_for_change(d, startpos, limpos)
Edit_display *d;
size_t startpos, limpos;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	size_t endpos, buflen;

	buflen = edit_get_buffer_length(d->buffer);
	edit_find_char_forwards(d->buffer, limpos, EDIT_MAX_POINT, "\n", &endpos);

	fs->change_affects_display =
		fl_visible(d, startpos) || fl_visible(d, endpos) ||
		(point_before_lmap(startpos, &fs->lmap[0]) &&
		 !point_before_lmap(endpos, &fs->lmap[fs->num_text_lines]));
	
	fs->do_full_repaint = FALSE;

	if (!fs->change_affects_display)
		return;
	
	if (!point_to_lnum(d, startpos, &fs->start_lnum, &fs->start_ypos) ||
	    fs->lmap[fs->start_lnum].ch_offset != 0) {
		fs->do_full_repaint = TRUE;
	}
	else if (limpos > startpos && fs->lmap[fs->start_lnum].column != 0) {
		
		/*  We are the start of a continuation line, and a delete
		 *  may lose the backslash continuation character on the
		 *  previous line, so back up.
		 */
		if (fs->start_lnum == 0) {
			fs->do_full_repaint = TRUE;
			return;
		}
			
		--fs->start_lnum;
		fs->start_ypos -= fs->lmap[fs->start_lnum].height;
	}
		
	fs->cursor_was_visible = d->want_cursor && fl_visible(d, d->point);
}
	
static void
fl_done_delete(d, pos, len)
Edit_display *d;
size_t pos, len;
{
	handle_change(d, pos, len, 0);
}

static void
fl_done_insert(d, pos, len)
Edit_display *d;
size_t pos, len;
{
	handle_change(d, pos, 0, len);
}

static void
handle_change(d, pos, oldlen, newlen)
Edit_display *d;
size_t pos, oldlen, newlen;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	size_t endpos, maxlen;
	int lnum;
	int ypos;
	Lmap end_fill_lmap;

	edit__init_rdesc(&end_fill_lmap);
		
	if (fs->change_affects_display && !fs->do_full_repaint)
		edit__copy_lmap(&end_fill_lmap, &fs->lmap[fs->start_lnum]);

	if (oldlen != newlen)
		note_change(d, pos, (long)newlen - (long)oldlen);

	if (!fs->change_affects_display) {
		edit__free_rdesc(&end_fill_lmap);
		return;
	}

	if (fs->do_full_repaint) {
		Lmap lmap;
		size_t point, buflen;
		
		edit__free_rdesc(&end_fill_lmap);

		point = fs->lmap[0].point;
		buflen = edit_get_buffer_length(d->buffer);

		if (point > buflen) {
			point = buflen;

			if (point > 0)
				--point;
		}
		
		edit__init_rdesc(&lmap);
		make_lmap(d, point, &lmap);
	
		fill_display(d, &lmap, 0, 0);
		change_y_offset(d, 0);
		      
		edit__free_rdesc(&lmap);
		
		return;
	}

	maxlen = (newlen > oldlen) ? newlen : oldlen;

	if (edit_find_char_forwards(d->buffer, pos + maxlen, EDIT_MAX_POINT,
				    "\n", &endpos))
		++endpos;

	ypos = fs->y_offset;
	for (lnum = 0; lnum < fs->num_text_lines; ++lnum) {
		if (fs->lmap[lnum].point == endpos &&
		    fs->lmap[lnum].column == 0)
			break;

		ypos += fs->lmap[lnum].height;
	}
	
	if (lnum == fs->num_text_lines) {
		edit__free_rdesc(&end_fill_lmap); /* BUG: why? */
		fill_display(d, &end_fill_lmap, fs->start_lnum, fs->start_ypos);
	}
	else {
		Edit_propchange *pc;
		Lmap lmap;
		int end_fill_lnum, old_scroll_start_lnum, new_scroll_start_lnum;
		int end_fill_ypos, old_scroll_start_ypos, new_scroll_start_ypos;
		
		old_scroll_start_lnum = lnum;
		old_scroll_start_ypos = ypos;
		
		/*  Display the lines that the update has changed, but stop
		 *  before overwriting lines that we can scroll.
		 */
		fill_lines(d, &end_fill_lmap, fs->start_lnum, fs->start_ypos,
			   old_scroll_start_ypos, endpos,
			   &end_fill_lnum, &end_fill_ypos);
	
		/*  Work out how many new lines there are, without
		 *  displaying them or overwriting fs->lmap entries.
		 */

		edit__init_rdesc(&lmap);
		edit__copy_lmap(&lmap, &end_fill_lmap);
		
		ypos = end_fill_ypos;
		lnum = end_fill_lnum;

		pc = NULL;

		while (lmap.point < endpos) {
			Lmap next_lmap;

			edit__init_rdesc(&next_lmap);
			edit__convert_line(d, &lmap, &next_lmap, &pc);

			if (ypos + lmap.height > d->display_height)
				break;

			++lnum;
			ypos += lmap.height;
			
			edit__copy_lmap(&lmap, &next_lmap);
			edit__free_rdesc(&next_lmap);
		}

		new_scroll_start_ypos = ypos;
		new_scroll_start_lnum = lnum;
		
		scroll_lines(d,
			     old_scroll_start_lnum, new_scroll_start_lnum, 
			     old_scroll_start_ypos, new_scroll_start_ypos,
			     &end_fill_lmap, end_fill_lnum, end_fill_ypos);

		edit__free_rdesc(&lmap);

	}
	
	if (fs->want_check_after_updates)
		check_lmap(d);

	edit__free_rdesc(&end_fill_lmap);
}

static void
fl_handle_prop_change(d, start, lim)
Edit_display *d;
size_t start, lim;
{
	prepare_for_change(d, start, lim);
	handle_change(d, start, lim - start, lim - start);
}

static void
check_lmap(d)
Edit_display *d;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	Lmap lmap;
	int lnum, ypos;
	size_t buflen;

	edit__init_rdesc(&lmap);
	edit__copy_lmap(&lmap, &fs->lmap[0]);

	if (fs->y_offset < 0)
		panic("y_offset botch in cl");

	if (fs->y_offset != 0 && lmap.point == 0 && lmap.ch_offset == 0)
		panic("off top botch in cl");

	lnum = 0;
	ypos = fs->y_offset;
	buflen = edit_get_buffer_length(d->buffer);
	
	while (lmap.point < buflen) {
		Lmap next_lmap;

		edit__init_rdesc(&next_lmap);
		edit__convert_line(d, &lmap, &next_lmap,
				   (Edit_propchange **)NULL);

		if (ypos + lmap.height > d->display_height)
			break;
		
		check_lmaps_same(&fs->lmap[lnum], &lmap, FALSE);

		ypos += lmap.height;
		++lnum;
		
		edit__copy_lmap(&lmap, &next_lmap);
		edit__free_rdesc(&next_lmap);
	}

	if (lnum != fs->num_text_lines)
		panic("num_text_lines mismatch in cl");

	check_lmaps_same(&fs->lmap[lnum], &lmap, TRUE);
	
	if (lmap.point > buflen)
		panic("buflen mismatch in cl");

	if (ypos > d->display_height)
		panic("display_height botch in cl");

	edit__free_rdesc(&lmap);
}

static void
check_lmaps_same(lm1, lm2, is_last_lmap)
Lmap *lm1, *lm2;
bool is_last_lmap;
{
	if (lm1->point != lm2->point ||
	    lm1->ch_offset != lm2->ch_offset ||
	    lm1->column != lm2->column)
		panic("lmap pos mismatch in cls");

	if (!is_last_lmap) {
		if (lm1->height != lm2->height ||
		    lm1->baseline != lm2->baseline ||
		    lm1->line_continues != lm2->line_continues)
			panic("lmap render mismatch in cls");
		
		edit__check_rdescs_same(lm1->rdesc, lm2->rdesc);
	}
}

static void
note_change(d, pos, delta)
Edit_display *d;
size_t pos;
long delta;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	Lmap *lm, *lim;

	lim = fs->lmap + fs->num_text_lines + 1;
	
	for (lm = fs->lmap; lm < lim; ++lm) {
		if (lm->point >= pos) {
			if (delta < 0 && -delta > lm->point)
				lm->point = 0;
			else
				lm->point += delta;
			
			edit__update_rdesc_points(lm->rdesc, pos, delta);
		}
	}
}

static void
fl_note_new_buffer(d)
Edit_display *d;
{
	Fl_state *fs = (Fl_state *)d->display_data;
	
	fs->lmap[0].point = 0;
	fs->num_text_lines = 0;
}

static bool
no_final_newline(buffer, buflen)
Edit_buffer *buffer;
size_t buflen;
{
	const char *text, *lim;

	if (buflen == 0)
		return TRUE;
	
	edit_get_bytes(buffer, buflen - 1, &text, &lim);
		 
	return text == lim || *text != '\n';
}

static void
expand_lmap(fs, count)
Fl_state *fs;
int count;
{
	int lnum, old_lmap_size;

	old_lmap_size = fs->lmap_size;
	
	while (fs->lmap_size <= count)
		fs->lmap_size += 20;

	if (old_lmap_size == 0) {
		fs->lmap = (Lmap *)e_malloc(fs->lmap_size * sizeof(Lmap));
	}
	else {
		fs->lmap = (Lmap *)e_realloc((char *)fs->lmap,
				     fs->lmap_size * sizeof(Lmap));
	}

	for (lnum = old_lmap_size; lnum < fs->lmap_size; ++lnum)
	{
				/* RGA for purify */
	  memset((char *)&fs->lmap[lnum], '\0', sizeof(Lmap));
	  edit__init_rdesc(&fs->lmap[lnum]);
	}
}
		
/*  Make an lmap pointing at the start of the display line containing point.
 */
static void
make_lmap(d, point, lm)
Edit_display *d;
size_t point;
Lmap *lm;
{
	size_t start_point, buflen;
	Edit_propchange *pc;
	
	if (edit_find_char_backwards(d->buffer, point, 0, "\n", &start_point))
		++start_point;

	edit__set_lmap(lm, start_point);

	if (lm->point == point)
		return;
	
	buflen = edit_get_buffer_length(d->buffer);
	pc = NULL;
	
	for (;;) {
		Lmap next_lmap;

		edit__init_rdesc(&next_lmap);
		edit__convert_line(d, lm, &next_lmap, &pc);

		if (next_lmap.point > point ||
		    (next_lmap.point == point && next_lmap.ch_offset != 0))
			break;

		if (point == buflen && no_final_newline(d->buffer, buflen) &&
		    next_lmap.point == point)
			break;
		
		edit__copy_lmap(lm, &next_lmap);
		edit__free_rdesc(&next_lmap);
	}
}

static void
fl_redraw_with_point_visible(d, point)
Edit_display *d;
size_t point;
{
	Lmap lmap;
	int junk_delta, junk_nlines, junk_y_offset;

	edit__init_rdesc(&lmap);
	make_lmap(d, point, &lmap);
	back_up(d, &lmap, d->display_height / 2, 
		&junk_delta, &junk_nlines, &junk_y_offset);

	fill_display(d, &lmap, 0, 0);
	change_y_offset(d, 0);
	edit__free_rdesc(&lmap);
}

static bool
fl_update_display_size(d, width, height)
Edit_display *d;
int width, height;
{
	Fl_state *fs = (Fl_state *)d->display_data;

	force_redraw(fs, 0, fs->num_text_lines + 1);
	fs->num_text_lines = 0;
	
	return TRUE;
}

static void
fl_destroy(d)
Edit_display *d;
{
	Fl_state *fs = (Fl_state *)d->display_data;

	/*  Free all the lmaps.
	 */
	force_redraw(fs, 0, fs->num_text_lines + 1);

	free((char *)d->render_context);
	free((char *)fs);
}

void
edit_use_folded_lines(d, tabspaces, want_checks, fold_lines)
Edit_display *d;
int tabspaces;
bool want_checks;
bool fold_lines;
{
	static Display_ops fl_display_ops = {
		fl_visible,
		fl_redraw_with_point_visible,
		fl_draw_cursor,
		
		fl_get_display_info,
		
		fl_scroll_display,
		fl_redraw_display,
		fl_display_from,
		
		fl_about_to_insert,
		fl_done_insert,
		fl_about_to_delete,
		fl_done_delete,
		
		fl_update_display_size,
		fl_note_new_buffer,
		
		fl_handle_prop_change,
		
		fl_point_to_pixel,
		fl_pixel_to_point,

		fl_destroy,
	};
	Fl_state *fs;
	

	fs = (Fl_state *)e_malloc(sizeof(Fl_state));
	fs->num_text_lines = 0;
	fs->y_offset = 0;
	fs->lmap_size = 0;
	fs->lmap = NULL;
	fs->want_check_after_updates = want_checks;

	expand_lmap(fs, 0);

	d->display_ops = &fl_display_ops;
	d->display_data = (char *)fs;
	
	d->render_context = edit__make_render_context(tabspaces, TRUE,
						      fold_lines,
						      0, 0,
						      RMORE_WIDTH,
						      RMORE_LMARGIN,
						      RMORE_VMARGIN);
}
