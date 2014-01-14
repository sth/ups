/* ui_layout.c - division of the window into regions, and window (re)drawing */

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


/* @(#)ui_layout.c	1.32 20 Jun 1995 (UKC) */
char ups_ui_layout_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <local/wn.h>
#include <local/menu3.h>

#include <local/ukcprog.h>
#include <local/obj/obj.h>
#include <local/obj/o_priv.h>
#include <local/obj/o_mkobj.h>
#include <local/obj/o_sel.h>
#include <local/obj/mhole.h>
#include <local/obj/newtb.h>
#include <local/edit/edit.h>
#include <local/edit/win.h>
#include <mtrprog/utils.h>

#include "cursors.h"
#include "reg.h"
#include "ups.h"
#include "symtab.h"
#include "srcwin.h"
#include "ui.h"
#include "tdr.h"
#include "ui_priv.h"
#include "state.h"
#include "menudata.h"
#include "debug.h"
#include "objtypes.h"
#include "obj_util.h"
#include "obj_misc.h"
#include "obj_env.h"
#include "obj_signal.h"
#include "obj_bpt.h"
#include "obj_wpt.h"
#include "obj_stack.h"
#include "va.h"
#include "sccsdata.h"
#include "util.h"
#include "target.h"
#include "st.h"
#include "exec.h"
#ifdef AO_ELF
#include "ao_elflib.h"
#endif

Edit_history* typing_line_history = NULL;

typedef struct {
	int ma_md;
	const char *ma_menu_name;
	bool ma_md_changed;
	void (*ma_menu_func)PROTO((char *data, int md, int rv));
	char *ma_data;
} menu_arg_t;

#define FIXED	0.0
#define REST	1.0

static void dragbox_input PROTO((Region *region, event_t *ev));
static void dragbox_draw PROTO((Region *region, int wn,
				    int width, int height,
				    re_draw_reason_t draw_reason));

static void historybutton_input PROTO((Region *region, event_t *ev));
static void historybutton_draw PROTO((Region *region, int wn,
				    int width, int height,
				    re_draw_reason_t draw_reason));
static void enterbutton_input PROTO((Region *region, event_t *ev));
static void enterbutton_draw PROTO((Region *region, int wn,
				    int width, int height,
				    re_draw_reason_t draw_reason));

static void line_input PROTO((Region *region, event_t *ev));
static void line_draw PROTO((Region *region, int wn,
				    int width, int height,
				    re_draw_reason_t draw_reason));

static void tbar_input PROTO((Region *region, event_t *ev));
static void tbar_draw PROTO((Region *region, int wn,
			     int width, int height,
			     re_draw_reason_t draw_reason));

static void outwin_draw PROTO((Region *region, int wn,
			       int width, int height,
			       re_draw_reason_t draw_reason));

static void srcwin_draw PROTO((Region *region, int wn,
			       int width, int height,
			       re_draw_reason_t draw_reason));

static void typing_line_input PROTO((Region *region, event_t *ev));
static void typing_line_draw PROTO((Region *region, int wn,
				    int width, int height,
				    re_draw_reason_t draw_reason));

static void display_area_input PROTO((Region *region, event_t *ev));
static void display_area_draw PROTO((Region *region, int wn,
			       int width, int height,
			       re_draw_reason_t draw_reason));

static void menu_draw PROTO((Region *region, int wn,
			     int width, int height,
			     re_draw_reason_t draw_reason));

static void srcpos_draw PROTO(( Region *region, int wn,
                                int width, int height,
                                re_draw_reason_t draw_reason ));

static void message_area_draw PROTO((Region *region, int wn,
				     int width, int height,
				     re_draw_reason_t draw_reason));

static void mousehole_draw PROTO((Region *region, int wn,
				  int width, int height,
				  re_draw_reason_t draw_reason));

static void initialise_menu PROTO((Region *region, int md,
			    const char *menu_name,
			    void (*menu_func)(char *data, int mfmd, int rv),
			    char *menu_func_data));
static int make_permanent_menu PROTO((Region* region));
static void create_srcwin PROTO((Region *region, double srcwin_fraction, int split));
static tbar_id_t make_tbar PROTO((Region *region));

static int tbar_srcwin_callback PROTO((char *data, tb_action_t action, int val));
static int tbar_outwin_callback PROTO((char *data, tb_action_t action, int val));
static int tbar_obj_callback PROTO((char *data, tb_action_t action, int val));

static void display_area_tbar_callback PROTO((char *data,
					      obj_change_t change, int val));
static void initialise_typing_line PROTO((Region *region));
static int get_default PROTO((const char *name, int default_value, int min_value));
static void output_window_menu_func PROTO((char *data, int md, int rv));
static void init_line_region PROTO((Region *line));
static bool typing_line_quit PROTO((Region *region, char *data, bool force));
static bool typing_line_accept_focus PROTO((Region *region, char *data));
static Edit_display* typing_line_get_display PROTO((Region *region));
static Edit_display* source_window_get_display PROTO((Region* region));
static Edit_display* display_area_get_display PROTO((Region *region));
static bool display_area_release_focus PROTO((Region *region, char *data, 
                                              bool force));
static void show_version PROTO((Edit_display *display));
static void create_outwin PROTO((Region *root, double outwin_fraction));
static bool load_outwin PROTO((Outwin *ow, const char *path));
static bool save_outwin PROTO((Outwin *ow, const char *path, const char *mode));
static void add_outwin PROTO((void));
static Outwin *create_display_area_overlay PROTO((int wn, tbar_id_t tbar));

static
Outwin *
create_display_area_overlay(wn, tbar)
int wn;
tbar_id_t tbar;
{
	Outwin *dw;

	dw = display_area_overlay_create(wn, tbar);
	return dw;
}

static int
get_default(name, default_value, min_value)
const char *name;
int default_value, min_value;
{
	const char *valstr;
	int value;

	/*  Some calls to us calculate default_value as 100 - percentage, so
	 *  set minumum constrain the default to ve above the minimum.
	 */
	if ((valstr = wn_get_default(name)) == NULL)
		return (default_value > min_value) ? default_value : min_value;

	if ((value = atoi(valstr)) >= min_value)
		return value;

	errf("Value %d for %s too small - using min value %d",
							value, name, min_value);
	return min_value;
}

/*  Do the initial division of the window into regions.
 *  The default configuration, not called if -split is used on the command line.
 */
