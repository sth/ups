/* obj_misc.c - miscellaneous object related stuff */

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


/* @(#)obj_misc.c	1.26 04 Jun 1995 (UKC) */
char ups_obj_misc_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#define UKC_WANT_COMMON_UNIX_EXTENSIONS 1/* for popen, pclose and lstat */

#include <sys/types.h>
#include <sys/param.h>		/* for MAXPATHLEN */
#include <sys/stat.h>
#include <sys/utsname.h>

#ifdef __STDC__
#include <unistd.h>
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <stdlib.h>

#include <local/wn.h>
#include <local/ukcprog.h>
#include <local/obj/obj.h>
#include <local/menu3.h>

#include <local/ukcprog.h>
#include <mtrprog/so.h>

#include "ups.h"
#include "symtab.h"
#include "objtypes.h"
#include "target.h"
#include "st.h"
#include "obj_misc.h"
#include "srcwin.h"
#include "srcpath.h"
#include "ui.h"
#include "va.h"
#include "obj_stack.h"
#include "menudata.h"
#include "tdr.h"
#include "state.h"
#include "cursors.h"

static int cblock_cmp PROTO((objid_t obj1, objid_t obj2));
static void set_max_src_width PROTO((fil_t *fil, bool dummy));
static int module_cmp PROTO((objid_t obj1, objid_t obj2));
static void add_mfunc_object PROTO((objid_t par, func_t *f, int pos));
static void expand_module PROTO((objid_t par));
static void add_module_object_if_necessary PROTO((Module *module));
static int modfunc_cmp PROTO((objid_t obj1, objid_t obj2));
static void set_max_module_width PROTO((Module *module));
static void fil_note_file_collapsed PROTO((fil_t *fil, bool flag));
static void update_displayed_file_entries PROTO((objid_t par));
static int base_src_cmp PROTO((objid_t obj1, objid_t obj2, bool no_hide));
static int src_cmp2 PROTO((objid_t obj1, objid_t obj2));
static int src_cmp3 PROTO((objid_t obj1, objid_t obj2));

const char Srchead_format[] = "Source files\n";

const char Sfile_format[] = "%[-]s";
const char Sfile_ex_format[] = "\n%[-]s\n";
#define FN_SFILE_NAME	0

const char Globals_format[] = "Untyped variables\n";

const char Fhead_format[] = "Functions\n";

const char Cblock_format[] = "%[-]16cs %[-]60cs\n";

#define FN_CBLOCK_NAME	0
#define FN_CBLOCK_DFUNC	1

const char Cbhead_format[] = "Common blocks\n";

const char Mhead_format[] = "Modules\n";

const char Module_format[] = "%[-]s";
const char Module_ex_format[] = "\n%[-]s\n";
#define FN_MODULE_NAME	0

const char Module_func_format[] = "\n%[-]10cs %[-]s\n";
#define FN_MODULE_FUNC_TYPE	0
#define FN_MODULE_FUNC_NAME	1

/*  Width in pixels of the longest source file name and module name.
 */
static int Max_src_width = 0;
static int Max_module_width = 0;

char *
header_format_obj(code)
objid_t code;
{
	const char *name;
	int len;

	name = (char *)code;
	len = strlen(name) - 1;

	if (len <= 0 || name[len] != '\n')
		panic("name botch in hd");

	return strf("%.*s", len, name);
}

/*  Add a header object for the global variables.
 */
void
add_globals_header(par)
objid_t par;
{
	new_object(GLOBALS_OBJCODE, OT_GLOBALS, par, OBJ_CHILD);
}

/*  Process the return from the globals menu. just display and conceal
 *  variables.
 */
void
do_globals(obj, command, arg)
objid_t obj;
int command;
char *arg;
{
	switch(command) {
	case MR_SHOW_UNTYPED_VARS:
		if (can_get_target_vars(get_current_target()))
			addvars(obj);
		else
			errf("Target not running");
		break;
	case MR_HIDE_UNTYPED_VARS:
		update_file_entry(obj);
		remove_object(obj, OBJ_DESCENDENTS);
		note_file_collapsed(obj, FALSE);
		break;
	default:
		panic("bad cmd in dg");
	}
}

