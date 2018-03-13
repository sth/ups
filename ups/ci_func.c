/* ci_func.c - function declaration handling for the C interpreter */

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


/* @(#)ci_func.c	1.20 04 Jun 1995 (UKC) */
char ups_ci_func_c_rcsid[] = "$Id$";


#include <string.h>

#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "expr.h"
#include "ci.h"
#include "ci_parse.h"
#include "ci_util.h"
#include "ci_decl.h"
#include "ci_func.h"
#include "ci_types.h"
#include "ci_expr.h"

static var_t *build_params_from_idlist PROTO((func_t *f, funcret_t *fr,
								var_t *vlist));
static void check_labels PROTO((func_t *f));

const char ci_Illegal_return_from_start_message[] =
					"Illegal return from function $start";

/*  BUG: GET RID OF THESE.
 */
extern func_t *Funclist;
extern block_t *Current_block;

/*  The labels list for the current function.
 */
static goto_label_t *Labels;

goto_label_t *
ci_name_to_label(id, is_definition)
identifier_t *id;
bool is_definition;
{
	goto_label_t *gl;

	for (gl = Labels; gl != NULL; gl = gl->gl_next)
		if (strcmp(id->id_name, gl->gl_name) == 0)
			break;

	if (gl == NULL) {
		gl = NEW(goto_label_t);
		gl->gl_name = id->id_name;
		gl->gl_lexinfo = id->id_lexinfo;
		gl->gl_used = !is_definition;
		gl->gl_defined = FALSE;
		gl->gl_labeldesc = NO_LABELDESC;
		gl->gl_next = Labels;
		Labels = gl;
	}

	if (is_definition) {
		if (gl->gl_defined) {
			diagf(ET_ERROR, gl->gl_lexinfo,
			     "Label `%s' already declared at %s in function %s",
				id->id_name,
				ci_lexinfo_to_string(gl->gl_lexinfo,id->id_lexinfo),
				Funclist->fu_name);
		}
		gl->gl_defined = TRUE;
	}
	else
		gl->gl_used = TRUE;

	return gl;
}
		
static void
check_labels(f)
func_t *f;
{
	goto_label_t *gl;

	for (gl = Labels; gl != NULL; gl = gl->gl_next) {
		if (!gl->gl_defined)
			diagf(ET_ERROR, gl->gl_lexinfo,
			      "Label `%s' undefined in function %s",
						     gl->gl_name, f->fu_demangled_name);

		if (!gl->gl_used)
			diagf(ET_WARNING, gl->gl_lexinfo,
			      "Label `%s' never used in function %s",
			      gl->gl_name, f->fu_demangled_name);
	}
}

void
ci_check_return_expr(expr)
expr_t *expr;
{
	func_t *f;
	type_t *ftype;

	f = Funclist;

	if (f->fu_type->ty_code != DT_FUNC_RETURNING)
		ci_panic("func type botch in cre");
	ftype = f->fu_type->ty_base;

	if (strcmp(f->fu_demangled_name, "$start") == 0) {
		diagf(ET_ERROR, expr != NULL ? expr->ex_lexinfo : NULL,
					ci_Illegal_return_from_start_message);
		return;
	}

	if (expr == NULL) {
		if (ftype->ty_code != TY_VOID)
			diagf(ET_WARNING, (lexinfo_t *)NULL,
			     "Return with no value from non-void function %s",
								f->fu_demangled_name);
		return;
	}
	
	if (ftype->ty_code == TY_VOID) {
		diagf(ET_WARNING, expr->ex_lexinfo,
		      "Return with a value from void function %s", f->fu_demangled_name);
		return;
	}

	if (expr->ex_type == NULL)
		return;
	
	if (ci_do_assignment_conversions(ftype, expr) != 0) {
		diagf(ET_WARNING, expr->ex_lexinfo,
		      "Type clash in return statement in function %s",
								f->fu_demangled_name);
		ci_show_type(ftype,         "Function type");
		ci_show_type(expr->ex_type, "Returned type");
	}
}
				