Region *
divide_window_into_regions(wn, want_outwin)
int wn;
bool want_outwin;
{
	enum {
		TOP, DISP_AREA_AND_TB, TOPLEFT,
		TYPING_LINE_AND_PMENU, TARGET_MENU_AND_DRAGBOX,
		TYPING_LINE, DMENU, MESSAGE_AREA, DRAGBOX,
		PMENU, MOUSEHOLE,
		TYPING_LINE_ENTER_AREA, HISTORY_BUTTON, ENTER_BUTTON,
		DISPLAY_AREA_TB, DISPLAY_AREA,
		TARGET_MENU,
		START_TARGET_MENU, NEXT_TARGET_MENU, STEP_TARGET_MENU,
		CONT_TARGET_MENU, STOP_TARGET_MENU, ATTACH_TARGET_MENU,
		DETACH_TARGET_MENU, EXIT_TARGET_MENU,
		OUTPUT, MAX_REGNUM
	};
	Region *root, *line, *dragbox_line, *regs[MAX_REGNUM];
	target_menu_info_t tmbuf;
	font_t *sysfont, *menufont;
	Outwin *dw;

	/*  Dimensions and percentages */
	int disable_mousehole;
	int mousehole_width = 0, menu_height;
	int historybutton_width, enterbutton_width;
	int pmenu_width, tbar_width;
	int display_area_percent, srcwin_percent, outwin_percent;
	float display_area_fraction, srcwin_fraction, outwin_fraction;

	int i;
	int md;
	tbar_id_t obj_tbar_id;
	extern MENU tgt_start_men, tgt_restart_men, tgt_next_men, tgt_step_men;
	extern MENU
	  tgt_cont_men, tgt_stop_men, tgt_kill_men,
	  tgt_attach_men, tgt_detach_men;
	static MENU *tgtmenus[TM_NTAGS] = {
		&tgt_start_men, &tgt_next_men, &tgt_step_men,
		&tgt_cont_men, &tgt_stop_men, &tgt_attach_men,
		&tgt_detach_men, &tgt_kill_men
	};

	/*  Set up fixed dimensions.  Some of these vary with font heights.
	 */
	sysfont = wn_get_sysfont();
	menufont = Mstdfont();
	menu_height = menufont->ft_height + 8;

	/* RCB: Allow user to suppress the mousehole.  Useful for large fonts
	*/
	disable_mousehole =  ( wn_get_default("WantMousehole")
	     && !strcmp(wn_get_default("WantMousehole"), "no"));
	if (!disable_mousehole)
	    mousehole_width = get_default("MouseholeWidth", 18*sysfont->ft_width, 32);

	historybutton_width = get_default("HistoryButtonWidth", 15, 12);
	enterbutton_width = get_default("EnterButtonWidth", 15, 12);
	pmenu_width = get_default("PermanentMenuWidth", 28*menufont->ft_width, 5);
	tbar_width = get_default("ScrollbarWidth", 20, 5);

	if (wn_get_default("SrcwinPercent") != NULL) {
		srcwin_percent = get_default("SrcwinPercent", 50, 10);
		display_area_percent = get_default("DisplayAreaPercent",
							  100 - srcwin_percent, 1);
	}
	else {
		display_area_percent = get_default("DisplayAreaPercent", 50, 1);
		srcwin_percent = get_default("SrcwinPercent", 
						100 - display_area_percent, 1);
	}
				/* RGA make output window default smaller */
	outwin_percent = get_default("OutwinPercent", 10, 1);

	display_area_fraction = (double)display_area_percent / 100;
	srcwin_fraction = (double)srcwin_percent / 100;
	outwin_fraction = (double)outwin_percent / 100;

	for (i = 0; i < MAX_REGNUM; ++i)
		regs[i] = NULL;
	line = NULL;

	root = re_make_region(wn);

	re_divide(root, RE_HORIZONTAL,
		  &regs[TOP],	        	0,		0.0,
		  &line,			1,		FIXED,
		  RE_END);
	init_line_region(line);
	re_divide(root, RE_HORIZONTAL,
		  &regs[DISP_AREA_AND_TB],	0,	       display_area_fraction,
		  &line,			1,		FIXED,
		  &regs[TARGET_MENU_AND_DRAGBOX],menu_height,FIXED,
		  RE_END);
	dragbox_line = line;
	create_srcwin(root, srcwin_fraction, FALSE);

	if ( disable_mousehole )
	    regs[TOPLEFT] = regs[TOP];
	else
	   re_divide(regs[TOP], RE_VERTICAL,
		  &regs[TOPLEFT],		0,			1.0,
		  &line,			1,			FIXED,
		  &regs[MOUSEHOLE],	mousehole_width,	FIXED,
		  RE_END);
	
	re_divide(regs[TOPLEFT], RE_HORIZONTAL,
		  &regs[TYPING_LINE_AND_PMENU],	menu_height,	FIXED,
		  &line,				1,		FIXED,
		  &regs[DMENU],			menu_height,	FIXED,
		  &line,				1,		FIXED,
		  &regs[MESSAGE_AREA],		menu_height,	FIXED,
		  RE_END);
	
	re_divide(regs[TYPING_LINE_AND_PMENU], RE_VERTICAL,
		  &regs[TYPING_LINE_ENTER_AREA],		0,		REST,
		  &line,				1,		FIXED,
		  &regs[PMENU],			pmenu_width,	FIXED,
		  RE_END);

	re_divide(regs[TYPING_LINE_ENTER_AREA], RE_VERTICAL,
		  &regs[TYPING_LINE],		0,		REST,
		  &line,			1,		FIXED,
		  &regs[HISTORY_BUTTON],	historybutton_width,	FIXED,
		  &line,			1,		FIXED,
		  &regs[ENTER_BUTTON],		enterbutton_width,	FIXED,
		  RE_END);
	
	re_divide(regs[TARGET_MENU_AND_DRAGBOX], RE_VERTICAL,
		  &regs[TARGET_MENU],		0,		REST,
		  &line,				1,		FIXED,
		  &regs[DRAGBOX],			menu_height,	FIXED,
		  RE_END);
	
	re_divide(regs[TARGET_MENU], RE_VERTICAL,
		  &regs[START_TARGET_MENU],		0,		1.0,
		  &line,				1,		FIXED,
		  &regs[NEXT_TARGET_MENU],		0,		1.0,
		  &line,				1,		FIXED,
		  &regs[STEP_TARGET_MENU],		0,		1.0,
		  &line,				1,		FIXED,
		  &regs[CONT_TARGET_MENU],		0,		1.0,
		  &line,				1,		FIXED,
		  &regs[STOP_TARGET_MENU],		0,		1.0,
		  &line,				1,		FIXED,
		  &regs[ATTACH_TARGET_MENU],		0,		1.0,
		  &line,				1,		FIXED,
		  &regs[DETACH_TARGET_MENU],		0,		1.0,
		  &line,				1,		FIXED,
		  &regs[EXIT_TARGET_MENU],		0,		1.0,
		  RE_END);
	
	re_divide(regs[DISP_AREA_AND_TB], RE_VERTICAL,
		  &regs[DISPLAY_AREA_TB],		tbar_width,	FIXED,
		  &line,				1,		FIXED,
		  &regs[DISPLAY_AREA],		0,		1.0,
		  RE_END);
	
	md = make_permanent_menu(regs[PMENU]);
	Mdelete_entry(md,MR_RAISE_DISPLAY_WIN);
	Mdelete_entry(md,MR_RAISE_SRC_WIN);
	if (!want_split_output(0))
	{
	    Mdelete_entry(md,MR_RAISE_OUT_WIN);
	}

	initialise_menu(regs[DMENU], -1, NULL,
					dynamic_menu_func, (char *)NULL);
	set_dynamic_menu_region(regs[DMENU]);

	re_set_callbacks(regs[MESSAGE_AREA], message_area_draw,
				(re_input_proc_t)NULL, (re_destroy_proc_t)NULL);
	set_message_wn(re_get_wn(regs[MESSAGE_AREA]));

	if ( !disable_mousehole)
	    re_set_callbacks(regs[MOUSEHOLE], mousehole_draw,
				(re_input_proc_t)NULL, (re_destroy_proc_t)NULL);

	re_set_callbacks(regs[HISTORY_BUTTON], historybutton_draw,
					historybutton_input, (re_destroy_proc_t)NULL);
	re_set_margins(regs[HISTORY_BUTTON], 1, 1, 1, 1);
	re_set_mhcaps(regs[HISTORY_BUTTON], "menu", "paste", "(menu)");
	re_set_cursor(regs[HISTORY_BUTTON], CU_MENU);
	re_set_callbacks(regs[ENTER_BUTTON], enterbutton_draw,
					enterbutton_input, (re_destroy_proc_t)NULL);
	re_set_margins(regs[ENTER_BUTTON], 1, 1, 1, 1);
	re_set_mhcaps(regs[ENTER_BUTTON], "enter", "paste", "(menu)");
	re_set_cursor(regs[ENTER_BUTTON], CU_MENU);

	re_set_callbacks(regs[DRAGBOX], dragbox_draw,
					dragbox_input, (re_destroy_proc_t)NULL);
	re_set_margins(regs[DRAGBOX], 1, 1, 1, 1);
	re_set_data(regs[DRAGBOX], (char *)dragbox_line);
	re_set_mhcaps(regs[DRAGBOX], "drag", "up", "down");
	re_set_cursor(regs[DRAGBOX], CU_DRAGBOX);

	obj_tbar_id = make_tbar(regs[DISPLAY_AREA_TB]);
	tb_set_callback_and_data(obj_tbar_id, tbar_obj_callback, (char *)NULL);
	obj_set_callback_and_data(display_area_tbar_callback, (char *)obj_tbar_id);
	set_display_area_tbar(obj_tbar_id);

	re_set_callbacks(regs[DISPLAY_AREA], display_area_draw,
				display_area_input, (re_destroy_proc_t)NULL);
	re_set_keyboard_focus_callbacks(regs[DISPLAY_AREA],
					(re_accept_focus_proc_t)NULL,
					display_area_release_focus);
	re_set_minsize(regs[DISPLAY_AREA], 1, 2 * sysfont->ft_height + 3);
	re_set_display_callback(regs[DISPLAY_AREA], display_area_get_display);
	set_display_area_region(regs[DISPLAY_AREA]);
	re_set_cursor(regs[DISPLAY_AREA], CU_DISPLAY);
	re_set_mhcaps(regs[DISPLAY_AREA], "select", "edit", "(menu)");
	
	for (i = 0; i < TM_NTAGS; ++i) {
		tmbuf.tm_mdtab[i].md = Minsert(tgtmenus[i]);
		tmbuf.tm_mdtab[i].wn = re_get_wn(regs[START_TARGET_MENU + i]);
		tmbuf.tm_mdtab[i].region = regs[START_TARGET_MENU + i];
		initialise_menu(regs[START_TARGET_MENU + i],
				tmbuf.tm_mdtab[i].md, "tmenu", target_menu_func,
				(char *)NULL);
	}
	tmbuf.tm_current_md = -1;
	set_target_menu_info(&tmbuf);

	set_start_target_menu(tmbuf.tm_mdtab[TM_START].md);
	set_restart_target_menu(Minsert(&tgt_restart_men));
	Mnobox(get_restart_target_menu());

	initialise_typing_line(regs[TYPING_LINE]);
	re_set_keyboard_focus(regs[TYPING_LINE], (char *)NULL);
	re_set_default_keyboard_focus(regs[TYPING_LINE], (char *)NULL);
	re_set_typing_regions(regs[TYPING_LINE],regs[TYPING_LINE_ENTER_AREA]);

	dw = create_display_area_overlay
	  (re_get_wn(regs[DISPLAY_AREA]), obj_tbar_id);
	re_set_data(regs[DISPLAY_AREA], (char *)dw);
	set_display_area_overlay(dw);

	if (want_outwin)
		create_outwin(root, outwin_fraction);

	re_add_rootwin(root, TRUE);
	re_redraw_root(EV_WINDOW_RESIZED, TRUE);

	return root;
}

/*  Do the initial division of the display/pmenu/cmdmenu window into regions.
 *  Only called if -split is used on the command line.
 */
Region *
divide_window_into_regions1(wn, want_outwin)
int wn;
bool want_outwin;
{
	enum {
		TOP, DISP_AREA_AND_TB, TOPLEFT,
		TYPING_LINE_AND_PMENU, 
		TYPING_LINE, DMENU, MESSAGE_AREA,
		PMENU, MOUSEHOLE,
		TYPING_LINE_ENTER_AREA, HISTORY_BUTTON, ENTER_BUTTON,
		DISPLAY_AREA_TB, DISPLAY_AREA,
		MAX_REGNUM
	};
	Region *root, *line, *regs[MAX_REGNUM];
	font_t *sysfont, *menufont;
	Outwin *dw;

	/*  Dimensions and percentages */
	int disable_mousehole;
	int mousehole_width = 0, menu_height;
	int historybutton_width, enterbutton_width;
	int pmenu_width, tbar_width;

	int i;
	int md;
	tbar_id_t obj_tbar_id;

	/*  Set up fixed dimensions.  Some of these vary with font heights.
	 */
	sysfont = wn_get_sysfont();
	menufont = Mstdfont();
	menu_height = menufont->ft_height + 8;

	/* RCB: Allow user to suppress the mousehole.  Useful for large fonts
	*/
	disable_mousehole =  ( wn_get_default("WantMousehole")
	     && !strcmp(wn_get_default("WantMousehole"), "no"));
	if (!disable_mousehole)
	    mousehole_width = get_default("MouseholeWidth", 18*sysfont->ft_width, 5);


	historybutton_width = get_default("HistoryButtonWidth", 15, 12);
	enterbutton_width = get_default("EnterButtonMenuWidth", 15, 12);
	pmenu_width = get_default("PermanentMenuWidth", 28*menufont->ft_width, 5);
	tbar_width = get_default("ScrollbarWidth", 20, 5);

	for (i = 0; i < MAX_REGNUM; ++i)
		regs[i] = NULL;
	line = NULL;

	root = re_make_region(wn);

	re_divide(root, RE_HORIZONTAL,
		  &regs[TOP],	        	0,		0.0,
		  &line,			1,		FIXED,
		  RE_END);
	init_line_region(line);
	re_divide(root, RE_HORIZONTAL,
		  &regs[DISP_AREA_AND_TB],	0,	        1.0,
		  &line,			1,		FIXED,
		  RE_END);

	if ( disable_mousehole )
	    regs[TOPLEFT] = regs[TOP];
	else
	   re_divide(regs[TOP], RE_VERTICAL,
		  &regs[TOPLEFT],		0,			1.0,
		  &line,			1,			FIXED,
		  &regs[MOUSEHOLE],	mousehole_width,	FIXED,
		  RE_END);
	
	re_divide(regs[TOPLEFT], RE_HORIZONTAL,
		  &regs[TYPING_LINE_AND_PMENU],	menu_height,	FIXED,
		  &line,				1,		FIXED,
		  &regs[DMENU],			menu_height,	FIXED,
		  &line,				1,		FIXED,
		  &regs[MESSAGE_AREA],		menu_height,	FIXED,
		  RE_END);
	
	re_divide(regs[TYPING_LINE_AND_PMENU], RE_VERTICAL,
		  &regs[TYPING_LINE_ENTER_AREA],		0,		REST,
		  &line,				1,		FIXED,
		  &regs[PMENU],			pmenu_width,	FIXED,
		  RE_END);

	re_divide(regs[TYPING_LINE_ENTER_AREA], RE_VERTICAL,
		  &regs[TYPING_LINE],		0,		REST,
		  &line,			1,		FIXED,
		  &regs[HISTORY_BUTTON],	historybutton_width,	FIXED,
		  &line,			1,		FIXED,
		  &regs[ENTER_BUTTON],		enterbutton_width,	FIXED,
		  RE_END);
	
	re_divide(regs[DISP_AREA_AND_TB], RE_VERTICAL,
		  &regs[DISPLAY_AREA_TB],		tbar_width,	FIXED,
		  &line,				1,		FIXED,
		  &regs[DISPLAY_AREA],		0,		1.0,
		  RE_END);
	
	md = make_permanent_menu(regs[PMENU]);
	Mdelete_entry(md,MR_RAISE_DISPLAY_WIN);
	if (!want_split_output(0))
	{
	    Mdelete_entry(md,MR_RAISE_OUT_WIN);
	}

	initialise_menu(regs[DMENU], -1, NULL,
					dynamic_menu_func, (char *)NULL);
	set_dynamic_menu_region(regs[DMENU]);

	re_set_callbacks(regs[MESSAGE_AREA], message_area_draw,
				(re_input_proc_t)NULL, (re_destroy_proc_t)NULL);
	set_message_wn(re_get_wn(regs[MESSAGE_AREA]));

	if ( !disable_mousehole)
	    re_set_callbacks(regs[MOUSEHOLE], mousehole_draw,
				(re_input_proc_t)NULL, (re_destroy_proc_t)NULL);

	re_set_callbacks(regs[HISTORY_BUTTON], historybutton_draw,
					historybutton_input, (re_destroy_proc_t)NULL);
	re_set_margins(regs[HISTORY_BUTTON], 1, 1, 1, 1);
	re_set_mhcaps(regs[HISTORY_BUTTON], "menu", "paste", "(menu)");
	re_set_cursor(regs[HISTORY_BUTTON], CU_MENU);
	re_set_callbacks(regs[ENTER_BUTTON], enterbutton_draw,
					enterbutton_input, (re_destroy_proc_t)NULL);
	re_set_margins(regs[ENTER_BUTTON], 1, 1, 1, 1);
	re_set_mhcaps(regs[ENTER_BUTTON], "enter", "paste", "(menu)");
	re_set_cursor(regs[ENTER_BUTTON], CU_MENU);

	obj_tbar_id = make_tbar(regs[DISPLAY_AREA_TB]);
	tb_set_callback_and_data(obj_tbar_id, tbar_obj_callback, (char *)NULL);
	obj_set_callback_and_data(display_area_tbar_callback, (char *)obj_tbar_id);
	set_display_area_tbar(obj_tbar_id);

	re_set_callbacks(regs[DISPLAY_AREA], display_area_draw,
				display_area_input, (re_destroy_proc_t)NULL);
	re_set_keyboard_focus_callbacks(regs[DISPLAY_AREA],
					(re_accept_focus_proc_t)NULL,
					display_area_release_focus);
	re_set_minsize(regs[DISPLAY_AREA], 1, 2 * sysfont->ft_height + 3);
	re_set_display_callback(regs[DISPLAY_AREA], display_area_get_display);
	set_display_area_region(regs[DISPLAY_AREA]);
	re_set_cursor(regs[DISPLAY_AREA], CU_DISPLAY);
	re_set_mhcaps(regs[DISPLAY_AREA], "select", "edit", "(menu)");
	
	initialise_typing_line(regs[TYPING_LINE]);
	re_set_typing_regions(regs[TYPING_LINE],regs[TYPING_LINE_ENTER_AREA]);

	dw = create_display_area_overlay
	  (re_get_wn(regs[DISPLAY_AREA]), obj_tbar_id);
	re_set_data(regs[DISPLAY_AREA], (char *)dw);
	set_display_area_overlay(dw);

	re_add_rootwin(root, TRUE);

	return root;
}

