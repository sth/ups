/* expr.c - expression evaluation */

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


/* @(#)expr.c	1.39 04 Jun 1995 (UKC) */
char ups_expr_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <local/wn.h>
#include <local/obj/obj.h>
#include <local/ukcprog.h>
#include <local/obj/obj.h>
#include <local/obj/o_priv.h>
#include <local/obj/o_mkobj.h>

#include "objtypes.h"
#include "ups.h"
#include "symtab.h"
#include "srcwin.h"
#include "target.h"
#include "ui.h"
#include "va.h"
#include "obj_stack.h"
#include "st.h"
#include "obj_misc.h"
#include "state.h"
#include "expr.h"
#include "tdr.h"
#include "cursors.h"
#include "menudata.h"

typedef struct argst {
	char *arg_name;
	char **arg_names;
	struct argst *arg_next;
} arg_t;

/*  Arguments for find_func.
 */
typedef struct find_func_argsst {
	func_t *ffa_func;	/* argument */
	objid_t ffa_stack_obj;	/* result */
} find_func_args_t;

static int get_double_click_action PROTO((objid_t var_obj));
static int show_member PROTO((struct selst *sel, char **names, int *p_found_a_member));
static int skip_sref PROTO((const char *name, int sepc, const char **p_next));
static int skip_varname PROTO((const char *name, const char *seps, const char **p_next));
static char **parse_dot_expr PROTO((const char *name_start, int sepc));
static void free_names PROTO((char **names));
static arg_t *split_line PROTO((char *line));
static void free_arglist PROTO((arg_t *list));
static bool show_local PROTO((Srcwin *sw, func_t *f, int lnum,
			      const char *name, bool double_click));
static bool find_func PROTO((objid_t stack_obj, func_t *f, char *args));
static var_t *find_local_at_lnum PROTO((func_t *f, int lnum, const char *name,
					block_t **blocks));
static bool find_local_no_var_obj PROTO((objid_t par, func_t *f, char *args));
static bool find_local PROTO((objid_t par, func_t *f, char *args,
			      objid_t *p_var_obj));
static int find_all_locals PROTO((func_t *f, const char *name,
				  objid_t par, block_t *block,
				  bool *p_cleared_selection,
				  objid_t *p_var_obj));
static bool search_stack PROTO((
			bool (*func)(objid_t stack_obj, func_t *f, char *args),
			char *args));
/*static bool search_stack_files PROTO((Srcwin *sw, char *name));*/

static int show_single_member PROTO((const char *name, type_t **btype,
				     bool double_click));
static block_t *lnum_to_block PROTO((block_t *par, int lnum));
static void show_macro PROTO((const char *name, fil_t *fil, int lnum,
			      bool double_click));

/*  Versions of isalpha() and isalnum() which count '_' and '$' as letters.
 */
#define isvaralpha(c)	(isalpha(c) || (c) == '_' || (c) == '$')
#define isvaralnum(c)	(isalnum(c) || (c) == '_' || (c) == '$')

static
int get_double_click_action(var_obj)
objid_t var_obj;
{
  struct objst *obj, *ob;
  
  obj = code_to_obj(var_obj);	
  for (ob = obj->ob_child; ob; ob = ob->ob_next)
    if (!ob->ob_child)
      break;
  return (!obj->ob_child || !ob) ? MR_VAR_ACCELERATOR_ACTION : MR_VAR_COLLAPSE;
}

bool
get_varname(language, line, pos, p_start, p_lim)
language_t language;
const char *line, *pos;
size_t *p_start, *p_lim;
{
	const char *lim;
	bool seen_alpha;
	int startpos;

	startpos = pos - line;
	seen_alpha = FALSE;
	
	for (startpos = pos - line; startpos >= 0; --startpos) {
		if (!isvaralnum(line[startpos]))
			break;
		if (isvaralpha(line[startpos]))
			seen_alpha = TRUE;
	}
	
	if (!seen_alpha)
		return FALSE;

	if ((language == LANG_C || language == LANG_CC ||
	     IS_FORTRAN(language)) && startpos > 0) {
		const char *start;
                int dotc;

                dotc = (language == LANG_C || language == LANG_CC) ? '.' : '%';
                
		/*  While the variable is preceded by `var.' or `var->',
		 *  back over that as well.
		 *  RGA for C++, back up over `::' as well
		 */
		for (;;) {
			start = &line[startpos];

/*			while (startpos >= 0 && isspace(line[startpos]))*/
				/* RGA for purify */
			while (startpos > 0 && isspace(line[startpos]))
				--startpos;

			if (startpos <= 0)
				break;
			
			if (line[startpos] != dotc &&
			    (startpos < 1 ||
			     (startpos > 0 &&
			      !(line[startpos-1] == '-' ||
				(language == LANG_CC &&
				line[startpos-1] == ':'))) || 
			     !(line[startpos] == '>' ||
			       (language == LANG_CC &&
			       line[startpos] == ':'))))
			  break;

			if (startpos <= 0)
				break;
			
			if (line[startpos] == dotc)
				--startpos;
			else
				startpos -= 2;
			
			while (startpos >= 0 && isspace(line[startpos]))
				--startpos;

			seen_alpha = FALSE;
			for (; startpos >= 0; --startpos) {
				if (!isvaralnum(line[startpos]))
					break;
				if (isvaralpha(line[startpos]))
					seen_alpha = TRUE;
			}
			
			if (!seen_alpha)
				break;
		}

		startpos = start - line;
	}

	if (startpos < 0)
		startpos = 0;
	
	while (!isvaralpha(line[startpos]))
		++startpos;
	
	for (lim = pos + 1; *lim != '\0' && isvaralnum(*lim); lim++)
		;
	
	*p_start = startpos;
	*p_lim = lim - line;
	
	return TRUE;
}

