/* obj_bpt.c - breakpoint object handling */

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


/* @(#)obj_bpt.c	1.42 24 May 1995 (UKC) */
char ups_obj_bpt_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <local/wn.h>
#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <local/obj/obj.h>
#include <local/edit/edit.h>

#include "objtypes.h"
#include "obj_util.h"
#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "ui.h"
#include "exec.h"
#include "target.h"
#include "breakpoint.h"
#include "st.h"
#include "data.h"
#include "obj_bpt.h"
#include "menudata.h"
#include "debug.h"
#include "tdr.h"
#include "config.h"
#include "srcbuf.h"
#include "reg.h"
#include "srcwin.h"
#include "state.h"
#include "util.h"
#include "expr.h"
#include "exec.h"
#include "obj_buildf.h"
#include "va.h"

typedef struct bpdescst {
	Editblock *editblock;
	fil_t *fil;
	int lnum;
	bool code_bad;
	bool data_needs_initialising;
	bool code_has_func_calls;
	bool want_save;
	bool bd_inactive;
	parse_id_t parse_id;
	machine_t *machine;

	func_t *func;
	breakpoint_t *breakpoint;
} bpdesc_t;

typedef struct {
	FILE *ba_fp;
	char *ba_path;
} bptargs_t;

#define HAS_CODE(bd)	((bd)->parse_id != NULL || (bd)->code_bad)

static void bpfunc_draw PROTO((struct drawst *dets));
static void bpstate_draw PROTO((struct drawst *dets)); /* RCB */
static int move_bpt PROTO((objid_t obj, char *new_text, int edit_lnum,
			   const char **p_display_string));
static void show_bpt_source PROTO((bpdesc_t *bd, bool beep_on_failure));
static void set_bpt_state PROTO((objid_t obj, bool disable, bool verbose));
static void enable_all_breakpoints PROTO(( bool enable));
static void bpfunc_edit PROTO((struct drawst fdets));
static void bplnum_edit PROTO((struct drawst fdets));
static char *build_source_text PROTO((fil_t *fil, int lnum, const char *text));
static void update_editblock PROTO((bpdesc_t *bd, bool is_bpt, char *text, bool show));
static int set_initialise_data_flag PROTO((objid_t obj, fval_t unused_arg));
static int read_data PROTO((taddr_t addr, void *buf, size_t nbytes));
static int write_data PROTO((taddr_t addr, const void *buf, size_t nbytes));

static taddr_t get_bpt_addr PROTO((objid_t obj));
static int save_object_to_file PROTO((const char *path, FILE *fp,
					   bpdesc_t *bd));
static int get_bp_lnum PROTO((func_t *f, const char *lnumstr,
			      const char *offsetstr));
static bool get_code_text PROTO((FILE *fp, int *p_lnum, ebuf_t *eb));
static void load_breakpoints PROTO((bool from_statefile));
static int skip_to_close_curly PROTO(( FILE *fp, int *p_lnum));
static void handle_editblock_action PROTO((Editblock *editblock, 
                                           Editblock_action action));
static bool handle_editblock_change PROTO((Editblock *eb, const char *text,
					   bool force, size_t *p_error_point));
static bool handle_editblock_change_internal PROTO((Editblock *eb, const char *text,
					   bool force, size_t *p_error_point));
static int bps_accelerator_action PROTO(( void ));
static bool bpfunc_quitfunc PROTO((objid_t obj, char *new_text, 
                                   const char **p_display_string));
static bool bplnum_quitfunc PROTO((objid_t obj, char *new_text, 
                                   const char **p_display_string));
static bool bpstate_quitfunc PROTO((objid_t obj, char *new_text, 
				    const char **p_display_string));
static bool fil_and_lnum_to_bdobj PROTO((fil_t *fil, int lnum, objid_t *p_obj));
static bpdesc_t *make_bd PROTO((fil_t *fil, int lnum, func_t *func));
static void save_interpreted_code_to_file PROTO((const char *path, FILE *fp, 
                                                 bpdesc_t *bd));
static void save_breakpoint_to_file PROTO((const char *path, FILE *fp, 
                                           bpdesc_t *bd));
static bool execute_bp_code_internal PROTO((breakpoint_t *bp,
					    taddr_t fp, taddr_t ap,
					    int button));
static void set_bpt_location PROTO((fval_t *fields, func_t *f, int lnum));

/*  Maximum length of a function name. If you change this you must also
 *  change the Bpt_format string to match.
 */
#define LEN_BP_FNAME 	61

/* RCB: Changed, changed to use bpstate_draw */
/* RGA bpt name resizes, obj name does too when there is enough space for
   the bpt name. See bpfunc_draw(). */
const char Bpt_format[] = "%8cs:%[-]:8c:64cb  %[F]:8c:64cf%1cs%[-]10cn%[-]9cq   \n";

/*  Field numbers for breakpoints
 */
#define FN_BPT_TYPESTR	0
#define FN_BPT_FNAME	1
#define FN_BPT_FILE	 2
#define FN_BPT_SEPARATOR 3
#define FN_BPT_LNUM	 4
#define FN_BPT_STATE     5
#define FN_BPT_LAST	 6

/*  Flist terminator for breakpoint fields for set_all_fields().
 *  Can't use NULL as one of the fields is an int which can legitimately
 *  have the value 0.
 */
#define BPT_FLIST_TERMINATOR	((fval_t)-1)

Edit_history* bpt_history = NULL;

fnamemap_t Bpt_fnamemap[] = {
	{ FN_BPT_FNAME,	"function-name",	FALSE,	bpfunc_quitfunc, &bpt_history	},
	{ FN_BPT_FILE,	"filename",		FALSE,	bpfunc_quitfunc	},
	{ FN_BPT_LNUM,	"lnum",			FALSE,	bplnum_quitfunc	},
	{ FN_BPT_STATE,	"state",		FALSE,	bpstate_quitfunc},
	{ 0,		NULL,			FALSE,	NULL		},
};

const char Bphead_format[] = "Breakpoints\n";

#define BPHEAD_OBJCODE	((objid_t)Bphead_format)

static const char Stop_keyword[] = "#stop";

fdef_t Bpt_fdefs[] = {
	{ 'b', bpfunc_draw, bpfunc_edit, NULL	},
	{ 'f', bpfunc_draw, no_edit, NULL	},
	{ 'n', bpfunc_draw, bplnum_edit, NULL	},
	{ 'q', bpstate_draw, no_edit, NULL },
	{ 0, NULL, NULL, NULL			},
};

static const char *Statefile_path = NULL;
static bool Breakpoint_header_exists = FALSE; 

void
set_breakpoint_statefile_path(path)
const char *path;
{
	Statefile_path = path;
}

static void
load_breakpoints(from_statefile)
bool from_statefile;
{
	static char *lastpath = NULL;
	char *path;
	bool junk;

	if (lastpath == NULL)
		lastpath = strsave("");
	
	if (prompt_for_string("filename", "Load breakpoints from file: ",
			      lastpath, &path) != 0)
		return;

	if (strcmp(path, lastpath) != 0) {
		free(lastpath);
		lastpath = strsave(path);
	}

	read_config_file(path, from_statefile, TRUE, TRUE, FALSE, &junk, FALSE);
}

static bool
fil_and_lnum_to_bdobj(fil, lnum, p_obj)
fil_t *fil;
int lnum;
objid_t *p_obj;
{
	objid_t obj;

	for (obj = get_code(BPHEAD_OBJCODE, OBJ_CHILD);
	     obj != NULL;
	     obj = get_code(obj, OBJ_NEXT)) {
		bpdesc_t *bd;

		bd = (bpdesc_t *)obj;
		
		if (bd->fil == fil && bd->lnum == lnum) {
			*p_obj = obj;
			return TRUE;
		}
	}

	return FALSE;
}

int
save_all_breakpoints_to_file(path, fp)
const char *path;
FILE *fp;
{
	objid_t obj;

	for (obj = get_code(BPHEAD_OBJCODE, OBJ_CHILD);
	     obj != NULL;
	     obj = get_code(obj, OBJ_NEXT)) {
		bpdesc_t *bd;

		bd = (bpdesc_t *)obj;
		
		if (bd->want_save && save_object_to_file(path, fp, bd) != 0) {
			return -1;
		}
	}

	return 0;
}

static int
save_object_to_file(path, fp, bd)
const char *path;
FILE *fp;
bpdesc_t *bd;
{
	if (bd->breakpoint != NULL)
		save_breakpoint_to_file(path, fp, bd);
	else
		save_interpreted_code_to_file(path, fp, bd);

	fflush(fp);

	if (ferror(fp)) {
		failmesg("Error saving object to", "file", path);
		return -1;
	}

	return 0;
}

static void
save_interpreted_code_to_file(path, fp, bd)
const char *path;
FILE *fp;
bpdesc_t *bd;
{
	fprintf(fp, "code %s %d {\n", bd->fil->fi_name, bd->lnum);
	dump_text(srcbuf_get_editblock_text(bd->editblock), fp);
	fputs("}\n", fp);
}

