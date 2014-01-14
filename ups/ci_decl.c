/* ci_decl.c - routines used by ci_parse.y to build the parse tree */

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


/* @(#)ci_decl.c	1.23 04 Jun 1995 (UKC) */
char ups_ci_decl_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <string.h>
#include <stdlib.h>

#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "ci_parse.h"
#include "ci_decl.h"
#include "ci_types.h"
#include "ci_util.h"
#include "ci_lex.h"
#include "ci_init.h"

static namedesc_t *lookup_name_in_block PROTO((block_t *block, const char *name));
static namedesc_t *lookup_name_in_varlist PROTO((var_t *vlist, const char *name));
static void check_redeclaration PROTO((namedesc_t *nd, var_t *v));
static statement_t *reverse_statements PROTO((statement_t *st));
static bool decl_wanted PROTO((namedesc_t *nd, var_t *v, initlist_t *il));
static taddr_t set_bitfield_addr PROTO((var_t *v, var_t *prev, taddr_t addr));
static type_t *check_bitfield_type PROTO((type_t *type, lexinfo_t *lx));
static bool is_incomplete_type PROTO((type_t *type));
static var_t *push_var_definitions PROTO((var_t *v, var_t *list));
static aggr_or_enum_def_t *find_tag PROTO((aggr_or_enum_def_t *ae, const char *tag));
static aggr_or_enum_def_t *lookup_enum_member PROTO((aggr_or_enum_def_t *ae,
								const char *tag));
static typedef_t *name_to_typedef PROTO((typedef_t *tdlist, const char *name));
static namedesc_t *make_namedesc PROTO((var_t *v));
static bool is_decl PROTO((var_t *v));

/*  These globals are active during a given run of yyparse() only.
 *
 *  BUG: Current_block and Funclist not static, as used in ci_func.c
 */
block_t *Current_block;
func_t *Funclist;
static var_t *Varlist;
ci_resolve_name_func_t Resolve_name_func;
static bool Doing_formal_parameters = FALSE;

static bool Make_externs_toplevel;

/*  The outermost block created by the C interpreter.  The parent of this
 *  block is not necessarily NULL - see ci_start_parse_tree().
 */
static block_t *Outer_ci_block;

/*  Return TRUE if a redeclaration is wanted.
 *
 *  This function returns FALSE is the new declaration is an old style
 *  function declaration and we already have a new style declaration.
 *
 *  It also returns FALSE if the declaration is not a definition and there
 *  is a previous declaration or definition.
 *
 *  In other words, we don't want an old style declaration to hide an
 *  existing prototype and thus lose type checking.
 */
static bool
decl_wanted(nd, v, il)
namedesc_t *nd;
var_t *v;
initlist_t *il;
{
	type_t *oldtype, *newtype;

	if (nd->nd_nametype != NT_FUNCNAME && nd->nd_nametype != NT_VARNAME)
		return TRUE;

	if (nd->nd_nametype == NT_VARNAME && v->va_class == CL_DECL)
		return FALSE;
	
	oldtype = nd->nd_var->va_type;
	newtype = v->va_type;

	/*  If we are seeing a definition of a variable for which we
	 *  already have a declaration, turn the declaration into a
	 *  definition by copying the relevant bits from the definition,
	 *  and forget about the new definition (i.e. return FALSE).
	 */
	if (nd->nd_nametype == NT_VARNAME &&
	    nd->nd_var->va_class == CL_DECL && v->va_class != CL_DECL) {
		if (!ci_types_same(oldtype, newtype))
			ci_panic("type botch in dw");
		nd->nd_var->va_class = v->va_class;

		if ((nd->nd_var->va_flags & ~VA_IS_CI_VAR) != 0)
			ci_panic("unexpected va_flags in dw");
		nd->nd_var->va_flags = v->va_flags;

		/*  It is important to copy the type here, as the definition
		 *  of the variable may include information like the size of
		 *  arrays which is absent in the declaration.
		 */
		nd->nd_var->va_type = v->va_type;

		/*  If there is an initialiser for the variable, make it
		 *  point at what is now the definition.
		 */
		if (il != NULL && il->il_var == v)
			il->il_var = nd->nd_var;

		return FALSE;
	}

	if (nd->nd_var->va_class != CL_DECL ||
	    v->va_class != CL_DECL ||
	    oldtype->ty_code != DT_FUNC_RETURNING ||
	    newtype->ty_code != DT_FUNC_RETURNING)
		return TRUE;
	
	return oldtype->ty_funcret->fr_is_old_style ||
					!newtype->ty_funcret->fr_is_old_style;
}