/*  Map a source line number to a function.
 *
 *  Source line numbers can appear out of sequence due to #line directives,
 *  so we simply search every line in the file for an exact match, or
 *  failing that for the function containing the line the smallest distance
 *  after the one we want.
 */
func_t *
lnum_to_func(fil, lnum)
fil_t *fil;
int lnum;
{
	funclist_t *fl;
	unsigned mindist;
	func_t *f;

	f = NULL;
	mindist = (unsigned)-1;		/* a very large number */

	/* RGA for g++ files, templates will map multiple functions */
	/* to the same line numbers, so check the first line numbers */
	/* in each function first */

	if (fil->fi_language == LANG_CC || get_user_lang() == LANG_CC
	    || (fil->fi_language == LANG_UNKNOWN &&
		get_mainfunc_lang() == LANG_CC))
	  for (fl = fil->fi_funclist; fl != NULL; fl = fl->fl_next)
	  {
	    lno_t *ln;

	    ln = FU_LNOS(fl->fl_func);
	    if (ln && ln->ln_fil != fil)
	      continue;
	    if (ln && ln->ln_num == lnum)
	      return fl->fl_func;
	  }

	for (fl = fil->fi_funclist; fl != NULL; fl = fl->fl_next)
	{
	  lno_t *ln;
	  
	  for (ln = FU_LNOS(fl->fl_func); ln != NULL; ln = ln->ln_next)
	  {
	    if (ln->ln_fil != fil)
	      continue;
	    if (ln->ln_num == lnum)
	      return fl->fl_func;
	    
	    if (ln->ln_num > lnum && ln->ln_num - lnum < mindist) {
	      mindist = ln->ln_num - lnum;
	      f = fl->fl_func;
	    }
	  }
	}

	return f;
}

static block_t *
lnum_to_block(par, lnum)
block_t *par;
int lnum;
{
	unsigned mindist;
	block_t *bl, *nearest;

	mindist = (unsigned)-1;			/* a very large number */
	nearest = NULL;
	
	for (bl = par->bl_blocks; bl != NULL; bl = bl->bl_next) {
		if (bl->bl_start_lnum <= lnum &&
		    lnum - bl->bl_start_lnum < mindist) {
			nearest = bl;
			mindist = lnum - bl->bl_start_lnum;
		}
	}

	return (nearest != NULL) ? lnum_to_block(nearest, lnum) : par;
}