static void
save_breakpoint_to_file(path, fp, bd)
const char *path;
FILE *fp;
bpdesc_t *bd;
{
	func_t *f;

	f = bd->func;

	/* RGA enclose name in double quotes, to allow for spaces
	   in C++ names.
	   3.37: use unquoted mangled name so overloaded methods are unique,
	   followed by demangled name in quotes for readability
	   */
	fprintf(fp, "breakpoint %s \"%s\"", f->fu_name, f->fu_demangled_name);
	
	if (f->fu_fil != NULL) {
		lno_t *ln;
		int n_code_lines;

		n_code_lines = 0;
		for (ln = FU_LNOS(f); ln != NULL; ln = ln->ln_next) {
			if (ln->ln_num == bd->lnum)
				break;
			++n_code_lines;
		}
		
		fprintf(fp, " %s %d [+%d] %s",
			f->fu_fil->fi_name, bd->lnum, n_code_lines,
			bd->bd_inactive ? "inactive" : "active");
	}
	else
		fprintf(fp, " %s",
			bd->bd_inactive ? "inactive" : "active");
	
	if (HAS_CODE(bd)) {
		fputs(" {\n", fp);
		dump_text(srcbuf_get_editblock_text(bd->editblock), fp);
		fputc('}', fp);
	}

	fputc('\n', fp);
}

int
handle_add_breakpoint_command(cmd, args, nargs, from_statefile, ignore,
			      fp, p_lnum)
const char *cmd;
char **args;
int nargs;
bool from_statefile, ignore;
FILE *fp;
int *p_lnum;
{
	func_t *f, *f1;
	char *funcname;
	const char *filename, *lnumstr, *offsetstr, *activationstr;
	bool ok, have_code;
	fil_t *fil;
	int lnum;
	objid_t obj;
#ifdef OS_SUNOS
	objid_t obj1 = NULL;
#endif /* OS_SUNOS */

	have_code = nargs > 1 && strcmp(args[nargs - 1], "{") == 0;

	if (have_code)
		--nargs;
	/* RGA: note demangled name is not in arg list */
	if (nargs < 1 || nargs > 5) {
		errf("Usage: breakpoint mangled-funcname \"funcname\" [filename [lnum [offset]] active|inactive] [{]");
		return -1;
	}

	funcname = args[0];
	/* RGA for backward compatibility strip off leading and trailing quotes
	   These will only appear when reading older stle breakpoints where
	   the function name is demangled and quoted and there is no preceding
	   mangled function name. */
	if (funcname[0] == '"' && funcname[strlen(funcname) - 1] == '"')
	{
	  funcname[strlen(funcname) - 1] = 0;
	  funcname++;
	}
	filename = (nargs > 1) ? args[1] : NULL;
	lnumstr = (nargs > 2) ? args[2] : NULL;
	offsetstr = (nargs > 3) ? args[3] : NULL;
	activationstr = (nargs > 4) ? args[4] : NULL;
	if (activationstr == NULL && nargs == 2) /* no file */
	  activationstr = args[1];;
	
	if (filename == NULL) {
		if (find_func_by_name(funcname, &f, &f1, FALSE) != 0) {
			skip_to_close_curly(fp, p_lnum);
			return -1;
		}

		fil = f->fu_fil;
		lnum = -1;
	}
	else {
		if ((fil = name_to_fil(filename)) == NULL ||
		    (f = name_and_fil_to_func(funcname, fil, FALSE)) == NULL) {
			/*  Function may have moved to another file.
			 */
			if (find_func_by_name(funcname, &f, &f1, FALSE) != 0) {
				skip_to_close_curly(fp, p_lnum);
				return -1;
			}
			fil = f->fu_fil;
		}

		if (lnumstr == NULL)
			lnum = -1;
		else
			lnum = get_bp_lnum(f, lnumstr, offsetstr);
	}

	if (ignore) {
		if (have_code && skip_to_close_curly(fp, p_lnum) != 0)
			return -1;
		return 0;
	}

	if (from_statefile && fil_and_lnum_to_bdobj(f->fu_fil, lnum, &obj))
		remove_object(obj, OBJ_SELF);
	
	if ((obj = add_breakpoint_object(f, fil, lnum, FALSE)) == NULL) {
	  /* RGA try again without a file. Bpts like purify_stop_here
	     can have bogus files */
	  if ((obj = add_breakpoint_object(f, 0, -1, FALSE)) == NULL) {
	    if (have_code)
	      skip_to_close_curly(fp, p_lnum);
	    return -1;
	  }
	}
	if (activationstr == NULL)
	  set_bpt_state(obj, FALSE, FALSE); /* default is active */
	else
	  set_bpt_state(obj, strcmp(activationstr, "active"), FALSE);

#ifdef OS_SUNOS
	if (fil && fil->fi_flags & FI_DUPLICATE)
	{
	  fil_t *fil1;
				    
	  if (find_duplicate_file(fil, &fil1))
	  {
	    f1 = lnum_to_func(fil1, lnum);
	    if ((obj1 = add_breakpoint_object(f1, fil1, lnum, FALSE)) == NULL) {
	      if (have_code)
		skip_to_close_curly(fp, p_lnum);
	      return -1;
	    }
	    set_bpt_state(obj1, strcmp(activationstr, "active"), FALSE);
	  }
	}
#endif /* OS_SUNOS */

	((bpdesc_t *)obj)->want_save = from_statefile;

	if (have_code) {
		ebuf_t *eb;
		
		eb = ebuf_create(TRUE);

		ok = get_code_text(fp, p_lnum, eb);
		if (ok)
		{
		  change_bd_text(obj, ebuf_get(eb, (int *)NULL));
#ifdef OS_SUNOS
		  if (obj1)
		    change_bd_text(obj1, ebuf_get(eb, (int *)NULL));
#endif /* OS_SUNOS */
		}
		if (!ok)
			remove_object(obj, OBJ_SELF);

		ebuf_free(eb);
	}
	else {
		ok = TRUE;
	}

	return ok ? 0 : -1;
}

int
handle_add_code_command(cmd, args, nargs, from_statefile, ignore, fp, p_lnum)
const char *cmd;
char **args;
int nargs;
bool from_statefile, ignore;
FILE *fp;
int *p_lnum;
{
	fil_t *fil;
	const char *filename, *lnumstr;
	int lnum;
	objid_t obj;
	ebuf_t *eb;
	char *text, *ends;
	
	if (nargs != 3 || strcmp(args[2], "{") != 0) {
		errf("Usage: code filename lnum {");
		return -1;
	}

	filename = args[0];
	lnumstr = args[1];
	
	if ((fil = name_to_fil(filename)) == NULL) {
		skip_to_close_curly(fp, p_lnum);
		return -1;
	}
	
	lnum = strtol(lnumstr, &ends, 10);

	if (ends == lnumstr || *ends != '\0') {
		errf("Line number `%s' is not a decimal integer", lnumstr);
		skip_to_close_curly(fp, p_lnum);
		return -1;
	}

	if (ignore)
		return skip_to_close_curly(fp, p_lnum);

	eb = ebuf_create(TRUE);
	
	if (!get_code_text(fp, p_lnum, eb)) {
		ebuf_free(eb);
		skip_to_close_curly(fp, p_lnum);
		return -1;
	}
	text = ebuf_get(eb, (int *)NULL);
	
	if (from_statefile && fil_and_lnum_to_bdobj(fil, lnum, &obj))
		remove_object(obj, OBJ_SELF);

	obj = add_interpreted_code(fil, lnum, strsave(text));
	ebuf_free(eb);
	
	if (obj != NULL)
		((bpdesc_t *)obj)->want_save = from_statefile;

	return (obj != NULL) ? 0 : -1;
}

static int
skip_to_close_curly(fp, p_lnum)
FILE *fp;
int *p_lnum;
{
	char *line;

	while ((line = fpgetline(fp)) != NULL) {
		if (strcmp(line, "}") == 0)
			return 0;
		++*p_lnum;
	}

	errf("Missing closing `}' in breakpoint code");
	return -1;
}

static bool
get_code_text(fp, p_lnum, eb)
FILE *fp;
int *p_lnum;
ebuf_t *eb;
{
	int code_lnum;
	char *line;

	code_lnum = *p_lnum;

	while ((line = fpgetline(fp)) != NULL) {

		++*p_lnum;

		if (strcmp(line, "}") == 0) {
			ebuf_add(eb, "", 1);
			return TRUE;
		}
		
		if (*line == '#')
			continue;
		if (*line == '\t')
			++line;

		ebuf_addstr(eb, line);
		ebuf_add(eb, "\n", 1);
	}

	*p_lnum = code_lnum;
	errf("Missing `}'");
	return FALSE;
}

