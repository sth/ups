/* ui_misc.c - miscellaneous user interface type support functions */

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


/* @(#)ui_misc.c	1.21 04 Jun 1995 (UKC) */
char ups_ui_misc_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <local/wn.h>
#include <local/obj/obj.h>

#include <local/ukcprog.h>
#include <local/obj/newtb.h>
#include <local/edit/edit.h>
#include <local/edit/textbuf.h>
#include <local/edit/win.h>
#include <local/edit/simplebuf.h>
#include <mtrprog/regex.h>

#include "ups.h"
#include "debug.h"
#include "objtypes.h"
#include "symtab.h"
#include "target.h"
#include "st.h"
#include "ci.h"
#include "srcwin.h"
#include "ui.h"
#include "ui_priv.h"
#include "exec.h"
#include "data.h"
#include "obj_stack.h"
#include "reg.h"
#include "state.h"
#include "tdr.h"

typedef enum {
	GS_BEFORE_LINES,
	GS_IN_LINES,
	GS_AT_END_OF_LINES,
	GS_AFTER_LINES,
	GS_EOF
} getline_state_t;

static const char *getline_from_nlines PROTO((char *arg));
static bool report_error PROTO((lexinfo_t *lx, const char *mesg));
static bool checkarg PROTO((type_t *type, int nargs, int argn, long *p_val));
static ci_nametype_t getaddr PROTO((const char *name, taddr_t *p_addr));
static unsigned long get_regaddr_for_ci PROTO((char *arg, int reg));
static void display_area_scroll_up PROTO((char *data, event_t *ev));
static void display_area_scroll_down PROTO((char *data, event_t *ev));

/*  Arguments for getline_from_nlines.
 */
typedef struct getline_from_nlines_argst {
	getline_state_t ge_state;
	const char **ge_lines;
	int ge_nlines;
	int ge_lnum;
	const char *ge_firstline;
	const char *ge_lastline;
	const char *ge_repl_text;
	const char *ge_repl_val;
} getline_from_nlines_arg_t;

static bool Func_addr_requested;
static lexinfo_t *Error_lexinfo;

static tbar_id_t Display_area_tbar;

const char Stop_funcname[] = "$st";

/*  Flags used when searching.  Active only for the duration of each search.
 */
static int Internal_regex_error;

static const char *
getline_from_nlines(arg)
char *arg;
{
	getline_from_nlines_arg_t *ge;
	const char *line, *cptr;
	bool need_copy;
	size_t len;

	ge = (getline_from_nlines_arg_t *)arg;

	switch (ge->ge_state) {
	case GS_BEFORE_LINES:
		line = ge->ge_firstline;
		ge->ge_state = GS_IN_LINES;
		break;
	case GS_IN_LINES:
		line = ge->ge_lines[ge->ge_lnum];
		if (++ge->ge_lnum == ge->ge_nlines)
			ge->ge_state = GS_AT_END_OF_LINES;
		break;
	case GS_AT_END_OF_LINES:
		line = ge->ge_lastline;
		ge->ge_state = GS_AFTER_LINES;
		break;
	case GS_AFTER_LINES:
		line = NULL;
		ge->ge_state = GS_EOF;
		break;
	case GS_EOF:
		panic("getline_from_lines called after EOF");
		line = NULL; /* to satisfy gcc */
		break;
	default:
		panic("bad state in getline_from_nlines");
		line = NULL; /* to satisfy gcc */
		break;
	}

	if (line == NULL || ge->ge_repl_text == NULL)
		return line;

	need_copy = FALSE;
	len = strlen(ge->ge_repl_text);
	for (cptr = line; *cptr != '\0'; ++cptr) {
		if (strncmp(cptr, ge->ge_repl_text, len) == 0) {
			need_copy = TRUE;
			break;
		}
	}

	if (need_copy) {
		static char *buf;
		static size_t bufsize = 0;
		const char *src;
		char *dst;

		if (bufsize <= strlen(line)) {
			if (bufsize != 0)
				free(buf);
			bufsize = strlen(line) + 1;
			buf = e_malloc(bufsize);
		}

		src = line;
		dst = buf;
		while (*src != '\0') {
			if (strncmp(src, ge->ge_repl_text, len) == 0 &&
			    !isalnum(src[len]) &&
			    src[len] != '_' && src[len] != '$') {
				sprintf(dst, "%s()", ge->ge_repl_val);
				dst += len;
				src += len;
			}
			else
				*dst++ = *src++;
		}
		*dst = '\0';
		
		line = buf;
	}

	return line;
}

