/* va_state.c - save and restore the state of the vars display */

/*  Copyright 1994 Mark Russell, University of Kent at Canterbury.
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


/* @(#)va_state.c	1.8 04 Jun 1995 (UKC) */
char ups_va_state_c_rcsid[] = "$Id$";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <local/wn.h>
#include <local/obj/obj.h>

#include "ups.h"
#include "objtypes.h"
#include "symtab.h"
#include "ci.h"
#include "target.h"
#include "va.h"
#include "ui.h"
#include "st.h"
#include "obj_stack.h"
#include "obj_misc.h"
#include "va_priv.h"
#include "srcwin.h"
#include "expr.h"
#include "util.h"

typedef enum {
	VT_VAR,
	VT_EXPR
} Vetype;

typedef struct {
	const char *name;
	vformat_t format;
	bool no_indent;
	int ilevel;
	int icount;
	int *indices;
} Vsvar;

typedef struct {
	const char *text;
	vformat_t format;
} Vsexpr;

typedef struct velist_s {
	Vetype type;
	int block_start;
	int block_end;
	union {
		Vsvar *var;
		Vsexpr *expr;
	} u;
	struct velist_s *children;
	struct velist_s *next;
} Velist;

typedef struct vstack_s {
	alloc_pool_t *apool;
	func_t *func;
	Velist *velist;
	bool updated;
	bool been_displayed;
	struct vstack_s *next;
} Vstack;

typedef struct vfile_s {
	alloc_pool_t *apool;
	objid_t obj;
	int objtype;
	Velist *velist;
	bool been_displayed;
	bool been_restored;
	bool user_changed_vars;
	struct vfile_s *next;
} Vfile;

static void note_file_display_change PROTO((objid_t obj));
static Vstack *func_to_vstack PROTO((func_t *f));
static Vstack *push_new_vstack PROTO((func_t *f, alloc_pool_t *ap,
					Velist *velist));
static bool save_any_stack_changes PROTO((objid_t obj, func_t *f, char *args));
static Velist *push_ve PROTO((alloc_pool_t *ap, Velist **p_velist,
				block_t *bl, objid_t par, Vetype vetype));
static void push_var_state PROTO((alloc_pool_t *ap, Velist **p_velist,
				  objid_t par, dvar_t *dv));
static void push_expr_state PROTO((alloc_pool_t *ap, Velist **p_velist,
				   objid_t par, objid_t obj));
static void push_children PROTO((alloc_pool_t *ap, Velist **p_velist,
				 objid_t par));
static void restore_display PROTO((objid_t par, Velist *velist));
static objid_t restore_var PROTO((objid_t par, Vsvar *vv,
			       int block_start, int block_end));
static void make_vv_ilist PROTO((dvar_t *dv, int *indices, int count));
static objid_t restore_expr PROTO((objid_t par, Vsexpr *vc,
				int block_start, int block_end));
static Vfile *obj_to_vfile PROTO((objid_t obj));
static block_t *lnum_range_to_block PROTO((block_t *parbl,
					   int start_lnum, int end_lnum));
static void restore_children PROTO((objid_t par, Velist *children));
static void restore_dv_state PROTO((Vsvar *vv, dvar_t *dv));
static int dump_children PROTO((const char *path, FILE *fp,
				Velist *velist, int level));
static void dump_var PROTO((FILE *fp, Vsvar *vv));
static void dump_expr PROTO((FILE *fp, Vsexpr *vc));
static int read_velist PROTO((alloc_pool_t *ap, Velist **p_velist,
			      FILE *fp, int line_offset, int *p_lnum));
static int read_var PROTO((alloc_pool_t *ap, char *line, Vsvar *vv));
static int read_expr PROTO((alloc_pool_t *ap, char *line, Vsexpr *vc));
static bool get_word PROTO((char **p_line, const char *what, char **p_cmd));
static Vfile *push_new_vfile PROTO((objid_t obj, int objtype, alloc_pool_t *ap,
				    Velist *velist));
static int load_velist PROTO((FILE *fp, int line_offset, int *p_lnum,
			      bool failed,
			      alloc_pool_t **p_ap, Velist **p_velist));
static Velist *reverse_velist PROTO((Velist *velist));
static bool update_stack_entry_if_needed PROTO((Stack *stk, bool force));
static void update_vf PROTO((Vfile *vf, bool just_started, bool set_been_displayed));

static Vstack *Vslist = NULL;
static Vfile *Vflist = NULL;

void
note_var_change(obj)
objid_t obj;
{
	for (;;) {
		obj = get_code(obj, OBJ_PARENT);

		if (obj == NULL)
			break;
		
		switch (ups_get_object_type(obj)) {
		case OT_SFILE:
		case OT_GLOBALS:
		case OT_CBLOCK:
			note_file_display_change(obj);
			break;
		case OT_FUNC:
			((Stack *)obj)->stk_user_changed_vars = TRUE;
			break;
		}
	}

}

static Vfile *
obj_to_vfile(obj)
objid_t obj;
{
	Vfile *vf;

	for (vf = Vflist; vf != NULL; vf = vf->next) {
		if (vf->obj == obj)
			return vf;
	}

	return NULL;
}

static void
note_file_display_change(obj)
objid_t obj;
{
	Vfile *vf;

	if ((vf = obj_to_vfile(obj)) == NULL) {
		vf = push_new_vfile(obj, ups_get_object_type(obj),
				    alloc_create_pool(), (Velist *)NULL);
	}

	vf->user_changed_vars = TRUE;
}