int
show_var(sw, fil, lnum, wholename, out_f, control_down, double_click)
Srcwin *sw;
fil_t *fil;
int lnum;
const char *wholename;
errf_ofunc_t out_f;
int control_down;
bool double_click;
{
	func_t *f;
	char **names, **nptr;
	bool oldval;
	int have_var, oldstate, sepc;
        static char seps[] = "?-> \t";
	type_t *btype = 0;
	int show_mem = 1;

	if (!isvaralpha(*wholename))
		panic("name botch in sv");
	
#ifdef ARCH_MIPS
	if (!(fil->fi_language == LANG_C || fil->fi_language == LANG_CC)) {
		errf("Sorry, can only show variables in C code");
		return;
	}
#endif

	if (macro_expand_word(fil, lnum, wholename, strlen(wholename)) != NULL) {
		show_macro(wholename, fil, lnum, double_click);
		return 1;
	}

        sepc = IS_FORTRAN(fil->fi_language) ? '%' : '.';
        seps[0] = sepc;
	names = ssplit(wholename, seps);

	oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
	oldval = td_set_select_recording(FALSE);

	f = lnum_to_func(fil, lnum);
	have_var = 0;
        /* Need to do this before looking at the variable because find_local */
	/* does a clear_selection() so the show_single_member() fails */
	if (fil->fi_language == LANG_CC &&
	    (strchr(wholename, '.') || strstr(wholename, "->") ||
	    strstr(wholename, "::")))
	{
	  for (nptr = names + 1; *nptr != NULL; ++nptr)
	  {
	    show_mem = show_single_member(*nptr, &btype, 
					  *(nptr + 1) ? FALSE : double_click); 
	  
	    if (fil->fi_language == LANG_CC && !show_mem && btype && 
		btype->ty_code == TY_STRUCT && btype->ty_aggr_or_enum->ae_tag)
	    {
	      func_t *f0, *f1;
	      char *class_name;
	      errf_ofunc_t oldf = NULL;

	      have_var = 2;
	      class_name = e_malloc(strlen(btype->ty_aggr_or_enum->ae_tag) +
				    strlen(*nptr) + 4);
	      sprintf(class_name, "%s::%s",btype->ty_aggr_or_enum->ae_tag, *nptr);
	      if (out_f)
		oldf = errf_set_ofunc(out_f);
	      if (!find_func_by_name(class_name, &f0, &f1, TRUE))
	      {
		if (f0)
		  show_func_source(sw, f0);
		have_var = 1;
	      }
	      else
	      {        /* may be a virtual or inherited method - try wilcard  */
		var_t *v, *v1;
		common_block_t *cblock;
		fil_t *fil0, *fil1;

		sprintf(class_name, "*::%s", *nptr);
		if (!find_global_by_name(class_name, fil, f,
					 (common_block_t *)NULL, 0, &f0, &f1,
					 &v, &v1, &cblock, &fil0, &fil1, TRUE))
		{
		  if (f0)
		    show_func_source(sw, f0);
		  else
		    if (v)
		      have_var = show_global
			(sw, fil, f, (common_block_t *)NULL,
			 v->va_name, TRUE, FALSE, FALSE, (objid_t *)NULL, FALSE, 
			 double_click);
		  else
		    have_var = 0;
		}
	      }
	      free(class_name);
	      if (out_f)
		errf_set_ofunc(oldf);
	    }
	  }
	}

	if (have_var != 1)
	{
	  if (!control_down && can_get_target_vars(get_current_target())) {
	    if (f == NULL ||
		!(have_var = show_local(sw, f, lnum, names[0],
					*(names + 1) ? FALSE : double_click)))
	    {
/*	      have_var = search_stack_files(sw, names[0]);*/
	      if (!have_var)
		have_var = show_global
		  (sw, fil, f, (common_block_t *)NULL,
		   names[0], TRUE, FALSE, FALSE, (objid_t *)NULL, FALSE,
		   *(names + 1) ? FALSE : double_click);
	    }
	  }
	  else {
	    have_var = show_global
	      (sw, fil, f, (common_block_t *)NULL,
		 names[0], TRUE, FALSE, FALSE, (objid_t *)NULL, FALSE,
		 *(names + 1) ? FALSE : double_click);
	  }

	  for (nptr = names + 1; *nptr != NULL; ++nptr)
	    show_mem = show_single_member
	      (*nptr, &btype, *(nptr + 1) ? FALSE : double_click); 
	}
	if (have_var != 1 && fil->fi_language == LANG_CC &&
	    !(strchr(wholename, '.') || strstr(wholename, "->")))
	{
	  func_t *f0, *f1;
	  char *class_name;
	  errf_ofunc_t oldf = NULL;
	  int i, method = 0;

	  if (f != NULL && strstr(f->fu_demangled_name, "::"))
	    method = 1;
	  if (method)
	    for (i = 0; f->fu_demangled_name[i] != ':'; i++);
	  else
	    i = 1;
	  class_name = e_malloc(i + strlen(wholename) + 3);
	  if (method)
	  {
	    strncpy(class_name, f->fu_demangled_name, i);
	    sprintf(class_name + i, "::%s", wholename);
	  }
	  if (out_f)
	    oldf = errf_set_ofunc(out_f);
	  if (method && !find_func_by_name(class_name, &f0, &f1, TRUE))
	  {
	    if (f0)
	      show_func_source(sw, f0);
	    have_var = 1;
	  }
	  else
	  {   /* may be a virtual or inherited method - try wilcard  */
	    var_t *v = 0, *v1;
	    common_block_t *cblock;
	    fil_t *fil0, *fil1;

	    if (method)
	      have_var = show_global
		(sw, fil, f, (common_block_t *)NULL,
		 class_name, TRUE, TRUE, FALSE, (objid_t *)NULL, FALSE, 
		 double_click);
	    if (!have_var)
	    {
	      sprintf(class_name, "*::%s", wholename);
	      if (!find_global_by_name(class_name, fil, f,
				       (common_block_t *)NULL, 0, &f0, &f1,
				       &v, &v1, &cblock, &fil0, &fil1, TRUE))
	      {
		if (f != NULL)
		  show_func_source(sw, f);
		else
		  if (v)
		    have_var = show_global
		      (sw, fil, f, (common_block_t *)NULL,
		       class_name, TRUE, TRUE, FALSE, (objid_t *)NULL, FALSE,
		       double_click);
		have_var = 1;
	      }
	    }
	  }
	  free(class_name);
	  if (out_f)
	    errf_set_ofunc(oldf);
	}
	td_set_select_recording(oldval);
	td_set_obj_updating(oldstate);

	free((char *)names);
	return(have_var);	/* RGA for C++ retry */
}

/*  If there is a single struct/union variable selected,
 *  and name is the name of a member of the struct/union,
 *  then add that member (if necessary) and select it.
 *
 *  This is called by show_var when the user has selected
 *  a struct/union member in the source window.
 */