static bool
report_error(lx, mesg)
lexinfo_t *lx;
const char *mesg;
{
	if (strcmp(mesg, ci_Illegal_return_from_start_message) == 0)
		mesg = "Illegal return statement in interpreted code";
	errf("%s", mesg);

	*Error_lexinfo = *lx;
	
	/*  We subtract one because error messages are numbered 1.. and we
	 *  and we want to number 0.. when reporting.  We subtract another 1
	 *  to adjust for the extra line we insert in front of the code
	 *  (see ge_firstline in getline_from_nlines()).
	 */
	Error_lexinfo->lx_lnum -= 2;

	return TRUE;
}

machine_t *
recompile_code(parse_id, grp_arg)
parse_id_t parse_id;
char *grp_arg;
{
	ci_get_regaddr_proc_t get_regaddr_proc;
	linkinfo_t *li;
	machine_t *ma;
	unsigned compile_flags;

	get_regaddr_proc = grp_arg != NULL ? get_regaddr_for_ci : NULL;
	Func_addr_requested = FALSE;

	compile_flags = CI_CP_DONT_PANIC | CI_CP_CHECK_DIV;
	if (grp_arg != NULL)
		compile_flags |= CI_CP_WANT_REG_RELOCS;

	li = ci_compile("<internal code>", parse_id, report_error, checkarg,
			regno_to_addr, compile_flags);
	if (li == NULL)
		return NULL;
	
	ma = ci_make_machine_from_linkinfo(li, (const char *)NULL, 0,
					   getaddr, get_regaddr_proc, grp_arg,
					   report_error);
	
	ci_free_linkinfo(li);
	
	if (ma == NULL)
		return NULL;
	
	ci_push_trap_frame(ma);

	if (Debug_flags & DBFLAG_SHOW_CI_CODE)
		ci_disassemble_machine(parse_id, ma, stdout);
	
	return ma;
}

void
free_parse_id_and_machine(parse_id, ma)
parse_id_t parse_id;
machine_t *ma;
{
	if (ma != NULL)
		ci_free_machine(ma);
	if (parse_id != NULL)
		ci_free_parse_id(parse_id);
}

compile_res_t *
compile_code(lines, nlines, block, grp_arg, error_lx,
	     firstline, lastline, repl_text, repl_val)
const char **lines;
int nlines;
block_t *block;
char *grp_arg;
lexinfo_t *error_lx;
const char *firstline, *lastline;
const char *repl_text, *repl_val;
{
	static bool first_call = TRUE;
	static compile_res_t crbuf;
	getline_from_nlines_arg_t gebuf;

	if (first_call) {
		ci_add_message_action((char *)NULL, MA_WARNING_ONLY);
		ci_add_message_action("No prototype in scope", MA_IGNORE);
		ci_add_message_action("Implicit declaration of", MA_IGNORE);
		first_call = FALSE;
	}
	
	gebuf.ge_state = GS_BEFORE_LINES;
	gebuf.ge_firstline = firstline;
	gebuf.ge_lastline = lastline;
	gebuf.ge_repl_text = repl_text;
	gebuf.ge_repl_val = repl_val;
	gebuf.ge_lines = lines;
	gebuf.ge_nlines = nlines;
	gebuf.ge_lnum = 0;
	
	error_lx->lx_lnum = -1;
	Error_lexinfo = error_lx;

	crbuf.cr_parse_id = NULL;
	crbuf.cr_machine = NULL;

	crbuf.cr_parse_id = ci_parse_file("<internal file>", block,
					  CI_DONT_PANIC, report_error,
					  resolve_untyped_name,
					  getline_from_nlines, (char *)&gebuf);
	
	if (crbuf.cr_parse_id != NULL) {
		Func_addr_requested = FALSE;
		crbuf.cr_machine = recompile_code(crbuf.cr_parse_id, grp_arg);
		crbuf.cr_code_has_func_calls = Func_addr_requested;
	}

	return &crbuf;
}

static bool
checkarg(type, nargs, argn, p_val)
type_t *type;
int nargs, argn;
long *p_val;
{
	*p_val = (long)type;
	return TRUE;
}