static void
add_module_object_if_necessary(module)
Module *module;
{
	if (obj_child_exists(MHEAD_OBJCODE, (objid_t)module))
		return;

	new_object((objid_t)module, OT_MODULE, MHEAD_OBJCODE, OBJ_LAST_CHILD);
	set_field_value((objid_t)module, FN_MODULE_NAME,
			(fval_t)get_module_name(module));
}

void
add_common_block_object_if_necessary(cblock)
common_block_t *cblock;
{
	char *buf, *fnbuf;
	const char *name, *dfunc_name;
	int len;

	if (obj_child_exists(CBHEAD_OBJCODE, (objid_t)cblock))
		return;

	name = get_cblock_name(cblock);
	dfunc_name = get_cblock_funcname(cblock);

	if (strcmp(name, "_BLNK__") == 0)
		buf = strsave("Blank common");
	else {
		len = strlen(name);
		if (name[len - 1] == '_')
			--len;
		buf = strf("common /%.*s/", len, name);
	}

	if (dfunc_name == NULL)
		fnbuf = strsave("");
	else
		fnbuf = strf("(as defined in %s)", dfunc_name);

	new_object((objid_t)cblock, OT_CBLOCK, CBHEAD_OBJCODE, OBJ_CHILD);
	set_field_value((objid_t)cblock, FN_CBLOCK_NAME, (fval_t)buf);
	set_field_value((objid_t)cblock, FN_CBLOCK_DFUNC, (fval_t)fnbuf);
}

const char *
module_getobjname(obj)
objid_t obj;
{
	return get_module_name((Module *)obj);
}

const char *
module_func_getobjname(obj)
objid_t obj;
{
	return ((func_t *)obj)->fu_name;
}

const char *
cblock_getobjname(obj)
objid_t obj;
{
	static char *last = NULL;
	const char *name, *dfunc_name;

	name = get_cblock_name((common_block_t *)obj);
	dfunc_name = get_cblock_funcname((common_block_t *)obj);

	if (last != NULL)
		free(last);

	if (dfunc_name == NULL) {
		last = NULL;
		return name;
	}

	last = strf("%s[%s]", name, dfunc_name);
	return last;
}

void
free_common_block_object(obj)
objid_t obj;
{
	free((char *)get_field_value(obj, FN_CBLOCK_NAME));
	free((char *)get_field_value(obj, FN_CBLOCK_DFUNC));
}

static int
cblock_cmp(obj1, obj2)
objid_t obj1, obj2;
{
	return strcmp(get_cblock_name((common_block_t *)obj1),
		      get_cblock_name((common_block_t *)obj2));
}

static int
modfunc_cmp(obj1, obj2)
objid_t obj1, obj2;
{
	return strcmp(((func_t *)obj1)->fu_name, ((func_t *)obj2)->fu_name);
}

void
do_cbhead(obj, command, arg)
objid_t obj;
int command;
char *arg;
{
	switch(command) {
	case MR_SHOW_COMMON_BLOCKS:
		iterate_over_common_blocks(add_common_block_object_if_necessary);
		sort_children(obj, cblock_cmp);
		break;
	case MR_HIDE_ALL_COMMON_BLOCKS:
		remove_object(obj, OBJ_DESCENDENTS);
		break;
	default:
		panic("bad cmd in dch");
	}
}

void
do_mhead(obj, command, arg)
objid_t obj;
int command;
char *arg;
{
	switch(command) {
	case MR_SHOW_COMMON_BLOCKS:
		iterate_over_all_modules(add_module_object_if_necessary);
		sort_children(obj, module_cmp);
		break;
	case MR_HIDE_ALL_COMMON_BLOCKS:
		remove_object(obj, OBJ_DESCENDENTS);
		break;
	default:
		panic("bad cmd in dch");
	}
}

char *
cblock_format_obj(code)
objid_t code;
{
	return strf("%s %s", get_field_value(code, FN_CBLOCK_NAME),
		    	     get_field_value(code, FN_CBLOCK_DFUNC));
}

char *
module_format_obj(code)
objid_t code;
{
	return strf("%s", get_field_value(code, FN_MODULE_NAME));
}