/*  Do the initial division of the source window and tmenu into regions.
 *  Only called if -split is used on the command line.
 */
Region *
divide_window_into_regions2(wn, want_outwin)
int wn;
bool want_outwin;
{
	enum {
		TOP,
		TYPING_LINE_AND_PMENU,
		TYPING_LINE,
		PMENU,
		TYPING_LINE_ENTER_AREA, HISTORY_BUTTON, ENTER_BUTTON,
		TARGET_MENU,
		START_TARGET_MENU, NEXT_TARGET_MENU, STEP_TARGET_MENU,
		CONT_TARGET_MENU, STOP_TARGET_MENU, ATTACH_TARGET_MENU,
		DETACH_TARGET_MENU, EXIT_TARGET_MENU,
		OUTPUT, MAX_REGNUM
	};
	Region *root, *line, *regs[MAX_REGNUM];
	target_menu_info_t tmbuf;
	font_t *menufont;

	/*  Dimensions and percentages */
	int menu_height;
	int historybutton_width, enterbutton_width;
	int pmenu_width;
	int outwin_percent;
	float srcwin_fraction, outwin_fraction;

	int i;
	int md;
	extern MENU tgt_start_men, tgt_restart_men, tgt_next_men, tgt_step_men;
	extern MENU
	  tgt_cont_men, tgt_stop_men, tgt_kill_men,
	  tgt_attach_men, tgt_detach_men;
	static MENU *tgtmenus[TM_NTAGS] = {
		&tgt_start_men, &tgt_next_men, &tgt_step_men,
		&tgt_cont_men, &tgt_stop_men, &tgt_attach_men,
		&tgt_detach_men, &tgt_kill_men
	};

	/*  Set up fixed dimensions.  Some of these vary with font heights.
	 */
	menufont = Mstdfont();
	menu_height = menufont->ft_height + 8;
	outwin_percent = get_default("OutwinPercent", 10, 1);
	outwin_fraction = (double)outwin_percent / 100;
	srcwin_fraction = 1.0;

	for (i = 0; i < MAX_REGNUM; ++i)
		regs[i] = NULL;
	line = NULL;

	root = re_make_region(wn);

	re_divide(root, RE_HORIZONTAL,
		  &regs[TOP],	        	0,		0.0,
		  &line,			1,		FIXED,
		  RE_END);
	init_line_region(line);

	historybutton_width = get_default("HistoryButtonWidth", 15, 12);
	enterbutton_width = get_default("EnterButtonMenuWidth", 15, 12);
	pmenu_width = get_default("PermanentMenuWidth", 28*menufont->ft_width, 5);

	re_divide(regs[TOP], RE_HORIZONTAL,
		  &regs[TYPING_LINE_AND_PMENU],	menu_height,	FIXED,
		  &line,				1,		FIXED,
		  &regs[TARGET_MENU],			menu_height,	FIXED,
		  RE_END);

	re_divide(regs[TYPING_LINE_AND_PMENU], RE_VERTICAL,
		  &regs[TYPING_LINE_ENTER_AREA],		0,		REST,
		  &line,				1,		FIXED,
		  &regs[PMENU],			pmenu_width,	FIXED,
		  RE_END);

	re_divide(regs[TYPING_LINE_ENTER_AREA], RE_VERTICAL,
		  &regs[TYPING_LINE],		0,		REST,
		  &line,			1,		FIXED,
		  &regs[HISTORY_BUTTON],	historybutton_width,	FIXED,
		  &line,			1,		FIXED,
		  &regs[ENTER_BUTTON],		enterbutton_width,	FIXED,
		  RE_END);
	create_srcwin(root, srcwin_fraction, TRUE);

	re_divide(regs[TARGET_MENU], RE_VERTICAL,
		  &regs[START_TARGET_MENU],		0,		1.0,
		  &line,				1,		FIXED,
		  &regs[NEXT_TARGET_MENU],		0,		1.0,
		  &line,				1,		FIXED,
		  &regs[STEP_TARGET_MENU],		0,		1.0,
		  &line,				1,		FIXED,
		  &regs[CONT_TARGET_MENU],		0,		1.0,
		  &line,				1,		FIXED,
		  &regs[STOP_TARGET_MENU],		0,		1.0,
		  &line,				1,		FIXED,
		  &regs[ATTACH_TARGET_MENU],		0,		1.0,
		  &line,				1,		FIXED,
		  &regs[DETACH_TARGET_MENU],		0,		1.0,
		  &line,				1,		FIXED,
		  &regs[EXIT_TARGET_MENU],		0,		1.0,
		  RE_END);
	
	md = make_permanent_menu(regs[PMENU]);
	Mdelete_entry(md,MR_RAISE_SRC_WIN);
	if (!want_split_output(0))
	{
	    Mdelete_entry(md,MR_RAISE_OUT_WIN);
	} 
	initialise_typing_line(regs[TYPING_LINE]);
	re_set_keyboard_focus(regs[TYPING_LINE], (char *)NULL);
	re_set_default_keyboard_focus(regs[TYPING_LINE], (char *)NULL);
	re_set_typing_regions(regs[TYPING_LINE],regs[TYPING_LINE_ENTER_AREA]);

	re_set_callbacks(regs[HISTORY_BUTTON], historybutton_draw,
					historybutton_input, (re_destroy_proc_t)NULL);
	re_set_margins(regs[HISTORY_BUTTON], 1, 1, 1, 1);
	re_set_mhcaps(regs[HISTORY_BUTTON], "menu", "paste", "(menu)");
	re_set_cursor(regs[HISTORY_BUTTON], CU_MENU);
	re_set_callbacks(regs[ENTER_BUTTON], enterbutton_draw,
					enterbutton_input, (re_destroy_proc_t)NULL);
	re_set_margins(regs[ENTER_BUTTON], 1, 1, 1, 1);
	re_set_mhcaps(regs[ENTER_BUTTON], "enter", "paste", "(menu)");
	re_set_cursor(regs[ENTER_BUTTON], CU_MENU);



	for (i = 0; i < TM_NTAGS; ++i) {
		tmbuf.tm_mdtab[i].md = Minsert(tgtmenus[i]);
		tmbuf.tm_mdtab[i].wn = re_get_wn(regs[START_TARGET_MENU + i]);
		tmbuf.tm_mdtab[i].region = regs[START_TARGET_MENU + i];
		initialise_menu(regs[START_TARGET_MENU + i],
				tmbuf.tm_mdtab[i].md, "tmenu", target_menu_func,
				(char *)NULL);
	}
	tmbuf.tm_current_md = -1;
	set_target_menu_info(&tmbuf);

	set_start_target_menu(tmbuf.tm_mdtab[TM_START].md);
	set_restart_target_menu(Minsert(&tgt_restart_men));

	if (want_outwin)
		create_outwin(root, outwin_fraction);

	re_add_rootwin(root, TRUE);
	re_redraw_root(EV_WINDOW_RESIZED, TRUE);

	return root;
}

static void
line_input(region, ev)
Region *region;
event_t *ev;
{
	int buttons, width, height;
	bool changed_position, is_vertical_shift;

	buttons = ev->ev_buttons;

	wn_get_window_size(re_get_wn(region), &width, &height);
	is_vertical_shift = width > height;
	changed_position = FALSE;

	if (buttons & B_LEFT) {
		int wn, x, y, last_coord;

		/*  As we are shifting windows around, get a stable window
		 *  to read events from, and translate the current event
		 *  coordinates to that window.
		 */
		wn = re_get_wn(re_get_parent(region));
		wn_trans_coords(ev->ev_wn, ev->ev_x, ev->ev_y, wn, &x, &y);

		last_coord = is_vertical_shift ? y : x;
		do {
			int coord;

			wn_next_event(wn, EV_MOUSE_MOVED | EV_BUTTON_UP, ev);
			coord = is_vertical_shift ? ev->ev_y : ev->ev_x;
			if (coord != last_coord) {
				re_change_position(region, coord - last_coord);
				last_coord = coord;
				changed_position = TRUE;
			}
		} while (ev->ev_type != EV_BUTTON_UP);
	}
	else if (buttons & (B_MIDDLE | B_RIGHT)) {
		int delta;

		delta = (buttons & B_MIDDLE) ? -1 : 1;
		re_change_position(region, delta);
		wn_wait_for_release_of(ev->ev_wn, B_MIDDLE | B_RIGHT);

		/*  The shift we've just done has moved the window, and we
		 *  want the cursor in the same window relative position so
		 *  that it remains over the line, so we warp it.
		 */
		wn_warp_mouse(ev->ev_wn, ev->ev_x, ev->ev_y);

		changed_position = TRUE;
	}

	if (changed_position)
		re_expose(re_get_parent(region), TRUE);
}

static void
dragbox_input(region, ev)
Region *region;
event_t *ev;
{
	line_input((Region *)re_get_data(region), ev);
}