static unsigned long
get_regaddr_for_ci(arg, reg)
char *arg;
int reg;
{
	/*  BUG: sizeof(taddr_t) is wrong here - we should pass the real size.
	 */
	return get_reg_addr(get_current_target(),
			    get_code((objid_t)arg, OBJ_PARENT), reg,
			    sizeof(taddr_t));
}

static ci_nametype_t
getaddr(name, p_addr)
const char *name;
taddr_t *p_addr;
{
	fil_t *junk_fil, *junk_fil1;
	common_block_t *junk_cblock;
	func_t *f, *f1;
	var_t *v, *v1;

	if (strcmp(name, "$set_expr_value") == 0) {
		*p_addr = (taddr_t)SET_EXPR_RESULT_ADDR;
		return CI_INDIRECT_LIBFUNC;
	}
	if (strcmp(name, "$st") == 0) {
		*p_addr = (taddr_t)STOP_ADDR;
		return CI_INDIRECT_LIBFUNC;
	}
	if (strcmp(name, "$printf") == 0) {
		*p_addr = (taddr_t)PRINTF_ADDR;
		return CI_INDIRECT_LIBFUNC;
	}

	if (find_global_by_name(name, get_displayed_fil(), (func_t *)NULL,
				(common_block_t *)NULL, TRUE,
				&f, &f1, &v, &v1,  &junk_cblock,
				&junk_fil, &junk_fil1, TRUE) != 0)
		return CI_UNDEFINED;

	if (f != NULL) {
		*p_addr = f->fu_addr;
		Func_addr_requested = TRUE;
		return CI_INDIRECT_LIBFUNC;
	}

	*p_addr = v->va_addr;
	return CI_DATA;
}

/*  Get the type of an object.  This is a wrapper round get_object_type()
 *  which maps OT_SFILE_EX to OT_SFILE, so source file objects all
 *  appear to have the same type regardless of whether they are
 *  expanded or not.
 */
int
ups_get_object_type(obj)
objid_t obj;
{
	int objtype;

	objtype = get_object_type(obj);
	
	switch (objtype) {
	case OT_SFILE_EX:
		return OT_SFILE;
	case OT_MODULE_EX:
		return OT_MODULE;
	default:
		return objtype;
	}
}

/*  If obj is not on the display, move the display so that it is central.
 */
void
ensure_visible(obj)
objid_t obj;
{
	int junk, obj_y;
	
	if (td_have_window() && !visible(obj)) {
		get_position(obj, &junk, &obj_y, &junk, &junk);
		obj_y -= wn_get_height(re_get_wn(get_display_area_region())) / 2;
		display_from(0, (obj_y < 0) ? 0 : obj_y);
	}
}

bool
change_field(obj, fnum, new)
objid_t obj;
int fnum;
const char *new;
{
	char *old;

	old = (char *)get_field_value(obj, fnum);

	if (strcmp(old, new) != 0) {
		set_field_value(obj, fnum, (fval_t)strsave(new));
		free(old);
		return TRUE;
	}
	
	return FALSE;
}

void
set_display_area_tbar(tbar_id)
tbar_id_t tbar_id;
{
	const char *button;
   
	Display_area_tbar = tbar_id;

	if ((button = wn_get_default("DisplayAreaScrollUpButton")) != NULL)
	{
	   re_set_button_callback(atoi(button), display_area_scroll_up, NULL);
	}

	if ((button = wn_get_default("DisplayAreaScrollDownButton")) != NULL)
	{
	   re_set_button_callback(atoi(button), display_area_scroll_down, NULL);
	}
}

static void
display_area_scroll_up(data, ev)
char *data;
event_t *ev;
{
	if (ev->ev_type & EV_BUTTON_DOWN)
	{
		int amount = 10;

		if (ev->ev_buttons & B_SHIFT_MASK)
			amount = amount * 10;
	   
		display_area_scroll(amount);
	}
}

static void
display_area_scroll_down(data, ev)
char *data;
event_t *ev;
{
	Srcwin *sw = (Srcwin *)data;

	if (ev->ev_type & EV_BUTTON_DOWN)
	{
		int amount = 10;
	   
		if (ev->ev_buttons & B_SHIFT_MASK)
			amount = amount * 10;

		display_area_scroll(-amount);
	}
}

/*  Scroll the object display by dist pixels and update the thumb bar.
 */
