/* st_lookup.c - look up things by name or address in the symbol tables */

/*  Copyright 1992 Mark Russell, University of Kent at Canterbury.
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


/* @(#)st_lookup.c	1.8 04 Jun 1995 (UKC) */
char ups_st_lookup_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include <local/wn.h>
#include <local/ukcprog.h>
#include <local/arg.h>
#include <local/obj/obj.h>	/* for OBJ_UPDATING_OFF */
#include <mtrprog/strcache.h>
#include <mtrprog/alloc.h>
#include <mtrprog/genmergesort.h>

#include "cursors.h"
#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "ci_util.h"
#include "st.h"
#include "target.h"
#include "state.h"
#include "srcwin.h"
#include "ui.h"
#include "expr.h"
#include "tdr.h"

#define CMP_STRCMP 0
#define CMP_STRNCMP 1
#define CMP_STRSTR 2

/*  List of globals added to by find_matching_funcs() and
 *  find_matching_globals().
 */
typedef struct global_list_s {
	const char *gl_name;
	Module *gl_module;
	fil_t *gl_fil;
	func_t *gl_func;
	var_t *gl_var;
	taddr_t gl_addr;
	const char *st_name;
	struct global_list_s *gl_next;
} global_list_t;

static bool filename_matches PROTO((const char *st_name,
				    fil_t *fil, fil_t *matchfil,
				    char *name, size_t namelen,
				    int cmp_style));
static void find_matching_funcs PROTO((symtab_t *st,
				       fil_t *matchfil, const char *filename,
				       size_t flen, const char *funcname,
				       global_list_t **p_glhead,
				       bool *p_found_file, bool use_demangled_name));
static void find_matching_funcs_only PROTO((symtab_t *st,
					    func_t *funclist,
					    fil_t *matchfil, const char *filename,
					    int flen, const char *funcname,
					    func_bp_list_t **funchead,
					    int cmp_style));
static void find_matching_globals PROTO((target_t *xp, fil_t *matchfil,
					 const char *filename,
					 size_t filename_len,
					 const char *varpat,
					 const char *varname,
					 global_list_t **p_glhead));
static void addvar PROTO((char *arg, var_t *v));
static void scan_symtabs PROTO((fil_t *fil, const char *filename,
				size_t filename_len, const char *funcpat,
				const char *funcname,
				bool want_vars, global_list_t **p_glhead,
				bool *p_found_file, bool all_syms,
				bool use_demangled_name));
static void add_var_to_global_list PROTO((global_list_t **p_glhead,
					  var_t *v, fil_t *fil,
					  const char *st_name));
static int glcmp PROTO((global_list_t *f1, global_list_t *f2));
static void add_common_block_globals PROTO((const char *name, func_t *f,
					    common_block_t **p_cblock,
					    global_list_t **p_glhead));
static void dupf_mesg PROTO((const char *name,
			     global_list_t *gl1, global_list_t *gl2));
static bool modname_matches PROTO((Module *m, const char *name, 
                                   size_t namelen));
static void add_func_to_global_list PROTO((global_list_t **p_glhead, func_t *f, 
                                           Module *module, const char *st_name,
					   bool use_demangled_name));
static void add_to_global_list PROTO((global_list_t **p_glhead, 
                                      const char *name, taddr_t addr, 
                                      fil_t *fil, Module *module,
				      const char *st_name));
static bool have_exact_match PROTO((global_list_t *glhead, const char *name));
static int flcmp PROTO((func_bp_list_t *fl1, func_bp_list_t *fl2));
static int funcname_cmp PROTO((const char *name1, const char *name2));


GENERIC_MERGE_SORT(static,sort_global_list,global_list_t,gl_next)

ALLOC_NEW_FREELIST(static,global_list_t,global_list,gl_next)

GENERIC_MERGE_SORT(static,sort_func_bp_list,func_bp_list_t,fl_next)

ALLOC_NEW_FREELIST(static,func_bp_list_t,func_bp_list,fl_next)

static bool
filename_matches(st_name, fil, matchfil, name, namelen, cmp_style)
const char *st_name;
fil_t *fil, *matchfil;
char *name;
size_t namelen;
int cmp_style;
{
	int len;
	bool match = FALSE;
	const char *file_name;
	char tmp;

	if (matchfil != NULL)
		return fil == matchfil;

	if (name == NULL)
		return TRUE;
	if (fil == NULL)
	{
	  if (namelen == 0)
	    return FALSE;
	  else
	    file_name = st_name;
	}
	else
	  file_name = fil->fi_name;
	if (namelen == 0)
		return TRUE;

	len = strlen(file_name);

	if (len < namelen)
	  return FALSE;
	else
	{
	  switch (cmp_style)
	  {
	  case CMP_STRCMP:
	    if (memcmp(&file_name[len - namelen], name, namelen) != 0)
	      return FALSE;
	    break;
	  case CMP_STRNCMP:
	    if (memcmp(file_name, name, namelen - 1) != 0)
	      return FALSE;
	    else
	      return TRUE;
	  case CMP_STRSTR:
	    tmp = name[namelen - 1];
	    name[namelen - 1] = 0;
	    match = !!strstr(file_name, name + 1);
	    name[namelen - 1] = tmp;
	    return match;
	  }
	}
	return len == namelen || file_name[len - namelen - 1] == '/';
}

static bool
modname_matches(m, name, namelen)
Module *m;
const char *name;
size_t namelen;
{
	size_t len;
	const char *modname;

	if (name == NULL)
		return TRUE;
	
	modname = get_module_name(m);
	len = strlen(modname);

	return len == namelen && memcmp(modname, name, namelen) == 0;
}

static void
find_matching_funcs(st, matchfil, filename, flen, funcpat,
		    p_glhead, p_found_file, use_demangled_name)