static int
show_single_member(name, btype, double_click)
const char *name;
type_t **btype;
bool double_click;
{
	sel_t *sel;
	objid_t par, var_obj;
	int objtype;
	var_t *v;
	taddr_t addr;

	/*  Check that we have a single variable selected.
	 *  This might be false as the show_local/show_global
	 *  above could have displayed a function and not
	 *  affected the selection.
	 */
	sel = get_selection();
	if (sel == NULL || sel->se_next != NULL)
		return 1;
	par = sel->se_code;
	objtype = get_object_type(par);
	if (objtype != OT_VAR)
		return 1;

	/* RGA Note return value of get_member_of_aggr() changed from 2.45.2 */
	if (get_member_of_aggr(par, name, &v, &addr, btype))
		return 0;
	if ((var_obj = find_var(par, v)) == NULL)
		var_obj = add_var_object(par, v, OBJ_FIRST_CHILD, FALSE);

	clear_selection();
	if (double_click) {
	  do_var_or_expr(var_obj, get_double_click_action(var_obj), NULL);
	}
	select_object(var_obj, TRUE, OBJ_SELF);
	ensure_visible(var_obj);
	return 1;
}

static int
show_member(sel, names, p_found_a_member)
struct selst *sel;
char **names;
int *p_found_a_member;
{
	char **nptr;
	objid_t par, var_obj;
	var_t *v;
	taddr_t addr;
	int selected_one, found_a_member;
	type_t *btype;

	selected_one = found_a_member = FALSE;
	for (; sel != NULL; sel = sel->se_next) {
		par = sel->se_code;
		var_obj = NULL;
		for (nptr = names; *nptr != NULL; nptr++) {
		  /* RGA Note return value of get_member_of_aggr() changed from 2.45.2 */
			if (get_member_of_aggr(par, *nptr + 1, &v, &addr, &btype) != 0)
				break;
			found_a_member = TRUE;
			if (addr == 0)
				break;
			if ( v->va_flags & VA_STATIC_MEMBER)
			{
				show_static_member(btype,v);
				break;
			} else if ((var_obj = find_var(par, v)) == NULL)
				var_obj = add_var_object(par, v,
							 OBJ_FIRST_CHILD,
							 FALSE);
			set_no_indent(var_obj, **nptr == '@');
			par = var_obj;
		}
		if (var_obj != NULL) {
			select_object(var_obj, TRUE, OBJ_SELF);
			selected_one = TRUE;
		}
		sel->se_user = TRUE;
	}
	if (found_a_member)
		*p_found_a_member = TRUE;
	return selected_one;
}

static int
skip_sref(name, sepc, p_next)
const char *name;
int sepc;
const char **p_next;
{
	if (*name == sepc || *name == '@')
		*p_next = name + 1;
	else if (*name == '-' && name[1] == '>')
		*p_next = name + 2;
	else {
		errf("Expected %c or -> but got %s in expression", sepc, name);
		return FALSE;
	}
	return TRUE;
}

static int
skip_varname(name, seps, p_next)
const char *name, *seps, **p_next;
{
	const char *cptr;

	for (cptr = name; *cptr != '\0' && isvaralnum(*cptr); cptr++)
		;
	if (!isvaralpha(*name) || (*cptr != '\0' && strchr(seps, *cptr) == NULL)) {
		errf("\"%s\" is not a correctly formed variable name", name);
		return FALSE;
	}
	*p_next = cptr;
	return TRUE;
}

static char **
parse_dot_expr(name_start, sepc)
const char *name_start;
int sepc;
{
	const char *name, *next;
	char *cptr, **names, **nptr;
	int memc;
	size_t len;
        static char seps[] = "?@- ";

        seps[0] = sepc;
        
	memc = 0;
	name = name_start;
	do {
		if (!skip_sref(name, sepc, &name))
			return NULL;
		memc++;
		if (!skip_varname(name, seps, &next))
			return NULL;
		name = next;
	} while (*name != ' ' && *name != '\0');

	nptr = names = (char **) e_malloc((memc + 1) * sizeof(char *));

	name = name_start;
	do {
		if (!skip_sref(name, sepc, &name) ||
                    !skip_varname(name, seps, &next))
			panic("bad name in parse_dot_expr");
		len = next - name;
		cptr = strncpy(e_malloc(len + 2), name - 1, len + 1);
		cptr[len + 1] = '\0';
		*nptr++ = cptr;
		name = next;
	} while (*name != ' ' && *name != '\0');
	*nptr = NULL;
	return names;
}

static void
free_names(names)
char **names;
{
	char **nptr;

	for (nptr = names; *nptr != NULL; nptr++)
		free(*nptr);
	free((char *)names);
}

static arg_t *
split_line(line)
char *line;
{
	arg_t *list, *arg;
	char *cptr;

	cptr = line;
	list = NULL;
	for (;;) {
		while (isspace(*cptr))
			cptr++;
		if (*cptr == '\0')
			break;

		arg = (arg_t *) e_malloc(sizeof(arg_t));
		arg->arg_name = cptr;
		arg->arg_names = NULL;	/* for safety */
		arg->arg_next = list;
		list = arg;

		while (*cptr != '\0' && !isspace(*cptr))
			cptr++;
		if (*cptr == '\0')
			break;
		*cptr++ = '\0';
	}
	return list;
}

static void
free_arglist(list)
arg_t *list;
{
	arg_t *next;

	for (; list != NULL; list = next) {
		next = list->arg_next;
		free((char *)list);
	}
}

