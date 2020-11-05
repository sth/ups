/* ci_constexpr.c - routines to check for and evaluate constant expressions */

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


/* @(#)ci_constexpr.c	1.16 16 Apr 1994 (UKC) */
char ups_ci_constexpr_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <stdlib.h>
#include <ctype.h>

#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "ci_parse.h"
#include "ci_showexpr.h"
#include "ci_constexpr.h"
#include "ci_types.h"
#include "ci_util.h"

/*  In this code we make the following assumptions:
 *
 *	int has the same size and representation as long
 *
 *	unsigned has the same size and representation as unsigned long
 *
 *	no distinct "long double" type  (see note below)
 *
 *	pointers can be cast to int and back without loss of information
 *
 *  It wouldn't take much work to avoid making these assumptions.
 *  You would have to add more members to the constval_t structure,
 *  and add code in various places here to handle the extra cases.
 *  Some of the stuff is n**2 in the number of constval_t members though.
 */
/*  (IDE) I made a start on support for 'long double' but it is not complete
 *  and does not work.  I suspect my approach may be wrong.
 */
static bool eval_unary_const_expr PROTO((unary_expr_t *ue, const char *what,
							      constval_t *cv));
static bool eval_binary_const_expr PROTO((binary_expr_t *be, const char *what,
							      constval_t *cv));
static bool eval_scale_const_expr PROTO((scale_expr_t *sc, const char *what,
							      constval_t *cv));
static bool eval_conditional_const_expr PROTO((cond_expr_t *co, const char *what,
							      constval_t *cv));

static bool eval_const_expr PROTO((expr_t *expr, const char *what, constval_t *cv));
static typecode_t operand_typecode PROTO((expr_t *expr, bool allow_short_types));
static void do_unary_conversion PROTO((expr_t *expr, constval_t *cv));

int
ci_evaluate_constant_expression(expr, what, must_be_integral, cv)
expr_t *expr;
const char *what;
bool must_be_integral;
constval_t *cv;
{
	if (expr->ex_type == NULL)
		return FALSE;
	if (must_be_integral && expr->ex_type->ty_code != TY_ENUM &&
				      !ci_is_integral(expr->ex_type->ty_code)) {
		diagf(ET_ERROR, expr->ex_lexinfo, "%c%s must be integral",
						toupper(*what), what + 1);
		ci_show_expr_and_type(expr, expr->ex_type);
		return FALSE;
	}
	return eval_const_expr(expr, what, cv);
}

static bool
eval_unary_const_expr(ue, what, cv)
unary_expr_t *ue;
const char *what;
constval_t *cv;
{
	const char *badop;
	constval_t val;
	bool ok;
	typecode_t ot;

	ok = eval_const_expr(ue->ue_expr, what, &val);
	ot = operand_typecode(ue->ue_expr, FALSE);

	badop = NULL;
	switch(ue->ue_op) {

	case OP_PREINC:		badop = "++";	break;
	case OP_PREDEC:		badop = "--";	break;
	case OP_POSTINC: 	badop = "++";	break;
	case OP_POSTDEC: 	badop = "--";	break;
	case OP_ADDRESS_OF: 	badop = "&";	break;
	case OP_DEREF:		badop = "*";	break;

	case OP_LOGICAL_NOT:
		switch(ot) {
			case TY_INT:	cv->cv_int = !val.cv_int; break;
			case TY_UINT:	cv->cv_int = !val.cv_unsigned; break;
			default:	panic("bad unary ot"); break;
		}
		break;
	case OP_BITWISE_NOT:
		switch(ot) {
			case TY_INT:	cv->cv_int = ~val.cv_int; break;
			case TY_UINT:	cv->cv_unsigned = ~val.cv_unsigned; break;
			default:	panic("bad unary ot"); break;
		}
		break;
	case OP_UNARY_PLUS:
		*cv = val;
		break;

	case OP_UNARY_MINUS:
		switch(ot) {
#if WANT_LDBL
			case TY_LONGDOUBLE:	cv->cv_long_double = -val.cv_long_double; break;
#endif
#if WANT_LL
			case TY_LONGLONG:	cv->cv_long_long = -val.cv_long_long; break;
			case TY_ULONGLONG:	cv->cv_ulong_long = -val.cv_ulong_long; break;
#endif
			case TY_DOUBLE:	cv->cv_double = -val.cv_double; break;
			case TY_FLOAT:	cv->cv_float = -val.cv_float; break;
			case TY_INT:	cv->cv_int = -val.cv_int; break;
			case TY_UINT:	cv->cv_unsigned = -val.cv_unsigned; break;
			default:	panic("bad unary ot"); break;
		}
		break;

	default:
		ci_panic("bad op in euce");
		break;
	}

	if (badop != NULL)
		diagf(ET_ERROR, ue->ue_expr->ex_lexinfo,
		      "Illegal use of %s operator in %s", badop, what);

	return ok && badop == NULL;
}