symtab_t *st;
fil_t *matchfil;
const char *filename;
size_t flen;
const char *funcpat;
global_list_t **p_glhead;
bool *p_found_file;
bool use_demangled_name;
{
	func_t *f;
	Module *m;

	for (f = st->st_funclist; f != NULL; f = f->fu_next) {
		if (filename_matches(st->st_path, f->fu_fil, matchfil,
				     (char *)filename, flen, CMP_STRCMP)) {
			*p_found_file = TRUE;

			if (arg_match(use_demangled_name ? f->fu_demangled_name :
				      f->fu_name, funcpat)) {
				add_func_to_global_list(p_glhead, f,
							(Module *)NULL,
							st->st_path,
							use_demangled_name);
			} else if (use_demangled_name
				    && f->fu_fil && IS_FORTRAN(f->fu_fil->fi_language)
				    && arg_match(f->fu_name, funcpat)) {
				add_func_to_global_list(p_glhead, f,
							(Module *)NULL,
							st->st_path,
							use_demangled_name);
			}
		}
	}

	for (m = st->st_modules; m != NULL; m = get_next_module(m)) {
		if (modname_matches(m, filename, flen)) {
			funclist_t *fl;

			*p_found_file = TRUE;
			
			for (fl = get_module_funclist(m);
			     fl != NULL;
			     fl = fl->fl_next) {
				if (arg_match(use_demangled_name ?
					      fl->fl_func->fu_demangled_name :
					      fl->fl_func->fu_name, funcpat)) {
					add_func_to_global_list(p_glhead,
								fl->fl_func, m,
								st->st_path,
								use_demangled_name);
				}
			}
		}
	}
}

#ifdef OS_SUNOS_5
int
find_duplicate_file(fil, fil1)
     fil_t *fil;
     fil_t **fil1;
{
  target_t *xp;
  symtab_t *st;

  xp = get_current_target();
  st = NULL;

  if (fil)
    while (xp_next_symtab(xp, st, TRUE, &st)) 
      for (*fil1 = st->st_sfiles; *fil1; *fil1 = (*fil1)->fi_next)
      {
	if (fil != *fil1 && (fil->fi_flags & FI_DUPLICATE) &&
	    ((*fil1)->fi_flags & FI_DUPLICATE) &&
	    !strcmp(fil->fi_name, (*fil1)->fi_name))
	  return 1;
      }
  return 0;
}

void
mark_duplicate_files()
{
  target_t *xp;
  symtab_t *st_dyn, *st;
  fil_t *fil, *fil2;

  xp = get_current_target();
  st_dyn = NULL;

  while (xp_next_symtab(xp, st_dyn, TRUE, &st_dyn))
    if (st_dyn->st_sfiles && st_dyn->st_modtime > xp->xp_modtime)
    {
      st = NULL;
      while (xp_next_symtab(xp, st, TRUE, &st))
	if (st != st_dyn)
	  for (fil = st->st_sfiles; fil; fil = fil->fi_next)
	    if (fil->fi_name && !(fil->fi_flags & FI_HEADER))
	      for (fil2 = st_dyn->st_sfiles; fil2; fil2 = fil2->fi_next)
	      {
		if (/*!((fil->fi_flags & FI_DUPLICATE) &&
		      (fil2->fi_flags & FI_DUPLICATE)) &&*/
		    fil2->fi_name  && !(fil2->fi_flags & FI_HEADER) &&
		    !strcmp(fil->fi_name, fil2->fi_name))
		{
		  if (st->st_modtime > xp->xp_modtime)
		    if (st_dyn->st_modtime > st->st_modtime)
		      st->st_eclipsed = 1;
		    else
		      st_dyn->st_eclipsed = 1;
		  fil->fi_flags |= FI_DUPLICATE;
		  fil2->fi_flags |= FI_DUPLICATE;
		}
	      }
    }
}
#endif /* OS_SUNOS_5 */

fil_t *
name_to_fil(name)
const char *name;
{
	target_t *xp;
	symtab_t *st;

	xp = get_current_target();
	st = NULL;
	
	while (xp_next_symtab(xp, st, TRUE, &st)) {
		fil_t *fil;

		for (fil = st->st_sfiles; fil != NULL; fil = fil->fi_next) {
			if (strcmp(fil->fi_name_only, name) == 0) /* RGA */
				return fil;
		}
	}

	return NULL;
}

fil_t *
name_and_st_to_fil(name, st)
const char *name;
symtab_t *st;
{
	fil_t *fil;

	for (fil = st_get_fi(st); fil != NULL; fil = fil->fi_next) {
		if (strcmp(fil->fi_name_only, name) == 0) /* RGA */
			return fil;
	}

	return NULL;
}

func_t *
name_and_fil_to_func(name, flist, use_demangled_name)
const char *name;
fil_t *flist;
bool use_demangled_name;
{
	funclist_t *fl;
	fil_t *fil;

	for (fil = flist; fil != NULL;)
	{
	  for (fl = fil->fi_funclist; fl != NULL; fl = fl->fl_next)
	  {
	    if (use_demangled_name)
	    {
	      if (strcmp(fl->fl_func->fu_demangled_name, name) == 0)
		return fl->fl_func;
	    }
	    else
	      if (strcmp(fl->fl_func->fu_name, name) == 0)
		return fl->fl_func;
	  }
	  if (fil->fi_language == LANG_CC)
	    fil = fil->fi_next;
	  else
	    break;
	}

	return NULL;
}

