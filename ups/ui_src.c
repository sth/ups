/* ui_src.c - input handling in the source window */

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


/* @(#)ui_src.c	1.40 04 Jun 1995 (UKC) */
char ups_ui_src_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <sys/types.h>
#include <sys/param.h>		/* for MAXPATHLEN */
#include <sys/stat.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctype.h>
#include <errno.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <local/wn.h>
#include <local/obj/obj.h>
#include <local/menu3.h>
#include <local/edit/edit.h>

#include "ups.h"
#include "reg.h"
#include "symtab.h"
#include "ci.h"
#include "srcwin.h"
#include "tdr.h"
#include "ui.h"
#include "ui_priv.h"
#include "cursors.h"
#include "menudata.h"
#include "expr.h"
#include "obj_bpt.h"
#include "obj_stack.h"
#include "obj_misc.h"
#include "obj_util.h"
#include "exec.h"
#include "state.h"
#include "st.h"

static void call_editor PROTO((Srcinfo *si, int lnum));
static void do_src_menu PROTO((Srcwin *sw, Srcinfo *si, event_t *ev));
static void do_var_selection PROTO((Srcwin *sw, event_t *ev, size_t sel_point,
				    fil_t *fil, int lnum, int skip_locals,
				    bool double_click));
static char *do_var_or_expr_selection PROTO((Edit_display *display,
					     event_t *ev, size_t sel_point,
					     size_t start, size_t lim,
					     bool *p_done_sel_loop,
					     bool do_global_selection));
static void do_expr_selection PROTO((Srcwin *sw, event_t *ev, size_t sel_point, 
                                     fil_t *fil, int lnum));
static void trim_expr PROTO((char *text, size_t sel_pos, size_t lim, 
                             size_t *p_start, size_t *p_lim));
static bool show_source_no_record PROTO((fil_t *fil, int lnum));
static void null_ofunc PROTO((const char *s));

/* { Stuff added by RCB */

static void
call_editor( si, lnum)
Srcinfo* si;
int lnum;
{
      const char *ed;
      char buf[MAXPATHLEN + 1];

      if ((ed = getenv("EDITOR")) == NULL)
	ed = "vi";		/* default if EDITOR is not set */
      
      if (strstr(ed,"emacs") || (*ed == 'x'))
	sprintf (buf, "%s +%d ", ed, lnum);
      else
	sprintf	/* vi is default */
	  (buf, "xterm -geometry 80x40 -title '%s %s' -e %s +%d ",
	   ed, si->fil->fi_name, ed, lnum);
      
      if (construct_source_path_used
	  (si->fil, buf+ strlen(buf)))
      {
	strcat(buf, "&");
	system(buf);
      }
      else
	errf("Unknown file");
}

