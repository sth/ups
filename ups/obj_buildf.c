/* obj_buildf.c - miscellaneous stuff - will be broken up soon */

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


/* @(#)obj_buildf.c	1.31 04 Jun 1995 (UKC) */
char ups_obj_buildf_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <stdlib.h>
#include <stdio.h>

#include <local/wn.h>
#include <local/obj/obj.h>
#include <local/menu3.h>
#include <local/ukcprog.h>
#include <mtrprog/utils.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "st.h"
#include "objtypes.h"
#include "obj_buildf.h"
#include "obj_misc.h"
#include "obj_target.h"
#include "obj_bpt.h"
#include "obj_signal.h"
#include "obj_env.h"
#include "obj_stack.h"
#include "obj_wpt.h"
#include "va.h"
#include "ui.h"
#include "exec.h"
#include "expr.h"
#include "state.h"
#include "menudata.h"
#include "tdr.h"

typedef struct
{
  int row;
  int user_row;
  int type;
  char *label;
} Display_row;

static const char *gen_getobjname PROTO((objid_t code));
static void init_objtype PROTO((int objtype, const char *format, ot_t *ot,
				bool have_window));
static int get_row_resource_number PROTO((char *resource, int row_default,
					  int max_rows, int check_only));
static int row_compare PROTO((const void *ap, const void *bp));

static bool  Src_header_header_exists = FALSE; 
static bool Have_watchpoints = FALSE;

#define BUILTIN_MENUS	/* define for compiled in menu files */

/* Menus
 */
#ifdef BUILTIN_MENUS
extern MENU core_men;		/* target menu */
extern MENU bph_men;		/* breakpoint header menu */
extern MENU bpt_men;		/* breakpoint menu */
extern MENU fun_men;		/* function menu */
extern MENU block_men;		/* block menu */
extern MENU fil_men;		/* source file menu */
extern MENU sh_men;		/* source file header menu */
extern MENU var_men;		/* variable menu */
extern MENU var_men2;		/* variable menu with watchpoint */
extern MENU sgh_men;		/* signal header menu */
extern MENU envhead_men;	/* environment header menu */
extern MENU sig_men;		/* signal menu */
extern MENU env_men;		/* environment menu */
extern MENU gbl_men;		/* globals menu */
extern MENU cbh_men;		/* common block header menu */
extern MENU cblock_men;		/* common block menu */
extern MENU mfunc_men;		/* module function menu */
extern MENU nolib_men;		/* unloaded library menu */
extern MENU wph_men;		/* watchpoint header menu */
extern MENU wpt_men;		/* watchpoint menu */
#endif