int
resolve_untyped_name(name, p_v)
const char *name;
var_t **p_v;
{
	global_list_t *glhead;

	glhead = NULL;
	find_matching_globals(get_current_target(), (fil_t *)NULL,
			      (const char *)NULL, 0, name, name, &glhead);

	/*  If there is more than one match, just pick the first one.
	 *
	 *  TODO: Pick best one.
	 */
	if (glhead != NULL) {
		*p_v = glhead->gl_var;
	}
	else {
		bool found_file;

		scan_symtabs(NULL, NULL, 0, name, name, FALSE, &glhead,
			     &found_file, TRUE, TRUE);

		if (glhead != NULL) {
			type_t *type = ci_make_type(Parse_apool, DT_FUNC_RETURNING);
			funcret_t *fr = (funcret_t *)alloc(Parse_apool, sizeof(funcret_t));

			(void)FU_BLOCKS(glhead->gl_func);

			*p_v = ci_make_var(Parse_apool,
					   alloc_strdup(Parse_apool, name),
					   CL_FUNC, type,
					   glhead->gl_func->fu_addr);

			type->ty_base = glhead->gl_func->fu_type;
			type->ty_funcret = fr;
			fr->fr_is_old_style = 1;
			fr->fr_params_type = FDT_IDLIST;
			fr->fr_nparams = -1;
			fr->fr_is_variadic = 0;
			fr->fr_params = NULL;
			fr->fr_idlist = NULL;
		}
		else {
			*p_v = NULL;
		}
	}
	
	free_global_list_list(glhead);

	return (*p_v != NULL) ? 0 : -1;
}

static void
find_matching_globals(xp, matchfil, filename, filename_len, varpat, varname,
		      p_glhead)
target_t *xp;
fil_t *matchfil;
const char *filename;
size_t filename_len;
const char *varpat, *varname;
global_list_t **p_glhead;
{
	symtab_t *st;
	int wants_stop = 0;
        cursor_t old_cursor = wn_get_window_cursor(WN_STDWIN);
	set_bm_cursor(WN_STDWIN, CU_WAIT);

	/*  First search the source files of all the symbol tables.
	 */
	st = NULL;
	update_target_menu_state(TS_SEARCHING, xp_is_attached(xp));
	stop_pressed(0, 1);	/* reset */

	while (!wants_stop &&
	       xp_next_symtab(xp, st, !have_exact_match(*p_glhead, varname), &st)) {
		fil_t *fil;
		
		if (lookup_global_addr(st, varpat))
			st_get_fi(st);
                
		for (fil = st->st_sfiles;
		     fil != NULL && !wants_stop; fil = fil->fi_next) {
			var_t *v;

			if (!filename_matches(st->st_path, fil, matchfil,
					      (char *)filename,
					      filename_len, CMP_STRCMP))
				continue;

			if (!st_fil_may_have_matching_globals(fil, varpat,
							      arg_match))
				continue;

			if (!matchfil && fil->fi_funclist) /* RGA testing... */
				continue;

			v = FI_VARS(fil);
			if (stop_pressed(0, 0))
			{
			  wants_stop = 1;
			  break;
			}
			for (; v != NULL; v = v->va_next) {
				if (arg_match(v->va_name, varpat)) {
					add_var_to_global_list(p_glhead,
							       v, fil,
							       st->st_path);
				}
			}
		}
	}

	st = NULL;
	while (!wants_stop &&
	       xp_next_symtab(xp, st, !have_exact_match(*p_glhead, varname), &st)) {
		fil_t *fil;
		
		for (fil = st->st_sfiles;
		     fil != NULL && !wants_stop; fil = fil->fi_next) {
			var_t *v;

			if (!filename_matches(st->st_path, fil, matchfil,
					      (char *)filename,
					      filename_len, CMP_STRCMP))
				continue;

			if (!st_fil_may_have_matching_globals(fil, varpat,
							      arg_match))
				continue;

			v = FI_VARS(fil);
			if (stop_pressed(0, 0))
			{
			  wants_stop = 1;
			  break;
			}
			for (; v != NULL; v = v->va_next) {
				if (arg_match(v->va_name, varpat)) {
					add_var_to_global_list(p_glhead,
							       v, fil,
							       st->st_path);
				}
			}
			if (have_exact_match(*p_glhead, varname))
			    wants_stop = 1;
		}
	}

	if (target_menu_search_disabled(0, 0) == FALSE)
	  update_target_menu_state(xp_get_state(xp), xp_is_attached(xp));
	wn_define_cursor(WN_STDWIN, old_cursor);
	if (matchfil == NULL && filename == NULL &&
	    !have_exact_match(*p_glhead, varname)) {
		/*  Not found under any source file, so search the
		 *  untyped variables.
		 */
		st = NULL;
		while (xp_next_symtab(xp, st,
				      !have_exact_match(*p_glhead, varname),
				      &st)) {
			scan_addrlist(st, arg_match, varpat, addvar,
				      (char *)p_glhead);
		}
	}
}

static void
addvar(arg, v)
char *arg;
var_t *v;
{
	add_var_to_global_list((global_list_t **)arg, v, (fil_t *)NULL, 0);
}

static void
add_var_to_global_list(p_glhead, v, fil, st_name)
global_list_t **p_glhead;
var_t *v;
fil_t *fil;
const char *st_name;
{
	add_to_global_list(p_glhead, v->va_name, v->va_addr,
			   fil, (Module *)NULL, st_name);
	(*p_glhead)->gl_var = v;
}

static void
add_func_to_global_list(p_glhead, f, module, st_name, use_demangled_name)
global_list_t **p_glhead;
func_t *f;
Module *module;
const char *st_name;
bool use_demangled_name;
{
	st_ensure_fu(f);
	add_to_global_list(p_glhead,
			   (use_demangled_name ? f->fu_demangled_name : f->fu_name),
			   f->fu_addr, f->fu_fil, module, st_name);
	(*p_glhead)->gl_func = f;
}

static void
add_to_global_list(p_glhead, name, addr, fil, module, st_name)
global_list_t **p_glhead;
const char *name;
taddr_t addr;
fil_t *fil;
Module *module;
const char *st_name;
{
	global_list_t *gl;

	gl = new_global_list();
	gl->gl_name = name;
	gl->gl_fil = fil;
	gl->gl_module = module;
	gl->gl_func = NULL;
	gl->gl_var = NULL;
	gl->gl_addr = addr;
	gl->gl_next = *p_glhead;
	gl->st_name = st_name;
	*p_glhead = gl;
}