static void
dragbox_draw(region, wn, win_width, win_height, draw_reason)
Region *region;
int wn;
int win_width, win_height;
re_draw_reason_t draw_reason;
{
	/*  The arrow_* #defines and data were generated by the X10 bitmap program.
	 *  They define the double headed arrow used in the source window drag box.
	 */
#define arrow_bitmap_width 23
#define arrow_bitmap_height 23
	static unsigned short arrow_bitmap_bits[] = {
	   0x0000, 0x0000, 0x0000, 0x0000,
	   0x0000, 0x0000, 0x0800, 0x0000,
	   0x1c00, 0x0000, 0x3e00, 0x0000,
	   0x7f00, 0x0000, 0xff80, 0x0000,
	   0xffc0, 0x0001, 0xffe0, 0x0003,
	   0x0000, 0x0000, 0x0000, 0x0000,
	   0x0000, 0x0000, 0xffe0, 0x0003,
	   0xffc0, 0x0001, 0xff80, 0x0000,
	   0x7f00, 0x0000, 0x3e00, 0x0000,
	   0x1c00, 0x0000, 0x0800, 0x0000,
	   0x0000, 0x0000, 0x0000, 0x0000,
	   0x0000, 0x0000
	};

	static bitmap_t arrow_bm = wn_static_bm(arrow_bitmap_width,
						arrow_bitmap_height, 1,
						0, 0, BM_BIT0_LEFT,
						arrow_bitmap_bits);
	int bm_x, bm_y, win_x, win_y;
	int width, height;

	if (draw_reason != RE_EXPOSED && draw_reason != RE_RESIZED)
		return;

	if (arrow_bm.bm_width < win_width) {
		bm_x = 0;
		width = arrow_bm.bm_width;
		win_x = (win_width - arrow_bm.bm_width) / 2;
	}
	else {
		bm_x = (arrow_bm.bm_width - win_width) / 2;
		width = win_width;
		win_x = 0;
	}

	if (arrow_bm.bm_height < win_height) {
		bm_y = 0;
		height = arrow_bm.bm_height;
		win_y = (win_height - arrow_bm.bm_height) / 2;
	}
	else {
		bm_y = (arrow_bm.bm_height - win_height) / 2;
		height = win_height;
		win_y = 0;
	}

	wn_rop_from_mem(&arrow_bm, bm_x, bm_y, width, height,
			wn, win_x, win_y, R_RPL);
}

static void
historybutton_input(region, ev)
Region *region;
event_t *ev;
{
/* We pick a larger max entries than will probably fit on the screen.
** Normally, the max is set by the individual EditHistory objects.
** Unless the user uses the Xresources to change them, the real max
** is 20.
*/
#define MAXHISTMENU 50

#define MAXMENULEN 60
    popup_t historybuttonpop;
    char buff[(MAXMENULEN+1)*MAXHISTMENU];
    char* cp_cap = buff;
    const char *historybutton[MAXHISTMENU+1];
    int buttons = ev->ev_buttons;
    Region* focus_re = re_get_keyboard_focus_region();
    Edit_display *display = re_get_display(focus_re);


    if (buttons == B_LEFT && display)
    {
	int res;
	int count = MAXHISTMENU;
	edit_get_recent_history(display, &count, historybutton);
	if ( count)
	{
	    int width, height;
	    historybutton[count] = NULL;
	    historybuttonpop.po_mdesc = -1;
	    historybuttonpop.po_save_last = FALSE;
	    historybuttonpop.po_last = 0;
	    historybuttonpop.po_caps = historybutton;
	    historybuttonpop.po_left = TRUE;

	    /* strip any leading tabs or spaces. */
	    /* if there are newlines, or if the line is very long,
	    ** copy the caption to the buffer */
	    while (count--)
	    {
		const char* cap = historybutton[count];
		while (*cap <= ' ' )
		    cap++;
		if (strchr(cap, '\n') || (strlen(cap) >= MAXHISTMENU))
		{
		    int len = 0;
		    int index = 0;
		    while ( len < MAXHISTMENU)
		    {
			char ch = cap[index++];
			if (ch == 0)
			    break;
			if ( ch == '\n')
			{
			    cp_cap[len++] = '\\';
			    if ( len < MAXHISTMENU)
			        cp_cap[len++] = 'n';
			}else if ( ch == '\t')
			    cp_cap[len++] = ' ';
			else
			    cp_cap[len++] = ch;
		    }
		    cp_cap[len++] = 0;
		    cap = cp_cap;
		    cp_cap+= len;
		}
		historybutton[count] = cap;
	    }
	    wn_get_window_size(ev->ev_wn, &width, &height);
	    res = select_from_popup(ev->ev_wn, "HistoryMenu", buttons, &historybuttonpop,
			    ev->ev_x, ev->ev_y);
	    if ( res >= 0 )
		edit_set_history_pointer(display, res);

	    Mclose(historybuttonpop.po_mdesc);

	}

    } else if (   focus_re && buttons == B_RIGHT)
    {
	/* Pass the right button event to the focus region */
	re_handle_event(focus_re, ev);
    } else if (   focus_re && buttons == B_MIDDLE)
    {
	/* Paste from the clipboard */

	/* invert the region */
	int wn = ev->ev_wn;
	int width, height;
	wn_get_window_size(wn, &width, &height);
	wn_invert_area(wn, 0,0, width, height);

	/* wait for button up */
	wn_next_event(WN_ANY, EV_BUTTON_UP, ev);
	if ( wn == ev->ev_wn)
	{
	    event_t key_event;
	    key_event = *ev;
	    key_event.ev_type = EV_KEY;
	    key_event.ev_flags = 0;
	    key_event.ev_char = 'y'&0x1f;  /* Control-Y */
	    key_event.ev_buttons = 0;
	    re_handle_event(focus_re, &key_event);
	}
	wn_invert_area(wn, 0,0, width, height);
    }
}

static void
historybutton_draw(region, wn, win_width, win_height, draw_reason)
Region *region;
int wn;
int win_width, win_height;
re_draw_reason_t draw_reason;
{
#define historybutton_image_width 12
#define historybutton_image_height 5
static unsigned char historybutton_image_bits[] = {
	0xfc,0x07,0xf8,0x03,0xf0,0x01,0xe0,0x00,0x40,0x00};
	static bitmap_t historybutton_bm =
	   wn_make_static_X11bm (historybutton_image_width,
	      historybutton_image_height, historybutton_image_bits);
	int bm_x, bm_y, win_x, win_y;
	int width, height;

	if (draw_reason != RE_EXPOSED && draw_reason != RE_RESIZED)
		return;

	if (historybutton_bm.bm_width < win_width) {
		bm_x = 0;
		width = historybutton_bm.bm_width;
		win_x = (win_width - historybutton_bm.bm_width) / 2;
	}
	else {
		bm_x = (historybutton_bm.bm_width - win_width) / 2;
		width = win_width;
		win_x = 0;
	}

	if (historybutton_bm.bm_height < win_height) {
		bm_y = 0;
		height = historybutton_bm.bm_height;
		win_y = (win_height - historybutton_bm.bm_height) / 2;
	}
	else {
		bm_y = (historybutton_bm.bm_height - win_height) / 2;
		height = win_height;
		win_y = 0;
	}

	wn_rop_from_mem(&historybutton_bm, bm_x, bm_y, width, height,
			wn, win_x, win_y, R_RPL);
}

static void
enterbutton_input(region, ev)
Region *region;
event_t *ev;
{
    int buttons = ev->ev_buttons;
    Region* focus_re = re_get_keyboard_focus_region();


    if (   focus_re
	&& (buttons == B_LEFT || buttons == B_MIDDLE))
    {
	/* invert the region */
	int wn = ev->ev_wn;
	int width, height;
	wn_get_window_size(wn, &width, &height);
	wn_invert_area(wn, 0,0, width, height);

	/* wait for button up */
	wn_next_event(WN_ANY, EV_BUTTON_UP, ev);
	wn_invert_area(wn, 0,0, width, height);
	if ( wn == ev->ev_wn)
	{
	    event_t key_event;
	    key_event = *ev;
	    key_event.ev_type = EV_KEY;
	    key_event.ev_flags = 0;
	    if ( buttons == B_LEFT)
		key_event.ev_char = '\033';  /* Escape */
	    else
		key_event.ev_char = 'y'&0x1f;  /* Control-Y */
	    key_event.ev_buttons = 0;
	    re_handle_event(focus_re, &key_event);
	}
    } else if (   focus_re && buttons == B_RIGHT)
    {
	/* Pass the right button event to the focus region */
	re_handle_event(focus_re, ev);
    }
}

static void
enterbutton_draw(region, wn, win_width, win_height, draw_reason)
Region *region;
int wn;
int win_width, win_height;
re_draw_reason_t draw_reason;
{
#define enter_image_width 15
#define enter_image_height 15
static unsigned char enter_image_bits[] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0xc0,0x30,0x60,0x18,0x30,0x0c,
	0x18,0x06,0x0c,0x03,0x18,0x06,0x30,0x0c,0x60,0x18,0xc0,0x30,
	0x00,0x00,0x00,0x00,0x00,0x00};

	static bitmap_t enter_bm =
	   wn_make_static_X11bm(enter_image_width,
	      enter_image_height, enter_image_bits);
	int bm_x, bm_y, win_x, win_y;
	int width, height;

	if (draw_reason != RE_EXPOSED && draw_reason != RE_RESIZED)
		return;

	if (enter_bm.bm_width < win_width) {
		bm_x = 0;
		width = enter_bm.bm_width;
		win_x = (win_width - enter_bm.bm_width) / 2;
	}
	else {
		bm_x = (enter_bm.bm_width - win_width) / 2;
		width = win_width;
		win_x = 0;
	}

	if (enter_bm.bm_height < win_height) {
		bm_y = 0;
		height = enter_bm.bm_height;
		win_y = (win_height - enter_bm.bm_height) / 2;
	}
	else {
		bm_y = (enter_bm.bm_height - win_height) / 2;
		height = win_height;
		win_y = 0;
	}

	wn_rop_from_mem(&enter_bm, bm_x, bm_y, width, height,
			wn, win_x, win_y, R_RPL);
}

void
load_library_input(handle, arg)
void *handle;
char *arg;
{
  event_t *ev;
  popup_t loadlibrarypop;
  const char **loadlibrary;
  int res, buttons;

  ev = (event_t *)arg;
  buttons = ev->ev_buttons;
  res = get_noload_list(&loadlibrary);
  if (res == 0)
  {
    wn_wait_for_release_of(ev->ev_wn, B_ANY);
    errf("\bAll libraries loaded");
  }
  else
  {
    loadlibrarypop.po_mdesc = -1;
    loadlibrarypop.po_save_last = FALSE;
    loadlibrarypop.po_last = 0;
    loadlibrarypop.po_caps = loadlibrary;
    loadlibrarypop.po_left = TRUE;
  
    res = select_from_popup(ev->ev_wn, "UnloadedLibMenu", buttons, &loadlibrarypop,
			    ev->ev_x, ev->ev_y);
    Mclose(loadlibrarypop.po_mdesc);
    if ( res >= 0 )
    {
      add_to_load_path(*(loadlibrary+res), 0, 1); /* load requested */
#ifdef AO_ELF
      if (can_get_target_vars((target_t *)handle))
      {
	rescan_dynamic_solibs((target_t *)handle, FALSE);
	refresh_target_display((target_t *)handle, SR_BPT, TRUE);
      }
      add_to_noload_list(*(loadlibrary+res), 1); /* mark as loaded */
#endif
#if defined(AO_AOUT_SHLIBS)
      if (can_get_target_vars((target_t *)handle))
	rescan_dynamic_solibs((target_t *)handle, FALSE);
      unload_shared_library_symtabs((target_t *)handle);
      load_shared_library_symtabs((target_t *)handle, TRUE);
      refresh_target_display((target_t *)handle, SR_BPT, TRUE);
#endif
    }
    free(loadlibrary);
  }
}