static bool
eval_scale_const_expr(sc, what, cv)
scale_expr_t *sc;
const char *what;
constval_t *cv;
{
	constval_t val;

	if (!eval_const_expr(sc->sc_expr, what, &val))
		return FALSE;

	switch (sc->sc_op) {
	case OP_MUL:
		cv->cv_int = val.cv_int * sc->sc_factor;
		break;
	case OP_DIV:
		if (sc->sc_factor == 0 || val.cv_int % sc->sc_factor != 0)
			ci_panic("scale botch in esce");
		cv->cv_int = val.cv_int / sc->sc_factor;
		break;
	default:
		ci_panic("bad op in esce");
	}

	return TRUE;
}

static typecode_t
operand_typecode(expr, allow_short_types)
expr_t *expr;
bool allow_short_types;
{
	switch (ci_effective_typecode(expr->ex_type->ty_code)) {
	case TY_CHAR:
	case TY_SHORT:
		if (allow_short_types)
			return TY_INT;
		break;
	case TY_UCHAR:
	case TY_USHORT:
		if (allow_short_types)
			return TY_UINT;
		break;
	case TY_LONG:
	case TY_INT:
	case TY_ENUM:
	case TY_U_ENUM:
	case TY_BOOLEAN:
		return TY_INT;
	case TY_ULONG:
	case TY_UINT:
	case DT_PTR_TO:
		return TY_UINT;
	case TY_FLOAT:
		return TY_FLOAT;
	case TY_DOUBLE:
		return TY_DOUBLE;
#if WANT_LL
	case TY_LONGLONG:
		return TY_LONGLONG;
	case TY_ULONGLONG:
		return TY_ULONGLONG;
#endif
#if WANT_LDBL
	case TY_LONGDOUBLE:
		return TY_LONGDOUBLE;
#endif
	default:
		break;
	}

	panic("type botch in ot");
	return TY_NOTYPE;	/* to satisfy gcc */

}

#if WANT_LDBL && WANT_LL
#define RELOP(ot,val,l,r,op) \
	switch(ot) { \
	case TY_LONGDOUBLE:	val.cv_int = l.cv_long_double op r.cv_long_double; break; \
	case TY_LONGLONG:	val.cv_int = l.cv_long_long op r.cv_long_long; break; \
	case TY_ULONGLONG:	val.cv_int = l.cv_ulong_long op r.cv_ulong_long; break; \
	case TY_DOUBLE:	val.cv_int = l.cv_double op r.cv_double; break; \
	case TY_FLOAT:	val.cv_int = l.cv_float op r.cv_float; break; \
	case TY_INT:	val.cv_int = l.cv_int op r.cv_int; break; \
	case TY_UINT:	val.cv_int = l.cv_unsigned op r.cv_unsigned; break; \
	default:	panic("bad relop"); break; \
	}