/*  Search for the common block where the variable called name
 *  is defined.  We loop over all the symbol tables, searching
 *  the common block lists of each.  (At present there are no
 *  shared library FORTRAN executables, so the only symbol
 *  table with a common block list will be the main one).
 */
static void
add_common_block_globals(name, f, p_cblock, p_glhead)
const char *name;
func_t *f;
common_block_t **p_cblock;
global_list_t **p_glhead;
{
	target_t *xp;
	symtab_t *st;

	xp = get_current_target();
	st = NULL;

	while (xp_next_symtab(xp, st, TRUE, &st)) {
		var_t *v;
		fil_t *fil;

		global_and_cblist_to_var(st->st_cblist, name, f,
					 p_cblock, &fil, &v);
		
		if (v != NULL) {
			add_var_to_global_list(p_glhead, v, fil, st->st_path);
			return;
		}
	}
}

static int
glcmp(gl1, gl2)
global_list_t *gl1, *gl2;
{
	int res;

	if ((res = strcmp(gl1->gl_name,  gl2->gl_name)) != 0)
		return res;
	
	if ((gl1->gl_func != NULL) != (gl2->gl_func != NULL))
		return (gl1->gl_func == NULL) ? -1 : 1;
	
	if (gl1->gl_func == NULL)
		return 0;

	if (gl1->gl_func == gl2->gl_func) {
		if ((gl1->gl_module != NULL) == (gl2->gl_module != NULL))
			return 0;
		
		return (gl1->gl_module != NULL) ? -1 : 1;
	}

	return addrcmp(gl1->gl_func, gl2->gl_func);
}

static bool
have_exact_match(glhead, name)
global_list_t *glhead;
const char *name;
{
	global_list_t *gl;
	
	for (gl = glhead; gl != NULL; gl = gl->gl_next) {
		if (strcmp(gl->gl_name, name) == 0)
			return TRUE;
	}

	return FALSE;
}

/*
 * Compare two function names ignoring any parameter definitions.
 * Return 0 if they match.
 */
static int
funcname_cmp(name1, name2)
const char *name1;
const char *name2;
{
    char *p1, *p2;
    int l1, l2;
    p1 = strchr(name1, '(');
    p2 = strchr(name2, '(');
    if ((p1 == NULL) && (p2 == NULL))
	return strcmp(name1, name2);
    if (p1 == NULL)
	l1 = strlen(name1);
    else
	l1 = p1 - name1;
    if (p2 == NULL)
	l2 = strlen(name2);
    else
	l2 = p2 - name2;
    if (l1 != l2)
	return strcmp(name1, name2);
    return strncmp(name1, name2, l1);
}

static void
scan_symtabs(matchfil, filename, filename_len, funcpat, funcname, want_vars,
	     p_glhead, p_found_file, all_syms, use_demangled_name)
fil_t *matchfil;
const char *filename;
size_t filename_len;
const char *funcpat, *funcname;
bool want_vars;
global_list_t **p_glhead;
bool *p_found_file;
bool all_syms;
bool use_demangled_name;
{
	target_t *xp;
	symtab_t *st;

	xp = get_current_target();

	st = NULL;
	/*
	**  RCB: Don't do blocking cursor here.
	**       There may not be any windows set up yet.
	*/
	update_target_menu_state(TS_SEARCHING, xp_is_attached(xp));
	stop_pressed(0, 1);	/* reset */
	while (xp_next_symtab(xp, st,
			      !have_exact_match(*p_glhead, funcname), &st)) {
	  	if (stop_pressed(0, 0))
		  break;
		find_matching_funcs(st, matchfil, filename, filename_len,
				    funcpat, p_glhead, p_found_file,
				    use_demangled_name);
	}

	if (want_vars && !stop_pressed(0, 0)) {
		find_matching_globals(xp, matchfil, filename,
				      filename_len, funcpat, funcname,
				      p_glhead);
	}
	if (target_menu_search_disabled(0, 0) == FALSE)
	  update_target_menu_state(xp_get_state(xp), xp_is_attached(xp));
}
					
int
find_func_by_name(name, p_f, p_f1, use_demangled_name)
const char *name;
func_t **p_f;
func_t **p_f1;
bool use_demangled_name;
{
	return find_global_by_name(name, (fil_t *)NULL, (func_t *)NULL,
				   (common_block_t *)NULL, FALSE,
				   p_f, p_f1, (var_t **)NULL, (var_t **)NULL,
				   (common_block_t **)NULL,
				   (fil_t **)NULL, (fil_t **)NULL, use_demangled_name);
				
}

/*  We search all the symbol tables including those in the cache.
 *  This is so we can match names in shared libraries (like printf)
 *  before the target has been started.
 *  Returns -1 if item not found
 */