static int
get_bp_lnum(f, lnumstr, offsetstr)
func_t *f;
const char *lnumstr, *offsetstr;
{
	int lnum;
	char *ends;
			
	lnum = strtol(lnumstr, &ends, 10);
	
	if (lnum < 0 || ends == lnumstr || *ends != '\0') {
		errf("Ignored illegal line number `%s'", lnumstr);
		lnum = -1;
	}

	if (offsetstr != NULL) {
		lno_t *ln;
		int i, count;
		char junkc;

		if (sscanf(offsetstr, "[+%d]%c", &count, &junkc) != 1)
			errf("Ignored malformed offset `%s'", offsetstr);
				
		ln = FU_LNOS(f);
		for (i = 0; i < count && ln != NULL; ++i)
			ln = ln->ln_next;

		if (ln != NULL)
			lnum = ln->ln_num;
		else
			errf("Ignored out of range offset `%s'", offsetstr);
	}

	return lnum;
}

const char *
bpt_getobjname(obj)
objid_t obj;
{
	static char *last = NULL;
	bpdesc_t *bd;

	bd = (bpdesc_t *)obj;

	if (bd->fil == NULL)
		return "[BLANK]";
	
	if (last != NULL)
		free(last);

	if (bd->breakpoint != NULL)
		last = strf("%s:%d", bd->func->fu_demangled_name, bd->lnum);
	else
		last = strf("file:%s:%d", bd->fil->fi_name, bd->lnum);

	return last;
}

static void
bpfunc_draw(dets)
register struct drawst *dets;
{
	int x, len, maxlen, width;
	bool too_long;
	char *s;
	font_t *font;
	
	font = wn_get_sysfont();

	s = (char *)dets->dr_fval;

	if (dets->dr_user_info != NULL && dets->dr_user_info[0] == 'F') {
		char *slash = strrchr( s, '/' );
		
		s = slash ? slash + 1 : s;
	}
	
	len = strlen(s);
	maxlen = dets->dr_width / font->ft_width;

	width = dets->dr_width;
	too_long = len > maxlen;
	if (too_long) {
		len = maxlen;
		width -= 3 * font->ft_width;
	}

	x = dets->dr_x;
	if (dets->dr_user_info == NULL || dets->dr_user_info[0] != '-')
		x += dets->dr_width - len * font->ft_width;

	if (dets->selected)
	    wn_wd_ttext(dets->dr_wn, s, x, dets->dr_y, width,
			dets->dr_bg, dets->dr_fg);
	else
	    wn_wd_ttext(dets->dr_wn, s, x, dets->dr_y, width,
			dets->dr_fg, dets->dr_bg);

	if (too_long) {
		static const char dots[] = "...";
		int inc;

		width = 3 * font->ft_width;
		inc = len - sizeof(dots) + 1;
		s += inc;
		x += inc * font->ft_width;
		if (*s == '/' && s[1] != '/') {
			++s;
			x += font->ft_width;
		}
/* AMC enhancements */
	if (dets->selected)
	    wn_wd_ttext(dets->dr_wn, dots, x, dets->dr_y,
			      width, dets->dr_bg, dets->dr_fg);
	  else
	    wn_wd_ttext(dets->dr_wn, dots, x, dets->dr_y,
			      width, dets->dr_fg, dets->dr_bg);
	}

/* AMC enhancements */
}

static void
bpstate_draw(dets)
register struct drawst *dets;
{
	long fg;
	static color_done = -1;
	static color_t color_fg;
	int bp_enabled, use_alloc_color;

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

	bp_enabled = global_bp_enabled(0,0);
	use_alloc_color = wn_use_alloc_color(0);

	if (!bp_enabled && use_alloc_color)
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
	if (!bp_enabled && !use_alloc_color) {
		wn_shade_area(  dets->dr_wn
			      , dets->dr_x, dets->dr_y
			      , dets->dr_width, dets->dr_depth
			      , WN_GREY12, R_AND);
	}
}


static taddr_t
get_bpt_addr(obj)
objid_t obj;
{
	bpdesc_t *bd;
	taddr_t addr;

	bd = (bpdesc_t *)obj;
	
	if (bd->lnum != 0) {
		addr = lnum_and_fil_to_addr(bd->func, bd->fil, bd->lnum);
	}
	else {
		if (get_min_bpt_addr(bd->func, &addr, FALSE) != 0)
			panic("gmba failed in gba");
	}

	return addr;
}

/*  Build a source line for a breakpoint.  We duplicate any whitespace
 *  at the beginning of the source line following the breakpoint.
 */
static char *
build_source_text(fil, lnum, text)
fil_t *fil;
int lnum;
const char *text;
{
	size_t textsize, point, start, lim;
	char *obuf, *optr;
	Edit_buffer *buffer;
	
	if (!open_source_file(fil, FALSE, FALSE) ||
	    !srcbuf_file_lnum_to_point_range(fil->fi_srcbuf, lnum - 1,
					     &start, &lim)) {
		return strsave(text);
	}

	/*  Worst case is that the whole line is whitespace.
	 */
	textsize = strlen(text) + 1;
	optr = obuf = e_malloc((lim - start) + textsize);

	buffer = srcbuf_get_buffer(fil->fi_srcbuf);
	
	if (edit_find_char_forwards(buffer, start, lim, "! \t", &point)) {
		edit_copy_bytes(buffer, start, point, optr);
		optr += point - start;
	}
	
	memcpy(optr, text, textsize);
	return obuf;
}

void
recalculate_bpt_addrs(xp)
target_t *xp;
{
	objid_t obj;
	bpdesc_t *bd;

	if (Breakpoint_header_exists == FALSE)
	  return;		/* RGA */
	obj = get_code(BPHEAD_OBJCODE, OBJ_CHILD);
	for (; obj != NULL; obj = get_code(obj, OBJ_NEXT)) {
		bd = (bpdesc_t *)obj;
		if (bd->breakpoint != NULL) {
			change_breakpoint_address(xp, bd->breakpoint,
						  get_bpt_addr(obj));
			set_breakpoint_data(bd->breakpoint, (long)obj);
		}

		if (bd->code_has_func_calls && bd->machine && bd->parse_id) {
		  /* RGA protect against bad bpt code loaded from a bpt file */
			ci_free_machine(bd->machine);
			bd->machine = recompile_code(bd->parse_id,
							(char *)NULL);
		}
	}
}

bool
change_bd_text(obj, text)
objid_t obj;
const char *text;
{
	bpdesc_t *bd;
	size_t junk;

	bd = (bpdesc_t *)obj;
	
	update_editblock(bd, bd->breakpoint != NULL, strsave(text), TRUE);
	return handle_editblock_change_internal(bd->editblock, text, FALSE, &junk);
}

static bool
handle_editblock_change(eb, text, force, p_error_pos)
Editblock *eb;
const char *text;
bool force;
size_t *p_error_pos;
{
#ifdef OS_SUNOS
  bpdesc_t *bd;
#endif /* OS_SUNOS */
  bool ret;

  ret = handle_editblock_change_internal(eb, text, force, p_error_pos);

#ifdef OS_SUNOS
  bd = (bpdesc_t *)srcbuf_get_editblock_data(eb);
  if (!bd->code_bad)
  {
    fil_t *fil1;
    objid_t obj1 = NULL;
    
    if (bd->func->fu_fil && bd->func->fu_fil->fi_flags & FI_DUPLICATE)
      if (find_duplicate_file(bd->fil, &fil1) && fil1->fi_srcbuf)
      {
	obj1 = (objid_t)srcbuf_get_editblock_in_tracking_file
	  (bd->editblock, bd->fil->fi_srcbuf, fil1->fi_srcbuf);
	if (obj1)
	  change_bd_text(obj1, text);
      }
  }
#endif /* OS_SUNOS */
  return ret;
}