static void
line_draw(region, wn, width, height, draw_reason)
Region *region;
int wn;
int width, height;
re_draw_reason_t draw_reason;
{
	switch (draw_reason) {
	case RE_EXPOSED:
	case RE_RESIZED:
		wn_set_area(wn, 0, 0, width, height, WN_FG(wn));
		break;
	case RE_RESIZING:
	case RE_UNDRAW:
		wn_invert_area(wn, 0, 0, width, height);
		break;
	default:
		panic("bad dr in ld");
	}
}

static void
initialise_typing_line(region)
Region *region;
{
	re_set_callbacks(region, typing_line_draw,
			 typing_line_input, (re_destroy_proc_t)NULL);
	
	re_set_keyboard_focus_callbacks(region,
					typing_line_accept_focus,
					typing_line_quit);
	re_set_display_callback(region, typing_line_get_display);
	
	re_set_margins(region, 5, 5, 4, 4);
	re_set_cursor(region, CU_DISPLAY);
	re_set_mhcaps(region, "", "edit", "(menu)");
	re_set_data(region, (char *)NULL);
}

static void
message_area_draw(region, wn, width, height, draw_reason)
Region *region;
int wn;
int width, height;
re_draw_reason_t draw_reason;
{
	if (draw_reason == RE_EXPOSED || draw_reason == RE_RESIZED)
		redraw_message(wn);
}

/*  Set up the typing area
 */
static void
typing_line_draw(region, wn, width, height, draw_reason)
Region *region;
int wn;
int width, height;
re_draw_reason_t draw_reason;
{
	Edit_display *display;

	display = (Edit_display *)re_get_data(region);

	if (display == NULL) {
		Edit_keymap *keymap;
		
		display = make_text_display(wn, WN_FG(wn), WN_BG(wn), "");
		re_set_data(region, (char *)display);

		keymap = edit_create_keymap(edit_get_keymap(display));
		edit_add_keymap_control_entry(keymap, 'v', show_version);
		edit_set_keymap(display, keymap);
		edit_set_history(display, typing_line_history);
	}
	else {
		switch (draw_reason) {
		case RE_EXPOSED:
			edit_redraw_display(display);
			break;
		case RE_RESIZED:
			edit_update_wn_window_size(display, wn);
			edit_redraw_display(display);
			break;
		case RE_RESIZING:
		case RE_UNDRAW:
			break;
		}
	}
}

static void
show_version(display)
Edit_display *display;
{
	errf("%s", ups__sccsdata[0]);
}	

static void
typing_line_input(region, ev)
Region *region;
event_t *ev;
{
	Edit_display *display;
	int list_options = 0;

	display = (Edit_display *)re_get_data(region);

	if (ev->ev_type == EV_KEY) {
	  list_options = ev->ev_buttons & B_SHIFT_MASK;
		edit_handle_wn_key_event(display, ev);

	  if (list_options)
	  {
	    /* clear force_quit_flag so listing options does */
	    /* not force the edit to clear */
	    re_clear_force_quit_flag(region);
	    list_matches_to_outwin(1, 0); /* set to display area */
	  }
		if (edit_quit_requested(display)) {
			char *text;
			
			edit_save_in_history(display);
			text = get_typing_line_string();
			process_typing_line_text(text);
			free(text);
		      }
	  if (list_options)
	    list_matches_to_outwin(0, 1); /* reset */
	}
	else if (ev->ev_type == EV_BUTTON_DOWN) {
		if (ev->ev_buttons == B_LEFT) {
			do_selection(display, ev);
		}
		else if (ev->ev_buttons == B_MIDDLE) {
			size_t point;
			
			if (!re_set_keyboard_focus(region, (char *)NULL)) {
				wn_wait_for_release_of(ev->ev_wn, B_ANY);
				return;
			}

			if (edit_pixel_to_point(display, ev->ev_x, ev->ev_y,
						&point))
				edit_move_point(display, point);

			edit_set_want_cursor(display, TRUE);
		}
		else if (ev->ev_buttons == B_RIGHT)
		{
		    if (region == re_get_keyboard_focus_region())
		    {
			get_custom_menu_str(handle_typing_line_char, ev);
			if (edit_quit_requested(display)) {
			    char *text;
			
			    edit_save_in_history(display);
			    text = get_typing_line_string();
			    process_typing_line_text(text);
			    free(text);
			}
		    }
		  return;
		} 
	}
}

char *
get_typing_line_string()
{
	Edit_display *display;
	
	display = (Edit_display *)re_get_data(get_typing_line_region());
	return get_buffer_contents(edit_get_buffer(display));
}
		
static bool
typing_line_accept_focus(region, data)
Region *region;
char *data;
{
	Edit_display *display;

	display = (Edit_display *)re_get_data(region);

	if (display != NULL)
		edit_set_want_cursor(display, TRUE);
	
	return TRUE;
}

static bool
typing_line_quit(region, data, force)
Region *region;
char *data;
bool force;
{
	edit_set_want_cursor((Edit_display *)re_get_data(region), FALSE);
	return TRUE;
}

static Edit_display* typing_line_get_display(region)
Region *region;
{
    return (Edit_display *)re_get_data(region);
}

static void
init_line_region(line)
Region *line;
{
	if ((Debug_flags & DBFLAG_MVLINES) == 0) {
		re_set_callbacks(line, line_draw, (re_input_proc_t)NULL,
							(re_destroy_proc_t)NULL);
	}
	else {
		re_set_callbacks(line, line_draw, line_input,
							(re_destroy_proc_t)NULL);
		re_set_mhcaps(line, "drag", "up", "down");
		re_set_cursor(line, CU_LINE);
	}
}

static void
add_outwin()
{
	Region *srcwin_region, *root;
	double outwin_fraction;
	int oldstate;
	
	srcwin_region = get_current_srcwin_region();

	if (srcwin_region == NULL) {
		/*  No window.
		 */
		set_current_outwin(outwin_create(-1, (tbar_id_t)NULL));
		return;
	}

	root = srcwin_region;
	for(;;) {
		Region *parent;

		parent = re_get_parent(root);
		if (parent == NULL)
			break;
		root = parent;
	}

				/* RGA make output window default smaller */
	outwin_fraction = (double)get_default("OutwinPercent", 10, 1) / 100;
	create_outwin(root, outwin_fraction);

	oldstate = td_set_obj_updating(OBJ_UPDATING_ON);
	re_redraw_root(EV_WINDOW_RESIZED, TRUE);
	td_set_obj_updating(oldstate);
}

Outwin *
get_or_create_outwin_unmapped()
{
	Outwin *ow;

	ow = get_current_outwin();

	if (ow == NULL) {
	  add_outwin();
	  ow = get_current_outwin();
	}

	return ow;
}
Outwin *
get_or_create_outwin()
{
	Outwin *ow = get_or_create_outwin_unmapped();
	outwin_map(ow);
	return ow;
}

static void
create_outwin(root, outwin_fraction)
Region *root;
double outwin_fraction;
{
	int out_wn = get_split_out_win();
	extern MENU out_men;
	enum {
		TOP, TOPLEFT,
		TYPING_LINE,
		PMENU, MOUSEHOLE,
		TYPING_LINE_ENTER_AREA, HISTORY_BUTTON, ENTER_BUTTON,
		MENU_AND_DRAGBOX,DRAGBOX,MENU,
		TEXT_AND_TB,
		OUTWIN,TBAR,
		MAX_REGNUM
	};
	Region *regs[MAX_REGNUM], *line, *dragbox_line;
	static int md = -1;
	int header_height, tbar_width;
	Outwin *ow;
	tbar_id_t tbar_id;
	int i;
	int pmd;

	for (i = 0; i < MAX_REGNUM; ++i)
		regs[i] = NULL;
	line = NULL;


	if ( out_wn != -1)
	{
	    font_t *menufont = Mstdfont();
	    int menu_height = menufont->ft_height + 8;
	    int historybutton_width = get_default("HistoryButtonWidth", 15, 12);
	    int enterbutton_width = get_default("EnterButtonMenuWidth", 15, 12);
	    int pmenu_width = get_default("PermanentMenuWidth", 28*menufont->ft_width, 5);
	    root = re_make_region(out_wn);
	    outwin_fraction = REST;
	    re_divide(root, RE_HORIZONTAL,
		      &regs[TOP],	        menu_height,	FIXED,
		      &line,			1,		FIXED,
		      RE_END);
	    init_line_region(line);

	    re_divide(regs[TOP], RE_VERTICAL,
		  &regs[TYPING_LINE_ENTER_AREA],		0,		REST,
		  &line,				1,		FIXED,
		  &regs[PMENU],			pmenu_width,	FIXED,
		  RE_END);

	    re_divide(regs[TYPING_LINE_ENTER_AREA], RE_VERTICAL,
		  &regs[TYPING_LINE],		0,		REST,
		  &line,			1,		FIXED,
		  &regs[HISTORY_BUTTON],	historybutton_width,	FIXED,
		  &line,			1,		FIXED,
		  &regs[ENTER_BUTTON],		enterbutton_width,	FIXED,
		  RE_END);
	    pmd = make_permanent_menu(regs[PMENU]);
	    Mdelete_entry(pmd,MR_RAISE_OUT_WIN);

	    /*  If the src and display are in the same region, remove the
	    **  raise source option */
	    if ( re_same_root_wn(get_display_area_region(),get_current_srcwin_region()))
		Mdelete_entry(pmd,MR_RAISE_SRC_WIN);

	    initialise_typing_line(regs[TYPING_LINE]);
	    re_set_typing_regions(regs[TYPING_LINE],regs[TYPING_LINE_ENTER_AREA]);

	    re_set_callbacks(regs[HISTORY_BUTTON], historybutton_draw,
					historybutton_input, (re_destroy_proc_t)NULL);
	    re_set_margins(regs[HISTORY_BUTTON], 1, 1, 1, 1);
	    re_set_mhcaps(regs[HISTORY_BUTTON], "menu", "paste", "(menu)");
	    re_set_cursor(regs[HISTORY_BUTTON], CU_MENU);
	    re_set_callbacks(regs[ENTER_BUTTON], enterbutton_draw,
					enterbutton_input, (re_destroy_proc_t)NULL);
	    re_set_margins(regs[ENTER_BUTTON], 1, 1, 1, 1);
	    re_set_mhcaps(regs[ENTER_BUTTON], "enter", "paste", "(menu)");
	    re_set_cursor(regs[ENTER_BUTTON], CU_MENU);
	    dragbox_line = NULL;
	} else
	{
	    line = NULL;
	    re_divide(root, RE_HORIZONTAL,
		  &line,			1,		FIXED,
		  RE_END);
	    dragbox_line = line;
	    init_line_region(line);
	}

	header_height = wn_get_sysfont()->ft_height + 6;
	tbar_width = get_default("ScrollbarWidth", 20, 5);


	re_divide(root, RE_HORIZONTAL,
		  &regs[MENU_AND_DRAGBOX],	header_height,	FIXED,
		  &line,			1,		FIXED,
		  &regs[TEXT_AND_TB],		0,		outwin_fraction,
		  RE_END);

	re_divide(regs[TEXT_AND_TB], RE_VERTICAL,
		  &regs[TBAR],			tbar_width,	FIXED,
		  &line,			1,		FIXED,
		  &regs[OUTWIN],		0,		REST,
		  RE_END);
	
	if (dragbox_line)
	{
	    re_divide(regs[MENU_AND_DRAGBOX], RE_VERTICAL,
		  &regs[MENU],			0,		REST,
		  &line,			1,		FIXED,
		  &regs[DRAGBOX],		header_height,	FIXED,
		  RE_END);
	} else
	{
		  regs[MENU] = regs[MENU_AND_DRAGBOX];
	}

	tbar_id = make_tbar(regs[TBAR]);
	ow = outwin_create(re_get_wn(regs[OUTWIN]), tbar_id);
	
	tb_set_callback_and_data(tbar_id, tbar_outwin_callback, (char *)ow);

	re_set_data(regs[OUTWIN], (char *)ow);
	re_set_mhcaps(regs[OUTWIN], "", "edit", "(menu)");
	re_set_cursor(regs[OUTWIN], CU_DISPLAY);
	re_set_callbacks(regs[OUTWIN], outwin_draw,
			 output_window_event_handler, (re_destroy_proc_t)NULL);
	re_set_keyboard_focus_callbacks(regs[OUTWIN],
					(re_accept_focus_proc_t)NULL,
					outwin_release_focus);
	
	if ( regs[DRAGBOX])
	{
	    re_set_callbacks(regs[DRAGBOX], dragbox_draw,
					dragbox_input, (re_destroy_proc_t)NULL);
	    re_set_margins(regs[DRAGBOX], 1, 1, 1, 1);
	    re_set_data(regs[DRAGBOX], (char *)dragbox_line);
	    re_set_mhcaps(regs[DRAGBOX], "drag", "up", "down");
	    re_set_cursor(regs[DRAGBOX], CU_DRAGBOX);
	}


	if (md == -1)
		md = Minsert(&out_men);
	else
		md = Mdup(md);

	if ( regs[TYPING_LINE] == NULL)
	    Mdelete_entry(md,MR_OUTWIN_CLOSE);
	else
	    Mdelete_entry(md,MR_OUTWIN_SEARCH);

	initialise_menu(regs[MENU], md, "outwin",
			output_window_menu_func, (char *)ow);

	set_current_outwin(ow);
	if ( out_wn >= 0)
	{
	    re_add_rootwin(root, FALSE);
	}
}
	
