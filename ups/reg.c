/* reg.c - general purpose window region package */

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


/* @(#)reg.c	1.18 16 Sep 1994 (UKC) */
char ups_reg_c_rcsid[] = "$Id$";

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <stdlib.h>

#include <local/ukcprog.h>
#include <mtrprog/alloc.h>
#include <local/wn.h>
#include <local/wn.h>
#include <local/menu3.h>
#include <local/obj/mhole.h>

#include "reg.h"
#include "cursors.h"
#include "debug.h"
#include "tdr.h"
#include "ui.h"
#include "srcwin.h"
#include "state.h"
#include "ups.h"
#include "symtab.h"
#include "target.h"

#define XPOS_OR_WIDTH	0
#define YPOS_OR_HEIGHT	1

#define XPOS	XPOS_OR_WIDTH
#define YPOS	YPOS_OR_HEIGHT

#define WIDTH	XPOS_OR_WIDTH
#define HEIGHT	YPOS_OR_HEIGHT

struct Region {
	int wn;

	float prop;
	short minsize[2];
	short size[2];

	unsigned char flags;
	unsigned char cursor;

	unsigned char left_margin;
	unsigned char right_margin;
	unsigned char top_margin;
	unsigned char bottom_margin;

	const char **mhcaps;

	re_draw_proc_t draw_proc;
	re_input_proc_t input_proc;
	re_destroy_proc_t destroy_proc;
	re_accept_focus_proc_t accept_focus_proc;
	re_release_focus_proc_t release_focus_proc;
	re_get_display_proc_t get_display_proc;

	char *focus_data;
	bool force_quit;

	char *data;

	Region *parent;
	Region *subregs;
	Region *next;
};

#define RF_VSPLIT	0x01	/* region is split vertically (cf horizontally) */
#define RF_SIZE_CHANGED	0x02	/* size of region has changed since last redraw */
#define RF_REDO_PROPS	0x04	/* proportions of subregs need re-normalising */
#define RF_FIXED_SIZE	0x08	/* prop was specified as 0.0 */

#define HH			/* Define this if you like Douglas Adams */

static Region *add_region PROTO((Region *par, int wn, double prop, int size));
static void resize PROTO((Region *par));
static void expose PROTO((Region *par, re_draw_reason_t expose_reason,
			  int x, int y, int width, int height, bool minimise));
static void check_margin_range PROTO((int n));
static void set_win_size PROTO((Region *re, int *pos));
static void await_big_enough_window PROTO((Region *re));
static void renormalise_props PROTO((Region *par, bool skip_minsize_adjustment));
static void check_minsizes PROTO((Region *par));
static void set_minsize PROTO((Region *re, int *minsize));
static void update_window_positions PROTO((Region *par, int dim));
static int avail_pixels PROTO((Region *start, Region *lim, int dim));
static Region *prev_re PROTO((Region *re));
static void swap_regions PROTO((Region *sel_re, event_t *event));
static void swap_children PROTO((Region *re1, Region *re2));
static void rswap_regions PROTO((Region *sel_re, event_t *ev, bool going_up));
#ifdef HH
static void grey_out PROTO((Region *region, int wn,
			    int width, int height,
			    re_draw_reason_t draw_reason));
#endif /* HH */
static void set_current_typing_region PROTO((int wn));
static void icon_notify PROTO(( int wn, int iconify));
static void close_window PROTO((int wn));

/*  BUG: these should be packaged up with the root of a region tree in
 *       a regtree_t structure
 */
static bool Quit_event_loop;
static Region  *Root_regions[MAXROOTS];
static bool    IsLeadRegion[MAXROOTS];
static int numRegions = 0;
static Region *Keyboard_re = NULL;
static Region *Default_keyboard_re = NULL;
static int iconic_state = FALSE;
static re_button_proc_t Button_proc[10];
static char *Button_proc_data[10];


/* structure used to arbitrate between multible typing line regions:
*/
static struct { int root_win; Region* typing_line, *extended; }
      typing_lines[MAXROOTS] = { { -1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}};


void
re_set_data(re, data)
Region *re;
char *data;
{
	re->data = data;
}

void
re_set_cursor(re, cursor)
Region *re;
int cursor;
{
	if (cursor < 0 || cursor > 255)
		panic("bad cursor in sc");

	re->cursor = cursor;
}

void
re_set_mhcaps(re, left_caption, middle_caption, right_caption)
Region *re;
const char *left_caption, *middle_caption, *right_caption;
{
	re->mhcaps = (const char **)e_malloc(4 * sizeof(char *));
	re->mhcaps[0] = left_caption;
	re->mhcaps[1] = middle_caption;
	re->mhcaps[2] = right_caption;
	re->mhcaps[3] = "";	/* anachronism from Perq days */
}

char *
re_get_data(re)
Region *re;
{
	return re->data;
}

int
re_get_wn(re)
Region *re;
{
	return re->wn;
}
int
re_get_root_wn(re)
Region *re;
{
    return wn_get_root_window(re->wn);
}

bool
re_same_root_wn(re1, re2)
Region* re1;
Region* re2;
{
    return wn_same_root_window(re1->wn, re2->wn);
}