#elif WANT_LL
#define RELOP(ot,val,l,r,op) \
	switch(ot) { \
	case TY_LONGLONG:	val.cv_int = l.cv_long_long op r.cv_long_long; break; \
	case TY_ULONGLONG:	val.cv_int = l.cv_ulong_long op r.cv_ulong_long; break; \
	case TY_DOUBLE:	val.cv_int = l.cv_double op r.cv_double; break; \
	case TY_FLOAT:	val.cv_int = l.cv_float op r.cv_float; break; \
	case TY_INT:	val.cv_int = l.cv_int op r.cv_int; break; \
	case TY_UINT:	val.cv_int = l.cv_unsigned op r.cv_unsigned; break; \
	default:	panic("bad relop"); break; \
	}
#elif WANT_LDBL
#define RELOP(ot,val,l,r,op) \
	switch(ot) { \
	case TY_LONGDOUBLE:	val.cv_int = l.cv_long_double op r.cv_long_double; break; \
	case TY_DOUBLE:	val.cv_int = l.cv_double op r.cv_double; break; \
	case TY_FLOAT:	val.cv_int = l.cv_float op r.cv_float; break; \
	case TY_INT:	val.cv_int = l.cv_int op r.cv_int; break; \
	case TY_UINT:	val.cv_int = l.cv_unsigned op r.cv_unsigned; break; \
	default:	panic("bad relop"); break; \
	}
#else
#define RELOP(ot,val,l,r,op) \
	switch(ot) { \
	case TY_DOUBLE:	val.cv_int = l.cv_double op r.cv_double; break; \
	case TY_FLOAT:	val.cv_int = l.cv_float op r.cv_float; break; \
	case TY_INT:	val.cv_int = l.cv_int op r.cv_int; break; \
	case TY_UINT:	val.cv_int = l.cv_unsigned op r.cv_unsigned; break; \
	default:	panic("bad relop"); break; \
	}
#endif

#define INTOP(ot,val,l,r,op) \
	switch(ot) { \
	case TY_INT:	val.cv_int = l.cv_int op r.cv_int; break; \
	case TY_UINT:	val.cv_unsigned = l.cv_unsigned op r.cv_unsigned; break; \
	default:	panic("bad binop"); break; \
	}

#if WANT_LL && WANT_LDBL
#define BINOP(ot,val,l,r,op) \
	switch(ot) { \
	case TY_LONGDOUBLE:	val.cv_long_double = l.cv_long_double op r.cv_long_double; break; \
	case TY_LONGLONG:	val.cv_long_long = l.cv_long_long op r.cv_long_long; break; \
	case TY_ULONGLONG:	val.cv_ulong_long = l.cv_ulong_long op r.cv_ulong_long; break; \
	case TY_DOUBLE:	val.cv_double = l.cv_double op r.cv_double; break; \
	case TY_FLOAT:	val.cv_float = l.cv_float op r.cv_float; break; \
	case TY_INT:	val.cv_int = l.cv_int op r.cv_int; break; \
	case TY_UINT:	val.cv_unsigned = l.cv_unsigned op r.cv_unsigned; break; \
	default:	panic("bad binop"); break; \
	}
#elif WANT_LL
#define BINOP(ot,val,l,r,op) \
	switch(ot) { \
	case TY_LONGLONG:	val.cv_long_long = l.cv_long_long op r.cv_long_long; break; \
	case TY_ULONGLONG:	val.cv_ulong_long = l.cv_ulong_long op r.cv_ulong_long; break; \
	case TY_DOUBLE:	val.cv_double = l.cv_double op r.cv_double; break; \
	case TY_FLOAT:	val.cv_float = l.cv_float op r.cv_float; break; \
	case TY_INT:	val.cv_int = l.cv_int op r.cv_int; break; \
	case TY_UINT:	val.cv_unsigned = l.cv_unsigned op r.cv_unsigned; break; \
	default:	panic("bad binop"); break; \
	}