void
ci_end_func(statement)
statement_t *statement;
{
	func_t *f;

	f = Funclist;
	check_labels(f);
	f->fu_statement = statement;
	ci_add_var_decl_for_func_if_necessary(f);
}

type_t *
ci_make_funcret_type(base, params_type, declarations, idlist)
type_t *base;
params_type_t params_type;
declaration_t *declarations;
identifier_list_t *idlist;
{
	int nparams;
	bool is_variadic;
	var_t *varlist;
	type_t *type;
	funcret_t *fr = NEW(funcret_t);

	is_variadic = FALSE;
	nparams = 0;
	if (params_type == FDT_TYPELIST) {
		var_t *v, *prev;
		initlist_t *initlist;

		ci_declarations_to_varlist(declarations, CL_ARG,
							&varlist, &initlist);
		ci_complain_about_any_initialisers(initlist, "formal parameter");
		prev = NULL;
		for (v = varlist; v != NULL; v = v->va_next) {
			if (v->va_type->ty_code == TY_ELLIPSIS) {
				if (prev == NULL)
					ci_panic("ellipsis botch in mft");
				prev->va_next = NULL;
				is_variadic = TRUE;
			}
			else {
				++nparams;
				prev = v;
			}
		}
		if (nparams == 1 && varlist->va_name == NULL &&
					      varlist->va_type->ty_code == TY_VOID) {
			nparams = 0;
			varlist = NULL;
		}
	}
	else {
		identifier_list_t *idl;

		for (idl = idlist; idl != NULL; idl = idl->idl_next)
			++nparams;
		varlist = NULL;
	}

	fr = ci_make_funcret(Parse_apool, params_type, nparams, is_variadic);
	fr->fr_params = varlist;
	fr->fr_idlist = idlist;
	
	type = ci_make_type(Parse_apool, DT_FUNC_RETURNING);
	type->ty_base = base;
	type->ty_funcret = fr;
	return type;
}

func_t *
ci_start_func(declaration)
declaration_t *declaration;
{
	var_t *v;
	namedesc_t *nd;
	initlist_t *initlist;
	func_t *f;
	
	ci_start_block(FALSE);
	ci_set_formal_params_flag(TRUE);

	ci_declarations_to_varlist(declaration, CL_EXT, &v, &initlist);
	ci_complain_about_incomplete_types(v, "Function");
	ci_complain_about_any_initialisers(initlist, "function definition");

	/*  BUG: For reasons that I don't understand, a top-level declaration
	 *       that's missing a type and storage class (i.e. has just a name)
	 *	 gets parsed as a function definition.  The return here and
	 *	 the check for f==NULL in ci_check_func_decls() is just to
	 *	 avoid a panic.  Yacc goes on to report a syntax error.
	 */
	if (v->va_type == NULL || v->va_type->ty_code != DT_FUNC_RETURNING) {
		return NULL;
		/* ci_panic("func type botch in sf"); */
	}

	/*  If there is no previous declaration for this function, or only
	 *  an old style declaration, then add this declaration at the top level.
	 *
	 *  The case where there is already a declaration is handled
	 *  in ci_check_func_decls() below.
	 */
	nd = ci_lookup_name(v->va_name);

	if (nd == NULL || (nd->nd_nametype == NT_FUNCNAME &&
	    nd->nd_var->va_type->ty_funcret->fr_params_type == FDT_IDLIST)) {
		v->va_next = Current_block->bl_parent->bl_vars;
		Current_block->bl_parent->bl_vars = v;
	}

	f = ci_make_func(Parse_apool, v->va_name, 0, (symtab_t *)NULL,
			 (fil_t *)NULL, Funclist);

	f->fu_flags = FU_DONE_BLOCKS;
	f->fu_lexinfo = v->va_lexinfo;
	f->fu_type = v->va_type;
	f->fu__blocks = Current_block;
	f->fu_language = LANG_C;

	Funclist = f;

	Labels = NULL;

	return f;
}

