/* win.c - an Edit_display with Wn rendering and input callbacks */

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


/* @(#)win.c	1.11 20/6/95 (UKC) */
char edit_win_c_rcsid[] = "$Id$";

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <local/wn.h>
#include <local/ukcprog.h>

#include "edit.h"
#include "win.h"
#include "display.h"

typedef struct {
	int wn;
	int fg;
	int bg;
} Wdesc;

void get_variable_colors PROTO((int wn, long *p_bg, long *p_fg));
void get_value_colors PROTO((int wn, bool val_changed, long *p_fg, long *p_bg));
void bpt_getcolor PROTO((int obj, int wn, long *p_fg, long *p_bg));
void env_getcolor PROTO((int obj, int wn, long *p_fg, long *p_bg));
void srcfile_getcolor PROTO((int obj, int wn, long *p_fg, long *p_bg));
void func_getcolor PROTO((int obj, int wn, long *p_fg, long *p_bg));
void sig_getcolor PROTO((int obj, int wn, long *p_fg, long *p_bg));
void wpt_getcolor PROTO((int obj, int wn, long *p_fg, long *p_bg));

static void wt_set_updating PROTO((char *data, bool on));
static void wt_render_text PROTO((char *data, char *font_handle,
				  Edit_flags flags,
				  int x, int y, int baseline,
				  int width, int height,
				  const char *text, int len));
static void wt_set_area PROTO((char *data,
			       int x, int y, int width, int height, bool on));
static void wt_show_cursor PROTO((char *data, char *font_handle,
				  Edit_flags flags,
				  int x, int y, int baseline,
				  int width, int height,
				  int ch, bool on));
static bool wt_copy_area PROTO((char *data,
				int old_x, int old_y, int new_x, int new_y,
				int width, int height));
static bool wt_get_selection PROTO((char *data, const char **p_bytes, 
                                    size_t *p_nbytes));
static void wt_destroy PROTO((char *data));

static void edit_clear_xclipboard PROTO((void*));

static void get_editblock_colors PROTO((int wn, long* p_bg, long* p_fg)); /* RCB */

Edit_fontinfo *
edit_make_wn_fontinfo(font)
font_t *font;
{
	return edit_make_fontinfo((char *)font,
				  font->ft_width_tab,
				  font->ft_height, font->ft_baseline);
}

Edit_display *
edit_create_wn_display(wn, fg, bg, want_cursor, keep_cursor_visible)
int wn;
int fg, bg;
bool want_cursor, keep_cursor_visible;
{
	static Edit_render_ops wt_display_ops = {
		wt_set_updating,
		wt_get_selection,
		wt_render_text,
		wt_set_area,
		wt_show_cursor,
		wt_copy_area,
		wt_destroy
	};
	Wdesc *wd;
	Edit_display *d;
	
	wd = (Wdesc *)e_malloc(sizeof(Wdesc));
	wd->wn = wn;
	wd->fg = fg;
	wd->bg = bg;
	
	d = edit_create_display(want_cursor, keep_cursor_visible,
				(char *)wd, &wt_display_ops);

	return d;
}

void
edit_update_wn_window_colors(d, fg, bg)
Edit_display *d;
int fg, bg;
{
	Wdesc *wd;
	char *render_data;
	Edit_render_ops *render_ops;
       
	edit_get_render_data_and_ops(d, &render_data, &render_ops);
	wd = (Wdesc *)render_data;
	
	wd->fg = fg;
	wd->bg = bg;
}

bool
edit_update_wn_window_size(d, wn)
Edit_display *d;
int wn;
{
	int width, height;
	Wdesc *wd;
	char *render_data;
	Edit_render_ops *render_ops;
       
	edit_get_render_data_and_ops(d, &render_data, &render_ops);
	wd = (Wdesc *)render_data;
	
	wn_get_window_size(wn, &width, &height);

	if (!edit_set_display_size(d, width, height))
		return FALSE;
	
	wd->wn = wn;
	
	return TRUE;
}
	