void
show_var_from_typing_line(sw, name)
Srcwin *sw;
const char *name;
{
	arg_t *start, *list, *arg;
	struct selst *head_sel, *sel;
	char *cptr;
        fil_t *fil;
	int oldstate, selected_one, found_a_member, ntimes, sepc;
	bool oldval;

	oldval = td_set_select_recording(FALSE);

        fil = get_displayed_fil();
        sepc = (fil != NULL && IS_FORTRAN(fil->fi_language)) ? '%' : '.';
        
	if (*name != sepc && *name != '@' && (*name != '-' || name[1] != '>')) {
		if (!show_local(sw, (func_t *)NULL, 0, name, FALSE)) {
			show_global(sw, get_displayed_fil(),
				    (func_t *)NULL, (common_block_t *)NULL,
				    name, TRUE, TRUE, FALSE, (objid_t *)NULL,
				    FALSE, FALSE);
		}

		td_set_select_recording(TRUE);
		return;
	}

	if ((head_sel = get_unordered_selection()) == NULL) {
		errf("No objects selected");
		td_set_select_recording(TRUE);
		return;
	}
	if (get_object_type(head_sel->se_code) != OT_VAR &&
	    get_object_type(head_sel->se_code) != OT_EXPR) {
		errf("No variables selected");
		td_set_select_recording(TRUE);
		return;
	}

	for (sel = head_sel; sel != NULL; sel = sel->se_next)
		sel->se_user = FALSE;

	cptr = strsave(name);
	start = list = split_line(cptr);
	selected_one = found_a_member = FALSE;

	if (*list->arg_name == '#') {
		ntimes = atoi(list->arg_name + 1);
		list = list->arg_next;
	}
	else
		ntimes = 1;
	
	for (arg = list; arg != NULL; arg = arg->arg_next) {
		arg->arg_names = parse_dot_expr(arg->arg_name, sepc);
		if (arg->arg_names == NULL) 
			break;
	}

	if (arg != NULL) {
		arg_t *arg2;

		for (arg2 = list; arg2 != arg; arg2 = arg2->arg_next)
			free_names(arg2->arg_names);

		free(cptr);
		free_arglist(list);
		td_set_select_recording(TRUE);
		return;
	}

	oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
	for (;;) {
		for (arg = list; arg != NULL; arg = arg->arg_next) {
			if (show_member(head_sel, arg->arg_names,
							     &found_a_member))
				selected_one = TRUE;
		}
		if (!selected_one)
			break;

		for (sel = head_sel; sel != NULL; sel = sel->se_next)
			if (sel->se_user)
				select_object(sel->se_code, FALSE, OBJ_SELF);

		if ((head_sel = get_unordered_selection()) == NULL)
			break;
		
		if (ntimes != 0 && --ntimes == 0)
			break;
	}
	
	if (!found_a_member) {
		if (list != NULL && list->arg_next != NULL)
			errf("None of the elements given are members of any selected structures");
		else
			errf("\"%s\" is not a member of any selected structure",
								list->arg_names[0]);
	}

	for (arg = list; arg != NULL; arg = arg->arg_next)
		free_names(arg->arg_names);

	td_set_obj_updating(oldstate);
	free(cptr);
	free_arglist(start);
	td_set_select_recording(TRUE);
}

static var_t *
find_local_at_lnum(f, lnum, name, blocks)
func_t *f;
int lnum;
const char *name;
block_t **blocks;
{
	int level, match_level;
	var_t *v, *match_var;
	block_t *bl;
	block_t *blocktab[MAX_BLOCK_LEVEL + 1];

	if (blocks == NULL)
		blocks = blocktab;

	bl = FU_BLOCKS(f);
	match_var = NULL;
	match_level = -1;
	level = 0;

	while (bl != NULL) {
		for (v = bl->bl_vars; v != NULL; v = v->va_next) {
			if (strcmp(v->va_name, name) == 0) {
				match_level = level;
				match_var = v;
			}
		}

		blocks[level++] = bl;
		for (bl = bl->bl_blocks; bl != NULL; bl = bl->bl_next)
			if (bl->bl_start_lnum <= lnum && lnum <= bl->bl_end_lnum)
				break;
	}
	
	blocks[match_level + 1] = NULL;
	return match_var;
}