ot_t Objtab[] = {
#define OT_COM		0
	      { &core_men, "target", "menus/tgtmen.c", -1,
		NULL, do_target, NULL, Com_format, Com_fdefs, Com_fnamemap,
		gen_select, can_select, free_com, NULL, NULL,
		gen_getobjname, target_format_obj },

#define OT_GLOBALS	1
	      { &gbl_men, "globals", "menus/gblmen.c", -1,
		NULL, do_globals, NULL, Globals_format, NULL, NULL,
		gen_select, can_select, NULL, NULL, NULL,
		gen_getobjname, header_format_obj },
	
#define OT_SRCHEAD	2
	      { &sh_men, "srchead", "menus/shmen.c", -1,
		NULL, do_srchead, NULL, Srchead_format, NULL, NULL,
		gen_select, can_select, NULL, NULL, NULL,
		gen_getobjname, header_format_obj },
	
#define OT_SFILE	3
	      { &fil_men, "source", "menus/filmen.c", -1,
		pre_do_file, do_file, NULL, Sfile_format, NULL, NULL,
		gen_select, can_select, NULL, srcfile_getsize, srcfile_getcolor,
		srcfile_getobjname, file_format_obj },
	
#define OT_FHEAD	4
	      { NULL, "stack", NULL, -1,
		NULL, NULL, NULL, Fhead_format, NULL, NULL,
		NULL, cannot_select, NULL, NULL, NULL,
		gen_getobjname, header_format_obj },

#define OT_FUNC		5
	      { NULL, "function", NULL, OT_SFILE,
		pre_do_func, do_func, NULL, Func_format, Func_fdefs, NULL,
		gen_select, can_select, free_func, NULL, func_getcolor,
		func_getobjname, func_format_obj },

#define OT_BLOCK	6
	      { &block_men, "block", "menus/blockmen.c", -1,
		pre_do_block, do_block, NULL, Block_format, NULL, NULL,
		gen_select, can_select, free_block, NULL, NULL,
		block_getobjname, block_format_obj },

#define OT_FSIG		7
	      { NULL, "*fsig", NULL, -1,
		NULL, NULL, NULL, Fsig_format, Fsig_fdefs, NULL,
		NULL, cannot_select, free_fsig, NULL, NULL,
		gen_getobjname, fsig_format_obj },
	
#define OT_BADFUNC	8
	      { NULL, "*badfunc", NULL, -1,
		NULL, NULL, NULL, Badfunc_format, NULL, NULL,
		NULL, cannot_select, free_badfunc, NULL, NULL,
		gen_getobjname, badstack_format_obj },
	
#define OT_BADFRAME	9
	      { NULL, "*badframe", NULL, -1,
		NULL, NULL, NULL, Badfunc_format, NULL, NULL,
		NULL, cannot_select, free_badfunc, NULL, NULL,
		gen_getobjname, badstack_format_obj },
	
#define OT_VAR		10
	      { &var_men, "var", "menus/varmen.c", -1,
		NULL, do_var_or_expr, NULL, Var_format, Var_fdefs, Var_fnamemap,
		gen_select, can_select, free_displayed_var, NULL, var_getcolor,
		var_getobjname, var_format_obj },
	
#define OT_EXPR		11
	      { NULL, "expr", NULL, OT_VAR,
		NULL, do_var_or_expr, NULL,
			Expr_format, Expr_fdefs, Expr_fnamemap,
		gen_select, can_select, free_displayed_expr, NULL, expr_getcolor,
	        expr_getobjname, expr_format_obj },

#define OT_BPHEAD	12
	      { &bph_men, "bphead", "menus/bphmen.c", -1,
		NULL, do_bps, NULL, Bphead_format, NULL, NULL,
		gen_select, can_select, NULL, NULL, NULL,
		gen_getobjname, header_format_obj },
	
#define OT_BPT		13
	      { &bpt_men, "bpt", "menus/bptmen.c", -1,
		pre_do_bpt, do_bpt, post_do_bpt,
	        Bpt_format, Bpt_fdefs, Bpt_fnamemap,
		gen_select, can_select, remove_breakpoint_object, NULL,
		bpt_getcolor, bpt_getobjname, bpt_format_obj },

#define OT_SGHEAD	14
	      { &sgh_men, "sghead", "menus/sghmen.c", -1,
		NULL, do_sgh, NULL, Sghead_format, NULL, NULL,
		gen_select, can_select, NULL, NULL, NULL,
		gen_getobjname, header_format_obj },
	
#define OT_SIG		15
	      { &sig_men, "signal", "menus/sigmen.c", -1,
		NULL, do_sig, NULL, Sig_format, NULL, NULL,
		gen_select, can_select, free_sig, sig_getsize, sig_getcolor,
		sig_getobjname, sig_format_obj },

#define OT_CBHEAD	16
	      { &cbh_men, "cbhead", "menus/cbhmen.c", -1,
		NULL, do_cbhead, NULL, Cbhead_format, NULL, NULL,
		gen_select, can_select, NULL, NULL, NULL,
		gen_getobjname, header_format_obj },
	
#define OT_CBLOCK	17
	      { &cblock_men, "cblock", "menus/cblockmen.c", -1,
		NULL, do_cblock, NULL, Cblock_format, NULL, NULL,
		gen_select, can_select, free_common_block_object, NULL, NULL,
		cblock_getobjname, cblock_format_obj },

#define OT_MHEAD	18
	      { NULL, "mhead", NULL, OT_CBHEAD,
		NULL, do_mhead, NULL, Mhead_format, NULL, NULL,
		gen_select, can_select, NULL, NULL, NULL,
		gen_getobjname, header_format_obj },
	
#define OT_MODULE	19
	      { NULL, "module", NULL, OT_CBLOCK,
		NULL, do_module, NULL, Module_format, NULL, NULL,
		gen_select, can_select, NULL, module_getsize, NULL,
		module_getobjname, module_format_obj },

#define OT_MODULE_FUNC	20
	      { &mfunc_men, "module-func", "menus/mfuncmen.c", -1,
		NULL, do_module_func, NULL, Module_func_format, NULL, NULL,
		gen_select, can_select, NULL, module_func_getsize, NULL,
		module_func_getobjname, module_func_format_obj },

#define OT_ENVHEAD	21
	      { &envhead_men, "envhead", "menus/envheadmen.c", -1,
		NULL, do_envhead, NULL, Envhead_format, NULL, NULL,
		gen_select, can_select, NULL, NULL, NULL,
		gen_getobjname, header_format_obj },
	
#define OT_ENV		22
	      { &env_men, "env", "menus/envmen.c", -1,
		NULL, do_env, NULL, Env_format, Env_fdefs, Env_fnamemap,
		gen_select, can_select, free_env, env_getsize, env_getcolor,
		env_getobjname, env_format_obj },

#define OT_UNLOADED_LIB	23
	      { &nolib_men, "*nolibfunc", "menus/nolibmen.c", -1,
		NULL, do_nolibfunc, post_do_nolibfunc, Nolibfunc_format, NULL, NULL,
		gen_select, can_select, free_badfunc, nolibfunc_getsize, NULL,
		gen_getobjname, badstack_format_obj },

#define OT_WPHEAD	24
	      { &wph_men, "wphead", "menus/wphmen.c", -1,
		NULL, do_wps, NULL,
		Wphead_format, NULL, NULL,
		gen_select, can_select, NULL, NULL, NULL,
		gen_getobjname, header_format_obj },

#define OT_WPT		25
	      { &wpt_men, "wpt", "menus/wptmen.c", -1,
		NULL, do_wpt, NULL, Wpt_format, Wpt_fdefs, Wpt_fnamemap,
		gen_select, can_select, remove_watchpoint_object, NULL,
		wpt_getcolor, gen_getobjname, wpt_format_obj },
};
	
