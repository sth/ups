/* ci_types.h - header file for ci_types.c */

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


/* @(#)ci_types.h	1.4 18/10/92 (UKC) */

declaration_t *ci_make_declaration PROTO((class_t class, type_t *basetype,
					  qualifiers_t qualifiers,
					  declarator_t *declarators));
void report_redecl PROTO((const char *name,
				 nametype_t nametype, lexinfo_t *this_lx,
				 nametype_t prev_nametype, lexinfo_t *prev_lx));
bool ci_types_same PROTO((type_t *t1, type_t *t2));
type_t *ci_resolve_typedefs PROTO((type_t *type));
void ci_complain_if_types_differ PROTO((const char *name,
					type_t *prev_type, lexinfo_t *prev_lexinfo,
					type_t *type, lexinfo_t *lexinfo));
bool ci_complain_about_any_void_types PROTO((var_t *vlist));
bool ci_is_integral PROTO((typecode_t typecode));
bool ci_is_signed_type PROTO((typecode_t typecode));
void ci_show_type PROTO((type_t *type, const char *what));
taddr_t ci_align_addr PROTO((taddr_t addr, int alignment));
taddr_t ci_align_addr_for_type PROTO((taddr_t addr, type_t *type));
int ci_type_alignment PROTO((type_t *type));
optype_t ci_typecode_to_cvt_op PROTO((typecode_t typecode));
const char * ci_typecode_to_name PROTO((typecode_t typecode));
void ci_rewrite_func_params PROTO((funcret_t *fr));

void ci_add_type_specifier PROTO((declaration_t *declaration, type_t *type));
type_t *ci_get_dn_basetype PROTO((declaration_t *declaration));

#if WANT_LDBL
#define IS_ARITHMETIC_TYPE(code) \
	(ci_is_integral(code) || \
	((code) == TY_FLOAT) || ((code) == TY_DOUBLE) || ((code) == TY_LONGDOUBLE))
#else
#define IS_ARITHMETIC_TYPE(code) \
	(ci_is_integral(code) || \
	((code) == TY_FLOAT) || ((code) == TY_DOUBLE))
#endif

#define IS_ARITHMETIC_OR_PTR_TYPE(code) \
				(IS_ARITHMETIC_TYPE(code) || ((code) == DT_PTR_TO))