char *
module_func_format_obj(code)
objid_t code;
{
	return strf("%s %s",
		    get_field_value(code, FN_MODULE_FUNC_TYPE),
		    get_field_value(code, FN_MODULE_FUNC_NAME));
}

void
do_module(obj, command, arg)
objid_t obj;
int command;
char *arg;
{
	switch(command) {
	case MR_EXPAND_MODULE:
		expand_module(obj);
		change_type(obj, OT_MODULE_EX);
		sort_children(MHEAD_OBJCODE, module_cmp);
		ensure_visible(obj);
		break;
		
	case MR_COLLAPSE_MODULE:
		remove_object(obj, OBJ_DESCENDENTS);
		change_type(obj, OT_MODULE);
		sort_children(MHEAD_OBJCODE, module_cmp);
		ensure_visible(obj);
		break;
		
	case MR_HIDE_MODULE:
		remove_object(obj, OBJ_DESCENDENTS);
		remove_object(obj, OBJ_SELF);
		break;
		
	default:
		panic("bad cmd in dcbom");
	}
}

void
do_cblock(obj, command, arg)
objid_t obj;
int command;
char *arg;
{
	switch(command) {
	case MR_EXPAND_COMMON_BLOCK_COMPLETELY:
	case MR_EXPAND_COMMON_BLOCK_LIKE_BEFORE:
		if (can_get_target_vars(get_current_target())) {
			if (command == MR_EXPAND_COMMON_BLOCK_COMPLETELY ||
			    !restore_file_display(obj, TRUE)) {
				addvars(obj);
			}
		}
		else {
			errf("Target not running");
		}
		break;
		
	case MR_COLLAPSE_COMMON_BLOCK:
		update_file_entry(obj);
		remove_object(obj, OBJ_DESCENDENTS);
		note_file_collapsed(obj, FALSE);
		break;
		
	case MR_HIDE_COMMON_BLOCK:
		remove_object(obj, OBJ_DESCENDENTS);
		remove_object(obj, OBJ_SELF);
		break;
		
	default:
		panic("bad cmd in dcbom");
	}
}

void
do_module_func(obj, command, arg)
objid_t obj;
int command;
char *arg;
{
	func_t *f;

	f = (func_t *)obj;
	
	switch (command) {
	case MR_MODULE_FUNC_DISPLAY_SOURCE:
		show_func_source((Srcwin *)NULL, f);
		break;
	case MR_MODULE_FUNC_HIDE:
		remove_object(obj, OBJ_DESCENDENTS); /* maybe one day */
		remove_object(obj, OBJ_SELF);
		break;
	default:
		panic("bad cmd in dmf");
	}
}

int
src_cmp(obj1, obj2)
objid_t obj1, obj2;
{
	bool ch1, ch2;

	ch1 = get_code(obj1, OBJ_CHILD) != NULL;
	ch2 = get_code(obj2, OBJ_CHILD) != NULL;

	if (ch1 != ch2)
		return ch1 ? 1 : -1;

	return strcmp(((fil_t *)obj1)->fi_name_only,
		      ((fil_t *)obj2)->fi_name_only);
}

static int
base_src_cmp(obj1, obj2, no_hide)
     objid_t obj1, obj2;
     bool no_hide;
{
  int strs_differ;
  
  /* RGA first see if just the file names are the same. If so */
  /* then check whole file name (which includes paths on headers) */
  
  strs_differ = strcmp(((fil_t *)obj1)->fi_name_only,
					  ((fil_t *)obj2)->fi_name_only);
  if (!strs_differ &&
      (((fil_t *)obj1)->fi_name_only != ((fil_t *)obj1)->fi_name ||
       ((fil_t *)obj2)->fi_name_only != ((fil_t *)obj2)->fi_name))
    strs_differ = strcmp(((fil_t *)obj1)->fi_name,
			 ((fil_t *)obj2)->fi_name);
  
  if (!strs_differ && (((fil_t *)obj1)->fi_flags & FI_DUPLICATE ||
		       ((fil_t *)obj2)->fi_flags & FI_DUPLICATE))
    return 1;
 if (no_hide == FALSE && !strs_differ && 
      (((fil_t *)obj1)->fi_flags & FI_HIDE_ENTRY) == 0 &&
      (((fil_t *)obj2)->fi_flags & FI_HIDE_ENTRY) == 0)
    ((fil_t *)obj2)->fi_flags |= FI_HIDE_ENTRY;
  return strs_differ;
}