static bool
handle_editblock_change_internal(eb, text, force, p_error_pos)
Editblock *eb;
const char *text;
bool force;
size_t *p_error_pos;
{
	bpdesc_t *bd;
	compile_res_t *cr;
	block_t *block;
	char **lines;
	const char *start_text, *end_text;
	int nlines;
	lexinfo_t lxbuf;
	char *expansion;
			      
	bd = (bpdesc_t *)srcbuf_get_editblock_data(eb);
	
	td_record_bpt_code_edit((objid_t)bd, text);

	if (bd->func == NULL) {
		block = bd->fil->fi_block;
		start_text = "";
		end_text = "";
	}
	else {
		block = FU_BLOCKS(bd->func);
		while (block != NULL) {
			block_t *child;

			for (child = block->bl_blocks;
			     child != NULL;
			     child = child->bl_next) {
				if (child->bl_start_lnum <= bd->lnum &&
				    bd->lnum <= child->bl_end_lnum)
					break;
			}
		
			if (child == NULL)
				break;
			block = child;
		}

		if (block == NULL)
			block = bd->fil->fi_block;

		start_text = "void $start(void) {";
		end_text = "}";
	}
	
	/*  Make sure block vars and type information of fil are loaded.
	 */
	FI_VARS(bd->fil);
	
	if (bd->parse_id != NULL)
		ci_free_parse_id(bd->parse_id);
	
	if (bd->machine != NULL) {
		ci_free_machine(bd->machine);
		bd->machine = NULL;
	}

	if ((expansion = macro_expand_string(bd->fil, bd->lnum, text)) != NULL)
		lines = ssplit(expansion, "\0\n");
	else
		lines = ssplit(text, "\0\n");

	for (nlines = 0; lines[nlines] != NULL; ++nlines)
		;
	
	cr = compile_code((const char **)lines, nlines, block, (char *)NULL,
					&lxbuf, start_text, end_text,
					Stop_keyword, Stop_funcname);

	if (expansion)
		free(expansion);

	bd->parse_id = cr->cr_parse_id;
	bd->machine = cr->cr_machine;
	bd->code_bad = cr->cr_machine == NULL || cr->cr_parse_id == NULL;
	bd->code_has_func_calls = cr->cr_code_has_func_calls;
	bd->want_save = TRUE;

	if (bd->code_bad) {
		int lnum;

		*p_error_pos = lxbuf.lx_cnum;

		for (lnum = 0; lnum < nlines && lnum < lxbuf.lx_lnum; ++lnum)
			*p_error_pos += strlen(lines[lnum]) + 1;

		/*  BUG: the following stuff should not really be necessary,
		 *       but I don't have time right now to investigate.
 		 *       RGA Added < 0 test to stop crashes on bad 
                 *       breakpoint code.
		 */
		if (*p_error_pos >= strlen(text) || (int)*p_error_pos < 0) {
			*p_error_pos = strlen(text);

			if (*p_error_pos != 0)
				--*p_error_pos;
		}

		return force;
	}
	
	return TRUE;
}

/*  Select or deselect a breakpoint object.  This function is registered
 *  as a callback for breakpoint editblock objects.
 */
static void
handle_editblock_action(editblock, action)
Editblock *editblock;
Editblock_action action;
{
	bpdesc_t *bd;

	bd = (bpdesc_t *)srcbuf_get_editblock_data(editblock);
	
	switch (action) {
	case EDL_SELECT:
		clear_selection();
		select_object((objid_t)bd, TRUE, OBJ_SELF);
		ensure_visible((objid_t)bd);
		break;
	case EDL_DESELECT:
		select_object((objid_t)bd, FALSE, OBJ_SELF);
		break;
	case EDL_REMOVE:
		bd->editblock = NULL;
		remove_object((objid_t)bd, OBJ_SELF);
		break;
	case EDL_ACCELERATOR_ACTION: /* RCB */
		do_bpt((objid_t)bd, bpt_accelerator_action((objid_t)bd), NULL);
		break;
	default:
		panic("unknown action in hea");
	}
}

int
bpt_accelerator_action(obj )
objid_t obj;
{
   /*
   **   Consult with the X resources to find out
   **   what the user likes to see for an accelerator
   **   when a breakpoint is double clicked on.
   **   Remove it, or toggel its state?
   */
   static enum { BPQ_FIRSTTIME, BPQ_REMOVE, BPQ_TOGGLE } what = BPQ_FIRSTTIME;
   int command;
   if ( what == BPQ_FIRSTTIME )
   {
       /*
       **  First time here.  Consult X Resources to so what
       **  the user wants to do on a quick change ( Left-Click or double-Click)
       */
       const char* res = wn_get_default("BreakPointAcceleratorAction");

       what = BPQ_TOGGLE;  /* Assume default */
       if ( res )
       {
	  if (   !strcmp(res,"Toggle") )
	     what = BPQ_TOGGLE;
	  else if (   !strcmp(res,"Remove") )
	     what = BPQ_REMOVE;
       }
   }
   if ( what == BPQ_REMOVE )
   {
	command = MR_BPTMEN_REMOVE;
   } else
   {
	bpdesc_t* bd = (bpdesc_t *)obj;
	if ( bd->bd_inactive )
	   command = MR_BPTMEN_ACTIVE;
	else
	   command = MR_BPTMEN_INACTIVE;

   }
   return command;
}

static int
bps_accelerator_action()
{
   /*
   **   Consult with the X resources to find out
   **   what the user likes to see for an accelerator
   **   when the breakpoints header double clicked on.
   **   Add a new breakpoint or toggle active/inactive
   */
   static enum { BPHQ_FIRSTTIME, BPHQ_ADD, BPHQ_TOGGLE } what = BPHQ_FIRSTTIME;
   int command;
   if ( what == BPHQ_FIRSTTIME )
   {
       /*
       **  First time here.  Consult X Resources to so what
       **  the user wants to do on a quick change ( Left-Click or double-Click)
       */
       const char* res = wn_get_default("BreakPointHeaderAcceleratorAction");
       if ( res == NULL )	/* Temp: Try old name */
	   res = wn_get_default("BreakPointHeaderQuickChange");
       what = BPHQ_TOGGLE;  /* Assume default */
       if ( res )
       {
	  if (   !strcmp(res,"Toggle") )
	     what = BPHQ_TOGGLE;
	  else if (   !strcmp(res,"Add") )
	     what = BPHQ_ADD;
       }
   }
   if ( what == BPHQ_ADD )
   {
	command = MR_ADD_BREAKPOINT;
   } else
   {
	if ( global_bp_enabled(0,0) )
	   command = MR_DISABLE_ALL_BREAKPOINTS;
	else
	   command = MR_ENABLE_ALL_BREAKPOINTS;

   }
   return command;
}


/*  Make a new breakpoint object.
 */
objid_t
add_breakpoint_object(f, fil, lnum, show)
func_t *f;
fil_t *fil;
int lnum;
bool show;
{
	fval_t fields[FN_BPT_LAST + 1];
	bpdesc_t *bd;
	const char *fname;
	taddr_t addr;
	target_t *xp;

	xp = get_current_target();
		
	if (f == NULL) {
		fil = NULL;
	}
	else {
		if (lnum == -1) {
			if (get_min_bpt_addr(f, &addr, fil != NULL) != 0)
				return NULL;
		}
		else {
			if (map_lnum_to_addr(f, fil, lnum, &addr) != 0)
				return NULL;
		}

		if (xp_addr_to_breakpoint(xp, addr) != 0) {
		  if (!(fil && fil->fi_flags & FI_DUPLICATE))
		    errf("There is already a breakpoint at line %d of %s",
			 lnum, f->fu_demangled_name);
			return NULL;
		}

		addr_to_fil_and_lnum(f, addr, &fil, &lnum, FALSE);
	}

	fields[FN_BPT_TYPESTR] = "function";
	set_bpt_location(fields, f, lnum);
	fields[FN_BPT_STATE] = (fval_t)"active";
	fields[FN_BPT_LAST] = BPT_FLIST_TERMINATOR;

	bd = make_bd(fil, lnum, f);
	
	display_area_overlay_unselect_global_selection();
	td_set_obj_updating(OBJ_UPDATING_OFF);
	new_object((objid_t)bd, OT_BPT, BPHEAD_OBJCODE, OBJ_LAST_CHILD);
	set_all_fields((objid_t)bd, fields, BPT_FLIST_TERMINATOR);
	td_set_obj_updating(OBJ_UPDATING_ON);
	ensure_visible((objid_t)bd);

	if (bd->func != NULL) {
		bd->breakpoint = xp_add_breakpoint(xp, addr);
		set_breakpoint_data(bd->breakpoint, (long)bd);
		update_editblock(bd, TRUE, (char *)NULL, show);
	}
	else {
		edit_field_obj((objid_t)bd, FN_BPT_FNAME);
	}

	return (objid_t)bd;
}

static bpdesc_t *
make_bd(fil, lnum, func)
fil_t *fil;
int lnum;
func_t *func;
{
	bpdesc_t *bd;
	
	bd = (bpdesc_t *)e_malloc(sizeof(bpdesc_t));
	bd->breakpoint = NULL;
	bd->fil = fil;
	bd->func = func;
	bd->lnum = lnum;
	bd->editblock = NULL;
	bd->parse_id = NULL;
	bd->machine = NULL;
	bd->code_bad = FALSE;
	bd->data_needs_initialising = FALSE;
	bd->code_has_func_calls = FALSE;
	bd->want_save = TRUE;
	bd->bd_inactive = FALSE;

	return bd;
}
		     
