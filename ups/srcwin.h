/* srcwin.h - public header file for srcwin.c */

/*  Copyright 1991 Mark Russell, University of Kent at Canterbury.
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


/* @(#)srcwin.h	1.3 09 Apr 1995 (UKC) */

#define SRCWIN_H_INCLUDED

extern struct Edit_history* breakpoint_history;
typedef struct Srcwin Srcwin;
typedef struct Outwin Outwin;

#ifdef WN_H_INCLUDED
void srcwin_set_fonts PROTO((font_t *srcfont, font_t *editfont,
			     font_t *stopline_font));
void display_overlay_set_font PROTO((font_t *font));
#endif

void srcwin_set_tabwidth PROTO((int tabwidth));
void srcwin_set_want_checks_flag PROTO((void));
void display_area_overlay_unselect_global_selection PROTO((void));

#ifdef TB_H_INCLUDED
Srcwin *srcwin_create PROTO((int wn, tbar_id_t tbar_id));
Outwin *outwin_create PROTO((int wn, tbar_id_t tbar_id));
Outwin * display_area_overlay_create PROTO((int wn, tbar_id_t tbar));
#endif

void outwin_lower PROTO((Outwin* ow));
void outwin_raise PROTO((Outwin* ow));
void outwin_iconify PROTO((Outwin* ow));
void outwin_insert PROTO((Outwin *ow, const char *text, size_t len));
void outwin_insert_string PROTO((const char *text));
void outwin_putc PROTO((Outwin *ow, int ch));
void outwin_goto_point_and_update PROTO((Outwin *ow, size_t point));
size_t outwin_get_buffer_length PROTO((Outwin *ow));
size_t outwin_get_pos PROTO((Outwin *ow));

void outwin_delete_all_text PROTO((Outwin *ow));

void srcwin_goto_point PROTO((Srcwin *sw, size_t point));
size_t srcwin_get_pos PROTO((Srcwin *sw));
void srcwin_redraw PROTO((Srcwin *sw));
void outwin_redraw PROTO((Outwin *sw));

/* De-iconize and raise the root window containing the window */
void outwin_raise PROTO((Outwin *ow));

/* Map or unmap the window. */
void outwin_map PROTO((Outwin *ow));
void outwin_unmap PROTO((Outwin *ow));

void srcwin_clear_history PROTO((Srcwin *sw));
void srcwin_push_current_pos PROTO((Srcwin *sw));
void srcwin_pop PROTO((Srcwin *sw));

size_t outwin_goto_point PROTO((Outwin *ow, size_t point));
int srcwin_goto_ypos PROTO((Srcwin *sw, int ypos));

int outwin_scroll PROTO((Outwin *ow, int npixels));
int srcwin_scroll PROTO((Srcwin *sw, int npixels));

void outwin_search PROTO((Outwin *ow, const char *pattern, bool forwards));
void srcwin_search PROTO((Srcwin *sw, const char *pattern, bool forwards));
void display_area_overlay_search PROTO((Outwin *dw, const char *pattern,
					bool forwards));

void srcwin_set_data PROTO((Srcwin *sw, char *data));
char *srcwin_get_data PROTO((Srcwin *sw));

void outwin_set_data PROTO((Outwin *sw, char *data));
char *outwin_get_data PROTO((Outwin *sw));

int outwin_get_window_height PROTO((Outwin *sw));
int srcwin_get_window_height PROTO((Srcwin *sw));

long srcwin_draw_arrow PROTO((Srcwin *sw, int lnum));

#ifdef EDIT_H_INCLUDED
Edit_display *srcwin_get_display PROTO((Srcwin *sw));
Edit_display *outwin_get_display PROTO((Outwin *ow));

Edit_buffer *outwin_get_buffer PROTO((Outwin *ow));
#endif

#ifdef SYMTAB_H_INCLUDED

/*  The information returned by srcwin_get_info.
 */
typedef struct srcinfost {
	fil_t *fil;		/* displayed file, or NULL */
	int fil_lnum;		/* fil lnum of line (x,y) is over, or -1 */
	bool have_point;
	size_t point;
} Srcinfo;

void srcwin_get_info PROTO((Srcwin *sw, int x, int y, Srcinfo *si));

bool srcwin_set_stopline PROTO((fil_t *fil, int fil_lnum));

bool srcwin_show PROTO((Srcwin *sw, fil_t *fil, int fil_lnum));

#ifdef SRCBUF_H_INCLUDED
Editblock *srcwin_add_editblock PROTO((Srcwin *sw, fil_t *fil, int fil_lnum,
				       char *text,
				       Editblock_callback callback,
				       Editblock_change_callback change_callback,
				       bool enabled,
				       char *user_data));

void  srcwin_enable_editblock PROTO(( Editblock* editblock,
				      bool enabled));

#ifdef REG_H_INCLUDED
void srcwin_start_edit PROTO((Srcwin *sw, Region *region, Editblock *eb,
			      size_t point));
#endif
#endif

#endif /* SYMTAB_H_INCLUDED */

#ifdef SRCBUF_H_INCLUDED
Srcbuf *srcwin_visit_file PROTO((alloc_pool_t *ap,
				 const char *what, const char *path));
#endif

#ifdef WN_H_INCLUDED
#ifdef REG_H_INCLUDED
int srcwin_set_point PROTO((Srcwin *sw, Region *region, event_t *ev));
void srcwin_handle_key_event PROTO((Srcwin *sw, Region *region, event_t *ev));
bool srcwin_release_focus PROTO((Region *region, char *data, bool force));

void outwin_set_point PROTO((Outwin *ow, Region *region, event_t *ev));
void outwin_handle_key_event PROTO((Outwin *ow, Region *region, event_t *ev));
bool outwin_release_focus PROTO((Region *region, char *data, bool force));
/**** RCB: Moved to libedit/win.c and renamed to get_editblock_colors
** void srcwin_get_editblock_colors PROTO((int *p_bg, int *p_fg));
*/
void srcwin_handle_insert_text PROTO((Srcwin *sw, char *text));
void handle_srcwin_char PROTO((int key_char, bool meta));
void handle_output_window_char PROTO((int key_char, bool meta));
bool srcwin_have_editblock PROTO((Srcwin *sw, Region *region, event_t *ev));
void display_area_overlay_insert_string PROTO((const char *text, int pos,
					       int len, int prepend_newline,
					       int fg_color,
					       int var_coloring_only));
int display_area_overlay_control PROTO((int set, int reset));
void display_area_overlay_delete_all_text PROTO((Outwin *dw));
void display_area_overlay_scroll PROTO((Outwin *dw, int npixels));
void display_area_overlay_tb_clear PROTO((Outwin *dw));
bool display_area_overlay_set_propchange_updates PROTO((Outwin *dw, bool val));
#endif
#endif
void srcwin_reset PROTO((void));
