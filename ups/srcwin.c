/* srcwin.c - source window code */

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


/* @(#)srcwin.c	1.7 24 May 1995 (UKC) */
char ups_srcwin_c_rcsid[] = "$Id$";

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>

#include <local/wn.h>
#include <local/ukcprog.h>
#include <local/obj/obj.h>
#include <local/obj/newtb.h>
#include <local/edit/edit.h>
#include <local/edit/win.h>
#include <local/edit/lbuf.h>
#include <mtrprog/utils.h>

#include "ups.h"
#include "reg.h"
#include "symtab.h"
#include "st.h"
#include "srcbuf.h"
#include "srcwin.h"
#include "target.h"
#include "srcpath.h"
#include "va_priv.h"
#include "ui.h"
#include "state.h"

void bpt_getcolor PROTO((objid_t obj, int wn, long *p_fg, long *p_bg));
void env_getcolor PROTO((objid_t obj, int wn, long *p_fg, long *p_bg));
void srcfile_getcolor PROTO((objid_t obj, int wn, long *p_fg, long *p_bg));
void func_getcolor PROTO((objid_t obj, int wn, long *p_fg, long *p_bg));
void sig_getcolor PROTO((objid_t obj, int wn, long *p_fg, long *p_bg));
void wpt_getcolor PROTO((objid_t obj, int wn, long *p_fg, long *p_bg));

#define TO_1BASE(lnum)	((lnum) + 1)
#define TO_0BASE(lnum)	((lnum) - 1)

#ifndef INT_MAX
#define INT_MAX	((int)~((unsigned)0 >> 1))
#endif

Edit_history* breakpoint_history = NULL;
typedef struct {
	Edit_fontinfo *fontinfo;
	Edit_flags flagmask;
	Edit_flags flags;
	Edit_buffer *buffer;
	Edit_propchange *start;
	Edit_propchange *lim;
	int old_visible_units;	/* for restoring tbar after search */
	int old_total_units;	/* for restoring tbar after search */
	int puck_x;		/* for selecting obj in overlay, after search */
	int puck_y;		/* for selecting obj in overlay, after search */
	int found;		/* for selecting obj in overlay, after search */
} Highlighting;

/*  Element in the list built by src_push_current_pos and used by src_pop().
 */
typedef struct histst {
	fil_t *fil;
	int y_offset;
	size_t start;
	struct histst *next;
} hist_t;

struct Outwin {
	tbar_id_t tbar;
	int wn;
	int fg;
	int bg;
	Highlighting search_hl;
	Edit_display *display;
	Edit_buffer *buffer;
	char *data;
	Outwin *next;
};

/*  Poor man's inheritance.
 */
struct Srcwin {
	Outwin outwin;
	fil_t *fil;
	hist_t *hist;
};

static void set_highlighting_style PROTO((Highlighting *hl, font_t *font,
					  Edit_flags flagmask,
					  Edit_flags flags));
static void trim_whitespace PROTO((Edit_buffer *buffer, 
				   const char *name, int fil_lnum, 
				   size_t *p_start, size_t *p_lim));
static bool file_lnum_to_point_range PROTO((fil_t *fil, int lnum,
					    size_t *p_start, size_t *p_lim));
static int point_to_ypos PROTO((fil_t *fil, size_t point));
static bool get_buffer PROTO((fil_t *fil, Edit_buffer **p_buffer));
static void adjust_sw_tbar_total_units PROTO((tbar_id_t tbar,
					      int old_eb_height, 
					      Editblock *eb));
static void ensure_sw_point_visible PROTO((Srcwin *sw, size_t point));
static void ensure_ow_point_visible PROTO((Outwin *sw, size_t point));
static void clear_highlighting PROTO((Highlighting *hl));
static void set_highlighting PROTO((Highlighting *hl, Edit_buffer *buffer, 
                                    size_t start, size_t lim, bool clear));
static void set_coloring PROTO((Highlighting *hl, Edit_buffer *buffer,
				size_t start, size_t lim, Edit_flags flagmask,
				Edit_flags flags));
static void set_coloring_on PROTO((Highlighting *hl, Edit_buffer *buffer,
				size_t start, size_t lim, Edit_flags flagmask,
				Edit_flags flags));
static void set_coloring_off PROTO((Highlighting *hl, Edit_buffer *buffer,
				size_t start, size_t lim, Edit_flags flagmask));
static void init_outwin PROTO((Outwin *ow, int wn, tbar_id_t tbar,
			       Edit_buffer *buffer, int fold_lines, Edit_history*));
static bool set_point PROTO((Outwin *ow, size_t point, Region *region, 
                             char *data));
static size_t update_ow_tbar PROTO((Outwin *ow, bool set_pos));
static void set_sw_fil PROTO((Srcwin *sw, fil_t *fil, Edit_buffer *buffer));
static void newline_and_indent PROTO((Edit_display *display));
static void electric_c_brace PROTO((Edit_display *display));
static void init_display_area_overlay PROTO((Outwin *dw, int wn,
					     tbar_id_t tbar,
					     Edit_buffer *buffer));


/*  List of all display windows (source and output).
 */
static Outwin *All_windows = NULL;

static Highlighting Stopline;

static Edit_fontinfo *Srcfont_info, *Editfont_info, *Sysfont_info;
static int Srcfont_height, Editfont_height, Sysfont_width;

static bool Want_display_checks = FALSE;
static int Tabspaces = 8;
static int overlay_last_flag = 0, overlay_last_buff_len, overlay_last_len;

void
display_overlay_set_font(font)
font_t *font;
{
	Sysfont_info = edit_make_wn_fontinfo(font);
	Sysfont_width = font->ft_width;
/*	set_highlighting_style(&Display_overlay_line, (font_t *)NULL, FLAG_HIGHLIGHT2, FLAG_HIGHLIGHT2);*/
}

void
srcwin_set_fonts(srcfont, editfont, stopline_font)
font_t *srcfont, *editfont, *stopline_font;
{
	Srcfont_info = edit_make_wn_fontinfo(srcfont);
	Srcfont_height = srcfont->ft_height;

	Editfont_info = edit_make_wn_fontinfo(editfont);
	Editfont_height = editfont->ft_height;
	
	set_highlighting_style(&Stopline, (font_t *)NULL, FLAG_HIGHLIGHT1, FLAG_HIGHLIGHT1);
}

static void
set_highlighting_style(hl, font, flagmask, flags)
Highlighting *hl;
font_t *font;
Edit_flags flagmask, flags;
{
	hl->fontinfo = (font != NULL) ? edit_make_wn_fontinfo(font) : NULL;
	hl->flagmask = flagmask;
	hl->flags = flags;
	hl->buffer = NULL;
	hl->start = NULL;
	hl->lim = NULL;
}

void
srcwin_set_tabwidth(tabwidth)
int tabwidth;
{
	Tabspaces = tabwidth;
}

void
srcwin_set_want_checks_flag()
{
	Want_display_checks = TRUE;
}