#elif WANT_LDBL
#define BINOP(ot,val,l,r,op) \
	switch(ot) { \
	case TY_LONGDOUBLE:	val.cv_long_double = l.cv_long_double op r.cv_long_double; break; \
	case TY_DOUBLE:	val.cv_double = l.cv_double op r.cv_double; break; \
	case TY_FLOAT:	val.cv_float = l.cv_float op r.cv_float; break; \
	case TY_INT:	val.cv_int = l.cv_int op r.cv_int; break; \
	case TY_UINT:	val.cv_unsigned = l.cv_unsigned op r.cv_unsigned; break; \
	default:	panic("bad binop"); break; \
	}
#else
#define BINOP(ot,val,l,r,op) \
	switch(ot) { \
	case TY_DOUBLE:	val.cv_double = l.cv_double op r.cv_double; break; \
	case TY_FLOAT:	val.cv_float = l.cv_float op r.cv_float; break; \
	case TY_INT:	val.cv_int = l.cv_int op r.cv_int; break; \
	case TY_UINT:	val.cv_unsigned = l.cv_unsigned op r.cv_unsigned; break; \
	default:	panic("bad binop"); break; \
	}
#endif

static bool
eval_binary_const_expr(be, what, cv)
binary_expr_t *be;
const char *what;
constval_t *cv;
{
	const char *badop;
	constval_t val, l, r;
	bool ok, left_ok, right_ok, is_zero;
	typecode_t ot;

	left_ok = eval_const_expr(be->be_left, what, &l);
	right_ok = eval_const_expr(be->be_right, what, &r);
	ok = TRUE;
	badop = NULL;

	ot = operand_typecode(be->be_left, FALSE);
	if (left_ok && right_ok &&
			    be->be_op != OP_LSHIFT && be->be_op != OP_RSHIFT) {
		if (operand_typecode(be->be_right, FALSE) != ot)
			panic("ot botch in ebc");
	}

	switch (be->be_op) {
	case OP_ASSIGN:			badop = "=";		break;
	case OP_MUL_ASSIGN:		badop = "*=";		break;
	case OP_DIV_ASSIGN:		badop = "/=";		break;
	case OP_MOD_ASSIGN:		badop = "%=";		break;
	case OP_PLUS_ASSIGN:		badop = "+=";		break;
	case OP_MINUS_ASSIGN:		badop = "-=";		break;
	case OP_LSHIFT_ASSIGN:		badop = "<<=";		break;
	case OP_RSHIFT_ASSIGN:		badop = ">>=";		break;
	case OP_BITWISE_AND_ASSIGN:	badop = "&=";		break;
	case OP_BITWISE_XOR_ASSIGN:	badop = "^=";		break;
	case OP_BITWISE_OR_ASSIGN:	badop = "|=";		break;
	case OP_COMMA:			badop = ",";		break;

	case OP_BITWISE_OR:		INTOP(ot,val,l,r, |  );		break;
	case OP_BITWISE_XOR:		INTOP(ot,val,l,r, ^  );		break;
	case OP_BITWISE_AND:		INTOP(ot,val,l,r, &  );		break;
	case OP_LSHIFT:			INTOP(ot,val,l,r, << );		break;
	case OP_RSHIFT:			INTOP(ot,val,l,r, >> );		break;

	case OP_LOGICAL_OR:		RELOP(ot,val,l,r, || );	break;
	case OP_LOGICAL_AND:		RELOP(ot,val,l,r, && );		break;
	case OP_IS_EQUAL:		RELOP(ot,val,l,r, == );		break;
	case OP_NOT_EQUAL:		RELOP(ot,val,l,r, != );		break;
	case OP_LESS:			RELOP(ot,val,l,r, <  );		break;
	case OP_GREATER:		RELOP(ot,val,l,r, >  );		break;
	case OP_LESS_OR_EQUAL:		RELOP(ot,val,l,r, <= );		break;
	case OP_GREATER_OR_EQUAL:	RELOP(ot,val,l,r, >= );		break;

	case OP_PLUS:			BINOP(ot,val,l,r, +  );		break;
	case OP_MINUS:			BINOP(ot,val,l,r, -  );		break;
	case OP_MUL:			BINOP(ot,val,l,r, *  );		break;

	case OP_DIV:
	case OP_MOD:
		switch(ot) {
#if WANT_LDBL
		case TY_LONGDOUBLE:
			is_zero = r.cv_long_double == 0.0;
			break;
#endif
		case TY_DOUBLE:
			is_zero = r.cv_double == 0.0;
			break;
		case TY_FLOAT:
			is_zero = r.cv_float == 0.0;
			break;
#if WANT_LL
		case TY_LONGLONG:
			is_zero = r.cv_long_long == 0.0;
			break;
		case TY_ULONGLONG:
			is_zero = r.cv_ulong_long == 0.0;
			break;
#endif
		case TY_INT:
			is_zero = r.cv_int == 0;
			break;
		case TY_UINT:
			is_zero = r.cv_unsigned == 0;
			break;
		default:
			panic("bad ot");
			is_zero = FALSE;	/* to satisfy gcc */
			break;
		}

		if (is_zero && right_ok) {
			diagf(ET_ERROR, be->be_right->ex_lexinfo,
						"Division by zero in %s", what);
			ok = FALSE;
			break;
		}
		if (be->be_op == OP_MOD) {
			INTOP(ot,val,l,r, % );
		}
		else {
			BINOP(ot,val,l,r, / );
		}
		break;
	default:
		ci_panic("bad op in ebce");
	}

	if (badop != NULL)
		diagf(ET_ERROR, be->be_left->ex_lexinfo,
			"Illegal use of %s operator in %s", badop, what);

	if (ok && left_ok && right_ok && badop == NULL) {
		*cv = val;
		return TRUE;
	}
	else {
		return FALSE;
	}
}