void
re_set_minsize(re, width, height)
Region *re;
int width, height;
{
	int minsize[2];

	/*  Composite regions minimum sizes must be derived from their
	 *  sub-regions.
	 */
	if (re->subregs != NULL)
		panic("setminsize botch");

	minsize[WIDTH] = width + re->left_margin + re->right_margin;
	minsize[HEIGHT] = height + re->top_margin + re->bottom_margin;

	set_minsize(re, minsize);
}

static void
set_minsize(re, minsize)
Region *re;
int *minsize;
{
	Region *par;

	/*  Propagate the new minumum size requirements up the tree.
	 */
	for (par = re->parent; par != NULL; par = par->parent) {
		int dim, otherdim, par_minsize[2];

		if (re->minsize[WIDTH] == minsize[WIDTH] &&
		    re->minsize[HEIGHT] == minsize[HEIGHT])
			return;

		dim = (par->flags & RF_VSPLIT) ? WIDTH : HEIGHT;
		otherdim = 1 - dim;

		par_minsize[WIDTH] = par->minsize[WIDTH];
		par_minsize[HEIGHT] = par->minsize[HEIGHT];

		par_minsize[dim] += minsize[dim] - re->minsize[dim];
		if (minsize[otherdim] > par_minsize[otherdim])
			par_minsize[otherdim] = minsize[otherdim];
		
		re->minsize[WIDTH] = minsize[WIDTH];
		re->minsize[HEIGHT] = minsize[HEIGHT];
		check_minsizes(re);

		re = par;
		minsize[WIDTH] = par_minsize[WIDTH];
		minsize[HEIGHT] = par_minsize[HEIGHT];
	}

	re->minsize[WIDTH] = minsize[WIDTH];
	re->minsize[HEIGHT] = minsize[HEIGHT];
	check_minsizes(re);

}

static void
check_minsizes(par)
Region *par;
{
	Region *re;
	int dim, otherdim, minsize[2];

	if (par->subregs == NULL)
		return;

	dim = (par->flags & RF_VSPLIT) ? WIDTH : HEIGHT;
	otherdim = 1 - dim;

	minsize[WIDTH] = minsize[HEIGHT] = 0;
	for (re = par->subregs; re != NULL; re = re->next) {
		minsize[dim] += re->minsize[dim];
		if (re->minsize[otherdim] > minsize[otherdim])
			minsize[otherdim] = re->minsize[otherdim];
	}

	if (minsize[dim] != par->minsize[dim])
		panic("dim botch in cms");
	if (minsize[otherdim] > par->minsize[otherdim])
		panic("odim botch in cms");
}

void
re_set_callbacks(re, draw_proc, input_proc, destroy_proc)
Region *re;
re_draw_proc_t draw_proc;
re_input_proc_t input_proc;
re_destroy_proc_t destroy_proc;
{
	re->draw_proc = draw_proc;
	re->input_proc = input_proc;
	re->destroy_proc = destroy_proc;
}

void
re_set_keyboard_focus_callbacks(re, accept_proc, release_proc)
Region *re;
re_accept_focus_proc_t accept_proc;
re_release_focus_proc_t release_proc;
{
	re->accept_focus_proc = accept_proc;
	re->release_focus_proc = release_proc;
}

void
re_set_display_callback(re, proc)
Region* re;
re_get_display_proc_t proc;
{
    re->get_display_proc = proc;
}

struct Edit_display*
re_get_display(re)
Region* re;
{
    if ( re &&  re->get_display_proc)
	return (*re->get_display_proc)(re);
    return NULL;
}

Region *
re_make_region(wn)
int wn;
{
	return add_region((Region *)NULL, wn, 1.0, 0);
}

static void
check_margin_range(n)
int n;
{
	if (n < 0 || n > 255)
		panic("bad margin value in cmr");
}

void
re_set_margins(re, left, right, top, bottom)
Region *re;
int left, right, top, bottom;
{
	int ldelta, rdelta, tdelta, bdelta;

	check_margin_range(left);
	check_margin_range(right);
	check_margin_range(top);
	check_margin_range(bottom);


	ldelta = left - re->left_margin;
	rdelta = right - re->right_margin;
	tdelta = top - re->top_margin;
	bdelta = bottom - re->bottom_margin;

	re->flags |= RF_SIZE_CHANGED;
	wn_adjust_win_size(re->wn, ldelta, tdelta,
			   ldelta + rdelta, tdelta + bdelta);

	re->left_margin = left;
	re->right_margin = right;
	re->top_margin = top;
	re->bottom_margin = bottom;
}

