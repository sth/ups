/* obj_wpt.c - watchpoint object handling */

/*  Copyright 2002 Tom Hughes <thh@cyberscience.com>
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


#include <mtrprog/ifdefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include <local/wn.h>
#include <local/ukcprog.h>
#include <local/obj/obj.h>
#include <local/edit/edit.h>

#include "objtypes.h"
#include "obj_util.h"
#include "ups.h"
#include "symtab.h"
#include "ui.h"
#include "target.h"
#include "watchpoint.h"
#include "obj_wpt.h"
#include "menudata.h"
#include "tdr.h"
#include "state.h"
#include "srcwin.h"

typedef struct wpdescst {
	taddr_t addr;
	size_t size;
	char *decl;
	bool inactive;
	watchpoint_t *watchpoint;
} wpdesc_t;

static void wpaddr_draw PROTO((struct drawst *dets));
static void wpdecl_draw PROTO((struct drawst *dets));
static void wpstate_draw PROTO((struct drawst *dets));
static void set_wpt_state PROTO((objid_t obj, bool disable, bool verbose));
static void enable_all_watchpoints PROTO((bool enable));
static bool move_wpt PROTO((objid_t obj, char *new_text, bool edit_size,
			   const char **p_display_string));
static int wps_accelerator_action PROTO((void));
static bool wpaddr_quitfunc PROTO((objid_t obj, char *new_text,
				   const char **p_display_string));
static bool wpsize_quitfunc PROTO((objid_t obj, char *new_text,
				   const char **p_display_string));
static bool wpstate_quitfunc PROTO((objid_t obj, char *new_text,
				    const char **p_display_string));
static void wpaddr_edit PROTO((struct drawst));
static void wpsize_edit PROTO((struct drawst));

/* Maximum lengths of watchpoint addresses and sizes
 */
#define LEN_WP_ADDR	60
#define LEN_WP_SIZE	10

const char Wpt_format[] = "address:%[-]16cw      %[-]:3:39cdlength:%[B]8cW  %[-]9cQ   \n";
/*  Field numbers for watchpoints
 */
#define FN_WPT_ADDR	0
#define FN_WPT_DECL	1
#define FN_WPT_SIZE	2
#define FN_WPT_STATE	3
#define FN_WPT_LAST	4

/*  Flist terminator for watchpoint fields for set_all_fields().
 *  Can't use NULL as one of the fields is an int which can legitimately
 *  have the value 0.
 */
#define WPT_FLIST_TERMINATOR	((fval_t)-1)

fnamemap_t Wpt_fnamemap[] = {
	{ FN_WPT_ADDR,	"address",	FALSE, wpaddr_quitfunc,  NULL	},
	{ FN_WPT_SIZE,	"length",	FALSE, wpsize_quitfunc,  NULL	},
	{ FN_WPT_STATE,	"state",	FALSE, wpstate_quitfunc, NULL	},
	{ 0,		NULL,		FALSE, NULL,		 NULL	},
};

const char Wphead_format[] = "Watchpoints\n";

#define WPHEAD_OBJCODE	((objid_t)Wphead_format)

fdef_t Wpt_fdefs[] = {
	{ 'w', wpaddr_draw, wpaddr_edit, NULL	},
	{ 'd', wpdecl_draw, no_edit, NULL	},
	{ 'W', n_draw, wpsize_edit, NULL	},
	{ 'Q', wpstate_draw, no_edit, NULL	},
	{ 0, NULL, NULL, NULL			},
};

#define REFRESH_NOTSET	-2
#define REFRESH_OFF	-1

static int Watchpoint_refresh_interval = REFRESH_NOTSET;

bool
global_wp_enabled(set, reset)
bool set;
bool reset;
{
	static bool enabled = TRUE;

	if (set)
		enabled = TRUE;
	if (reset)
		enabled = FALSE;

	return enabled;
}

void
wpaddr_draw(dets)
struct drawst *dets;
{
	char buf[20];

	buf[0] = 0;
	if ((long)dets->dr_fval)
	  sprintf(buf, "0x%lx", (long)dets->dr_fval);

	if (dets->selected)
		wn_wd_ttext(dets->dr_wn, buf, dets->dr_x, dets->dr_y,
			    dets->dr_width, dets->dr_bg, dets->dr_fg);
	else
		wn_wd_ttext(dets->dr_wn, buf, dets->dr_x, dets->dr_y,
			    dets->dr_width, dets->dr_fg, dets->dr_bg);
}

