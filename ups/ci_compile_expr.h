/* ci_compile_expr.h - header file for ci_compile_expr.c */

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


/* @(#)ci_compile_expr.h	1.4 18/10/92 (UKC) */

typedef enum { EC_ADDRESS, EC_EFFECT, EC_VALUE } expr_context_t;

typedef enum { AT_CI_ADDR, AT_PROC_ADDR } addrtype_t;

addrtype_t ci_compile_expression PROTO((text_t *tx, expr_t *expr,
						expr_context_t expr_context));
void ci_code_assign PROTO((text_t *tx, type_t *type,
					bool want_value, addrtype_t addrtype));

addrtype_t ci_compile_var_reference PROTO((text_t *tx, var_t *v,
					lexinfo_t *lx, expr_context_t context));

void ci_do_static_initialisations PROTO((text_t *tx, initlist_t *initlist,
					 char *data, char *extdata));