int
find_global_by_name(name, fil, f, cblock, exact, p_f, p_f1, p_v, p_v1, p_cblock, p_fil, p_fil1, use_demangled_name)
const char *name;
fil_t *fil;
func_t *f;
common_block_t *cblock;
bool exact;
func_t **p_f, **p_f1;
var_t **p_v, **p_v1;
common_block_t **p_cblock;
fil_t **p_fil, **p_fil1;
bool use_demangled_name;
{
	global_list_t *gl, *gl1 = NULL, *glhead, *prev;
	global_list_t *exact_gls[2], *partial_gls[2], *exact_extern_gl;
	int matches, exact_matches, exact_extern_matches, partial_matches;
	size_t filename_len;
	const char *filename, *funcpat;
	char *infile, *funcname, *funcname_expanded;
	char c, c1;
	int ok = 1, ret = -1, len;
	bool found_file;
	int funcname_len;

	if (p_f1)
	  *p_f1 = NULL;
	if (p_v1)
	  *p_v1 = NULL;
	if (p_fil1)
	  *p_fil1 = NULL;
	
	if ((funcname = strchr(name, ':')) != NULL && funcname[1] != ':') {
		if (funcname[1] == '\0') {
			errf("Missing function name after source file");
			return -1;
		}
		
		filename = name;
		filename_len = funcname - name; 
		++funcname;
	}
	else {
		filename = NULL;
		filename_len = 0;	/* to satisfy gcc */
		funcname = (char *)name;
	}

	if (exact) {
		funcpat = funcname;
	}
	else {
	  /* RGA removed "[]" from globchars to allow for operator[] methods */
		static const char globchars[] = "*?";
		const char *s, *globname;
		char *namecopy;

		for (s = funcname; s != '\0'; ++s) {
			if (strchr(globchars, *s) != NULL)
				break;
		}
		
		if (*s == '\0') {
			namecopy = strf("%s*", funcname);
			globname = namecopy;
		}
		else {
			namecopy = NULL;
			globname = funcname;
		}

		funcpat = arg_do_quoting(&globname, globchars, TRUE);

		if (namecopy != NULL)
			free(namecopy);

		if (funcpat == NULL)
			return -1;
	}

	glhead = NULL;
	found_file = FALSE;

	if (p_v != NULL)
		*p_cblock = NULL;

	if (cblock != NULL) {
		var_t *v;
		
		for (v = get_cblock_vars(cblock); v != NULL; v = v->va_next) {
			if (arg_match(v->va_name, funcpat)) {
				add_var_to_global_list(&glhead, v, fil, 0);
				*p_cblock = cblock;
			}
		}
	}
	
	if ((glhead == NULL) && (fil != NULL) && (filename == NULL)) {
		scan_symtabs(fil, (const char *)NULL, 0, funcpat, funcname,
			     p_v != NULL, &glhead, &found_file, 1,
			     use_demangled_name);
	}

	scan_symtabs((fil_t *)NULL, filename, filename_len,
		     funcpat, funcname,
		     p_v != NULL, &glhead, &found_file, 0, use_demangled_name);

	if ((glhead == NULL) && exact && fil && IS_FORTRAN(fil->fi_language)) {
		/* C routines called from Fortran may have added underscore(s). */
		char *funcnam2;
		funcnam2 = strf("%s_", funcname);
		scan_symtabs((fil_t *)NULL, filename, filename_len, funcnam2, funcnam2,
			     p_v != NULL, &glhead, &found_file, 0, use_demangled_name);
		if (funcnam2 != NULL)
			free(funcnam2);
		funcnam2 = strf("%s__", funcname);
		scan_symtabs((fil_t *)NULL, filename, filename_len, funcnam2, funcnam2,
			     p_v != NULL, &glhead, &found_file, 0, use_demangled_name);
		if (funcnam2 != NULL)
			free(funcnam2);
	}

	if (p_v != NULL)
		add_common_block_globals(name, f, p_cblock, &glhead);


	/* If nothing found, give up. */

	if (glhead == NULL) {
		const char *orvar;

		orvar = (p_v != NULL) ? " or variable" : "";

		if (filename != NULL)
			errf("No function%s matching `%s' in %.*s",
				     orvar, funcname,
				     (int)filename_len, filename);
		else
			errf("No function%s `%s'", orvar, funcname);

		return -1;
	}


	matches = exact_matches = exact_extern_matches = partial_matches = 0;
	exact_extern_gl = NULL;		/* to satisfy gcc */

	for (gl = glhead; gl != NULL; gl = gl->gl_next)
		++matches;
	glhead = sort_global_list(glhead, matches, glcmp);

	prev = NULL;

	for (gl = glhead; gl != NULL; gl = gl->gl_next) {

		/*  We can get duplicate symbol table entries for a
		 *  function or variable - ignore these.
		 */
		if (prev != NULL && gl->gl_addr == prev->gl_addr &&
		    strcmp(prev->gl_name, gl->gl_name) == 0 &&
		    (prev->gl_func != NULL) == (gl->gl_func != NULL))
			continue;
		
		if (funcname_cmp(gl->gl_name, funcname) == 0) {
			if ((gl->gl_func != NULL) ?
				      (gl->gl_func->fu_flags & FU_STATIC) == 0 :
				      gl->gl_var->va_class == CL_EXT) {
				++exact_extern_matches;
				exact_extern_gl = gl;
			}

			if (exact_matches < 2)
				exact_gls[exact_matches] = gl;
			++exact_matches;
		}
		else {
			if (partial_matches < 2)
				partial_gls[partial_matches] = gl;
			++partial_matches;
		}

		prev = gl;
	}
	if (exact_matches == 1)
		gl = exact_gls[0];
	else if (partial_matches == 1 && exact_matches == 0)
		gl = partial_gls[0];
	else if (exact_extern_matches == 1)
		gl = exact_extern_gl;
	else
		gl = NULL;

	/* RGA fix for multiple "main" functions, where only second */
	/* has file info */
/*	if (gl && !gl->gl_fil && exact_matches > 1 &&
	    exact_gls[0]->gl_func != NULL &&
	    exact_gls[1]->gl_func != NULL)
	  if (!exact_gls[0]->gl_fil && exact_gls[1]->gl_fil)
	    gl->gl_fil = exact_gls[1]->gl_fil;
	  else
	    if (exact_gls[0]->gl_fil && !exact_gls[1]->gl_fil)
	      gl->gl_fil = exact_gls[0]->gl_fil;
	
	if (gl && !gl->gl_fil && partial_matches > 1 &&
	    partial_gls[0]->gl_func != NULL &&
	    partial_gls[1]->gl_func != NULL)
	  if (!partial_gls[0]->gl_fil && partial_gls[1]->gl_fil)
	    gl->gl_fil = partial_gls[1]->gl_fil;
	  else
	    if (partial_gls[0]->gl_fil && !partial_gls[1]->gl_fil)
	      gl->gl_fil = partial_gls[0]->gl_fil;*/


	/* RGA when duplicate function names exist, and the files are */
	/* the same, just use the first one found. This can happen when */
	/* a function is linked statically ahead of a dynamic library */
	/* where the same function is also defined. Then the link order */
	/* takes precedence */

	if (exact_matches > 1 &&
	    exact_gls[0]->gl_func != NULL &&
	    exact_gls[1]->gl_func != NULL &&
	    exact_gls[0]->gl_fil && exact_gls[1]->gl_fil &&
	    !strcmp(exact_gls[0]->gl_fil->fi_name,
		    exact_gls[1]->gl_fil->fi_name) &&
	    exact_gls[0]->gl_fil->fi_path_hint && /* russ@amc.com added */
	    exact_gls[1]->gl_fil->fi_path_hint && /* russ@amc.com added */
	    !strcmp(exact_gls[0]->gl_fil->fi_path_hint,
		    exact_gls[1]->gl_fil->fi_path_hint))
	{
	    gl = exact_gls[0];
	    gl1 = exact_gls[1];
	}

	if (exact_matches > 0 && partial_matches > 1 &&
	    partial_gls[0]->gl_func != NULL &&
	    partial_gls[1]->gl_func != NULL &&
	    !strcmp(partial_gls[0]->gl_name, partial_gls[1]->gl_name) &&
	    partial_gls[0]->gl_fil && partial_gls[1]->gl_fil &&
	    !strcmp(partial_gls[0]->gl_fil->fi_name,
		    partial_gls[1]->gl_fil->fi_name) &&
	    !strcmp(partial_gls[0]->gl_fil->fi_path_hint,
		    partial_gls[1]->gl_fil->fi_path_hint))
	{
	    gl = partial_gls[0];
	    gl1 = partial_gls[1];
	}

	if (exact_matches > 1)
	{
	    if (exact)
		dupf_mesg(funcname, exact_gls[0], exact_gls[1]);
	    else
		errf("\bUsing first one of multiple/overloaded symbols...");
	    for (gl = glhead; gl != NULL; gl = gl->gl_next) 
		if (gl->gl_func && !(gl->gl_func->fu_flags & FU_STATIC))
		    break;
	    gl = gl ? gl : exact_gls[0];
	    gl1 = exact_gls[1];
	}

	if (gl != NULL) {
		*p_f = gl->gl_func;
		if (p_v != NULL) {
			*p_v = gl->gl_var;
			*p_fil = gl->gl_var ? gl->gl_fil : NULL;
		}
		if (gl1 != NULL)
		{
		  if (p_f1 != NULL)
		    *p_f1 = gl1->gl_func;
		  if (p_v1 != NULL)
		  {
		    *p_v1 = gl1->gl_var;
		    *p_fil1 = gl1->gl_var ? gl1->gl_fil : NULL;
		  }
		}
		free_global_list_list(glhead);
		return 0;
	}

	infile = (filename != NULL && *filename != '\0')
				? strf(" in %.*s", (int)filename_len, filename)
				: strsave("");

	funcname_len = strlen(funcname);
	len = 0;
	c = glhead->gl_name[len];
	ok = !!c;
	while(ok)
	{
	  for (gl = glhead; c && ok && gl != NULL; gl = gl->gl_next)
	  {
	    c1 = gl->gl_name[len];
	    if (!c1 || c1 != c)
	      ok = 0;
	  }
	  if (ok)
	  {
	    len++;
	    c = glhead->gl_name[len];
	  }
	}
	if (len > funcname_len)
	{
	  handle_display_area_char('\005', FALSE); /* ^e */
	  handle_display_area_char('\025', FALSE); /* ^u */
	  funcname_len = 0;
	  c = glhead->gl_name[funcname_len];
	  ok = !!c;
	  while(ok)
	  {
	    c = glhead->gl_name[funcname_len];
	    if (!c)
	      ok = 0;
	    for (gl = glhead; c && ok && gl != NULL; gl = gl->gl_next)
	    {
	      c1 = gl->gl_name[funcname_len];
	      if (!c1 || c1 != c)
		ok = 0;
	    }
	    if (ok)
	    {
	      ret = -2;
	      handle_display_area_char(c, FALSE);
	      funcname_len++;
	    }
	  }
	}
	funcname_expanded = e_malloc(funcname_len + 1);
	strncpy(funcname_expanded, partial_gls[0]->gl_name, funcname_len);
	funcname_expanded[funcname_len] = 0;
	if (partial_matches == 2) {
	  errf("`%s' matches both `%s' and `%s'%s",
	       funcname_expanded, partial_gls[0]->gl_name,
	       partial_gls[1]->gl_name, infile);
	}
	else {
	  /* RGA added message about SHIFT-ESC */
	  errf("`%s' matches `%s' and %d more. Use SHIFT-(ESC|RET) for list%s",
	       funcname_expanded, partial_gls[0]->gl_name,
	       partial_matches - 1, infile);
	}
	free(funcname_expanded);
	if (list_matches_to_outwin(0, 0)) /* dump matches to output win */
	{
	  char buff[256];
	  Outwin *ow;
	  
	  ow = get_or_create_outwin();

	  for (gl = glhead; gl != NULL; gl = gl->gl_next)
	  {
	    if (gl->gl_fil)
	      if (gl->gl_fil->fi_source_path)
		sprintf(buff, "%s%s%s:\t%s\n",
			gl->gl_fil->fi_source_path,
			gl->gl_fil->fi_source_path
			[strlen(gl->gl_fil->fi_source_path)-1] == '/' ? "": "/",
			gl->gl_fil->fi_name,
			gl->gl_name);
	      else
		sprintf(buff, "%s:\t%s\n",
			gl->gl_fil->fi_symtab->st_path,
			gl->gl_name);
	    else
	      if (gl->st_name)
		sprintf(buff, "%s:\t%s\n",gl->st_name, gl->gl_name);
	      else
		sprintf(buff, "%s\n",gl->gl_name);
	    outwin_insert(ow, buff, strlen(buff));
	  }
	}
	free_global_list_list(glhead);

	free(infile);
	return (ret);
}