Srcwin *
srcwin_create(wn, tbar)
int wn;
tbar_id_t tbar;
{
	Srcwin *sw;
	Edit_keymap *keymap;

	sw = (Srcwin *)e_malloc(sizeof(Srcwin));
	init_outwin(&sw->outwin, wn, tbar, (Edit_buffer *)NULL, TRUE, breakpoint_history); /* folded lines, history */
	sw->fil = NULL;
	sw->hist = NULL;
	
	keymap = edit_create_keymap(edit_get_keymap(sw->outwin.display));
	edit_add_keymap_control_entry(keymap, 'j', newline_and_indent);
	edit_add_keymap_entry(keymap, 0, '}', electric_c_brace);
	edit_add_keymap_entry(keymap, EDIT_SHIFT, '}', electric_c_brace);
	edit_set_keymap(sw->outwin.display, keymap);

	return sw;
}

static void
electric_c_brace(display)
Edit_display *display;
{
	Edit_buffer *buffer;
	size_t sol, lim, point;

	buffer = edit_get_buffer(display);
	point = edit_get_point(display);

	if (edit_find_char_backwards(buffer, point, 0, "\n", &sol))
		++sol;

	edit_find_char_forwards(buffer, sol, point, "! \t", &lim);

	if (lim > sol && lim == point) {
		edit_delete(buffer, point - 1, 1);
		--point;
	}

	edit_insert(buffer, point, "}", 1, TRUE);
}

static void
newline_and_indent(display)
Edit_display *display;
{
	Edit_buffer *buffer;
	size_t sol, lim, point, wslen;
	char c;
	char *ws;

	buffer = edit_get_buffer(display);
	point = edit_get_point(display);

	if (edit_find_char_backwards(buffer, point, 0, "\n", &sol))
		++sol;

	edit_find_char_forwards(buffer, sol, EDIT_MAX_POINT, "! \t", &lim);

	if (point == 0)
		c = '\0';
	else
		edit_copy_bytes(buffer, point - 1, point, &c);
	
	wslen = lim - sol;
	ws = e_malloc(wslen + 2);	/* newline, whitespace, maybe tab */

	ws[0] = '\n';
	edit_copy_bytes(buffer, sol, lim, ws + 1);

	
	if (c == '{') {
		ws[wslen + 1] = '\t';
		++wslen;
	}
	else if (c == '}' && wslen != 0) {
		--wslen;
	}
	
	edit_insert(buffer, point, ws, wslen + 1, TRUE);

	free(ws);
}

Outwin *
outwin_create(wn, tbar)
int wn;
tbar_id_t tbar;
{
	Outwin *ow;
	Text_buffer *tb;
	Edit_buffer *buffer;

	tb = text_create_empty_lbuf_buffer(alloc_create_pool(), 0, 1024);
	buffer = edit_create_buffer(tb, Srcfont_info, 0);
	
	ow = (Outwin *)e_malloc(sizeof(Outwin));
	init_outwin(ow, wn, tbar, buffer, TRUE, NULL); /* Folded lines, no history */

	ow->next = All_windows;
	All_windows = ow;

	return ow;
}

static void
init_outwin(ow, wn, tbar, buffer, fold_lines, history)
Outwin *ow;
int wn;
tbar_id_t tbar;
Edit_buffer *buffer;
int fold_lines;
Edit_history* history;
{
	Edit_display *display;

	if (wn == -1) {
		display = NULL;
	}
	else {
		display = edit_create_wn_display(wn, WN_FG(wn), WN_BG(wn),
						 FALSE, FALSE);

		/*
		**  RCB ( 4/15/99):
		**   There seems to be a bug in the redraw code if lines
		**   are folded and the top most displayed line is the
		**   tail end of a folded line.  Streching the window
		**   to un-fold the lines leaves the top most displayed
		**   line unchanged.  A subsequent scroll gets into an
		**   infinite loop.  Rather than track down the bug, I
		**   turned off the line folding for the output window.
		**
		**   On the other hand, not folding lines in the source display
		**   messes up the highlighting of the highlighting of the
		**   active line.
		**
		**  RGA ( 6/17/99):
		**   Restored the folded lines for the output window.
		**   Eliminated the infinite loop by setting the window to
		**   the top when resizing, then restoring the postion
		**   after the redraw. See code in srcwin_draw() and 
		**   outwin_draw().
		*/
		edit_use_folded_lines(display, Tabspaces, Want_display_checks,
				      fold_lines);

		if (buffer != NULL)
			edit_set_buffer(display, buffer);
		edit_set_history(display, history);
	}
	
	ow->tbar = tbar;
	ow->display = display;
	ow->buffer = buffer;
	ow->wn = wn;
	ow->fg = WN_FG(wn);
	ow->bg = WN_BG(wn);
	ow->data = NULL;
	set_highlighting_style(&ow->search_hl, (font_t *)NULL, FLAG_HIGHLIGHT2, FLAG_HIGHLIGHT2);
}

void
srcwin_set_data(sw, data)
Srcwin *sw;
char *data;
{
	sw->outwin.data = data;
}

char *
srcwin_get_data(sw)
Srcwin *sw;
{
	return sw->outwin.data;
}

long
srcwin_draw_arrow(sw, fil_lnum)
Srcwin *sw;
int fil_lnum;
{
	static unsigned short bits[] = {
		0x0000, 0x0000, 0x0400, 0x0c00,
		0x1c00, 0x3fff, 0x7c00, 0x3fff,
		0x1c00, 0x0c00, 0x0400, 0x0000,
		0x0000, 0x0000, 0x0000, 0x0000
	};
#define ARROW_X_HOT 15
#define ARROW_Y_HOT 6
	static bitmap_t arrow = wn_static_bm(16, 16, 1, 0, 0,
					     BM_BIT0_LEFT, bits);
	int x, y, width, height, baseline;
	size_t point, start, lim;
	Edit_buffer *buffer;
	long sa;

	if (!file_lnum_to_point_range(sw->fil, TO_0BASE(fil_lnum),
				      &start, &lim))
		panic("fil_lnum botch in sda");

	get_buffer(sw->fil, &buffer);
	
	if (edit_find_char_forwards(buffer, start, lim, "! \t", &point))
		start = point;

	edit_ensure_visible(sw->outwin.display, start);

	if (!edit_point_to_pixel(sw->outwin.display, start,
				&x, &y, &width, &height, &baseline))
		panic("visibility botch in sda");

	x -= ARROW_X_HOT + 5;
	y += height / 2 - ARROW_Y_HOT;

	sa = wn_save_area(sw->outwin.wn, x, y, 16, 16);
	wn_put_image(&arrow, 0, 0, 16, 16, sw->outwin.wn, x, y, R_RPL, sw->outwin.fg, sw->outwin.bg);
	
	return sa;
}

void
srcwin_search(sw, pattern, forwards)
Srcwin *sw;
const char *pattern;
bool forwards;
{
  if (sw->outwin.buffer != NULL)
	outwin_search(&sw->outwin, pattern, forwards);
}