#define OT_MAXTYPE	25
#define N_OBJTYPES (sizeof(Objtab) / sizeof(Objtab[0]))

#define CHILD_INDENT	20	/* RGA not used now */

static const char *
gen_getobjname(code)
objid_t code;
{
	int objtype;

	objtype = ups_get_object_type(code);

	if (objtype < 0 || objtype >= N_OBJTYPES)
		panic("ot botch in ggn");

	return Objtab[objtype].ot_menuname;
}

void
do_formats(have_window)
bool have_window;
{
	fdef_t *fdef;
	register ot_t *ot;

#ifdef ARCH_386
	Have_watchpoints = TRUE;
#endif

	if (Have_watchpoints == FALSE)
	{
	  char *def;
	  
	  def = (char *)wn_get_default("WantWatchpoints");
	  
	  if (def && strcmp(def, "yes") == 0)
	    Have_watchpoints = TRUE;
	}
  
	if (Have_watchpoints)		/* use var_men2 which has watch item */
	{
	  Objtab[OT_VAR].ot_men = &var_men2;
	  Objtab[OT_VAR].ot_menupath = "menus/varmen2.c";
	}

	init_objects();

	if (have_window) {
		font_t *sysfont;

		sysfont = wn_get_sysfont();
		set_field_scale_factors(sysfont->ft_width, sysfont->ft_height);
	}

	for (ot = Objtab; ot < Objtab + N_OBJTYPES; ot++) {
		fdef = ot->ot_fdefs;
		if (fdef != NULL) {
			for (; fdef->fd_char != '\0'; fdef++)
				define_vsformat(fdef->fd_char,
						fdef->fd_edit,
						fdef->fd_draw,
						fdef->fd_getwidth);
		}
	}

	for (ot = Objtab; ot < Objtab + N_OBJTYPES; ot++) {
		if (have_window && ot->ot_men != NULL) {
#ifdef BUILTIN_MENUS
			ot->ot_md = Minsert((MENU *)ot->ot_men);
#else
			ot->ot_md = Mopen(ot->ot_menupath);
#endif
			if (ot->ot_md == -1) {
				Mperror("Mopen");
				exit(1);
			}
			Mnobox(ot->ot_md);
		}

		init_objtype(ot - Objtab, ot->ot_format, ot, have_window);
	}

	/*  An objtype with ot_men NULL and ot_md != -1 means that the
	 *  objtype shares a menu with objtype ot_md.
	 */
	for (ot = Objtab; ot < Objtab + N_OBJTYPES; ++ot) {
		if (ot->ot_men == NULL && ot->ot_md != -1)
			ot->ot_md = Objtab[ot->ot_md].ot_md;
	}

	/*  Do the OT_SFILE_EX and OT_MODULE_EX special cases.
	 */
	init_objtype(OT_SFILE_EX, Sfile_ex_format, &Objtab[OT_SFILE],
		     have_window);
	init_objtype(OT_MODULE_EX, Module_ex_format, &Objtab[OT_MODULE],
		     have_window);
}