static void
create_srcwin(region, srcwin_fraction, split)
Region *region;
double srcwin_fraction;
int split;
{
	static int md = -1;
	extern MENU src_men;
	enum {
		LINE,
		HEADER,
		TEXT_AND_TB,
		TEXT, TB,
		NAMEWIN,
		MENU_AREA,
		NREGS
	};
	tbar_id_t tbar_id;
	Srcwin *sw;
	Region *regs[NREGS];
	int i, tbar_width;

	tbar_width = get_default("ScrollbarWidth", 20, 5);

	for (i = 0; i < NREGS; ++i)
		regs[i] = NULL;

	re_divide(region, RE_HORIZONTAL,
		  &regs[LINE],		1,		FIXED,
		  RE_END);

	init_line_region(regs[LINE]);

	re_divide(region, RE_HORIZONTAL,
		  &regs[HEADER],	1,		0.0,
		  &regs[LINE],		1,		FIXED,
		  &regs[TEXT_AND_TB],	0,		srcwin_fraction,
		  RE_END);

	re_divide(regs[HEADER], RE_VERTICAL,
		  &regs[NAMEWIN],	1,		FIXED,
		  &regs[LINE],		1,		FIXED,
		  &regs[MENU_AREA],	0,		REST,
		  RE_END);

	re_divide(regs[TEXT_AND_TB], RE_VERTICAL,
		  &regs[TB],		tbar_width,	FIXED,
		  &regs[LINE],		1,		FIXED,
		  &regs[TEXT],		0,		REST,
		  RE_END);

	re_set_minsize(regs[NAMEWIN], wn_get_sysfont()->ft_width * 30,
					   wn_get_sysfont()->ft_height + 6);
	re_set_minsize(regs[MENU_AREA], 0, Mstdfont()->ft_height + 6);

	tbar_id = make_tbar(regs[TB]);
	sw = srcwin_create(re_get_wn(regs[TEXT]), tbar_id);

	/*  Tie all the bits together with callbacks.
	 */

	re_set_data(regs[TEXT], (char *)sw);
	re_set_data(regs[NAMEWIN], (char *)sw);
	re_set_display_callback(regs[TEXT], source_window_get_display);

	tb_set_callback_and_data(tbar_id, tbar_srcwin_callback, (char *)sw);
	srcwin_set_data(sw, (char *)(long)re_get_wn(regs[NAMEWIN]));

	re_set_callbacks(regs[TEXT], srcwin_draw, source_window_event_handler,
			 (re_destroy_proc_t)NULL);
	re_set_keyboard_focus_callbacks(regs[TEXT],
					(re_accept_focus_proc_t)NULL,
					srcwin_release_focus);
	
	re_set_cursor(regs[TEXT], CU_DISPLAY);
	re_set_mhcaps(regs[TEXT], "select", "edit/goto", "menu");

	re_set_callbacks(regs[NAMEWIN], srcpos_draw,
			 name_window_event_handler, (re_destroy_proc_t)NULL);
	re_set_cursor(regs[NAMEWIN], CU_DISPLAY);
	re_set_mhcaps(regs[NAMEWIN], "", "", "menu");

	if (md == -1)
		md = Minsert(&src_men);
	else
		md = Mdup(md);

        if ( split)
	    Mdelete_entry(md,MR_SRCWIN_SEARCH);


	initialise_menu(regs[MENU_AREA], md, "srcwin",
			source_window_menu_func, (char *)sw);
	
	set_current_srcwin_region(regs[TEXT]);
	set_current_srcwin_menu(md);
}

static int
tbar_obj_callback(unused_data, action, val)
char *unused_data;
tb_action_t action;
int val;
{
	int retval;

	switch (action) {
	case TB_GOTO:
		display_area_overlay_unselect_global_selection();
		display_from(0, val);
		retval = val;
		break;
	case TB_SCROLL:
		display_area_overlay_unselect_global_selection();
		retval = v_scroll(val);
		break;
	case TB_SHOW_POS:
		retval = 0;
		break;
	default:
		panic("bad action in tsc");
		retval = 0;	/* to satisfy gcc */
	}
	return retval;
}

static Edit_display*
source_window_get_display(region)
Region *region;
{
	Srcwin *sw;
	sw = (Srcwin *)re_get_data(region);
	return srcwin_get_display(sw);
}

static int
tbar_srcwin_callback(data, action, val)
char *data;
tb_action_t action;
int val;
{
	Srcwin *sw;
	int retval;
	const char *name;
	Srcinfo srcinfo;

	sw = (Srcwin *)data;

	switch (action) {
	case TB_GOTO:
		retval = srcwin_goto_ypos(sw, val);
		break;
	case TB_SCROLL:
		retval = srcwin_scroll(sw, val);
		break;
	case TB_SHOW_POS:
		srcwin_get_info(sw, -1, val, &srcinfo);
		name = (srcinfo.fil != NULL) ? srcinfo.fil->fi_name_only : NULL;
		echo_src_name_and_lnum(sw, name, srcinfo.fil_lnum);
		retval = 0;
		break;
	default:
		panic("bad action in tsc");
		retval = 0;	/* to satisfy gcc */
	}
	return retval;
}

static int
tbar_outwin_callback(data, action, val)
char *data;
tb_action_t action;
int val;
{
	Outwin *sw;
	int retval;

	sw = (Outwin *)data;

	switch (action) {
	case TB_GOTO:
		retval = (int)outwin_goto_point(sw, (size_t)val);
		break;
	case TB_SCROLL:
		retval = outwin_scroll(sw, val);
		break;
	case TB_SHOW_POS:
		retval = 0;
		break;
	default:
		panic("bad action in tsc");
		retval = 0;	/* to satisfy gcc */
	}
	return retval;
}

static void
display_area_tbar_callback(data, change, val)
char *data;
obj_change_t change;
int val;
{
	tbar_id_t tbar_id;

	tbar_id = (tbar_id_t)data;
	switch (change) {
	case OBJ_NEW_WINDOW_HEIGHT:
		tb_set_visible_units(tbar_id, val);
		break;
	case OBJ_NEW_HEIGHT:
		tb_set_total_units(tbar_id, val);
		break;
	case OBJ_NEW_YPOS:
		display_area_overlay_unselect_global_selection();
		tb_goto(tbar_id, val, FALSE);
		break;
	default:
		panic("bad change in datc");
	}
}

static tbar_id_t
make_tbar(region)
Region *region;
{
	int wn;
	tbar_id_t tbar_id;

	wn = re_get_wn(region);
	tbar_id = tb_create(wn);
	re_set_callbacks(region, tbar_draw, tbar_input,
			 (re_destroy_proc_t)NULL);
	re_set_data(region, (char *)tbar_id);
	re_set_cursor(region, CU_SBAR);
	re_set_mhcaps(region, "down", "goto", "up");
	return tbar_id;
}

/*  Input function for the display area.  Just push back the event
 *  and hand control over to the object library.
 *
 *  BUG: for the moment, we ignore the region, and assume that there
 *       is only one display area.
 */
