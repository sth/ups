/* render.h - header file for render.c */

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


/* @(#)render.h	1.2 04 Sep 1994 (UKC) */

typedef struct Render_desc Render_desc;

typedef struct {
	size_t point;
	int ch_offset;		/* Offset within multibyte character */
	size_t column;		/* Display column # */
	Render_desc *rdesc;	/* Info from convert_line() to render_line() */
	
	int height;		/* Height of line in pixels */
	int baseline;		/* Baseline for rendering text */
	bool line_continues;	/* More of this logical line to come */
} Lmap;

Render_context *edit__make_render_context PROTO((int tabspaces,
						 bool stop_at_newline,
						 bool fold_lines,
						 int lmore_width,
						 int lmore_lmargin,
						 int rmore_width,
						 int rmore_lmargin,
						 int more_vmargin));

void edit__draw_lmap_cursor PROTO((Edit_display *d, Lmap *lm, size_t point,
				   int ypos, bool on));

void edit__init_rdesc PROTO((Lmap *lmap));
void edit__copy_lmap PROTO((Lmap *dst, Lmap *src));
void edit__free_rdesc PROTO((Lmap *lmap));

void edit__set_lmap PROTO((Lmap *lm, size_t point));

void edit__convert_line PROTO((Edit_display *d, Lmap *lm, Lmap *nextlm, 
			       Edit_propchange **p_pc));

bool edit__xpos_to_point PROTO((Edit_display *d, Lmap *lm, int xpos,
				size_t *p_point));

bool edit__lmap_point_to_pixel PROTO((Edit_display *d, Lmap *lm, size_t point, 
				      int *p_x, int *p_width,
				      int *p_height, int *p_baseline));


void edit__render_line PROTO((Edit_display *d, Lmap *lm, int ypos,
			      Lmap *oldrd));

void edit__update_rdesc_points PROTO((Render_desc *rd, size_t point,
				      long delta));

void edit__check_rdescs_same PROTO((Render_desc *rd1, Render_desc *rd2));

bool edit__extra_char_would_fit PROTO((Edit_display *d, Lmap *lm, int ch));
