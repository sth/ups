/* ui_iploop.c - main loop, and some miscellaneous ui functions */

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


/* @(#)ui_iploop.c	1.41 04 Jun 1995 (UKC) */
char ups_ui_iploop_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#ifdef __STDC__
#include <unistd.h>
#endif

#ifndef F_OK
#define F_OK	0
#endif

#include <local/wn.h>
#include <local/obj/obj.h>

#include <local/ukcprog.h>
#include <local/obj/newtb.h>
#include <local/edit/edit.h>
#include <local/edit/win.h>
#include <mtrprog/strcache.h>

#include "cursors.h"
#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "st.h"
#include "ui.h"
#include "tdr.h"
#include "srcwin.h"
#include "ui_priv.h"
#include "data.h"
#include "expr.h"
#include "reg.h"	/* for re_redraw_root() and clear_message() */
#include "state.h"
#include "as.h"		/* for dump_as_assember() */
#include "debug.h"
#include "obj_bpt.h"
#include "va.h"
#include "st_debug.h"

#ifndef __STDC__
double strtod();
#endif

Edit_history* prompt_history = NULL;

static void my_cu_set PROTO((int wn, int cursor));
static void draw_message PROTO((int wn, const char *mesg));
static int prompt_for_input PROTO((const char *what, const char *prompt,
				   const char *default_string,
				   const char *options,
				   char **p_string));
static bool loop_scroll PROTO((tbar_id_t tbar_id, event_t *ev, int normal));

/*  Scroll loop.
 *
 *  If the left mouse button is pressed, go into sample mode and
 *  scroll continuously at a rate depending on the distance
 *  that the mouse is moved from the place it was first pressed.
 *
 *  If the middle button is pressed, goto position y in the display area
 *  or source window, where y is the y coordinate in the window originally
 *  in ev->ev_wn.
 */
void
tbar_event_handler(tbar_id, ev)
tbar_id_t tbar_id;
event_t *ev;
{
	static int normal = 1;
	static int type_found = 0;

	if (!type_found)
	{
	  /*
	   * Scrollbar type function (1 = normal UPS, 0 = opposite)
	   */
	  const char *pol;
	  char *c;
	  
	  pol = wn_get_default("ScrollbarType");
	  if (pol)
	    for (c = (char *)pol; *c; c++)
	      *c = tolower(*c);
	  if (!pol || (pol && !strcmp(pol, "ups"))) /* RCB: Added ()'s */
	    normal = 1;
	  else
	    normal = 0;
	  type_found = 1;
	}
	
	if (ev->ev_buttons == 0) {
		int unit;

		unit = tb_tbpos_to_unit(tbar_id, ev->ev_wn, ev->ev_y, FALSE);
		tb_show_pos(tbar_id, unit);
	}

	if (ev->ev_type == EV_BUTTON_DOWN && ev->ev_flags == B_LEFT) {
		if (!loop_scroll(tbar_id, ev, normal))
			tb_scroll(tbar_id, ev->ev_y, TRUE);
		
	}
	else if (ev->ev_buttons & B_MIDDLE) {
		int unit;

		unit = tb_tbpos_to_unit(tbar_id, ev->ev_wn, ev->ev_y, TRUE);
		tb_goto(tbar_id, unit, TRUE);
	}
	else if (ev->ev_type == EV_BUTTON_UP && ev->ev_flags == B_RIGHT) {
		tb_scroll(tbar_id, -ev->ev_y, TRUE);
	}
}