static void
dupf_mesg(funcname, gl1, gl2)
const char *funcname;
global_list_t *gl1, *gl2;
{
	const char *what;
	fil_t *fil1, *fil2;

	if ((gl1->gl_func != NULL) != (gl2->gl_func != NULL))
		what = "Name";
	else
		what = (gl1->gl_func != NULL) ? "Function" : "Variable";

	fil1 = gl1->gl_fil;
	fil2 = gl2->gl_fil;

	if (gl1->gl_module != NULL && gl2->gl_module != NULL) {
		errf("\bWarning: %s `%s' appears in modules %s and %s",
		     what, funcname,
		     get_module_name(gl1->gl_module),
		     get_module_name(gl2->gl_module));
	}
	else if (fil1 != NULL && fil2 != NULL) {
		errf("\bWarning: %s `%s' appears in both %s and %s",
				what, funcname, fil1->fi_name, fil2->fi_name);
	}
	else if (fil1 != NULL && fil2 == NULL) {
		errf("\bWarning: %s `%s' appears in both %s and elsewhere",
				what, funcname, fil1->fi_name);
	}
	else if (fil2 != NULL && fil1 == NULL) {
		errf("\bWarning: %s `%s' appears in both %s and elsewhere",
		     what, funcname, fil2->fi_name);
	}
	else
		errf("\bWarning: %s `%s' appears more than once!",
		     what, funcname);
}