static void
display_area_input(region, ev)
     Region *region;
     event_t *ev;
{
  int buttons;
  int list_options = 0;
  
  if (ev->ev_type == EV_KEY) {
    char *handle;
		
    list_options = ev->ev_buttons & (B_SHIFT_MASK|B_MULT_CLICKS);
    handle = re_get_keyboard_focus_data(region);

    if (handle == NULL)
      panic("unexpected display area key event");
		
    if (list_options)
    {
      /* clear force_quit_flag so listing options does */
      /* not force the edit to clear */
      re_clear_force_quit_flag(region);
      list_matches_to_outwin(1, 0); /* set to display area */
    }
    field_edit_handle_key_event(region, handle, ev);
    if (list_options)
      list_matches_to_outwin(0, 1); /* reset */

  }
  else {
    static int want_threshold = -1, x_threshold = 30; /* default 30 pixels */
    struct objst *obj;
    int threshold_exceeded;
    char *def;

    buttons = ev->ev_buttons & B_ANY;
    
    if (want_threshold == -1)
    {
      want_threshold = 0;
      def = (char *)wn_get_default("SelectionThreshold");
      if (def)
      {
	char *ptr;
	long k = 0;

	k = strtol(def, &ptr, 0);
	if (k && ptr)
	  x_threshold = k;
      }
    }

    if (buttons != 0) {
      int disp_wn;
      sel_t *sel;

      /*  When editing is in progress there is a window
       *  just for that.  If the user clicks on an object
       *  over this edit window, the end of the edit can
       *  close the window under us.  Thus we make sure
       *  we are not using this window.
       */
      disp_wn = re_get_wn(get_display_area_region());
      if (ev->ev_wn != disp_wn) {
	int x, y;
				
	wn_trans_coords(ev->ev_wn, ev->ev_x, ev->ev_y,
			disp_wn, &x, &y);
	ev->ev_x = x;
	ev->ev_y = y;
	ev->ev_wn = disp_wn;
      }
			
      if (buttons != B_MIDDLE) {
        if (ev->ev_type == EV_BUTTON_DOWN) {
	  if (buttons == B_RIGHT &&
	      re_get_keyboard_focus_data(region))
	  {
	    get_custom_menu_str(handle_display_area_char, ev);
	    return;
	  }
	  else if (!re_set_keyboard_focus((Region *)NULL, (char *)NULL)) {
	    wn_wait_for_release_of(ev->ev_wn, B_ANY);
	    return;
	  }
        }
        
	set_dynamic_menu_updating_state(DMU_OFF);
      }
      wn_pushback_event(ev);
      display_area_overlay_unselect_global_selection();
      obj = select_list(x_threshold, &threshold_exceeded);
      if (threshold_exceeded)
      {
	Outwin *dw;
	int y = -1, ft_ht, y_offset;
	event_t event;
	bool oldval;
	const int evmask =
	  EV_BUTTON_UP | EV_BUTTON_DOWN | EV_MOUSE_MOVED | EV_KEY |
	    EV_INTERRUPT | EV_WINDOW_EXPOSED | EV_WINDOW_RESIZED;
	
	wn_next_event(WN_ANY, evmask, &event);
	dw = (Outwin *)re_get_data(region);
	ft_ht = wn_get_sysfont()->ft_height;
	y_offset = get_cur_posn()%ft_ht;

	/* RGA need to clear selection if wn_use_alloc_color() is true
	 * but since we can't easily restore the selection after the
	 * overlay code, call clear_selection() beforehand in all cases
         */
	clear_selection();
	wn_text_capture(1, 0, &y); /* set */
	display_area_overlay_delete_all_text(dw);
	oldval = display_area_overlay_set_propchange_updates(dw, FALSE);
	set_obj_wn(event.ev_wn);
	display_area_overlay_set_propchange_updates(dw, oldval);
	outwin_redraw(dw);
	if (y_offset == 0)
	  display_area_overlay_scroll(dw, ft_ht);
	else
	  display_area_overlay_scroll(dw, y_offset);
	wn_text_capture(0, 1, &y); /* reset */

	do_selection(outwin_get_display(dw), &event);
	/**  RCB: This causes the selection to be imediately erased on button up.
	** display_area_overlay_tb_clear(dw);
	** set_obj_wn(event.ev_wn);
	*/
      }
		
      if (buttons != B_MIDDLE)
	set_dynamic_menu_updating_state(DMU_ON);
		
      /* RGA: accelerators for (shift) left click on some objects... */ 
      if (buttons == B_LEFT && obj)
      {
	td_set_obj_updating(OBJ_UPDATING_OFF);
	for (sel = get_selection(); sel != NULL;)
	{
	  switch (obj->ob_type)
	  {
	  case OT_VAR:
	    if (ev->ev_buttons & (B_SHIFT_MASK|B_MULT_CLICKS))
	    {
	      struct objst *ob;
	      for (ob = obj->ob_child; ob; ob = ob->ob_next)
		if (!ob->ob_child)
		  break;
	      do_var_or_expr
		((objid_t)sel->se_code,
		 (!obj->ob_child || !ob) ?
		 MR_VAR_ACCELERATOR_ACTION : MR_VAR_COLLAPSE,
		 (char *)NULL);
	    }
	    break;
	  case OT_EXPR:
	    if (ev->ev_buttons & (B_SHIFT_MASK|B_MULT_CLICKS))
	    {
	      struct objst *ob;
	      for (ob = obj->ob_child; ob; ob = ob->ob_next)
		if (!ob->ob_child)
		  break;
	      do_var_or_expr
		((objid_t)sel->se_code,
		 (!obj->ob_child || !ob) ?
		 MR_VAR_ACCELERATOR_ACTION : MR_VAR_COLLAPSE,
		 (char *)NULL);
	    }
	    break;
	  case OT_BPHEAD:
	    set_bphead_menu_state(global_bp_enabled(0, 0));
	    if (ev->ev_buttons & (B_SHIFT_MASK|B_MULT_CLICKS))
	      do_bps((objid_t)sel->se_code, MR_BP_ACCELERATOR_ACTION, (char *)NULL);
	    break;
	  case OT_WPHEAD:
	    set_wphead_menu_state(global_wp_enabled(0, 0));
	    if (ev->ev_buttons & (B_SHIFT_MASK|B_MULT_CLICKS))
	      do_wps((objid_t)sel->se_code, MR_WP_ACCELERATOR_ACTION, (char *)NULL);
	    break;
	  case OT_SRCHEAD:
	    if (ev->ev_buttons & (B_SHIFT_MASK|B_MULT_CLICKS))
	    {
	      struct objst *ob;
	      for (ob = obj->ob_child; ob; ob = ob->ob_next)
		if (!ob->ob_child)
		  break;
	      do_srchead((objid_t)sel->se_code,
			 (!obj->ob_child || !ob) ?
			 MR_SHOW_SOURCE_FILES : MR_HIDE_SOURCE_FILES,
			 (char *)NULL);
	    }
	    break;
	  case OT_ENVHEAD:
	    if (ev->ev_buttons & (B_SHIFT_MASK|B_MULT_CLICKS))
	      do_envhead
		((objid_t)sel->se_code, !obj->ob_child ?
		 MR_SHOW_ALL_ENV_ENTRIES : MR_HIDE_ALL_ENV_ENTRIES,
		 (char *)NULL);
	    break;
	  case OT_SGHEAD:
	    if (ev->ev_buttons & (B_SHIFT_MASK|B_MULT_CLICKS))
	      do_sgh((objid_t)sel->se_code, !obj->ob_child ?
		     MR_SHOW_ALL_SIGNALS : MR_HIDE_ALL_SIGNALS,
		     (char *)NULL);
	    break;
	  case OT_BPT:
	    if (ev->ev_buttons & (B_SHIFT_MASK|B_MULT_CLICKS))
	    {
	      int command = bpt_accelerator_action( (objid_t)sel->se_code );
	      do_bpt((objid_t)sel->se_code, command, (char *)NULL);
	      /* RGA need this check to handle selecting a bpt */
	      /* and its duplicate in a tracking file - first */
	      /* call will remove both bpts, and then sel->se_next */
	      /* becomes invalid */
	      if ( command == MR_BPTMEN_REMOVE )
	      {
	          sel = get_selection();
	          continue;
	      }
	    }
	    else
	      do_bpt((objid_t)sel->se_code, MR_BPTMEN_SOURCE, (char *)NULL);
	    break;
	  case OT_WPT:
	    if (ev->ev_buttons & (B_SHIFT_MASK|B_MULT_CLICKS))
	    {
	      int command = wpt_accelerator_action( (objid_t)sel->se_code );
	      do_wpt((objid_t)sel->se_code, command, (char *)NULL);
	    }
	    break;
	  case OT_SFILE:
	  case OT_SFILE_EX:
	    if (ev->ev_buttons & (B_SHIFT_MASK|B_MULT_CLICKS))
	      do_file((objid_t)sel->se_code, !obj->ob_child ?
		      MR_EXPAND_LIKE_BEFORE : MR_COLLAPSE, (char *)NULL);
	    else
	      do_file((objid_t)sel->se_code, MR_DISPLAY_SOURCE, (char *)NULL);
	    break;
	  case OT_FUNC:
	    if (ev->ev_buttons & (B_SHIFT_MASK|B_MULT_CLICKS))
	      do_func((objid_t)sel->se_code, !obj->ob_child ?  
		      MR_EXPAND_LIKE_BEFORE : MR_COLLAPSE, (char *)NULL);
	    else
	      do_func((objid_t)sel->se_code, MR_DISPLAY_SOURCE, (char *)NULL);
	    break;
	  case OT_BLOCK:
	    if (ev->ev_buttons & (B_SHIFT_MASK|B_MULT_CLICKS))
	      do_block((objid_t)sel->se_code, !obj->ob_child ?  
		       MR_BLOCK_EXPAND : MR_BLOCK_COLLAPSE, (char *)NULL);
	    else
	      do_block((objid_t)sel->se_code, MR_BLOCK_DISPLAY_SOURCE, (char *)NULL);
	    break;
	  }
	  sel = sel->se_next;
	}
	td_set_obj_updating(OBJ_UPDATING_ON);
      }
      else
      {
	sel = get_selection();
	    
	if (sel != NULL && sel->se_next == NULL &&
	    ups_get_object_type(sel->se_code) == OT_FUNC) {
	  maybe_show_stack_entry_source(sel->se_code); 
	}
      }
    }
  }
}

static Edit_display*
display_area_get_display(region)
Region *region;
{
    char* handle = re_get_keyboard_focus_data(region);
    return field_edit_get_display(handle);
}

static bool
display_area_release_focus(region, data, force)
Region *region;
char *data;
bool force;
{
  bool result;
  cursor_t old_cursor = wn_get_window_cursor(WN_STDWIN);
  set_bm_cursor(WN_STDWIN, CU_WAIT);

  indicate_target_menu_search(1);
  target_menu_search_disabled(1, 0); /* set */
  result = field_edit_finish(data, force);
  target_menu_search_disabled(0, 1); /* reset */
  indicate_target_menu_search(0);
  wn_define_cursor(WN_STDWIN, old_cursor);
  return result;
}

static void
display_area_draw(region, wn, width, height, draw_reason)
Region *region;
int wn;
int width, height;
re_draw_reason_t draw_reason;
{
	if (draw_reason == RE_EXPOSED || draw_reason == RE_RESIZED) {
		/* If the global selection is currently highlighted,
		** unhighlight it before doing anything else
		*/
		display_area_overlay_unselect_global_selection();
		set_obj_wn(wn);
	
		if (re_get_keyboard_focus_region() == region) {
			char *handle;

			handle = re_get_keyboard_focus_data(region);
			field_edit_redraw_display(handle);
		}
	}
}

static void
output_window_menu_func(arg, unused_md, cmd)
char *arg;
int unused_md, cmd;
{
	do_output_window_command((Outwin *)arg, cmd);
}

static bool
load_outwin(ow, path)
Outwin *ow;
const char *path;
{
	Edit_buffer *buffer;
	int fd, nread;
	char buf[1024];
	size_t pos;

	if ((fd = open_with_twiddle(path, O_RDONLY)) == -1) {
		failmesg("Can't open", "file", path);
		return -1;
	}
	
	buffer = edit_get_buffer(outwin_get_display(ow));
	pos = edit_get_buffer_length(buffer);

	while ((nread = read(fd, buf, sizeof(buf))) > 0) {
		edit_insert(buffer, pos, buf, (size_t)nread, TRUE);
		pos += nread;
	}
	
	if (nread < 0)
		failmesg("Warning: Error reading from", "file", path);

	close(fd);

	/*  Update thumb bar and make point visible.
	 */
	outwin_insert(ow, "", 0);
	
	return TRUE;
}

static bool
save_outwin(ow, path, mode)
Outwin *ow;
const char *path, *mode;
{
	Edit_buffer *buffer;
	size_t pos;
	FILE *fp;
	const char *buf, *lim;
	bool had_error;

	if ((fp = fopen_with_twiddle(path, mode)) == NULL) {
		if (strcmp(mode, "a") == 0)
			failmesg("Can't append to", "file", path);
		else
			failmesg("Can't create", "file", path);
		return FALSE;
	}

	buffer = edit_get_buffer(outwin_get_display(ow));

	pos = 0;
	while (edit_get_bytes(buffer, pos, &buf, &lim)) {
		if (fwrite(buf, (size_t)(lim - buf), 1, fp) == EOF)
			break;
		pos += lim - buf;
	}

	had_error = ferror(fp);

	if (fclose(fp) == EOF || had_error) {
		failmesg("Error writing to", "file", path);
		return FALSE;
	}

	return TRUE;
}