void
ci_check_func_decls(f)
func_t *f;
{
	funcret_t *fr;
	var_t *v;

	/*  See the BUG comment in ci_start_func().
	 */
	if (f == NULL)
		return;

	if (f->fu_type->ty_code != DT_FUNC_RETURNING)
		ci_panic("bad type for func in cfd");
	fr = f->fu_type->ty_funcret;

	if (fr->fr_params_type == FDT_TYPELIST) {
		if (Current_block->bl_vars != NULL)
			diagf(ET_ERROR, f->fu_lexinfo,
		  "%s has both old and new style argument declarations",
							f->fu_demangled_name);
	}
	else {
		ci_complain_about_any_initialisers(Current_block->bl_initlist,
							    "formal parameter");
		fr->fr_params = build_params_from_idlist(f, fr,
							Current_block->bl_vars);
		fr->fr_params_type = FDT_TYPELIST;
	}
	Current_block->bl_vars = fr->fr_params;

	for (v = Current_block->bl_vars; v != NULL; v = v->va_next) {
		if (v->va_class != CL_ARG) {
			diagf(ET_ERROR, v->va_lexinfo,
		      "Storage class specified for formal parameter `%s'",
							v->va_name);
		}
	}

	if (!ci_complain_about_any_void_types(fr->fr_params)) {
		namedesc_t *nd;
		block_t *save_block;

		/* Lookup fix : Dibyendu 10/1/99 */
		/* The function name would have been defined in the parent
		 * block of this function. Avoid searching the function's own
		 * block because a parameter may have the same name as
		 * the function in which it is declared (!).
		 * e.g.   void ss(int ss) {}
		 */
		save_block = Current_block;
		Current_block = Current_block->bl_parent;

		nd = ci_lookup_name(f->fu_demangled_name);

		Current_block = save_block;	/* Lookup fix. */
		
		/*  ci_start_func() above should have added a declaration
		 *  for this function if there wasn't one already.
		 */
		if (nd == NULL)
			ci_panic("missing func decl botch in cfd");

		if (nd->nd_nametype != NT_FUNCNAME) {
			report_redecl(f->fu_demangled_name, NT_FUNCNAME, f->fu_lexinfo,
						nd->nd_nametype, nd->nd_lexinfo);
		}
		else if (nd->nd_var->va_type->ty_funcret->fr_params_type !=
								FDT_TYPELIST) {
			diagf(ET_WARNING, f->fu_lexinfo,
			      "No prototype in scope for function %s",
								f->fu_demangled_name);
		}
		else {
			ci_complain_if_types_differ(f->fu_demangled_name,
						 nd->nd_var->va_type, nd->nd_lexinfo,
						 f->fu_type, f->fu_lexinfo);
			
			/*  If this entry is currently just a declaration, mark
			 *  it as an external variable, available to other source
			 *  files.  Note that this doesn't happen for static
			 *  functions.
			 */
			switch (nd->nd_var->va_class) {
			case CL_DECL:
				nd->nd_var->va_class = CL_EXT;
				break;
			case CL_STAT:
				f->fu_flags |= FU_STATIC;
				break;
			default:
				ci_panic("bad class in cfd");
			}
		}
	}

	ci_rewrite_func_params(fr);

	ci_complain_about_incomplete_types(fr->fr_params, "Formal parameter");

	ci_set_formal_params_flag(FALSE);
}