/*  Return a pointer to the func_t block of the function wherein
 *  lies address addr, or NULL if addr does not lie within any
 *  known function.  We search all the symbol tables.
 */
func_t *
addr_to_func(addr)
taddr_t addr;
{
	symtab_t *st;
	func_t *f;
	target_t *xp;

	xp = get_current_target();
	st = NULL;
	
	while (xp_next_symtab(xp, st, TRUE, &st)) {
		if (addr_and_functab_to_func(st->st_functab, addr, &f))
			return st_ensure_fu(f);
	}
	
	return NULL;
}

/*  Call (*func)() for each source file with the source file as an argument.
 *
 *  Do the source files in the cache as well.
 */
void
iterate_over_source_files(func, arg)
void (*func)PROTO((fil_t *fil, bool arg));
bool arg;
{
	static bool first_call = TRUE, call_level = 0;
	target_t *xp;
	symtab_t *st;
	fil_t *fil;
	cursor_t old_cursor = (cursor_t)NULL;

	call_level++;
	xp = get_current_target();
	st = NULL;

	if (first_call)
	{
	  old_cursor = wn_get_window_cursor(WN_STDWIN);
	  set_bm_cursor(WN_STDWIN, CU_WAIT);
	}
	while (xp_next_symtab(xp, st, TRUE, &st)) {
		for (fil = st->st_sfiles; fil != NULL; fil = fil->fi_next) {
		  if (first_call && call_level == 1)
		    scan_elf_dot_o_file_symtab(fil);
		  if ((fil->fi_flags & FI_HIDE_ENTRY) == 0)
		    (*func)(fil, arg);
		}
	}
	if (first_call)
	  wn_define_cursor(WN_STDWIN, old_cursor);
	first_call = FALSE;
	call_level--;
}

/*  Call (*func)(cblock) for each common block.
 */
void
iterate_over_common_blocks(func)
void (*func)PROTO((common_block_t *cblock));
{
	target_t *xp;
	symtab_t *st;

	xp = get_current_target();
	st = NULL;

	while (xp_next_symtab(xp, st, TRUE, &st))
		iterate_over_cblist(st->st_cblist, func);
}

void
iterate_over_all_modules(func)
void (*func)PROTO((Module *module));
{
	target_t *xp;
	symtab_t *st;

	xp = get_current_target();
	st = NULL;

	while (xp_next_symtab(xp, st, TRUE, &st))
		iterate_over_modules(st->st_modules, func);
}

static void
find_matching_funcs_only(st, funclist, matchfil, filename,
			 flen, funcpat, funchead, cmp_style)
symtab_t *st;
func_t *funclist;
fil_t *matchfil;
const char *filename;
int flen;
const char *funcpat;
func_bp_list_t **funchead;
int cmp_style;

{
	func_t *f;
	Module *m;

	for (f = st->st_funclist; f != NULL; f = f->fu_next) {
		if (filename_matches(st->st_path, f->fu_fil, matchfil,
				     (char *)filename, flen, CMP_STRCMP)) {
			if (arg_match(f->fu_demangled_name, funcpat))
			{
			  func_bp_list_t *fl;
			  
			  fl = new_func_bp_list();
			  fl->fl_func = f;
			  fl->fl_bp = 0;
			  fl->fl_next = *funchead;
			  *funchead = fl;
			}
		      }
	      }

	for (m = st->st_modules; m != NULL; m = get_next_module(m)) {
		if (modname_matches(m, filename, flen)) {
			funclist_t *fl;

			for (fl = get_module_funclist(m);
			     fl != NULL;
			     fl = fl->fl_next) {
				if (arg_match(fl->fl_func->fu_demangled_name, funcpat)) {
				  func_bp_list_t *fl2;
				  
				  fl2 = new_func_bp_list();
				  fl2->fl_func = f;
				  fl2->fl_bp = 0;
				  fl2->fl_next = *funchead;
				  *funchead = fl2;
				}
			}
		}
	}
}