static bool
eval_conditional_const_expr(co, what, cv)
cond_expr_t *co;
const char *what;
constval_t *cv;
{
	bool cond_ok, left_ok, right_ok;
	constval_t condval, leftval, rightval;

	cond_ok = eval_const_expr(co->co_cond, what, &condval);
	left_ok = eval_const_expr(co->co_if_true, what, &leftval);
	right_ok = eval_const_expr(co->co_if_false, what, &rightval);

	*cv = condval.cv_int ? leftval : rightval;
	return cond_ok && left_ok && right_ok;
}

static void
do_unary_conversion(expr, cv)
expr_t *expr;
constval_t *cv;
{
	constval_t newval;
	typecode_t oldot, newot;

	oldot = operand_typecode(expr->ex_unary_expr->ue_expr, TRUE);
	newot = operand_typecode(expr, TRUE);

	/*  We do this in two stages rather that saying cv->cv_old = cv->cv_new
	 *  because it seems that assigning one member of a union to another
	 *  it not guaranteed to work.
	 */

	switch (ci_effective_typecode(oldot)) {
#if WANT_LDBL
	case TY_LONGDOUBLE:
		switch (newot) {
		case TY_LONGDOUBLE: newval.cv_long_double = cv->cv_long_double; break;
		case TY_DOUBLE: newval.cv_double = cv->cv_long_double; break;
		case TY_FLOAT: newval.cv_float = cv->cv_long_double; break;
		case TY_INT: newval.cv_int = cv->cv_long_double; break;
		case TY_UINT: newval.cv_unsigned = cv->cv_long_double; break;
		default: panic("duc botch"); break;
		}
		break;
#endif
	case TY_DOUBLE:
		switch (newot) {
#if WANT_LDBL
		case TY_LONGDOUBLE: newval.cv_long_double = cv->cv_double; break;
#endif
		case TY_DOUBLE: newval.cv_double = cv->cv_double; break;
		case TY_FLOAT: newval.cv_float = cv->cv_double; break;
		case TY_INT: newval.cv_int = cv->cv_double; break;
		case TY_UINT: newval.cv_unsigned = cv->cv_double; break;
		default: panic("duc botch"); break;
		}
		break;
	case TY_FLOAT:
		switch (newot) {
#if WANT_LDBL
		case TY_LONGDOUBLE: newval.cv_long_double = cv->cv_float; break;
#endif
		case TY_DOUBLE: newval.cv_double = cv->cv_float; break;
		case TY_FLOAT: newval.cv_float = cv->cv_float; break;
		case TY_INT: newval.cv_int = cv->cv_float; break;
		case TY_UINT: newval.cv_unsigned = cv->cv_float; break;
		default: panic("duc botch"); break;
		}
		break;
	case TY_BOOLEAN:
	case TY_INT:
		switch (newot) {
#if WANT_LDBL
		case TY_LONGDOUBLE: newval.cv_long_double = cv->cv_int; break;
#endif
		case TY_DOUBLE: newval.cv_double = cv->cv_int; break;
		case TY_FLOAT: newval.cv_float = cv->cv_int; break;
#if WANT_LL
		case TY_LONGLONG: newval.cv_long_long = cv->cv_int; break;
		case TY_ULONGLONG: newval.cv_ulong_long = cv->cv_int; break;
#endif
		case TY_INT: newval.cv_int = cv->cv_int; break;
		case TY_UINT: newval.cv_unsigned = cv->cv_int; break;
		default: panic("duc botch"); break;
		}
		break;
	case TY_UINT:
		switch (newot) {
#if WANT_LDBL
		case TY_LONGDOUBLE: newval.cv_long_double = cv->cv_unsigned; break;
#endif
		case TY_DOUBLE: newval.cv_double = cv->cv_unsigned; break;
		case TY_FLOAT: newval.cv_float = cv->cv_unsigned; break;
#if WANT_LL
		case TY_LONGLONG: newval.cv_long_long = cv->cv_unsigned; break;
		case TY_ULONGLONG: newval.cv_ulong_long = cv->cv_unsigned; break;
#endif
		case TY_INT: newval.cv_int = cv->cv_unsigned; break;
		case TY_UINT: newval.cv_unsigned = cv->cv_unsigned; break;
		default: panic("duc botch"); break;
		}
		break;
#if WANT_LL
	case TY_LONGLONG:
		switch (newot) {
#if WANT_LDBL
		case TY_LONGDOUBLE: newval.cv_long_double = cv->cv_long_long; break;
#endif
		case TY_DOUBLE: newval.cv_double = cv->cv_long_long; break;
		case TY_FLOAT: newval.cv_float = cv->cv_long_long; break;
		case TY_LONGLONG: newval.cv_long_long = cv->cv_long_long; break;
		case TY_ULONGLONG: newval.cv_ulong_long = cv->cv_long_long; break;
		case TY_INT: newval.cv_int = cv->cv_long_long; break;
		case TY_UINT: newval.cv_unsigned = cv->cv_long_long; break;
		default: panic("duc botch"); break;
		}
		break;
	case TY_ULONGLONG:
		switch (newot) {
#if WANT_LDBL
		case TY_LONGDOUBLE: newval.cv_long_double = cv->cv_ulong_long; break;
#endif
		case TY_DOUBLE: newval.cv_double = cv->cv_ulong_long; break;
		case TY_FLOAT: newval.cv_float = cv->cv_ulong_long; break;
		case TY_LONGLONG: newval.cv_long_long = cv->cv_ulong_long; break;
		case TY_ULONGLONG: newval.cv_ulong_long = cv->cv_ulong_long; break;
		case TY_INT: newval.cv_int = cv->cv_ulong_long; break;
		case TY_UINT: newval.cv_unsigned = cv->cv_ulong_long; break;
		default: panic("duc botch"); break;
		}
		break;
#endif
	default:
		panic("oldot botch in duc"); break;
	}

	switch (expr->ex_type->ty_code) {
	case TY_CHAR:
		cv->cv_int = (char)newval.cv_int;
		break;
	case TY_SHORT:
		cv->cv_int = (short)newval.cv_int;
		break;
	case TY_UCHAR:
		cv->cv_int = (unsigned char)newval.cv_int;
		break;
	case TY_USHORT:
		cv->cv_int = (unsigned short)newval.cv_int;
		break;
	default:
		*cv = newval;
	}
}