static void do_src_menu(sw, si, ev)
Srcwin *sw;
Srcinfo* si;
event_t *ev;
{
    static const char *bptcaps[] = {
	"Add breakpoint",
	"Execute to here",
	"Jump to here",
#ifdef NOTYET
	"Add interpreted code",
#endif
	"Edit source",
	NULL
    };
    enum {  RV_ADD_BPT
	  , RV_EXEC_TO_HERE
	  , RV_JUMP_TO_HERE
#ifdef NOTYET
	  , RV_ADD_CODE
#endif
	  , RV_EDIT_SOURCE
	  , RV_LAST_CHOICE
	  , RV_NEW_DEFAULT
	};
    static popup_t bptpop = { -1, TRUE, 0, bptcaps, 0 };
    static int default_choice = -1;
    static int alt_default_choice = -1;
    long sa;
    int res, lnum;
    fil_t *fil;
    func_t *f;
    bool change_deflt = FALSE;

    if ( default_choice == -1 )
    {
	int* p_choice = &default_choice;
	int cnt;
	const char* resouce = "SourceMenuDefault";
	default_choice = RV_LAST_CHOICE;
	for (cnt = 0; cnt < 2; cnt++ )
	{
	    const char* choice = wn_get_default(resouce);
	    *p_choice = default_choice;
	    if ( choice )
	    {
		if ( cnt == 0 &&  !strcmp( choice, "LastSelection"))
		{
		    *p_choice = RV_LAST_CHOICE;
		} else
		if ( cnt == 1 &&  !strcmp( choice, "ResetDefault"))
		{
		    *p_choice = RV_NEW_DEFAULT;
		} else
		if ( !strcmp( choice, "AddBreakPoint")
		   || !strcmp( choice, "AddBreakpoint"))
		{
		    *p_choice = RV_ADD_BPT;
		} else
		if ( !strcmp( choice, "ExecuteToHere"))
		{
		    *p_choice = RV_EXEC_TO_HERE;
		} else
		if ( !strcmp( choice, "JumpToHere"))
		{
		    *p_choice = RV_JUMP_TO_HERE;
		} else
#ifdef NOTYET
		if ( !strcmp( choice, "AddInterpretedCode"))
		{
		    *p_choice = RV_ADD_CODE;
		} else
#endif
		if ( !strcmp( choice, "EditFile"))
		{
		    *p_choice = RV_EDIT_SOURCE;
		}
	    }
	    p_choice = &alt_default_choice;
	    resouce = "AltSourceMenuDefault";
	}
    }

    fil = si->fil;
    lnum = si->fil_lnum;


    if ( default_choice < RV_LAST_CHOICE )
      bptpop.po_last = default_choice;

    if ((ev->ev_buttons & (B_SHIFT_MASK)) != 0)
    {
      if ( alt_default_choice < RV_LAST_CHOICE )
	bptpop.po_last = alt_default_choice;
      else
        change_deflt = TRUE;
    }

    sa = srcwin_draw_arrow(sw, lnum);
    res = select_from_popup(ev->ev_wn, "SourceMenu", B_RIGHT, &bptpop,
		ev->ev_x, ev->ev_y);
    wn_restore_area(sa);
    wn_free_area(sa);
		
    switch(res)
    {
    case RV_ADD_BPT:
    case RV_EXEC_TO_HERE:
    case RV_JUMP_TO_HERE:
	f = lnum_to_func(fil, lnum);
			
	if (f != NULL)
	{
	    td_record_func_and_lnum_cmd
		(f, fil, lnum, (res == 0) ? "breakpoint" : "execto");
			  
	    if (res == RV_ADD_BPT)
	    {
		if (!add_breakpoint_object(f, fil, lnum, TRUE))
		    return;
	    }
	    else if (res == RV_EXEC_TO_HERE)
	       exec_to_lnum(f, fil, lnum);
	    else
	       jump_to_lnum(f, fil, lnum);
#ifdef OS_SUNOS_4
	    if (si->fil->fi_flags & FI_DUPLICATE)
	    {
		fil_t *fil1;
		func_t *f1;
			    
		if (find_duplicate_file(si->fil, &fil1))
		{
		    f1 = lnum_to_func(fil1, lnum);
			      
		    if (f1 != NULL)
		    {
			td_record_func_and_lnum_cmd
				  (f1, fil1, lnum, (res == 0) ?
				   "breakpoint" : "execto");
			if (res == RV_ADD_BPT)
			{
			    add_breakpoint_object(f1, fil1, lnum, TRUE);
			    errf("\bAdding duplicate breakpoint because function has been multiply linked");
			}
			else
			    exec_to_lnum(f, fil1, lnum);
		    }
		    else
		        errf("Line %d of %s is not within a function",
				lnum, fil1->fi_name_only);
		}
	    }
#endif /* OS_SUNOS_4 */
	}
	else
	{
	    errf("Line %d of %s is not within a function",
			       lnum, fil->fi_name_only);
	}
	break;
			
#ifdef NOTYET
    case RV_ADD_CODE:
	add_interpreted_code(fil, lnum, (char *)NULL);
	break;
			
#endif
    case RV_EDIT_SOURCE:
	call_editor(si,lnum);
	break;

    case NO_SELECTION:
	break;

    default:
	panic("unknown popup return value");
    }
    if ( change_deflt )
	default_choice = bptpop.po_last;
}
/* End of RCB stuff } */

static void
null_ofunc(s)
const char *s;
{
}