objid_t
add_interpreted_code(fil, lnum, text)
fil_t *fil;
int lnum;
char *text;
{
	objid_t obj;
	fval_t fields[FN_BPT_LAST + 1];
	bpdesc_t *bd;
	func_t *f;
	taddr_t addr;

	if ((f = lnum_to_func(fil, lnum)) != NULL &&
	    (addr = lnum_and_fil_to_addr(f, fil, lnum)) != 1 && addr != 0) {
		errf("There is executable code at line %d of %s",
		     lnum, fil->fi_name);
		return NULL;
	}
	
	if (fil_and_lnum_to_bdobj(fil, lnum, &obj)) {
		bd = (bpdesc_t *)obj;
		
		errf("There is already %s at line %d of %s",
		     (bd->breakpoint != NULL) ? "a breakpoint"
		     			      : "interpreted code",
		     lnum, fil->fi_name);
		return NULL;
	}

	
	fields[FN_BPT_TYPESTR] = "file";
	fields[FN_BPT_FNAME] = fil->fi_name;
	fields[FN_BPT_FILE] = fil->fi_name;
	fields[FN_BPT_SEPARATOR] = ":";
	fields[FN_BPT_LNUM] = strf("%d", lnum);
	fields[FN_BPT_LAST] = BPT_FLIST_TERMINATOR;

	bd = make_bd(fil, lnum, (func_t *)NULL);
	
	td_set_obj_updating(OBJ_UPDATING_OFF);
	new_object((objid_t)bd, OT_BPT, BPHEAD_OBJCODE, OBJ_LAST_CHILD);
	set_all_fields((objid_t)bd, fields, BPT_FLIST_TERMINATOR);
	td_set_obj_updating(OBJ_UPDATING_ON);
	ensure_visible((objid_t)bd);

	update_editblock(bd, FALSE, text, TRUE);

	return (objid_t)bd;
}

/*  Update the editblock for code or a breakpoint after a change of status
 *  or position.  If necessary, create the editblock.
 */
static void
update_editblock(bd, is_bpt, text, show)
bpdesc_t *bd;
bool is_bpt;
char *text;
bool show;
{
	Editblock *editblock;
	char textbuf[sizeof(Stop_keyword) + 2];	/* word, semicolon, nl, nul */

	editblock = bd->editblock;

	if (bd->fil == NULL || bd->lnum == 0) {
		if (editblock != NULL) {
			srcbuf_remove_editblock(editblock, FALSE);
			bd->editblock = NULL;
		}
		return;
	}

	if (text == NULL) {
		if (is_bpt) {
			sprintf(textbuf, "%s;\n", Stop_keyword);
	
			text = build_source_text(bd->fil, bd->lnum, textbuf);
		}
		else {
			text = strsave("");
		}
	}
	else {
		bd->want_save = TRUE;
	}

	if (editblock != NULL) {
		srcbuf_remove_editblock(editblock, FALSE);
		editblock = NULL;
	}

	bd->editblock = srcwin_add_editblock(show ? get_current_srcwin() : NULL,
						bd->fil, bd->lnum, text,
						handle_editblock_action,
						handle_editblock_change,
						!breakpoint_is_disabled((long)bd),
						(char *)bd);

	if (!is_bpt && *text == '\0') {
		size_t start, lim;
		
		srcbuf_get_editblock_point_range(bd->editblock, &start, &lim);
		
		srcwin_start_edit(get_current_srcwin(),
				  get_current_srcwin_region(),
				  bd->editblock, start);
	}
}

/*  Attempt to move a breakpoint after editing the function name
 *  or line number.
 */
static bool
move_bpt(obj, newstring, edit_lnum, p_display_string)
objid_t obj;
char *newstring;
int edit_lnum;
const char **p_display_string;
{
#ifdef OS_SUNOS
	fil_t *fil1;
	fval_t fields1[FN_BPT_LAST + 1];
	objid_t obj1 = NULL;
	taddr_t addr1;
#endif /* OS_SUNOS */
	fval_t fields[FN_BPT_LAST + 1];
	func_t *f, *f1 = NULL;
	bpdesc_t *bd, *bd1 = NULL;
	const char *fname;
	fil_t *fil;
	int lnum, lnum1, ret, method_call = 0, multi_func = 0, lnum_n;
	taddr_t addr;
	target_t *xp;
	objid_t obj_n;

	xp = get_current_target();
	bd = (bpdesc_t *)obj;
	f = bd->func;

	if (edit_lnum) {
		lnum = atoi(newstring);
		fname = f->fu_demangled_name;
	}
	else {
		lnum = bd->lnum;
		fname = newstring;
	}

	get_all_fields(obj, fields);
	
	if (strstr(fname, "::"))
	  method_call = 1;
	if (f == NULL || strcmp(fname, f->fu_demangled_name) != 0) {
	  if (method_call && !list_matches_to_outwin(0, 0))
	  {
	    func_bp_list_t *fl, *funchead;
	    taddr_t call_bpt_addr;

	    funchead = 0;

	    get_matching_func_bp_list(fname, &funchead);

	    if (funchead)
	    {
	      int ans, ans_stat, first_only = 0;

	      multi_func = 1;

	      if (funchead->fl_next)
	      {
		ans_stat = prompt_for_choice
		  ("answer",
		   "Place breakpoints in all matching methods [y/n] : ",
		   "", "yn", &ans);
		first_only = ans_stat == 0 && ans == 'n';

		if (!first_only)
		  errf("\bPlacing multiple breakpoints in overloaded/multiply linked methods");
	      }
	      for (fl = funchead; fl != NULL; fl = fl->fl_next)
	      {		 
		get_min_bpt_addr(fl->fl_func, &call_bpt_addr, FALSE);
		if (xp_addr_to_breakpoint(xp, call_bpt_addr) != 0)
		  continue;	/* duplicate */
		if (f == NULL)
		{		/* skip  - use code at bottom for this */
		  f = fl->fl_func;
		  get_min_bpt_addr(f, &addr, FALSE);
		  addr_to_fil_and_lnum(f, addr, &fil, &lnum, FALSE);
		  lnum1 = lnum;
		  if (first_only)
		    break;
		  else
		    continue;
		}
		addr_to_fil_and_lnum(fl->fl_func, call_bpt_addr,
				     &fil, &lnum_n, FALSE);
		if (!lnum_n)
		  lnum_n = -1;	/* allow multiple breakpoints in libraries */
		obj_n = add_breakpoint_object(fl->fl_func, fil, lnum_n, TRUE);
	      }
	      do_free_func_bp_list_list(funchead);
	    }
	    else
	    {
	      errf("No function `%s'", fname);
	      return FALSE;
	    }
	  }
	  else
	  {
	    ret = find_func_by_name(fname, &f, &f1, TRUE);
	    if (ret != 0)
	      return FALSE;
	    
	    get_min_bpt_addr(f, &addr, FALSE);
	    addr_to_fil_and_lnum(f, addr, &fil, &lnum, FALSE);
#ifdef OS_SUNOS
	    addr1 = addr;
	    lnum1 = lnum;
	    if (bd->func && (!f1 || edit_lnum < 0)) /* func name change */
	    {
	      if (find_duplicate_file(fil, &fil1) && fil1->fi_srcbuf)
	      {
		obj1 = (objid_t)srcbuf_get_editblock_in_tracking_file
		  (bd->editblock, bd->fil->fi_srcbuf, fil1->fi_srcbuf);
		if (obj1)
		{
		  bd1 = (bpdesc_t *)obj1;
		  if (bd->lnum != bd1->lnum)
		    obj1 = NULL;
		  else
		    get_all_fields(obj1, fields1);
		}
	      }
	    }
#endif /* OS_SUNOS */
	  }
	  if (f1)
	  {
	    func_bp_list_t *fl, *funchead;
	    taddr_t call_bpt_addr;

	    funchead = 0;

	    get_matching_func_bp_list(fname, &funchead);

	    if (funchead)
	    {
	      multi_func = 1;
	      if (method_call)
		errf("\bPlacing multiple breakpoints in overloaded/multiply linked methods");
	      else
		errf("\bAdding multiple breakpoints because function has been multiply linked");
	      for (fl = funchead->fl_next; fl != NULL; fl = fl->fl_next)
	      {		 /* skip first one - use code at bottom for this */
		get_min_bpt_addr(fl->fl_func, &call_bpt_addr, FALSE);
		if (xp_addr_to_breakpoint(xp, call_bpt_addr) != 0)
		  continue;	/* duplicate */
		addr_to_fil_and_lnum(fl->fl_func, call_bpt_addr,
				     &fil, &lnum_n, FALSE);
		if (!lnum_n)
		  lnum_n = -1;	/* allow multiple breakpoints in libraries */
		obj_n = add_breakpoint_object(fl->fl_func, fil, lnum_n, TRUE);
	      }
	      do_free_func_bp_list_list(funchead);
	    }
	  }
	}
	else {
	  if (map_lnum_to_addr(f, bd->fil, lnum, &addr) != 0)
	    return FALSE;
	  addr_to_fil_and_lnum(f, addr, &fil, &lnum, FALSE);
#ifdef OS_SUNOS
	  lnum1 = lnum;
	  if (find_duplicate_file(fil, &fil1) && fil1->fi_srcbuf)
	  {
	    obj1 = (objid_t)srcbuf_get_editblock_in_tracking_file
	      (bd->editblock, bd->fil->fi_srcbuf, fil1->fi_srcbuf);
	    if (obj1)
	    {
	      bd1 = (bpdesc_t *)obj1;
	      f1 = bd1->func;
	      get_all_fields(obj1, fields1);
	      if (map_lnum_to_addr(f1, bd1->fil, lnum1, &addr1) != 0)
		return FALSE;
	      addr_to_fil_and_lnum(f1, addr1, &fil, &lnum1, FALSE);
	    }
	  }
#endif /* OS_SUNOS */
	}
	if (f == NULL)
	{
	  errf("No match for `%s'", fname);
	  return FALSE;
	}
	
	/*  We don't allow more than one breakpoint at a given address.
	 */
	if (xp_addr_to_breakpoint(xp, addr) != 0) {
		errf("There is already a breakpoint at line %d of %s",
							lnum, f->fu_demangled_name);
		return FALSE;
	}

	addr_to_fil_and_lnum(f, addr, &fil, &lnum, FALSE);
	fields[FN_BPT_TYPESTR] = "function";
	set_bpt_location(fields, f, lnum);
	fields[FN_BPT_LAST] = BPT_FLIST_TERMINATOR;
	set_all_fields(obj, fields, BPT_FLIST_TERMINATOR);

	bd->fil = fil;
	bd->func = f;
	bd->lnum = lnum;
	bd->want_save = TRUE;

	/*  Move the actual breakpoint in the code to the new position.
	 */
	if (bd->breakpoint != NULL)
		xp_remove_breakpoint(xp, bd->breakpoint);
	bd->breakpoint = xp_add_breakpoint(xp, addr);
	set_breakpoint_data(bd->breakpoint, (long)bd);

	update_editblock(bd, TRUE, (char *)NULL, TRUE);
#ifdef OS_SUNOS
	if (obj1)
	{
	  fields1[FN_BPT_TYPESTR] = "function";
	  set_bpt_location(fields1, f1, lnum1);
	  fields1[FN_BPT_LAST] = BPT_FLIST_TERMINATOR;
	  set_all_fields(obj1, fields1, BPT_FLIST_TERMINATOR);
	  
	  bd1->fil = fil1;
	  bd1->func = f1;
	  bd1->lnum = lnum1;
	  bd1->want_save = TRUE;
	  
	  /*  Move the actual breakpoint in the code to the new position.
	   */
	  if (bd1->breakpoint != NULL)
	    xp_remove_breakpoint(xp, bd1->breakpoint);
	  bd1->breakpoint = xp_add_breakpoint(xp, addr1);
	  set_breakpoint_data(bd1->breakpoint, (long)bd1);
	  
	  update_editblock(bd1, TRUE, (char *)NULL, TRUE);
	}
#endif /* OS_SUNOS */

	if (edit_lnum) {
		static char buf[20];

		sprintf(buf, "%d", lnum);
		*p_display_string = buf;
	}
	else {
		*p_display_string = f->fu_demangled_name;
	}

	return TRUE;
}