static int
src_cmp2(obj1, obj2)
     objid_t obj1, obj2;
{
  return base_src_cmp(obj1, obj2, FALSE);
}


static int
src_cmp3(obj1, obj2)
     objid_t obj1, obj2;
{
  return base_src_cmp(obj1, obj2, TRUE);
}

static int
module_cmp(obj1, obj2)
objid_t obj1, obj2;
{
	bool ch1, ch2;

	ch1 = get_code(obj1, OBJ_CHILD) != NULL;
	ch2 = get_code(obj2, OBJ_CHILD) != NULL;

	if (ch1 != ch2)
		return ch1 ? 1 : -1;

	return strcmp(get_module_name((Module *)obj1),
		      get_module_name((Module *)obj2));
}

/*  Process the return from the source header menu.
 *  Add or remove source files from the display.
 */
void
do_srchead(obj, command, arg)
objid_t obj;
int command;
char *arg;
{
        char *path_str;
	Pathentry *tail;

	switch(command) {
	case MR_SHOW_SOURCE_FILES:
		iterate_over_source_files
		  (add_source_file_object_if_necessary, TRUE);
		sort_children(obj, src_cmp);
		break;
	case MR_HIDE_SOURCE_FILES:
		update_displayed_file_entries(obj);
		remove_object(obj, OBJ_CHILDREN);
		break;
	case MR_HIDE_SOURCE_FILES_COMPLETELY:
		update_displayed_file_entries(obj);
		iterate_over_source_files(fil_note_file_collapsed, FALSE);
		remove_object(obj, OBJ_DESCENDENTS);
		break;
	case MR_ADD_SOURCE_PATH:
		set_bm_cursor(WN_STDWIN, CU_MENU);
		tail = get_source_path_tail();
		if (prompt_for_string("filename", "Add path: ",
				      tail ? tail->path : "", &path_str) != 0)
		  break;
		srcpath_check_and_add_path(path_str);
		break;
	}
}

static void
fil_note_file_collapsed(fil, flag)
fil_t *fil;
bool flag;
{
  note_file_collapsed((objid_t)fil, flag);
}
  
static void
update_displayed_file_entries(par)
objid_t par;
{
	objid_t obj;
	
	for (obj = get_code(SRCHEAD_OBJCODE, OBJ_CHILD);
	     obj != NULL;
	     obj = get_code(obj, OBJ_NEXT)) {
		update_file_entry(obj);
	}
}
	
void
add_source_file_object_if_required(fil, restore_file_displays,
				   use_src_cmp2, use_src_cmp3)
fil_t *fil;
bool restore_file_displays;
bool use_src_cmp2;
bool use_src_cmp3;
{
  if (fil->fi_symtab && fil->fi_symtab->st_eclipsed)
    return;

  if (use_src_cmp2)
  {
    if (obj_child_exists2(SRCHEAD_OBJCODE, (objid_t)fil, src_cmp2))
      return;
  }
  else
    if (use_src_cmp3)
    {
      if (obj_child_exists2(SRCHEAD_OBJCODE, (objid_t)fil, src_cmp3))
	return;
    }
    else
      if (obj_child_exists(SRCHEAD_OBJCODE, (objid_t)fil))
	return;
  
  new_object((objid_t)fil, OT_SFILE, SRCHEAD_OBJCODE, OBJ_CHILD);
  set_field_value((objid_t)fil, FN_SFILE_NAME, (fval_t)fil->fi_name_only);
  if (restore_file_displays == TRUE &&
      can_get_target_vars(get_current_target()))
    restore_file_display((objid_t)fil, FALSE);
}

void
add_source_file_object_if_necessary(fil, restore_file_displays)
     fil_t *fil;
     bool restore_file_displays;
{
  add_source_file_object_if_required(fil, restore_file_displays, TRUE, FALSE);
}