static void
wpdecl_draw(dets)
register struct drawst *dets;
{
	int x, len, maxlen;
	bool too_long;
	char *s;
	font_t *font;
	
	font = wn_get_sysfont();

	s = (char *)dets->dr_fval;

	len = strlen(s);
	maxlen = dets->dr_width / font->ft_width;

	too_long = len > maxlen;
	if (too_long) {
		s += len - maxlen;
		len = maxlen;
	}

	x = dets->dr_x;

	if (too_long) {
		static const char dots[] = "...";

		s += sizeof(dots) - 1;
		if (*s == '/' && s[1] != '/') {
			++s;
			x += font->ft_width;
		}
/* AMC enhancements */
	if (dets->selected)
	    wn_wd_ttext(dets->dr_wn, dots, x, dets->dr_y,
			dets->dr_width, dets->dr_bg, dets->dr_fg);
	  else
	    wn_wd_ttext(dets->dr_wn, dots, x, dets->dr_y,
			dets->dr_width, dets->dr_fg, dets->dr_bg);
	  x += (sizeof(dots) - 1) * font->ft_width;
	}

/* AMC enhancements */
	if (dets->selected)
	    wn_wd_ttext(dets->dr_wn, s, x, dets->dr_y, dets->dr_width,
			dets->dr_bg, dets->dr_fg);
	else
	    wn_wd_ttext(dets->dr_wn, s, x, dets->dr_y, dets->dr_width,
			dets->dr_fg, dets->dr_bg);
}

static void
wpstate_draw(dets)
register struct drawst *dets;
{
	long fg;
	static int color_done = -1;
	static color_t color_fg;
	int wp_enabled, use_alloc_color;

	if (color_done == -1)
	{
	    const char *colorname;

	    color_fg.co_pixel = WN_FG(dets->dr_wn);
	    if (wn_use_extended_text_colors() || wn_use_menu_colors()) {
		colorname = wn_get_default("DisabledTextColor");
		if (colorname == NULL)
			colorname = "gray";
		wn_parse_color(dets->dr_wn, colorname, &color_fg);
		if (!wn_alloc_color(dets->dr_wn, &color_fg, colorname))
			color_fg.co_pixel = WN_FG(dets->dr_wn);
		color_done = 1;	/* done and using colors */
	    }
	}

	wp_enabled = global_wp_enabled(FALSE, FALSE);
	use_alloc_color = wn_use_alloc_color(0);

	if (!wp_enabled && use_alloc_color)
	    fg = color_fg.co_pixel;
	else
	    fg = dets->dr_fg;

	if (dets->selected)
	    wn_wd_ttext(dets->dr_wn, (char *)dets->dr_fval,
			dets->dr_x, dets->dr_y, dets->dr_width,
			dets->dr_bg, fg);
	else
	    wn_wd_ttext(dets->dr_wn, (char *)dets->dr_fval,
			dets->dr_x, dets->dr_y, dets->dr_width,
			fg, dets->dr_bg);
	/*
	**   If globally disabled, grey out the field
	*/
	if (!wp_enabled && !use_alloc_color) {
		wn_shade_area(  dets->dr_wn
			      , dets->dr_x, dets->dr_y
			      , dets->dr_width, dets->dr_depth
			      , WN_GREY12, R_AND);
	}
}

int
wpt_accelerator_action(obj)
objid_t obj;
{
   static enum { WPQ_FIRSTTIME, WPQ_REMOVE, WPQ_TOGGLE } what = WPQ_FIRSTTIME;
   int command;

   /*
    *   Consult with the X resources to find out
    *   what the user likes to see for an accelerator
    *   when a watchpoint is double clicked on.
    *   Remove it, or toggle its state?
    */
   if (what == WPQ_FIRSTTIME) {
       /*
        *  First time here.  Consult X Resources to so what
        *  the user wants to do on a quick change (Left-Click or double-Click)
        */
       const char *res = wn_get_default("WatchPointAcceleratorAction");

       what = WPQ_TOGGLE;  /* Assume default */

       if (res) {
	  if (!strcmp(res, "Toggle"))
	     what = WPQ_TOGGLE;
	  else if (!strcmp(res, "Remove"))
	     what = WPQ_REMOVE;
       }
   }

   if (what == WPQ_REMOVE) {
	command = MR_WPTMEN_REMOVE;
   }
   else {
	wpdesc_t* wd = (wpdesc_t *)obj;

	if (wd->inactive)
	   command = MR_WPTMEN_ACTIVE;
	else
	   command = MR_WPTMEN_INACTIVE;
   }

   return command;
}

