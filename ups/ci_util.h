/* ci_util.h - utility routines for the C interpreter */

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


/* @(#)ci_util.h	1.9 16 Apr 1994 (UKC) */

#ifdef CI_PARSE_H_INCLUDED
const char *ci_lexinfo_to_string PROTO((lexinfo_t *lx, lexinfo_t *cur_lx));
type_t *ci_make_pointer PROTO((type_t *base, qualifiers_t qualifiers));
declarator_t *ci_make_declarator PROTO((type_t *type));
initexpr_t *ci_make_initexpr PROTO((bool is_list,
					  expr_t *expr, initexpr_t *list));
type_t *ci_make_array_type PROTO((type_t *type, expr_t *dim_expr));
enum_member_t *ci_build_enum_member PROTO((identifier_t *id, expr_t *expr));
type_t *ci_make_expr_bitfield_type PROTO((type_t *base, expr_t *expr));

initlist_t *ci_push_initlist PROTO((initlist_t *newlist, initlist_t *list));
expr_list_t *ci_reverse_expr_list PROTO((expr_list_t *el));
type_t *ci_push_types PROTO((type_t *typelist, type_t *types));

#endif /* CI_PARSE_H_INCLUDED */

typedef enum { ET_ERROR, ET_WARNING, ET_MORE, ET_IGNORE } errtype_t;

void diagf PROTO((errtype_t errtype, lexinfo_t *lx, const char *fmt, ...))
							    FORMF_ARGS(3, 4);

typedef void (*diag_handler_func_t) PROTO((char *arg, errtype_t errtype,
					   lexinfo_t *lx, const char *mesg));
void ci_set_diag_handler PROTO((diag_handler_func_t func, char *arg));

void ci_panic PROTO((const char *s));


identifier_list_t *ci_reverse_idlist PROTO((identifier_list_t *idlist));

extern alloc_pool_t *Parse_apool;

#define NEW(type)	((type *)alloc(Parse_apool, sizeof(type)))

/* RCB: Hooks to handle variable size of bool */
void ci_set_sizeof_bool PROTO((int));
typecode_t ci_effective_typecode PROTO((typecode_t));