language_t 
get_user_lang()
{
  static language_t lang = LANG_UNKNOWN;
  static int checked = 0;
  char *c, *c_str = NULL;
  
  if (!checked)
  {
    c_str = (char *)getenv("UPS_LANGUAGE");
    if (c_str)
      for (c = c_str; *c; c++)
	*c = tolower(*c);
    if (c_str)
    {
      if (!strcmp(c_str, "c++"))
	lang = LANG_CC;
      else
	if (!strcmp(c_str, "c"))
	  lang = LANG_C;
	else
	  if (!strcmp(c_str, "f77"))
	    lang = LANG_F77;
	  else
	    if (!strcmp(c_str, "f90"))
	      lang = LANG_F90;
    }
    checked = 1;
  }
  return lang;
}

static char last_srcstr[60];

/*  Display the name and lnum as the source file name and line
 *  number.
 *
 *  BUG: this function is a quick hack, full of magic numbers.
 *  BUG: should handle variable width fonts.
 */
void
echo_src_name_and_lnum(sw, name, lnum)
Srcwin *sw;
const char *name;
int lnum;
{
	static const char *last_name = NULL;
	static int last_width, last_lnum;
	int wn, i, ndigits, width, height, nchars, name_nchars, namelen;
	size_t len;
	font_t *font;

	wn = (int)srcwin_get_data(sw);
	
	if (name == NULL || lnum == -1)
		return;

	wn_get_window_size(wn, &width, &height);
	font = wn_get_sysfont();
	nchars = (width - 8) / font->ft_width;
	if (nchars > sizeof(last_srcstr) - 1)
		nchars = sizeof(last_srcstr) - 1;

	if (width == last_width && name == last_name && lnum == last_lnum)
		return;

	namelen = strlen(name);

	ndigits = 0;
	for (i = 1; i <= lnum; i *= 10)
		++ndigits;

	if (ndigits < 4 && namelen + 1 + 4 < nchars)
		ndigits = 4;

	name_nchars = nchars - (1 + ndigits);
	
	if (name_nchars < namelen)
		name += namelen - name_nchars;

	if (ndigits > nchars) {
		strcpy(last_srcstr, "-");
	}
	else if (lnum == 0) {
		sprintf(last_srcstr, "%*s", name_nchars, name);
	}
	else {
		sprintf(last_srcstr, "%*s:%d", name_nchars, name, lnum);
	}

	len = strlen(last_srcstr);
	
	if (len < nchars) {
		memset(last_srcstr + len, ' ', nchars - len);
		last_srcstr[nchars] = '\0';
	}
	
	wn_tputs(wn, last_srcstr, 4, (height - font->ft_height) / 2);

	last_width = width;
	last_name = name;
	last_lnum = lnum;
}

void
draw_src_name_and_lnum(wn)
int wn;
{
	int width, height;
	font_t *font;

	wn_get_window_size(wn, &width, &height);
	font = wn_get_sysfont();

	wn_tputs(wn, last_srcstr, 4, (height - font->ft_height) / 2);
}

/*  Mouse hole caption function for the source window.  No captions
 *  displayed if there is no source in the source window.
 */
int
mfn_source(caps, arg)
int caps;
char *arg;
{
	Srcinfo srcinfo;

	srcwin_get_info((Srcwin *)arg, 0, 0, &srcinfo);
	return (srcinfo.fil != NULL) ? caps : 0;
}

bool
set_and_display_stopline(fil, lnum)
fil_t *fil;
int lnum;
{
	if (td_set_displayed_source(fil, lnum, "highlight"))
		return TRUE;

	if (!srcwin_set_stopline(fil, lnum))
		return FALSE;

	return (fil != NULL) ? show_source_no_record(fil, lnum) : TRUE;
}

bool
show_source(fil, lnum)
fil_t *fil;
int lnum;
{
	if (td_set_displayed_source(fil, lnum, "showsource"))
		return TRUE;
	
	srcwin_push_current_pos(get_current_srcwin());

	return show_source_no_record(fil, lnum);
}

static bool
show_source_no_record(fil, lnum)
fil_t *fil;
int lnum;
{
	Srcwin *sw;

	sw = get_current_srcwin();

	if (!srcwin_show(sw, fil, lnum))
		return FALSE;
	
	echo_src_name_and_lnum(sw, fil->fi_name_only, lnum);
	return TRUE;
}