int
global_bp_enabled(set, reset)
     int set;
     int reset;
{
  static int enabled = 1;

  if (set)
    enabled = 1;
  if (reset)
    enabled = 0;
  return(enabled);
}

static bool
execute_bp_code_internal(bp, fp, ap, button)
breakpoint_t *bp;
taddr_t fp, ap;
int button;
{
	objid_t obj;
	bpdesc_t *bd;
	ci_exec_result_t res;

	obj = get_code(BPHEAD_OBJCODE, OBJ_CHILD);
	for (; obj != NULL; obj = get_code(obj, OBJ_NEXT)) {
		if (((bpdesc_t *)obj)->breakpoint == bp)
			break;
	}

	if (obj == NULL)
		return TRUE;

	bd = (bpdesc_t *)obj;
	
	if (!button && bd->bd_inactive)
		return FALSE;

	if (bd->code_bad) {
		errf("Bad breakpoint code");
		return TRUE;
	}
	if (bd->machine == NULL)
		return TRUE;
	
	if (bd->data_needs_initialising) {
		ci_initialise_machine(bd->machine, TRUE, FALSE,
						(char **)NULL, (char **)NULL);
		bd->data_needs_initialising = FALSE;
	}
	else {
		ci_initialise_machine(bd->machine, FALSE, FALSE,
						(char **)NULL, (char **)NULL);
	}

	res = ci_execute_machine(bd->machine, fp, ap, read_data, write_data,
							call_target_function);

	if (res != CI_ER_TRAP && res != STOP)
		errf("%s in breakpoint code", ci_exec_result_to_string(res));

	return res != CI_ER_TRAP;
}

bool
execute_bp_code(bp, fp, ap)
breakpoint_t *bp;
taddr_t fp, ap;
{
 	if (!global_bp_enabled(0, 0))
 		return FALSE;
 
	if ((get_breakpoint_data(bp) == 0))
		return TRUE;

	return (execute_bp_code_internal(bp, fp, ap, 0));
}

static int
read_data(addr, buf, nbytes)
taddr_t addr;
void *buf;
size_t nbytes;
{
	return dread(get_current_target(), addr, buf, nbytes);
}

static int
write_data(addr, buf, nbytes)
taddr_t addr;
constvoidptr buf;
size_t nbytes;
{
	return dwrite(get_current_target(), addr, buf, nbytes);
}

static int
set_initialise_data_flag(obj, unused_arg)
objid_t obj;
fval_t unused_arg;
{
	((bpdesc_t *)obj)->data_needs_initialising = TRUE;
	return 0;
}

void
reinitialise_bpt_code_data()
{
	visit_objects(BPHEAD_OBJCODE, OBJ_CHILDREN, set_initialise_data_flag,
						(fval_t)0, (objid_t *)NULL);
}

void
remove_breakpoint_object(obj)
objid_t obj;
{
	bpdesc_t *bd;

	bd = (bpdesc_t *)obj;

	if (bd->editblock != NULL)
		srcbuf_remove_editblock(bd->editblock, FALSE);

	if (bd->breakpoint != NULL)
		xp_remove_breakpoint(get_current_target(), bd->breakpoint);

	if (bd->parse_id != NULL)
		ci_free_parse_id(bd->parse_id);
	if (bd->machine != NULL)
		ci_free_machine(bd->machine);
	
	free((char *)bd);
}

static void
show_bpt_source(bd, beep_on_failure)
bpdesc_t *bd;
bool beep_on_failure;
{
	if (bd->func == NULL) {
		show_source(bd->fil, bd->lnum);
	}
	else if (bd->lnum != 0 && bd->func->fu_fil != NULL) {
		show_source(bd->fil, bd->lnum);
	}
	else {
		errf("%s`%s' was not compiled with the -g flag",
					beep_on_failure ? "" : "\bNote: ",
					bd->func->fu_demangled_name);
	}
}

static void
set_bpt_state(obj, disable, verbose)
objid_t obj;
bool disable;
bool verbose;
{
	fval_t fields[FN_BPT_LAST + 1];
	bpdesc_t *bd;

	if (verbose)
	  if (disable)
	    errf("\bSelected breakpoint code will be ignored");
	  else
	    if (global_bp_enabled(0, 0))
	      errf("\bSelected breakpoint code will be executed");
	    else
	      errf("\bSelected breakpoint code will be executed when globally enabled");
	bd = (bpdesc_t *)obj;
	bd->bd_inactive = disable;
/*	if (disable)
	  uninstall_breakpoint(bd->breakpoint);*/
	get_all_fields(obj, fields);
	fields[FN_BPT_STATE] = disable ? (fval_t)"inactive" : (fval_t)"active";
	fields[FN_BPT_LAST] = BPT_FLIST_TERMINATOR;
	set_all_fields((objid_t)bd, fields, BPT_FLIST_TERMINATOR);
	srcwin_enable_editblock(bd->editblock, !breakpoint_is_disabled((long)bd));
        srcwin_redraw(get_current_srcwin());
}

static void
enable_all_breakpoints(enable)
bool enable;
{
    objid_t obj;
    for (obj = get_code(BPHEAD_OBJCODE, OBJ_CHILD);
	 obj != NULL;
	 obj = get_code(obj, OBJ_NEXT))
    {
	bpdesc_t *bd;

	/* Force change in object window */
	obj_has_changed(obj);

	/* Force change in src window */
	bd = (bpdesc_t *)obj;
	srcwin_enable_editblock(bd->editblock, enable);
    }
    srcwin_redraw(get_current_srcwin());
}


char *
bpt_format_obj(code)
objid_t code;
{
	const char *fname;
	const char *lnum, *obj, *state;

	fname = get_field_value(code, FN_BPT_FNAME);
	obj = get_field_value(code, FN_BPT_FILE);
	lnum = get_field_value(code, FN_BPT_LNUM);
	state = get_field_value(code, FN_BPT_STATE);

	return strf("%s %s %s %s", fname, obj, lnum, state);
}

int
pre_do_bpt(command, p_arg)
int command;
char **p_arg;
{
	bptargs_t *ba;
	const char *mode, *prompt;
	FILE *fp;
	char *path;
	
	if (command != MR_BPTMEN_SAVE) {
		*p_arg = NULL;
		return 0;
	}

	if (get_num_selected() == 1)
		prompt = "Save breakpoint to file: ";
	else
		prompt = "Save breakpoints to file: ";
	
	if (prompt_for_output_file(prompt, &path, &mode) != 0)
		return -1;

	if ((fp = fopen_with_twiddle(path, mode)) == NULL) {
		failmesg("Can't create", "saved breakpoints file", path);
		return -1;
	}

	ba = (bptargs_t *)e_malloc(sizeof(bptargs_t));
	ba->ba_fp = fp;
	ba->ba_path = path;
	*p_arg = (char *)ba;
	return 0;
}