void
outwin_search(ow, pattern, forwards)
Outwin *ow;
const char *pattern;
bool forwards;
{
	size_t dstart, dlim, fstart, flim, point;
	int pixel_offset;
	
	edit_get_display_info(ow->display, &pixel_offset, &dstart, &dlim);
	
	if (ow->search_hl.start != NULL) {
		size_t start, lim;
		bool old_visible;
		
		start = edit_get_propchange_point(ow->search_hl.start);
		lim = edit_get_propchange_point(ow->search_hl.lim);

		old_visible = start >= dstart && start < dlim;

		if (forwards)
			point = old_visible ? lim : dstart;
		else
			point = old_visible ? start : dstart;
	}
	else {
		point = dstart;
	}

	if (search_buffer(ow->buffer, pattern, point, forwards,
			  &fstart, &flim, TRUE)) {
		ensure_ow_point_visible(ow, fstart);
		set_highlighting(&ow->search_hl, ow->buffer, fstart, flim, TRUE);
	}
}

void
srcwin_redraw(sw)
Srcwin *sw;
{
	outwin_redraw(&sw->outwin);
}

void
outwin_redraw(ow)
Outwin *ow;
{
	int width, height;
	
	edit_update_wn_window_size(ow->display, ow->wn);
	edit_redraw_display(ow->display);

	edit_get_display_size(ow->display, &width, &height);
	tb_set_visible_units(ow->tbar, height);
}

/* De-iconize and raise the root window containing the region */
void outwin_raise(ow)
Outwin* ow;
{
    if ( ow && ow->wn >= 0)
	wn_raise_window(ow->wn);
}

void outwin_lower(ow)
Outwin* ow;
{
    if ( ow && ow->wn >= 0)
        wn_lower_window(ow->wn);
}

void outwin_iconify(ow)
Outwin* ow;
{
    if ( ow && ow->wn >= 0)
        wn_iconify_window(ow->wn);
}


void outwin_map(ow)
Outwin* ow;
{
    if ( ow && ow->wn >= 0)
        wn_map_window(ow->wn);
}

void outwin_unmap(ow)
Outwin* ow;
{
    if ( ow && ow->wn >= 0)
        wn_unmap_window(ow->wn);
}


int
srcwin_get_window_height(sw)
Srcwin *sw;
{
	return outwin_get_window_height(&sw->outwin);
}

int
outwin_get_window_height(ow)
Outwin *ow;
{
	int width, height;
	
	wn_get_window_size(ow->wn, &width, &height);
	return height;
}

/*  Fill in the srcinfo_t block pointed to by si with information about
 *  what is under position x,y in the source window.
 *
 *  If x is zero or positive, y is taken as relative to the top of the window,
 *  otherwise y is taken as relative to the start of the source file.
 */
void
srcwin_get_info(sw, x, y, si)
Srcwin *sw;
int x, y;
Srcinfo *si;
{
	si->fil = sw->fil;

	if (si->fil == NULL) {
		si->have_point = FALSE;
		si->fil_lnum = 0;
	}
	else {
		int lnum;
	
		si->have_point = edit_pixel_to_point(sw->outwin.display,
						     x, y, &si->point);
	
		if (si->have_point &&
		    srcbuf_point_to_file_lnum(sw->fil->fi_srcbuf, si->point,
					      &lnum)) {
			si->fil_lnum = TO_1BASE(lnum);
		}
		else {
			int junk_ypos;
		
			srcbuf_count_pixels(sw->fil->fi_srcbuf, EDIT_MAX_POINT,
                                            y,
					    Editfont_height, Srcfont_height,
					    &si->fil_lnum, &junk_ypos);
		}
	}
}

Edit_display *
srcwin_get_display(sw)
Srcwin *sw;
{
	return outwin_get_display(&sw->outwin);
}

Edit_display *
outwin_get_display(ow)
Outwin *ow;
{
	return ow->display;
}

Edit_buffer *
outwin_get_buffer(ow)
Outwin *ow;
{
	return ow->buffer;
}

void
srcwin_clear_history(sw)
Srcwin *sw;
{
	hist_t *hist, *next;

	hist = sw->hist;
	
	for (hist = sw->hist; hist != NULL; hist = next) {
		next = hist->next;
		free((char *)hist);
		hist = next;
	}
	
	sw->hist = NULL;
}

void
srcwin_push_current_pos(sw)
Srcwin *sw;
{
	hist_t *hist;

	if (sw->fil != NULL) {
		int y_offset;
		size_t start, lim;
		
		edit_get_display_info(sw->outwin.display,
				      &y_offset, &start, &lim);
		
		hist = (hist_t *)e_malloc(sizeof(hist_t));
		hist->fil = sw->fil;
		hist->start = start;
		hist->y_offset = y_offset;
		hist->next = sw->hist;
		sw->hist = hist;
	}
}

void
srcwin_pop(sw)
Srcwin *sw;
{
	hist_t *hist;
	Edit_buffer *buffer;

	if (sw->hist == NULL) {
		errf("No previous position to go back to");
		return;
	}

	if (!get_buffer(sw->hist->fil, &buffer))
		return;
	
	hist = sw->hist;
	sw->hist = hist->next;

	if (sw->fil != hist->fil)
		set_sw_fil(sw, hist->fil, buffer);
	
	edit_display_from(sw->outwin.display, hist->y_offset, hist->start);
	tb_goto(sw->outwin.tbar,
		point_to_ypos(sw->fil, hist->start) - hist->y_offset, FALSE);
	
	free((char *)hist);
}

static void
set_sw_fil(sw, fil, buffer)
Srcwin *sw;
fil_t *fil;
Edit_buffer *buffer;
{
	const char *path;
	Outwin *ow;
	struct stat stbuf;
	time_t mtime;
	target_t *xp;
	int last_fg;

	ow = &sw->outwin;

	path = srcbuf_get_path(fil->fi_srcbuf);
	
	if (stat(path, &stbuf) != 0) {
		failmesg("Can't stat", "source file", path);
		mtime = 0;
	}
	else {
		mtime = stbuf.st_mtime;
	}

	xp = get_current_target();
	
	if (fil && fil->fi_symtab && fil->fi_symtab->st_modtime != 0)
	{
	  if (mtime > fil->fi_symtab->st_modtime)
	  {
	    errf("\bWarning: %s is newer than %s", path, fil->fi_symtab->st_path);
	    mtime = 0;
	  }
	}
	else
	{
	  if (mtime > get_current_target()->xp_modtime)
	  {
	    errf("\bWarning: %s is newer than %s", path, xp->xp_textpath);
	    mtime = 0;
	  }
	}

	last_fg = ow->fg;
	
	if (mtime == 0) {
		ow->fg = WN_BG(ow->wn);
		ow->bg = WN_FG(ow->wn);
	}
	else {
		ow->fg = WN_FG(ow->wn);
		ow->bg = WN_BG(ow->wn);
	}

	if (ow->fg != last_fg) {
		int width, height;

		/*  BUG: should be able to tell edit code to forget
		 *       cached lines.
		 */
		wn_get_window_size(ow->wn, &width, &height);
		wn_set_area(ow->wn, 0, 0, width, height, ow->bg);
		
		edit_update_wn_window_colors(ow->display, ow->fg, ow->bg);

		/* RGA force a redraw for duplicate files in which */
		/* the text is identical, but backgrounds differ. */
		/* The render routines optimize out the redraw because */
		/* there is no change in text. Problem seen when clicking */
		/* from one breakpoint to its duplicate */
		srcwin_redraw(sw);
	}
	
	ow->buffer = buffer;
	edit_set_buffer(ow->display, buffer);
		
	tb_set_total_units(ow->tbar, point_to_ypos(fil, EDIT_MAX_POINT));
		
	fil->fi_flags &= ~FI_NEEDS_RESCAN;
	sw->fil = fil;
}