fil_t *
get_displayed_fil()
{
	Srcinfo srcinfo;

	if (td_get_displayed_fil(&srcinfo.fil) != 0)
		srcwin_get_info(get_current_srcwin(), 0, 0, &srcinfo);
	return srcinfo.fil;
}

void
source_window_menu_func(arg, md, cmd)
char *arg;
int md, cmd;
{
	Srcwin *sw;
	char *pattern;

	sw = (Srcwin *)arg;

	switch (cmd) {
	case MR_SRCWIN_BACK:
		srcwin_pop(sw);
		break;
	case MR_SRCWIN_SEARCH_FORWARDS:
	case MR_SRCWIN_SEARCH_BACKWARDS:
	   {
		int offset = 0;
		wn_clr_xclipboard();
		pattern = get_typing_line_string();
		/* Skip initial direction indicator if any */
		if ( *pattern == '/' || *pattern == '?') 
		   offset=1;
		srcwin_search(sw, pattern+offset, cmd == MR_SRCWIN_SEARCH_FORWARDS);
		free(pattern);
		break;
	   }
	case MR_SRCWIN_UP_STACK:
		change_displayed_stack_level(TRUE);
		break;
	case MR_SRCWIN_DOWN_STACK:
		change_displayed_stack_level(FALSE);
		break;
	default:
		panic("bad cmd in swmf");
	}

	if (md != -1)
		Mclear(md);
}

void
output_window_event_handler(region, ev)
Region *region;
event_t *ev;
{
	Outwin *ow;
	int buttons;
	
	ow = (Outwin *)re_get_data(region);

	if (ev->ev_type == EV_KEY) {
		outwin_handle_key_event(ow, region, ev);
		return;
	}
	
	buttons = ev->ev_buttons & (B_ANY | B_SHIFT_MASK | B_CONTROL_MASK);

	if (buttons == B_LEFT) {
	  wn_clr_xclipboard();
	  do_selection(outwin_get_display(ow), ev);
	}
	else if (buttons == B_MIDDLE) {
		outwin_set_point(ow, region, ev);
	}
	else if (ev->ev_buttons == B_RIGHT)
	{
	  if (region == re_get_keyboard_focus_region())
	    get_custom_menu_str(handle_output_window_char, ev);
	  return;
	} 
}

static char *
do_var_or_expr_selection(display, ev, sel_point, start, lim, p_done_sel_loop,do_global_selection)
Edit_display *display;
event_t *ev;
size_t sel_point, start, lim;
bool *p_done_sel_loop;
bool do_global_selection;
{
	Edit_buffer *buffer;
	Edit_propchange *startpc, *limpc;
	
	buffer = edit_get_buffer(display);
	
	if (start == lim) {
		*p_done_sel_loop = TRUE;

		return selection_loop(display, ev, (Edit_propchange *)NULL,
				      (Edit_propchange *)NULL, do_global_selection, FALSE);
	}

	add_propchange_pair(buffer, start, lim, 2, &startpc, &limpc);

	for (;;) {
		size_t point;
		
		wn_next_event(ev->ev_wn, EV_BUTTON_UP | EV_MOUSE_MOVED, ev);

		if (ev->ev_type == EV_BUTTON_UP)
			break;

		if (!edit_pixel_to_point(display, ev->ev_x, ev->ev_y, &point))
			continue;

		if (point != sel_point) {
			edit_move_propchange(buffer, startpc, sel_point);
			edit_move_propchange(buffer, limpc, sel_point);
			*p_done_sel_loop = TRUE;
			return selection_loop(display, ev, startpc, limpc,do_global_selection, FALSE);
		}
	}

	*p_done_sel_loop = FALSE;
	return remove_propchange_pair(buffer, startpc, limpc);
}