static Vfile *
push_new_vfile(obj, objtype, ap, velist)
objid_t obj;
int objtype;
alloc_pool_t *ap;
Velist *velist;
{
	Vfile *vf;
	
	vf = (Vfile *)e_malloc(sizeof(Vfile));
	vf->apool = ap;
	vf->obj = obj;
	vf->objtype = objtype;
	vf->velist = velist;
	vf->next = Vflist;
	vf->user_changed_vars = FALSE;
	vf->been_displayed = FALSE;
	vf->been_restored = FALSE;
	Vflist = vf;

	return vf;
}


static Vstack *
func_to_vstack(f)
func_t *f;
{
	Vstack *vs;

	for (vs = Vslist; vs != NULL; vs = vs->next) {
		if (vs->func == f)
			return vs;
	}

	return NULL;
}

static Vstack *
push_new_vstack(f, ap, velist)
func_t *f;
alloc_pool_t *ap;
Velist *velist;
{
	Vstack *vs;
	
	vs = (Vstack *)e_malloc(sizeof(Vstack));
	vs->apool = ap;
	vs->func = f;
	vs->velist = velist;
	vs->updated = FALSE;
	/*	vs->been_displayed = FALSE;*/
	/* RGA Changing this so stack variables are reinstated on repeated
	   attach/detach cycles, as well as start/kill cycles */
	vs->been_displayed = TRUE;
	vs->next = Vslist;
	Vslist = vs;

	return vs;
}

static bool
save_any_stack_changes(obj, f, args)
objid_t obj;
func_t *f;
char *args;
{
	return update_stack_entry_if_needed((Stack *)obj, FALSE);
}

void
update_stack_entry(stk)
Stack *stk;
{
	update_stack_entry_if_needed(stk, TRUE);
}

static bool
update_stack_entry_if_needed(stk, force)
Stack *stk;
bool force;
{
	Vstack *vs;

	if (!stk->stk_user_changed_vars)
		return FALSE;

	if ((vs = func_to_vstack(stk->stk_func)) != NULL) {
		
		/*  We ignore all but the innermost stack entry for a
		 *  given function.
		 */
		if (vs->updated && !force)
			return FALSE;
		
		alloc_reset_pool(vs->apool);
		vs->velist = NULL;
	}
	else {
		vs = push_new_vstack(stk->stk_func, alloc_create_pool(),
				     (Velist *)NULL);
	}

	push_children(vs->apool, &vs->velist, (objid_t)stk);
	vs->velist = reverse_velist(vs->velist);
	vs->updated = TRUE;
	vs->been_displayed = TRUE;
	
	stk->stk_user_changed_vars = FALSE;
	return FALSE;
}

static Velist *
push_ve(ap, p_velist, bl, par, vetype)
alloc_pool_t *ap;
Velist **p_velist;
block_t *bl;
objid_t par;
Vetype vetype;
{
	objid_t obj;
	Velist *ve, *children;

	ve = (Velist *)alloc(ap, sizeof(Velist));
	ve->type = vetype;
	
	if (bl != NULL) {
		ve->block_start = bl->bl_start_lnum;
		ve->block_end = bl->bl_end_lnum;
	}
	else {
		ve->block_start = 0;
		ve->block_end = 0;
	}

	children = NULL;
	
	for (obj = get_code(par, OBJ_CHILD);
	     obj != NULL;
	     obj = get_code(obj, OBJ_NEXT)) {
		if (ups_get_object_type(obj) == OT_VAR) {
			push_var_state(ap, &children,
				       par, (dvar_t *)obj);
		}
	}

	ve->children = reverse_velist(children);
	ve->next = *p_velist;
	*p_velist = ve;

	return ve;
}

static void
push_var_state(ap, p_velist, par, dv)
alloc_pool_t *ap;
Velist **p_velist;
objid_t par;
dvar_t *dv;
{
	Velist *ve;
	Vsvar *vv;
	ilist_t *il;
	int icount, *indices;
	block_t *bl;

	icount = 0;
	for (il = dv->dv_ilist; il != NULL; il = il->il_next)
		++icount;

	if (icount == 0) {
		indices = NULL;
	}
	else {
		int *ic;
		
		indices = (int *)alloc(ap, icount * sizeof(int));

		if (IS_FORTRAN(dv->dv_var->va_language)) {
			ic = &indices[icount];
			for (il = dv->dv_ilist; il != NULL; il = il->il_next)
				*--ic = il->il_index;
		}
		else {
			ic = indices;
			for (il = dv->dv_ilist; il != NULL; il = il->il_next)
				*ic++ = il->il_index;
		}
	}

	vv = (Vsvar *)alloc(ap, sizeof(Vsvar));
	vv->name = dv->dv_var->va_name;
	vv->format = dv->dv_format;
	vv->ilevel = dv->dv_ilevel;
	vv->icount = icount;
	vv->no_indent = get_no_indent((objid_t)dv);
	vv->indices = indices;

	if (ups_get_object_type(par) == OT_BLOCK)
		bl = obj_to_block(par);
	else
		bl = NULL;

	ve = push_ve(ap, p_velist, bl, (objid_t)dv, VT_VAR);
	ve->u.var = vv;
}