static void
init_objtype(objtype, format, ot, have_window)
int objtype;
const char *format;
ot_t *ot;
bool have_window;
{
/* RGA make CHILD_INDENT an exact multiple of font width so display */
/* overlay matches */

	define_objtype(objtype, format, have_window ?
		       3*wn_get_sysfont()->ft_width : CHILD_INDENT,
		       ot->ot_select, ot->ot_can_select,
		       ot->ot_free, ot->ot_get_size, ot->ot_get_color);
	set_objtype_get_name_func(objtype, ot->ot_get_name);
	set_objtype_format_func(objtype, ot->ot_format_func);
}
	
void
close_target_display()
{
	delete_functions(FHEAD_OBJCODE);
	remove_object(GLOBALS_OBJCODE, OBJ_DESCENDENTS);
	
	if (obj_child_exists((objid_t)NULL, CBHEAD_OBJCODE)) {
		objid_t obj;

		for (obj = get_code(CBHEAD_OBJCODE, OBJ_CHILD);
		     obj != NULL;
		     obj = get_code(obj, OBJ_NEXT)) {
			remove_object(obj, OBJ_DESCENDENTS);
		}
	}

	hide_source_vars();
}

objid_t
rebuild_display(xp)
target_t *xp;
{
	objid_t obj_to_make_visible;

	clear_selection();
	obj_to_make_visible = add_functions(xp, FHEAD_OBJCODE);
	update_variable_values();
#ifdef OS_SUNOS_5
	if (xp->xp_new_dynamic_libs == TRUE)
	{
	  mark_duplicate_files();
	  xp->xp_new_dynamic_libs = FALSE;
	}
	if (xp->xp_restore_bpts == TRUE)
	{
	  restore_target_state(FALSE, TRUE);
	  xp->xp_restore_bpts = FALSE;
	}
#endif /* OS_SUNOS_5 */

	return obj_to_make_visible;
}

void
update_variable_values()
{
	objid_t obj;

	update_local_variable_values();

	update_vars_of(GLOBALS_OBJCODE, FALSE);

	if (obj_child_exists((objid_t)NULL, CBHEAD_OBJCODE)) {
		for (obj = get_code(CBHEAD_OBJCODE, OBJ_CHILD);
		     obj != NULL;
		     obj = get_code(obj, OBJ_NEXT)) {
			update_vars_of(obj, FALSE);
		}
	}

	for (obj = get_code(SRCHEAD_OBJCODE, OBJ_CHILD);
	     obj != NULL;
	     obj = get_code(obj, OBJ_NEXT)) {
		update_vars_of(obj, FALSE);
	}
}