bool
display_local(par, f, lnum, name, restoring, p_var_obj, double_click)
objid_t par;
func_t *f;
int lnum;
const char *name;
bool restoring;
objid_t *p_var_obj;
bool double_click;
{
	/*  TODO: use add_block_if_necessary instead of the blocks array.
	 */
	block_t *blocks[MAX_BLOCK_LEVEL + 1];
	var_t *v;
	objid_t var_obj, child;
	int oldstate, level, nmatches_at_this_level;
	bool cleared_selection;

	if (p_var_obj != NULL)
		*p_var_obj = NULL;
	
	/*  We don't store the found variable, as we rescan the list
	 *  looking for multiple matches if we find one.
	 */
	if (find_local_at_lnum(f, lnum, name, blocks) == NULL)
		return find_local(par, f, (char *)name, p_var_obj);
	
	oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);

	/*  Note that we start level from one here, because the "block" at
	 *  level zero is the function entry in the stack, which is always
	 *  present.
	 */
	for (level = 1; blocks[level] != NULL; ++level) {
		if ((child = find_block(par, blocks[level])) == 0)
			child = add_block_object(par, blocks[level]);
		par = child;
	}
	--level;

	/*  You can get multiple declarations for a variable at one level
	 *  when a formal parameter is redeclared as a local variable, so
	 *  we check for this.
	 *  RGA  If restoring is true, always add the object because we may
	 *  be restoring a series of duplicated vars, with different indicies.
	 */
	cleared_selection = FALSE;
	nmatches_at_this_level = 0;
	
	for (v = blocks[level]->bl_vars; v != NULL; v = v->va_next) {
		if (strcmp(v->va_name, name) == 0) {
			if ((var_obj = find_var(par, v)) == NULL ||
			    restoring == TRUE) {
				var_obj = add_var_object(par, v,
							 OBJ_LAST_CHILD,
							 restoring);
			}

			if (p_var_obj != NULL) {
				*p_var_obj = var_obj;
			}
			else {
				if (!cleared_selection) {
					clear_selection();
					cleared_selection = TRUE;
				}
				
				if (double_click) {
				  do_var_or_expr(var_obj,
						 get_double_click_action(var_obj),
						 NULL);
				} 
				select_object(var_obj, TRUE, OBJ_SELF);

				ensure_visible(var_obj);
			}
			
			++nmatches_at_this_level;
		}
	}

	td_set_obj_updating(oldstate);

	if (p_var_obj != NULL && nmatches_at_this_level > 1)
		errf("Warning: %s declared %d times at the same level",
						name, nmatches_at_this_level);

	return TRUE;
}

static bool
find_local_no_var_obj(par, f, name)
objid_t par;
func_t *f;
char *name;
{
	return find_local(par, f, name, (objid_t *)NULL);
}

static bool
find_local(par, f, name, p_var_obj)
objid_t par;
func_t *f;
char *name;
objid_t *p_var_obj;
{
	int nvars;
	bool cleared_selection;

#ifdef ARCH_MIPS
	if (!(f->fu_language == LANG_C || f->fu_language == LANG_CC))
		return FALSE;
#endif
	if (FU_BLOCKS(f) == NULL)
		return FALSE;

	cleared_selection = FALSE;
	nvars = find_all_locals(f, name, par, FU_BLOCKS(f),
				&cleared_selection, p_var_obj);

	if (nvars > 1)
		errf("Warning: there are %d declarations of %s in function %s",
		     nvars, name, f->fu_demangled_name);
	
	return nvars != 0;
}

static int
find_all_locals(f, name, par, block, p_cleared_selection, p_var_obj)
func_t *f;
const char *name;
objid_t par;
block_t *block;
bool *p_cleared_selection;
objid_t *p_var_obj;
{
	int total_nvars;
	block_t *bl;
	objid_t var_obj;
	var_t *v;

	total_nvars = 0;

	for (bl = block->bl_blocks; bl != NULL; bl = bl->bl_next) {
		objid_t child, new_child;
		int nvars;

		child = find_block(par, bl);
		new_child = (child != 0) ? child : add_block_object(par, bl);

		if ((nvars = find_all_locals(f, name, new_child, bl,
					     p_cleared_selection,
					     p_var_obj)) != 0) {
			total_nvars += nvars;
		}
		else if (child == 0) {
			remove_object(new_child, OBJ_SELF);
		}
	}

	var_obj = NULL;
	for (v = block->bl_vars; v != NULL; v = v->va_next) {
		if (strcmp(v->va_name, name) == 0) {
			if ((var_obj = find_var(par, v)) == NULL) {
				var_obj = add_var_object(par, v,
							 OBJ_LAST_CHILD,
							 FALSE);
			}

			if (p_var_obj != NULL) {
				*p_var_obj = var_obj;
			}
			else {
				if (!*p_cleared_selection) {
					clear_selection();
					*p_cleared_selection = TRUE;
				}
				
				select_object(var_obj, TRUE, OBJ_SELF);
				ensure_visible(var_obj);
			}
			
			++total_nvars;
		}
	}
				
	return total_nvars;
}

static bool
find_func(stack_obj, f, args)
objid_t stack_obj;
func_t *f;
char *args;
{
	find_func_args_t *ffa;

	ffa = (find_func_args_t *)args;
	if (f == ffa->ffa_func) {
		ffa->ffa_stack_obj = stack_obj;
		return TRUE;
	}
	return FALSE;
}

static bool
search_stack(func, args)
bool (*func)PROTO((objid_t stack_obj, func_t *f, char *a_args));
char *args;
{
  bool ret;

  ret = iterate_over_stack_funcs(func, args, STK_SELECTED, STK_OUTER);
  if (!ret)
    ret = iterate_over_stack_funcs(func, args, STK_INNER, STK_SELECTED);
  return ret;
}

/*****    Not used ( RCB)
static bool
search_stack_files(sw, name)
     Srcwin *sw;
     char *name;
{
  bool ret;
  target_t *xp;
  cursor_t old_cursor = wn_get_window_cursor(WN_STDWIN);
  set_bm_cursor(WN_STDWIN, CU_WAIT);

  xp = get_current_target();
  update_target_menu_state(TS_SEARCHING, xp_is_attached(xp));
  ret = back_iterate_over_stack_files(sw, name, STK_OUTER, STK_SELECTED);
  if (!ret)
    ret = back_iterate_over_stack_files(sw, name, STK_SELECTED, STK_INNER);
  if (target_menu_search_disabled(0, 0) == FALSE)
    update_target_menu_state(xp_get_state(xp), xp_is_attached(xp));
  wn_define_cursor(WN_STDWIN, old_cursor);
  return ret;
}
******/