void
edit_handle_wn_key_event(d, ev)
Edit_display *d;
event_t *ev;
{
	int ch;
	unsigned modifiers;

	ch = ev->ev_char;

	modifiers = 0;
	if (ev->ev_buttons & B_SHIFT_MASK)
		modifiers |= EDIT_SHIFT;
	if (ev->ev_buttons & B_ALT_MASK)
		modifiers |= EDIT_ALT;
	if (ev->ev_buttons & B_CONTROL_MASK)
		modifiers |= EDIT_CONTROL;
			
	switch (ch) {
	case WN_CH_LEFT_ARROW:
		ch = EDIT_LEFT_ARROW;
		break;
	case WN_CH_RIGHT_ARROW:
		ch = EDIT_RIGHT_ARROW;
		break;
	case WN_CH_UP_ARROW:
		ch = EDIT_UP_ARROW;
		break;
	case WN_CH_DOWN_ARROW:
		ch = EDIT_DOWN_ARROW;
		break;
	default:
		if (ch >= WN_CH_FUNCKEY(0) && ch <= WN_CH_FUNCKEY(35)) {
			ch = EDIT_FUNC_KEY(ch - WN_CH_FUNCKEY(0));
		}
		else {
			/*  Lose effect of keyboard modifiers.
			 */
			if (ch < 0x20) {
				ch |= '@';
				modifiers |= EDIT_CONTROL;
			}
					
			if (isupper(ch)) {
				ch = tolower(ch);
				modifiers |= EDIT_SHIFT;
			}
			else if ((unsigned char)ch == ch) {
				/*  Some of the standard keys (e.g. `#')
				 *  are obtained on most keyboards using
				 *  shift and another key (e.g. shift-3).
				 *
				 *  As far as the edit key mappings are
				 *  concerned these are not shifted.
				 */
				modifiers &= ~EDIT_SHIFT;
			}
		}
	}
			
	edit_handle_key_event(d, ch, modifiers);
}

static void
wt_destroy(data)
char *data;
{
	free(data);
}

static bool
wt_get_selection(data, p_bytes, p_nbytes)
char *data;
const char **p_bytes;
size_t *p_nbytes;
{
	int int_nbytes;
	Wdesc *wd;

	wd = (Wdesc *)data;

	wn_get_selection(wd->wn, p_bytes, &int_nbytes);
	*p_nbytes = int_nbytes;
	return TRUE;
}

static void
wt_set_updating(data, on)
char *data;
bool on;
{
	Wdesc *wd;

	wd = (Wdesc *)data;

	if (on)
		wn_updating_on(wd->wn);
	else
		wn_updating_off(wd->wn);
}

static void
wt_render_text(data, font_handle, flags, x, y, baseline, width, height,
	       text, len)