/*  Display source so that line fil_lnum of file fil is visible in the source
 *  window.
 */
bool
srcwin_show(sw, fil, fil_lnum)
Srcwin *sw;
fil_t *fil;
int fil_lnum;
{
	size_t start, lim;
	Edit_buffer *buffer;

	clear_highlighting(&sw->outwin.search_hl);

	fil_lnum = TO_0BASE(fil_lnum);

	if (!get_buffer(fil, &buffer))
		return FALSE;
	
	if (fil->fi_flags & FI_NEEDS_RESCAN || fil != sw->fil)
		set_sw_fil(sw, fil, buffer);
	
	if (!file_lnum_to_point_range(fil, fil_lnum, &start, &lim))
	{
	  ensure_sw_point_visible(sw, start); /* RGA show what it can anyway.. */
	  return FALSE;
	}

	ensure_sw_point_visible(sw, start);
	
	return TRUE;
}

static void
ensure_sw_point_visible(sw, point)
Srcwin *sw;
size_t point;
{
	if (edit_ensure_visible(sw->outwin.display, point)) {
		size_t start, lim;
		int y_offset;
	
		edit_get_display_info(sw->outwin.display, &y_offset,
				      &start, &lim);
		tb_goto(sw->outwin.tbar,
			point_to_ypos(sw->fil, start) - y_offset, FALSE);
	}
}

static void
ensure_ow_point_visible(ow, point)
Outwin *ow;
size_t point;
{
	edit_ensure_visible(ow->display, point);
	update_ow_tbar(ow, TRUE);
}

Editblock *
srcwin_add_editblock(sw, fil, fil_lnum, text,
		     callback, change_callback,enabled, user_data)
Srcwin *sw;
fil_t *fil;
int fil_lnum;
char *text;
Editblock_callback callback;
Editblock_change_callback change_callback;
bool enabled;
char *user_data;
{
	Editblock *eb;

	if (!open_source_file(fil, FALSE, FALSE))
		return NULL;

				/* RGA try to get color edit text */
				/* RCB: Add enabled_callback */
	eb = srcbuf_add_editblock(fil->fi_srcbuf, TO_0BASE(fil_lnum),
				  FLAG_EDITLINE|FLAG_DISABLED,
				  enabled?FLAG_EDITLINE:FLAG_DISABLED,
				  Editfont_info, Srcfont_info,
				  text, callback, change_callback, user_data);

	if (!eb)
		return NULL;
	if (sw != NULL) {
		/*  If we are not about to switch windows we need to nudge the
		 *  thumb bar height.
		 */
		if (fil == sw->fil)
			adjust_sw_tbar_total_units(sw->outwin.tbar, 0, eb);
	
		srcwin_show(sw, fil, fil_lnum);
	}

	return eb;
}

/* RCB: New */
void  srcwin_enable_editblock(eb, enabled)
Editblock* eb;
bool enabled;
{
   if ( eb)
     srcbuf_editblock_change_flags(eb, enabled?FLAG_EDITLINE:FLAG_DISABLED);
}


static void
adjust_sw_tbar_total_units(tbar, old_eb_height, eb)
tbar_id_t tbar;
int old_eb_height;
Editblock *eb;
{
	int total_height, eb_height;
		
	total_height = tb_get_total_units(tbar);
		
	eb_height = srcbuf_get_editblock_height(eb, EDIT_MAX_POINT,
					     Editfont_height, Srcfont_height);


	if (eb_height != old_eb_height) {
		tb_set_total_units(tbar,
				   total_height + (eb_height - old_eb_height));
	}
}

static int
point_to_ypos(fil, point)
fil_t *fil;
size_t point;
{
	int ypos, junk_lnum;
	
	srcbuf_count_pixels(fil->fi_srcbuf, point, INT_MAX,
			    Editfont_height, Srcfont_height,
			    &junk_lnum, &ypos);

	return ypos;
}
		
static bool
file_lnum_to_point_range(fil, lnum, p_start, p_lim)
fil_t *fil;
int lnum;
size_t *p_start, *p_lim;
{
	if (lnum < 0) {
		errf("Line number %d less than one for %s", TO_1BASE(lnum),
		     fil->fi_name);
		return FALSE;
	}
	
	if (!srcbuf_file_lnum_to_point_range(fil->fi_srcbuf, lnum,
					     p_start, p_lim)) {
		errf("Can't display line %d of %s - it only has %ld lines",
		     TO_1BASE(lnum), fil->fi_name,
		     (long)srcbuf_nlines_in_file(fil->fi_srcbuf));
		return FALSE;
	}

	return TRUE;
}

static bool
get_buffer(fil, p_buffer)
fil_t *fil;
Edit_buffer **p_buffer;
{
	if (!open_source_file(fil, FALSE, FALSE))
		return FALSE;

	*p_buffer = srcbuf_get_buffer(fil->fi_srcbuf);
	return TRUE;
}

Srcbuf *
srcwin_visit_file(ap, what, path)
alloc_pool_t *ap;
const char *what, *path;
{
	return srcbuf_visit_file(ap, what, path, Srcfont_info, 0);
}

bool
srcwin_set_stopline(fil, fil_lnum)
fil_t *fil;
int fil_lnum;
{
	Edit_buffer *buffer;
	size_t start, lim;
	Outwin *ow;

	for (ow = All_windows; ow != NULL; ow = ow->next)
		clear_highlighting(&ow->search_hl);
	
	if (fil == NULL ||
	    !get_buffer(fil, &buffer) ||
	    !file_lnum_to_point_range(fil, TO_0BASE(fil_lnum), &start, &lim)) {
		clear_highlighting(&Stopline);
		return FALSE;
	}

	/*  Don't highlight whitespace.
	 */
	trim_whitespace(buffer, fil->fi_name, fil_lnum, &start, &lim);
	
	set_highlighting(&Stopline, buffer, start, lim, TRUE);
	return TRUE;
}