static void
push_expr_state(ap, p_velist, par, obj)
alloc_pool_t *ap;
Velist **p_velist;
objid_t par, obj;
{
	Vsexpr *vc;
	Velist *ve;
	const char *text;
	vformat_t format;
	block_t *bl;

	get_expr_info(obj, &bl, &text, &format);

	if (ups_get_object_type(par) != OT_BLOCK)
		bl = NULL;

	vc = (Vsexpr *)alloc(ap, sizeof(Vsexpr));
	vc->text = text;
	vc->format = format;

	ve = push_ve(ap, p_velist, bl, obj, VT_EXPR);
	ve->u.expr = vc;
}

static void
push_children(ap, p_velist, par)
alloc_pool_t *ap;
Velist **p_velist;
objid_t par;
{
	objid_t obj;

	for (obj = get_code(par, OBJ_CHILD);
	     obj != NULL;
	     obj = get_code(obj, OBJ_NEXT)) {
		switch (ups_get_object_type(obj)) {
		case OT_VAR:
			push_var_state(ap, p_velist, par, (dvar_t *)obj);
			break;
		case OT_EXPR:
			push_expr_state(ap, p_velist, par, obj);
			break;
		case OT_BLOCK:
			push_children(ap, p_velist, obj);
			break;
		default:
			panic("unknown obj type in actv"); 
		}
	}
}

void
save_var_display_state(just_started, set_been_displayed)
bool just_started;
bool set_been_displayed;
{
	Vstack *vs;
	Vfile *vf;

	for (vs = Vslist; vs != NULL; vs = vs->next)
		vs->updated = FALSE;
	
	iterate_over_stack_funcs(save_any_stack_changes, (char *)NULL,
				 STK_INNER, STK_OUTER);
	
	for (vs = Vslist; vs != NULL; vs = vs->next)
		vs->updated = TRUE;

	for (vf = Vflist; vf != NULL; vf = vf->next)
		update_vf(vf, just_started, set_been_displayed);
}

static void
update_vf(vf, just_started, set_been_displayed)
Vfile *vf;
bool just_started;
bool set_been_displayed;
{
        if (vf->user_changed_vars) {
		Velist *velist;
		
		alloc_reset_pool(vf->apool);

		velist = NULL;
		push_children(vf->apool, &velist, vf->obj);
		vf->velist = reverse_velist(velist);
		
		vf->user_changed_vars = FALSE;
		if (just_started)
		  vf->been_displayed = TRUE;
	}
	else
	  if (set_been_displayed)
	    vf->been_displayed = TRUE;

}

void
update_file_entry(obj)
objid_t obj;
{
	Vfile *vf;

	if ((vf = obj_to_vfile(obj)) != NULL)
		update_vf(vf, TRUE, FALSE);
}

void
restore_file_displays()
{
	Vfile *vf;
	
	for (vf = Vflist; vf != NULL; vf = vf->next) {
		if (vf->been_displayed)
			continue;

		switch (vf->objtype) {
		case OT_CBLOCK:
			add_common_block_object_if_necessary((common_block_t *)vf->obj);
			break;
		case OT_SFILE:
		  	if (((fil_t *)(vf->obj))->fi_symtab &&
			    ((fil_t *)(vf->obj))->fi_symtab->st_eclipsed)
			  continue;
			add_source_file_object_if_necessary((fil_t *)vf->obj, FALSE);
			break;
		case OT_GLOBALS:
			break;
		default:
			panic("unknown objtype in rfd");
			break;
		}
	
		restore_display(vf->obj, vf->velist);
		/*	RGA always want an update for multiple attach/detach cycles */
		/*		vf->user_changed_vars = FALSE;*/
	}
}

bool
restore_file_display(obj, force)
objid_t obj;
bool force;
{
	Vfile *vf;
	
	vf = obj_to_vfile(obj);

 	if (vf != NULL && (force || vf->been_displayed)) {
		restore_display(vf->obj, vf->velist);
		vf->been_displayed = TRUE;
	}

	return vf != NULL;
}

bool
restore_files(obj)
     objid_t obj;
{
  Vfile *vf;
	
  vf = obj_to_vfile(obj);

  if (vf != NULL && !vf->been_restored)
  {
    restore_display(vf->obj, vf->velist);
    if (vf->objtype == OT_SFILE || vf->objtype == OT_GLOBALS)
      add_source_file_object_if_required((fil_t *)obj, FALSE, FALSE, TRUE);
    vf->been_restored = TRUE;
  }

  return vf != NULL;
}

void
note_file_collapsed(obj, dummy)
objid_t obj;
bool dummy;
{
	Vfile *vf;
	
	if ((vf = obj_to_vfile(obj)) != NULL) {
		vf->been_displayed = FALSE;
		vf->been_restored = FALSE;
		vf->user_changed_vars = FALSE;
	}
}

void
note_stack_entry_collapsed(stk)
Stack *stk;
{
	Vstack *vs;
	
	if ((vs = func_to_vstack(stk->stk_func)) != NULL)
		vs->been_displayed = FALSE;

	stk->stk_user_changed_vars = FALSE;
}

bool
restore_stack_entry_display(stk, force)
Stack *stk;
bool force;
{
	Vstack *vs;
	
	vs = func_to_vstack(stk->stk_func);
	
	if (vs != NULL && (force || vs->been_displayed)) {
		restore_display((objid_t)stk, vs->velist);
		vs->been_displayed = TRUE;
	}

	stk->stk_user_changed_vars = FALSE;

	return vs != NULL;
}