static var_t *
build_params_from_idlist(f, fr, vlist)
func_t *f;
funcret_t *fr;
var_t *vlist;
{
	bool had_missing_decl, had_disorder;
	var_t *v, *newlist;
	identifier_list_t *idl;

	had_missing_decl = had_disorder = FALSE;
	newlist = NULL;
	for (idl = fr->fr_idlist; idl != NULL; idl = idl->idl_next) {
		var_t *prev;
		const char *name;

		name = idl->idl_id->id_name;
		prev = NULL;
		for (v = vlist; v != NULL; v = v->va_next) {
			if (strcmp(v->va_name, name) == 0)
				break;
			prev = v;
		}

		if (v != NULL) {
			if (v != vlist)
				had_disorder = TRUE;
			if (prev != NULL)
				prev->va_next = v->va_next;
			else
				vlist = v->va_next;
		}
		else {
			v = ci_make_var(Parse_apool, name, CL_ARG,
							ci_code_to_type(TY_INT), 0);
			v->va_flags = VA_IS_CI_VAR;
			v->va_lexinfo = idl->idl_id->id_lexinfo;
			diagf(ET_WARNING, v->va_lexinfo,
			      "Type defaults to int for parameter `%s'", name);
			had_missing_decl = TRUE;
		}

		v->va_next = newlist;
		newlist = v;
	}

	if (!had_missing_decl && had_disorder) {
		diagf(ET_WARNING, f->fu_lexinfo,
		      "Order of parameters and declarations of %s differ",
							f->fu_demangled_name);
	}
	for (v = vlist; v != NULL; v = v->va_next)
		diagf(ET_ERROR, v->va_lexinfo,
				"No parameter matching \"%s\"", v->va_name);
	
	return newlist;
}

expr_list_t *
ci_check_func_call(func_expr, funcname, arglist, is_implicit_declaration)
expr_t *func_expr;
const char *funcname;
expr_list_t *arglist;
bool is_implicit_declaration;
{
	funcret_t *fr;
	var_t *v;
	expr_list_t *el;
	int nargs, argno;

	/*  Count and reverse the argument list.
	 */
	arglist = ci_reverse_expr_list(arglist);
	nargs = 0;
	for (el = arglist; el != NULL; el = el->el_next)
		++nargs;

	if (func_expr->ex_type->ty_code != DT_PTR_TO ||
	    func_expr->ex_type->ty_base->ty_code != DT_FUNC_RETURNING)
		ci_panic("non func passed to cfc");
	fr = func_expr->ex_type->ty_base->ty_funcret;

	/*  No checks if we are calling an old style function or function pointer.
	 */
	if (fr->fr_is_old_style) {
		if (func_expr->ex_exprtype == ET_FUNCNAME) {
			if (!is_implicit_declaration) {
				diagf(ET_WARNING, func_expr->ex_lexinfo,
					"No prototype in scope for function %s",
					       	    func_expr->ex_var->va_name);
			}
		}
		else
			diagf(ET_WARNING, func_expr->ex_lexinfo,
			"Function call via old style function expression %s",
								funcname);

		for (el = arglist; el != NULL; el = el->el_next)
			ci_do_old_style_argument_promotions(el->el_expr);
		
		return arglist;
	}

	/*  Right #args?
	 */
	if (nargs < fr->fr_nparams) {
		diagf(ET_ERROR, func_expr->ex_lexinfo,
		      "Only %d arguments passed to function %s (expected %s%d)",
			nargs, funcname,
			fr->fr_is_variadic ? "at least " : "", fr->fr_nparams);
		return arglist;
	}
	if (!fr->fr_is_variadic && nargs > fr->fr_nparams) {
		diagf(ET_ERROR, func_expr->ex_lexinfo,
		      "Function %s expects only %d arguments (%d passed)",
					    funcname, fr->fr_nparams, nargs);
		return arglist;
	}

	/*  Right types?
	 */
	argno = 1;
	el = arglist;
	for (v = fr->fr_params; v != NULL; v = v->va_next) {
		type_t *argtype;

		argtype = el->el_expr->ex_type;

		if (ci_do_assignment_conversions(v->va_type, el->el_expr) != 0) {
			diagf(ET_WARNING, el->el_expr->ex_lexinfo,
				"Type clash on argument %d of %s",
							     argno, funcname);
			ci_show_type(v->va_type, "Formal parameter type");
			ci_show_type(argtype,    "Actual parameter type");
		}
		el = el->el_next;
		++argno;
	}

	/*  Default promotions for remaining arguments - these occur
	 *  for calls to foo(a, b, ...) type functions.
	 */
	for (; el != NULL; el = el->el_next)
		ci_do_old_style_argument_promotions(el->el_expr);

	return arglist;
}