static void
clear_highlighting(hl)
Highlighting *hl;
{
	if (hl->buffer != NULL) {
		bool oldval;
		
		oldval = edit_set_propchange_updates(hl->buffer, FALSE);
		if (hl->start)
		    edit_delete_propchange(hl->buffer, hl->start);
		if (hl->lim)
		    edit_delete_propchange(hl->buffer, hl->lim);
		edit_set_propchange_updates(hl->buffer, oldval);

		hl->buffer = NULL;
		hl->start = hl->lim = NULL;
	}
}

static void
set_highlighting(hl, buffer, start, lim, clear)
Highlighting *hl;
Edit_buffer *buffer;
size_t start, lim;
bool clear;
{
	bool oldval;

	if (clear)
	  clear_highlighting(hl);
		
	oldval = edit_set_propchange_updates(buffer, FALSE);
	
	hl->buffer = buffer;
	
	hl->start = edit_add_propchange(buffer, start, (char *)NULL,
					hl->fontinfo, hl->flagmask,
					hl->flags, FALSE);
	
	hl->lim = edit_add_propchange(buffer, lim, (char *)NULL,
				      hl->fontinfo, hl->flagmask,
				      0, TRUE);
	
	edit_set_propchange_updates(buffer, oldval);
}

static void
set_coloring_on(hl, buffer, start, lim, flagmask, flags)
Highlighting *hl;
Edit_buffer *buffer;
size_t start, lim;
Edit_flags flagmask;
Edit_flags flags;
{
	hl->buffer = buffer;
	
	edit_add_propchange(buffer, start, (char *)NULL,
			    hl->fontinfo, flagmask,
			    flags, FALSE);
}

static void
set_coloring_off(hl, buffer, start, lim, flagmask)
Highlighting *hl;
Edit_buffer *buffer;
size_t start, lim;
Edit_flags flagmask;
{
	hl->buffer = buffer;
	
	edit_add_propchange(buffer, lim, (char *)NULL,
			    hl->fontinfo, flagmask,
			    0, TRUE);
}

static void
set_coloring(hl, buffer, start, lim, flagmask, flags)
Highlighting *hl;
Edit_buffer *buffer;
size_t start, lim;
Edit_flags flagmask;
Edit_flags flags;
{
	hl->buffer = buffer;
	
	edit_add_propchange(buffer, start, (char *)NULL,
			    hl->fontinfo, flagmask,
			    flags, FALSE);
	
	edit_add_propchange(buffer, lim, (char *)NULL,
			    hl->fontinfo, flagmask,
			    0, TRUE);
}

static void
trim_whitespace(buffer, name, fil_lnum, p_start, p_lim)
Edit_buffer *buffer;
const char *name;
int fil_lnum;
size_t *p_start, *p_lim;
{
	const char *iptr, *ilim;
	size_t point, first_letter_point, last_letter_point;
	
	iptr = ilim = NULL;

	first_letter_point = last_letter_point = *p_lim;
	
	for (point = *p_start; point < *p_lim; ++point, ++iptr) {
		if (iptr == ilim &&
		    !edit_get_bytes(buffer, point, &iptr, &ilim))
			break;
		
		if (!isspace(*iptr)) {
			if (first_letter_point == *p_lim)
				first_letter_point = point;
			
			last_letter_point = point;
		}
	}


	if (first_letter_point == *p_lim) {
		/*  Lost in space.  Nobody can hear us scream,
		 *  so don't beep.
		 */
		errf("\bWarning: stopped at %s line %d, but %s",
		     name, TO_1BASE(fil_lnum), "that line is blank");
	}
	else {
		*p_start = first_letter_point;
		*p_lim = last_letter_point + 1;
	}
}

size_t
outwin_goto_point(ow, point)
Outwin *ow;
size_t point;
{
	edit_display_from(ow->display, 0, point);
	return update_ow_tbar(ow, FALSE);
}

void
outwin_goto_point_and_update(ow, point)
Outwin *ow;
size_t point;
{
	edit_display_from(ow->display, 0, point);
	update_ow_tbar(ow, TRUE);
}

size_t
outwin_get_buffer_length(ow)
Outwin *ow;
{
  return edit_get_buffer_length(ow->buffer);
}


static size_t
update_ow_tbar(ow, set_pos)
Outwin *ow;
bool set_pos;
{
	size_t start, lim, buflen;
	int pixel_offset;
	
	edit_get_display_info(ow->display, &pixel_offset, &start, &lim);
	buflen = edit_get_buffer_length(ow->buffer);

	tb_set_visible_units(ow->tbar, (int)(lim - start));
	tb_set_total_units(ow->tbar, (int)buflen);
	
	if (set_pos)
		tb_goto(ow->tbar, (int)start, FALSE);

	return start;
}

size_t
outwin_get_pos(ow)
Outwin *ow;
{
	size_t start, lim;
	int pixel_offset;
	
	edit_get_display_info(ow->display, &pixel_offset, &start, &lim);
	return start;
}

void
outwin_insert(ow, text, len)
Outwin *ow;
const char *text;
size_t len;
{
	edit_insert(ow->buffer, edit_get_buffer_length(ow->buffer), text,
		    len, TRUE);

	if (ow->display != NULL) {
		edit_set_point(ow->display, edit_get_buffer_length(ow->buffer));
		ensure_ow_point_visible(ow, edit_get_point(ow->display));
	}
}

void
outwin_insert_string(text)
const char *text;
{
  Outwin *ow;

  ow = get_or_create_outwin_unmapped();
  edit_insert(ow->buffer, edit_get_buffer_length(ow->buffer), text,
	      (size_t)strlen(text), TRUE);
  
  if (ow->display != NULL) {
    edit_set_point(ow->display, edit_get_buffer_length(ow->buffer));
    ensure_ow_point_visible(ow, edit_get_point(ow->display));
  }
}

void
outwin_delete_all_text(ow)
Outwin *ow;
{
	clear_highlighting(&ow->search_hl);
	
	edit_delete(ow->buffer, 0, edit_get_buffer_length(ow->buffer));

	if (ow->tbar != NULL)
		update_ow_tbar(ow, TRUE);
}

void
outwin_putc(ow, ch)
Outwin *ow;
int ch;
{
	char c;

	c = ch;
	outwin_insert(ow, &c, 1);
}

size_t
srcwin_get_pos(sw)
Srcwin *sw;
{
	size_t start, lim;
	int pixel_offset;
	
	edit_get_display_info(sw->outwin.display, &pixel_offset, &start, &lim);
	return start;
}

void
srcwin_goto_point(sw, point)
Srcwin *sw;
size_t point;
{
	edit_display_from(sw->outwin.display, 0, point);
}

int
srcwin_scroll(sw, npixels)
Srcwin *sw;
int npixels;
{
	return edit_scroll_display(sw->outwin.display, npixels);
}

int
outwin_scroll(ow, npixels)
Outwin *ow;
int npixels;
{
	int val, pixel_offset;
	size_t start, lim;

	edit_get_display_info(ow->display, &pixel_offset, &start, &lim);
	
	val = edit_scroll_display(ow->display, npixels);

	if (val == 0)
		return 0;
	
	return update_ow_tbar(ow, FALSE) - start;
}