static int
wps_accelerator_action()
{
   static enum { WPHQ_FIRSTTIME, WPHQ_ADD, WPHQ_TOGGLE } what = WPHQ_FIRSTTIME;
   int command;

   /*
    *   Consult with the X resources to find out
    *   what the user likes to see for an accelerator
    *   when the watchpoints header double clicked on.
    *   Add a new watchpoint or toggle active/inactive
    */
   if (what == WPHQ_FIRSTTIME) {
       /*
        *  First time here.  Consult X Resources to so what
        *  the user wants to do on a quick change (Left-Click or double-Click)
        */
       const char *res = wn_get_default("WatchPointHeaderAcceleratorAction");

       what = WPHQ_TOGGLE;  /* Assume default */

       if (res) {
	  if (!strcmp(res, "Toggle"))
	     what = WPHQ_TOGGLE;
	  else if (!strcmp(res, "Add"))
	     what = WPHQ_ADD;
       }
   }

   if (what == WPHQ_ADD) {
	command = MR_ADD_WATCHPOINT;
   }
   else {
	if (global_wp_enabled(FALSE, FALSE))
	   command = MR_DISABLE_ALL_WATCHPOINTS;
	else
	   command = MR_ENABLE_ALL_WATCHPOINTS;

   }

   return command;
}

/*  Make a new watchpoint object.
 */
objid_t
add_watchpoint_object(addr, size, decl)
taddr_t addr;
size_t size;
const char *decl;
{
	fval_t fields[FN_WPT_LAST + 1];
	wpdesc_t *wd;
	target_t *xp;

	xp = get_current_target();

	if (addr != 0 && size > 0) {
		objid_t obj;

		for (obj = get_code(WPHEAD_OBJCODE, OBJ_CHILD);
		     obj != NULL;
		     obj = get_code(obj, OBJ_NEXT))
		{
			wd = (wpdesc_t *)obj;
			if (addr >= wd->addr &&
			    addr + size <= wd->addr + wd->size)
				break;
		}

		if (obj) {
			errf("There is already a watchpoint on address 0x%x", addr);
			return NULL;
		}
	}

	if (!decl) {
		decl = "";
	}

	wd = (wpdesc_t *)e_malloc(sizeof(*wd));
	wd->addr = addr;
	wd->size = size;
	wd->decl = strsave(decl);
	wd->inactive = FALSE;
	wd->watchpoint = NULL;

	fields[FN_WPT_ADDR] = (fval_t)wd->addr;
	fields[FN_WPT_DECL] = (fval_t)wd->decl;
	fields[FN_WPT_SIZE] = (fval_t)wd->size;
	fields[FN_WPT_STATE] = (fval_t)"active";
	fields[FN_WPT_LAST] = WPT_FLIST_TERMINATOR;

	td_set_obj_updating(OBJ_UPDATING_OFF);
	new_object((objid_t)wd, OT_WPT, WPHEAD_OBJCODE, OBJ_LAST_CHILD);
	set_all_fields((objid_t)wd, fields, WPT_FLIST_TERMINATOR);
	td_set_obj_updating(OBJ_UPDATING_ON);
	ensure_visible((objid_t)wd);

	if (wd->addr != 0 && wd->size > 0) {
		wd->watchpoint = xp_add_watchpoint(xp, wd->addr, wd->size);
	}
	else if (wd->addr == 0) {
		edit_field_obj((objid_t)wd, FN_WPT_ADDR);
	}
	else if (wd->size == 0) {
		edit_field_obj((objid_t)wd, FN_WPT_SIZE);
	}

	return (objid_t)wd;
}

/*  Attempt to move a watchpoint after editing the address or length.
 */