char *data, *font_handle;
Edit_flags flags;
int x, y, baseline, width, height;
const char *text;
int len;
{
	Wdesc *wd = (Wdesc *)data;
	font_t *font;
	int fg, bg, color;
	static long val_fg, env_fg, bpt_fg, src_fg, func_fg, sig_fg, edit_fg,
	  var_fg, var_bg, val_bg, env_bg, bpt_bg, src_bg, func_bg, sig_bg,
	  wpt_fg, wpt_bg, edit_bg;
	static int got_colors = 0;
	
	if (!got_colors)
	{
	  got_colors = 1;
	  get_editblock_colors(wd->wn, &edit_fg, &edit_bg);
	  get_variable_colors(wd->wn, &var_fg, &var_bg);
	  get_value_colors(wd->wn, TRUE, &val_fg, &val_bg);
	  bpt_getcolor(0, wd->wn, &bpt_fg, &bpt_bg);
	  env_getcolor(0, wd->wn, &env_fg, &env_bg);
	  srcfile_getcolor(0, wd->wn, &src_fg, &src_bg);
	  func_getcolor(0, wd->wn, &func_fg, &func_bg);
	  sig_getcolor(0, wd->wn, &sig_fg, &sig_bg);
	  wpt_getcolor(0, wd->wn, &wpt_fg, &wpt_bg);
	}

	if (flags & FLAG_EDITLINE)
	{
	  fg = edit_bg;
	  bg = edit_fg;
	}
	else if (flags & FLAG_VARIABLE)
	{
	  fg = var_bg;
	  bg = var_fg;
	}
	else if (flags & FLAG_VALUE)
	{
	  fg = val_fg;
	  bg = val_bg;
	}
	else if (flags & FLAG_BPT)
	{
	  fg = bpt_fg;
	  bg = bpt_bg;
	}
	else if (flags & FLAG_ENV)
	{
	  fg = env_fg;
	  bg = env_bg;
	}
	else if (flags & FLAG_SRC)
	{
	  fg = src_fg;
	  bg = src_bg;
	}
	else if (flags & FLAG_FUNC)
	{
	  fg = func_fg;
	  bg = func_bg;
	}
	else if (flags & FLAG_SIG)
	{
	  fg = sig_fg;
	  bg = sig_bg;
	}
	else if (flags & FLAG_WPT)
	{
	  fg = wpt_fg;
	  bg = wpt_bg;
	}
	else
	{
	  fg = wd->fg;
	  bg = wd->bg;
	}
	
	/* Both FLAG_HIGHLIGHT1 and FLAG_HIGHLIGHT2 reverse the
	** forground and background.  If both are set we do nothing,
	** equivalent to reversing twice.
	*/
	if ((flags & FLAG_HIGHLIGHTS)
	    && (flags & FLAG_HIGHLIGHTS) != FLAG_HIGHLIGHTS)
	{
	  color = fg;
	  fg = bg;
	  bg = color;
	}

	font = (font_t *)font_handle;
	
	wn_set_area(wd->wn, x, y, width, height, bg);
	
	wn_xwrite(wd->wn, font, text, len,
		  x,  y + baseline,
		  R_RPL, fg, bg, WN_USE_BASELINE, FALSE);

	if (flags & FLAG_DISABLED && !wn_use_alloc_color(0))
	{
		wn_shade_area(wd->wn, x, y, width, height, WN_GREY12, R_AND);
	}
}

static void
wt_set_area(data, x, y, width, height, on)
char *data;
int x, y, width, height;
bool on;
{
	Wdesc *wd = (Wdesc *)data;

	wn_set_area(wd->wn, x, y, width, height, on ? wd->fg : wd->bg);
}

static void
wt_show_cursor(data, font_handle, flags, x, y, baseline, width, height, ch, on)
char *data, *font_handle;
Edit_flags flags;
int x, y, baseline, width, height, ch;
bool on;
{
	Wdesc *wd = (Wdesc *)data;
	font_t *font;
	char c;
	long fg, bg;
	long sh_bg, sh_fg;

	c = ch;

	if (flags != 0 && flags != FLAG_HIGHLIGHTS) {
	  if (flags == FLAG_EDITLINE)	/* RGA attempt to get colored breakpoints */
	    get_editblock_colors(wd->wn, &bg, &fg);
	  else
	    if (flags == FLAG_VARIABLE)
	    {
	      get_variable_colors(wd->wn, &sh_bg, &sh_fg);
	      bg = sh_bg;
	      fg = sh_fg;
	    }
	    else
	      if (flags == FLAG_VALUE)
	      {
		get_value_colors(wd->wn, TRUE, &sh_fg, &sh_bg);
		bg = sh_bg;
		fg = sh_fg;
	      }
	      else
	      {
		fg = wd->bg;
		bg = wd->fg;
	    }
	}
	else {
	  fg = wd->fg;
	  bg = wd->bg;
	}

	font = (font_t *)font_handle;

	wn_xwrite(wd->wn, font, &c, 1,
		  x, y + baseline,
		  R_RPL, fg, bg, WN_USE_BASELINE, FALSE);

	if (on)
		wn_set_area(wd->wn, x, y, 2, height, fg);
}