static void
do_var_selection(sw, ev, sel_point, fil, lnum, skip_locals, double_click)
Srcwin *sw;
event_t *ev;
size_t sel_point;
fil_t *fil;
int lnum;
int skip_locals;
bool double_click;
{
        /* RGA add ' ' so we can click on 'a -> b" */
	/* RCB: add '#' to check for "#stop" */
	/* RCB: Add ':' for '::' */
	static const char pat[] = "!-a-zA-Z0-9$_.>% #:";
	Edit_display *display;
	Edit_buffer *buffer;
	size_t start, lim;
	bool done_sel_loop;
	char *text;
	int found = 0, *this_obj_var, this_obj_exists = 0, mangled = 0;
	errf_ofunc_t oldf = NULL;
	language_t fi_language;

	display = srcwin_get_display(sw);
	buffer = edit_get_buffer(display);

	/* RGA file may be preprocessed, so look at parent to guess at
	   underlying language.
	   */
	fi_language = fil->fi_language;
	if (fi_language == LANG_UNKNOWN &&
	    (fil->fi_flags & FI_RENAMED_OTHER) &&
	    fil->fi_next)
	  fi_language = fil->fi_next->fi_language;
	if (fi_language == LANG_UNKNOWN)
	  fi_language = get_user_lang();
	if (fi_language == LANG_UNKNOWN)
	  fi_language = get_mainfunc_lang();

	if (edit_find_char_backwards(buffer, sel_point, 0, pat, &start))
		++start;
	edit_find_char_forwards(buffer, sel_point, EDIT_MAX_POINT, pat, &lim);

	if (lim > start) {
		char *vartext;
		size_t lstart, llim;

		vartext = edit_copy_bytes(buffer, start, lim, (char *)NULL);

		if ( get_varname(fi_language, vartext,
				vartext + (sel_point - start),
				&lstart, &llim) &&
		     /* RCB: check for and ignore click on common breakpoint text */
		     strcmp(&vartext[lstart],"$printf")  &&
		     ( lstart == 0 || strcmp(&vartext[lstart-1],"#stop") ))
		{
			lim = start + llim;
			start += lstart;
		}
		else {
			start = lim = sel_point;
		}

		free(vartext);
	}

	text = do_var_or_expr_selection(display, ev, sel_point, start, lim,
					&done_sel_loop, TRUE);

	if (text != NULL && !done_sel_loop) {
            cursor_t old_cursor = wn_get_window_cursor(WN_STDWIN);
	    set_bm_cursor(WN_STDWIN, CU_WAIT);
	    /*  f77 maps all upper case variable names to
	     *  lower case, so follow suit.
	     */
	    if (IS_FORTRAN(fi_language)) {
	      char *cptr;
		
	      for (cptr = text; *cptr != '\0'; ++cptr)	
		if (isupper(*cptr))
		  *cptr = tolower(*cptr);
	    }

	    td_record_show_var(fil, lnum, text);

	    /* RGA for mangled C++ names */
	    if (fi_language == LANG_CC
		|| get_user_lang() == LANG_CC
		|| (fi_language == LANG_UNKNOWN &&
		    get_mainfunc_lang() == LANG_CC))
	    {
	      /* suppress error messages for all except last try */
	      oldf = errf_set_ofunc(null_ofunc);
	      mangled = 1;
	    }
	    if (mangled)
	      found = display_var_in_mangled_form
		(text, &this_obj_exists, &this_obj_var);
	    if (!found || double_click)	/* expand/compress if double click */
	      found = show_var
		(sw, fil, lnum, text, oldf, skip_locals, double_click);
			
	    if (mangled)
	      errf_set_ofunc(oldf);
			
	    if (found != 1)
	    {
	      if (mangled)
	      {
		if (!this_obj_exists)
		{
		  if (this_obj_var)
		    delete_variable(this_obj_var); /* get rid of "this"  */
		}
		else
		  if (this_obj_var)
		    clear_selection();
	      }
	      if (found != 2)	/* 2 = may be message about virtual or */
		/* inherited match */
		errf("No function, variable or macro `%s'", text);
	    }

	    free(text);
	    wn_define_cursor(WN_STDWIN, old_cursor);
	  }
}