bool
show_local_expr(fil, lnum, text)
fil_t *fil;
int lnum;
const char *text;
{
	func_t *f;
	int oldstate;
	bool found;
	find_func_args_t ffargs;

	if (!can_get_target_vars(get_current_target())) {
		errf("Target not running");
		return FALSE;
	}
	
	if ((f = lnum_to_func(fil, lnum)) == NULL) {
		errf("Can't map line %d of %s to a function",
		     lnum, fil->fi_name);
		return FALSE;
	}
	
	oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
	
	
	ffargs.ffa_func = f;
	ffargs.ffa_stack_obj = NULL;

	found = search_stack(find_func, (char *)&ffargs);

	if (found) {
		objid_t par;
		block_t *bl;

		bl = lnum_to_block(FU_BLOCKS(f), lnum);
		par = add_blocks_as_needed(ffargs.ffa_stack_obj, FU_BLOCKS(f),
					   bl);
		clear_selection();

		add_expr_object(par, bl, text, f->fu_language, OBJ_LAST_CHILD);
	}
	else {
		errf("Function %s is not currently active", f->fu_demangled_name);
	}

	td_set_obj_updating(oldstate);
	
	return found;
}

static bool
show_local(sw, f, lnum, name, double_click)
Srcwin *sw;
func_t *f;
int lnum;
const char *name;
bool double_click;
{
	int oldstate;
	bool found;
	target_t *xp;
        cursor_t old_cursor;

	if (!can_get_target_vars(get_current_target()))
		return FALSE;
	
	old_cursor = wn_get_window_cursor(WN_STDWIN);
	set_bm_cursor(WN_STDWIN, CU_WAIT);
	oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
	xp = get_current_target();
	update_target_menu_state(TS_SEARCHING, xp_is_attached(xp));
	stop_pressed(0, 1);	/* reset */
	if (f == NULL) {
		found = search_stack(find_local_no_var_obj, (char *)name);
	}
	else {
		find_func_args_t ffargs;

		ffargs.ffa_func = f;
		ffargs.ffa_stack_obj = NULL;
		
		if (search_stack(find_func, (char *)&ffargs)) {
			found = display_local(ffargs.ffa_stack_obj, f,
					      lnum, name, FALSE,
					      (objid_t *)NULL, double_click);
		}
		else {
			if (!show_global(sw, f->fu_fil, f,
					 (common_block_t *)NULL, name,
					 FALSE, FALSE, FALSE,
					 (objid_t *)NULL, FALSE,
					 double_click)) {
				errf("Function %s is not currently active",
				     f->fu_demangled_name);
			}
			found = TRUE;
		}
	}
	td_set_obj_updating(oldstate);
	if (stop_pressed(0, 0))
	  found = TRUE;
	if (target_menu_search_disabled(0, 0) == FALSE)
	  update_target_menu_state(xp_get_state(xp), xp_is_attached(xp));
	
	wn_define_cursor(WN_STDWIN, old_cursor);
	return found;
}

bool
show_global(sw, srcfil, srcfunc, srccblock, name,
	    want_errmesg, from_typing_line, restoring, p_var_obj, 
	    undef_check, double_click)