void
add_source_file_object_if_necessary2(fil, restore_file_displays)
     fil_t *fil;
     bool restore_file_displays;
{
  add_source_file_object_if_required(fil, restore_file_displays, FALSE, TRUE);
}

static void
set_max_src_width(fil, dummy)
fil_t *fil;
bool dummy;
{
	int width;

	width = wn_strwidth(fil->fi_name_only, (font_t *)NULL);
	if (width > Max_src_width)
		Max_src_width = width;
}

static void
set_max_module_width(module)
Module *module;
{
	int width;

	width = wn_strwidth(get_module_name(module), (font_t *)NULL);
	
	if (width > Max_module_width)
		Max_module_width = width;
}

/*  Add the variables to the display.
 */
int
addvars(par)
objid_t par;
{
	fil_t *fi;
	var_t *v;

	/*  Add the variables to the list
	 */
	switch(ups_get_object_type(par)) {
	case OT_SFILE:
		fi = (fil_t *)par;
		if (FI_VARS(fi) == NULL) {
			errf("No global variables in source file %s",
			     fi->fi_name);
			return -1;
		}
		v = FI_VARS(fi);
		break;
	case OT_CBLOCK:
		v = get_cblock_vars((common_block_t *)par);
		break;
	default:
		panic("unknown object type in addvars");
		v = NULL; /* to satisfy gcc */
	}

	for (; v != NULL; v = v->va_next)
		if (find_var(par, v) == NULL)
			(void) add_var_object(par, v, OBJ_FIRST_CHILD, FALSE);
	return 0;
}

static void
add_mfunc_object(par, f, pos)
objid_t par;
func_t *f;
int pos;
{
	const char *typestr;
	
	if (f->fu_type != NULL && f->fu_type->ty_code == TY_VOID)
		typestr = "subroutine";
	else
		typestr = "function";
	
	new_object((objid_t)f, OT_MODULE_FUNC, par, pos);
	set_field_value((objid_t)f, FN_MODULE_FUNC_TYPE, typestr);
	set_field_value((objid_t)f, FN_MODULE_FUNC_NAME, f->fu_name);
}

static void
expand_module(par)
objid_t par;
{
	funclist_t *funclist, *fl;
	bool was_empty;

	funclist = get_module_funclist((Module *)par);

	was_empty = TRUE;
	
	for (fl = funclist; fl != NULL; fl = fl->fl_next) {
		if (obj_child_exists(par, (objid_t)fl->fl_func))
			was_empty = FALSE;
		else
			add_mfunc_object(par, fl->fl_func, OBJ_LAST_CHILD);
	}

	if (was_empty)
		sort_children(par, modfunc_cmp);
}

static 
void
srcfile_getsize_internal(unused_obj, unused_par, sz, reset)
objid_t unused_obj, unused_par;
struct szst *sz;
bool reset;
{
	static bool first_call = TRUE;

	if (reset)
	{
	  first_call = TRUE;
	  return;
	}
	if (first_call) {
		iterate_over_source_files(set_max_src_width, FALSE);
		first_call = FALSE;
	}
	sz->sz_depth = wn_get_sysfont()->ft_height;
	sz->sz_width = Max_src_width + wn_get_sysfont()->ft_width;
}

/*  Return the width in pixels of a source file object.
 *
 *  A pointer to this function is passed to the obj library.
 */
/* ARGSUSED */
void
srcfile_getsize(unused_obj, unused_par, sz)
objid_t unused_obj, unused_par;
struct szst *sz;
{
  srcfile_getsize_internal(unused_obj, unused_par, sz, FALSE);
}

static 
void
module_getsize_internal(unused_obj, unused_par, sz, reset)
objid_t unused_obj, unused_par;
struct szst *sz;
bool reset;
{
	static bool first_call = TRUE;

	if (reset)
	{
	  first_call = TRUE;
	  return;
	}
	if (first_call) {
		iterate_over_all_modules(set_max_module_width);
		first_call = FALSE;
	}

	sz->sz_depth = wn_get_sysfont()->ft_height;
	sz->sz_width = Max_src_width + wn_get_sysfont()->ft_width;
}		
			