static bool
wt_copy_area(data, old_x, old_y, new_x, new_y, width, height)
char *data;
int old_x, old_y, new_x, new_y, width, height;
{
	Wdesc *wd = (Wdesc *)data;

	wn_rop(wd->wn, old_x, old_y, width, height, new_x, new_y);

	return !wn_last_rop_was_damaged(wd->wn);
}

/*
**   RCB: Moved this here from ups/srcwin.c
*/
static void
get_editblock_colors(wn, p_bg, p_fg)
int wn;
long *p_bg, *p_fg;
{
  static color_t color = { -1, 0, 0, 0 };
 
  if (wn_use_mono())
    color.co_pixel = WN_FG(wn);
  else
    if (color.co_pixel == -1)
    {
      const char *colorname;

      colorname = wn_get_default("EditlinesColor");
      if (colorname == NULL)
	colorname = "blue";
      wn_parse_color(wn, colorname, &color);
      if (!wn_use_alloc_color(0))
      {
	if (wn_get_nplanes(wn) == 1 || wn_get_pixels_by_color(wn, &color, 1) != 0)
	  color.co_pixel = WN_FG(wn);
      }
      else
      {
	if (!wn_alloc_color(wn, &color,colorname))
	  color.co_pixel = WN_FG(wn);
      }
    }
  *p_fg = color.co_pixel;
  *p_bg = WN_BG(wn);
}

/*  Make the text between startpc and endpc the global selection.
**  Arrange that edit_clear_xclipboard will be called
**  when that selection is changed.  If want_text is set, return
**  the selection, otherwise return NULL.
*/
char* edit_set_global_selection(d, startpc, endpc,timestamp, want_text)
Edit_display* d;
Edit_propchange* startpc;
Edit_propchange* endpc;
long timestamp;
bool want_text;
{
    char *render_data;
    Edit_render_ops *render_ops;
       
    Edit_buffer* buffer = edit_get_buffer(d);
    Wdesc *wd;
    size_t start_point;
    size_t end_point;
    char *text;

    if (startpc == NULL)
	return NULL;

    edit_get_render_data_and_ops(d, &render_data, &render_ops);
    wd = (Wdesc *)render_data;
	
    start_point = edit_get_propchange_point(startpc);
    end_point = edit_get_propchange_point(endpc);
    text = edit_copy_bytes(buffer, start_point, end_point, (char *)NULL);
    wn_set_xclipboard (wd->wn,text, strlen(text), timestamp, edit_clear_xclipboard, d);
    d->sel_startpc = startpc;
    d->sel_endpc = endpc;
    if ( want_text)
       return text;
    free(text);
    return NULL;
}

/* if the display has the current global selection highlighted,
** unhighlight it.   ( Called by the window manager when the
** the global selection changes.)
*/
static void edit_clear_xclipboard( vd )
void* vd;
{
   Edit_display* d = (Edit_display*) vd;
   Edit_buffer* buffer = edit_get_buffer(d);
   edit_set_propchange_updates(buffer, FALSE);
   if ( d->sel_startpc)
	edit_delete_propchange(buffer, d->sel_startpc);
   if ( d->sel_endpc)
	edit_delete_propchange(buffer, d->sel_endpc);
   edit_set_propchange_updates(buffer, TRUE);
   d->sel_startpc = NULL;
   d->sel_endpc = NULL;
}

/* If the display currently has the global selection highlighted,
** unhighlight it.
*/
void
edit_unselect_global_selection(d)
Edit_display* d;
{
    if ( d->sel_startpc)
	wn_clr_xclipboard();
}


