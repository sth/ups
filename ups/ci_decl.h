/* ci_decl.h - header file for ci_decl.c */

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


/* @(#)ci_decl.h	1.6 12/22/93 (UKC) */

type_t *ci_lookup_typedef PROTO((const char *name));

type_t *ci_build_aggr_or_enum_def PROTO((typecode_t typecode,
					    identifier_t *id,
					    ae_is_complete_t new_is_complete,
					    declaration_t *aggr_decls,
					    enum_member_t *enum_members,
					    bool is_definition));

statement_t *ci_end_compound_statement PROTO((statement_t *statements));

type_t *ci_make_funcret_type PROTO((type_t *type,
				    params_type_t params_type, 
				    declaration_t *declarations,
				    identifier_list_t *idlist));

namedesc_t *ci_lookup_name PROTO((const char *name));
void ci_add_typedefs_to_current_block PROTO((declaration_t *declarations));

void ci_insert_var_declaration PROTO((var_t *v));

void ci_start_parse_tree PROTO((ci_resolve_name_func_t resolve_name_func,
				block_t *block, unsigned long flags));
parse_res_t *ci_end_parse_tree PROTO((bool parse_succeeded));

void ci_start_block PROTO((bool add_to_parent));
void ci_end_block PROTO((void));

void ci_add_var_decl_for_func_if_necessary PROTO((func_t *f));

void ci_declarations_to_varlist PROTO((declaration_t *declarations,
				       class_t default_class,
				       var_t **p_varlist, initlist_t **p_initlist));

void ci_set_formal_params_flag PROTO((bool val));
void ci_handle_declaration PROTO((declaration_t *declarations));
void ci_check_declaration PROTO((declaration_t *declarations));

void ci_complain_about_any_initialisers PROTO((initlist_t *initlist,
					       const char *what));
void ci_complain_about_incomplete_types PROTO((var_t *vlist, const char *what));
void ci_set_storage_class PROTO((declaration_t *dn, class_t class));