static bool
loop_scroll(tbar_id, ev, normal)
tbar_id_t tbar_id;
event_t *ev;
int normal;
{
	static char scrolldist[] = { 0, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4 };
	int orig_y, old_inmode;
	int wn, height, dist;
	bool done_scroll;
	static unsigned int sleep_for = 0;
	static bool delay_found = FALSE;

	done_scroll = FALSE;
	if (!delay_found)
	{
	  const char *defstr = wn_get_default("ScrollDelay");
	  char *ptr;
	  long k = 0;
	  
	  if (defstr != NULL)
	  {
	    k = strtol (defstr, &ptr, 0);
	    if (k && ptr)
	      sleep_for = 1000.0 * k;
	  }
	  else
	    sleep_for = 1000.0 * 25; /* RGA make 25 the default */
	  delay_found = TRUE;
	}
	wn = ev->ev_wn;
	height = wn_get_height(wn);
	
	orig_y = ev->ev_y;
	old_inmode = wn_inmode(wn, WN_SAMPLE);
	
	for (;;) {
		wn_next_event(wn, EVENT_MASK, ev);
		
		if ((ev->ev_buttons & B_LEFT) == 0)
			break;

		if (ev->ev_y < 0 || ev->ev_y >= height)
			continue;

		if (orig_y == ev->ev_y) {
			my_cu_set(wn, CU_SC_PRESSED);
		}
		else {
			my_cu_set(wn, (orig_y > ev->ev_y) ? CU_SCROLL_UP
				  			  : CU_SCROLL_DOWN);
		}

		dist = (orig_y - ev->ev_y) / 3;

		if (!normal)
			dist = -dist;

		if (dist >= 0 && dist < sizeof(scrolldist))
			dist = scrolldist[dist];
		else if (dist < 0 && -dist < sizeof(scrolldist))
			dist = -scrolldist[-dist];

		if (dist != 0) {
			wn_updating_off(wn);
			tb_scroll(tbar_id, dist, TRUE);
			wn_updating_on(wn);
			done_scroll = TRUE;
			if (sleep_for > 0)
				usleep (sleep_for/(abs(dist)*abs(dist)));
		}
	}

	wn_inmode(wn, old_inmode);
	my_cu_set(wn, CU_SBAR);

	return done_scroll;
}
	
/*  Set the cursor if the new cursor is different from the current one.
 */
static void
my_cu_set(wn, cursor)
int wn;
int cursor;
{
	static int old_cursor = -1;

	if (cursor != old_cursor) {
		old_cursor = cursor;
		set_bm_cursor(wn, cursor);
	}
}

wn_abort_func_t
set_user_abort_func(func)
wn_abort_func_t func;
{
	return wn_is_open(WN_STDWIN) ? wn_set_abort_func(WN_STDWIN, func)
				     : NULL;
}

/*  Display a message in the message area and ring the bell.
 *  The beep is suppressed if the format starts with \b.
 *
 *  The value of errno is unchanged by this function (some functions
 *  rely on this property).
 */
void
display_message(mesg)
const char *mesg;
{
	int wn, want_bell;
	int save_errno;

	want_bell = *mesg != '\b';
	if (!want_bell)
		++mesg;

	save_errno = errno;

	wn = get_message_wn();
	if (wn == -1) {
		write(2, mesg, strlen(mesg));
		write(2, "\n", 1);
	}
	else {
		if (want_bell)
			wn_bell(wn);
		draw_message(wn, mesg);
	}

	errno = save_errno;
}

void
clear_message()
{
	draw_message(get_message_wn(), "");
}

static char last_message[256] = { 0 };

static void
draw_message(wn, mesg)
int wn;
const char *mesg;
{
	int w, h;
	size_t maxlen;

	wn_get_window_size(wn, &w, &h);
	wn_set_area(wn, 1, 1, w - 1, h - 1, WN_BG(wn));

	maxlen = wn_strpos(mesg, w - 3, (font_t *)NULL, FALSE);
	if (maxlen < strlen(mesg)) {
		(void) memcpy(last_message, mesg, maxlen);
		last_message[maxlen] = '\0';
		mesg = last_message;
	}
	else {
		strcpy(last_message, mesg);
	}
	wn_tputs(wn, mesg, 2, 2);
}

void
redraw_message(wn)
int wn;
{
	int w, h;

	wn_get_window_size(wn, &w, &h);
	wn_set_area(wn, 1, 1, w - 1, h - 1, WN_BG(wn));
	if (strlen(last_message) > 0)
		wn_tputs(wn, last_message, 2, 2);
}

/*  Handle the contents of the typing line when the user hits return.
 *  We normally interpret this as a request to treat the typing line
 *  as a variable name to be displayed.
 *
 *  If the string starts with "$debug:" then invoke a debugging routine.
 *  If the string starts with "$check" invoke td_check on the rest of it.
 */