static Region *
add_region(par, wn, prop, fixed_size)
Region *par;
int wn;
double prop;
int fixed_size;
{
	static const char *nullcaps[] = { "", "", "", "" };
	Region *new_re;

	new_re = (Region *)e_malloc(sizeof(Region));

	if (par == NULL)
		new_re->wn = wn;
	else {
		Region *re;

		for (re = par; re != NULL; re = re->parent)
			check_minsizes(re);
		new_re->wn = wn_create_subwin(wn, 0, 0, 0, 0,
						 WN_INPUT_OUTPUT);
	}


	/*  We set the minimum size to zero intially, so we can use
	 *  re_set_minsize to set the actual values as a delta.
	 */
	new_re->minsize[WIDTH] = new_re->minsize[HEIGHT] = 0;

	new_re->size[WIDTH] = new_re->size[HEIGHT] = 0;

	new_re->left_margin = new_re->right_margin = 0;
	new_re->top_margin = new_re->bottom_margin = 0;

	new_re->flags = RF_SIZE_CHANGED;
	new_re->cursor = CU_DEAD;
	new_re->mhcaps = nullcaps;

	new_re->subregs = NULL;
	new_re->prop = prop;
	
	new_re->draw_proc = NULL;
	new_re->input_proc = NULL;
	new_re->destroy_proc = NULL;
	new_re->accept_focus_proc = NULL;
	new_re->release_focus_proc = NULL;
	new_re->get_display_proc = NULL;

	new_re->data = NULL;
	new_re->force_quit = FALSE;
	new_re->focus_data = NULL;
	new_re->subregs = NULL;
	new_re->next = NULL;
	new_re->parent = par;

	if (prop == 0.0)
		new_re->flags |= RF_FIXED_SIZE;

	wn_set_win_data(new_re->wn, (long)new_re);

	if (par != NULL) {
		Region *re, *last;
		int dim;

		dim = (par->flags & RF_VSPLIT) ? WIDTH : HEIGHT;

		last = NULL;
		for (re = par->subregs; re != NULL; re = re->next)
			last = re;
		if (last == NULL) {
			if (par->minsize[dim] != 0)
				panic("par ms botch");
			par->subregs = new_re;
		}
		else
			last->next = new_re;
		
		if (prop != 0.0)
			par->flags |= RF_REDO_PROPS;

		if (fixed_size != 0) {
			int minsize[2];

			minsize[dim] = fixed_size;
			minsize[1 - dim] = 0;
			set_minsize(new_re, minsize);
		}
	}

	{
		Region *re;

		for (re = new_re; re != NULL; re = re->parent)
			check_minsizes(re);
	}

	return new_re;
}

/*  Scale the re_prop fields of all the variable size children
 *  of par so that the total is reasonably near to 1.0
 */
static void
renormalise_props(par, skip_minsize_adjustment)
Region *par;
bool skip_minsize_adjustment;
{
	Region *re;
	double total_props, scale, average_minsize;
	int total_vpixels, total_minsize, standard_minsize, nvregs, dim;
	bool all_minsizes_equal;

	dim = (par->flags & RF_VSPLIT) ? WIDTH : HEIGHT;

	nvregs = total_minsize = 0;
	total_props = 0.0;
	all_minsizes_equal = TRUE;
	standard_minsize = 0;		/* to satisfy gcc */

	for (re = par->subregs; re != NULL; re = re->next) {
		total_props += re->prop;
		if ((re->flags & RF_FIXED_SIZE) == 0) {
			++nvregs;
			total_minsize += re->minsize[dim];
			if (nvregs == 0)
				standard_minsize = total_minsize;
			else if (re->minsize[dim] != standard_minsize)
				all_minsizes_equal = FALSE;
		}
	}

	if (nvregs == 0 || total_props < 0.0001)
		panic("props botch in rp");

	scale = 1.0 / total_props;
	for (re = par->subregs; re != NULL; re = re->next)
		re->prop *= scale;
	
	if (all_minsizes_equal || skip_minsize_adjustment)
		return;

	/*  Compensate for the minumum size allocations by adding
	 *  or subtracting to re_prop an amount proportional to the
	 *  deviation of each min_size from the average min_size.
	 */

	average_minsize = total_minsize / nvregs;
	total_vpixels = par->size[dim] - par->minsize[dim];

	for (re = par->subregs; re != NULL; re = re->next) {
		double minsize_dev;

		if (re->flags & RF_FIXED_SIZE)
			continue;
		minsize_dev = re->minsize[dim] - average_minsize;
		re->prop -= minsize_dev / total_vpixels;
		if (re->prop < 0.0)
			re->prop = 0.0;
	}

	renormalise_props(par, TRUE);
}

/*  Resize the children of par such that the total width and height
 *  of the children matches par->size.  Recursively update descendents.
 */
