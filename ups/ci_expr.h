/* ci_expr.h - header file for ci_expr.c */

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


/* @(#)ci_expr.h	1.6 26/7/92 (UKC) */

#if WANT_LL
expr_t *ci_make_integer_constant_expr PROTO((expr_type_t exprtype,
					     lexinfo_t *lx, long_long_t ival,
					     bool is_unsigned,
                                             bool is_longlong));
#else
expr_t *ci_make_integer_constant_expr PROTO((expr_type_t exprtype,
					     lexinfo_t *lx, long ival,
					     bool is_unsigned,
                                             bool is_longlong));
#endif
expr_t *ci_make_floating_constant_expr PROTO((lexinfo_t *lx, double dval));
expr_t *ci_make_string_constant_expr PROTO((lexinfo_t *lx, string_const_t *sc));

expr_t *ci_make_identifier_expr PROTO((identifier_t *id));

expr_t *ci_make_multi_arrow_expr PROTO((expr_t *aggr_expr,
					expr_t *index_expr,
				        identifier_t *member));
expr_t *ci_make_dot_expr PROTO((expr_t *aggr_expr,
				identifier_t *member,
				const char *opname));
expr_t *ci_make_comma_expr PROTO((expr_t *left, expr_t *right));

expr_t *ci_make_deref_expr PROTO((expr_t *expr));
expr_t *ci_make_address_of_expr PROTO((expr_t *expr));
expr_t *ci_make_unary_plus_expr PROTO((expr_t *expr));
expr_t *ci_make_unary_minus_expr PROTO((expr_t *expr));
expr_t *ci_make_bitwise_not_expr PROTO((expr_t *expr));
expr_t *ci_make_logical_not_expr PROTO((expr_t *expr));
expr_t *ci_make_inc_or_dec_expr PROTO((optype_t op, expr_t *expr,
							const char *opname));

expr_t *ci_make_mul_or_div_expr PROTO((optype_t op, const char *opname,
						expr_t *left, expr_t *right));
expr_t *ci_make_mod_expr PROTO((optype_t op, const char *opname,
						      expr_t *left, expr_t *right));
expr_t *ci_make_add_or_subtract_expr PROTO((optype_t op, const char *opname,
						      expr_t *left, expr_t *right));
expr_t *ci_make_shift_expr PROTO((optype_t op, const char *opname,
						      expr_t *left, expr_t *right));
expr_t *ci_make_comparison_expr PROTO((optype_t op, const char *opname,
						      expr_t *left, expr_t *right));
expr_t *ci_make_bitwise_expr PROTO((optype_t op, const char *opname,
						      expr_t *left, expr_t *right));
expr_t *ci_make_logical_expr PROTO((optype_t op, const char *opname,
						      expr_t *left, expr_t *right));
expr_t *ci_make_assignment_expr PROTO((optype_t op, expr_t *left, expr_t *right));

expr_t *ci_make_conditional_expression PROTO((expr_t *cond,
					      expr_t *if_true,
					      expr_t *if_false));

expr_t *ci_make_cast_expr PROTO((type_t *type, expr_t *expr));
expr_t *ci_make_sizeof_expr PROTO((expr_t *expr, type_t *type));
expr_t *ci_make_func_call_expr PROTO((expr_t *func, expr_list_t *args));

void ci_do_old_style_argument_promotions PROTO((expr_t *expr));
int ci_do_assignment_conversions PROTO((type_t *type, expr_t *expr));
void ci_do_integral_promotions PROTO((expr_t *expr));
void ci_do_pointer_generation PROTO((expr_t *expr));
type_t *ci_push_conversion PROTO((expr_t *expr, typecode_t typecode,
						expr_type_t exprtype));