Srcwin *sw;
fil_t *srcfil;
func_t *srcfunc;
common_block_t *srccblock;
const char *name;
bool want_errmesg, from_typing_line, restoring, undef_check, double_click;
objid_t *p_var_obj;
{
	fil_t *fil, *fil1;
	func_t *f, *f1;
	var_t *v, *v1;
	objid_t var_obj, var_obj1 = NULL;
	objid_t obj, obj1 = NULL;
	common_block_t *cblock;
	int oldstate;
	typecode_t tcode;
	type_t *tbase;

	if (p_var_obj != NULL)
		*p_var_obj = NULL;
	
	if (find_global_by_name
	    (name, srcfil, srcfunc, srccblock,
	     !from_typing_line, &f, &f1, &v, &v1, &cblock, &fil, &fil1, TRUE) != 0)
		return FALSE;

	if (undef_check && v && v->va_type)
	{
	  tbase =  v->va_type;
	  tcode = tbase->ty_code;
	  if (tcode == TY_U_STRUCT ||
	      tcode == TY_U_UNION ||
	      tcode == TY_U_ENUM)
	    return FALSE;
	  
	  while(tbase->ty_code == DT_PTR_TO)
	  {
	    if (tbase->ty_base->ty_code == TY_U_STRUCT ||
		tbase->ty_base->ty_code == TY_U_UNION ||
		tbase->ty_base->ty_code == TY_U_ENUM)
	      return FALSE;
	    tbase = tbase->ty_base;
	    tcode = tbase->ty_code;
	  }
	}

	if (f != NULL) {
		show_func_source(sw, f);
		return TRUE;
	}

	if (!can_get_target_vars(get_current_target())) {
		errf("Can't show variables as the target isn't running");
		return FALSE;
	}

	oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
	if (cblock != NULL) {
		add_common_block_object_if_necessary(cblock);
		obj = (objid_t)cblock;
	}
	else if (fil != NULL) {
		add_source_file_object_if_required(fil, FALSE, FALSE, FALSE);
		obj = (objid_t)fil;
	}
	else
		obj = GLOBALS_OBJCODE;
	
       if (fil1 != NULL) {
		add_source_file_object_if_required(fil1, FALSE, FALSE, FALSE);
		obj1 = (objid_t)fil1;
	}

	clear_selection();
	
	/*  RGA  If restoring is true, always add the object because we may
	 *  be restoring a series of duplicated vars, with different indicies.
	 */

	if ((var_obj = find_var(obj, v)) == NULL ||
	    restoring == TRUE) {
		var_obj = add_var_object(obj, v, OBJ_LAST_CHILD, restoring);
		if (get_object_type(obj) == OT_SFILE) {
			change_type(obj, OT_SFILE_EX);
			sort_children(SRCHEAD_OBJCODE, src_cmp);
		}
	}

       if (fil1 != NULL) 
	if ((var_obj1 = find_var(obj1, v1)) == NULL ||
	    restoring == TRUE) {
		var_obj1 = add_var_object(obj1, v1, OBJ_LAST_CHILD, restoring);
		if (get_object_type(obj1) == OT_SFILE) {
			change_type(obj1, OT_SFILE_EX);
			sort_children(SRCHEAD_OBJCODE, src_cmp);
		}
	}
	if (p_var_obj != NULL) {
		*p_var_obj = var_obj;
	}
	else {
		if (double_click) {
		  do_var_or_expr(var_obj, get_double_click_action(var_obj), NULL);
		}
		select_object(var_obj, TRUE, OBJ_SELF);
		ensure_visible(var_obj);
		if (var_obj1)
		{
		  if (double_click) {
		    do_var_or_expr(var_obj1, get_double_click_action(var_obj), NULL);
		  }
		  select_object(var_obj1, TRUE, OBJ_SELF);
		  ensure_visible(var_obj1);
		}
	}
	
	td_set_obj_updating(oldstate);

	return TRUE;
}

void
show_given_global(fil, v)
fil_t *fil;
var_t *v;
{
  objid_t var_obj;
  objid_t obj;

  if (fil != NULL) {
    add_source_file_object_if_required(fil, FALSE, FALSE, FALSE);
    obj = (objid_t)fil;
  }
  else
    obj = GLOBALS_OBJCODE;

  if ((var_obj = find_var(obj, v)) == NULL) {
    var_obj = add_var_object(obj, v, OBJ_LAST_CHILD, FALSE);
    if (get_object_type(obj) == OT_SFILE) {
      change_type(obj, OT_SFILE_EX);
      sort_children(SRCHEAD_OBJCODE, src_cmp);
    }
  }

  select_object(var_obj, TRUE, OBJ_SELF);
  ensure_visible(var_obj);
}

bool show_static_member(type, v)
type_t* type;
var_t* v;
{
    /*
    **  Call show_global() to display a static member of a class
    */
    char buff[200];
    const char* name = (const char*) v->va_addr;
    if ( name == NULL)
    {
	/* If the full name was not given in the stabs info, attempt to
	** construct if from the class name. This may fail for
	** nested classes 
	*/
        sprintf(buff, "%s::%s", type->ty_aggr_or_enum->ae_tag, v->va_name);
	name = buff;
    }
    return show_global(
        get_current_srcwin()	/* sw */
      , NULL			/* srcfil */
      , (func_t *)NULL		/* srcfunc */
      , (common_block_t*)NULL	/* srccblock */
      , name			/* name */
      , TRUE			/* want_errmesg */
      , FALSE			/* from typing line */
      , FALSE			/* restoring */
      , (objid_t *)NULL		/*p_var_obj */
      , FALSE 			/* undef_check */
      , FALSE                   /* double_click */
    );
}

static void
show_macro(name, fil, lnum, double_click)
const char *name;
fil_t *fil;
int lnum;
bool double_click;
{
   find_func_args_t ffargs;
   block_t *bl;
   int oldstate;
   objid_t obj;

   oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);

   ffargs.ffa_func = lnum_to_func(fil, lnum);
   ffargs.ffa_stack_obj = NULL;

   bl = FU_BLOCKS(ffargs.ffa_func);

   if (lnum >= bl->bl_start_lnum &&
       lnum <= bl->bl_end_lnum &&
       search_stack(find_func, (char *)&ffargs)) {
      obj = add_expr_object(ffargs.ffa_stack_obj, bl, name,
			    ffargs.ffa_func->fu_language, OBJ_LAST_CHILD);
   }
   else {
      add_source_file_object_if_required(fil, FALSE, FALSE, FALSE);
      obj = add_expr_object((objid_t)fil, fil->fi_block, name,
			    fil->fi_language, OBJ_LAST_CHILD);
   }

   clear_selection();

   if (double_click)
      do_var_or_expr(obj, get_double_click_action(obj), NULL);
   
   select_object(obj, TRUE, OBJ_SELF);
   ensure_visible(obj);
   
   td_set_obj_updating(oldstate);

   return;
}