void
ci_declarations_to_varlist(declarations, default_class, p_vlist, p_initlist)
declaration_t *declarations;
class_t default_class;
var_t **p_vlist;
initlist_t **p_initlist;
{
	var_t *vlist;
	initlist_t *initlist;
	declaration_t *dn;

	initlist = NULL;
	vlist = NULL;
	for (dn = declarations; dn != NULL; dn = dn->dn_next) {
		declarator_t *dr;
		class_t class;
		block_t *block;

		if ((class = dn->dn_class) == CL_NOCLASS)
			class = default_class;
		
		/*  The `auto' keyword in a formal parameter declaration
		 *  means class CL_ARG, not CL_AUTO.
		 */
		if (class == CL_AUTO && default_class == CL_ARG)
			class = CL_ARG;
		
		if (Make_externs_toplevel && class == CL_DECL &&
							default_class == CL_EXT)
			block = Outer_ci_block;
		else
			block = Current_block;

		for (dr = dn->dn_declarators; dr != NULL; dr = dr->dr_next) {
			const char *name;
			bool is_bitfield;
			lexinfo_t *lx;
			type_t *type, *basetype;
			var_t *v;
			bool want_decl;

			is_bitfield = dr->dr_type != NULL &&
					   dr->dr_type->ty_code == TY_BITFIELD;


			if ((basetype = ci_get_dn_basetype(dn)) != NULL)
				type = ci_push_types(basetype, dr->dr_type);
			else
				type = NULL;

			if (type != NULL && type->ty_code == TY_IDENTIFIER) {
				name = type->ty_identifier->id_name;
				lx = type->ty_identifier->id_lexinfo;
				type = type->ty_base;
			}
			else {
				name = NULL;
				lx = NULL;
			}

			if (type != NULL && is_bitfield)
				type = check_bitfield_type(type, lx);
			
			/*  We don't object to void types here if the name
			 *  is null.  This is so we don't bomb on the
			 *  special case prototype syntax f(void).
			 */
#if 0
			if (type != NULL && type->ty_code == TY_VOID &&
								name != NULL) {
#else
			if (type != NULL && type->ty_code == TY_VOID 
			&& name != NULL && class != CL_TYPEDEF) {
				/* Allow 'typedef void void_t;' */
#endif
				diagf(ET_ERROR, lx,
					"Illegal type void for `%s'", name);
				type = ci_code_to_type(TY_INT);
			}
								
			v = ci_make_var(Parse_apool, name, class, type, 0);
			v->va_lexinfo = lx;
			v->va_flags |= VA_IS_CI_VAR;

			if (type != NULL &&
			    type->ty_code == DT_FUNC_RETURNING &&
			    (class == CL_EXT || class == CL_AUTO))
				v->va_class = CL_DECL;

			if (type != NULL && dr->dr_initialiser != NULL) {
				initlist_t *il;

				il = ci_make_var_initialiser(v,
							dr->dr_initialiser);
				if (il != NULL) {
					il->il_next = initlist;
					initlist = il;
				}
			}

			if (type != NULL && type->ty_code == DT_FUNC_RETURNING)
				ci_rewrite_func_params(type->ty_funcret);

			want_decl = TRUE;

			if (name != NULL) {
				namedesc_t *nd;

				name = v->va_name;
				nd = lookup_name_in_varlist(vlist, name);
				if (nd != NULL && type != NULL) {
					check_redeclaration(nd, v);
					want_decl = decl_wanted(nd, v, initlist);
				}
				else if (default_class == CL_EXT ||
					 default_class == CL_AUTO ||
					 default_class == CL_TYPEDEF) {
					nd = lookup_name_in_block(block, name);
					if (nd != NULL && type != NULL) {
						check_redeclaration(nd, v);
						want_decl = decl_wanted(nd, v,
									initlist);
					}
				}
			}

			if (want_decl) {
				v->va_next = vlist;
				vlist = v;
			}
		}
	}
	*p_vlist = vlist;
	*p_initlist = initlist;
}

/*  BUG: Should build list of storage classes and complain later, then
 *       we'd be able to give the variable name and lexinfo.  We could
 *	 also look at the total result and give a better message.
 */
void
ci_set_storage_class(dn, class)
declaration_t *dn;
class_t class;
{
	if (class == CL_NOCLASS)
		return;

	if (dn->dn_class != CL_NOCLASS) {
		if (dn->dn_class == class)
			diagf(ET_WARNING, (lexinfo_t *)NULL,
					"Repeated storage class specifiers");
		else {
			diagf(ET_WARNING, (lexinfo_t *)NULL,
				 "Multiple different storage class specifiers");
			class = CL_NOCLASS;	/* avoid furthur messages */
		}
	}

	dn->dn_class = class;
}

void
ci_complain_about_any_initialisers(initlist, what)
initlist_t *initlist;
const char *what;
{
	initlist_t *il;

	for (il = initlist; il != NULL; il = il->il_next) {
		const char *name;

		name = il->il_var->va_name;
		if (name == NULL)
			name = "";
		diagf(ET_ERROR, il->il_var->va_lexinfo,
			"Illegal initialisation of %s `%s'", what, name);
	}
}

void
ci_complain_about_incomplete_types(vlist, what)
var_t *vlist;
const char *what;
{
	var_t *v;
	type_t *type;

	for (v = vlist; v != NULL; v = v->va_next) {
		type = v->va_type;

		if (type == NULL)
			continue;

		if (type->ty_code == DT_FUNC_RETURNING)
			type = type->ty_base;
		else if (v->va_class == CL_DECL)
			continue;

		if (is_incomplete_type(type)) {
			diagf(ET_ERROR, v->va_lexinfo,
			      "%s `%s' is of incomplete type", what,v->va_name);
			ci_show_type(type, "type");
		}
	}
}

static aggr_or_enum_def_t *
find_tag(ae, tag)
aggr_or_enum_def_t *ae;
const char *tag;
{
	if (ae->ae_tag != NULL && strcmp(ae->ae_tag, tag) == 0)
		return ae;
	
	return NULL;
}

type_t *
ci_build_aggr_or_enum_def(typecode, id, new_is_complete, aggr_decls, enum_members, is_definition)
typecode_t typecode;
identifier_t *id;
ae_is_complete_t new_is_complete;
declaration_t *aggr_decls;
enum_member_t *enum_members;
bool is_definition;
{
	const char *tag;
	block_t *bl;
	aggr_or_enum_def_t *ae = NULL;

	tag = (id == NULL) ? NULL : id->id_name;
	if (tag != NULL) {
		for (bl = Current_block; bl != NULL; bl = bl->bl_parent) {
			ae = ci_apply_to_aelist(bl->bl_aggr_or_enum_defs,
						find_tag, tag);
			if (ae != NULL)
				break;
		}
	}
	else {
		bl = NULL;	/* to satisfy gcc */
	}

	/*  Give error messages for any enum members which are
	 *  redeclarations of identifiers already declared at
	 *  this level.
	 */
	if (typecode == TY_ENUM) {
		enum_member_t *em1, *em2;

		for (em1 = enum_members; em1 != NULL; em1 = em1->em_next)
			for (em2 = enum_members; em2 != em1; em2 = em2->em_next)
				if (strcmp(em1->em_name, em2->em_name) == 0)
					report_redecl(em1->em_name,
						      NT_ENUM_CONST,em1->em_lexinfo,
						      NT_ENUM_CONST,em2->em_lexinfo);
	}

	/* Redeclaration Fix : Dibyendu 8/1/99 */
	if (ae != NULL && ae->ae_is_complete == AE_INCOMPLETE) {

		/* A redeclaration in the same scope is valid */
		if (bl == Current_block && new_is_complete == AE_INCOMPLETE)
			return ae->ae_type;

	}

	if (ae != NULL && ae->ae_is_complete == AE_COMPLETE) {
		/* Redeclaration Fix : Dibyendu 8/1/99 */
		/* Because an incomplete type can mean the start of a new type
		 * if the complete type was defined in the enclosing block.
		 */
		if (bl == Current_block && new_is_complete == AE_INCOMPLETE)
			return ae->ae_type;

		/*  Redeclaration.  Print an error message if it's at the same
		 *  level as the original declaration, then forget the old
		 *  declaration so we can insert a new one.
		 */
		/* Redeclaration Fix : Dibyendu 8/1/99 */
		/* A redeclaration is an error if both are complete
		 * definitions and in the same block.
		 */
		if (bl == Current_block && new_is_complete == AE_COMPLETE) {
			report_redecl(tag, NT_TAG, (id == NULL) ? NULL
								: id->id_lexinfo,
					   NT_TAG, ae->ae_lexinfo);
			ae = NULL;
		}

		/* Redeclaration Fix : Dibyendu 8/1/99 */
		/* A complete definition in this block will hide
		 * any previous complete definition in enclosing block.
		 * The special case of 'struct tag;' also hides any complete
		 * definition of tag in enclosing scope.
		 */
		else if (new_is_complete == AE_COMPLETE ||
		   (new_is_complete == AE_INCOMPLETE && id->id_semi_follows)) {
			ae = NULL;
		}
		else {
			/* See changes in the ci_parse.y. We now define
			 * an incomplete type immediately after we see
			 * the tag (ISO C) - earlier this was deferred until we
			 * saw a reference to the tag.
			 * The is_definition flag tells us that although
			 * this is currently being defined as an incomplete
			 * type - its full definition is following
			 * immediately after. Hence this definition hides
			 * any previous definition in enclosing block.
			 */
			if (is_definition)
				ae = NULL;
			else
				return ae->ae_type;
		}

	}
			
	if (ae == NULL) {
		/*  This aggregate or enum definition has not been seen
		 *  before at this level.
		 *
		 *  Create a new aggregate entry, and assign the fields of
		 *  the type that don't change when an incomplete type is
		 *  completed.
		 */
		ae = ci_make_aggr_or_enum_def(Parse_apool, tag, typecode,
								(type_t *)NULL);

		ae->ae_next = Current_block->bl_aggr_or_enum_defs;
		Current_block->bl_aggr_or_enum_defs = ae;
	}

	/*  At this point, there was either no previous definition of this
	 *  type, or the previous definition did not give the members.
	 */

	/*  Assign the fields that change when an incomplete type is completed.
	 */
	ae->ae_is_complete = new_is_complete;
	ae->ae_type->ty_code = typecode;
	if (id != NULL)
		ae->ae_lexinfo = id->id_lexinfo;

	if (typecode == TY_ENUM) {
		enum_member_t *em;

		for (em = enum_members; em != NULL; em = em->em_next)
			em->em_enum = ae;

		ae->ae_enum_members = enum_members;
	}
	else {
		var_t *v, *prev, *aggr_members;
		initlist_t *initlist;
		class_t class;
		taddr_t addr, max_addr;
		int alignment, max_alignment;

		class = (typecode == TY_STRUCT) ? CL_MOS : CL_MOU;
		ci_declarations_to_varlist(aggr_decls, class,
							&aggr_members, &initlist);
		ci_complain_about_any_initialisers(initlist, "Struct/union member");

		max_alignment = 1;
		max_addr = 0;
		prev = NULL;
		for (v = aggr_members; v != NULL; prev = v, v = v->va_next) {
			if (v->va_type->ty_code == TY_BITFIELD) {
				if (typecode != TY_STRUCT)
					diagf(ET_ERROR, v->va_lexinfo,
					      "Bitfield illegal in union");
				addr = set_bitfield_addr(v, prev, max_addr);
			}
			else if (is_incomplete_type(v->va_type)) {
				diagf(ET_ERROR, v->va_lexinfo,
				      "Incomplete type for %s member %s",
				      (typecode == TY_STRUCT) ? "struct"
							      : "union",
							v->va_name);
				ci_show_type(v->va_type, "type");
				continue;
			}
			else {
				if (typecode == TY_STRUCT)
					addr = ci_align_addr_for_type(max_addr,
								      v->va_type);
				else
					addr = 0;
				v->va_addr = addr;
				addr += ci_typesize(v->va_lexinfo, v->va_type);
			}

			alignment = ci_type_alignment(v->va_type);
			if (alignment > max_alignment)
				max_alignment = alignment;
			
			if (addr > max_addr)
				max_addr = addr;
		}
		ae->ae_size = ci_align_addr(max_addr, max_alignment);
		ae->ae_alignment = max_alignment;
		ae->ae_aggr_members = aggr_members;
	}

	return ae->ae_type;
}

static bool
is_incomplete_type(type)
type_t *type;
{
	if (type->ty_code == TY_U_STRUCT || type->ty_code == TY_U_UNION)
		return TRUE;
	if (type->ty_code == DT_ARRAY_OF &&
	    type->ty_dim->di_htype != DT_CONSTANT)
		return TRUE;
	return FALSE;
}

static type_t *
check_bitfield_type(type, lx)
type_t *type;
lexinfo_t *lx;
{
	if (type->ty_code != TY_BITFIELD) {
		diagf(ET_ERROR, lx, "Bitfields must have type int or unsigned");
		return type;
	}

	switch (type->ty_base->ty_code) {
	case TY_INT:
	case TY_UINT:
		break;
	case TY_ULONG:
	case TY_USHORT:
	case TY_UCHAR:
		diagf(ET_WARNING, lx, "Illegal bitfield type %s",
					ci_basetype_name(type->ty_base));
		break;
	case TY_LONG:
	case TY_SHORT:
	case TY_CHAR:
		diagf(ET_WARNING, lx, "Illegal bitfield type %s",
					ci_basetype_name(type->ty_base));
		break;
	default:
		diagf(ET_ERROR, lx, "Illegal bitfield type %s",
					ci_basetype_name(type->ty_base));
		break;
	}
	type->ty_bitfield->bf_type = type->ty_base;
	type->ty_base = NULL;
	return type;
}

static taddr_t
set_bitfield_addr(v, prev, addr)
var_t *v, *prev;
taddr_t addr;
{
	bitfield_t *bf;
	int offset, bits_per_word;

	if (v->va_type->ty_code != TY_BITFIELD)
		ci_panic("sba called on non bitfield");
	bf = v->va_type->ty_bitfield;

	bits_per_word = ci_typesize((lexinfo_t *)NULL, ci_code_to_type(TY_INT)) * 8;

	if (bf->bf_width < 0 || bf->bf_width > bits_per_word) {
		diagf(ET_ERROR, v->va_lexinfo,
				"Illegal bitfield width %d", bf->bf_width);
		return addr;
	}

#if IS_BIG_ENDIAN
	if (prev == NULL || prev->va_type->ty_code != TY_BITFIELD)
		offset = -1;
	else
		offset = prev->va_type->ty_bitfield->bf_offset - bf->bf_width;

	if (bf->bf_width == 0 || offset < 0) {
		v->va_addr = ci_align_addr_for_type(addr, v->va_type);
		bf->bf_offset = bits_per_word - bf->bf_width;
	}
	else {
		v->va_addr = prev->va_addr;
		bf->bf_offset = offset;
	}

	return v->va_addr + bits_per_word / 8 - bf->bf_offset / 8;
#else
	if (prev == NULL || prev->va_type->ty_code != TY_BITFIELD)
		offset = bits_per_word;
	else {
		/* 
		 * Dibyendu : following caused bit fields to have
		 * negative offsets - causing CX to panic().
		 * 8/12/98.
		 *
		 * offset = prev->va_type->ty_bitfield->bf_offset -
	 	 *			prev->va_type->ty_bitfield->bf_width;
		 */
		offset = prev->va_type->ty_bitfield->bf_offset +
					prev->va_type->ty_bitfield->bf_width;
	}

	if (bf->bf_width == 0 || offset + bf->bf_width > bits_per_word) {
		v->va_addr = ci_align_addr_for_type(addr, v->va_type);
		bf->bf_offset = 0;
	}
	else {
		v->va_addr = prev->va_addr;
		bf->bf_offset = offset;
	}

	return v->va_addr + (bf->bf_offset + bf->bf_width + 7) / 8;
#endif /* !IS_BIG_ENDIAN */
}

void
ci_set_formal_params_flag(val)
bool val;
{
	Doing_formal_parameters = val;
}

void
ci_handle_declaration(declaration)
declaration_t *declaration;
{
	class_t default_class;
	var_t *vlist;
	initlist_t *initlist;

	if (declaration->dn_class == CL_TYPEDEF) {
		ci_add_typedefs_to_current_block(declaration);
		return;
	}

	if (Current_block == Outer_ci_block)
		default_class = CL_EXT;
	else
		default_class = Doing_formal_parameters ? CL_ARG : CL_AUTO;

	ci_declarations_to_varlist(declaration, default_class,
							&vlist, &initlist);

	/*  We check function parameters in ci_check_func_decls() so don't'
	 *  check them here.  This is partly because we want the incomplete
	 *  type check to come after the rewrite of `int x[]' to `int *x'
	 *  that happens for formal parameters.
	 */
	if (Doing_formal_parameters)
		ci_complain_about_incomplete_types(vlist, "Variable");

	Current_block->bl_initlist = ci_push_initlist(initlist,
						 Current_block->bl_initlist);
	if (Make_externs_toplevel) {
		var_t *v, *next;

		for (v = vlist; v != NULL; v = next) {
			next = v->va_next;
			if (v->va_class == CL_DECL && default_class != CL_ARG) {
				v->va_next = Outer_ci_block->bl_vars;
				Outer_ci_block->bl_vars = v;
			}
			else {
				v->va_next = Current_block->bl_vars;
				Current_block->bl_vars = v;
			}
		}
	}
	else {
		Current_block->bl_vars = ci_push_vars(vlist, Current_block->bl_vars);
	}
}

/* Dibyendu 26/12/98 - new function added 
 * This is called for type declarations without declaration specifiers.
 * C allows a type declaration for side-effect - such as declaring a 
 * struct, union or enum type. However, the grammer allows non-sensical 
 * constructs as well, such as:
 *     extern;
 *     static struct s { int dummy; };
 *     struct { int dummy; };
 *     typedef int;
 * The purpose of this function is to catch such errors. 
 */
void
ci_check_declaration(declaration)
declaration_t *declaration;
{
	type_t *type;

	if (declaration->dn_class == CL_TYPEDEF) {
		/* This function is called only when there are no
		 * declarators.
		 */
		diagf(ET_ERROR, NULL, "Typedef without a declarator");
		return;
	}

	type = ci_get_dn_basetype(declaration);
	if (type->ty_code != TY_STRUCT 
	 && type->ty_code != TY_UNION 
	 && type->ty_code != TY_ENUM
	 && type->ty_code != TY_U_STRUCT
	 && type->ty_code != TY_U_UNION
	 && type->ty_code != TY_U_ENUM) {
		/* Only struct, union or enum declarations are valid
		 * without a declarator.
		 */
		diagf(ET_ERROR, NULL, 
		"Declaration does not have a declarator");
		return;
	}

	if (type->ty_code == TY_STRUCT 
	 || type->ty_code == TY_UNION
	 || type->ty_code == TY_U_STRUCT
	 || type->ty_code == TY_U_UNION) {
		if (type->ty_aggr_or_enum->ae_tag == NULL) {
			/* A struct or union without a tag name 
			 * or a declarator, cannot be accessed.
			 */
			diagf(ET_ERROR, NULL,
			"Unnamed aggregate type cannot be accessed");
		}
	}

	if (type->ty_code == TY_STRUCT 
	 || type->ty_code == TY_UNION 
	 || type->ty_code == TY_ENUM
	 || type->ty_code == TY_U_STRUCT
	 || type->ty_code == TY_U_UNION
	 || type->ty_code == TY_U_ENUM) {
		if (declaration->dn_class != CL_NOCLASS) {
			diagf(ET_ERROR, NULL, 
			"Aggregate type declaration has incorrect storage class");
		}
	}
}

void
ci_insert_var_declaration(v)
var_t *v;
{
	v->va_next = Current_block->bl_vars;
	Current_block->bl_vars = v;
}

static statement_t *
reverse_statements(st)
statement_t *st;
{
	statement_t *list, *next;

	list = NULL;
	for (; st != NULL; st = next) {
		next = st->st_next;
		st->st_next = list;
		list = st;
	}
	return list;
}

statement_t *
ci_end_compound_statement(statements)
statement_t *statements;
{
	statement_t *st = NEW(statement_t);
	
	st->st_type = STT_COMPOUND;
	st->st_lexinfo = NULL;
	st->st_compound = NEW(compound_stm_t);
	st->st_compound->co_block = Current_block;
	st->st_compound->co_statements = reverse_statements(statements);
	ci_end_block();
	return st;
}

void
ci_add_var_decl_for_func_if_necessary(f)
func_t *f;
{
	namedesc_t *nd;
	var_t *v;
	block_t *bl;

	bl = Outer_ci_block;
	if ((nd = lookup_name_in_varlist(bl->bl_vars, f->fu_name)) == NULL) {
		v = ci_make_var(Parse_apool, f->fu_name, CL_DECL, f->fu_type, 0);
		v->va_lexinfo = f->fu_lexinfo;
		v->va_next = bl->bl_vars;
		bl->bl_vars = v;
	}
}

static namedesc_t *
make_namedesc(v)
var_t *v;
{
	static namedesc_t ndbuf;

	ndbuf.nd_nametype = (v->va_type->ty_code == DT_FUNC_RETURNING)
						? NT_FUNCNAME : NT_VARNAME;
	ndbuf.nd_lexinfo = v->va_lexinfo;
	ndbuf.nd_var = v;
	return &ndbuf;
}

static namedesc_t *
lookup_name_in_varlist(vlist, name)
var_t *vlist;
const char *name;
{
	var_t *v;

	for (v = vlist; v != NULL; v = v->va_next) {
		if (v->va_name != NULL && strcmp(v->va_name, name) == 0)
			return make_namedesc(v);
	}
	return NULL;
}

static aggr_or_enum_def_t *
lookup_enum_member(ae, tag)
aggr_or_enum_def_t *ae;
const char *tag;
{
	enum_member_t *em;

	if (ae->ae_type->ty_code != TY_ENUM)
		return NULL;

	for (em = ae->ae_enum_members; em != NULL; em = em->em_next) {
		if (strcmp(em->em_name, tag) == 0)
			return (aggr_or_enum_def_t *)em;
	}
	
	return NULL;
}

/*  Search the names defined in block block for name name.
 *
 *  We search for name as a object name, a typedef name or an enum constant.
 *
 *  The returned pointer points to a static object, which is overwritten
 *  on each call.
 */
static namedesc_t *
lookup_name_in_block(block, name)
block_t *block;
const char *name;
{
	static namedesc_t ndbuf;
	namedesc_t *nd;
	enum_member_t *em;
	typedef_t *td;

	/*  Variable name?
	 */
	if ((nd = lookup_name_in_varlist(block->bl_vars, name)) != NULL)
		return nd;

	/*  Enum constant?
	 */
	em = (enum_member_t *)ci_apply_to_aelist(block->bl_aggr_or_enum_defs,
						 lookup_enum_member, name);
	if (em != NULL) {
		ndbuf.nd_nametype = NT_ENUM_CONST;
		ndbuf.nd_lexinfo = em->em_lexinfo;
		ndbuf.nd_enum_member = em;
		return &ndbuf;
	}

	/*  Typedef name?
	 */
	if ((td = name_to_typedef(block->bl_typedefs, name)) != NULL) {
		ndbuf.nd_nametype = NT_TYPEDEF_NAME;
		ndbuf.nd_lexinfo = td->td_lexinfo;
		ndbuf.nd_typedef = td;
		return &ndbuf;
	}

	return NULL;
}

namedesc_t *
ci_lookup_name(name)
const char *name;
{
	block_t *bl;
	namedesc_t *nd;
	var_t *v;

	for (bl = Current_block; bl != NULL; bl = bl->bl_parent)
		if ((nd = lookup_name_in_block(bl, name)) != NULL)
			return nd;

	if (Resolve_name_func != NULL && (*Resolve_name_func)(name, &v) == 0)
		return make_namedesc(v);

	return NULL;
}

static typedef_t *
name_to_typedef(tdlist, name)
typedef_t *tdlist;
const char *name;
{
	typedef_t *td, *rtd;

	for (td = tdlist; td != NULL; td = td->td_next) {
		if (td->td_type == NULL) {
			if ((rtd = name_to_typedef(td->td_sublist, name)) != NULL)
				return rtd;
		}
		else if (strcmp(td->td_name, name) == 0)
			return td;
	}

	return NULL;
}

type_t *
ci_lookup_typedef(name)
const char *name;
{
	block_t *bl;
	typedef_t *td;

	for (bl = Current_block; bl != NULL; bl = bl->bl_parent) {
		if ((td = name_to_typedef(bl->bl_typedefs, name)) != NULL)
			return td->td_type;

		/* Dibyendu : 28/12/98.
		 * typedef names are ordinary identifiers. A variable
		 * or function declaration may have hidden the typedef
		 * name.
		 */
		if (lookup_name_in_block(bl, name) != NULL)
			return NULL;
	}

	return NULL;
}

void
ci_start_block(add_to_parent)
bool add_to_parent;
{
	block_t *bl, *lastchild, *child;

	child = ci_make_block(Parse_apool, Current_block);

	/*  We add one here because symbol tables number from one upwards
	 *  rather than from zero.
	 */
	child->bl_start_lnum = Lex_env->le_lnum + 1;

	if (add_to_parent) {
		lastchild = NULL;
		for (bl = Current_block->bl_blocks; bl != NULL; bl = bl->bl_next)
			lastchild = bl;
		if (lastchild != NULL)
			lastchild->bl_next = child;
		else
			Current_block->bl_blocks = child;
	}
	
	Current_block = child;
}

void
ci_end_block()
{
	/*  We add one here because symbol tables number from one upwards
	 *  rather than from zero.
	 */
	Current_block->bl_end_lnum = Lex_env->le_lnum + 1;

	Current_block = Current_block->bl_parent;
}

void
ci_free_parse_id(parse_id)
parse_id_t parse_id;
{
	alloc_free_pool(((parse_res_t *)parse_id)->pr_apool);
}

void
ci_start_parse_tree(resolve_name_func, block, flags)
ci_resolve_name_func_t resolve_name_func;
block_t *block;
unsigned long flags;
{

	Parse_apool = alloc_create_pool();

	Funclist = NULL;
	Current_block = ci_make_block(Parse_apool, block);
	Varlist = NULL;

	Outer_ci_block = Current_block;
	Make_externs_toplevel = (flags & CI_MAKE_EXTERNS_TOPLEVEL) != 0;

	Resolve_name_func = resolve_name_func;
}

static var_t *
push_var_definitions(v, list)
var_t *v, *list;
{
	var_t *next;

	for (; v != NULL; v = next) {
		next = v->va_next;
		if (v->va_class != CL_DECL) {
			v->va_next = list;
			list = v;
		}
	}
	return list;
}

parse_res_t *
ci_end_parse_tree(parse_succeeded)
bool parse_succeeded;
{
	parse_res_t *pr;
	fil_t *fil;
	funclist_t dummy_funclist, *last;
	initlist_t *il;
	func_t *f;
	const char *name;

	if (!parse_succeeded) {
		alloc_free_pool(Parse_apool);
		return NULL;
	}

	if (Current_block != Outer_ci_block)
		ci_panic("block level botch in ept");

	pr = (parse_res_t *)alloc(Parse_apool, sizeof(parse_res_t));
	pr->pr_apool = Parse_apool;
	pr->pr_block = Current_block;
	pr->pr_funcs = Funclist;
	pr->pr_block->bl_vars = push_var_definitions(pr->pr_block->bl_vars,
								   Varlist);

	/*  Build a fil structure and point all the functions' fu_fil
	 *  pointer at it.
	 */

	if (pr->pr_funcs != NULL)
		name = pr->pr_funcs->fu_lexinfo->lx_filename;
	else if (pr->pr_block->bl_vars != NULL)
		name = pr->pr_block->bl_vars->va_lexinfo->lx_filename;
	else
		name = "Unknown name";

	fil = ci_make_fil(Parse_apool, name, (char *)NULL, Current_block,
			  (fil_t *)NULL);

	fil->fi_flags |= FI_DONE_VARS;

	last = &dummy_funclist;
	for (f = pr->pr_funcs; f != NULL; f = f->fu_next) {
		funclist_t *fl;

		f->fu_fil = fil;
		fl = NEW(funclist_t);
		fl->fl_func = f;

		last->fl_next = fl;
		last = fl;
	}
	last->fl_next = NULL;
	fil->fi_funclist = dummy_funclist.fl_next;

	for (il = pr->pr_block->bl_initlist; il != NULL; il = il->il_next)
		il->il_fil = fil;

	return pr;
}

static bool
is_decl(v)
var_t *v;
{
	return v->va_class == CL_DECL ||
	       (v->va_class == CL_STAT && v->va_type->ty_code ==
							DT_FUNC_RETURNING);
}

static void
check_redeclaration(nd, v)
namedesc_t *nd;
var_t *v;
{
	nametype_t var_nametype;

	var_nametype = (v->va_type->ty_code == DT_FUNC_RETURNING) ? NT_FUNCNAME
								  : NT_VARNAME;
	
	if ((nd->nd_nametype == NT_VARNAME || nd->nd_nametype == NT_FUNCNAME) &&
	    (var_nametype == NT_VARNAME || var_nametype == NT_FUNCNAME) &&
	    (is_decl(nd->nd_var) || is_decl(v))) {
		ci_complain_if_types_differ(v->va_name,
				    nd->nd_var->va_type, nd->nd_var->va_lexinfo,
				    v->va_type, v->va_lexinfo);
	}
	else {
		report_redecl(v->va_name,
			      NT_VARNAME, v->va_lexinfo,
			      nd->nd_nametype, nd->nd_lexinfo);
	}
}

void
ci_add_typedefs_to_current_block(declarations)
declaration_t *declarations;
{
	var_t *vlist, *v;
	initlist_t *initlist;

	ci_declarations_to_varlist(declarations, CL_TYPEDEF, &vlist, &initlist);
	ci_complain_about_any_initialisers(initlist, "typedef");

	for (v = vlist; v != NULL; v = v->va_next) {
		typedef_t *td;
		type_t *type;

		/*  We are about to change the ty_typedef field of v->va_type.
		 *  If the type has a tag (i.e. is a struct, union or enum)
		 *  then we want this to happen, because it is necessary to
		 *  make the corrent tie up with structs and unions that are
		 *  typedef'ed before they are defined.  Otherwise we make
		 *  a copy.
		 */
		switch (v->va_type->ty_code) {
		case TY_STRUCT:
		case TY_U_STRUCT:
		case TY_UNION:
		case TY_U_UNION:
		case TY_ENUM:
		case TY_U_ENUM:
			type = v->va_type;
			break;
		default:
			type = ci_make_type(Parse_apool, TY_NOTYPE);
			*type = *v->va_type;
			break;
		}

		td = ci_make_typedef(Parse_apool, v->va_name, type);
		td->td_lexinfo = v->va_lexinfo;
		
		td->td_next = Current_block->bl_typedefs;
		Current_block->bl_typedefs = td;
	}
}