static int
flcmp(fl1, fl2)
func_bp_list_t *fl1, *fl2;
{
	int res;

	if ((res = strcmp(fl1->fl_func->fu_demangled_name,  fl2->fl_func->fu_demangled_name)) != 0)
		return res;
	
	if ((fl1->fl_func != NULL) != (fl2->fl_func != NULL))
		return (fl1->fl_func == NULL) ? -1 : 1;
	
	if (fl1->fl_func == NULL)
		return 0;

	return addrcmp(fl1->fl_func, fl2->fl_func);
/*	return strcmp(fl1->fl_func->fu_demangled_name, fl2->fl_func->fu_demangled_name);*/
}

void
get_matching_func_bp_list(name, funchead)
const char *name;
func_bp_list_t **funchead;
{
  symtab_t *st;
  /* RGA removed "[]" from globchars to allow for operator[] methods */
  static const char globchars[] = "*?";
  const char *s, *globname, *filename, *funcpat;
  char *funcname, *namecopy, *c;
  int matches, filename_len, cmp_style = CMP_STRCMP;
  func_bp_list_t *fl;
  target_t *xp;
  
  xp = get_current_target();
  
  st = NULL;
  if ((funcname = strchr(name, ':')) != NULL && funcname[1] != ':') {
    if (funcname[1] == '\0') {
      errf("Missing function name after source file");
      return;
    }
		
    filename = name;
    filename_len = funcname - name;
    if (filename[0]  == '*')
      cmp_style = CMP_STRSTR;
    else
      if (filename[filename_len - 1]  == '*')
	cmp_style = CMP_STRNCMP;
    ++funcname;
  }
  else {
    filename = NULL;
    filename_len = 0;		/* to satisfy gcc */
    funcname = (char *)name;
  }
  
  for (s = funcname; s != '\0'; ++s) {
    if (strchr(globchars, *s) != NULL)
      break;
  }

  /* RGA append "*" to C++ names like "array<array<int> *>::" ... */
  if (*s == '\0' || ((c = strchr(name, ':')) != NULL && c[1] == ':')) {
    namecopy = strf("%s*", funcname);
    globname = namecopy;
  }
  else {
    namecopy = NULL;
    globname = funcname;
  }
  
  funcpat = arg_do_quoting(&globname, globchars, TRUE);
  
  if (namecopy != NULL)
    free(namecopy);
  
  if (funcpat == NULL)
    return ;
    
  while (xp_next_symtab(xp, st, TRUE, &st)) 
    find_matching_funcs_only(st, st->st_funclist,
			     (fil_t *)NULL, filename, filename_len,
			     funcpat, funchead, cmp_style);
  
				/* sort list so duplicates are together */
  matches = 0;
  for (fl = *funchead; fl != NULL; fl = fl->fl_next)
    ++matches;
  *funchead = sort_func_bp_list(*funchead, matches, flcmp);
}

void
do_free_func_bp_list_list(funchead)
  func_bp_list_t *funchead;
{
  free_func_bp_list_list(funchead);
}

int
find_and_show_matching_globals(fil, fi_name, fi_name_len, name)
     fil_t *fil;
     const char *fi_name;
     int fi_name_len;
     char *name;
{
  global_list_t *glhead = 0, *gl;
  int found, oldstate;
  const char *funcpat;
  char *funcname;
  static const char globchars[] = "*?";
  const char *globname;
  
  funcname = (char *)name;
  globname = funcname;
  funcpat = arg_do_quoting(&globname, globchars, TRUE);

  find_matching_globals(get_current_target(), fil, fi_name, fi_name_len,
			funcpat, funcname, &glhead);
  found = !!glhead;
  if (found)
  {
    oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
    clear_selection();
    for (gl = glhead; gl != NULL; gl = gl->gl_next) 
    {
      if (gl->gl_var)
      {
	show_given_global(gl->gl_fil, gl->gl_var);
      }
    }
    td_set_obj_updating(oldstate);
  }
  if (glhead)
    free_global_list_list(glhead);
  return found;
}

const char *
macro_expand_word(fil, lnum, w, wl)
fil_t *fil;
int lnum;
const char *w;
int wl;
{
   const macro_t *ma;
   const macro_value_t *mav;
   
   for (ma = FI_MACROS(fil); ma != NULL; ma = ma->ma_next)
      if (strlen(ma->ma_name) == wl && memcmp(ma->ma_name, w, wl) == 0)
	 for (mav = ma->ma_values; mav != NULL; mav = mav->mav_next)
 	    if (lnum == 0 ||
		(lnum >= mav->mav_start_lnum &&
	         (mav->mav_end_lnum == 0 || lnum <= mav->mav_end_lnum)))
	       return mav->mav_value;
   
   return NULL;
}

char *
macro_expand_string(fil, lnum, s)
fil_t *fil;
int lnum;
const char *s;
{
   bool expanded = TRUE;
   char *result = NULL;
   
   if (FI_MACROS(fil) == NULL) return NULL;

   while (expanded) {
      const char *start = result ? result : s;
      const char *ptr = start;

      expanded = FALSE;
      
      while (*ptr && !expanded) {
	 while (*ptr && !isalpha(*ptr) && *ptr != '_') ptr++;
	 
	 if (*ptr) {
	    const char *word = ptr++;
            const char *expansion;

	    while (isalnum(*ptr) || *ptr == '_') ptr++;
	    
	    if ((expansion = macro_expand_word(fil, lnum, word, ptr - word)) != NULL) {
	       char *new = e_malloc(strlen(start) + strlen(expansion) - (ptr-word) + 1);
	       
	       memcpy(new, start, word - start);
	       strcpy(new + (word - start), expansion);
	       strcat(new, ptr);

	       if (result) free(result);

	       result = new;
	       expanded = TRUE;
	    }
	 }
      }
   }

   return result;
}