bool
maybe_restore_var_state(par, dv)
objid_t par;
dvar_t *dv;
{
	Vstack *vs;
	Vfile *vf;
	Velist *velist, *ve;

	switch (ups_get_object_type(par)) {
	case OT_FUNC:
		if ((vs = func_to_vstack(((Stack *)par)->stk_func)) == NULL)
			velist = NULL;
		else
			velist = vs->velist;
		break;
	case OT_SFILE:
	case OT_CBLOCK:
	case OT_GLOBALS:
		if ((vf = obj_to_vfile(par)) == NULL)
			velist = NULL;
		else
			velist = vf->velist;
		break;
	default:
		/*  TODO: OT_BLOCK
		 */
		velist = NULL;
		break;
	}

	for (ve = velist; ve != NULL; ve = ve->next) {
		if (ve->type == VT_VAR &&
		    strcmp(ve->u.var->name, dv->dv_var->va_name) == 0)
			break;
	}

	if (ve == NULL)
		return maybe_restore_format(dv);

	restore_dv_state(ve->u.var, dv);
	restore_children((objid_t)dv, ve->children);

	return TRUE;
}

static void
restore_display(par, velist)
objid_t par;
Velist *velist;
{
	Velist *ve;
	
	for (ve = velist; ve != NULL; ve = ve->next) {
		objid_t obj;
		
		switch(ve->type) {
		case VT_VAR:
			obj = restore_var(par, ve->u.var,
					  ve->block_start, ve->block_end);
			break;
		case VT_EXPR:
			obj = restore_expr(par, ve->u.expr,
					   ve->block_start, ve->block_end);
			break;
		default:
			panic("bad vtype in rdv");
			obj = NULL; /* to satisfy gcc */
		}

		if (obj != NULL)
			restore_children(obj, ve->children);
	}
}

static void
restore_children(par, children)
objid_t par;
Velist *children;
{
	Velist *ve;
	
	for (ve = children; ve != NULL; ve = ve->next) {
		Vsvar *vv;
		taddr_t junk;
		var_t *v;
		type_t *btype;
		
		if (ve->type != VT_VAR)
			panic("type botch in rc");
		vv = ve->u.var;

		if (get_member_of_aggr(par, vv->name, &v, &junk, &btype) == 0) {
			objid_t obj;
		
			obj = add_var_object(par, v, OBJ_LAST_CHILD, TRUE);

			restore_dv_state(vv, (dvar_t *)obj);
			restore_children(obj, ve->children);
		}
	}
}

static objid_t
restore_var(par, vv, block_start, block_end)
objid_t par;
Vsvar *vv;
int block_start, block_end;
{
	objid_t obj = NULL;
	
	switch (ups_get_object_type(par)) {
	case OT_FUNC:
		display_local(par, ((Stack *)par)->stk_func, block_start,
			      vv->name, TRUE, &obj, FALSE);
		break;
		
	case OT_SFILE:
	  	if (((fil_t *)par)->fi_symtab &&
		    ((fil_t *)par)->fi_symtab->st_eclipsed)
		  break;
		show_global((Srcwin *)NULL, (fil_t *)par, (func_t *)NULL,
			    (common_block_t *)NULL,
			    vv->name, FALSE, FALSE, TRUE, &obj, FALSE, FALSE);
		break;
		
	case OT_GLOBALS:
		show_global((Srcwin *)NULL, (fil_t *)NULL, (func_t *)NULL,
			    (common_block_t *)NULL,
			    vv->name, FALSE, FALSE, TRUE, &obj, FALSE, FALSE);
		
		break;
		
	case OT_CBLOCK:
		show_global((Srcwin *)NULL, (fil_t *)NULL, (func_t *)NULL,
			    (common_block_t *)par,
			    vv->name, FALSE, FALSE, TRUE, &obj, FALSE, FALSE);
		
		break;
		
	default:
		panic("bad objtype in rv");
	}

	if (obj != NULL)
		restore_dv_state(vv, (dvar_t *)obj);
	
	return obj;
}

static void
restore_dv_state(vv, dv)
Vsvar *vv;
dvar_t *dv;
{
	make_vv_ilist(dv, vv->indices, vv->icount);
	
	dv->dv_format = vv->format;

	if (vv->no_indent)
		set_no_indent((objid_t)dv, TRUE);

	redo_decl(dv);
}

static void
make_vv_ilist(dv, indices, count)
dvar_t *dv;
int *indices, count;
{
	int i;
	ilist_t *il;
	
	for (i = dv->dv_ilevel; i < count; ++i)
		change_dv_level(dv, CL_UP);

	/*  It is not a fatal error for there not to be enough indices,
	 *  as the saved state may not match the variable (it can come
	 *  from an out of date state file, for example).
	 */
	if (IS_FORTRAN(dv->dv_var->va_language)) {
		i = count;
		for (il = dv->dv_ilist; il != NULL; il = il->il_next) {
			if (i > 0)
				il->il_index = indices[--i];
		}
	}
	else {
		i = 0;
		for (il = dv->dv_ilist; il != NULL; il = il->il_next) {
			if (i < count)
				il->il_index = indices[i++];
		}
	}
}