static bool
eval_const_expr(expr, what, cv)
expr_t *expr;
const char *what;
constval_t *cv;
{
	char *tmp;
	bool ok;

	switch(expr->ex_exprtype) {
	case ET_FUNCNAME:
		diagf(ET_ERROR, expr->ex_lexinfo,
		      "Reference to address of function %s in %s",
						expr->ex_var->va_name, what);
		ok = FALSE;
		break;
	case ET_VAR:
		diagf(ET_ERROR, expr->ex_lexinfo,
		      "Reference to variable `%s' in %s",
						expr->ex_var->va_name, what);
		ok = FALSE;
		break;
	case ET_UNDEF_VAR:
		diagf(ET_ERROR, expr->ex_lexinfo,
		      "Reference to undefined variable %s in %s",
						expr->ex_var->va_name, what);
		ok = FALSE;
		break;
	case ET_FUNC_CALL:
		tmp = ci_expr_to_english(expr->ex_func_call_expr->fce_func);
		diagf(ET_ERROR, expr->ex_lexinfo,
				"Call of function %s in %s", tmp, what);
		free(tmp);
		ok = FALSE;
		break;
	case ET_DOT:
		diagf(ET_ERROR, expr->ex_lexinfo,
				"Structure member operator (.) in %s", what);
		ok = FALSE;
		break;
	case ET_STRING_CONST:
		diagf(ET_ERROR, expr->ex_lexinfo,
						"String constant in %s", what);
		ok = FALSE;
		break;
	case ET_SIZEOF:
		cv->cv_unsigned = expr->ex_sizeof_expr->sz_size;
		ok = TRUE;
		break;
	case ET_ENUM_CONST:
		cv->cv_int = expr->ex_enum_member->em_val;
		ok = TRUE;
		break;
	case ET_CHAR_CONST:
	case ET_INT_CONST:
#if WANT_LL
		if (expr->ex_type->ty_code == TY_LONGLONG ||
		    expr->ex_type->ty_code == TY_ULONGLONG) {
			cv->cv_long_long = expr->ex_integer_constant_val;
		}
		else
#endif
			cv->cv_int = expr->ex_integer_constant_val;
		ok = TRUE;
		break;
	case ET_FLOATING_CONST:
		cv->cv_double = expr->ex_floating_constant_val;
		ok = TRUE;
		break;
	case ET_PROMOTION:
	case ET_ASSIGNMENT_CONVERSION:
		ok = eval_const_expr(expr->ex_unary_expr->ue_expr, what, cv);
		if (ok)
			do_unary_conversion(expr, cv);
		break;
	case ET_SCALE:
		ok = eval_scale_const_expr(expr->ex_scale_expr, what, cv);
		break;
	case ET_BINARY:
		ok = eval_binary_const_expr(expr->ex_binary_expr, what, cv);
		break;
	case ET_UNARY:
		ok = eval_unary_const_expr(expr->ex_unary_expr, what, cv);
		break;
	case ET_CAST:
		if (!IS_ARITHMETIC_OR_PTR_TYPE(expr->ex_type->ty_code) &&
					expr->ex_type->ty_code != TY_ENUM) {
			diagf(ET_ERROR, expr->ex_lexinfo,
			      "Cast to a type that that's illegal in %s", what);
			ci_show_type(expr->ex_type, "type");
			ok = FALSE;
			break;
		}
		ok = eval_const_expr(expr->ex_unary_expr->ue_expr, what, cv);
		if (ok)
			do_unary_conversion(expr, cv);
		break;
	case ET_CONDITIONAL:
		ok = eval_conditional_const_expr(expr->ex_cond_expr, what, cv);
		break;
	default:
		ci_panic("bad expr type in ice");
		ok =  FALSE; /* to satisfy gcc */
	}
	return ok;
}