static void
resize(par)
Region *par;
{
	Region *re, *maxsize_re;
	int dim, otherdim;
	int total_vpixels, vpixels_left, maxsize_vpixels;

	if (par->size[WIDTH] < par->minsize[WIDTH])
		panic("width botch in rs");
	if (par->size[HEIGHT] < par->minsize[HEIGHT])
		panic("height botch in rs");

	if (par->subregs == NULL)
		return;

	if (par->flags & RF_REDO_PROPS) {
		renormalise_props(par, FALSE);
		par->flags &= ~RF_REDO_PROPS;
	}

	dim = (par->flags & RF_VSPLIT) ? XPOS_OR_WIDTH : YPOS_OR_HEIGHT;
	otherdim = 1 - dim;

	vpixels_left = total_vpixels = par->size[dim] - par->minsize[dim];
	
	/*  And each region shall recieve pixels according to its need ...
	 *  Remember the largest variable size region so we can feed it
	 *  any roundoff pixels.
	 */
	maxsize_re = NULL;
	maxsize_vpixels = 0;
	for (re = par->subregs; re != NULL; re = re->next) {
		int npixels, vpixels;

		npixels = re->minsize[dim];

		vpixels = total_vpixels * re->prop;
		npixels += vpixels;
		vpixels_left -= vpixels;

		if (vpixels > maxsize_vpixels) {
			maxsize_re = re;
			maxsize_vpixels = vpixels;
		}

		if (re->size[dim] != npixels ||
		    re->size[otherdim] != par->size[otherdim])
			re->flags |= RF_SIZE_CHANGED;

		re->size[dim] = npixels;
		re->size[otherdim] = par->size[otherdim];
	}

	/*  BUG: what if vpixels_left is negative and maxsize_re is already
	 *       at its minimum size?
	 */
	if (vpixels_left != 0) {
		if (maxsize_re == NULL)
			panic("vpix botch");
		maxsize_re->size[dim] += vpixels_left;
	}
	
	update_window_positions(par, dim);
}

/*  Recalculate positions based on the new sizes and recursively
 *  resize children.
 */
static void
update_window_positions(par, dim)
Region *par;
int dim;
{
	int pos[2];
	Region *re;

	pos[XPOS] = pos[YPOS] = 0;
	for (re = par->subregs; re != NULL; re = re->next) {
		set_win_size(re, pos);
		if (re->flags & RF_SIZE_CHANGED)
			resize(re);
		pos[dim] += re->size[dim];
	}

	/*  Sanity check
	 */
	if (pos[dim] != par->size[dim])
		panic("resize botch");
}

static void
set_win_size(re, pos)
Region *re;
int *pos;
{
	int x, y, width, height;

	x = pos[XPOS] + re->left_margin;
	y = pos[YPOS] + re->top_margin;
	width = re->size[WIDTH] - (re->left_margin + re->right_margin);
	height = re->size[HEIGHT] - (re->top_margin+re->bottom_margin);

	wn_change_win(re->wn, re->parent->wn, x, y, width, height);
}

static void
expose(par, expose_reason, x, y, width, height, minimise)
Region *par;
re_draw_reason_t expose_reason;
int x, y, width, height;
bool minimise;
{
	Region *re;
	int l_width, l_height;
	re_draw_reason_t draw_reason;

	for (re = par->subregs; re != NULL; re = re->next)
		expose(re, expose_reason, x, y, width, height, minimise);

	if (par->draw_proc == NULL)
		return;

	if ((par->flags & RF_SIZE_CHANGED) && expose_reason == RE_EXPOSED) {
		par->flags &= ~RF_SIZE_CHANGED;
		draw_reason = RE_RESIZED;
	}
	else
		draw_reason = expose_reason;

	wn_get_window_size(par->wn, &l_width, &l_height);
	if (!minimise || wn_intersects_rect(par->wn, x, y, width, height)) {
		(*par->draw_proc)(par, par->wn, l_width, l_height, draw_reason);
	}
}

void
re_set_exit_event_loop_flag()
{
	Quit_event_loop = TRUE;
}

bool
re_get_exit_event_loop_flag()
{
	return Quit_event_loop;
}

Region *
re_get_keyboard_focus_region()
{
	return Keyboard_re;
}

char *
re_get_keyboard_focus_data(re)
Region *re;
{
	return (re != NULL) ? re->focus_data : NULL;
}

void
re_clear_force_quit_flag(re)
Region *re;
{
	re->force_quit = FALSE;
}

bool
re_set_keyboard_focus(re, focus_data)
Region *re;
char *focus_data;
{
	Region *oldre;

	oldre = Keyboard_re;

	if (oldre == re && oldre->focus_data == focus_data)
		return TRUE;
	
	if (oldre != NULL) {
		if (oldre->release_focus_proc != NULL &&
		    !(*oldre->release_focus_proc)(oldre, oldre->focus_data,
						  oldre->force_quit)) {
			if (oldre->force_quit)
				panic("re did not quit when forced");

			oldre->force_quit = TRUE;
			return FALSE;
		}
		
		oldre->force_quit = FALSE;
		oldre->focus_data = NULL;
	}

	if (re == NULL)
		re = Default_keyboard_re;	/* (might also be NULL) */

	if (re != NULL) {
		re->focus_data = focus_data;
		
		if (re->accept_focus_proc != NULL)
			(*re->accept_focus_proc)(re, focus_data);
	}
	
	Keyboard_re = re;
	
	return TRUE;
}

void
re_set_default_keyboard_focus(re, focus_data)
Region *re;
char *focus_data;
{
	re->focus_data = focus_data;
	Default_keyboard_re = re;
}


void
re_add_rootwin(region, lead_region)
Region *region;
bool lead_region;  /* True if iconifying region iconifys all */
{
   
        Root_regions[numRegions] = region;
        IsLeadRegion[numRegions]  = lead_region;
        numRegions++;
}