static bool
move_wpt(obj, newstring, edit_size, p_display_string)
objid_t obj;
char *newstring;
bool edit_size;
const char **p_display_string;
{
	fval_t fields[FN_WPT_LAST + 1];
	wpdesc_t *wd;
	target_t *xp;

	xp = get_current_target();
	wd = (wpdesc_t *)obj;

	if (edit_size) {
		wd->size = atoi(newstring);
		if (wd->size == 0) {
			errf("Zero length watchpoint invalid");
			return FALSE;
		}
	}
	else {
#if HAVE_STRTOUL
		wd->addr = strtoul(newstring, NULL, 0);
#else
		wd->addr = strtol(newstring, NULL, 0);
#endif
		if (wd->addr == 0) {
			errf("Not a valid watchpoint address");
			return FALSE;
		}
	}

	if (wd->watchpoint)
		xp_remove_watchpoint(xp, wd->watchpoint);
	/* RGA always add watchpoint */
	/*	if (wd->addr != 0 && wd->size != 0)*/
	wd->watchpoint = xp_add_watchpoint(xp, wd->addr, wd->size);

	get_all_fields(obj, fields);
	fields[FN_WPT_ADDR] = (fval_t)wd->addr;
	fields[FN_WPT_DECL] = (fval_t)"";
	fields[FN_WPT_SIZE] = (fval_t)wd->size;
	fields[FN_WPT_LAST] = WPT_FLIST_TERMINATOR;
	set_all_fields(obj, fields, WPT_FLIST_TERMINATOR);

	return TRUE;
}

void
remove_watchpoint_object(obj)
objid_t obj;
{
	wpdesc_t *wd;

	wd = (wpdesc_t *)obj;

	free(wd->decl);

	if (wd->watchpoint != NULL)
		xp_remove_watchpoint(get_current_target(), wd->watchpoint);

	free((char *)wd);
}

static void
set_wpt_state(obj, disable, verbose)
objid_t obj;
bool disable;
bool verbose;
{
	fval_t fields[FN_WPT_LAST + 1];
	target_t *xp;
	wpdesc_t *wd;

	xp = get_current_target();

	if (verbose)
	{
	  if (disable)
	    errf("\bSelected watchpoint code will be ignored");
	  else
	    if (global_wp_enabled(FALSE, FALSE))
	      errf("\bSelected watchpoint code will be executed");
	    else
	      errf("\bSelected watchpoint code will be executed when globally enabled");
	}
	wd = (wpdesc_t *)obj;
	wd->inactive = disable;
	if (wd->watchpoint && disable)
		xp_disable_watchpoint(xp, wd->watchpoint);
	else if (wd->watchpoint && global_wp_enabled(FALSE, FALSE))
		xp_enable_watchpoint(xp, wd->watchpoint);
	get_all_fields(obj, fields);
	fields[FN_WPT_STATE] = disable ? (fval_t)"inactive" : (fval_t)"active";
	fields[FN_WPT_LAST] = WPT_FLIST_TERMINATOR;
	set_all_fields(obj, fields, WPT_FLIST_TERMINATOR);
}

static void
enable_all_watchpoints(enable)
bool enable;
{
	target_t *xp;
	objid_t obj;

	xp = get_current_target();

	for (obj = get_code(WPHEAD_OBJCODE, OBJ_CHILD);
	     obj != NULL;
	     obj = get_code(obj, OBJ_NEXT))
	{
		wpdesc_t *wd = (wpdesc_t *)obj;

		if (enable && !wd->inactive)
			xp_enable_watchpoint(xp, wd->watchpoint);
		else
			xp_disable_watchpoint(xp, wd->watchpoint);

		obj_has_changed(obj);
	}
}

char *
wpt_format_obj(code)
objid_t code;
{
	taddr_t addr;
	size_t size;
        char *decl, *state;

	addr = (taddr_t)get_field_value(code, FN_WPT_ADDR);
	decl = (char *)get_field_value(code, FN_WPT_DECL);
	size = (size_t)get_field_value(code, FN_WPT_SIZE);
	state = (char *)get_field_value(code, FN_WPT_STATE);

	return addr ? strf("0x%x %s %d %s", addr, decl, size, state) : strf("");
}

/*  Process the return from the watchpoint menu.
 */