void
post_do_bpt(command, arg)
int command;
char *arg;
{
	bptargs_t *ba;
	
	if (command != MR_BPTMEN_SAVE)
		return;

	ba = (bptargs_t *)arg;

	if (fflush(ba->ba_fp) == EOF || ferror(ba->ba_fp)) {
		failmesg("Warning: Error writing to", "saved breakpoints file",
			 ba->ba_path);
	}

	fclose(ba->ba_fp);

	free(ba->ba_path);
	free((char *)ba);
}

/*  Process the return from the breakpoint menu enabling and disabling
 *  ACTIVE, RESTART etc. and displaying source.
 */
void
do_bpt(obj, command, arg)
objid_t obj;
int command;
char *arg;
{
#ifdef OS_SUNOS
	objid_t obj1 = NULL;
#endif /* OS_SUNOS */
	int oldstate;
	bptargs_t *ba;
	bpdesc_t *bd;
	target_t *xp;
	tstate_t tstate;

	if ( command == MR_BPTMEN_ACCELERATOR_ACTION )
	    command =  bpt_accelerator_action(obj);


	ba = (bptargs_t *)arg;

	display_area_overlay_unselect_global_selection();
	oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
	
	bd = (bpdesc_t *)obj;
#ifdef OS_SUNOS
	if (command != MR_BPTMEN_SOURCE && bd->func && bd->func->fu_fil &&
	    (bd->func->fu_fil && bd->func->fu_fil->fi_flags & FI_DUPLICATE))
	{
	  fil_t *fil1;
				    
	  if (find_duplicate_file(bd->fil, &fil1) && fil1->fi_srcbuf)
	    obj1 = (objid_t)srcbuf_get_editblock_in_tracking_file
	      (bd->editblock, bd->fil->fi_srcbuf, fil1->fi_srcbuf);
	}
#endif /* OS_SUNOS */
	switch (command) {
	case MR_BPTMEN_SOURCE:
	  if (bd->func || bd->fil)
		show_bpt_source((bpdesc_t *)obj, TRUE);
		break;
	case MR_BPTMEN_REMOVE:
		remove_object(obj, OBJ_SELF);
#ifdef OS_SUNOS
		if (obj1)
		{
		  remove_object(obj1, OBJ_SELF);
		  /*ret;*/	/* RGA redo selection list! */
		}
#endif /* OS_SUNOS */
		break;
	case MR_BPTMEN_SAVE:
		save_object_to_file(ba->ba_path, ba->ba_fp, (bpdesc_t *)obj);
		break;
	case MR_BPTMEN_ACTIVE:
		set_bpt_state(obj, FALSE, TRUE);
#ifdef OS_SUNOS
		if (obj1)
		  set_bpt_state(obj1, FALSE, TRUE);
#endif /* OS_SUNOS */
		break;
	case MR_BPTMEN_INACTIVE:
		set_bpt_state(obj, TRUE, TRUE);
#ifdef OS_SUNOS
		if (obj1)
		  set_bpt_state(obj1, TRUE, TRUE);
#endif /* OS_SUNOS */
		break;
	case MR_BPTMEN_EXECUTE:
		xp = get_current_target();
		tstate = xp_get_state(xp);
		if (tstate != TS_NOTR)
		{
		  execute_bp_code_internal(bd->breakpoint,
					   xp_getreg(xp, UPSREG_FP),
					   xp_getreg(xp, UPSREG_AP), 1);
		  update_variable_values();
		}
		else
		  errf("Target must be running to execute breakpoint code");
		break;
	default:
		panic("bad rv in db");
	}
	
	td_set_obj_updating(oldstate);
}

void
remove_matching_breakpoints(st, fil)
symtab_t *st;
fil_t *fil;
{
	objid_t obj, next;
	int oldstate;

	if (Breakpoint_header_exists == FALSE)
	  return;		/* RGA */
	oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
	for (obj = get_code(BPHEAD_OBJCODE, OBJ_CHILD); obj != NULL; obj = next) {
		next = get_code(obj, OBJ_NEXT);
		if ((st && ((bpdesc_t *)obj)->func->fu_symtab == st) ||
		    (fil && ((bpdesc_t *)obj)->fil == fil))
		  remove_object(obj, OBJ_SELF);
	}
	td_set_obj_updating(oldstate);
}

void
save_matching_bpts(st, fil, handle)
     symtab_t *st;
     fil_t *fil;
     char *handle;
{
	objid_t obj, next;
	FILE *fp;

	fp = (FILE *)handle;
	if (Breakpoint_header_exists == FALSE)
	  return;		/* RGA */
	for (obj = get_code(BPHEAD_OBJCODE, OBJ_CHILD); obj != NULL; obj = next) {
		next = get_code(obj, OBJ_NEXT);
		if ((st && ((bpdesc_t *)obj)->func->fu_symtab == st) ||
		    (fil && ((bpdesc_t *)obj)->fil == fil))
		  save_object_to_file(NULL, fp, (bpdesc_t *)obj);
	}
}

void
update_matching_bpts(oldst, newst)
symtab_t *oldst;
symtab_t *newst;
{
	target_t *xp;
	objid_t obj, next;

	if (Breakpoint_header_exists == FALSE)
		return;
	
	xp = get_current_target();

	for (obj = get_code(BPHEAD_OBJCODE, OBJ_CHILD); obj != NULL; obj = next) {
		bpdesc_t *bd = (bpdesc_t *)obj;

		next = get_code(obj, OBJ_NEXT);
		
		if (bd->func->fu_symtab == oldst) {
			func_t *f, *f1;
			fil_t *fil;
			int lnum;
			taddr_t addr;
			fval_t fields[FN_BPT_LAST + 1];
			
			if ((fil = name_and_st_to_fil(bd->fil->fi_name, newst)) == NULL ||
			    (f = name_and_fil_to_func(bd->func->fu_name, fil, FALSE)) == NULL) {
				if (find_func_by_name(bd->func->fu_name, &f, &f1, FALSE) != 0) {
					errf("Function %s no longer appears to exist",
					     bd->func->fu_demangled_name);
					remove_object(obj, OBJ_SELF);
					continue;
				}
				fil = f->fu_fil;
			}

			if (map_lnum_to_addr(f, fil, bd->lnum, &addr) != 0) {
				errf("Can't find line %d in %s",
				     lnum, f->fu_demangled_name);
				remove_object(obj, OBJ_SELF);
				continue;
			}

			addr_to_fil_and_lnum(f, addr, &fil, &lnum, FALSE);
			
			get_all_fields(obj, fields);
			fields[FN_BPT_TYPESTR] = "function";
                        set_bpt_location(fields, f, lnum);
			fields[FN_BPT_LAST] = BPT_FLIST_TERMINATOR;
			set_all_fields(obj, fields, BPT_FLIST_TERMINATOR);
			
			if (xp_addr_to_breakpoint(xp, addr) != 0) {
				errf("There is already a breakpoint at line %d of %s",
				     lnum, f->fu_demangled_name);
				remove_object(obj, OBJ_SELF);
				continue;
			}
			
			bd->func = f;
			bd->fil = fil;
			bd->lnum = lnum;
			
			if (bd->editblock)
			   update_editblock(bd, bd->breakpoint != NULL, strsave(srcbuf_get_editblock_text(bd->editblock)), FALSE);
		}
	}
}

static void
null_ofunc(s)
const char *s;
{
}

int
save_target_state(xp, remove_bpts, remove_vars, append)
target_t *xp;
bool remove_bpts;
bool remove_vars;
bool append;
{
  FILE *fp;
  int res = 0;
  char *state_path;

  state_path = get_temp_state_filename();

  if ((fp = fopen(state_path, append ? "a" : "w")) == NULL)
  {
    errf("Can't create temp state file `%s'",
	 state_path);
    return 0;
  }
  if (target_process_exists(xp))
    save_var_display_state(TRUE, TRUE);

  if (remove_bpts == TRUE)
    res = save_all_breakpoints_to_file(state_path, fp); 
		
  if (res == 0)
    res = write_var_state_to_file(state_path, fp);
		
  if (ferror(fp) || fflush(fp) == EOF) 
    errf("Error writing to `%s' ups state file", state_path);

  fclose(fp);
  if (remove_bpts == TRUE)
    remove_all_breakpoints();
  if (remove_vars == TRUE)
  {
    remove_all_vars();
    free_vslist();
    free_vflist();
    free_ftlist();
  }
  return 1;
}