int
srcwin_goto_ypos(sw, ypos)
Srcwin *sw;
int ypos;
{
	size_t start, lim;
	int fil_lnum;

	clear_highlighting(&sw->outwin.search_hl);
	
	if (sw->fil == NULL)
		return 0;

	/*  TODO: this breaks if edit lines are a different height than
	 *        source lines.
	 */
	fil_lnum = ypos / Srcfont_height;

	if (file_lnum_to_point_range(sw->fil, fil_lnum, &start, &lim))
		edit_display_from(sw->outwin.display, 0, start);

	return fil_lnum * Srcfont_height;
}

void
outwin_set_point(ow, region, ev)
Outwin *ow;
Region *region;
event_t *ev;
{
	size_t point;
	
	if (edit_pixel_to_point(ow->display, ev->ev_x, ev->ev_y, &point))
		set_point(ow, point, region, (char *)NULL);
}

static bool
set_point(ow, point, region, data)
Outwin *ow;
size_t point;
Region *region;
char *data;
{
	Region *old_region;
	char *old_data;
	
	old_region = re_get_keyboard_focus_region();
	old_data = re_get_keyboard_focus_data(old_region);

	if (old_region != region || old_data != data) {
		if (!re_set_keyboard_focus(region, data))
			return FALSE;
		
		clear_highlighting(&ow->search_hl);
		
		edit_set_want_cursor(ow->display, TRUE);
	}

	edit_move_point(ow->display, point);

	return TRUE;
}

int
srcwin_set_point(sw, region, ev)
Srcwin *sw;
Region *region;
event_t *ev;
{
	Editblock *eb = NULL;
	Edit_propchange *pc;
	size_t point;

	if (!edit_pixel_to_point(sw->outwin.display, ev->ev_x, ev->ev_y,
				 &point))
		return 0;

	/*  Need to do this here so we don't get the wrong propchange
	 *  below.
	 */
	clear_highlighting(&sw->outwin.search_hl);
	
	pc = srcbuf_get_propchange_at_point(sw->fil->fi_srcbuf, point);
	if ( pc)
	    eb = (Editblock *)edit_get_propchange_data(pc);

	if (eb == NULL) {
	  /*		errf("Not in an editable section");*/
	  /*		wn_wait_for_release_of(ev->ev_wn, B_ANY);*/
	  return 0;		/* RGA allow middle click to uses tags */
	}
	else {
		srcwin_start_edit(sw, region, eb, point);
	}
	return 1;
}

void
srcwin_start_edit(sw, region, eb, point)
Srcwin *sw;
Region *region;
Editblock *eb;
size_t point;
{
	size_t start, lim;
	Edit_buffer *buffer;
		
	get_buffer(sw->fil, &buffer);
	
	if (!set_point(&sw->outwin, point, region, (char *)eb))
		return;

	srcbuf_select_editblock(eb, TRUE);
		
	srcbuf_get_editblock_point_range(eb, &start, &lim);

	if (lim != 0) {
		char c;
			
		edit_copy_bytes(buffer, lim - 1, lim, &c);

		if (c == '\n')
			--lim;
	}
		
	edit_set_point_limits(sw->outwin.display, start, lim);
}

void
outwin_handle_key_event(ow, region, ev)
Outwin *ow;
Region *region;
event_t *ev;
{
	size_t start, lim, buflen;
	int pixel_offset;
	
	wn_updating_off(ev->ev_wn);

	clear_highlighting(&ow->search_hl);
	
	edit_handle_wn_key_event(ow->display, ev);

	edit_get_display_info(ow->display, &pixel_offset, &start, &lim);
	buflen = edit_get_buffer_length(ow->buffer);
	
	tb_set_visible_units(ow->tbar, (int)(lim - start));
	tb_set_total_units(ow->tbar, (int)buflen);
	
	wn_updating_on(ev->ev_wn);
}

void
srcwin_handle_key_event(sw, region, ev)
Srcwin *sw;
Region *region;
event_t *ev;
{
	int old_eb_height;
	Editblock *eb;

	eb = (Editblock *)re_get_keyboard_focus_data(region);

	old_eb_height = srcbuf_get_editblock_height(eb, EDIT_MAX_POINT,
						    Editfont_height,
						    Srcfont_height);

	wn_updating_off(ev->ev_wn);
	edit_handle_wn_key_event(sw->outwin.display, ev);
	adjust_sw_tbar_total_units(sw->outwin.tbar, old_eb_height, eb);
	wn_updating_on(ev->ev_wn);

	if (edit_quit_requested(sw->outwin.display))
		re_set_keyboard_focus((Region *)NULL, (char *)NULL);
}

bool
outwin_release_focus(region, data, force)
Region *region;
char *data;
bool force;
{
	Outwin *ow;
	
	ow = (Outwin *)re_get_data(region);
	
	edit_set_want_cursor(ow->display, FALSE);
	return TRUE;
}

bool
srcwin_release_focus(region, data, force)
Region *region;
char *data;
bool force;
{
	Srcwin *sw;
	Editblock *eb;
	size_t error_point;

	sw = (Srcwin *)re_get_data(region);
	eb = (Editblock *)data;

	edit_set_want_cursor(sw->outwin.display, FALSE);

	if (srcbuf_change_editblock(eb, force, &error_point))
	{
		edit_save_in_history(sw->outwin.display);
		return TRUE;
	}
	
	edit_set_mark(sw->outwin.display, edit_get_point(sw->outwin.display));
	edit_move_point(sw->outwin.display, error_point);
	edit_set_want_cursor(sw->outwin.display, TRUE);
	
	return FALSE;
}

/* RGA insert text string from UPS_F*_STR menus */
void
srcwin_handle_insert_text(sw, text)
Srcwin *sw;
char *text;
{
  int i;
  
  for (i = 0; i < (int)strlen(text); i++)
    edit_handle_key_event(sw->outwin.display, (int)text[i], 0);
}

void
handle_srcwin_char(key_char, meta)
int key_char;
bool meta;
{
  static event_t evbuf;
	    
  evbuf.ev_type = EV_KEY;
  evbuf.ev_char = key_char;
  if (meta == TRUE)
    evbuf.ev_buttons |= B_ALT_MASK;
  else
    evbuf.ev_buttons = 0;
  srcwin_handle_key_event
    (get_current_srcwin(), get_current_srcwin_region(), &evbuf);
}

void
handle_output_window_char(key_char, meta)
int key_char;
bool meta;
{
  static event_t evbuf;
  Outwin *ow;
	    
  ow = get_current_outwin();
  evbuf.ev_type = EV_KEY;
  evbuf.ev_char = key_char;
  if (meta == TRUE)
    evbuf.ev_buttons |= B_ALT_MASK;
  else
    evbuf.ev_buttons = 0;
  edit_handle_wn_key_event (ow->display, &evbuf);
}