void
do_wpt(obj, command, arg)
objid_t obj;
int command;
char *arg;
{
	int oldstate;

	if (command == MR_WPTMEN_ACCELERATOR_ACTION)
		command = wpt_accelerator_action(obj);

	display_area_overlay_unselect_global_selection();
	oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);

	switch(command) {
	case MR_WPTMEN_REMOVE:
		remove_object(obj, OBJ_SELF);
		break;
	case MR_WPTMEN_ACTIVE:
		set_wpt_state(obj, FALSE, TRUE);
		break;
	case MR_WPTMEN_INACTIVE:
		set_wpt_state(obj, TRUE, TRUE);
		break;
	default:
		panic("bad rv in dw");
	}

	td_set_obj_updating(oldstate);
}


/*  Add the watchpoint header to the display.  Called on startup from main().
 */
void
add_watchpoint_header(par)
objid_t par;
{
	new_object(WPHEAD_OBJCODE, OT_WPHEAD, par, OBJ_CHILD);
}

int
get_watchpoint_refresh_interval()
{
	const char *interval;

	if (Watchpoint_refresh_interval == REFRESH_NOTSET) {
		if ((interval = wn_get_default("WatchPointRefreshInterval")) != NULL) {
			Watchpoint_refresh_interval = atoi(interval);
		}
		else {
			Watchpoint_refresh_interval = REFRESH_OFF;
		}
	}

	return Watchpoint_refresh_interval;
}

/*  Process the return from the watchpoint header menu.
 */
void
do_wps(unused_obj, command, arg)
objid_t unused_obj;
int command;
char *arg;
{
	if (command == MR_WP_ACCELERATOR_ACTION)
		command = wps_accelerator_action();

	switch(command) {
	case MR_ADD_WATCHPOINT:
		add_watchpoint_object(0, 4, NULL); /* RGA default to 4 bytes */
		break;
	case MR_REMOVE_ALL_WATCHPOINTS:
		remove_object(WPHEAD_OBJCODE, OBJ_CHILDREN);
		break;
	case MR_ENABLE_ALL_WATCHPOINTS:
		global_wp_enabled(TRUE, FALSE); /* set */
		enable_all_watchpoints(TRUE);
		set_wphead_menu_state(1);
		errf("\bGlobally enable all active watchpoints");
		break;
	case MR_DISABLE_ALL_WATCHPOINTS:
		global_wp_enabled(FALSE, TRUE); /* reset */
		enable_all_watchpoints(FALSE);
		set_wphead_menu_state(0);
		errf("\bGlobally disable all watchpoints");
		break;
	default:
		panic("bad cmd in dw");
	}
}

static bool
wpaddr_quitfunc(obj, new_text, p_display_string)
objid_t obj;
char *new_text;
const char **p_display_string;
{
	return move_wpt(obj, new_text, FALSE, p_display_string);
}

static bool
wpsize_quitfunc(obj, new_text, p_display_string)
objid_t obj;
char *new_text;
const char **p_display_string;
{
	return move_wpt(obj, new_text, TRUE, p_display_string);
}

static bool
wpstate_quitfunc(obj, new_text, p_display_string)
objid_t obj;
char *new_text;
const char **p_display_string;
{
	return TRUE;
}

static void
wpaddr_edit(fdets)
struct drawst fdets;
{
	field_edit_start(&fdets, "watchpoint address",
			 (long)fdets.dr_fval ? strf("0x%lx", (long)fdets.dr_fval) :
			 strf(""));
}

static void
wpsize_edit(fdets)
struct drawst fdets;
{
	field_edit_start(&fdets, "watchpoint length",
			 strf("%ld", (long)fdets.dr_fval));
}

void
wpt_getcolor(obj, wn, p_fg, p_bg)
objid_t obj;
int wn;
long *p_fg, *p_bg;
{
	static color_t color = { -1, 0, 0, 0 };
	wpdesc_t *wd;

	wd = (wpdesc_t *)obj;

	if (color.co_pixel == -1)
	{
		const char *colorname;

		color.co_pixel = WN_FG(wn);
		if (wn_use_extended_text_colors() == TRUE) {
			colorname = wn_get_default("WatchpointTextColor");
			if (colorname != NULL) {
				wn_parse_color(wn, colorname, &color);
				if (!wn_alloc_color(wn, &color,colorname))
					color.co_pixel = WN_FG(wn);
			}
		}
	}

	if (wd && wd->inactive)
		*p_fg = WN_FG(wn);
	else
		*p_fg = color.co_pixel;
	*p_bg = WN_BG(wn);
}