void
process_typing_line_text(text)
const char *text;
{
	static const char debug[] = "$debug:";
	static const char check[] = "$check";
	/*   Typing line shortcuts
	 *    ( Russ Browne ( RCB), russ@amc.com
	 * %g - display global without going into local search
	 * %b - add breakpoint
	 * %l - list
	 * /...  or ?... do a search ala vi
	 * % used instead of $ because:
	 *   A '$' in a startup file, to set up UPS_Fx__STR, has
	 *   unexpected effects if the string is inside double instead
	 *   of single quotes
	 */
	static const char display_global[] = "%g "; 
	static const char add_brkpoint[] = "%b ";
	static const char list_file[] = "%l ";
	static const char dump_mem[] = "%d";


	/*  Ignore a zero length string.
	 */
	if (*text == '\0')
		return;

	if (strncmp(text, debug, sizeof(debug) - 1) == 0) {
		do_debug_command(text + sizeof(debug) - 1);
	}
	else if (strncmp(text, check, sizeof(check) - 1) == 0) {
		td_check(text + sizeof(check) - 1);
	}
	else if (strncmp(text, display_global, sizeof(display_global) - 1) == 0) 
	{
	  show_global(
		  get_current_srcwin()		/* srcwin */
		, get_displayed_fil()		/* srcfil */
		, (func_t *)NULL		/* srcfunc */
		, (common_block_t *)NULL        /* srccblock */
		, text+sizeof(display_global)-1 /* name */
		, TRUE				/* want_errmesg */
		, TRUE				/* from_typing_line */
		, FALSE				/* restoring */
		, (objid_t *)NULL		/*p_var_obj */
		, FALSE				/* undef check */
		, FALSE				/* double_click */
		);
	}
	else if (strncmp(text, add_brkpoint, sizeof(add_brkpoint) - 1) == 0)
	{
	    func_t* f;
	    int cnt;
	    const char* fname = text+sizeof(add_brkpoint)-1;
	    while ( *fname == ' ')
	       fname++;
	    cnt = add_breakpoints_for_fname( fname, &f);
	    if ( cnt == 0 )
	      errf("No function `%s'", fname);
	    else
	    {
		if (f->fu_fil == NULL)
		    errf("Can't find source for function %s", f->fu_name);
		else if (FU_LNOS(f) == NULL)
		    errf("No line number information for function %s", f->fu_name);
		else
		    show_source(f->fu_fil, FU_LNOS(f)->ln_num);
	    }
	}
	else if (strncmp(text, list_file, sizeof(list_file) - 1) == 0)
	{
	    fil_t* fil;
	    const char* finame = text+sizeof(list_file)-1;
	    while ( *finame == ' ')
	       finame++;
	    fil = name_to_fil(finame);
	    if ( fil)
		show_source(fil, 1);
	    else
		 errf("\bNo Source File", finame);
	}
	else if (strncmp(text, dump_mem, sizeof(dump_mem) - 1) == 0)
	{
	    dump_memory(text);
	}
	else if (*text == '/' || *text == '?') {
	    /* For split screens, search something on the current window */
	    Region* typing_region =  get_typing_line_region();
	    int    typing_wn = re_get_wn(typing_region);
	    int    ourRoot = wn_get_root_window(typing_wn);
	    Region* src_region = get_current_srcwin_region();
	    int	   src_wn = re_get_wn(src_region);
	    int    srcRoot = wn_get_root_window(src_wn);
	    if ( srcRoot == ourRoot)
	    {
	        srcwin_search(get_current_srcwin(), text+1, *text == '/');
	    } else
	    {
	        Region* displ_region = get_display_area_region();
	        int    displ_wn = re_get_wn(displ_region);
	        int    displRoot = wn_get_root_window(displ_wn);
		if ( displRoot == ourRoot)
		{
    		    Outwin* dw = (Outwin *)re_get_data(get_display_area_region());
    		    display_area_overlay_search
      		       ((Outwin*)displ_region, text+1, *text == '/');
		} else
		{
	  	    Outwin* ow = get_current_outwin();
		    if  (ow)
		        outwin_search(ow, text+1, *text == '/');
		}
	    }
	}
	else {
		cursor_t old_cursor;

		old_cursor = wn_get_window_cursor(WN_STDWIN);
		set_bm_cursor(WN_STDWIN, CU_WAIT);
		td_record_show_var((fil_t *)NULL, 0, text);
		show_var_from_typing_line(get_current_srcwin(), text);
		wn_define_cursor(WN_STDWIN, old_cursor);
	}
}
	