void
re_redraw_root(event_type, clear_first)
unsigned long event_type;
bool clear_first;
{
	int regNum;
	Region* root;
	if (event_type == EV_WINDOW_RESIZED) {
	    for ( regNum = 0; regNum < numRegions; regNum++)
		{
		root = Root_regions[regNum];
		await_big_enough_window(root);
		resize(root);
		}
	}
	for ( regNum = 0; regNum < numRegions; regNum++)
	{
		root = Root_regions[regNum];
		re_expose(root, clear_first);
	}

}


void
re_redraw_root_clip(event_type, x, y, width, height, clear_first)
unsigned long event_type;
int x, y, width, height;
bool clear_first;
{
	int regNum;
	Region* root;
	if (event_type == EV_WINDOW_RESIZED) {
	    for ( regNum = 0; regNum < numRegions; regNum++)
		{
		root = Root_regions[regNum];
		await_big_enough_window(root);
		resize(root);
		}
	}
	for ( regNum = 0; regNum < numRegions; regNum++)
	{
		root = Root_regions[regNum];
		re_expose_clip(root, clear_first, x, y, width, height);
	}

}


static void
await_big_enough_window(re)
Region *re;
{
	font_t *font;

	font = wn_get_sysfont();

	for (;;) {
		bool wide_enough, deep_enough;
		int win_width, win_height, mesg_width, mesg_height;
		const char *mesg;
		event_t event;

		wn_get_window_size(re->wn, &win_width, &win_height);

		wide_enough = win_width >= re->minsize[WIDTH];
		deep_enough = win_height >= re->minsize[HEIGHT];

		if (wide_enough && deep_enough) {
			re->size[WIDTH] = win_width;
			re->size[HEIGHT] = win_height;
			break;
		}

		if (wide_enough)
			mesg = "Window too short";
		else if (deep_enough)
			mesg = "Window too narrow";
		else
			mesg = "Window too small";
		
		mesg_width = wn_strwidth(mesg, font);
		mesg_height = font->ft_height;

		wn_set_area(re->wn, 0, 0, win_width, win_height, WN_BG(re->wn));
		wn_tputs(re->wn, mesg, (win_width - mesg_width) / 2,
					(win_height - mesg_height) / 2);
		wn_next_event(re->wn, EV_WINDOW_RESIZED | EV_WINDOW_EXPOSED,
								    &event);
	}
}

void
re_set_button_callback(button, button_proc, data)
int button;
re_button_proc_t button_proc;
char *data;
{
	if (button > 0 && button < 11)
	{
		Button_proc[button-1] = button_proc;
		Button_proc_data[button-1] = data;
	}
}

void
re_event_loop()
{
	const int evmask = EV_BUTTON_UP | EV_BUTTON_DOWN | EV_MOUSE_MOVED | EV_KEY |
			   EV_INTERRUPT | EV_WINDOW_EXPOSED | EV_WINDOW_RESIZED;
	Region *root;
	int last_cursor;
	int last_wn = -1;
	static const char **last_mhcaps;
	wn_set_icon_notify_func(icon_notify);
	wn_set_close_func(close_window);

	last_cursor = -1;
	
	for (Quit_event_loop = FALSE; !Quit_event_loop; ) {
		bool resized = FALSE;
		int cursor;
		Region *re;
		event_t event;
		int regNum;

		for ( regNum = 0; regNum < numRegions; regNum++)
		{
		    unsigned long resize_event;
		    root = Root_regions[regNum];
		resize_event = wn_get_resize_event(root->wn);
		if (resize_event != 0) {
			if (display_area_overlay_control(0, 0))
			  display_area_overlay_control(0, 2);
				if (event.ev_type & (EV_WINDOW_RESIZED|EV_WINDOW_EXPOSED)) {
					re_redraw_root_clip(resize_event, event.ev_xpos, event.ev_ypos,
							    event.ev_width, event.ev_height, FALSE);
			td_record_refresh();
				}	
			resized = TRUE;
		    }
		}
		if ( resized)
		{
		    /* reset, no paint */
		    continue;
		  }

		wn_next_event(WN_ANY, evmask, &event);

		/*  Call any button callback that applies to this event.
		 */
		if (event.ev_type & (EV_BUTTON_DOWN|EV_BUTTON_UP))
		{
			int button = 0;
		   
			switch (event.ev_flags)
			{
			case B_LEFT:
				button = 1;
				break;
			case B_MIDDLE:
				button = 2;
				break;
			case B_RIGHT:
				button = 3;
				break;
			case B_BUTTON4:
				button = 4;
				break;
			case B_BUTTON5:
				button = 5;
				break;
			case B_BUTTON6:
				button = 6;
				break;
			case B_BUTTON7:
				button = 7;
				break;
			case B_BUTTON8:
				button = 8;
				break;
			case B_BUTTON9:
				button = 9;
				break;
			case B_BUTTON10:
				button = 10;
				break;
			}

			if (button > 0 && Button_proc[button-1])
				Button_proc[button-1](Button_proc_data[button-1], &event);
		}

		/*  Cancel any displayed error message on a mouse ior key press
		 */
		if (event.ev_type & (EV_BUTTON_DOWN | EV_KEY))
			clear_message();

		/*  We want key presses anywhere in the window to go to the
		 *  typing line.
		 */
		set_current_typing_region(event.ev_wn);
		if (Keyboard_re != NULL && event.ev_type == EV_KEY) {
			re = Keyboard_re;
		}
		else
		{
			re = (Region *)wn_get_win_data(event.ev_wn);
			if ( event.ev_wn != last_wn)
			{

			    if (re)
			    {
			        cursor = re->cursor;
				set_bm_cursor(re->wn, cursor);
			if (re->mhcaps != last_mhcaps) {
				mhdraw(re->mhcaps);
				last_mhcaps = re->mhcaps;
			}
		}
			    last_wn= event.ev_wn;
			}
		}

		if (re != NULL) {
			const int swmask = B_SHIFT_MASK | B_CONTROL_MASK;

			if (event.ev_type == EV_BUTTON_DOWN &&
					(event.ev_buttons & swmask) == swmask)
				swap_regions(re, &event);
			else if (re->input_proc != NULL)
			{
			  if (event.ev_type & (EV_BUTTON_DOWN | EV_KEY))
			  {
			    if (re->input_proc != menu_input &&
				display_area_overlay_control(0, 0))
			      display_area_overlay_control(0, 1); /* reset */
			  }
			  (*re->input_proc)(re, &event);
			}
		}
	}
}

