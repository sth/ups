/* ci_util.c - utility routines for the C interpreter */

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


/* @(#)ci_util.c	1.25 04 Jun 1995 (UKC) */
char ups_ci_util_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <stdlib.h>

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <setjmp.h>
#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "expr.h"
#include "ci.h"
#include "ci_parse.h"
#include "ci_expr.h"
#include "ci_constexpr.h"
#include "ci_util.h"
#include "st.h"

/*  Alloc pool identifier for parse tree allocation.
 *  Declared extern in ci_alloc.h.
 */
alloc_pool_t *Parse_apool;

jmp_buf ci_Catch_panic_env;
bool ci_Catching_panics = FALSE;

static diag_handler_func_t Diag_handler_func;
static char *Diag_handler_arg;

#ifdef __STDC__
void
diagf(errtype_t errtype, lexinfo_t *lx, const char *fmt, ...)
{
#else /* !__STDC__ */
void
diagf(va_alist)
va_dcl
{
	errtype_t errtype;
	lexinfo_t *lx;
	char *fmt;
#endif /* !__STDC__ */
	va_list args;
	char buffer[150];
	char *s;

#ifdef __STDC__
	va_start(args, fmt);
#else
	va_start(args);
	errtype = va_arg(args, errtype_t);
	lx = va_arg(args, lexinfo_t *);
	fmt = va_arg(args, char *);
#endif

	s = formf(buffer, sizeof(buffer), fmt, args);

	va_end(args);

	if (Diag_handler_func == NULL)
		panic("no diag func");

	(*Diag_handler_func)(Diag_handler_arg, errtype, lx, s);

	if (s != buffer)
		free(s);
}

void
ci_set_diag_handler(func, arg)
diag_handler_func_t func;
char *arg;
{
	Diag_handler_func = func;
	Diag_handler_arg = arg;
}

void
ci_panic(s)
const char *s;
{
	if (ci_Catching_panics) {
		errf("Compiler error: %s", s);
		longjmp(ci_Catch_panic_env, 1);
	}
	else {
		panic(s);
	}
}

func_t *
ci_get_parse_id_funcs(parse_id)
parse_id_t parse_id;
{
	return ((parse_res_t *)parse_id)->pr_funcs;
}

/*  Create an aggregate type for an undefined structure, union or enum.
 *  If type is NULL space is allocated.
 *
 *  The created aggregate has no members.
 */
type_t *
ci_make_undef_type(ap, tag, typecode, type)
alloc_pool_t *ap;
const char *tag;
typecode_t typecode;
type_t *type;
{
	aggr_or_enum_def_t *ae;

	switch(typecode) {
	case TY_U_STRUCT:
	case TY_U_UNION:
		ae = ci_make_aggr_or_enum_def(ap, tag, typecode, type);
		ae->ae_aggr_members = NULL;
		type = ae->ae_type;
		break;
	case TY_U_ENUM:
		ae = ci_make_aggr_or_enum_def(ap, tag, typecode, type);
		ae->ae_enum_members = NULL;
		type = ae->ae_type;
		break;
	case TY_INT_ASSUMED:
		if (type == NULL)
			type = ci_code_to_type(typecode);
		else
			*type = *ci_code_to_type(typecode);
		break;
	default:
		ci_panic("unknown typecode in mut");
	}
	return type;
}

aggr_or_enum_def_t *
ci_make_aggr_or_enum_def(ap, tag, typecode, type)
alloc_pool_t *ap;
const char *tag;
typecode_t typecode;
type_t *type;
{
	aggr_or_enum_def_t *ae;

	ae = (aggr_or_enum_def_t *)alloc(ap, sizeof(aggr_or_enum_def_t));

	if (type != NULL)
		ci_init_type(type, typecode);
	else
		type = ci_make_type(ap, typecode);
	type->ty_aggr_or_enum = ae;

	ae->ae_tag = tag;
	ae->ae_is_complete = AE_INCOMPLETE;
	ae->ae_size = -1;
	/* We leave ae_alignment uninitialised */
	ae->ae_type = type;
	ae->ae_next = NULL;
	/* We leave ae_aggr_members/ae_enum_members uninitialised. */
	ae->ae_lexinfo = NULL;

	return ae;
}

aggr_or_enum_def_t *
ci_apply_to_aelist(aelist, func, arg)
aggr_or_enum_def_t *aelist;
aggr_or_enum_def_t *(*func)PROTO((aggr_or_enum_def_t *ae, const char *farg));
const char *arg;
{
	aggr_or_enum_def_t *ae, *res;

	for (ae = aelist, res = NULL; ae != NULL && res == NULL; ae = ae->ae_next) {
		if (ae->ae_type != NULL)
			res = (*func)(ae, arg);
		else
			res = ci_apply_to_aelist(ae->ae_sublist, func, arg);
	}

	return res;
}

block_t *
ci_make_block(ap, parent)
alloc_pool_t *ap;
block_t *parent;
{
	block_t *bl = (block_t *)alloc(ap, sizeof(block_t));

	bl->bl_start_lnum = 0;
	bl->bl_vars = NULL;
	bl->bl_typedefs = NULL;
	bl->bl_aggr_or_enum_defs = NULL;
	bl->bl_initlist = NULL;
	bl->bl_parent = parent;
	bl->bl_blocks = NULL;
	bl->bl_next = NULL;
	return bl;
}

fil_t *
ci_make_fil(ap, name, data, block, next)
alloc_pool_t *ap;
const char *name;
char *data;
block_t *block;
fil_t *next;
{
	fil_t *fil;
	char *c;
	int len;

	fil = (fil_t *)alloc(ap, sizeof(fil_t));

	if (name && *name == '.' && *(name+1) == '/') /* RGA */
	  fil->fi_name = name+2;
	else
	  fil->fi_name = name;
	if (name)
	{
	  len = strlen(fil->fi_name);
	  if (len > 0)
	    len --;
	  for(c = (char *)fil->fi_name + len;
	      c != (char *)fil->fi_name && c && *c !='/'; c--);
	  if (c && *c =='/')
	    c++;
	  fil->fi_name_only = (const char *)c;
	}
	else
	  fil->fi_name_only = name;
	fil->fi_flags = 0;
	fil->fi_path_hint = NULL;
	fil->fi_source_path = NULL;
	fil->fi_language = LANG_C;
	fil->fi_symtab = NULL;	/* Filled in later */
	fil->fi_data = data;
	fil->fi_srcbuf = NULL;
	fil->fi_block = block;
	fil->fi_funclist = NULL;
	fil->fi_next = next;

	return fil;
}

func_t *
ci_make_func(ap, name, addr, symtab, fil, next)
alloc_pool_t *ap;
const char *name;
taddr_t addr;
symtab_t *symtab;
fil_t *fil;
func_t *next;
{
	func_t *f;
	size_t len;
	const char *cptr;

	f = (func_t *) alloc(ap, sizeof(func_t));

	f->fu_flags = 0;
	f->fu_name = name;
				/* RGA demangle here */
	for (cptr = name; *cptr != '\0' && *cptr != ':'; cptr++) ;
	len = cptr - name;
	demangle_name_2((char *)name, len, ap, (char **)&f->fu_demangled_name, 0, fil);

	f->fu_type = NULL;
	f->fu_addr = addr;
	f->fu_fil = fil;
	f->fu_language = (fil != NULL) ? fil->fi_language : LANG_UNKNOWN;
	f->fu_symtab = symtab;
	f->fu_cblist = NULL;
	f->fu_next = next;

	f->fu_predata = NULL;
	f->fu_symdata = NULL;

	f->fu_statement = NULL;
	f->fu_lexinfo = NULL;

	f->fu__lnos = NULL;
	/* Leave fu_max_lnum ununitialised - no obvious initial value */

	f->fu__blocks = NULL;

	return f;
}

funcret_t *
ci_make_funcret(ap, params_type, nparams, is_variadic)
alloc_pool_t *ap;
params_type_t params_type;
int nparams;
bool is_variadic;
{
	funcret_t *fr;
	
	fr = (funcret_t *)alloc(ap, sizeof(funcret_t));
	
	fr->fr_params_type = params_type;
	fr->fr_nparams = nparams;
	fr->fr_is_variadic = is_variadic;
	fr->fr_params = NULL;
	fr->fr_idlist = NULL;
	fr->fr_is_old_style = params_type == FDT_IDLIST;

	return fr;
}

var_t *
ci_make_var(ap, name, class, type, addr)
alloc_pool_t *ap;
const char *name;
class_t class;
type_t *type;
taddr_t addr;
{
	var_t *v;

	v = (var_t *)alloc(ap, sizeof(var_t));
	v->va_name = name;
	v->va_class = class;
	v->va_language = LANG_C;
	v->va_flags = 0;
	v->va_type = type;
	v->va_addr = addr;
	v->va_next = NULL;
	v->va_lexinfo = NULL;
	return v;
}

initlist_t *
ci_push_initlist(newlist, list)
initlist_t *newlist, *list;
{
	initlist_t *next;

/* Following caused variable initializations to be compiled in the reverse
 * order of declaration because newlist was always inserted at the top of the
 * list. The change below will add newlist to the bottom of the list.
 * Dibyendu : 8/12/98 at 01:50 am !!
 */
#if 0
	for (; newlist != NULL; newlist = next) {
		next = newlist->il_next;
		newlist->il_next = list;
		list = newlist;
	}
#else
	if (newlist == NULL)
		return list;

	if (list == NULL)
		return newlist;

	for (next = list; next->il_next != NULL; next = next->il_next)
		;
	next->il_next = newlist;
#endif
	return list;
}

type_t *
ci_push_types(typelist, types)
type_t *typelist, *types;
{
	type_t *base;

	for (; types != NULL; types = base) {
		base = types->ty_base;
		types->ty_base = typelist;
		typelist = types;
	}
	return typelist;
}

identifier_list_t *
ci_reverse_idlist(idl)
identifier_list_t *idl;
{
	identifier_list_t *list, *next;

	list = NULL;
	for (; idl != NULL; idl = next) {
		next = idl->idl_next;
		idl->idl_next = list;
		list = idl;
	}
	return list;
}

expr_list_t *
ci_reverse_expr_list(el)
expr_list_t *el;
{
	expr_list_t *list, *next;

	list = NULL;
	for (; el != NULL; el = next) {
		next = el->el_next;
		el->el_next = list;
		list = el;
	}
	return list;
}

/*  Construct a bitfield type given an offset and a width.
 */
type_t *
ci_make_bitfield_type(ap, bftype, bit_offset, bit_width)
alloc_pool_t *ap;
type_t *bftype;
int bit_offset, bit_width;
{
	bitfield_t *bf;
	type_t *type;

	bf = (bitfield_t *) alloc(ap, sizeof(bitfield_t));
	bf->bf_type = bftype;
	bf->bf_offset = bit_offset;
	bf->bf_width = bit_width;
	bf->bf_expr_id = NULL;

	type = ci_make_type(ap, TY_BITFIELD);
	type->ty_bitfield = bf;

	return type;
}

type_t *
ci_make_expr_bitfield_type(base, expr)
type_t *base;
expr_t *expr;
{
	type_t *type;
	constval_t val;

	ci_evaluate_constant_expression(expr, "bitfield width", TRUE, &val);
	type = ci_make_bitfield_type(Parse_apool, ci_code_to_type(TY_INT),
				     0, val.cv_int);
	type->ty_base = base;
	type->ty_bitfield->bf_expr_id = (expr_id_t)expr;
	return type;
}

type_t *
ci_make_pointer(base, qualifiers)
type_t *base;
qualifiers_t qualifiers;
{
	type_t *type;

	type = ci_make_type(Parse_apool, DT_PTR_TO);
	type->ty_qualifiers = qualifiers;
	type->ty_base = base;
	return type;
}

var_t *
ci_push_vars(v, list)
var_t *v, *list;
{
	var_t *next;

	for (; v != NULL; v = next) {
		next = v->va_next;
		v->va_next = list;
		list = v;
	}
	return list;
}

type_t *
ci_make_array_type(base, expr)
type_t *base;
expr_t *expr;
{
	type_t *type;

	type = ci_make_type(Parse_apool, DT_ARRAY_OF);
	type->ty_base = base;
	type->ty_dim = NEW(dim_t);
	type->ty_dim->di_ltype = DT_CONSTANT;
	type->ty_dim->di_htype = (expr != NULL) ? DT_CONSTANT : DT__UNKNOWN;
	type->ty_dim->di_low = 0;
	type->ty_dim->di_high = 0;
	type->ty_dim->di_type = ci_code_to_type(TY_INT);
	if (expr != NULL) {
		constval_t val;

		if (ci_evaluate_constant_expression(expr, "array size",
								      TRUE, &val)) {
			if (val.cv_int <= 0)
				diagf(ET_ERROR, expr->ex_lexinfo,
					"Illegal %s array dimension %d",
					(val.cv_int == 0) ? "zero"
							  : "negative",
					val.cv_int);
			type->ty_dim->di_high = val.cv_int - 1;
		}
	}
	return type;
}

enum_member_t *
ci_build_enum_member(id, expr)
identifier_t *id;
expr_t *expr;
{
	enum_member_t *em;
	constval_t val;

	if (expr != NULL)
		ci_evaluate_constant_expression(expr, "enum constant value", TRUE,
									      &val);
	else
		val.cv_int = 0;
	em = ci_make_enum_member(Parse_apool, id->id_name, val.cv_int);
	em->em_expr_id = (expr_id_t)expr;
	em->em_lexinfo = id->id_lexinfo;
	return em;
}

enum_member_t *
ci_make_enum_member(ap, name, val)
alloc_pool_t *ap;
const char *name;
long val;
{
	enum_member_t *em;

	em = (enum_member_t *)alloc(ap, sizeof(enum_member_t));
	em->em_expr_id = NULL;
	em->em_name = name;
	em->em_val = val;
	em->em_lexinfo = NULL;
	return em;
}

declarator_t *
ci_make_declarator(type)
type_t *type;
{
	declarator_t *dr = NEW(declarator_t);

	dr->dr_type = type;
	dr->dr_initialiser = NULL;
	dr->dr_next = NULL;

	return dr;
}

initexpr_t *
ci_make_initexpr(is_list, expr, list)
bool is_list;
expr_t *expr;
initexpr_t *list;
{
	initexpr_t *in = NEW(initexpr_t);

	in->ie_is_list = is_list;
	if (is_list) {
		initexpr_t *newlist, *next;

		for (newlist = NULL; list != NULL; list = next) {
			next = list->ie_next;
			list->ie_next = newlist;
			newlist = list;
		}
		in->ie_list = newlist;
	}
	else
		in->ie_expr = expr;
	in->ie_next = NULL;

	return in;
}

type_t *
ci_code_to_type(typecode)
typecode_t typecode;
{
	static type_t typetab[TY_MAXTYPE];

	if ((int)typecode < 0 || (int)typecode >= (int)TY_MAXTYPE)
		ci_panic("bad typecode in ctt");

	if (typetab[(int)typecode].ty_name == NULL) {
		const char *name;
		typecode_t typetab_code;

		typetab_code = typecode;
		switch(typecode) {
			case TY_VOID:	name = "void";			break;
			case TY_CHAR:	name = "char";			break;
			case TY_UCHAR:	name = "unsigned char";		break;
			case TY_SHORT:	name = "short";			break;
			case TY_USHORT:	name = "unsigned short";	break;
			case TY_INT:	name = "int";			break;
			case TY_BOOLEAN:name = "bool";			break;
			case TY_UINT:	name = "unsigned int";		break;
			case TY_LONG:	name = "long";			break;
#if WANT_LL
			case TY_LONGLONG:name = "long long";		break;
#endif
			case TY_ULONG:	name = "unsigned long";		break;
#if WANT_LL
			case TY_ULONGLONG:name = "unsigned long long";	break;
#endif
			case TY_FLOAT:	name = "float";			break;
			case TY_DOUBLE:	name = "double";		break;
#if WANT_LDBL
			case TY_LONGDOUBLE:name = "long double";	break;
#endif
			case TY_INT_ASSUMED:
				name = "(int assumed)";
				typetab_code = TY_INT;
				break;
			case TY_ELLIPSIS:
			case TY_SIGNED:
			case TY_UNSIGNED:
				name = "<internal type>";
				break;
			default:
				ci_panic("bad typecode in ctt");
				name = 0; /* to satisfy gcc */
				break;
		
		}
		typetab[(int)typecode].ty_code = typetab_code;
		typetab[(int)typecode].ty_size = -1;
		typetab[(int)typecode].ty_base = NULL;
		typetab[(int)typecode].ty_typedef = NULL;
		typetab[(int)typecode].ty_name = name;
	}
	return &typetab[(int)typecode];
}

typedef_t *
ci_make_typedef(ap, name, type)
alloc_pool_t *ap;
const char *name;
type_t *type;
{
	typedef_t *td;

	td = (typedef_t *)alloc(ap, sizeof(typedef_t));
	td->td_name = name;
	td->td_lexinfo = NULL;
	td->td_type = type;
	td->td_sublist = NULL;
	if (type)
	    type->ty_typedef = td;

	return td;
}

type_t *
ci_make_type(ap, typecode)
alloc_pool_t *ap;
typecode_t typecode;
{
	type_t *type;

	type = (type_t *)alloc(ap, sizeof(type_t));
	ci_init_type(type, typecode);
	return type;
}

void
ci_init_type(type, typecode)
type_t *type;
typecode_t typecode;
{
	type->ty_code = typecode;
	type->ty_qualifiers = 0;
	type->ty_size = -1;	/* i.e., not yet set */
	type->ty_base = NULL;
	type->ty_typedef = NULL;
	type->ty_lexinfo = NULL;
	type->ty_name = NULL;
	type->ty_dim = NULL;	/* for safety */
}

const char *
ci_lexinfo_to_string(lx, prev_lx)
lexinfo_t *lx, *prev_lx;
{
	static char buf[512];

	if (strcmp(prev_lx->lx_filename, lx->lx_filename) == 0)
		strnf(buf, sizeof(buf), "line %d", lx->lx_lnum);
	else
		strnf(buf, sizeof(buf), "line %d of %s", 
						lx->lx_lnum, lx->lx_filename);
	return buf;
}

/* RCB:
 *   Added ci_set_sizeof_bool and ci_effective_typecode
 *   to handle that the bool type may be of size 1 or 4.
 */

static typecode_t effective_bool_type = TY_VOID;
void ci_set_sizeof_bool(size)
int size;
{
   typecode_t bool_type;
   static bool warnedOnce = FALSE;
   switch(size)
   {
   case 1:
      bool_type = TY_UCHAR;break;
   case 2:
      bool_type = TY_USHORT;break;
   case 4:
      bool_type = TY_ULONG;break;
   }
   if ( effective_bool_type != TY_VOID
      && effective_bool_type != bool_type
      && !warnedOnce)
   {
	static const char* warnVar = "UPS_WARN_INCONSISTENT_BOOL";
	const char* warn = getenv(warnVar);
	if ( !warn || strcmp(warn, "off"))
	{
	   fprintf(stderr,
	      "Warning:  sizeof(bool) inconsistent across compile modules!\n\
	  This may be caused by compiling with different versions of g++.\n\
	  (You may suppress this warning by setting the environment\n\
	  variable %s=off)\n", warnVar);
	}
	warnedOnce = TRUE;
   }
   effective_bool_type = bool_type;
}
typecode_t ci_effective_typecode(typecode)
typecode_t typecode;
{
    if ( typecode == TY_BOOLEAN)
       return effective_bool_type;
    return typecode;
}