static block_t *
lnum_range_to_block(parbl, start_lnum, end_lnum)
block_t *parbl;
int start_lnum, end_lnum;
{
	block_t *bl;
	
	if (start_lnum == parbl->bl_start_lnum &&
	    end_lnum == parbl->bl_end_lnum)
		return parbl;

	for (bl = parbl->bl_blocks; bl != NULL; bl = bl->bl_next) {
		block_t *cbl;

		cbl = lnum_range_to_block(bl, start_lnum, end_lnum); 
		if (cbl != NULL)
			return cbl;
	}

	return NULL;
}

objid_t
add_blocks_as_needed(par, rootbl, bl)
objid_t par;
block_t *rootbl, *bl;
{
	objid_t child;
	
	if (bl == rootbl)
		return par;
	
	if (bl->bl_parent != rootbl)
		par = add_blocks_as_needed(par, rootbl, bl->bl_parent);
	
	if ((child = find_block(par, bl)) == 0)
		child = add_block_object(par, bl);

	return child;
}

static objid_t
restore_expr(par, vc, block_start, block_end)
objid_t par;
Vsexpr *vc;
int block_start, block_end;
{
	block_t *rootbl = NULL, *bl;
	
	switch (ups_get_object_type(par)) {
	case OT_FUNC:
		rootbl = FU_BLOCKS(((Stack *)par)->stk_func);
		break;
	case OT_SFILE:
	  	if (((fil_t *)par)->fi_symtab &&
		    ((fil_t *)par)->fi_symtab->st_eclipsed)
		  break;
		rootbl = ((fil_t *)par)->fi_block;
		break;
	default:
		panic("bad objtype in rv");
		rootbl = NULL;	/* to satisfy gcc */
	}

	if (block_start == 0 && block_end == 0)
		bl = rootbl;
	else
		bl = lnum_range_to_block(rootbl, block_start, block_end);
	
	if (bl == NULL) {
		errf("Can't find block to restore expression `%s'",
		     vc->text);
		return NULL;
	}

	par = add_blocks_as_needed(par, rootbl, bl);
	
	return restore_displayed_expr(par, bl, vc->text, vc->format);
}

int
write_var_state_to_file(path, fp)
const char *path;
FILE *fp;
{
	Vstack *vs;
	Vfile *vf;

	for (vs = Vslist; vs != NULL; vs = vs->next) {
		func_t *f;
		lno_t *ln;

		if (vs->velist == NULL)
			continue;

		f = vs->func;
		ln = FU_LNOS(f);
		
		/* RGA enclose name in double quotes, to allow for spaces
		   in C++ names.
		   3.37: use unquoted mangled name so overloaded methods are unique,
		   followed by demangled name in quotes for readability
		   */
		fprintf(fp, "\nfunction %s \"%s\"",
			f->fu_name, f->fu_demangled_name);
		
		if (f->fu_fil != NULL) {
			fprintf(fp, " %s %d",
				f->fu_fil->fi_name,
				(ln != NULL) ? ln->ln_num : 0);
		}

		if (dump_children(path, fp, vs->velist, 1) != 0)
			return -1;
	}

	for (vf = Vflist; vf != NULL; vf = vf->next) {
		if (vf->velist == NULL || !vf->been_displayed)
			continue;

		fputc('\n', fp);

		switch (vf->objtype) {
			common_block_t *cb;
			const char *funcname;
			
		case OT_GLOBALS:
			fputs("globals", fp);
			break;
			
		case OT_CBLOCK:
			cb = (common_block_t *)vf->obj;
			fprintf(fp, "common %s", get_cblock_name(cb));
			
			funcname = get_cblock_funcname(cb);
			if (funcname != NULL)
				fprintf(fp, " %s", funcname);
			
			break;

		case OT_SFILE:
		  	if (((fil_t *)(vf->obj))->fi_symtab &&
			    ((fil_t *)(vf->obj))->fi_symtab->st_eclipsed)
			  continue;
			fprintf(fp, "file %s", ((fil_t *)vf->obj)->fi_name);
			break;

		default:
			panic("unknown objtype in wvstf");
			break;
		}

		if (dump_children(path, fp, vf->velist, 1) != 0)
			return -1;
	}

	return write_format_state_to_file(path, fp);
}

static Velist *
reverse_velist(velist)
Velist *velist;
{
	Velist *ve, *next, *newlist;

	newlist = NULL;

	for (ve = velist; ve != NULL; ve = next) {
		next = ve->next;
		ve->next = newlist;
		newlist = ve;
	}

	return newlist;
}

static int
dump_children(path, fp, velist, level)
const char *path;
FILE *fp;
Velist *velist;
int level;
{
	Velist *ve;
	bool done_one;

	done_one = FALSE;

	for (ve = velist; ve != NULL; ve = ve->next) {
		int i;

		if (!done_one) {
			fputs(" {\n", fp);
			done_one = TRUE;
		}
		
		for (i = 0; i < level; ++i)
			fputc('\t', fp);
		
		if (ve->block_start != 0 || ve->block_end != 0)
			fprintf(fp, "%d..%d ", ve->block_start, ve->block_end);
		
		switch (ve->type) {
		case VT_VAR:
			dump_var(fp, ve->u.var);
			break;
		case VT_EXPR:
			dump_expr(fp, ve->u.expr);
			break;
		default:
			panic("bad vetype in dc");
			break;
		}

		if (ferror(fp)) {
			failmesg("Error writing to", "file", path);
			break;
		}

		if (dump_children(path, fp, ve->children, level + 1) != 0)
			break;
	}

	if (ve != NULL)
		return -1;
	
	if (done_one) {
		int i;
		
		for (i = 0; i < level - 1; ++i)
			fputc('\t', fp);
		fputc('}', fp);
	}

	fputc('\n', fp);

	return 0;
}