void re_handle_event(re, ev)
Region* re;
event_t* ev;
{
    (*re->input_proc)(re, ev);
}

void
re_expose(re, clear_first)
Region *re;
bool clear_first;
{
	int xpos, ypos, width, height;

	if (clear_first)
		wn_set_area(re->wn, 0, 0,
			    re->size[WIDTH], re->size[HEIGHT], WN_BG(re->wn));
	wn_get_window_rect(re->wn, &xpos, &ypos, &width, &height);
	expose(re, RE_EXPOSED, xpos, ypos, width, height, TRUE);
}

void
re_expose_clip(re, clear_first, x, y, width, height)
Region *re;
bool clear_first;
int x, y, width, height;
{
	wn_set_clip(re->wn, x, y, width, height);
	if (clear_first)
		wn_set_area(re->wn, x, y, width, height, WN_BG(re->wn));
	expose(re, RE_EXPOSED, x, y, width, height, TRUE);
	wn_clear_clip(re->wn);
}

#ifdef __STDC__
void
re_divide(Region *re, re_orientation_t orientation, ...)
{
	va_list ap;

	va_start(ap, orientation);
#else
void
re_divide(va_alist)
va_dcl
{
	va_list ap;
	re_orientation_t orientation;
	Region *re;

	va_start(ap);
	re = va_arg(ap, Region *);
	orientation = va_arg(ap, re_orientation_t);
#endif

	switch (orientation) {
	case RE_VERTICAL:
		if (!(re->flags & RF_VSPLIT)) {
			if (re->subregs != NULL)
				panic("tried to change region split type");
			re->flags |= RF_VSPLIT;
		}
		break;
	case RE_HORIZONTAL:
		if (re->flags & RF_VSPLIT) {
			if (re->subregs != NULL)
				panic("tried to change region split type");
			re->flags &= ~RF_VSPLIT;
		}
		break;
	default:
		panic("bad orientation in re_divide");
	}

	for(;;) {
		Region **p_region;
		Region *new_re;
		int size;
		double prop;

		p_region = va_arg(ap, Region **);
		if (p_region == NULL)
			break;

		size = va_arg(ap, int);
		prop = va_arg(ap, double);

		new_re = add_region(re, re->wn, prop, size);
		if (*p_region != NULL) {
			Region *old_re;

			old_re = *p_region;
			new_re->draw_proc = old_re->draw_proc;
			new_re->input_proc = old_re->input_proc;
			new_re->destroy_proc = old_re->destroy_proc;
			new_re->cursor = old_re->cursor;
			new_re->mhcaps = old_re->mhcaps;
		}
		*p_region = new_re;
	}

	va_end(ap);
}

static Region *
prev_re(lim)
Region *lim;
{
	Region *re, *prev;

	prev = NULL;
	for (re = lim->parent->subregs; re != lim; re = re->next)
		prev = re;
	return prev;
}

static int
avail_pixels(start, lim, dim)
Region *start, *lim;
int dim;
{
	Region *re;
	int avail;

	avail = 0;
	for (re = start; re != lim; re = re->next)
		avail += re->size[dim] - re->minsize[dim];

	return avail;
}

#ifdef HH
static void
grey_out(re, wn, width, height, draw_reason)
Region *re;
int wn;
int width, height;
re_draw_reason_t draw_reason;
{
	switch (draw_reason) {
	case RE_EXPOSED:
	case RE_RESIZED:
		wn_shade_area(wn, 0, 0, width, height, WN_GREY2, R_RPL);
		break;
	case RE_RESIZING:
	case RE_UNDRAW:
		break;
	default:
		panic("bad dr in ld");
	}
}
#endif

static void
swap_regions(sel_re, ev)
Region *sel_re;
event_t *ev;
{
	int buttons;

	buttons = ev->ev_buttons & B_ANY;

	if (buttons == 0)
		return;
	wn_next_event(WN_ANY, EV_BUTTON_UP, ev);
	if (ev->ev_wn != sel_re->wn)
		return;

#ifdef HH
	if (buttons == B_MIDDLE && (Debug_flags & DBFLAG_MISC) != 0) {
		static Region *last_middle_re;

		/*  The DECstation 3100 compiler objects to the following
		 *  comparison unless the (noop) cast is in place ...
		 */
		if (sel_re->draw_proc == (re_draw_proc_t)grey_out)
			return;

		if (sel_re == last_middle_re) {
			errf("I did say not to press this button again ...");
			sel_re->draw_proc = grey_out;
			sel_re->input_proc = NULL;
			sel_re->destroy_proc = NULL;
			re_expose(sel_re, TRUE);
		}
		else {
			errf("Please do not press this button again");
			last_middle_re = sel_re;
		}
	}
#endif /* HH */

	if (buttons & (B_LEFT | B_RIGHT))
		rswap_regions(sel_re, ev, buttons == B_LEFT);
}

static void
rswap_regions(sel_re, ev, going_up)
Region *sel_re;
event_t *ev;
bool going_up;
{
	Region *re, *par;
	int dim;

	if ((par = sel_re->parent) == NULL)
		return;
	dim = (par->flags & RF_VSPLIT) ? XPOS_OR_WIDTH : YPOS_OR_HEIGHT;

	re = sel_re;
	do {
		re = going_up ? prev_re(re) : re->next;
	} while (re != NULL && re->size[dim] <= 1);

	if (re == NULL)
		rswap_regions(par, ev, going_up);
	else {
		swap_children(re, sel_re);
		wn_swap_wins(re->wn, sel_re->wn);
		update_window_positions(par, dim);
		re_expose(par, TRUE);
		wn_warp_mouse(ev->ev_wn, ev->ev_x, ev->ev_y);
	}
}

static void
swap_children(re1, re2)
Region *re1, *re2;
{
	Region **tab, *re, *par;
	int nregs, i, re1pos, re2pos;
	
	par = re1->parent;
	if (re2->parent != par)
		panic("par botch in sc");

	re1pos = re2pos = 0;	/* to satisfy gcc */
	nregs = 0;
	for (re = par->subregs; re != NULL; re = re->next) {
		if (re == re1)
			re1pos = nregs;
		if (re == re2)	
			re2pos = nregs;
		++nregs;
	}

	tab = (Region **)e_malloc(nregs * sizeof(Region *));
	for (i = 0, re = par->subregs; re != NULL; ++i, re = re->next)
		tab[i] = re;
	
	re = tab[re1pos];
	tab[re1pos] = tab[re2pos];
	tab[re2pos] = re;

	re = NULL;
	for (i = nregs - 1; i >= 0; --i) {
		tab[i]->next = re;
		re = tab[i];
	}
	par->subregs = re;

	free((char *)tab);
}

int
re_change_position(mv_re, delta)
Region *mv_re;
int delta;
{
	int dim, avail, total_vpixels, needed;
	bool going_up;
	Region *re, *exp_re, *par, *first_re, *last_re;

	par = mv_re->parent;

	dim = (par->flags & RF_VSPLIT) ? XPOS_OR_WIDTH : YPOS_OR_HEIGHT;

	going_up = delta < 0;
	if (going_up)
		delta = -delta;

	/*  Check that we have pixels to spare in the area that's
	 *  getting smaller.
	 */
	if (going_up)
		avail = avail_pixels(par->subregs, mv_re, dim);
	else
		avail = avail_pixels(mv_re->next, (Region *)NULL, dim);
	if (avail < delta)
		delta = avail;

	/*  Find a variable size region in the expanded area to put the
	 *  extra pixels.
	 */
	exp_re = mv_re;
	do {
		exp_re = going_up ? exp_re->next : prev_re(exp_re);
	} while (exp_re != NULL && (exp_re->flags & RF_FIXED_SIZE) != 0);

	if (exp_re == NULL || delta == 0)
		return 0;
	
	/*  Find out how far we have to go to get the pixels we want in
	 *  the area that's getting smaller.  We do this before doing
	 *  any actual size changes as we want to undraw (with the old
	 *  sizes) the regions that are going to move.
	 */
	re = mv_re;
	needed = delta;
	while (needed > 0) {
		int spare;

		re = going_up ? prev_re(re) : re->next;
		if (re == NULL)
			panic("pixel famine");

		spare = re->size[dim] - re->minsize[dim];
		if (spare != 0 && (re->flags & RF_FIXED_SIZE) == 0)
			needed -= spare;
	}

	if (going_up) {
		first_re = re;
		last_re = exp_re->next;
	}
	else {
		first_re = exp_re;
		last_re = re->next;
	}

	/*  OK, we have some pixels and we know which regions are going to
	 *  change.  We are definitely going to munge the display, so do the
	 *  undraw now.
	 */
	wn_updating_off(par->wn);
	for (re = first_re; re != last_re; re = re->next)
		expose(re, RE_UNDRAW, 0, 0, re->size[WIDTH], re->size[HEIGHT], FALSE);
	 
	total_vpixels = par->size[dim] - par->minsize[dim];

	/*  First do the compressed area.  We search back for a region with spare
	 *  pixels, and take all the spare pixels or delta, whichever is greater.
	 *  We keep going like this until we have all the pixels we want.
	 */
	re = mv_re;
	needed = delta;
	while (needed != 0) {
		int spare;

		re = going_up ? prev_re(re) : re->next;
		if (re == NULL)
			panic("pixel famine");

		spare = re->size[dim] - re->minsize[dim];
		if (spare == 0 || (re->flags & RF_FIXED_SIZE))
			continue;
		
		if (spare > needed)
			spare = needed;
		
		/*  Steal the pixels from the region, and change re_prop
		 *  to that the new proportions will be preserved on a
		 *  window size change.
		 */
		re->size[dim] -= spare;
		re->prop -= (double)spare / total_vpixels;
		re->flags |= RF_SIZE_CHANGED;

		needed -= spare;
	}

	/*  Add delta pixels to the region to be expanded and munge it's
	 *  proportion to match.
	 */
	exp_re->size[dim] += delta;
	exp_re->prop += (double)delta / total_vpixels;
	exp_re->flags |= RF_SIZE_CHANGED;

	/*  Update the window positions.
	 */
	update_window_positions(par, dim);

	for (re = first_re; re != last_re; re = re->next)
		expose(re, RE_RESIZING, 0, 0, re->size[WIDTH], re->size[HEIGHT], FALSE);
	wn_updating_on(par->wn);

	return going_up ? -delta : delta;
}

Region *
re_get_parent(region)
Region *region;
{
	return region->parent;
}

/*  Set up a typing line region, and the "extended" region containing it
**  The "extended" region is a possibly larger region that should be
**  used to prompt for input.
*/
void
re_set_typing_regions(typing_line, extended)
Region* typing_line;
Region* extended;
{
    int index =0;
    while (typing_lines[index].root_win >= 0)
	index++;

    typing_lines[index].root_win = wn_get_root_window(typing_line->wn);
    typing_lines[index].typing_line = typing_line;
    typing_lines[index].extended = extended;
}

/* See that the current typing line is in the same main window as the cursor */
static void
set_current_typing_region(wn)
int wn;
{
    static int last_wn = -1;
    int root;

    if (last_wn >= 0 && wn_same_root_window(wn,  last_wn))
	return;

    root = wn_get_root_window(wn);
    if ( root>=0)
    {
	bool saw_Keyboard_re = FALSE;
	int index = -1;
	int i;
	for ( i = 0; i < numRegions; i++)
	{
	    if (typing_lines[i].root_win == root)
		index = i;
	    if ( Keyboard_re == typing_lines[i].typing_line)
		saw_Keyboard_re = TRUE;
	}

	if ( index >= 0)
	{
	    Default_keyboard_re = typing_lines[index].typing_line;
	    set_typing_line_region(Default_keyboard_re);
	    set_extended_typing_line_region(typing_lines[index].extended);

	    if (saw_Keyboard_re)
		re_set_keyboard_focus(Default_keyboard_re, NULL);
	}
    }
    last_wn = wn;
}

/*  If a lead window is iconified, iconify all the others.
**  If a lead window is de-iconified, de-iconify all others.
*/
static void icon_notify( wn, iconify)
int wn;
int iconify;
{
   static int icon_state[MAXROOTS];
   static int was_icon_state[MAXROOTS];
   int i,j;
   int do_raise = FALSE;
   wn = wn_get_root_window(wn);
   for (i = 0; i < numRegions; i++)
      if ( wn==Root_regions[i]->wn)
          break;

   if ( i >= numRegions)
       return;  /* should never happen */

   icon_state[i] = iconify;

   /* If it is a leader window, iconify/deiconify all the others */
   if (   iconic_state != iconify
       && IsLeadRegion[i])
   {
        iconic_state = iconify;
        for (j = 0; j < numRegions; j++)
        {
            if ( j == i)
                continue;
            if( iconify)
            {
                was_icon_state[j] = icon_state[j];
                wn_iconify_window(Root_regions[j]->wn);
            } else if ( !was_icon_state[j])
            {
                wn_raise_window(Root_regions[j]->wn);
                do_raise = TRUE;
            }
        }
        /* If we raised anything, put the main window on top */
        if (do_raise)
            wn_raise_window(wn);
    }
}

/* Close the window, if it is not a lead window, otherwise quit ups
*/
static void
close_window(wn)
int wn;
{
  extern MENU tgt_stop_men;
  int i;
  wn = wn_get_root_window(wn);
  for (i = 0; i < numRegions; i++)
      if ( wn==Root_regions[i]->wn)
          break;

  if ( i >= numRegions)
       return;  /* should never happen */

   if ( IsLeadRegion[i] )
   {
#if WANT_ICONIFY_WM_DELETE
        /* Iconify the lead window, thus the entire app */
        wn_iconify_window(wn);
#else
	/* Quit - could cause unpleasant surprises */
        if (Mselectable(&tgt_stop_men))
            target_stop();
        re_set_exit_event_loop_flag();
#endif
   } else
   {
        /* "close" the window
        **  we just unmap it, so it doesn't get displayed
        */
        wn_unmap_window(wn);
   }
}

int
re_in_icon_state()
{
   return iconic_state;
}