void
do_debug_command(line)
const char *line;
{
	typedef enum {
		ASM, ASMSRC, SCSTATS, SETDEFAULTOBJ,
		LOADSYMS, DUMPSYMS, DUMPDISPLAY, DUMPOBJS, DUMPSTACK, DBFLAGS,
		PANIC, QUIT, BADADDR, DUMPSTATE,
		LIST
	} command_t;
	typedef struct cmdst {
		const char *cmd_name;
		command_t cmd_command;
	} cmd_t;
	static cmd_t cmdtab[] = {
		{ "asm",		ASM		},
		{ "asmsrc",		ASMSRC		},
		{ "scstats",		SCSTATS		},
		{ "loadsyms",		LOADSYMS	},
		{ "dumpsyms",		DUMPSYMS	},
		{ "setdefobj",		SETDEFAULTOBJ	},
		{ "dumpstack",		DUMPSTACK	},
		{ "dumpstate",		DUMPSTATE	},
		{ "dbflags",		DBFLAGS		},
		{ "panic",		PANIC		},
		{ "badaddr",		BADADDR		},
		{ "quit",		QUIT		},
		{ "?",			LIST		}
	};
#define NCMDS	(sizeof cmdtab / sizeof *cmdtab)
	static const unsigned long badaddr = 0x42424242;
	cmd_t *cmd;
	const char *cmdname;
	size_t len;
	target_t *xp;

	while (*line == ' ')
		++line;
	if (*line == '\0') {
		errf("Null $debug command (`?' for list)");
		return;
	}

	td_record_debug_command(line);

	cmdname = line;
	while (*line != ' ' && *line != '\0')
		++line;
	len = line - cmdname;
	while (*line == ' ')
		++line;
	
	xp = get_current_target();

	for (cmd = cmdtab; cmd < &cmdtab[NCMDS]; ++cmd)
		if (strncmp(cmd->cmd_name, cmdname, len) == 0 &&
						cmd->cmd_name[len] == '\0')
			break;

	if (cmd == cmdtab + sizeof cmdtab / sizeof *cmdtab) {
		if (xp->xp_ops->xo_do_debug_command == NULL ||
		    !xp_do_debug_command(xp, cmdname, len, line)) {
			errf("Unknown $debug command %.*s (`?' for list)",
							(int)len, cmdname);
		}
		return;
	}

	switch (cmd->cmd_command) {
	case LIST:
		{
			char *s, *new;

			s = strsave("cmds:");
			for (cmd = cmdtab; cmd < &cmdtab[NCMDS]; ++cmd) {
				new = strf("%s %s", s, cmd->cmd_name);
				free(s);
				s = new;
			}
			errf("\b%s", s);
		}
		break;
	case DBFLAGS:
		if (*line == '\0')
			errf("\bDebug_flags=0x%04lx", Debug_flags);
		else {
			unsigned long old;

			old = Debug_flags;
			Debug_flags = strtol(line, (char **)NULL, 0);
			errf("Debug_flags=0x%04lx (was 0x%04lx)",
							Debug_flags, old);
		}
		break;
	case ASM:
	case ASMSRC:
		dump_as_assembler(line, cmd->cmd_command == ASMSRC);
		break;
	case SCSTATS:
		sc_dump_stats_of_newest_sc();
		break;
	case LOADSYMS:
		debug_load_symbols(xp, line);
		break;
	case DUMPSYMS:
		debug_dump_symbols(xp, line);
		break;
	case SETDEFAULTOBJ:
		td_set_default_obj_to_selection();
		break;
	case DUMPSTACK:
		dump_stack_to_file(xp, line);
		break;
	case DUMPSTATE:
		save_all_breakpoints_to_file("stdout", stdout);
		write_var_state_to_file("stdout", stdout);
		break;
	case PANIC:
		panic("you asked for it, you got it.  Goodbye ...");
		break;
	case BADADDR:
		errf("Writing a byte to address 0x%lx", badaddr);
		*(char *)badaddr = 42;
		errf("Write to address 0x%lx did not fault!", badaddr);
		break;
	case QUIT:
		exit(1);
	default:
		panic("unknown command");
	}
	return;
}