void
display_area_scroll(dist)
int dist;
{
	tb_scroll(Display_area_tbar, v_scroll(dist), FALSE);
}

/*  Move the object display.  Y is an offset from the start of the
 *  window containing the object scroll bar - the object display
 *  and thumb bar are updated to reflect this.
 */
void
display_area_goto(y)
int y;
{
	int disp_y, disp_wn;

	disp_wn = re_get_wn(get_display_area_region());
	disp_y = tb_tbpos_to_unit(Display_area_tbar, disp_wn, y, TRUE);
	display_from(0, disp_y);
	tb_goto(Display_area_tbar, disp_y, FALSE);
}

/*  Display a number.
 */
void
n_draw(dets)
struct drawst *dets;
{
	char buf[12];

	(void) sprintf(buf, "%d", (int) dets->dr_fval);
/* AMC enhancements */
	if (dets->selected)
	  wn_wd_ttext(dets->dr_wn, buf, dets->dr_x, dets->dr_y, dets->dr_width, dets->dr_bg, dets->dr_fg);
	else
	  wn_wd_ttext(dets->dr_wn, buf, dets->dr_x, dets->dr_y, dets->dr_width, dets->dr_fg, dets->dr_bg);
}

void
do_selection(display, ev)
Edit_display *display;
event_t *ev;
{
	selection_loop(display, ev,
			      (Edit_propchange *)NULL, (Edit_propchange *)NULL,
			      TRUE, FALSE /* set global selection, no text */
			      );
}

char *
selection_loop(display, ev, startpc, endpc, set_global_selection, want_text)
Edit_display *display;
event_t *ev;
Edit_propchange *startpc, *endpc;
bool set_global_selection;
bool want_text;
{
	size_t point = 0;
	size_t start_point = 0;
	Edit_propchange *movepc;
	Edit_buffer *buffer;
	bool had_cursor;
	int wn;

	wn = ev->ev_wn;
	buffer = edit_get_buffer(display);
	
	movepc = NULL;
	
	had_cursor = edit_set_want_cursor(display, FALSE);

        do {
		if (edit_pixel_to_point(display, ev->ev_x, ev->ev_y, &point)) {
			if (startpc == NULL) {
				start_point = point;
				add_propchange_pair(buffer, point, point,
					FLAG_HIGHLIGHT2, &startpc, &endpc);
			}
			else {
				edit_update_propchange_pair_pos(buffer, point,
								startpc, endpc,
								&movepc);
			}
		}
		
		/* RGA added EV_WINDOW_EXPOSED for CDE with 'click to focus' and 
		   'raise on focus' set. Then a click raises ups but it did not 
		   refresh itself because the expose events were not requested */
		wn_next_event(wn, EV_BUTTON_UP | EV_MOUSE_MOVED | EV_WINDOW_EXPOSED, 
			      ev);

	} while (ev->ev_type != EV_BUTTON_UP);

	edit_set_want_cursor(display, had_cursor);

	if (set_global_selection && (start_point != point))
	{
	    return edit_set_global_selection(display, startpc, endpc,
		ev->ev_time, want_text);
	} else
	{
	    return remove_propchange_pair(buffer, startpc, endpc);

	}
}

char *
remove_propchange_pair(buffer, startpc, endpc)
Edit_buffer *buffer;
Edit_propchange *startpc, *endpc;
{
	size_t start_point, end_point;
	char *text;

	if (startpc == NULL)
		return NULL;
	
	start_point = edit_get_propchange_point(startpc);
	end_point = edit_get_propchange_point(endpc);

	if (start_point == end_point) {
		text = NULL;
	}
	else  {
		text = edit_copy_bytes(buffer, start_point, end_point,
				       (char *)NULL);
	}
		
	edit_set_propchange_updates(buffer, FALSE);
	edit_delete_propchange(buffer, startpc);
	edit_delete_propchange(buffer, endpc);
	edit_set_propchange_updates(buffer, TRUE);

	return text;
}

void
add_propchange_pair(buffer, start, lim, flags, p_startpc, p_endpc)
Edit_buffer *buffer;
size_t start, lim;
Edit_flags flags;
Edit_propchange **p_startpc, **p_endpc;
{
	edit_set_propchange_updates(buffer, FALSE);
			
	*p_startpc = edit_add_propchange(buffer, start, (char *)NULL,
					 (Edit_fontinfo *)NULL,
					 flags, flags, TRUE);

	*p_endpc = edit_add_propchange(buffer, lim, (char *)NULL,
				       (Edit_fontinfo *)NULL,
				       flags, 0, FALSE);

	edit_set_propchange_updates(buffer, TRUE);
}

