/* ci_compile_expr.c - routines to compile an expression into interpreter code */

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


/*
 * parameter of type 'expr_t *' added to code_conversion() and
 * compile_conversion() so that a proper error message can be generated.
 * Dibyendu 26/12/98
 */
/* @(#)ci_compile_expr.c	1.27 04 Jun 1995 (UKC) */
char ups_ci_compile_expr_c_rcsid[] = "$Id$";

#define TEST_DATA

#include <mtrprog/ifdefs.h>

#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "ci_parse.h"
#include "ci_types.h"
#include "ci_constexpr.h"
#include "xc_opcodes.h"
#include "xc_machine.h"
#include "cx_link.h"
#include "ci_compile.h"
#include "ci_compile_expr.h"
#include "ci_util.h"
#include "ci_showexpr.h"

static void compile_func_call_expr PROTO((text_t *tx, func_call_expr_t *fce,
					  typecode_t typecode,
					  expr_context_t context));
static addrtype_t compile_multi_arrow_expr PROTO((text_t *tx,
						  multi_arrow_expr_t *ma,
						  expr_context_t context));
static addrtype_t compile_dot_expr PROTO((text_t *tx, dot_expr_t *de,
				    expr_context_t context));
static void compile_scale_expr PROTO((text_t *tx, scale_expr_t *sc,
				       expr_context_t context));
static addrtype_t compile_binary_expr PROTO((text_t *tx, binary_expr_t *be,
				       expr_context_t context));
static addrtype_t compile_unary_expr PROTO((text_t *tx, unary_expr_t *ue,
				      expr_context_t context));
static addrtype_t compile_conditional_expr PROTO((text_t *tx, cond_expr_t *co,
					    expr_context_t context));

static void compile_logical_expression PROTO((text_t *tx,
					      expr_t *left, expr_t *right,
					      optype_t op, expr_context_t context));

static int opcode_offset PROTO((typecode_t typecode));
static void code_deref PROTO((text_t *tx, typecode_t typecode, addrtype_t addrtype));
static opcode_t operator_to_opcode PROTO((text_t *tx, optype_t op, type_t *ltype, type_t *rtype));
static bool is_aggregate PROTO((type_t *type));

static addrtype_t unknown_addrtype PROTO((text_t *tx));
static void code_bitfield_op PROTO((text_t *tx, bitfield_t *bf, opcode_t op));
static void code_get_whole_bitfield PROTO((text_t *tx, addrtype_t addrtype));
static addrtype_t compile_conversion PROTO((text_t *tx, unary_expr_t *ue,
					    expr_context_t context,
					    expr_t *expr));
#ifdef TEST_DATA
static void vtest PROTO((const char *addr, int count));
#else
#define vtest(addr, count)	/* nothing */
#endif
static void initialise_array PROTO((text_t *tx, taddr_t addr, type_t *type,
				    initialiser_t *in,
				    loctype_t loctype, char *data));
static void initialise_struct PROTO((text_t *tx, taddr_t addr, type_t *type,
				     initialiser_t *in,
				     loctype_t loctype, char *data));
static long get_initialiser_value PROTO((text_t *tx, initialiser_t *in));
static void initialise PROTO((text_t *tx, taddr_t addr, type_t *type,
			      initialiser_t *in,
			      loctype_t loctype, char *data));
static void initialise_word PROTO((text_t *tx, initialiser_t *in,
				   loctype_t loctype, char *data,
				   taddr_t addr));
static addrtype_t code_conversion PROTO((text_t *tx, addrtype_t addrtype,
					 optype_t op, typecode_t typecode,
					 expr_t *expr));
static void compile_expr_for_jump PROTO((text_t *tx, expr_t *expr));
static long short_form PROTO((int func_index));

static bool
is_aggregate(type)
type_t *type;
{
	typecode_t code;

	code = type->ty_code;
	return code == TY_STRUCT || code == TY_UNION || code == DT_ARRAY_OF;
}

/*  Do all the static initialisation.  Note that we write into a copy
 *  of the data area rather then the real one, because we want to be
 *  able to reset and restart the machine.  To do this we zero the bss
 *  and memcpy the data copy area to the data.
 */
void
ci_do_static_initialisations(tx, initlist, data, extdata)
text_t *tx;
initlist_t *initlist;
char *data, *extdata;
{
	initlist_t *il;

	for (il = initlist; il != NULL; il = il->il_next) {
		char *initdata;
		loctype_t loctype;

		tx->tx_curfil = il->il_fil;

		if ((il->il_var->va_flags & VA_EXT_RELOC) != 0) {
			loctype = RT_EXTDATALOC;
			initdata = extdata;
		}
		else {
			loctype = RT_DATALOC;
			initdata = data;
		}
		initialise(tx, (taddr_t)il->il_var->va_addr,
			   il->il_var->va_type,
			   (initialiser_t *)il->il_initialiser_id,
			   loctype, initdata);
	}
	tx->tx_curfil = NULL;
}

static void
initialise_array(tx, addr, type, in, loctype, data)
text_t *tx;
taddr_t addr;
type_t *type;
initialiser_t *in;
loctype_t loctype;
char *data;
{
	int elsize;
	taddr_t addrlim;

	if (type->ty_code != DT_ARRAY_OF)
		ci_panic("array type botch in ia");
	if (type->ty_dim->di_htype != DT_CONSTANT)
		ci_panic("unknown array size botch in ia");
	elsize = ci_typesize((lexinfo_t *)NULL, type->ty_base);
	addrlim = addr + elsize * (type->ty_dim->di_high + 1);

	switch (in->in_inittype) {
	case IT_STRINGBYTES:
		if (addr + in->in_stringval->sc_size > addrlim)
			ci_panic("init string len botch in ia");
		memcpy(data + addr, in->in_stringval->sc_val,
						in->in_stringval->sc_size);
		break;
	case IT_LIST:
		for (in = in->in_list; in != NULL; in = in->in_next) {
			if (addr >= addrlim)
				ci_panic("too many initialisers botch in ia");
			initialise(tx, addr, type->ty_base, in, loctype, data);
			addr += elsize;
		}
		break;
	default:
		ci_panic("init type botch in ia");
	}
}

static void
initialise_struct(tx, addr, type, in, loctype, data)
text_t *tx;
taddr_t addr;
type_t *type;
initialiser_t *in;
loctype_t loctype;
char *data;
{
	taddr_t maxaddr;
	var_t *v;

	if (type->ty_code != TY_STRUCT)
		ci_panic("struct type botch in is");
	if (in->in_inittype != IT_LIST)
		ci_panic("init type botch in is");

	maxaddr = addr + ci_typesize((lexinfo_t *)NULL, type);
	v = type->ty_aggr_or_enum->ae_aggr_members;

	for (in = in->in_list; in != NULL; in = in->in_next) {
		if (v == NULL || addr + v->va_addr >= maxaddr)
			ci_panic("too many initialisers botch in is");
		initialise(tx, addr+v->va_addr, v->va_type, in, loctype, data);
		v = v->va_next;
	}
}

static long
get_initialiser_value(tx, in)
text_t *tx;
initialiser_t *in;
{
	long val;

	switch (in->in_inittype) {
	case IT_INTVAL:
		val = in->in_intval;
		break;
	case IT_FLOATVAL:
		ci_panic("float botch in giv");
		val = 0; /* to satisfy gcc */
		break;
	case IT_STRINGADDR:
		diagf(ET_ERROR, (lexinfo_t *)NULL,
		      "Can't initialise non-word with string `%.*s'",
					(int)in->in_stringval->sc_size,
					in->in_stringval->sc_val);
		val = 0;
		break;
	case IT_VARADDR:
		diagf(ET_ERROR, (lexinfo_t *)NULL,
		      "Can't initialise non-word with address of `%s'",
						in->in_addrvar->va_name);
		val = 0;
		break;
	case IT_FUNCADDR:
		diagf(ET_ERROR, in->in_func_lexinfo,
		      "Can't initialise non-word with address of function `%s'",
						in->in_func_name);
		val = 0;
		break;
	default:
		ci_panic("bad inittype in giv");
		val = 0; /* to satisfy gcc */
		break;
	}

	return val;
}