static void
do_expr_selection(sw, ev, sel_point, fil, lnum)
Srcwin *sw;
event_t *ev;
size_t sel_point;
fil_t *fil;
int lnum;
{
	Edit_display *display;
	Edit_buffer *buffer;
	size_t start, lim;
	bool done_sel_loop;
	char *text;
	
	display = srcwin_get_display(sw);
	buffer = edit_get_buffer(display);
	
	if (edit_find_char_backwards(buffer, sel_point, 0, "{};\n", &start))
		++start;
	edit_find_char_forwards(buffer, sel_point, EDIT_MAX_POINT, "{};",
				&lim);

	if (lim > start) {
		char *exprtext;
		size_t lstart, llim;

		exprtext = edit_copy_bytes(buffer, start, lim, (char *)NULL);
		trim_expr(exprtext, sel_point - start, lim - start,
			  &lstart, &llim);
		free(exprtext);
		
		lim = start + llim;
		start += lstart;
	}
		
	text = do_var_or_expr_selection(display, ev, sel_point, start, lim,
					&done_sel_loop, FALSE);

	if (text != NULL) {
		ebuf_t *eb;

		eb = ebuf_create(TRUE);
		
		if (do_replacements(text, eb)) {
			char *exprtext;
			
			exprtext = ebuf_get(eb, (int *)NULL);
			crush_whitespace(exprtext);
			show_local_expr(fil, lnum, exprtext);
		}

		ebuf_free(eb);
		free(text);
	}
}

static void
trim_expr(text, sel_pos, lim, p_start, p_lim)
char *text;
size_t sel_pos, lim, *p_start, *p_lim;
{
	size_t start, pos, last_nonws;
	size_t first_assign_after_sel;
	int level;
	
	start = 0;

	if (sel_pos != 0) {
		level = 0;
		
		/*  Lose text before an unmatched opening bracket.
		 *  A comma at level 0 also ends the text.
		 */
		for (pos = sel_pos - 1; pos != 0; --pos) {
			if (text[pos] == ',' && level == 0) {
				start = pos + 1;
				break;
			}
			else if (text[pos] == ')') {
				++level;
			}
			else if (text[pos] == '(') {
				if (--level < 0) {
					start = pos + 1;
					break;
				}
			}
		}
	}
		
	first_assign_after_sel = lim;
	
	for (pos = start; pos < lim; ++pos) {
		if (text[pos] == '=' && pos != 0 &&
		    strchr("!><=", text[pos - 1]) == NULL &&
		    pos < lim && text[pos + 1] != '=') {
			if (pos < sel_pos)
				start = pos + 1;

			if (pos > sel_pos && first_assign_after_sel == lim)
				first_assign_after_sel = pos - 1;
		}
	}

	lim = first_assign_after_sel;

	/*  Lose leading whitespace.
	 */
	for (pos = start; pos < lim && isspace(text[pos]); ++pos)
		;
	start = pos;

	/*  Lose trailing whitespace and stuff after an unmatched closing
	 *  bracket.
	 */

	level = 0;
	last_nonws = pos;
	
	for (; pos < lim; ++pos) {
		int ch;
		
		ch = text[pos];

		if (text[pos] == ',' && level == 0) {
			break;
		}
		else if (ch == '(') {
			++level;
		}
		else if (ch == ')') {
			if (--level < 0)
				break;
		}
		
		if (!isspace(ch))
			last_nonws = pos;
	}

	*p_start = start;
	*p_lim = last_nonws + 1;
}

/*  Input function for the source display area.  The left button
 *  selects variable or function names, the middle button
 *  produces the forward/back popup menu, and the right button
 *  the add breakpoint/execute to here menu.
 */
