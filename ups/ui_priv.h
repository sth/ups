/* ui_priv.h - private header file for the ui_*.c files */

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


/* @(#)ui_priv.h	1.13 24 May 1995 (UKC) */

/*  Event mask for wn_next_event()
 */
#define EVENT_MASK	(EV_BUTTON_UP | EV_BUTTON_DOWN | EV_MOUSE_MOVED | \
					EV_KEY | EV_INTERRUPT | \
					EV_WINDOW_EXPOSED | EV_WINDOW_RESIZED)

#define NO_SELECTION (-1)

/*  Definition of a popup menu for use with select_from_popup().
 */
typedef struct popupst {
	int po_mdesc;		/* Menu descriptor */
	int po_save_last;	/* If set, remember caption the user last selected */
	int po_last;		/* Last caption selected by user */
	const char **po_caps;	/* NULL terminated array of captions */
	int po_left;            /* Captions left justified */
} popup_t;

/*  Source display window functions.
 */
void get_source_title_area PROTO((int wn, int *p_x, int *p_y,
				  int *p_width, int *p_height,
				  int *p_min_y, int *p_max_y));
int get_source_window_height PROTO((void));

#ifdef SRCWIN_H_INCLUDED
void echo_src_name_and_lnum PROTO((Srcwin *sw,
						const char *name, int lnum));
void draw_src_name_and_lnum PROTO((int wn));
#endif

/*  Miscellaneous functions.
 */
int mhc_setcaps PROTO((int val));
void show_menu PROTO((int md, int wn));

#ifdef FED_H_INCLUDED
void ta_init PROTO((edesc_t *edesc, int wn, font_t *font));
#endif

void scroll PROTO((event_t *ev, char *arg));
void display_area_scroll PROTO((int dist));
void display_area_goto PROTO((int y));
int select_from_popup PROTO((int wn, const char *name,
			     int button, popup_t *po, int x, int y));
void updating_callback_func PROTO((int oldstate, int newstate));
void set_start_menu PROTO((int md));
void set_dynamic_menu PROTO((int md, const char *menu_name));

void process_typing_line_text PROTO((const char *text));

/*  Input handling functions.
 */
#ifdef TB_H_INCLUDED
void tbar_event_handler PROTO((tbar_id_t tbar_id, event_t *ev));
#endif

#ifdef EDIT_H_INCLUDED
void do_selection PROTO((Edit_display *display, event_t *ev));

char *selection_loop PROTO((Edit_display *display, event_t *ev,
			    Edit_propchange *startpc,
			    Edit_propchange *endpc,
			    bool set_global_selection,
			    bool want_text));

void add_propchange_pair PROTO((Edit_buffer *buffer, size_t start, size_t lim,
				Edit_flags flags, Edit_propchange **p_startpc,
				Edit_propchange **p_endpc));

char *remove_propchange_pair PROTO((Edit_buffer *buffer,
				    Edit_propchange *startpc,
				    Edit_propchange *endpc));
#endif

#ifdef REG_H_INCLUDED
void source_window_event_handler PROTO((Region *region, event_t *ev));
void output_window_event_handler PROTO((Region *region, event_t *ev));
void name_window_event_handler PROTO((Region *region, event_t *ev));
#endif

/*  Mousehole functions.
 */
int mfn_dmenu PROTO((int caps, char *arg));
int mfn_source PROTO((int caps, char *arg));

void get_custom_menu_str PROTO((void (*func)(int c, bool meta), event_t *ev));