static void
initialise_word(tx, in, loctype, data, addr)
text_t *tx;
initialiser_t *in;
loctype_t loctype;
char *data;
taddr_t addr;
{
	long val;

	switch (in->in_inittype) {
	case IT_STRINGADDR:
		ci_add_string_reloc(tx, loctype, addr,
				    in->in_stringval, in->in_offset);
		val = 0;
		break;
	case IT_VARADDR:
		ci_add_var_reloc(tx, loctype, addr,
				 in->in_addrvar, in->in_offset);
		val = 0;
		break;
	case IT_FUNCADDR:
		ci_add_func_reloc(tx, loctype, addr);
		val = short_form(ci_funcname_to_index(tx, in->in_func_lexinfo,
						      in->in_func_name));
		break;
	default:
		val = get_initialiser_value(tx, in);
		break;
	}

	vtest(data + addr, sizeof(long));
	*(long *)(data + addr) = val;
}

static void
initialise(tx, addr, type, in, loctype, data)
text_t *tx;
taddr_t addr;
type_t *type;
initialiser_t *in;
loctype_t loctype;
char *data;
{
	switch (ci_effective_typecode(type->ty_code)) {
	case DT_ARRAY_OF:
		initialise_array(tx, addr, type, in, loctype, data);
		break;
	case TY_STRUCT:
		initialise_struct(tx, addr, type, in, loctype, data);
		break;
	case TY_UNION:
		initialise(tx, addr,
			   type->ty_aggr_or_enum->ae_aggr_members->va_type,
			   in->in_list, loctype, data);
		break;
	case DT_PTR_TO:
		initialise_word(tx, in, loctype, data, addr);
		break;
	case TY_CHAR:
	case TY_UCHAR:
		vtest(data + addr, sizeof(char));
		*(char *)(data + addr) = get_initialiser_value(tx, in);
		break;
	case TY_SHORT:
	case TY_USHORT:
		vtest(data + addr, sizeof(short));
		*(short *)(data + addr) = get_initialiser_value(tx, in);
		break;
	case TY_ENUM:
	case TY_U_ENUM:
	case TY_INT:
	case TY_UINT:
	case TY_BOOLEAN:
		vtest(data + addr, sizeof(int));
		*(int *)(data + addr) = get_initialiser_value(tx, in);
		break;
	case TY_LONG:
	case TY_ULONG:
		vtest(data + addr, sizeof(long));
		*(long *)(data + addr) = get_initialiser_value(tx, in);
		break;
#if WANT_LL
	case TY_LONGLONG:
	case TY_ULONGLONG:
		vtest(data + addr, sizeof(long_long_t));
		*(long_long_t *)(data + addr) = get_initialiser_value(tx, in);
		break;
#endif
	case TY_FLOAT:
		if (in->in_inittype != IT_FLOATVAL)
			ci_panic("it botch in i");
		vtest(data + addr, sizeof(float));
		*(float *)(data + addr) = in->in_floatval;
		break;
	case TY_DOUBLE:
		if (in->in_inittype != IT_FLOATVAL)
			ci_panic("it botch in i");
		vtest(data + addr, sizeof(double));
		*(double *)(data + addr) = in->in_floatval;
		break;
#if WANT_LDBL
	case TY_LONGDOUBLE:
		if (in->in_inittype != IT_FLOATVAL)
			ci_panic("it botch in i");
		vtest(data + addr, sizeof(long_double_t));
		*(long_double_t *)(data + addr) = in->in_floatval;
		break;
#endif
	case TY_BITFIELD:
		{
			long *p_val, mask, initval;
			bitfield_t *bf;

			bf = type->ty_bitfield;
			mask = ((1 << bf->bf_width) - 1) << bf->bf_offset;
			p_val = (long *)(data + addr);
			initval = get_initialiser_value(tx, in);
			*p_val = (*p_val & ~mask) |
					    ((initval << bf->bf_offset) & mask);
		}
		break;
	default:
		ci_panic("bad type in initialise");
	}
}

#ifdef TEST_DATA
static void
vtest(addr, count)
const char *addr;
int count;
{
	while (--count >= 0) {
		if (*addr++ != 0)
			ci_panic("vtest botch");
	}
}
#endif