int
prompt_for_output_file(prompt, p_path, p_mode)
const char *prompt;
char **p_path;
const char **p_mode;
{
	char *path;
	const char *mode;
	
	if (prompt_for_string("filename", prompt, "", &path) != 0)
		return -1;

	if (access(path, F_OK) == 0) {
		char *rprompt;
		int choice;

		rprompt = strf("%s exists - replace, append or cancel [rac]: ",
			       path);

		if (prompt_for_choice("file mode", rprompt, "",
				      "rac", &choice) != 0)
			return -1;

		free(rprompt);

		switch (choice) {
		case 'r':
			mode = "w";
			break;
		case 'a':
			mode = "a";
			break;
		case 'c':
			return -1;
		default:
			panic("bad mode in pdp");
			mode = NULL; /* to satisfy gcc */
		}
	}
	else {
		mode = "w";
	}

	*p_path = path;
	*p_mode = mode;
	return 0;
}

bool
prompt_for_choice(what, prompt, default_choice, options, p_choice)
const char *what, *prompt, *default_choice, *options;
int *p_choice;
{
	char *str;
	
	if (prompt_for_input(what, prompt, default_choice, options, &str) != 0)
		return -1;

	if (strlen(str) != 1)
		panic("prompt botch in pfc");
	*p_choice = tolower(*str);
	free(str);
	
	return 0;
}

int
prompt_for_string(what, prompt, default_string, p_string)
const char *what, *prompt, *default_string;
char **p_string;
{
	return prompt_for_input(what, prompt, default_string,
				(const char *)NULL, p_string);
}

static int
prompt_for_input(what, prompt, default_string, options, p_string)
const char *what, *prompt, *default_string, *options;
char **p_string;
{
	const char *preprompt;
	Region *tx_region;
	Region *ta_region;
	int tx_wn, tx_width, tx_height, tx_margin, tx_base_margin;
	int ta_wn, ta_width, ta_height;
	int wn, orig_wn, pwidth, maxlen;
	char *text;
	bool force;
	font_t *font;
	Edit_buffer *buffer;
	Edit_display *display;
	
	/* Because the history and enter buttons are not functional
	** dueing the local event loop, we cover them up
	*/
	tx_region = get_extended_typing_line_region();
	tx_wn = re_get_wn(tx_region);
	wn_get_window_size(tx_wn, &tx_width, &tx_height);

	ta_region = get_typing_line_region();
	ta_wn = orig_wn = re_get_wn(ta_region);
	wn_get_window_size(ta_wn, &ta_width, &ta_height);
	tx_margin = (tx_height-ta_height)/2;
	tx_base_margin = (tx_height-ta_height)/4;

	font = wn_get_sysfont();
	pwidth = 2+wn_strwidth(prompt, font);
	
	maxlen = (ta_width - pwidth) / font->ft_width;

	if (maxlen < 1) {
		int prompt_offset;
		
		prompt_offset = (1 + 3) - maxlen;
		maxlen = 1;
		
		if (prompt_offset > (int)strlen(prompt)) {
			errf("No room to prompt for input");
			return -1;
		}

		prompt += prompt_offset;
		pwidth -= (prompt_offset - 3) * font->ft_width;

		preprompt = "...";
	}
	else {
		preprompt = "";
	}
	
	wn_set_area(tx_wn, 0, 0, tx_width, tx_height, WN_FG(orig_wn));

	wn = wn_create_subwin(tx_wn, pwidth, tx_margin, tx_width - pwidth, tx_height-tx_margin,
			      WN_INPUT_OUTPUT);

	wn_ttext(ta_wn, preprompt, 2, tx_base_margin, WN_BG(wn), WN_FG(wn));
	wn_ttext(ta_wn, prompt, 2+wn_strwidth(preprompt, font),
		 tx_base_margin, WN_BG(wn), WN_FG(wn));

	display = make_text_display(wn, WN_BG(wn), WN_FG(wn), default_string);
	edit_set_history(display, prompt_history);

	buffer = edit_get_buffer(display);
	force = FALSE;

	for (;;) {
		event_t event;
		
		wn_next_event(wn,
			      EV_KEY | EV_WINDOW_EXPOSED | EV_WINDOW_RESIZED,
			      &event);

		if (event.ev_type == EV_WINDOW_RESIZED ||
		    event.ev_type == EV_WINDOW_EXPOSED) {
			re_redraw_root(event.ev_type, FALSE);

			if (event.ev_type == EV_WINDOW_RESIZED) {
				text = NULL;
				break;
			}

			wn_set_area(tx_wn, 0, 0, tx_width, tx_height,
				    WN_FG(wn));
			wn_ttext(ta_wn, preprompt, 2, tx_base_margin, WN_BG(wn),
				    WN_FG(wn));
			wn_ttext(ta_wn, prompt, 2+wn_strwidth(preprompt, font),
				    tx_base_margin, WN_BG(wn), WN_FG(wn));
			edit_redraw_display(display);
			continue;
		}
		
		edit_handle_wn_key_event(display, &event);

		if (!edit_quit_requested(display)) {
			force = FALSE;
			continue;
		}
		
		text = get_buffer_contents(buffer);
		
		if (options != NULL) {
			if (strlen(text) != 1 ||
			    strchr(options, tolower(*text)) == NULL) {
				errf("Must choose one of [%s]", options);
				free(text);
				text = NULL;
				
				if (force)
					break;
			}
			else {
				break;
			}
		}
		else {
			if (*text == '\0') {
				errf("Zero length %s illegal", what);
				free(text);
				text = NULL;

				if (force)
					break;
			}
			else {
				break;
			}
		}

		force = TRUE;
		
	}

	edit_save_in_history(display);
	edit_close_display(display);
	edit_destroy_buffer(buffer);
	wn_close_window(wn);

	wn_set_area(tx_wn, 0, 0, tx_width, tx_height, WN_BG(orig_wn));
	
	re_expose(tx_region,FALSE);
	
	if (text == NULL)
		return -1;
	
	*p_string = text;
	return 0;
}