bool
srcwin_have_editblock(sw, region, ev)
Srcwin *sw;
Region *region;
event_t *ev;
{
	Editblock *eb = NULL;
	Edit_propchange *pc;
	size_t point;

	if (!edit_pixel_to_point(sw->outwin.display, ev->ev_x, ev->ev_y,
				 &point))
		return FALSE;

	/*  Need to do this here so we don't get the wrong propchange
	 *  below.
	 */
	clear_highlighting(&sw->outwin.search_hl);
	
	pc = srcbuf_get_propchange_at_point(sw->fil->fi_srcbuf, point);
	if ( pc)
	    eb = (Editblock *)edit_get_propchange_data(pc);
	if (eb == NULL) 
	  return FALSE;
 	else 
	{
	  /* RCB: Use double click to select accelerator */
	  if (   ev->ev_type == EV_BUTTON_DOWN
	      && ev->ev_buttons & B_MULT_CLICKS)
	     srcbuf_accel_editblock_change(eb);
	  else
	     srcbuf_select_editblock(eb, TRUE);
	}
	return TRUE;
}

static void
init_display_area_overlay(dw, wn, tbar, buffer)
Outwin *dw;
int wn;
tbar_id_t tbar;
Edit_buffer *buffer;
{
	Edit_display *display;

	if (wn == -1) {
		display = NULL;
	}
	else {
		display = edit_create_wn_display(wn, WN_FG(wn), WN_BG(wn),
						 FALSE, FALSE);

		edit_use_folded_lines(display, Tabspaces, Want_display_checks,
				      FALSE); /* non-folded lines */
	
		if (buffer != NULL)
			edit_set_buffer(display, buffer);
	}
	
	dw->tbar = tbar;
	dw->display = display;
	dw->buffer = buffer;
	dw->wn = wn;
	dw->fg = WN_FG(wn);
	dw->bg = WN_BG(wn);
	dw->data = NULL;
	set_highlighting_style(&dw->search_hl, (font_t *)NULL, FLAG_HIGHLIGHT2, FLAG_HIGHLIGHT2);
}

Outwin *
display_area_overlay_create(wn, tbar)
int wn;
tbar_id_t tbar;
{
	Outwin *dw;
	Text_buffer *tb;
	Edit_buffer *buffer;

	tb = text_create_empty_lbuf_buffer(alloc_create_pool(), 0, 1024);
	buffer = edit_create_buffer(tb, Sysfont_info, 0);
	
	dw = (Outwin *)e_malloc(sizeof(Outwin));
	init_display_area_overlay(dw, wn, tbar, buffer);

	return dw;
}

void
display_area_overlay_insert_string(text, pos, len, prepend_newline,
				   fg_color, var_coloring_only)
const char *text;
int pos;
int len;
int prepend_newline;
int fg_color;
int var_coloring_only;
{
  Outwin *dw;
  size_t buff_len, sol = 0;
  int width, height, i, j = 0, max_chars, sol_int = 0, buff_len_int,
    flag = 0, blank_text_len;
  char blank_text[256];
  static long bg, val_fg, env_fg, bpt_fg, src_fg, func_fg, sig_fg, wpt_fg;
  static int got_colors = 0;
	
  dw = get_display_area_overlay();
  if (!got_colors)
  {
    got_colors = 1;
    get_value_colors(dw->wn, TRUE, &val_fg, &bg);
    bpt_getcolor(0, dw->wn, &bpt_fg, &bg);
    env_getcolor(0, dw->wn, &env_fg, &bg);
    srcfile_getcolor(0, dw->wn, &src_fg, &bg);
    func_getcolor(0, dw->wn, &func_fg, &bg);
    sig_getcolor(0, dw->wn, &sig_fg, &bg);
    wpt_getcolor(0, dw->wn, &wpt_fg, &bg);
  }
  if (!prepend_newline)
  {
    for (i = 0; i < len; i++)
      if (text[i] != ' ')
	break;
    if (i == len)
      return;			/* skip trailing blank strings */
  }
 
  wn_get_window_size(dw->wn, &width, &height);
  max_chars = (int)width/Sysfont_width;
  buff_len = edit_get_buffer_length(dw->buffer);
  if (prepend_newline)
  {
    blank_text[0] = '\n';
    j = 1;
    sol_int = buff_len - 1;
  }
  else
  {
    edit_find_char_backwards(dw->buffer, buff_len, 0, "\n", &sol);
    sol++;
    sol_int = (int)sol;
  }
  if (var_coloring_only)
  {
    set_coloring(&dw->search_hl, dw->buffer,
		 sol + pos, sol + pos + len, FLAG_VARIABLE, FLAG_VARIABLE);
  }
  else
  {
    buff_len_int = buff_len;
    for (i = 0; i < pos + sol_int - buff_len_int + j; i++)
      blank_text[i + j] = ' ';
    blank_text[i + j] = 0;
    blank_text_len = strlen(blank_text);
    if (blank_text_len + len >= 255)
    {
      edit_insert(dw->buffer, buff_len, (const char *)blank_text,
		  blank_text_len, FALSE);
      edit_insert(dw->buffer, buff_len + blank_text_len, text, len, FALSE);
    }
    else
    {
      strncat(blank_text, text, len);
      blank_text[i + j + len] = 0;
      if (sol_int < 0)
	sol_int = 0;
      for (i = 0; i > pos + sol_int - buff_len_int + j; i--, buff_len--)
	edit_delete(dw->buffer, buff_len - 1, 1);

      blank_text_len = strlen(blank_text);
      edit_insert(dw->buffer, buff_len, (const char *)blank_text,
		  blank_text_len, FALSE);
    }
    if (fg_color == val_fg)
      flag = FLAG_VALUE;
    else
      if (fg_color == bpt_fg)
	flag = FLAG_BPT;
      else
	if (fg_color == env_fg)
	  flag = FLAG_ENV;
	else
	  if (fg_color == src_fg)
	    flag = FLAG_SRC;
	  else
	    if (fg_color == func_fg)
	      flag = FLAG_FUNC;
	    else
	      if (fg_color == sig_fg)
		flag = FLAG_SIG;
	      else
		if (fg_color == wpt_fg)
		  flag = FLAG_WPT;
    /* RGA Optimize the coloring by turning it on and off only when
       necessary. Long source file lists will all be one color
       for instance. This techinque can reduce the setup time for a 
       search from minutes, for targets with thousands of files,
       to millisecs */

    /*    if (flag)
      set_coloring(&dw->search_hl, dw->buffer,
		   buff_len, buff_len + strlen(blank_text),
		   flag, flag);*/

    if (overlay_last_flag != flag)
    {
      if (overlay_last_flag > 0)
	set_coloring_off(&dw->search_hl, dw->buffer,
			 overlay_last_buff_len, overlay_last_len,
			 overlay_last_flag);
      if (flag)
	set_coloring_on(&dw->search_hl, dw->buffer,
			buff_len, buff_len + blank_text_len,
			flag, flag);
    }
    overlay_last_flag = flag;
    overlay_last_buff_len = buff_len;
    overlay_last_len = buff_len + blank_text_len;
  }
}