static void
dump_var(fp, vv)
FILE *fp;
Vsvar *vv;
{
	int i;
	
	fprintf(fp, "var %-15s %-16s %s %d [",
		vv->name,
		format_to_string(vv->format),
		vv->no_indent ? "no-indent" : "indent",
		vv->ilevel);

	for (i = 0; i < vv->icount; ++i) {
		fprintf(fp, "%d%s",
			vv->indices[i],
			(i < vv->icount - 1) ? " " : "");
	}

	fputc(']', fp);
}

static void
dump_expr(fp, vc)
FILE *fp;
Vsexpr *vc;
{
	fprintf(fp, "expr %s ", format_to_string(vc->format));
	put_quoted_string(fp, vc->text);
}

int
handle_function_command(cmd, args, nargs, from_statefile, fp, p_lnum)
const char *cmd;
char **args;
int nargs;
bool from_statefile;
FILE *fp;
int *p_lnum;
{
	func_t *f, *f1;
	const char *funcname, *filename, *lnumstr;
	int func_lnum, line_offset;
	alloc_pool_t *ap;
	Velist *velist;
	lno_t *ln;
	Vstack *vs;
		
	if (nargs < 1 || strcmp(args[nargs - 1], "{") != 0) {
		errf("Missing `{'");
		return -1;
	}
	--nargs;

	if (nargs < 1 || nargs > 3) {
		errf("Usage: function mangled-funcname \"funcname\" [filename [lnum]] {");
		return -1;
	}

	
	funcname = args[0];
	filename = (nargs > 1) ? args[1] : NULL;
	lnumstr = (nargs > 2) ? args[2] : NULL;

	if (filename == NULL) {
		if (find_func_by_name(funcname, &f, &f1, FALSE) != 0)
			f = NULL;
	}
	else {
		fil_t *fil;
		
		if ((fil = name_to_fil(filename)) == NULL ||
		    (f = name_and_fil_to_func(funcname, fil, TRUE)) == NULL) {
			/*  Function may have moved to another file.
			 */
			if (find_func_by_name(funcname, &f, &f1, FALSE) != 0)
				f = NULL;
		}
	}

	if (lnumstr == NULL) {
		func_lnum = -1;
	}
	else {
		char *ends;

		func_lnum = strtol(lnumstr, &ends, 10);
		if (func_lnum < 0 || ends == lnumstr || *ends != '\0') {
			errf("Illegal line number `%s' ignored", lnumstr);
			func_lnum = -1;
		}
	}

	if (f != NULL && func_lnum != -1 && (ln = FU_LNOS(f)) != NULL)
		line_offset = ln->ln_num - func_lnum;
	else
		line_offset = 0;

	if (load_velist(fp, line_offset, p_lnum, f == NULL, &ap, &velist) != 0)
		return -1;
	
	if ((vs = func_to_vstack(f)) != NULL) {
		alloc_free_pool(vs->apool);
		vs->apool = ap;
		vs->velist = velist;
	}
	else {
		push_new_vstack(f, ap, velist);
	}

	return 0;
}

static int
load_velist(fp, line_offset, p_lnum, failed, p_ap, p_velist)
FILE *fp;
int line_offset;
int *p_lnum;
bool failed;
alloc_pool_t **p_ap;
Velist **p_velist;
{
	alloc_pool_t *ap;
	Velist *velist;

	ap = alloc_create_pool();
	velist = NULL;

	if (read_velist(ap, &velist, fp, line_offset, p_lnum) != 0)
		failed = TRUE;

	if (failed) {
		alloc_free_pool(ap);
		return -1;
	}

	*p_ap = ap;
	*p_velist = velist;
	return 0;
}

