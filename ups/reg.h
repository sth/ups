/* reg.h - public header file for reg.c */

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


/* @(#)reg.h	1.11 24 May 1995 (UKC) */

#define REG_H_INCLUDED

typedef struct Region Region;

typedef enum { RE_VERTICAL, RE_HORIZONTAL } re_orientation_t;

typedef unsigned long re_nomsize_t;

#define RE_END		((Region *)NULL)

/* The max number of root regions supported: */
#define MAXROOTS 3

typedef enum {
	RE_EXPOSED,	/* region exposed, but no size change since last draw */
	RE_RESIZED,	/* region resized */
	RE_RESIZING,	/* in middle of resizing: only draw if cheap to do so */
	RE_UNDRAW	/* undo effects of last RE_RESIZING draw, if any */
} re_draw_reason_t;

#define RE_MAX_GROUPS	32

typedef void (*re_draw_proc_t)PROTO((Region *region, int wn,
				     int width, int height,
				     re_draw_reason_t draw_reason));

#ifdef WN_H_INCLUDED
typedef void (*re_input_proc_t)PROTO((Region *region, event_t *ev));
#endif

typedef void (*re_destroy_proc_t)PROTO((Region *region));

typedef bool (*re_release_focus_proc_t)PROTO((Region *region, char *data,
					      bool force));
typedef bool (*re_accept_focus_proc_t)PROTO((Region *region, char *data));

typedef struct Edit_display* (*re_get_display_proc_t)PROTO((Region *region));

Region *re_make_region PROTO((int wn));
void re_divide PROTO((Region *region, re_orientation_t orientation, ...));
int re_change_position PROTO((Region *region, int delta));

void re_set_rootwin PROTO((Region *region));
void re_set_rootwin1 PROTO((Region *region));

void re_set_default_keyboard_focus PROTO((Region *region, char *data));
bool re_set_keyboard_focus PROTO((Region *region, char *data));
Region *re_get_keyboard_focus_region PROTO((void));
char *re_get_keyboard_focus_data PROTO((Region *region));
void re_clear_force_quit_flag PROTO((Region *region));

void re_set_minsize PROTO((Region *region, int width, int height));
void re_set_data PROTO((Region *region, char *data));
void re_set_cursor PROTO((Region *region, int cursor));

#ifdef WN_H_INCLUDED
void re_set_callbacks PROTO((Region *region,
			     re_draw_proc_t draw_proc,
			     re_input_proc_t input_proc,
			     re_destroy_proc_t destroy_proc));
#endif

void re_set_display_callback PROTO((Region* region, re_get_display_proc_t));
struct Edit_display* re_get_display PROTO((Region *region));

void re_set_keyboard_focus_callbacks PROTO((Region *region,
					    re_accept_focus_proc_t accept_proc,
					    re_release_focus_proc_t quit_proc));

void re_set_margins PROTO((Region *region, int left, int right,
						  int top, int bottom));
void re_set_mhcaps PROTO((Region *region,
			  const char *left_caption,
			  const char *middle_caption,
			  const char *right_caption));



char *re_get_data PROTO((Region *region));
int re_get_wn PROTO((Region *region));
int re_get_root_wn PROTO((Region *region));
bool re_same_root_wn PROTO((Region *region1, Region* region2));
Region *re_get_parent PROTO((Region *region));

void re_expose PROTO((Region *region, bool clear_first));
void re_expose_clip PROTO((Region *region, bool clear_first, int x,
			   int y, int width, int height));
void re_redraw_root PROTO((unsigned long event_type, bool clear_first));
void re_redraw_root_clip PROTO((unsigned long event_type, int x, int y,
				int width, int height, bool clear_first));
void re_add_rootwin PROTO((Region* region, bool lead_region));
int  re_in_icon_state PROTO((void));

#ifdef WN_H_INCLUDED
void re_handle_event PROTO((Region* region, event_t* ev));
#endif

void re_event_loop PROTO((void));

void re_set_exit_event_loop_flag PROTO((void));
bool re_get_exit_event_loop_flag PROTO((void));

void re_set_typing_regions PROTO((Region* typing_line, Region* extended));

/*  clear_message() must be provided by the application.
 *  It should remove any currently displayed error message.
 *  TODO: zap this!
 */
void clear_message PROTO((void));