void
display_area_overlay_delete_all_text(dw)
Outwin *dw;
{
	edit_update_wn_window_size(dw->display, dw->wn);
	edit_delete(dw->buffer, 0, edit_get_buffer_length(dw->buffer));
}

void
display_area_overlay_scroll(dw, npixels)
Outwin *dw;
int npixels;
{
	edit_scroll_display(dw->display, npixels);
}

void
display_area_overlay_search(dw, pattern, forwards)
Outwin *dw;
const char *pattern;
bool forwards;
{
	size_t buff_len, dstart, dlim, fstart, flim, point, sol;
	int pixel_offset, y = -1, ft_ht, y_offset, disp_wn,
	lx, ly, width, height, baseline, cur_posn, offset_lines = 0;
	bool clear, oldval;
	
	dw = (Outwin *)re_get_data(get_display_area_region());
	clear = TRUE;
	if (display_area_overlay_control(0, 0) == 0)
	{
	  disp_wn = re_get_wn(get_display_area_region());
	  clear_selection();
	  display_area_overlay_control(1, 0); /* set */
	  clear = FALSE;
	  ft_ht = wn_get_sysfont()->ft_height;
	  cur_posn = get_cur_posn();
	  set_cur_posn(0);
	  y_offset = cur_posn%ft_ht;
	  dw->search_hl.old_visible_units = tb_get_visible_units(dw->tbar);
	  dw->search_hl.old_total_units = tb_get_total_units(dw->tbar);
	  dw->search_hl.found = 0;	  
	  wn_text_capture(1, 0, &y); /* set */
	  display_area_overlay_delete_all_text(dw);
	  oldval = edit_set_propchange_updates(dw->buffer, FALSE);
	  overlay_last_flag = -1; /* initialize */
	  display_all_lines(0, 0, 1);
	  if (overlay_last_flag > 0)
	    set_coloring_off(&dw->search_hl, dw->buffer,
			     overlay_last_buff_len, overlay_last_len,
			     overlay_last_flag);
	  edit_set_propchange_updates(dw->buffer, oldval);
	  outwin_redraw(dw);
	  dstart = 0;
	  buff_len = edit_get_buffer_length(dw->buffer);
	  while (edit_find_char_forwards(dw->buffer, dstart, buff_len,
					 "\n", &sol) &&
		 offset_lines < (int)cur_posn/ft_ht)
	  {
	    offset_lines++;
	    dstart = sol;
	  }
	  display_area_overlay_scroll(dw, y_offset + ft_ht + offset_lines*ft_ht);
	  wn_text_capture(0, 1, &y); /* reset */
	  display_area_overlay_tb_clear(dw);
	  set_cur_posn(cur_posn);
	}

	edit_get_display_info(dw->display, &pixel_offset, &dstart, &dlim);
	
	if (dw->search_hl.start != NULL) {
	  size_t start, lim;
	  bool old_visible;
		
	  start = edit_get_propchange_point(dw->search_hl.start);
	  lim = edit_get_propchange_point(dw->search_hl.lim);

	  old_visible = start >= dstart && start < dlim;

	  if (forwards)
	    point = old_visible ? lim : dstart;
	  else
	    point = old_visible ? start : dstart;
	}
	else {
	  point = dstart;
	}
	
	if (search_buffer(dw->buffer, pattern, point, forwards,
			  &fstart, &flim, FALSE)) {
	  ensure_ow_point_visible(dw, fstart);
	  set_highlighting(&dw->search_hl, dw->buffer, fstart, flim, clear);
	  edit_point_to_pixel(dw->display, fstart, &lx, &ly, &width, &height,
			      &baseline);
	  dw->search_hl.puck_x = lx + width/2;
	  dw->search_hl.puck_y = ly + height/2;
	  dw->search_hl.found = 1;
	}
      }

int
display_area_overlay_control(set, reset)
     int set;
     int reset;
{
  static int create = 0;
  int y;

  if (set)
    create = set;
  else
    if (reset && create)
    {
      int pixel_offset, offset_lines = -1, ft_ht, offset;
      size_t dstart, dlim, sol;
      Outwin *dw;

      dw = (Outwin *)re_get_data(get_display_area_region());

      create = 0;
      wn_text_capture(1, 0, &y); /* set */
      edit_get_display_info(dw->display, &pixel_offset, &dstart, &dlim);

      while (edit_find_char_backwards(dw->buffer, dstart, 0, "\n", &sol))
      {
	offset_lines++;
	dstart = sol;
      }

      display_area_overlay_delete_all_text(dw);
      wn_text_capture(0, 1, &y); /* reset */
      tb_set_visible_units(dw->tbar, dw->search_hl.old_visible_units);
      tb_set_total_units(dw->tbar, dw->search_hl.old_total_units);
      ft_ht = wn_get_sysfont()->ft_height;
      offset = offset_lines*ft_ht-pixel_offset;
/*      if (offset < 0)*/
/*	offset = 0;*/
      if (reset == 1)
	tb_goto(dw->tbar, offset, TRUE);
      else
	set_cur_posn(offset);
      if (dw->search_hl.found)
	select_object_at(dw->search_hl.puck_x, dw->search_hl.puck_y);
}
  return(create);
}

void
display_area_overlay_tb_clear(dw)
Outwin *dw;
{
  tb_expose(dw->tbar);
}

bool
display_area_overlay_set_propchange_updates(dw, val)
Outwin *dw;
bool val;
{
  return edit_set_propchange_updates(dw->buffer, val);
}

void display_area_overlay_unselect_global_selection()
{
    Outwin *dw = (Outwin *)re_get_data(get_display_area_region());
    edit_unselect_global_selection(dw->display);
}

static 
void
srcwin_clear_stopline()
{
  Highlighting *hl = &Stopline;

  hl->buffer = NULL;
  hl->start = NULL;
  hl->lim = NULL;
}

void 
srcwin_reset()
{
  Srcwin *sw = get_current_srcwin();
  Outwin *ow;
  Text_buffer *tb;
  Edit_buffer *buffer;
  font_t *font;
  int wn, width, height, nchars;
  char nbuf[60];

  srcwin_clear_stopline();
  sw->fil = NULL;
  font = wn_get_sysfont();
  wn = (int)srcwin_get_data(sw);
  wn_get_window_size(wn, &width, &height);
  nchars = (width - 8) / font->ft_width;
  if (nchars > sizeof(nbuf) - 1)
    nchars = sizeof(nbuf) - 1;
  memset(nbuf, ' ', 60);
  nbuf[nchars] = 0;
  wn_tputs(wn, nbuf, 4, (height - font->ft_height) / 2);

  ow = &sw->outwin;
  tb = text_create_empty_lbuf_buffer(alloc_create_pool(), 0, 1024);
  buffer = edit_create_buffer(tb, Srcfont_info, 0);
  if (buffer != NULL)
    edit_set_buffer(ow->display, buffer);
  ow->buffer = buffer;
  srcbuf_reset_file_handle();
  srcwin_redraw(sw);
}