static int
read_velist(ap, p_velist, fp, line_offset, p_lnum)
alloc_pool_t *ap;
Velist **p_velist;
FILE *fp;
int line_offset;
int *p_lnum;
{
	int save_lnum, res;
	char *line;
	Velist first, *last;

	save_lnum = *p_lnum;

	last = &first;
	res = 0;
	
	while ((line = fpgetline(fp)) != NULL) {
		char *cmd;
		char *endline;
		bool have_children;
		Velist *ve;
		int block_start, block_end;
		
		++*p_lnum;

		line = config_trim_line(line);
		if (*line == '\0')
			continue;

		if (strcmp(line, "}") == 0)
			break;

		endline = line + strlen(line) - 1;
		have_children = *endline == '{';
		
		if (have_children)
			*endline = '\0';

		if (isdigit(*line)) {
			char *rangestr, junkc;
			
			if (!get_word(&line, "line range", &rangestr)) {
				res = -1;
				continue;
			}
			
			if (sscanf(rangestr, "%d..%d%c",
				   &block_start, &block_end, &junkc) != 2) {
				errf("Expected num..num, got `%s' - ignored",
				     rangestr);
				block_start = block_end = 0;
			}
			else {
				block_start += line_offset;
				block_end += line_offset;
			}
		}
		else {
			block_start = block_end = 0;
		}
		
		ve = (Velist *)alloc(ap, sizeof(Velist));
		ve->block_start = block_start;
		ve->block_end = block_end;
		ve->children = NULL;

		if (!get_word(&line, "`var' or `expr'", &cmd)) {
			res = -1;
		}
		else if (strcmp(cmd, "var") == 0) {
			ve->type = VT_VAR;
			ve->u.var = (Vsvar *)alloc(ap, sizeof(Vsvar));
			
			if (read_var(ap, line, ve->u.var) != 0)
				res = -1;
		}
		else if (strcmp(cmd, "expr") == 0) {
			ve->type = VT_EXPR;
			ve->u.expr = (Vsexpr *)alloc(ap, sizeof(Vsexpr));
			
			if (read_expr(ap, line, ve->u.expr) != 0)
				res = -1;
		}
		else {
			errf("Expected `var' or `expr', got `%s'", cmd);
			res = -1;
		}
		
		if (have_children &&
		    read_velist(ap, &ve->children, fp,
				line_offset, p_lnum) != 0) {
			res = -1;
		}

		last->next = ve;
		last = ve;
	}
	
	if (line == NULL) {
		*p_lnum = save_lnum;
		errf("Missing closing `}'");
		res = -1;
	}

	last->next = NULL;
	*p_velist = first.next;
	return res;
}

static bool
get_word(p_line, what, p_cmd)
char **p_line;
const char *what;
char **p_cmd;
{
	char *line;

	line = *p_line;

	while (isspace(*line))
		++line;

	if (*line == '\0') {
		errf("Expected %s but got end of line", what);
		return FALSE;
	}

	*p_cmd = line;
	
	while (!isspace(*line) && *line != '\0')
		++line;

	while (isspace(*line))
		*line++ = '\0';

	*p_line = line;
	return TRUE;
}

bool
get_num(p_line, what, p_val)
char **p_line;
const char *what;
int *p_val;
{
	char *str, *endstr;
	
	if (!get_word(p_line, what, &str))
		return FALSE;

	*p_val = strtol(str, &endstr, 10);

	if (*endstr != '\0') {
		errf("%c%s `%s' is not a decimal integer",
		     toupper(*what), what + 1, str);
		return FALSE;
	}

	return TRUE;
}

static int
read_var(ap, line, vv)
alloc_pool_t *ap;
char *line;
Vsvar *vv;
{
	static ebuf_t *eb = NULL;
	int int_nbytes, ilevel;
	size_t nbytes;
	char *name, *formatstr, *indentstr, *ibuf, *closebr;
	vformat_t format;
	bool indent;
	
	if (!get_word(&line, "variable name", &name) ||
	    !get_word(&line, "value format", &formatstr) ||
	    !get_word(&line, "`indent' or `no-indent'", &indentstr) ||
	    !get_num(&line, "indirection level", &ilevel))
		return -1;

	if (!string_to_format(formatstr, &format))
		return -1;
	
	indent = strcmp(indentstr, "indent") == 0;
	if (!indent && strcmp(indentstr, "no-indent") != 0) {
		errf("Expected `indent' or `no-indent', got `%s'", indentstr);
		return -1;
	}

	if (*line != '[') {
		if (*line == '\0')
			errf("Missing `['");
		else
			errf("Expected '[', got `%c'", *line);
		return -1;
	}

	for (++line; isspace(*line); ++line)
		;
	
	if ((closebr = strchr(line, ']')) == NULL) {
		errf("Missing ']'");
		return -1;
	}
	*closebr = '\0';
	
	eb = ebuf_start(eb, TRUE);

	while (*line != '\0') {
		int n;

		if (!get_num(&line, "array index", &n))
			return -1;

		ebuf_add(eb, (char *)&n, sizeof(n));
	}

	for (line = closebr + 1; isspace(*line); ++line)
		;
	
	if (*line != '\0') {
		errf("Unexpected text at end of line: `%s'", line);
		return -1;
	}
	
	ibuf = ebuf_get(eb, &int_nbytes);
	nbytes = (size_t)int_nbytes; /* due to wrong ebuf_get interface */

	vv->name = alloc_strdup(ap, name);
	vv->format = format;
	vv->no_indent = !indent;
	vv->ilevel = ilevel;
	vv->icount = nbytes / sizeof(int);
	vv->indices = (int *)memcpy(alloc(ap, nbytes), ibuf, nbytes);
		
	return 0;
}

static int
read_expr(ap, line, vc)
alloc_pool_t *ap;
char *line;
Vsexpr *vc;
{
	vformat_t format;
	char **words;
	int nwords;

	if (!parse_line(line, &words, &nwords))
		return FALSE;

	if (nwords != 2) {
		errf("Expected format exprstring");
		free((char *)words);
		return -1;
	}
	
	if (!string_to_format(words[0], &format)) {
		free((char *)words);
		return -1;
	}
	
	vc->text = alloc_strdup(ap, words[1]);
	vc->format = format;

	free((char *)words);
	
	return 0;
}