void
restore_target_state(remove_vars, remove_bpts)
bool remove_vars;
bool remove_bpts;
{
  bool junk;
  struct stat stat_buf;
  char *state_path;
  int res = 0;
  FILE *fp;
  errf_ofunc_t oldf, oldf1;

  state_path = get_temp_state_filename();
  if ( stat(state_path, &stat_buf) != 0)
    return;

  if (remove_vars == TRUE)
  {
    remove_all_vars();
    free_vslist();
    free_vflist();
    free_ftlist();
  }
  if (Breakpoint_header_exists == TRUE)
  {
    if ((fp = fopen(state_path, "a")) == NULL)
    {
      errf("Can't append to temp state file `%s'",
	   state_path);
      return;
    }
    save_all_breakpoints_to_file(state_path, fp); 
    if (ferror(fp) || fflush(fp) == EOF) 
      errf("Error writing to `%s' ups state file", state_path);

    fclose(fp);
    if (remove_bpts == TRUE)
      remove_all_breakpoints();
  }
  /* suprres messages because the target may be completely different */
  oldf = errf_set_ofunc(null_ofunc);
  oldf1 = errf_set_ofunc1(null_ofunc);
  read_config_file(get_temp_state_filename(), TRUE,  FALSE, FALSE, FALSE, &junk, TRUE);
  errf_set_ofunc(oldf);
  errf_set_ofunc1(oldf1);
}

/*  Add the breakpoint header to the display.  Called on startup from main().
 */
void
add_breakpoint_header(par)
objid_t par;
{
	new_object(BPHEAD_OBJCODE, OT_BPHEAD, par, OBJ_CHILD);
	Breakpoint_header_exists = TRUE; 
	/* RGA - ugh, but can't do remove_matching_breakpoints if this */
	/* has not been done */
}

/*  Process the return from the breakpoint header menu, creating new
 *  breakpoints and such.
 */
/* ARGSUSED */
void
do_bps(unused_obj, command, arg)
objid_t unused_obj;
int command;
char *arg;
{
	bool junk;

	/* RCB: */
	if ( command == MR_BP_ACCELERATOR_ACTION )
	    command = bps_accelerator_action();

	
	switch(command) {
	case MR_ADD_BREAKPOINT:
		add_breakpoint_object((func_t *)NULL, (fil_t *)NULL, 0, TRUE);
		break;
	case MR_REMOVE_ALL_BREAKPOINTS:
		remove_object(BPHEAD_OBJCODE, OBJ_CHILDREN);
		break;
	case MR_RESTORE_BREAKPOINTS:
		if (Statefile_path == NULL)
			errf("No saved state file to load breakpoints from");
		else
			read_config_file(Statefile_path, TRUE, TRUE, TRUE,
					 FALSE, &junk, FALSE);
		break;
	case MR_LOAD_BREAKPOINTS_EXC:
		load_breakpoints(FALSE);
		break;
	case MR_LOAD_BREAKPOINTS_INC:
		load_breakpoints(TRUE);
		break;
       case MR_ENABLE_ALL_BREAKPOINTS:
		global_bp_enabled(1, 0); /* set */
		enable_all_breakpoints(TRUE);
		set_bphead_menu_state(1);
		errf("\bGlobally enable all active breakpoints");
		break;
	case MR_DISABLE_ALL_BREAKPOINTS:
		global_bp_enabled(0, 1); /* reset */
		enable_all_breakpoints(FALSE);
		set_bphead_menu_state(0);
		errf("\bGlobally disable all breakpoints");
		break;
	default:
		panic("bad cmd in db");
	}
}

static bool
bpfunc_quitfunc(obj, new_text, p_display_string)
objid_t obj;
char *new_text;
const char **p_display_string;
{
	return move_bpt(obj, new_text, FALSE, p_display_string);
}

static bool
bplnum_quitfunc(obj, new_text, p_display_string)
objid_t obj;
char *new_text;
const char **p_display_string;
{
	return move_bpt(obj, new_text, TRUE, p_display_string);
}

static bool
bpstate_quitfunc(obj, new_text, p_display_string)
objid_t obj;
char *new_text;
const char **p_display_string;
{
	return TRUE;
}

static void
bplnum_edit(fdets)
struct drawst fdets;
{
	if (HAS_CODE((bpdesc_t *)fdets.dr_code)) {
		errf("Can't move a breakpoint that has interpreted code");
		wn_wait_for_release_of(fdets.dr_wn, B_ANY);
	}
	else {
		field_edit_start(&fdets, "line number", strsave(fdets.dr_fval));
	}
}

static void
bpfunc_edit(fdets)
struct drawst fdets;
{
	if (HAS_CODE((bpdesc_t *)fdets.dr_code)) {
		errf("Can't move a breakpoint that has interpreted code");
		wn_wait_for_release_of(fdets.dr_wn, B_ANY);
	}
	else {
		field_edit_start(&fdets, "function name", (char *)NULL);
	}
}

int breakpoint_is_disabled(bp_data)
long bp_data;
{
  bpdesc_t *bd = ( bpdesc_t *) bp_data;
  if (!global_bp_enabled(0, 0) || bd->bd_inactive)
    return 1;
  else
    return 0;
}

void
remove_all_breakpoints()
{
  if (Breakpoint_header_exists == TRUE)
  {
    errf_ofunc_t oldf, oldf1;

    oldf = errf_set_ofunc(null_ofunc);
    oldf1 = errf_set_ofunc1(null_ofunc);
    remove_object(BPHEAD_OBJCODE, OBJ_CHILDREN);
    errf_set_ofunc(oldf);
    errf_set_ofunc1(oldf1);
  }
}

int
add_breakpoints_for_fname( fname, pfirst)
const char* fname;
func_t** pfirst;
{
    /* Add breakpoints for all functions matching fname and
     * return the number of such functions.  Set *pfirst to
     * the first function.
     */
    int cnt = 0;
    func_bp_list_t *funchead = 0;

    *pfirst = NULL;
    get_matching_func_bp_list(fname, &funchead);
    if (funchead)
    {
	target_t *xp = get_current_target();
	func_bp_list_t *fl;
	for (fl = funchead; fl != NULL; fl = fl->fl_next)
	{		 
	    objid_t obj_n;
	    fil_t *fil;
	    int lnum;
	    taddr_t call_bpt_addr;
	    get_min_bpt_addr(fl->fl_func, &call_bpt_addr, FALSE);
	    if (xp_addr_to_breakpoint(xp, call_bpt_addr) != 0)
		continue;	/* duplicate */
	    if ( cnt++ == 0 )
	    {
		if (pfirst)
		   *pfirst = fl->fl_func;
	    }
	    addr_to_fil_and_lnum(fl->fl_func, call_bpt_addr,
				     &fil, &lnum, FALSE);
	    if (!lnum)
		lnum = -1;	/* allow multiple breakpoints in libraries */
	    obj_n = add_breakpoint_object(fl->fl_func, fil, lnum, TRUE);
	}
	do_free_func_bp_list_list(funchead);
    }
    return cnt;
}

void
bpt_getcolor(obj, wn, p_fg, p_bg)
objid_t obj;
int wn;
long *p_fg, *p_bg;
{
  static color_t color = { -1, 0, 0, 0 };
  bpdesc_t *bd;

  bd = (bpdesc_t *)obj;
 
  if (color.co_pixel == -1)
  {
    const char *colorname;

    color.co_pixel = WN_FG(wn);
    if (wn_use_extended_text_colors() == TRUE)
    {
      colorname = wn_get_default("BreakpointTextColor");
      if (colorname != NULL)
      {
	wn_parse_color(wn, colorname, &color);
	if (!wn_alloc_color(wn, &color,colorname))
	  color.co_pixel = WN_FG(wn);
      }
    }
  }
  if (bd && bd->bd_inactive)
    *p_fg = WN_FG(wn);
  else
    *p_fg = color.co_pixel;
  *p_bg = WN_BG(wn);
}
 
void
src_window_redraw()
{
  srcwin_redraw(get_current_srcwin());
}

static void
set_bpt_location(fields, f, lnum)
fval_t *fields;
func_t *f;
int lnum;
{
	if (f) {
		fields[FN_BPT_FNAME] = f->fu_demangled_name;
      
		if ((f->fu_flags & FU_NOSYM) != 0 || lnum == 0) {
			fields[FN_BPT_FILE] = (fval_t)f->fu_symtab->st_path;
			fields[FN_BPT_SEPARATOR] = (fval_t)"";

			if ((f->fu_flags & FU_NOSYM) != 0)
				fields[FN_BPT_LNUM] = (fval_t)"(no syms)";
			else
				fields[FN_BPT_LNUM] = (fval_t)"(no ln #)";
		}
		else {
			fields[FN_BPT_FILE] = (fval_t)f->fu_fil->fi_name;
			fields[FN_BPT_SEPARATOR] = (fval_t)":";
			fields[FN_BPT_LNUM] = (fval_t)strf("%d", lnum);
		}
	}
	else {
		fields[FN_BPT_FNAME] = (fval_t)"";
		fields[FN_BPT_FILE] = (fval_t)"";
		fields[FN_BPT_SEPARATOR] = (fval_t)"";
		fields[FN_BPT_LNUM] = (fval_t)"";
	}

	return;
}