static int get_row_resource_number(resource, row_default, max_rows, check_only)
char *resource; 
int row_default;
int max_rows;
int check_only;
{
  static int rows[9], first_time = 1;
  char *env_str, *ptr = 0;
  char violation_msg[100];
  int j, k, n, used, str_ok = 0;
  long i = 0;

  if (first_time)
  {
    first_time = 0;
    for (i = 0; i < max_rows; i++)
      rows[i] = 0;
  }
  i = row_default;
  env_str = (char *)wn_get_default(resource);
  if (env_str)
  {
    i = strtol(env_str, &ptr, 0);
    str_ok = i && ptr;
  }
  else
    if (check_only)
      return 0;

  if (env_str && str_ok)
  {
    str_ok = (i > 0 && i <= max_rows);
  }
  if (env_str && !str_ok)
  {
    sprintf(violation_msg,
	    "WARNING: ups*%s resource must be an integer between 1 and %d\n",
	    resource, max_rows);
    fprintf(stderr, violation_msg);
    i = row_default;
  }

  for (j = 0; j < max_rows; j++)
  {
    if (rows[j] == i)
    {
      if (env_str && str_ok)
      {
	sprintf(violation_msg,
		"WARNING: ups*%s resource value of %ld already used\n",
		resource, i);
	fprintf(stderr, violation_msg);
      }
      for (n = 1; n <= max_rows; n++)
      {
	for (used = 0, k = 0; k < max_rows; k++)
	  if (rows[k] == n)
	  {
	    used = 1;
	    break;
	  }
	if (!used)
	{
	  i = n;
	  if (env_str && str_ok)
	  {
	    sprintf(violation_msg,
		    "  - using %ld for ups*%s instead.\n",
		    i, resource);
	    fprintf(stderr, violation_msg);
	  }
	  break;
	}
      }
    }
    if (rows[j] == 0)
    {
      rows[j] = i;
      if (env_str && !str_ok)
      {
	sprintf(violation_msg,
		"  - using %ld for ups*%s instead.\n",
		i, resource);
	fprintf(stderr, violation_msg);
      }
      break;
    }
  }
  return (i);
}
  
static int row_compare(ap, bp)
const void *ap, *bp;
{
  const Display_row *a = ap;
  const Display_row *b = bp;
  if (a->row < b->row)
    return (1);
  if (a->row > b->row)
    return (-1);
  return (0);  
}