Edit_display *
make_text_display(wn, fg, bg, text)
int wn, fg, bg;
const char *text;
{
	static Edit_fontinfo *fi = NULL;
	static Edit_keymap *keymap;
	Edit_buffer *buffer;
	Text_buffer *tb;
	Edit_display *display;

	if (fi == NULL) {
		fi = edit_make_wn_fontinfo(wn_get_sysfont());
		keymap = edit_create_keymap(edit_get_default_keymap());
		edit_add_keymap_control_entry(keymap, 'j', edit_f_request_quit);
		edit_add_keymap_control_entry(keymap, 'm', edit_f_request_quit);
	}
		
	tb = text_create_simple_buffer();
	(*tb->ops->insert)(tb->bufdata, 0, text, strlen(text));
	buffer = edit_create_buffer(tb, fi, 0);
		
	display = edit_create_wn_display(wn, fg, bg, TRUE, TRUE);
	edit_use_single_line(display);
	edit_set_buffer(display, buffer);
	edit_set_keymap(display, keymap);

	edit_update_wn_window_size(display, wn);
	edit_display_from(display, 0, 0);
	edit_move_point(display, strlen(text));
	edit_set_want_cursor(display, TRUE);

	return display;
}

char *
get_buffer_contents(buffer)
Edit_buffer *buffer;
{
	return edit_copy_bytes(buffer, 0, edit_get_buffer_length(buffer),
			       (char *)NULL);
}

void
yre_fail(mesg, op)
const char *mesg;
int op;
{
	Internal_regex_error = TRUE;
}

bool
search_buffer(buffer, pattern, start_point, forwards, p_start, p_lim, file)
Edit_buffer *buffer;
const char *pattern;
size_t start_point;
bool forwards;
size_t *p_start, *p_lim;
bool file;
{
	Edit_lineinfo *li;
	const char *line;
	const char *mesg;
	int lstart, llim;
	bool found, have_eol;
	size_t linelen;
	
	if (buffer == NULL)
		return FALSE;

	if ((mesg = yre_comp(pattern)) != NULL) {
		errf("%s", mesg);
		return FALSE;
	}

	li = edit_make_lineinfo(buffer, '\n');
	edit_set_lineinfo_pos(li, start_point);

	Internal_regex_error = FALSE;

	found = FALSE;
	linelen = 0;
	have_eol = FALSE;
	
	for (;;) {
		char *linecopy;
		bool eof;
		
		if (forwards)
			eof = !edit_nextline(li, &line, &linelen, &have_eol);
		else
			eof = !edit_prevline(li, &line, &linelen, &have_eol);

		if (eof) {
			errf("Reached %s of %s without finding \"%s\"",
			     forwards ? "end" : "start",
			     file ? "file" : "display", pattern);
			break;
		}

		linecopy = edit_get_writable_copy(li, line, linelen);
		linecopy[linelen] = '\0';
		
		if (e_re_exec(linecopy, 0, &lstart, &llim)) {
			if (!forwards) {
				while (e_re_exec(linecopy, llim,
						 &lstart, &llim))
					;
			}

			found = TRUE;
			break;
		}
	}

	*p_start = edit_get_lineinfo_pos(li);

	if (forwards)
		*p_start -= (linelen + (have_eol ? 1 : 0));
	
	edit_free_lineinfo(li);
	
	if (Internal_regex_error) {
		errf("Internal regular expression error");
		*p_start = start_point;
		return FALSE;
	}

	if (found) {
		*p_start += lstart;
		*p_lim = *p_start + (llim - lstart);
	}

	return found;
}

void
show_func_source(sw, f)
Srcwin *sw;
func_t *f;
{
	lno_t *ln;
	
	if (f->fu_fil == NULL) {
		errf("Can't find source for function %s", f->fu_demangled_name);
		return;
	}

	ln = FU_LNOS(f);

	if (ln == NULL) {
		errf("No line number information for function %s", f->fu_demangled_name);
		return;
	}

	show_source(ln->ln_fil, ln->ln_num);
}


language_t
get_mainfunc_lang()
{
  return (xp_get_mainfunc(get_current_target()))->fu_language;
}