void
module_getsize(unused_obj, unused_par, sz)
objid_t unused_obj, unused_par;
struct szst *sz;
{
  module_getsize_internal(unused_obj, unused_par, sz, FALSE);
}		
			
void
module_func_getsize(obj, par, sz)
objid_t obj, par;
struct szst *sz;
{
	font_t *font;
	size_t len;

	font = wn_get_sysfont();
	len = 10 + 1 + strlen(((func_t *)obj)->fu_name);

/* RGA need to keep to char dimensions so overlay works */
/*	sz->sz_depth = font->ft_height + 1;*/
	sz->sz_depth = font->ft_height;
	sz->sz_width = len * font->ft_width;
}

const char *
srcfile_getobjname(obj)
objid_t obj;
{
	static char *last = NULL;
	const char *name;
	objid_t obj2;
	int count;

	name = ((fil_t *)obj)->fi_name;

	obj2 = get_code(SRCHEAD_OBJCODE, OBJ_CHILD);

	count = 1;
	for (; obj2 != obj && obj2 != NULL; obj2 = get_code(obj2, OBJ_NEXT)) {
		if (strcmp(((fil_t *)obj2)->fi_name, name) == 0)
			++count;
	}
	
	if (last != NULL)
		free(last);

	if (count == 1) {
		last = NULL;
		return name;
	}
	else {
		last = strf("%d-%s", count, name);
		return last;
	}
}

char *
file_format_obj(code)
objid_t code;
{
	return strf("%s", ((fil_t *)code)->fi_name);
}

int
pre_do_file(command, p_arg)
int command;
char **p_arg;
{
	switch (command) {
	case MR_ADD_EXPRESSION:	
	case MR_EXPAND_COMPLETELY:
	case MR_EXPAND_LIKE_BEFORE:
		if (!can_get_target_vars(get_current_target())) {
			errf("Target not running");
			return -1;
		}
		return check_single_expr(command == MR_ADD_EXPRESSION);
	default:
		return 0;
	}
}

/*  Process the return from a source file menu. Either show the variables
 *  or call the editor.
 */
void
do_file(par, command, arg)
objid_t par;
int command;
char *arg;
{
	fil_t *fil;

	fil = (fil_t *)par;

	switch(command) {
	case MR_ADD_EXPRESSION:
		change_type(par, OT_SFILE_EX);
		sort_children(SRCHEAD_OBJCODE, src_cmp);
		ensure_visible(par);

		st_get_fi_vars(fil);
		add_expr_object(par, fil->fi_block, "", fil->fi_language,
				OBJ_LAST_CHILD);

		if (get_code(par, OBJ_CHILD) == NULL) {
			change_type(par, OT_SFILE);
			sort_children(SRCHEAD_OBJCODE, src_cmp);
			ensure_visible(par);
		}
		break;
	case MR_EXPAND_COMPLETELY:
	case MR_EXPAND_LIKE_BEFORE:
		if (!can_get_target_vars(get_current_target())) {
			errf("Target not running");
			return;
		      }
		if (command == MR_EXPAND_COMPLETELY ||
		    !restore_file_display(par, TRUE)) {
			addvars(par);
		}
		
		if (get_code(par, OBJ_CHILD) != NULL)
			change_type(par, OT_SFILE_EX);

		sort_children(SRCHEAD_OBJCODE, src_cmp);
		ensure_visible(par);

		break;
	case MR_COLLAPSE:
		if (!can_get_target_vars(get_current_target())) {
			errf("Target not running");
			return;
		      }
		update_file_entry(par);
		remove_object(par, OBJ_DESCENDENTS);
		note_file_collapsed(par, FALSE);
		
		change_type(par, OT_SFILE);
		sort_children(SRCHEAD_OBJCODE, src_cmp);
		ensure_visible(par);
		break;
	case MR_DISPLAY_SOURCE:
		show_source(fil, 1);
		break;
	case MR_SHOW_PATH:
		show_source_path_assumed(fil, TRUE);
		break;
	case MR_SHOW_SOURCE_PATH:
		show_source_path_used(fil, TRUE);
		break;
	case MR_REMATCH_SOURCE_FILE:
		if (fil->fi_name)
		  open_source_file(fil, TRUE, FALSE);
		show_source(fil, 1);
		break;
	case MR_RELOAD_SOURCE_FILE:
		if (fil->fi_name)
		  open_source_file(fil, FALSE, TRUE);
		show_source(fil, 1);
		break;
	case MR_FILE_DATES:
		show_file_and_target_dates(fil, NULL);
		break;
	default:
		panic("bad cmd in df");
	}
}

