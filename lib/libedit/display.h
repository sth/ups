/* display.h - header file for display.c - private to the edit library */

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


/* @(#)display.h	1.8 09 Sep 1994 (UKC) */

typedef struct {
	bool (*visible)PROTO((Edit_display *d, size_t point));
	void (*redraw_with_point_visible)PROTO((Edit_display *d, size_t point));
	void (*draw_cursor)PROTO((Edit_display *d, size_t point, bool on));
	
	void (*get_display_info)PROTO((Edit_display *d, int *p_pixel_offset, 
				       size_t *p_start_point, 
				       size_t *p_lim_point));
	
	int (*scroll_display)PROTO((Edit_display *d, int delta));
	void (*redraw_display)PROTO((Edit_display *d));
	void (*display_from)PROTO((Edit_display *d, int pixel_offset,
				   size_t point));
	
	void (*about_to_insert)PROTO((Edit_display *d, size_t pos, size_t len));
	void (*done_insert)PROTO((Edit_display *d, size_t pos, size_t len));
	void (*about_to_delete)PROTO((Edit_display *d, size_t pos, size_t len));
	void (*done_delete)PROTO((Edit_display *d, size_t pos, size_t len));
	
	bool (*update_display_size)PROTO((Edit_display *d,
					  int nlines, int nchars));
	void (*note_new_buffer)PROTO((Edit_display *d));
	
	void (*handle_prop_change)PROTO((Edit_display *d,
					 size_t start, size_t lim));
	
	bool (*point_to_pixel)PROTO((Edit_display *d, size_t point,
				     int *p_x, int *p_y,
				     int *p_width, int *p_height,
				     int *p_baseline));
	bool (*pixel_to_point)PROTO((Edit_display *d, int x, int y,
				     size_t *p_point));

	void (*destroy)PROTO((Edit_display *display));
} Display_ops;

typedef struct Keymap Keymap;
typedef struct Proplist Proplist;
typedef struct Render_context Render_context;

struct Edit_buffer {
	Text_buffer *textbuf;
       	Proplist *proplist;
};

struct Edit_display {
	Edit_keymap *keymap;
	Edit_buffer *buffer;

	size_t point;
	size_t mark;
	size_t start;
	size_t lim;
	
	size_t goal_column;

	char *render_data;
	Edit_render_ops *render_ops;

	Render_context *render_context;

	int display_width;	/* Width of display in pixels */
	int display_height;	/* Height of display in pixels */

	char *display_data;
	Display_ops *display_ops;

	bool want_cursor;
	bool keep_cursor_visible;

	bool quit_requested;
	int last_key;
	int last_modifiers;
	char *user_data;
	Edit_propchange* sel_startpc;
	Edit_propchange* sel_endpc;
	Edit_history* history;
	int           history_offset;

	Edit_display *next;
};

void edit__get_default_keymap PROTO((Keymap **p_keymap, int *p_keymap_len));

Edit_display *edit__get_display_list PROTO((void));

const char*  edit_get_history PROTO((Edit_history* h, int num));
int  edit_history_top_index PROTO((Edit_history* h));