void
do_output_window_command(ow, cmd)
Outwin *ow;
int cmd;
{
	static char *lastfile = NULL;
	char *pattern, *path;
	const char *mode;
	
	switch (cmd) {
	case MR_OUTWIN_LOAD:
		if (lastfile == NULL)
			lastfile = strsave("");
		
		if (prompt_for_string("filename", "Read from file: ", lastfile,
				      &path) == 0) {
			load_outwin(ow, path);

			free(lastfile);
			lastfile = path;
		}
		break;
	case MR_OUTWIN_SAVE:
		if (prompt_for_output_file("Save to file: ",
					   &path, &mode) == 0) {
			save_outwin(ow, path, mode);
			free(path);
		}
		break;
	case MR_OUTWIN_CLEAR:
		outwin_delete_all_text(ow);
		break;
	case MR_OUTWIN_SEARCH_FORWARDS:
	case MR_OUTWIN_SEARCH_BACKWARDS:
	   {
		int offset = 0;
		wn_clr_xclipboard();
		pattern = get_typing_line_string();
		/* Skip initial direction indicator if any */
		if ( *pattern == '/' || *pattern == '?') 
		   offset=1;
		outwin_search(ow, pattern+offset, cmd == MR_OUTWIN_SEARCH_FORWARDS);
		free(pattern);
		break;
	   }
	case MR_OUTWIN_CLOSE:
		outwin_unmap(ow);
		break;
	default:
		panic("bad cmd in owmf");
	}
}

static void
outwin_draw(region, wn, width, height, draw_reason)
Region *region;
int wn;
int width, height;
re_draw_reason_t draw_reason;
{
    Outwin *ow;
    size_t pos = 0;

    ow = (Outwin *)re_get_data(region);
    if (draw_reason == RE_RESIZED)
    {
	pos = outwin_get_pos(ow);
	outwin_goto_point_and_update(ow, 0); /* set to beginning */
    }
    if (draw_reason == RE_EXPOSED || draw_reason == RE_RESIZED)
	outwin_redraw(ow);
    if (draw_reason == RE_RESIZED)
	outwin_goto_point_and_update(ow, pos); /* restore original position */
}

static void
srcwin_draw(region, wn, width, height, draw_reason)
Region *region;
int wn;
int width, height;
re_draw_reason_t draw_reason;
{
    Srcwin *sw;
    size_t pos = 0;

    sw = (Srcwin *)re_get_data(region);
    if (draw_reason == RE_RESIZED)
    {
	pos = srcwin_get_pos(sw);
	srcwin_goto_point(sw, 0); /* set to beginning */
    }
    if (draw_reason == RE_EXPOSED || draw_reason == RE_RESIZED)
	srcwin_redraw(sw);
    if (draw_reason == RE_RESIZED)
	srcwin_goto_point(sw, pos);	/* restore original position */
}

static void
tbar_draw(region, wn, width, height, draw_reason)
Region *region;
int wn;
int width, height;
re_draw_reason_t draw_reason;
{
	tbar_id_t tbar_id;

	if (draw_reason != RE_EXPOSED && draw_reason != RE_RESIZED)
		return;

	tbar_id = (tbar_id_t)re_get_data(region);
	if (draw_reason == RE_RESIZED)
		tb_set_box_pos_and_size(tbar_id, TRUE, 0, width / 3);
	tb_expose(tbar_id);
}

static void
tbar_input(region, ev)
Region *region;
event_t *ev;
{
	tbar_event_handler((tbar_id_t)re_get_data(region), ev);
}

static int make_permanent_menu(region)
Region* region;
{
    extern MENU cmd_men;
    static int md  = -1;
    int md2;

    if (md == -1)
    {
	md = Minsert(&cmd_men);
    }
    md2 = Mdup(md);
    init_menu_toggle_items(md2);
    initialise_menu(region, md2, "pmenu",
					permanent_menu_func, (char *)NULL);
    return md2;
}

static void
initialise_menu(region, md, menu_name, menu_func, menu_func_data)
Region *region;
int md;
const char *menu_name;
void (*menu_func)PROTO((char *mf_data, int mf_md, int mf_rv));
char *menu_func_data;
{
	menu_arg_t *ma;

	ma = (menu_arg_t *)e_malloc(sizeof(menu_arg_t));

	if (md != -1) {
		ma->ma_md = md;
		Mnobox(ma->ma_md);
	}
	else
		ma->ma_md = -1;

	ma->ma_menu_name = menu_name;
	ma->ma_menu_func = menu_func;
	ma->ma_data = menu_func_data;
	ma->ma_md_changed = FALSE;

	re_set_data(region, (char *)ma);
	re_set_callbacks(region, menu_draw, menu_input, (re_destroy_proc_t)NULL);
	re_set_cursor(region, CU_MENU);
	re_set_mhcaps(region, "select", "", "");
}

void
menu_input(region, ev)
Region *region;
event_t *ev;
{

	menu_arg_t *ma;
	int wn, mask, rv, step_menu = 0;

	ma = (menu_arg_t *)re_get_data(region);

	if (ma->ma_md == -1 || (ev->ev_buttons & B_LEFT) == 0)
		return;

	if (!re_set_keyboard_focus((Region *)NULL, (char *)NULL))
		return;

	wn = ev->ev_wn;
	mask = ~(1 << ma->ma_md);
	rv = Mselect(ev->ev_x, ev->ev_y, wn, MS_PRESSED, mask);
	if (!rv)
	  return;
	if (rv == (int)MR_LOAD_ONE_LIBRARY && !strcmp(ma->ma_menu_name, "target"))
	    ma->ma_data = (char *)ev;
	else
	{
	  for (;;) {
	    wn_next_event(wn, EVENT_MASK, ev);
	    if ((ev->ev_buttons & B_LEFT) == 0)
	      break;
	    Mselect(ev->ev_x, ev->ev_y, wn, MS_CONTINUE, mask);
	  }
	  rv = MS_rv(Mselect(ev->ev_x, ev->ev_y, wn, MS_RELEASED, mask));
	}

	if (rv != 0) {

	  /* Refresh before moving stepping into possible reverse video */
	  /* file. Also refresh outwin when listing help verbiage  RGA */

	  if (step_menu || !strcmp(ma->ma_menu_name, "pmenu"))	
	    re_expose(region, FALSE);
	  if (!strcmp(ma->ma_menu_name, "target"))
	    ma->ma_data = (char *)ev;
	  td_record_menu_command(ma->ma_menu_name, rv);
	  if (ma->ma_menu_func != permanent_menu_func ||
	      !(rv == MR_DISWIN_SEARCH_FORWARDS ||
		rv == MR_DISWIN_SEARCH_BACKWARDS))
	    display_area_overlay_control(0, 1); /* reset */

	  (*ma->ma_menu_func)(ma->ma_data, ma->ma_md, rv);
	}

	Mclear(ma->ma_md);
}

/*  Show the target menu.
 */
static void
menu_draw(region, wn, width, height, draw_reason)
Region *region;
int wn;
int width, height;
re_draw_reason_t draw_reason;
{
	menu_arg_t *ma;

	if (draw_reason != RE_EXPOSED && draw_reason != RE_RESIZED)
		return;

	ma = (menu_arg_t *)re_get_data(region);

	if (ma->ma_md != -1) {
		if (draw_reason == RE_RESIZED || ma->ma_md_changed) {
			Mplace(ma->ma_md, 0, 0);
			Msize(ma->ma_md, width, height);
			ma->ma_md_changed = FALSE;
		}
		Mdisplay(ma->ma_md, wn, FALSE);
	}
	else
	  if (!wn_use_alloc_color_for_highlight(0))
		wn_shade_area(wn, 0, 0, width, height, WN_GREY4, R_RPL);
}

void
set_start_menu(md)
int md;
{
	target_menu_info_t *tm = get_target_menu_info();
	Region *region = tm->tm_mdtab[TM_START].region;
	menu_arg_t *ma = (menu_arg_t *)re_get_data(region);
	
	Mremove(ma->ma_md);
	
	ma->ma_md = md;
	ma->ma_md_changed = TRUE;

	re_expose(region, FALSE);

	return;
}

/*  Set the dynamic menu to md.
 */
void
set_dynamic_bphead_menu_state(md, enable)
int md;
bool enable;
{
	Region *dmenu_region;
	char rvs[2];

	dmenu_region = get_dynamic_menu_region();

	rvs[0] = (enable) ?
	  (char)MR_ENABLE_ALL_BREAKPOINTS : (char)MR_DISABLE_ALL_BREAKPOINTS;
	rvs[1] = 0;
	Mnonsel(md, rvs, 1);
	re_expose(dmenu_region, FALSE);
}

/*  Set the dynamic menu to md.
 */
void
set_dynamic_wphead_menu_state(md, enable)
int md;
bool enable;
{
	Region *dmenu_region;
	char rvs[2];

	dmenu_region = get_dynamic_menu_region();

	rvs[0] = (enable) ?
	  (char)MR_ENABLE_ALL_WATCHPOINTS : (char)MR_DISABLE_ALL_WATCHPOINTS;
	rvs[1] = 0;
	Mnonsel(md, rvs, 1);
	re_expose(dmenu_region, FALSE);
}

/*  Set the dynamic menu to md.
 */
void
set_dynamic_menu(md, menu_name)
int md;
const char *menu_name;
{
	menu_arg_t *ma;
	Region *dmenu_region;

	dmenu_region = get_dynamic_menu_region();

	ma = (menu_arg_t *)re_get_data(dmenu_region);

	if (md != ma->ma_md && ma->ma_md != -1)
		Mremove(ma->ma_md);

	ma->ma_menu_name = menu_name;
	ma->ma_md = md;
	ma->ma_md_changed = TRUE;

	re_expose(dmenu_region, FALSE);
}

static void
mousehole_draw(region, wn, width, height, draw_reason)
Region *region;
int wn;
int width, height;
re_draw_reason_t draw_reason;
{
	if (draw_reason == RE_EXPOSED || draw_reason == RE_RESIZED) {
		wn_set_area(wn, 0, 0, width, height, WN_BG(wn));
		draw_mousehole(wn, 0, 0, width, height);
	}
}

static void
srcpos_draw(region, wn, width, height, draw_reason)
Region *region;
int wn;
int width, height;
re_draw_reason_t draw_reason;
{
	if (draw_reason == RE_EXPOSED || draw_reason == RE_RESIZED) {
		draw_src_name_and_lnum(wn);
        }
}

/*  For setting and getting redirection to outwin
 */
int
list_matches_to_outwin(set, reset)
     int set;
     int reset;
{
  static int win;

  if (set)
    win = set;
  if (reset)
    win = 0;
  return(win);
    
}

void
handle_display_area_char(key_char, meta)
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
  field_edit_handle_key_event
    (get_display_area_region(),
     re_get_keyboard_focus_data(get_display_area_region()), &evbuf);
}

void
handle_typing_line_char(key_char, meta)
int key_char;
bool meta;
{
  static event_t evbuf;
  Edit_display *display;
	    
  display = (Edit_display *)re_get_data(get_typing_line_region());
  evbuf.ev_type = EV_KEY;
  evbuf.ev_char = key_char;
  if (meta == TRUE)
    evbuf.ev_buttons |= B_ALT_MASK;
  else
    evbuf.ev_buttons = 0;
  edit_handle_wn_key_event (display, &evbuf);
}