static opcode_t
operator_to_opcode(tx, op, ltype, rtype)
text_t *tx;
optype_t op;
type_t *ltype, *rtype;
{
	bool is_signed;
	typecode_t typecode;
	opcode_t opcode;

	typecode = ltype->ty_code;
	is_signed = ci_is_signed_type(ltype->ty_code);

	/*  There are signed and unsigned variants of the integer
	 *  relational operators, and the signs of both the operands
	 *  must agree.
	 */
	switch (op) {
	case OP_MUL:
	case OP_DIV:
	case OP_LESS:
	case OP_GREATER:
	case OP_LESS_OR_EQUAL:
	case OP_GREATER_OR_EQUAL:
		if (is_signed != ci_is_signed_type(rtype->ty_code))
			ci_panic("type botch in rto");
		break;
	default:
		break;
	}

	switch(op) {
	case OP_BITWISE_AND:
	case OP_BITWISE_AND_ASSIGN:
		opcode = OC_BITWISE_AND;
		break;
	case OP_BITWISE_XOR:
	case OP_BITWISE_XOR_ASSIGN:
		opcode = OC_BITWISE_XOR;
		break;
	case OP_BITWISE_OR:
	case OP_BITWISE_OR_ASSIGN:
		opcode = OC_BITWISE_OR;
		break;
	case OP_MOD:
	case OP_MOD_ASSIGN:
		opcode = OC_MOD;
		break;
	case OP_PLUS:
	case OP_PLUS_ASSIGN:
		opcode = OC_ADD;
		break;
	case OP_MUL:
	case OP_MUL_ASSIGN:
		opcode = is_signed ? OC_MUL_SIGNED : OC_MUL_UNSIGNED;
		break;
	case OP_DIV:
	case OP_DIV_ASSIGN:
		if (tx->tx_flags & CI_CP_CHECK_DIV)
			opcode = is_signed ? OC_CHKDIV_SIGNED : OC_CHKDIV_UNSIGNED;
		else
			opcode = is_signed ? OC_DIV_SIGNED : OC_DIV_UNSIGNED;
		break;
	case OP_MINUS:
	case OP_MINUS_ASSIGN:
		opcode = OC_SUB;
		break;
	case OP_LSHIFT:
	case OP_LSHIFT_ASSIGN:
		opcode = OC_LSHIFT;
		break;
	case OP_RSHIFT:
	case OP_RSHIFT_ASSIGN:
		opcode = OC_RSHIFT;
		break;
	case OP_IS_EQUAL:
		opcode = OC_IS_EQUAL;
		break;
	case OP_NOT_EQUAL:
		opcode = OC_NOT_EQUAL;
		break;
	case OP_LESS:
		opcode = is_signed ? OC_LESS_SIGNED : OC_LESS_UNSIGNED;
		break;
	case OP_GREATER:
		opcode = is_signed ? OC_GREATER_SIGNED : OC_GREATER_UNSIGNED;
		break;
	case OP_LESS_OR_EQUAL:
		opcode = is_signed ? OC_LESS_OR_EQUAL_SIGNED
				   : OC_LESS_OR_EQUAL_UNSIGNED;
		break;
	case OP_GREATER_OR_EQUAL:
		opcode = is_signed ? OC_GREATER_OR_EQUAL_SIGNED
				     : OC_GREATER_OR_EQUAL_UNSIGNED;
		break;
	default:
		ci_panic("unknown operator in oto");
		opcode = (opcode_t)-1; /* to satisfy gcc */
		break;
	}

#if WANT_LL
	if (typecode == TY_LONGLONG || typecode == TY_ULONGLONG) {
		if (rtype->ty_code != typecode)
			ci_panic("ll type botch in oto");

		opcode = (opcode_t)((int)opcode - 1);
	}
#endif

#if WANT_LDBL
	if (typecode == TY_FLOAT || typecode == TY_DOUBLE || typecode == TY_LONGDOUBLE) {
#else
	if (typecode == TY_FLOAT || typecode == TY_DOUBLE) {
#endif
		if (rtype->ty_code != typecode)
			ci_panic("fp type botch in oto");

		/*  Check that this is a generic opcode (one of the
		 *  ones that has integer, floating, double and long double
		 *  variants in that order).
		 */
		switch (opcode) {
		case OC_MUL_SIGNED:
		case OC_DIV_SIGNED:
		case OC_CHKDIV_SIGNED:
		case OC_ADD:
		case OC_SUB:
		case OC_IS_EQUAL:
		case OC_NOT_EQUAL:
		case OC_LESS_SIGNED:
		case OC_GREATER_SIGNED:
		case OC_LESS_OR_EQUAL_SIGNED:
		case OC_GREATER_OR_EQUAL_SIGNED:
			break;
		default:
			ci_panic("fp operands for non generic opcode");
		}

		switch (typecode) {
		case TY_FLOAT:
			opcode = (opcode_t)((int)opcode + 1);
			break;
		case TY_DOUBLE:
			opcode = (opcode_t)((int)opcode + 2);
			break;
#ifdef WANT_LBL
		case TY_LONGDOUBLE:
			opcode = (opcode_t)((int)opcode + 3);
			break;
#endif
		default:
			ci_panic("fp type botch in oto");
		}
	}

	return opcode;
}

void
ci_code_constpush(tx, val)
text_t *tx;
long val;
{
	if (val < 0) {
		ci_code_generic_opcode(tx, OC_NEG_CONSTPUSH_B,
							(stackword_t)-val);
	}
	else {
		ci_code_generic_opcode(tx, OC_CONSTPUSH_B, (stackword_t)val);
	}
}

static int
opcode_offset(typecode)
typecode_t typecode;
{
	switch (ci_effective_typecode(typecode)) {
#if WANT_LL
	case TY_LONGLONG:
	case TY_ULONGLONG:
		return LONGLONG_FORM_OFFSET;
#endif
	case TY_LONG:
	case TY_ULONG:
	case TY_INT:
	case TY_UINT:
	case TY_ENUM:
	case TY_U_ENUM:
	case TY_BOOLEAN:
	case DT_PTR_TO:
		return LONG_FORM_OFFSET;
	case TY_FLOAT:
		return FLOAT_FORM_OFFSET;
#if WANT_LDBL
	case TY_LONGDOUBLE:
		return LDOUBLE_FORM_OFFSET;
#endif
	case TY_DOUBLE:
		return DOUBLE_FORM_OFFSET;
	case TY_SHORT:
	case TY_USHORT:
		return SHORT_FORM_OFFSET;
	case TY_CHAR:
	case TY_UCHAR:
		return BYTE_FORM_OFFSET;
	default:
		ci_panic("bad type in oo");
		return 0; /* to satisfy gcc */
	}
}

static addrtype_t
unknown_addrtype(tx)
text_t *tx;
{
	return tx->tx_have_proc_vars ? AT_PROC_ADDR : AT_CI_ADDR;
}

static void
code_deref(tx, typecode, addrtype)
text_t *tx;
typecode_t typecode;
addrtype_t addrtype;
{
	opcode_t opcode;

	if (ci_is_signed_type(typecode))
		opcode = (addrtype == AT_PROC_ADDR) ? OC_PROC_DEREF_SIGNED_BYTE
				     		    : OC_DEREF_SIGNED_BYTE;
	else
		opcode = (addrtype == AT_PROC_ADDR) ? OC_PROC_DEREF_UNSIGNED_BYTE
						    : OC_DEREF_UNSIGNED_BYTE;

	ci_code_opcode(tx, (opcode_t)((int)opcode + opcode_offset(typecode)));
}

static void
code_bitfield_op(tx, bf, op)
text_t *tx;
bitfield_t *bf;
opcode_t op;
{
	if (bf->bf_type->ty_code == TY_UINT)
		op = (opcode_t)((int)op + 1);

	ci_code_opcode(tx, op);
	ci_code_byte(tx, bf->bf_offset);
	ci_code_byte(tx, bf->bf_width);
}

static void
code_get_whole_bitfield(tx, addrtype)
text_t *tx;
addrtype_t addrtype;
{
	opcode_t opcode;

	opcode = (addrtype == AT_PROC_ADDR) ? OC_PROC_DEREF_UNSIGNED_LONG
					    : OC_DEREF_UNSIGNED_LONG;
	ci_code_opcode(tx, opcode);
}

void
ci_code_assign(tx, type, want_value, addrtype)
text_t *tx;
type_t *type;
bool want_value;
addrtype_t addrtype;
{
	opcode_t opcode;
	typecode_t typecode;

	typecode = type->ty_code;
	if (typecode == TY_BITFIELD) {
		typecode = type->ty_bitfield->bf_type->ty_code;
		code_bitfield_op(tx, type->ty_bitfield, OC_INSERT_SIGNED_BITFIELD);
	}

	if (addrtype == AT_PROC_ADDR)
		opcode = want_value ? OC_PROC_ASSIGN_AND_PUSH_BYTE
				    : OC_PROC_ASSIGN_BYTE;
	else
		opcode = want_value ? OC_ASSIGN_AND_PUSH_BYTE
				    : OC_ASSIGN_BYTE;
	ci_code_opcode(tx, (opcode_t)((int)opcode + opcode_offset(typecode)));

	if (want_value && type->ty_code == TY_BITFIELD)
		code_bitfield_op(tx, type->ty_bitfield, OC_EXTRACT_SIGNED_BITFIELD);
}

addrtype_t
ci_compile_var_reference(tx, v, lx, context)
text_t *tx;
var_t *v;
lexinfo_t *lx;
expr_context_t context;
{
	opcode_t generic_opcode;
	bool is_civar, need_deref;
	addrtype_t addrtype;
	taddr_t addr;

	is_civar = (v->va_flags & VA_IS_CI_VAR) != 0;

	if (context == EC_EFFECT)
		return AT_CI_ADDR;

	if (context != EC_ADDRESS && context != EC_VALUE)
		ci_panic("bad context in cvr");

	need_deref = FALSE;
	if (context == EC_ADDRESS || is_aggregate(v->va_type)) {
		if (IS_LOCAL_CLASS(v->va_class)) {
			if (is_civar)
				generic_opcode = OC_PUSH_STACKADDR_B;
			else {
				generic_opcode = (v->va_class == CL_ARG)
							? OC_PROC_PUSH_AP_ADDR_B
							: OC_PROC_PUSH_FP_ADDR_B;
			}
		}
		else
			generic_opcode = OC_CONSTPUSH_B;
		addrtype = is_civar ? AT_CI_ADDR : unknown_addrtype(tx);
	}
	else {
		bool is_signed;

		is_signed = ci_is_signed_type(v->va_type->ty_code);
		if (is_civar) {
			opcode_t generic_byte_opcode;

			if (IS_LOCAL_CLASS(v->va_class)) {
				generic_byte_opcode = is_signed
					 ? OC_PUSH_SIGNED_BYTE_AT_STACKADDR_B
					 : OC_PUSH_UNSIGNED_BYTE_AT_STACKADDR_B;
			}
			else {
				generic_byte_opcode = is_signed
					 ? OC_PUSH_SIGNED_BYTE_AT_ADDR_B
					 : OC_PUSH_UNSIGNED_BYTE_AT_ADDR_B;
			}
			generic_opcode = (opcode_t)((int)generic_byte_opcode +
						N_OPCODE_SIZES *
						opcode_offset(v->va_type->ty_code));
		}
		else {
			if (IS_LOCAL_CLASS(v->va_class))
				generic_opcode = (v->va_class == CL_ARG)
							? OC_PROC_PUSH_AP_ADDR_B
							: OC_PROC_PUSH_FP_ADDR_B;
			else
				generic_opcode = OC_CONSTPUSH_B;
			need_deref = TRUE;
		}

		/*  Currently, we don't track variable values, so we don't
		 *  know whether an address stored in a variable is in the
		 *  C interpreter address space or not.  Thus we have to
		 *  say that it's not, and let the interpreter decide at
		 *  execution time.
		 */
		addrtype = unknown_addrtype(tx);
	}

	if (v->va_class == CL_REG && !tx->tx_want_reg_relocs) {
		if (tx->tx_regno_to_addr_proc == NULL)
			ci_panic("can't get reg addr in cvr");
		addr = (*tx->tx_regno_to_addr_proc)((int)v->va_addr);
	}
	else
		addr = v->va_addr;

	if (v->va_class == CL_REG && tx->tx_want_reg_relocs) {
		reg_reloc_t *rr;

		ci_code_opcode(tx, LONG_FORM(generic_opcode));

		rr = (reg_reloc_t *)alloc(tx->tx_apool,
						sizeof(reg_reloc_t));
		rr->rr_addr = tx->tx_pc;
		rr->rr_regno = v->va_addr;
		rr->rr_next = tx->tx_reg_relocs;
		tx->tx_reg_relocs = rr;

		ci_code_long(tx, 0);

	}
	else if (is_civar && !IS_LOCAL_CLASS(v->va_class)) {
		ci_code_opcode(tx, LONG_FORM(generic_opcode));
		ci_add_var_reloc(tx, RT_TEXTLOC, tx->tx_pc, v, (long)0);
		ci_code_long(tx, 0);
	}
	else
		ci_code_generic_opcode(tx, generic_opcode, (stackword_t)addr);

	if (need_deref)
		code_deref(tx, v->va_type->ty_code, AT_PROC_ADDR);

	return addrtype;
}

static long
short_form(func_index)
int func_index;
{
	long res;

	res = (unsigned short)func_index;

	if ((short)res != func_index)
		panic("func_index botch in short_form");

	return res;
}

static void
compile_func_call_expr(tx, fce, typecode, context)
text_t *tx;
func_call_expr_t *fce;
typecode_t typecode;
expr_context_t context;
{
	expr_list_t *el, *arglist;
	taddr_t save_sp, before_args_sp, bytes_to_pop;
	bool is_direct_call;
	int nargs, excess_slots, func_index;

	is_direct_call = fce->fce_func->ex_exprtype == ET_FUNCNAME;
	if (is_direct_call) {
		func_index = ci_funcname_to_index(tx,
						fce->fce_func->ex_lexinfo,
						fce->fce_func->ex_var->va_name);
	}
	else
		func_index = 0;

	arglist = ci_reverse_expr_list(fce->fce_expr_list);

	for (nargs = 0, el = arglist; el != NULL; el = el->el_next, ++nargs)
		;

	save_sp = tx->tx_sp;

	/*  If there is an argument checker callback, and this is a call to
	 *  an undefined function, or a call via a function pointer expression,
	 *  then call the argument checker for each argument.
	 */
	if (tx->tx_checkarg_proc != NULL &&
	    (func_index <= 0 || func_index >= tx->tx_num_funcs)) {
		int argn;

		argn = 0;
		for (el = arglist; el != NULL; el = el->el_next) {
			long val;

			if ((*tx->tx_checkarg_proc)(el->el_expr->ex_type,
						    nargs, argn, &val))
				ci_code_constpush(tx, val);

			++argn;
		}
	}

	/*  We save this just so we can do a consistency check of nargs
	 *  and excess_slots against the number of bytes pushed on the stack.
	 */
	before_args_sp = tx->tx_sp;

	excess_slots = 0;
	for (el = arglist; el != NULL; el = el->el_next) {
		type_t *type;
		expr_t *expr;

		expr = el->el_expr;
		type = expr->ex_type;
		if (!type)
		{
			ci_panic("no type in value context");
		}
		else
		if (type->ty_code == TY_STRUCT || type->ty_code == TY_UNION) {
			long nbytes;

			/*  DUBIOUS: we round the type size up to a multiple
			 *           of four (the size of a stack slot).
			 *	     This means we copy some extra bytes.
			 *	     If we had memory protection at byte rather
			 *	     than word granularity this could bite us.
			 */
			nbytes = ci_typesize(expr->ex_lexinfo, type);
			if ((nbytes % 4) != 0)
				nbytes += 4 - nbytes % 4;

			if (expr->ex_exprtype == ET_FUNC_CALL) {
				ci_code_opcode(tx, OC_RESERVE_BYTES);
				ci_code_long(tx, nbytes);
				tx->tx_sp += nbytes + 4;
				ci_compile_expression(tx, expr, EC_ADDRESS);
				ci_code_opcode(tx, OC_POP);  /* lose address */
			}
			else {
				addrtype_t addrtype;

				addrtype = ci_compile_expression(tx, expr,
								  EC_ADDRESS);
				if (addrtype == AT_CI_ADDR)
					ci_code_opcode(tx, OC_PUSH_BYTES);
				else
					ci_code_opcode(tx, OC_PROC_PUSH_BYTES);

				ci_code_long(tx, nbytes);
				tx->tx_sp += nbytes - 4;
			}
			excess_slots += nbytes / 4 - 1;
		}
		else {
			ci_compile_expression(tx, expr, EC_VALUE);
			switch (expr->ex_type->ty_code) {
			case TY_FLOAT:
				excess_slots += FLOAT_NSLOTS - 1;
				break;
			case TY_DOUBLE:
				excess_slots += DOUBLE_NSLOTS - 1;
				break;
#if WANT_LDBL
			case TY_LONGDOUBLE:
				excess_slots += LDOUBLE_NSLOTS - 1;
				break;
#endif
#if WANT_LL
			case TY_LONGLONG:
			case TY_ULONGLONG:
				excess_slots += LONGLONG_NSLOTS - 1;
				break;
#endif
			default:
				break;
			}
		}
	}

	/*  If we are calling a structure or union valued function,
	 *  emit code to push the number of stack slots taken by
	 *  the argument list on the stack.  This is so the called
	 *  function knows where to look for the address where it
	 *  should copy the result struct to.
	 */
	if (typecode == TY_STRUCT || typecode == TY_UNION) {
		++excess_slots;
		ci_code_constpush(tx,
				  (context == EC_VALUE) ? nargs + excess_slots : 0);
	}

	/*  Put the argument list back in the original order.
	 */
	fce->fce_expr_list = ci_reverse_expr_list(arglist);

	if ((nargs+excess_slots) * sizeof(stackword_t) != tx->tx_sp - before_args_sp)
		ci_panic("args botch in cfc");

	if (is_direct_call)
		ci_code_generic_opcode(tx, OC_CALL_B, (stackword_t)func_index);
	else {
		ci_compile_expression(tx, fce->fce_func, EC_VALUE);
		ci_code_opcode(tx, OC_CALL_INDIRECT);
	}
	ci_code_byte(tx, nargs + excess_slots);

#if WANT_TYPE_PUSHED
	ci_code_byte(tx, typecode);
#endif

	bytes_to_pop = tx->tx_sp - save_sp;

	if (bytes_to_pop != 0) {
		if (bytes_to_pop == 4)
			ci_code_opcode(tx, OC_POP);
		else {
			ci_code_generic_opcode(tx, OC_POPMANY_B, bytes_to_pop);
			tx->tx_sp -= bytes_to_pop;
		}
	}

	if (context == EC_VALUE) {
		switch (typecode) {
#if WANT_LDBL
		case TY_LONGDOUBLE:
			ci_code_opcode(tx, OC_PUSH_LONG_DOUBLE_RETVAL);
			break;
#endif
		case TY_DOUBLE:
			ci_code_opcode(tx, OC_PUSH_DOUBLE_RETVAL);
			break;
		case TY_FLOAT:
			ci_code_opcode(tx, OC_PUSH_FLOAT_RETVAL);
			break;
#if WANT_LL
		case TY_LONGLONG:
		case TY_ULONGLONG:
			ci_code_opcode(tx, OC_PUSH_LONG_LONG_RETVAL);
			break;
#endif
		case TY_STRUCT:
		case TY_UNION:
			break;
		default:
			ci_code_opcode(tx, OC_PUSH_WORD_RETVAL);
			break;
		}
	}
}

static addrtype_t
compile_multi_arrow_expr(tx, ma, context)
text_t *tx;
multi_arrow_expr_t *ma;
expr_context_t context;
{
	if (context != EC_VALUE && context != EC_EFFECT)
		panic("context botch in cmae");

	ci_compile_expression(tx, ma->ma_index, context);
	ci_compile_expression(tx, ma->ma_aggr, context);

	if (context == EC_VALUE) {
		ci_code_constpush(tx, (long)ma->ma_member->va_addr);
		ci_code_opcode(tx, OC_MULTI_ARROW);
	}

	return AT_PROC_ADDR;
}

static addrtype_t
compile_dot_expr(tx, de, context)
text_t *tx;
dot_expr_t *de;
expr_context_t context;
{
	addrtype_t expr_addrtype, addrtype;

	expr_addrtype = ci_compile_expression(tx, de->de_aggr, EC_ADDRESS);
	if (context == EC_EFFECT) {
		ci_code_opcode(tx, OC_POP);
		addrtype = AT_CI_ADDR;
	}
	else {
		var_t *v;

		v = de->de_member;
		if (v->va_addr != 0) {
			ci_code_constpush(tx, (long)v->va_addr);
			ci_code_opcode(tx, OC_ADD);
		}
		if (context == EC_VALUE && !is_aggregate(v->va_type)) {
			if (v->va_type->ty_code == TY_BITFIELD) {
				bitfield_t *bf;

				bf = v->va_type->ty_bitfield;
				code_deref(tx, bf->bf_type->ty_code,
					   expr_addrtype);
				code_bitfield_op(tx, bf,
						 OC_EXTRACT_SIGNED_BITFIELD);
			}
			else
				code_deref(tx, v->va_type->ty_code, expr_addrtype);
			addrtype = unknown_addrtype(tx);
		}
		else
			addrtype = expr_addrtype;
	}
	return addrtype;
}

static void
compile_logical_expression(tx, left, right, op, context)
text_t *tx;
expr_t *left, *right;
optype_t op;
expr_context_t context;
{
	labeldesc_t endlabel;
	opcode_t opcode;

	opcode = (op == OP_LOGICAL_AND) ? OC_JUMP_IF_ZERO : OC_JUMP_IF_NON_ZERO;

	compile_expr_for_jump(tx, left);

	if (context == EC_VALUE)
		ci_code_opcode(tx, OC_DUP);
	endlabel = ci_code_jump_to_label(tx, opcode, NO_LABELDESC);
	if (context == EC_VALUE)
		ci_code_opcode(tx, OC_POP);

	if (context == EC_VALUE)
		compile_expr_for_jump(tx, right);
	else
		ci_compile_expression(tx, right, context);

	ci_resolve_and_free_label(tx, endlabel);

	if (context == EC_VALUE)
		ci_code_opcode(tx, OC_CVT_TO_BOOL);
}

static void
compile_expr_for_jump(tx, expr)
text_t *tx;
expr_t *expr;
{
	ci_compile_expression(tx, expr, EC_VALUE);

	switch (expr->ex_type->ty_code) {
	case TY_FLOAT:
		ci_code_opcode(tx, OC_CVT_FLOAT_TO_BOOL);
		break;
	case TY_DOUBLE:
		ci_code_opcode(tx, OC_CVT_DOUBLE_TO_BOOL);
		break;
#if WANT_LDBL
	case TY_LONGDOUBLE:
		ci_code_opcode(tx, OC_CVT_LONG_DOUBLE_TO_BOOL);
		break;
#endif
#if WANT_LL
	case TY_LONGLONG:
	case TY_ULONGLONG:
		ci_code_opcode(tx, OC_CVT_LONG_LONG_TO_BOOL);
		break;
#endif
	default:
		break;
	}
}

static void
compile_scale_expr(tx, sc, context)
text_t *tx;
scale_expr_t *sc;
expr_context_t context;
{
	type_t *etype;
	typecode_t rtypecode;

	ci_compile_expression(tx, sc->sc_expr, context);

	if (context == EC_EFFECT)
		return;
	if (context != EC_VALUE)
		ci_panic("bad context in cse");

	ci_code_constpush(tx, sc->sc_factor);

	etype = sc->sc_expr->ex_type;
	rtypecode = ci_is_signed_type(etype->ty_code) ? TY_INT : TY_UINT;
	ci_code_opcode(tx, operator_to_opcode(tx, sc->sc_op, etype,
					      ci_code_to_type(rtypecode)));
}

static addrtype_t
compile_binary_expr(tx, be, context)
text_t *tx;
binary_expr_t *be;
expr_context_t context;
{
	expr_t *addrleft, *left, *right;
	optype_t op;
	addrtype_t addrtype, left_at, right_at;

	op = be->be_op;
	left = be->be_left;
	right = be->be_right;

	if (context == EC_ADDRESS && op != OP_ASSIGN)
		panic("be in addr context");

	switch(op) {
	case OP_ASSIGN:
		left_at = ci_compile_expression(tx, left, EC_ADDRESS);
		if (left->ex_type->ty_code == TY_STRUCT ||
					left->ex_type->ty_code == TY_UNION) {
			opcode_t opcode;

			if (context == EC_VALUE || context == EC_ADDRESS)
				ci_code_opcode(tx, OC_DUP);
			right_at = ci_compile_expression(tx, right, EC_ADDRESS);

			/*  BUG: the following is dubious
			 */
			if (left_at == AT_CI_ADDR && right_at == AT_CI_ADDR)
				addrtype = AT_CI_ADDR;
			else
				addrtype = unknown_addrtype(tx);

			/*  We don't need a memcpy if the RHS is a function
			 *  call because the return statement in the function
			 *  does the copy for us.
			 */
			if (right->ex_exprtype == ET_FUNC_CALL)
				ci_code_opcode(tx, OC_POP);
			else {
				stackword_t arg;

				opcode = (addrtype == AT_PROC_ADDR) ?
						OC_PROC_MEMCPY_B : OC_MEMCPY_B;
				arg = ci_typesize((lexinfo_t *)NULL,
								left->ex_type);
				ci_code_generic_opcode(tx, opcode, arg);
			}
		}
		else {
			if (left->ex_type->ty_code == TY_BITFIELD) {
				ci_code_opcode(tx, OC_DUP);
				code_get_whole_bitfield(tx, left_at);
			}
			right_at = ci_compile_expression(tx, right, EC_VALUE);
			ci_code_assign(tx, left->ex_type,
						context == EC_VALUE, left_at);
			addrtype = right_at;
		}
		break;

	case OP_MUL_ASSIGN:
	case OP_DIV_ASSIGN:
	case OP_MOD_ASSIGN:
	case OP_PLUS_ASSIGN:
	case OP_MINUS_ASSIGN:
	case OP_LSHIFT_ASSIGN:
	case OP_RSHIFT_ASSIGN:
	case OP_BITWISE_AND_ASSIGN:
	case OP_BITWISE_XOR_ASSIGN:
	case OP_BITWISE_OR_ASSIGN:
		addrleft = left;
		while (addrleft->ex_exprtype == ET_PROMOTION)
			addrleft = addrleft->ex_unary_expr->ue_expr;

		left_at = ci_compile_expression(tx, addrleft, EC_ADDRESS);
		ci_code_opcode(tx, OC_DUP);

		if (addrleft->ex_type->ty_code == TY_BITFIELD) {
			code_get_whole_bitfield(tx, left_at);
			ci_code_opcode(tx, OC_DUP);
			code_bitfield_op(tx, left->ex_type->ty_bitfield,
						OC_EXTRACT_SIGNED_BITFIELD);
		}
		else
			code_deref(tx, addrleft->ex_type->ty_code, left_at);

		if (left != addrleft) {
			code_conversion(tx, left_at,
					left->ex_unary_expr->ue_op,
					addrleft->ex_type->ty_code, NULL);
		}

		ci_compile_expression(tx, right, EC_VALUE);
		ci_code_opcode(tx, operator_to_opcode(tx, op, left->ex_type,
							      right->ex_type));

		if (left != addrleft) {
			code_conversion(tx, left_at,
					ci_typecode_to_cvt_op(addrleft->ex_type->ty_code),
					left->ex_type->ty_code, NULL);
		}

		ci_code_assign(tx, addrleft->ex_type,
						context == EC_VALUE, left_at);
		addrtype = left_at;
		break;

	case OP_COMMA:
		ci_compile_expression(tx, left, EC_EFFECT);
		addrtype = ci_compile_expression(tx, right, context);
		break;

	case OP_LOGICAL_OR:
	case OP_LOGICAL_AND:
		compile_logical_expression(tx, left, right, op, context);
		addrtype = AT_CI_ADDR;
		break;

	case OP_PLUS:
	case OP_MINUS:
	case OP_BITWISE_OR:
	case OP_BITWISE_XOR:
	case OP_BITWISE_AND:
	case OP_IS_EQUAL:
	case OP_NOT_EQUAL:
	case OP_LSHIFT:
	case OP_RSHIFT:
	case OP_MUL:
	case OP_DIV:
	case OP_MOD:
		left_at = ci_compile_expression(tx, left, context);
		right_at = ci_compile_expression(tx, right, context);
		if (context == EC_VALUE)
			ci_code_opcode(tx, operator_to_opcode(tx, op, left->ex_type,
							      	  right->ex_type));
		if (left_at == AT_CI_ADDR && right_at == AT_CI_ADDR)
			addrtype = AT_CI_ADDR;
		else
			addrtype = unknown_addrtype(tx);
		break;

	case OP_LESS:
	case OP_GREATER:
	case OP_LESS_OR_EQUAL:
	case OP_GREATER_OR_EQUAL:
		ci_compile_expression(tx, left, context);
		ci_compile_expression(tx, right, context);
		if (context == EC_VALUE)
			ci_code_opcode(tx, operator_to_opcode(tx, op, left->ex_type,
							          right->ex_type));
		addrtype = AT_CI_ADDR;
		break;

	default:
		ci_panic("bad operator in cbe");
		addrtype = AT_CI_ADDR;	/* to satisfy gcc */
		break;
	}
	return addrtype;
}

static addrtype_t
compile_unary_expr(tx, ue, context)
text_t *tx;
unary_expr_t *ue;
expr_context_t context;
{
	optype_t op;
	opcode_t opcode;
	expr_t *expr;
	int size;
	bool is_preop;
	expr_context_t expr_context;
	addrtype_t expr_addrtype, addrtype;

	op = ue->ue_op;
	expr = ue->ue_expr;

	switch (context) {
	case EC_ADDRESS:
		if (op != OP_DEREF)
			ci_panic("bad context in cue");
		break;
	case EC_VALUE:
	case EC_EFFECT:
		break;
	default:
		ci_panic("bad context in cue");
	}

	switch (op) {
	case OP_ADDRESS_OF:
		expr_context = (context == EC_EFFECT) ? EC_EFFECT : EC_ADDRESS;
		addrtype = ci_compile_expression(tx, expr, expr_context);
		break;

	case OP_DEREF:
		{
			type_t *base;

			expr_context = (context == EC_EFFECT) ? EC_EFFECT
							      : EC_VALUE;
			expr_addrtype = ci_compile_expression(tx, expr,
							      expr_context);
			base = expr->ex_type->ty_base;
			if (context == EC_VALUE && !is_aggregate(base) &&
					      base->ty_code != DT_FUNC_RETURNING) {
				code_deref(tx, base->ty_code, expr_addrtype);
				addrtype = unknown_addrtype(tx);
			}
			else
				addrtype = expr_addrtype;
		}
		break;

	case OP_PREINC:
	case OP_PREDEC:
	case OP_POSTINC:
	case OP_POSTDEC:
		is_preop = op == OP_PREINC || op == OP_PREDEC;
		if (context == EC_VALUE && !is_preop)
			ci_code_opcode(tx, OC_RESERVE_SLOT);
		expr_addrtype = ci_compile_expression(tx, ue->ue_expr, EC_ADDRESS);
		ci_code_opcode(tx, OC_DUP);

		if (expr->ex_type->ty_code == TY_BITFIELD) {
			code_get_whole_bitfield(tx, expr_addrtype);
			ci_code_opcode(tx, OC_DUP);
			code_bitfield_op(tx, expr->ex_type->ty_bitfield,
							OC_EXTRACT_SIGNED_BITFIELD);
		}
		else
			code_deref(tx, expr->ex_type->ty_code, expr_addrtype);

		if (context == EC_VALUE && !is_preop)
			ci_code_opcode(tx, OC_DUP_BACK_ONE);

		if (expr->ex_type->ty_code == DT_PTR_TO)
			size = ci_typesize((lexinfo_t *)NULL,
							expr->ex_type->ty_base);
		else
			size = 1;
		ci_code_constpush(tx, size);
		opcode = (op == OP_PREDEC || op == OP_POSTDEC) ? OC_SUB
							       : OC_ADD;
		ci_code_opcode(tx, opcode);
		ci_code_assign(tx, expr->ex_type,
				is_preop && context == EC_VALUE, expr_addrtype);
		addrtype = unknown_addrtype(tx);
		break;

	case OP_LOGICAL_NOT:
		ci_compile_expression(tx, expr, context);
		if (context == EC_VALUE)
			ci_code_opcode(tx, OC_LOGICAL_NOT);
		addrtype = AT_CI_ADDR;
		break;
	case OP_BITWISE_NOT:
		ci_compile_expression(tx, expr, context);
		if (context == EC_VALUE)
			ci_code_opcode(tx, OC_BITWISE_NOT);
		addrtype = AT_CI_ADDR;
		break;
	case OP_UNARY_MINUS:
		addrtype = ci_compile_expression(tx, expr, context);
		if (context == EC_VALUE) {
			switch (expr->ex_type->ty_code) {
#if WANT_LDBL
			case TY_LONGDOUBLE:
				opcode = OC_LONG_DOUBLE_UNARY_MINUS;
				break;
#endif
			case TY_DOUBLE:
				opcode = OC_DOUBLE_UNARY_MINUS;
				break;
			case TY_FLOAT:
				opcode = OC_FLOAT_UNARY_MINUS;
				break;
#if WANT_LL
			case TY_LONGLONG:
			case TY_ULONGLONG:
				opcode = OC_LONG_LONG_UNARY_MINUS;
				break;
#endif
			default:
				opcode = OC_UNARY_MINUS;
				break;
			}
			ci_code_opcode(tx, opcode);
		}
		break;
	case OP_UNARY_PLUS:
		addrtype = ci_compile_expression(tx, expr, context);
		break;
	default:
		ci_panic("bad operator in cue");
		addrtype = AT_CI_ADDR;	/* to satisfy gcc */
	}
	return addrtype;
}

static addrtype_t
compile_conditional_expr(tx, co, context)
text_t *tx;
cond_expr_t *co;
expr_context_t context;
{
	labeldesc_t false_label, end_label;
	addrtype_t left_at, right_at;
	taddr_t save_sp;

	compile_expr_for_jump(tx, co->co_cond);

	false_label = ci_code_jump_to_label(tx, OC_JUMP_IF_ZERO, NO_LABELDESC);
	save_sp = tx->tx_sp;
	left_at = ci_compile_expression(tx, co->co_if_true, context);
	end_label = ci_code_jump_to_label(tx, OC_JUMP, NO_LABELDESC);

	/*  If we get to here when executing, we didn't evaluate the
	 *  previous expression, so undo any effect of the expression
	 *  on the value of the sp as maintained by the static sp checker.
	 */
	tx->tx_sp = save_sp;

	ci_resolve_and_free_label(tx, false_label);
	right_at = ci_compile_expression(tx, co->co_if_false, context);
	ci_resolve_and_free_label(tx, end_label);
	if (left_at == AT_CI_ADDR && right_at == AT_CI_ADDR)
		return AT_CI_ADDR;
	else
		return unknown_addrtype(tx);
}

static addrtype_t
compile_conversion(tx, ue, context, expr)
text_t *tx;
unary_expr_t *ue;
expr_context_t context;
expr_t *expr;				/* Just for dumping proper error msg */
{
	addrtype_t addrtype;
	typecode_t typecode;

	addrtype = ci_compile_expression(tx, ue->ue_expr, context);

	if (context == EC_EFFECT)
		return addrtype;

	typecode = ue->ue_expr->ex_type->ty_code;

	return code_conversion(tx, addrtype, ue->ue_op, typecode, expr);
}

static addrtype_t
code_conversion(tx, addrtype, op, typecode, expr)
text_t *tx;
addrtype_t addrtype;
optype_t op;
typecode_t typecode;
expr_t *expr;				/* Just for dumping proper error msg */
{
	bool is_integral;
	const char *what = "";

	is_integral = ci_is_integral(typecode) ||
					typecode == TY_ENUM ||
					typecode == DT_PTR_TO ||
					typecode == DT_ARRAY_OF;

	/*  We treat OP_CVT_TO_PTR applied to a function type or an
	 *  array type as special cases. The first is caused by the
	 *  promotion of type  "function returning T" to "pointer to
	 *  function returning T".  The second is occurs when casting
	 *  an array to a pointer.  Neither generates any code.
	 *
	 */
	if (op == OP_CVT_TO_PTR &&
		     (typecode == DT_FUNC_RETURNING || typecode == DT_ARRAY_OF))
		return addrtype;

	switch (op) {
#if WANT_LDBL
	case OP_CVT_TO_LONG_DOUBLE:
		if (typecode == TY_DOUBLE)
			ci_code_opcode(tx, OC_CVT_DOUBLE_TO_LONG_DOUBLE);
		else if (typecode == TY_FLOAT)
			ci_code_opcode(tx, OC_CVT_FLOAT_TO_LONG_DOUBLE);
#if WANT_LL
		else if (typecode == TY_LONGLONG)
			ci_code_opcode(tx, OC_CVT_SIGNED_LONG_LONG_TO_LONG_DOUBLE);
		else if (typecode == TY_ULONGLONG)
			ci_code_opcode(tx, OC_CVT_UNSIGNED_LONG_LONG_TO_LONG_DOUBLE);
#endif
		else if (is_integral) {
			if (ci_is_signed_type(typecode)) {
				ci_code_opcode(tx,
					       OC_CVT_SIGNED_LONG_TO_LONG_DOUBLE);
			}
			else {
				ci_code_opcode(tx,
					       OC_CVT_UNSIGNED_LONG_TO_LONG_DOUBLE);
			}
		}
		else if (typecode != TY_LONGDOUBLE)
			ci_panic("bad type in ctd");
		break;
#endif
	case OP_CVT_TO_DOUBLE:
		if (typecode == TY_FLOAT)
			ci_code_opcode(tx, OC_CVT_FLOAT_TO_DOUBLE);
#if WANT_LDBL
		else if (typecode == TY_LONGDOUBLE)
			ci_code_opcode(tx, OC_CVT_LONG_DOUBLE_TO_DOUBLE);
#endif
#if WANT_LL
		else if (typecode == TY_LONGLONG)
			ci_code_opcode(tx, OC_CVT_SIGNED_LONG_LONG_TO_DOUBLE);
		else if (typecode == TY_ULONGLONG)
			ci_code_opcode(tx, OC_CVT_UNSIGNED_LONG_LONG_TO_DOUBLE);
#endif
		else if (is_integral) {
			if (ci_is_signed_type(typecode)) {
				ci_code_opcode(tx,
					       OC_CVT_SIGNED_LONG_TO_DOUBLE);
			}
			else {
				ci_code_opcode(tx,
					       OC_CVT_UNSIGNED_LONG_TO_DOUBLE);
			}
		}
		else if (typecode != TY_DOUBLE)
			ci_panic("bad type in ctd");
		break;
	case OP_CVT_TO_FLOAT:
		if (typecode == TY_DOUBLE)
			ci_code_opcode(tx, OC_CVT_DOUBLE_TO_FLOAT);
#if WANT_LDBL
		else if (typecode == TY_LONGDOUBLE)
			ci_code_opcode(tx, OC_CVT_LONG_DOUBLE_TO_FLOAT);
#endif
#if WANT_LL
		else if (typecode == TY_LONGLONG)
			ci_code_opcode(tx, OC_CVT_SIGNED_LONG_LONG_TO_FLOAT);
		else if (typecode == TY_ULONGLONG)
			ci_code_opcode(tx, OC_CVT_UNSIGNED_LONG_LONG_TO_FLOAT);
#endif
		else if (is_integral) {
			if (ci_is_signed_type(typecode)) {
				ci_code_opcode(tx,
					       OC_CVT_SIGNED_LONG_TO_FLOAT);
			}
			else {
				ci_code_opcode(tx,
					       OC_CVT_UNSIGNED_LONG_TO_FLOAT);
			}
		}
		else if (typecode != TY_FLOAT)
			ci_panic("bad type in ctf");
		break;
#if WANT_LL
	case OP_CVT_TO_LONG_LONG:
	case OP_CVT_TO_ULONG_LONG:
		if (typecode == TY_FLOAT)
			ci_code_opcode(tx, OC_CVT_FLOAT_TO_LONG_LONG);
		else if (typecode == TY_DOUBLE)
			ci_code_opcode(tx, OC_CVT_DOUBLE_TO_LONG_LONG);
#if WANT_LDBL
		else if (typecode == TY_LONGDOUBLE)
			ci_code_opcode(tx, OC_CVT_LONG_DOUBLE_TO_LONG_LONG);
#endif
		else if (!is_integral)
			ci_panic("bad type in ctll");
		else if (typecode != TY_LONGLONG && typecode != TY_ULONGLONG) {
			if (ci_is_signed_type(typecode)) {
				ci_code_opcode(tx,
					       OC_CVT_SIGNED_LONG_TO_LONG_LONG);
			}
			else {
				ci_code_opcode(tx,
					       OC_CVT_UNSIGNED_LONG_TO_LONG_LONG);
			}
		}
		break;
#endif
	case OP_CVT_TO_PTR:
	case OP_CVT_TO_LONG:
	case OP_CVT_TO_ULONG:
	case OP_CVT_TO_INT:
	case OP_CVT_TO_UINT:

	/* Dibyendu : 'what' set for showing better error message */
	case OP_CVT_TO_SHORT: 	what = "short"; 		goto doit;
	case OP_CVT_TO_USHORT: 	what = "unsigned short"; 	goto doit;
	case OP_CVT_TO_CHAR: 	what = "char"; 			goto doit;
	case OP_CVT_TO_UCHAR: 	what = "unsigned char";
doit:
		/*  If we are converting a non-pointer to a pointer then
		 *  we don't know the address type of the resulting pointer.
		 */
		if (op == OP_CVT_TO_PTR && typecode != DT_PTR_TO)
			addrtype = unknown_addrtype(tx);

		/* BUG: we don't cope with the sub-word types.  All we do
		 *      is make sure the result is integral.
		 */
		if (typecode == TY_FLOAT)
			ci_code_opcode(tx, OC_CVT_FLOAT_TO_LONG);
		else if (typecode == TY_DOUBLE)
			ci_code_opcode(tx, OC_CVT_DOUBLE_TO_LONG);
#if WANT_LDBL
		else if (typecode == TY_LONGDOUBLE)
			ci_code_opcode(tx, OC_CVT_LONG_DOUBLE_TO_LONG);
#endif
#if WANT_LL
		else if (typecode == TY_LONGLONG)
			ci_code_opcode(tx, OC_CVT_SIGNED_LONG_LONG_TO_LONG);
		else if (typecode == TY_ULONGLONG)
			ci_code_opcode(tx, OC_CVT_UNSIGNED_LONG_LONG_TO_LONG);
#endif
		else if (is_integral &&
			(op == OP_CVT_TO_SHORT || op == OP_CVT_TO_USHORT ||
			 op == OP_CVT_TO_CHAR  || op == OP_CVT_TO_UCHAR) &&
			 expr != NULL) {
		    /* Dibyendu : 26/12/98 : Warning added because this can
		     * change the meaning of an expression.
		     */
			diagf(ET_WARNING, expr->ex_lexinfo,
				"Conversion to `%s' ignored", what);
			ci_show_expr_and_type(expr, expr->ex_type);
		}
		else if (!is_integral)
			ci_panic("bad type in cti");
		break;
	case OP_CVT_TO_VOID:
		/*  The compiler should not have allowed a void expression
		 *  in a value context.
		 */
		ci_panic("got CTV in a value context");
	default:
		ci_panic("unknown cvt op in ct");
	}

	return addrtype;
}

addrtype_t
ci_compile_expression(tx, expr, context)
text_t *tx;
expr_t *expr;
expr_context_t context;
{
	taddr_t expected_sp;
	addrtype_t addrtype;

	switch (context) {
	case EC_ADDRESS:
		switch(expr->ex_exprtype) {
		case ET_VAR:
		case ET_DOT:
		case ET_UNARY:
		case ET_FUNCNAME:
			expected_sp = tx->tx_sp + sizeof(stackword_t);
			break;
		case ET_FUNC_CALL:
			if (expr->ex_type->ty_code != TY_STRUCT &&
						expr->ex_type->ty_code != TY_UNION)
				ci_panic("func call in address context");
			context = EC_VALUE;
			expected_sp = tx->tx_sp;
			break;
		case ET_CONDITIONAL:
		case ET_BINARY:
			expected_sp = tx->tx_sp + sizeof(stackword_t);
			break;
		default:
			ci_panic("illegal expr type for address context");
			expected_sp = 0;
		}
		break;
	case EC_VALUE:
		if (!expr->ex_type)
		{
			ci_panic("no type in value context");
			expected_sp = 0;
		}
		else
		switch(expr->ex_type->ty_code) {
		case TY_FLOAT:
			expected_sp = tx->tx_sp + FLOAT_NSLOTS * sizeof(stackword_t);
			break;
		case TY_DOUBLE:
			expected_sp = tx->tx_sp + DOUBLE_NSLOTS*sizeof(stackword_t);
			break;
#if WANT_LDBL
		case TY_LONGDOUBLE:
			expected_sp = tx->tx_sp + LDOUBLE_NSLOTS*sizeof(stackword_t);
			break;
#endif
#if WANT_LL
		case TY_LONGLONG:
		case TY_ULONGLONG:
			expected_sp = tx->tx_sp + LONGLONG_NSLOTS*sizeof(stackword_t);
			break;
#endif
		case TY_STRUCT:
		case TY_UNION:
			ci_panic("struct in value context");
			expected_sp = 0;	/* to satisfy gcc */
			break;
		default:
			expected_sp = tx->tx_sp + sizeof(stackword_t);
			break;
		}
		break;
	case EC_EFFECT:
		expected_sp = tx->tx_sp;
		break;
	default:
		ci_panic("bad context in ce");
		expected_sp = 0; /* to satisfy gcc */
	}

	/*  BUG: we should fold floating point constant expressions as well.
	 */
	if (expr->ex_is_constant && (expr->ex_type->ty_code == TY_ENUM ||
				     ci_is_integral(expr->ex_type->ty_code))) {
		if (context == EC_VALUE) {
			constval_t val;

			ci_evaluate_constant_expression(expr, "expression",
									TRUE, &val);

			switch (expr->ex_type->ty_code)
			{
#if WANT_LL
			case TY_LONGLONG:
			case TY_ULONGLONG:
				ci_code_constpush(tx, ((long *)&val.cv_long_long)[1]);
				ci_code_constpush(tx, ((long *)&val.cv_long_long)[0]);
				break;
#endif
			default:
				ci_code_constpush(tx, val.cv_int);
				break;
			}
		}
		if (tx->tx_sp != expected_sp)
			ci_panic("sp botch in ce");
		return AT_CI_ADDR;
	}

	switch(expr->ex_exprtype) {
	case ET_CAST:
		addrtype = compile_conversion(tx, expr->ex_unary_expr, context, expr);
		break;
	case ET_PROMOTION:
	case ET_ASSIGNMENT_CONVERSION:
		addrtype = compile_conversion(tx, expr->ex_unary_expr, context, NULL);
		break;
	case ET_VAR:
		addrtype = ci_compile_var_reference(tx,
						       expr->ex_var,
						       expr->ex_lexinfo,
						       context);
		break;
	case ET_FUNCNAME:
		if (context == EC_VALUE || context == EC_ADDRESS) {
			int func_index;

			func_index = ci_funcname_to_index(tx,
							 expr->ex_lexinfo,
							 expr->ex_var->va_name);

			ci_code_opcode(tx, OC_CONSTPUSH_L);
			ci_add_func_reloc(tx, RT_TEXTLOC, tx->tx_pc);
			ci_code_long(tx, short_form(func_index));
		}
		addrtype = AT_CI_ADDR;
		break;
	case ET_FUNC_CALL:
		compile_func_call_expr(tx, expr->ex_func_call_expr,
				       expr->ex_type ?	expr->ex_type->ty_code :
				       TY_INT, context);
		addrtype = AT_CI_ADDR;
		break;
	case ET_MULTI_ARROW:
		addrtype = compile_multi_arrow_expr(tx,
					expr->ex_multi_arrow_expr, context);
		break;
	case ET_DOT:
		addrtype = compile_dot_expr(tx, expr->ex_dot_expr, context);
		break;
	case ET_FLOATING_CONST:
		if (context == EC_VALUE) {
			unsigned char buf[DOUBLE_NBYTES];
			int i, count;

			if (expr->ex_type->ty_code == TY_FLOAT) {
				float fval;

				fval = expr->ex_floating_constant_val;
				count = FLOAT_NBYTES;
				memcpy(buf, (char *)&fval, sizeof(fval));
				ci_code_opcode(tx, OC_PUSH_FLOAT_CONST);
			}
			else if (expr->ex_type->ty_code == TY_DOUBLE) {
				double dval;

				dval = expr->ex_floating_constant_val;
				count = DOUBLE_NBYTES;
				memcpy(buf, (char *)&dval, sizeof(dval));
				ci_code_opcode(tx, OC_PUSH_DOUBLE_CONST);
			}
#if WANT_LDBL
			else if (expr->ex_type->ty_code == TY_LONGDOUBLE) {
				long_double_t dval;

				dval = expr->ex_floating_constant_val;
				count = LDOUBLE_NBYTES;
				memcpy(buf, (char *)&dval, sizeof(dval));
				ci_code_opcode(tx, OC_PUSH_DOUBLE_CONST);
			}
#endif
			else {
				count = 0;	/* to satisfy gcc */
				ci_panic("non fp type in cce");
			}

			for (i = 0; i < count; ++i)
				ci_code_byte(tx, (long)buf[i]);
		}
		addrtype = AT_CI_ADDR;
		break;
	case ET_STRING_CONST:
		if (context == EC_VALUE) {
			ci_code_opcode(tx, OC_CONSTPUSH_L);
			ci_add_string_reloc(tx, RT_TEXTLOC, tx->tx_pc,
					expr->ex_string_constant_val, (long)0);
			ci_code_long(tx, MAX_WORD + 1);
		}
		addrtype = AT_CI_ADDR;
		break;
	case ET_SCALE:
		compile_scale_expr(tx, expr->ex_scale_expr, context);
		addrtype = AT_CI_ADDR;
		break;
	case ET_BINARY:
		addrtype = compile_binary_expr(tx, expr->ex_binary_expr, context);
		break;
	case ET_UNARY:
		addrtype = compile_unary_expr(tx, expr->ex_unary_expr, context);
		break;
	case ET_CONDITIONAL:
		addrtype = compile_conditional_expr(tx,
						       expr->ex_cond_expr,
						       context);
		break;
	default:
		ci_panic("bad expression type in ce");
		addrtype = AT_CI_ADDR;	/* to satisfy gcc */
	}

	if (tx->tx_sp != expected_sp)
		ci_panic("sp botch in ce");

	return addrtype;
}