void
initialise_display_area(xp, args, use_full_path)
target_t *xp;
const char *args;
bool use_full_path;
{
  /* RGA 11/5/99 Add X resource support for controlling the order of
     objects in the display window. Numbering starts at 1 at the top
     The target row is a double height row, all others are single.
     Defaults to historical ordering. If one wants to put the breakpoints
     above the function stack (and one is not using Fortran), use:

     Ups*BreakpointsRow: 6
     Ups*FunctionsRow: 7

     Only rows of interest need to be specified  other rows are positioned 
     according to their default values. The resources control the positions on 
     tool invocation only. 

     */

  static Display_row display_row_array[] = {
    {1, 0, OT_COM, "TargetRow"}, 
    {2, 0, OT_SGHEAD, "SignalsRow"}, 
    {3, 0, OT_ENVHEAD, "EnvironmentRow"}, 
    {4, 0, OT_GLOBALS, "UntypedVariablesRow"}, 
    {5, 0, OT_SRCHEAD, "SourceFilesRow"}, 
    {6, 0, OT_MHEAD, "ModulesRow"}, 
    {7, 0, OT_CBHEAD, "CommonBlocksRow"}, 
    {8, 0, OT_FHEAD, "FunctionsRow"}, 
    {9, 0, OT_BPHEAD, "BreakpointsRow"},
    {10, 0, OT_WPHEAD, "WatchpointsRow"}};

#define MAX_ROW	(sizeof display_row_array / sizeof *display_row_array)
 
   Display_row *dis;
   int i, j, max_rows = MAX_ROW;
   symtab_t *st;
   bool have_common_blocks, have_modules;

  have_modules = have_common_blocks = FALSE;

  st = NULL;
  while (xp_next_symtab(xp, st, FALSE, &st)) {
    if (st->st_cblist != NULL)
      have_common_blocks = TRUE;
    if (st->st_modules != NULL)
      have_modules = TRUE;
  }

  if (have_modules == FALSE)
  {
    display_row_array[5].row = 0; /* disable OT_MHEAD */
    max_rows--;
  }
  if (have_common_blocks == FALSE)
  {
    display_row_array[6].row = 0; /* disable OT_CBHEAD*/
    max_rows--;
  }
  if (Have_watchpoints == FALSE)
  {
    display_row_array[9].row = 0; /* disable OT_WPHEAD*/
    max_rows--;
  }
  
  for (i = j = 0, dis = display_row_array; j < MAX_ROW; j++, dis++)
    if (dis->row > 0)
    {
      dis->user_row = get_row_resource_number(dis->label, i + 1, max_rows, 1);
      i++;
    }

  for (i = j = 0, dis = display_row_array; j < MAX_ROW; j++, dis++)
    if (dis->row > 0)
    {
      if (dis->user_row)
	dis->row = dis->user_row;
      else
	dis->row = get_row_resource_number(dis->label, i + 1, max_rows, 0);
      i++;
    }
  qsort((char *)display_row_array, MAX_ROW, sizeof(Display_row), row_compare);

  for (j = 0, dis = display_row_array; j < MAX_ROW; j++, dis++)
    switch (dis->type)
    {
    case OT_COM:
      add_target_object((objid_t)NULL, xp->xp_textpath, args, use_full_path);
      break;
    case OT_SGHEAD:
      add_signals_header((objid_t)NULL);
      break;
    case OT_ENVHEAD:
      add_env_header((objid_t)NULL);
      break;
    case OT_GLOBALS:
      add_globals_header((objid_t)NULL);
      break;
    case OT_SRCHEAD:
      new_object(SRCHEAD_OBJCODE, OT_SRCHEAD, (objid_t)NULL, OBJ_CHILD);
	/* RGA - ugh, but can't do collapse_files if this */
	/* has not been done */
      Src_header_header_exists = TRUE; 
      break;
    case OT_MHEAD:
      if (have_modules)
	new_object(MHEAD_OBJCODE, OT_MHEAD, (objid_t)NULL, OBJ_CHILD);
      break;
    case OT_CBHEAD:
      if (have_common_blocks)
	new_object(CBHEAD_OBJCODE, OT_CBHEAD, (objid_t)NULL, OBJ_CHILD);
      break;
    case OT_FHEAD:
      new_object(FHEAD_OBJCODE, OT_FHEAD, (objid_t)NULL, OBJ_CHILD);
      if (can_get_target_vars(xp))	
	add_functions(xp, FHEAD_OBJCODE);
      break;
    case OT_BPHEAD:
      add_breakpoint_header((objid_t)NULL);
      break;
    case OT_WPHEAD:
      if (Have_watchpoints)
	add_watchpoint_header((objid_t)NULL);
      break;
    }
}

void
  collapse_files()
{
  int oldstate;

  if (Src_header_header_exists == TRUE)
  {
    oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
    do_srchead(SRCHEAD_OBJCODE, MR_HIDE_SOURCE_FILES_COMPLETELY, NULL);
    td_set_obj_updating(oldstate);
  }
}

void
restore_expanded_file_entries()
{
	objid_t obj, next_obj = NULL;
	int oldstate;

	oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
	reset_file_restore_states();
	for (obj = get_code(SRCHEAD_OBJCODE, OBJ_CHILD); obj != NULL;)
	{
	  next_obj = get_code(obj, OBJ_NEXT);
	  restore_files(obj);
	  obj = next_obj;
	}
	iterate_over_source_files
	  (add_source_file_object_if_necessary2, TRUE);
	sort_children(SRCHEAD_OBJCODE, src_cmp);
	do_srchead(SRCHEAD_OBJCODE, MR_HIDE_SOURCE_FILES, NULL);
	do_srchead(SRCHEAD_OBJCODE, MR_SHOW_SOURCE_FILES, NULL);
	do_srchead(SRCHEAD_OBJCODE, MR_HIDE_SOURCE_FILES, NULL);
	td_set_obj_updating(oldstate);
}
	