int
handle_file_command(cmd, args, nargs, from_statefile, fp, p_lnum)
const char *cmd;
char **args;
int nargs;
bool from_statefile;
FILE *fp;
int *p_lnum;
{
	objid_t obj;
	int res, objtype;
	alloc_pool_t *ap;
	Velist *velist;
	Vfile *vf;
	
	res = 0;

	if (strcmp(cmd, "globals") == 0) {
		if (nargs != 1 || strcmp(args[0], "{") != 0) {
			errf("Usage: globals {");
			return -1;
		}
		
		obj = GLOBALS_OBJCODE;
		objtype = OT_GLOBALS;
	}
	else if (strcmp(cmd, "file") == 0) {
		if (nargs != 2 || strcmp(args[1], "{") != 0) {
			errf("Usage: file filename {");
			return -1;
		}
		
		obj = (objid_t)name_to_fil(args[0]);

		if (obj == NULL)
			errf("Unknown filename `%s'", args[0]);

		objtype = OT_SFILE;
	}
	else if (strcmp(cmd, "common") == 0) {
		func_t *f, *f1;
		
		if ((nargs != 2 && nargs != 3) ||
		    strcmp(args[nargs - 1], "{") != 0) {
			errf("Usage: common cblockname [func] {");
			return -1;
		}
		
		if (nargs == 2 || find_func_by_name(args[1], &f, &f1, FALSE) != 0)
			f = NULL;
		
		obj = (objid_t)name_to_cblock(args[0], f);

		if (obj == NULL)
			errf("Unknown common block `%s'", args[0]);

		objtype = OT_CBLOCK;
	}
	else {
		panic("bad cmd in hfc");
		obj = NULL;		/* to satisfy gcc */
		objtype = OT_MAXTYPE;	/* to satisfy gcc */
	}

	if (load_velist(fp, 0, p_lnum, obj == NULL, &ap, &velist) != 0)
		return -1;

	if ((vf = obj_to_vfile(obj)) != NULL) {
		alloc_free_pool(vf->apool);
		vf->apool = ap;
		vf->velist = velist;
	}
	else {
		vf = push_new_vfile(obj, objtype, ap, velist);
	}

	return res;
}

void
reset_file_restore_states()
{
  Vfile *vf;
  
  for (vf = Vflist; vf != NULL; vf = vf->next) 
      vf->been_restored = FALSE;
}

void
free_vslist()
{
  Vstack *vs, *vs_next;

  for (vs = Vslist; vs != NULL;)
  {
    vs_next = vs->next;
    alloc_free_pool(vs->apool);
    free(vs);
    vs = vs_next;
  }
  Vslist = NULL;
}

void
free_vflist()
{
  Vfile *vf, *vf_next;
  
  for (vf = Vflist; vf != NULL;)
  {
    vf_next = vf->next;
    alloc_free_pool(vf->apool);
    free(vf);
    vf = vf_next;
  }
  Vflist = NULL;
}

void
update_var_state(oldst, newst)
symtab_t *oldst;
symtab_t *newst;
{
   Vstack **vsp = &Vslist;
   Vfile **vfp = &Vflist;
   
   while (*vsp) {
      Vstack *vs = *vsp;
      
      if (vs->func->fu_symtab == oldst) {
	 func_t *f, *f1;
	 fil_t *fil;
	 
	 if ((fil = name_and_st_to_fil(vs->func->fu_fil->fi_name, newst)) == NULL ||
	     (f = name_and_fil_to_func(vs->func->fu_name, fil, FALSE)) == NULL) {
	    if (find_func_by_name(vs->func->fu_name, &f, &f1, FALSE) != 0) {
	       errf("Function %s no longer appears to exist",
		    vs->func->fu_demangled_name);
	       alloc_free_pool(vs->apool);
	       free(vs);
	       *vsp = vs->next;
	       continue;
	    }
	 }

	 vs->func = f;
      }

      vsp = &(*vsp)->next;
   }

   while (*vfp) {
      Vfile *vf = *vfp;
      common_block_t *cblock;
      func_t *f;
      fil_t *fil;

      switch (vf->objtype) {
      case OT_CBLOCK:
         cblock = (common_block_t *)vf->obj;
         if (get_cblock_symtab(cblock) == oldst) {
            f = get_cblock_func(cblock);
            if ((fil = name_and_st_to_fil(f->fu_fil->fi_name, newst)) == NULL ||
                (f = name_and_fil_to_func(f->fu_name, fil, FALSE)) == NULL) {
               f = NULL;
            }
            if ((cblock = name_and_st_to_cblock(get_cblock_name(cblock), f, newst)) == NULL ) {
               errf("Common block %s no longer appears to exist", get_cblock_name(cblock));
	       alloc_free_pool(vf->apool);
	       free(vf);
	       *vfp = vf->next;
	       continue;
            }
            
            vf->obj = (objid_t)cblock;
         }         
         break;
      case OT_SFILE:
         fil = (fil_t *)vf->obj;
         if (fil->fi_symtab == oldst) {
            if ((fil = name_and_st_to_fil(fil->fi_name, newst)) == NULL ) {
               errf("File %s no longer appears to exist", fil->fi_name);
	       alloc_free_pool(vf->apool);
	       free(vf);
	       *vfp = vf->next;
	       continue;
            }
            
            vf->obj = (objid_t)fil;
         }
         break;
      case OT_GLOBALS:
         break;
      default:
         panic("unknown objtype in rfd");
         break;
      }

      vfp = &(*vfp)->next;
   }

   update_format_state(oldst, newst);
   
   return;
}
