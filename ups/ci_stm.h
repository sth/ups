/* ci_stm.h - header file for ci_stm.c */

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


/* @(#)ci_stm.h	1.2 4/7/91 (UKC) */

statement_t *ci_make_labeled_statement PROTO((identifier_t *id,
						     statement_t *statement));

statement_t *ci_make_goto_statement PROTO((identifier_t *id));

statement_t *ci_make_case_labeled_statement PROTO((bool in_switch,
						   expr_t *label_expr,
						   statement_t *statement));

statement_t *ci_make_expression_statement PROTO((expr_t *expr));

statement_t *ci_make_if_statement PROTO((expr_t *expr,
					 statement_t *ifpart,
					 statement_t *elsepart));

statement_t *ci_make_switch_statement PROTO((expr_t *expr, statement_t *statement));

statement_t *ci_make_while_statement PROTO((statement_type_t statement_type,
						   expr_t *expr,
						   statement_t *statement));

statement_t *ci_make_for_statement PROTO((expr_t *init,
					  expr_t *test,
					  expr_t *reinit,
					  statement_t *statement,
					  lexinfo_t *lx));

statement_t *ci_make_continue_statement PROTO((bool in_loop, lexinfo_t *lx));

statement_t *ci_make_break_statement PROTO((bool in_loop_or_switch, lexinfo_t *lx));

statement_t *ci_make_return_statement PROTO((expr_t *expr, lexinfo_t *lx));