void
hide_source_vars()
{
	objid_t obj;

	obj = get_code(SRCHEAD_OBJCODE, OBJ_CHILD);
	for (; obj != NULL; obj = get_code(obj, OBJ_NEXT)) {
		remove_object(obj, OBJ_DESCENDENTS);
		change_type(obj, OT_SFILE);
	}
	sort_children(SRCHEAD_OBJCODE, src_cmp);
}

int
show_source_path_assumed(fil, show_error)
     fil_t *fil;
     bool show_error;

{
  if (!fil)
    return 0;

  if (fil->fi_name && fil->fi_name[0] == '/')
    errf("\b%s", fil->fi_name);
  else
    if (fil && fil->fi_path_hint && fil->fi_name_only)
      errf("\b%s%s",fil->fi_path_hint, fil->fi_name_only);
    else
    {
      if (show_error)
	errf("\b(unknown)");
      return 0;
    }
  return 1;
}

int
show_source_path_used(fil, show_error)
     fil_t *fil;
     bool show_error;
{
  char buff[MAXPATHLEN + 1];

  if (!fil)
    return 0;

  if (construct_source_path_used(fil, buff))
    errf("\b%s", buff);
  else
  {
    if (show_error)
      errf("\b(unknown)");
    return 0;
  }
  return 1;
}


int
construct_source_path_used(fil, buff)
     fil_t *fil;
     char *buff;
{
  if (!fil)
    return 0;

  if (fil->fi_name && fil->fi_name[0] == '/')
    sprintf(buff, "%s", fil->fi_name);
  else
    if (fil && fil->fi_source_path && fil->fi_name_only)
      if (fil->fi_source_path[strlen(fil->fi_source_path)-1] == '/')
	sprintf(buff, "%s%s",fil->fi_source_path, fil->fi_name_only);
      else
	sprintf(buff, "%s/%s",fil->fi_source_path, fil->fi_name_only);
    else
      return 0;
  return 1;
}

void
srcfile_getcolor(obj, wn, p_fg, p_bg)
objid_t obj;
int wn;
long *p_fg, *p_bg;
{
  static color_t color= { -1, 0, 0, 0 };

  if (color.co_pixel == -1)
  {
    const char *colorname;
    
    color.co_pixel = WN_FG(wn);
    if (wn_use_extended_text_colors() == TRUE)
    {
      colorname = wn_get_default("SourceFileTextColor");
      if (colorname != NULL)
      {
	wn_parse_color(wn, colorname, &color);
	if (!wn_alloc_color(wn, &color,colorname))
	  color.co_pixel = WN_FG(wn);
      }
    }
  }
  *p_fg = color.co_pixel;
  *p_bg = WN_BG(wn);
}


void
func_getcolor(obj, wn, p_fg, p_bg)
objid_t obj;
int wn;
long *p_fg, *p_bg;
{
  static color_t color= { -1, 0, 0, 0 };

  if (color.co_pixel == -1)
  {
    const char *colorname;
    
    color.co_pixel = WN_FG(wn);
    if (wn_use_extended_text_colors() == TRUE)
    {
      colorname = wn_get_default("FunctionTextColor");
      if (colorname != NULL)
      {
	wn_parse_color(wn, colorname, &color);
	if (!wn_alloc_color(wn, &color,colorname))
	  color.co_pixel = WN_FG(wn);
      }
    }
  }
  *p_fg = color.co_pixel;
  *p_bg = WN_BG(wn);
}

void
reset_filesize_operations()
{
  srcfile_getsize_internal(NULL, NULL, NULL, TRUE); /* reset */
  module_getsize_internal(NULL, NULL, NULL, TRUE); /* reset */
  Max_src_width = 0;
  Max_module_width = 0;
}
