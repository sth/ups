/* ci_func.h - header file for ci_func.c */

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


/* @(#)ci_func.h	1.2 7/4/91 (UKC) */

goto_label_t *ci_name_to_label PROTO((identifier_t *id, bool is_definition));
func_t *ci_start_func PROTO((declaration_t *declaration));
void ci_check_func_decls PROTO((func_t *f));
expr_list_t *ci_check_func_call PROTO((expr_t *func_expr,
				       const char *funcname,
				       expr_list_t *arglist,
				       bool is_implicit_declaration));
void ci_end_func PROTO((statement_t *statement));
void ci_check_return_expr PROTO((expr_t *expr));