/*
**   Dump target memory to the output window and/or file.
**   Syntax of command line
**
**     %d[size] address [ length | .. end_address ]
**
**       size : b for bytes, s for shorts, l for longs, or omit.
**              if omitted, byte or short will be selected based on
**		target endianness.
**
*/
void dump_memory (text)
const char *text;
{
#if IS_BIG_ENDIAN
#define DEFAULT_GROUPING 2
#else
#define DEFAULT_GROUPING 1
#endif
  char buffer[100];
  taddr_t addr;
  int length = 16;
  int grp = 0;
  int parse_error = FALSE;
  int read_error = FALSE;
  const char* t = text;
  target_t * xp = get_current_target();
  if ( xp == NULL || !can_get_target_vars(xp))
  {
    errf("Target not running");
    return;
  }

  /*  Parse the command line */
  t += 2;  /* skip %d */
  switch (*t++)
  {
  case 'b':
    grp = 1; break;
  case 's':
    grp = 2; break;
  case 'l':
    grp = 4; break;
  case ' ':
    grp=DEFAULT_GROUPING; break;
  default:
    parse_error = TRUE; break;
  }
  while (*t == ' ')
    t++;
  if ( !parse_error)
  {
    if ( t[0] == '0' && (t[1] == 'x' || t[1] == 'X'))
      t+=2;
    parse_error = (0==sscanf(t,"%lx",&addr));
    while (isxdigit(*t))
      t++;
  }
  while (*t == ' ')
    t++;
  if (*t && !parse_error)
  {
    if ( t[0] == '.' && t[1] == '.')
    {
      taddr_t end_addr;
      t+= 2;
      while (*t == ' ')
	t++;
      if ( t[0] == '0' && (t[1] == 'x' || t[1] == 'X'))
	t+=2;
      parse_error = (0==sscanf(t,"%lx",&end_addr));
      while (isxdigit(*t))
	t++;
      if ( end_addr <= addr)
	parse_error = TRUE;
      length = end_addr - addr;
    } else if (isxdigit(*t))
    {
      while (*t == ' ')
	t++;
      if ( t[0] == '0' && (t[1] == 'x' || t[1] == 'X'))
      {
	t+=2;
	parse_error = (0==sscanf(t,"%x",&length));
	while (isxdigit(*t))
	  t++;
      }else
      {
	parse_error = (0==sscanf(t,"%u",&length));
	while (isdigit(*t))
	  t++;
      }
    } else
      parse_error = TRUE;
  }
  while (*t == ' ')
    t++;


  /*  Write the command to the output */
  (void)get_current_outwin(); /* ensure window displayed */
  outwin_insert_string(text);
  outwin_insert_string("\n");

  if (parse_error)
  {
    outwin_insert_string("Invalid %d command\n");
    return;
  }
  while(length && !read_error)
  {
    int len = length;
    if ( len > 16)
      len = 16;
    read_error = dump_to_buffer (xp, addr, len, grp, buffer);
    outwin_insert_string(buffer);
    length -= len;
    addr += len;
  }
}