/* ARGSUSED */
void
source_window_event_handler(region, ev)
Region *region;
event_t *ev;
{
	Srcwin *sw;
	Srcinfo srcinfo;
	int buttons, skip_locals = 0, set_point = 1;
	
	sw = (Srcwin *)re_get_data(region);

	if (ev->ev_type == EV_KEY) {
		srcwin_handle_key_event(sw, region, ev);
		return;
	}
	
	srcwin_get_info(sw, ev->ev_x, ev->ev_y, &srcinfo);
	echo_src_name_and_lnum(sw, 
			       (srcinfo.fil != NULL) ? srcinfo.fil->fi_name_only
			       			      : NULL,
			       srcinfo.fil_lnum);

	buttons = ev->ev_buttons & (B_ANY | B_SHIFT_MASK | B_CONTROL_MASK | B_MULT_CLICKS);

	if (buttons == B_MIDDLE) {
		set_point = srcwin_set_point(sw, region, ev);
	}

	if ((!set_point ||	/* RGA allow middle to use tags */
	     buttons == B_LEFT) &&
	    srcinfo.have_point) {
	    srcwin_have_editblock(sw, region, ev);
	    indicate_target_menu_search(1);
	    target_menu_search_disabled(1, 0); /* set */
	    wn_clr_xclipboard();
	    skip_locals = !!(ev->ev_buttons & B_CONTROL_MASK) || !set_point;
	    do_var_selection(sw, ev, srcinfo.point,
			     srcinfo.fil, srcinfo.fil_lnum, skip_locals,
			     FALSE);
	    target_menu_search_disabled(0, 1); /* reset */
	    indicate_target_menu_search(0);
	}
	if (buttons == (B_LEFT | B_SHIFT_MASK) && srcinfo.have_point) {
          cursor_t old_cursor = wn_get_window_cursor(WN_STDWIN);
	  indicate_target_menu_search(1);
	  target_menu_search_disabled(1, 0); /* set */
	  do_expr_selection(sw, ev, srcinfo.point,
			    srcinfo.fil, srcinfo.fil_lnum);
	  target_menu_search_disabled(0, 1); /* reset */
	  indicate_target_menu_search(0);
	  wn_define_cursor(WN_STDWIN, old_cursor);
	}
	/* RCB: Check for Double click on breakpoint
	   RGA: Add Double click on variables for expand/compress */
	if ((buttons & B_LEFT) && (buttons&B_MULT_CLICKS) &&  srcinfo.have_point) {
	    srcwin_have_editblock(sw, region, ev);
	    do_var_selection(sw, ev, srcinfo.point,
			     srcinfo.fil, srcinfo.fil_lnum, skip_locals,
			     TRUE);
	}
	else if (ev->ev_type == EV_BUTTON_DOWN &&
		 (buttons & ~B_SHIFT_MASK) == B_RIGHT &&
		 srcinfo.fil_lnum != 0)
	{
	  if (re_get_keyboard_focus_data(region))
	  {
	    get_custom_menu_str(handle_srcwin_char, ev);
	  } else
	  {
	    do_src_menu(sw, &srcinfo, ev);
	  }
	}
}


void
name_window_event_handler(region, ev)
Region *region;
event_t *ev;
{
  Srcwin *sw;
  Srcinfo srcinfo;
  int buttons;
	
  sw = (Srcwin *)re_get_data(region);
  srcwin_get_info(sw, ev->ev_x, ev->ev_y, &srcinfo);
  buttons = ev->ev_buttons & (B_ANY | B_SHIFT_MASK | B_CONTROL_MASK);
  if (buttons == B_RIGHT && srcinfo.fil_lnum != -1)
  {
    static const char *bptcaps[] = {
      "Edit source",
      "Filepath used",
      "Filepath assumed",
      "Rematch file",
      "Reload file",
      "File dates",
      NULL
    };
    static popup_t bptpop = { -1, TRUE, 0, bptcaps, 0 };
    int res, lnum;
    fil_t *fil;

    fil = srcinfo.fil;
    if (!fil)
      return;
    res = select_from_popup(ev->ev_wn, "FileBoxMenu", B_RIGHT, &bptpop,
			    ev->ev_x, ev->ev_y);

    lnum = srcinfo.fil_lnum + 1;

    switch(res) {
    case 0:
      call_editor(&srcinfo,lnum); /* RCB*/
      break;
    case 1:			/* filepath used */
      show_source_path_used(fil, TRUE);
      break;
    case 2:			/* filepath assumed */
      show_source_path_assumed(fil, TRUE);
      break;
    case 3:			/* file rematch */
      if (fil->fi_name)
	open_source_file(fil, TRUE, FALSE);
      show_source(fil, 1);
      break;
    case 4:			/* file reload */
      if (fil->fi_name)
	open_source_file(fil, FALSE, TRUE);
      show_source(fil, 1);
      break;
    case 5:			/* file dates */
      show_file_and_target_dates(fil, NULL);
      break;
    case NO_SELECTION:
      break;
    default:
      panic("unknown popup return value");
    }
  }
}

