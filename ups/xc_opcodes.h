/* xc_opcodes.h - definition of opcodes for the C interpreter machine */

/*  Copyright 1993 Mark Russell, University of Kent at Canterbury.
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


/* @(#)xc_opcodes.h	1.2 16 Apr 1994 (UKC) */

/*  See ci_execute.c for the meanings of the individual opcodes.
 *
 *  Warning: the code generation generates byte, word and long forms
 *           of some opcodes by arithmetic on these enum values.
 *	     The order of opcodes that are grouped between --- comments
 *	     is significant, and no new opcodes should be inserted into
 *	     such groups.
 */

#ifdef WANT_LABADDRS
#	define x(oc, se, os)	&&oc
#else
#ifdef WANT_OPINFO
#	define STACKW_SIZE		((int)sizeof(stackword_t))
#	ifdef __STDC__
#		define x(oc, se, os)	{ #oc, se, os }
#	else
#		define x(oc, se, os)	{ "oc", se, os }
#	endif
#else
#	define WANT_ENUM
#	define x(oc, se, os)	oc
	enum opcode_e {
#endif
#endif
/* ----------------------- */
	x(OC_CALL_B, 0, OS_OTHER),
	x(OC_CALL_W, 0, OS_OTHER),
	x(OC_CALL_L, 0, OS_OTHER),
	x(OC_CALL_Q, 0, OS_OTHER),
/* ----------------------- */
	x(OC_POPMANY_B, 0, OS_BYTE),
	x(OC_POPMANY_W, 0, OS_WORD),
	x(OC_POPMANY_L, 0, OS_LONG),
	x(OC_POPMANY_Q, 0, OS_QUAD),
/* ----------------------- */
	x(OC_LINK_B, 0, OS_OTHER),
	x(OC_LINK_W, 0, OS_OTHER),
	x(OC_LINK_L, 0, OS_OTHER),
	x(OC_LINK_Q, 0, OS_OTHER),
/* ----------------------- */
	x(OC_PROC_MEMCPY_B, -(2 * STACKW_SIZE), OS_BYTE),
	x(OC_PROC_MEMCPY_W, -(2 * STACKW_SIZE), OS_WORD),
	x(OC_PROC_MEMCPY_L, -(2 * STACKW_SIZE), OS_LONG),
	x(OC_PROC_MEMCPY_Q, -(2 * STACKW_SIZE), OS_QUAD),
/* ----------------------- */
	x(OC_MEMCPY_B, -(2 * STACKW_SIZE), OS_BYTE),
	x(OC_MEMCPY_W, -(2 * STACKW_SIZE), OS_WORD),
	x(OC_MEMCPY_L, -(2 * STACKW_SIZE), OS_LONG),
	x(OC_MEMCPY_Q, -(2 * STACKW_SIZE), OS_QUAD),
/* ----------------------- */
	x(OC_PUSH_BYTES, 0, OS_LONG),
	x(OC_PROC_PUSH_BYTES, 0, OS_LONG),
	x(OC_RESERVE_BYTES, 0, OS_LONG),
/* ----------------------- */
#if WANT_LL
	x(OC_PROC_ASSIGN_LONG_LONG, -(STACKW_SIZE + LONGLONG_NBYTES), OS_ZERO),
#endif
	x(OC_PROC_ASSIGN_BYTE, -(2 * STACKW_SIZE), OS_ZERO),
	x(OC_PROC_ASSIGN_WORD, -(2 * STACKW_SIZE), OS_ZERO),
	x(OC_PROC_ASSIGN_LONG, -(2 * STACKW_SIZE), OS_ZERO),
	x(OC_PROC_ASSIGN_FLOAT, -(STACKW_SIZE + FLOAT_NBYTES), OS_ZERO),
	x(OC_PROC_ASSIGN_DOUBLE, -(STACKW_SIZE + DOUBLE_NBYTES), OS_ZERO),
#if WANT_LDBL
	x(OC_PROC_ASSIGN_LONG_DOUBLE, -(STACKW_SIZE + LDOUBLE_NBYTES), OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_ASSIGN_LONG_LONG, -(STACKW_SIZE + LONGLONG_NBYTES), OS_ZERO),
#endif
	x(OC_ASSIGN_BYTE, -(2 * STACKW_SIZE), OS_ZERO),
	x(OC_ASSIGN_WORD, -(2 * STACKW_SIZE), OS_ZERO),
	x(OC_ASSIGN_LONG, -(2 * STACKW_SIZE), OS_ZERO),
	x(OC_ASSIGN_FLOAT, -(STACKW_SIZE + FLOAT_NBYTES), OS_ZERO),
	x(OC_ASSIGN_DOUBLE, -(STACKW_SIZE + DOUBLE_NBYTES), OS_ZERO),
#if WANT_LDBL
	x(OC_ASSIGN_LONG_DOUBLE, -(STACKW_SIZE + LDOUBLE_NBYTES), OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_PROC_ASSIGN_AND_PUSH_LONG_LONG, -STACKW_SIZE, OS_ZERO),
#endif
	x(OC_PROC_ASSIGN_AND_PUSH_BYTE, -STACKW_SIZE, OS_ZERO),
	x(OC_PROC_ASSIGN_AND_PUSH_WORD, -STACKW_SIZE, OS_ZERO),
	x(OC_PROC_ASSIGN_AND_PUSH_LONG, -STACKW_SIZE, OS_ZERO),
	x(OC_PROC_ASSIGN_AND_PUSH_FLOAT, -STACKW_SIZE, OS_ZERO),
	x(OC_PROC_ASSIGN_AND_PUSH_DOUBLE, -STACKW_SIZE, OS_ZERO),
#if WANT_LDBL
	x(OC_PROC_ASSIGN_AND_PUSH_LONG_DOUBLE, -STACKW_SIZE, OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_ASSIGN_AND_PUSH_LONG_LONG, -STACKW_SIZE, OS_ZERO),
#endif
	x(OC_ASSIGN_AND_PUSH_BYTE, -STACKW_SIZE, OS_ZERO),
	x(OC_ASSIGN_AND_PUSH_WORD, -STACKW_SIZE, OS_ZERO),
	x(OC_ASSIGN_AND_PUSH_LONG, -STACKW_SIZE, OS_ZERO),
	x(OC_ASSIGN_AND_PUSH_FLOAT, -STACKW_SIZE, OS_ZERO),
	x(OC_ASSIGN_AND_PUSH_DOUBLE, -STACKW_SIZE, OS_ZERO),
#if WANT_LDBL
	x(OC_ASSIGN_AND_PUSH_LONG_DOUBLE, -STACKW_SIZE, OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_PROC_DEREF_SIGNED_LONG_LONG, LONGLONG_NBYTES - STACKW_SIZE, OS_ZERO),
#endif
	x(OC_PROC_DEREF_SIGNED_BYTE, 0, OS_ZERO),
	x(OC_PROC_DEREF_SIGNED_WORD, 0, OS_ZERO),
	x(OC_PROC_DEREF_SIGNED_LONG, 0, OS_ZERO),
	x(OC_PROC_DEREF_FLOAT, FLOAT_NBYTES - STACKW_SIZE, OS_ZERO),
	x(OC_PROC_DEREF_DOUBLE, DOUBLE_NBYTES - STACKW_SIZE, OS_ZERO),
#if WANT_LDBL
	x(OC_PROC_DEREF_LONG_DOUBLE, LDOUBLE_NBYTES - STACKW_SIZE, OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_PROC_DEREF_UNSIGNED_LONG_LONG, LONGLONG_NBYTES - STACKW_SIZE, OS_ZERO),
#endif
	x(OC_PROC_DEREF_UNSIGNED_BYTE, 0, OS_ZERO),
	x(OC_PROC_DEREF_UNSIGNED_WORD, 0, OS_ZERO),
	x(OC_PROC_DEREF_UNSIGNED_LONG, 0, OS_ZERO),
/* ----------------------- */
#if WANT_LL
	x(OC_DEREF_SIGNED_LONG_LONG, LONGLONG_NBYTES - STACKW_SIZE, OS_ZERO),
#endif
	x(OC_DEREF_SIGNED_BYTE, 0, OS_ZERO),
	x(OC_DEREF_SIGNED_WORD, 0, OS_ZERO),
	x(OC_DEREF_SIGNED_LONG, 0, OS_ZERO),
	x(OC_DEREF_FLOAT, FLOAT_NBYTES - STACKW_SIZE, OS_ZERO),
	x(OC_DEREF_DOUBLE, DOUBLE_NBYTES - STACKW_SIZE, OS_ZERO),
#if WANT_LDBL
	x(OC_DEREF_LONG_DOUBLE, LDOUBLE_NBYTES - STACKW_SIZE, OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_DEREF_UNSIGNED_LONG_LONG, LONGLONG_NBYTES - STACKW_SIZE, OS_ZERO),
#endif
	x(OC_DEREF_UNSIGNED_BYTE, 0, OS_ZERO),
	x(OC_DEREF_UNSIGNED_WORD, 0, OS_ZERO),
	x(OC_DEREF_UNSIGNED_LONG, 0, OS_ZERO),
/* ----------------------- */
	x(OC_NEG_CONSTPUSH_B, STACKW_SIZE, OS_BYTE),
	x(OC_NEG_CONSTPUSH_W, STACKW_SIZE, OS_WORD),
	x(OC_NEG_CONSTPUSH_L, STACKW_SIZE, OS_LONG),
	x(OC_NEG_CONSTPUSH_Q, STACKW_SIZE, OS_QUAD),
/* ----------------------- */
	x(OC_CONSTPUSH_B, STACKW_SIZE, OS_BYTE),
	x(OC_CONSTPUSH_W, STACKW_SIZE, OS_WORD),
	x(OC_CONSTPUSH_L, STACKW_SIZE, OS_LONG),
	x(OC_CONSTPUSH_Q, STACKW_SIZE, OS_QUAD),
/* ----------------------- */
	x(OC_PUSH_FLOAT_CONST, FLOAT_NBYTES, OS_OTHER),
	x(OC_PUSH_DOUBLE_CONST, DOUBLE_NBYTES, OS_OTHER),
/* ----------------------- */
	x(OC_CHECK_SP_B, 0, OS_BYTE),
	x(OC_CHECK_SP_W, 0, OS_WORD),
	x(OC_CHECK_SP_L, 0, OS_LONG),
	x(OC_CHECK_SP_Q, 0, OS_QUAD),

/* ----------------------- */
	x(OC_PROC_PUSH_FP_ADDR_B, STACKW_SIZE, OS_BYTE),
	x(OC_PROC_PUSH_FP_ADDR_W, STACKW_SIZE, OS_WORD),
	x(OC_PROC_PUSH_FP_ADDR_L, STACKW_SIZE, OS_LONG),
	x(OC_PROC_PUSH_FP_ADDR_Q, STACKW_SIZE, OS_QUAD),
/* ----------------------- */
	x(OC_PROC_PUSH_AP_ADDR_B, STACKW_SIZE, OS_BYTE),
	x(OC_PROC_PUSH_AP_ADDR_W, STACKW_SIZE, OS_WORD),
	x(OC_PROC_PUSH_AP_ADDR_L, STACKW_SIZE, OS_LONG),
	x(OC_PROC_PUSH_AP_ADDR_Q, STACKW_SIZE, OS_QUAD),
/* ----------------------- */
	x(OC_PROC_PUSH_CFA_ADDR_B, STACKW_SIZE, OS_BYTE),
	x(OC_PROC_PUSH_CFA_ADDR_W, STACKW_SIZE, OS_WORD),
	x(OC_PROC_PUSH_CFA_ADDR_L, STACKW_SIZE, OS_LONG),
	x(OC_PROC_PUSH_CFA_ADDR_Q, STACKW_SIZE, OS_QUAD),
/* ----------------------- */
	x(OC_PUSH_STACKADDR_B, STACKW_SIZE, OS_BYTE),
	x(OC_PUSH_STACKADDR_W, STACKW_SIZE, OS_WORD),
	x(OC_PUSH_STACKADDR_L, STACKW_SIZE, OS_LONG),
	x(OC_PUSH_STACKADDR_Q, STACKW_SIZE, OS_QUAD),
/* ----------------------- */

/* ----------------------- */
#if WANT_LL
	x(OC_PUSH_SIGNED_LONG_LONG_AT_STACKADDR_B, LONGLONG_NBYTES, OS_BYTE),
	x(OC_PUSH_SIGNED_LONG_LONG_AT_STACKADDR_W, LONGLONG_NBYTES, OS_WORD),
	x(OC_PUSH_SIGNED_LONG_LONG_AT_STACKADDR_L, LONGLONG_NBYTES, OS_LONG),
	x(OC_PUSH_SIGNED_LONG_LONG_AT_STACKADDR_Q, LONGLONG_NBYTES, OS_QUAD),
#endif

	x(OC_PUSH_SIGNED_BYTE_AT_STACKADDR_B, STACKW_SIZE, OS_BYTE),
	x(OC_PUSH_SIGNED_BYTE_AT_STACKADDR_W, STACKW_SIZE, OS_WORD),
	x(OC_PUSH_SIGNED_BYTE_AT_STACKADDR_L, STACKW_SIZE, OS_LONG),
	x(OC_PUSH_SIGNED_BYTE_AT_STACKADDR_Q, STACKW_SIZE, OS_QUAD),

	x(OC_PUSH_SIGNED_SHORT_AT_STACKADDR_B, STACKW_SIZE, OS_BYTE),
	x(OC_PUSH_SIGNED_SHORT_AT_STACKADDR_W, STACKW_SIZE, OS_WORD),
	x(OC_PUSH_SIGNED_SHORT_AT_STACKADDR_L, STACKW_SIZE, OS_LONG),
	x(OC_PUSH_SIGNED_SHORT_AT_STACKADDR_Q, STACKW_SIZE, OS_QUAD),

	x(OC_PUSH_SIGNED_LONG_AT_STACKADDR_B, STACKW_SIZE, OS_BYTE),
	x(OC_PUSH_SIGNED_LONG_AT_STACKADDR_W, STACKW_SIZE, OS_WORD),
	x(OC_PUSH_SIGNED_LONG_AT_STACKADDR_L, STACKW_SIZE, OS_LONG),
	x(OC_PUSH_SIGNED_LONG_AT_STACKADDR_Q, STACKW_SIZE, OS_QUAD),

	x(OC_PUSH_FLOAT_AT_STACKADDR_B, FLOAT_NBYTES, OS_BYTE),
	x(OC_PUSH_FLOAT_AT_STACKADDR_W, FLOAT_NBYTES, OS_WORD),
	x(OC_PUSH_FLOAT_AT_STACKADDR_L, FLOAT_NBYTES, OS_LONG),
	x(OC_PUSH_FLOAT_AT_STACKADDR_Q, FLOAT_NBYTES, OS_QUAD),

	x(OC_PUSH_DOUBLE_AT_STACKADDR_B, DOUBLE_NBYTES, OS_BYTE),
	x(OC_PUSH_DOUBLE_AT_STACKADDR_W, DOUBLE_NBYTES, OS_WORD),
	x(OC_PUSH_DOUBLE_AT_STACKADDR_L, DOUBLE_NBYTES, OS_LONG),
	x(OC_PUSH_DOUBLE_AT_STACKADDR_Q, DOUBLE_NBYTES, OS_QUAD),

#if WANT_LDBL
	x(OC_PUSH_LONG_DOUBLE_AT_STACKADDR_B, LDOUBLE_NBYTES, OS_BYTE),
	x(OC_PUSH_LONG_DOUBLE_AT_STACKADDR_W, LDOUBLE_NBYTES, OS_WORD),
	x(OC_PUSH_LONG_DOUBLE_AT_STACKADDR_L, LDOUBLE_NBYTES, OS_LONG),
	x(OC_PUSH_LONG_DOUBLE_AT_STACKADDR_Q, LDOUBLE_NBYTES, OS_QUAD),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_PUSH_UNSIGNED_LONG_LONG_AT_STACKADDR_B, LONGLONG_NBYTES, OS_BYTE),
	x(OC_PUSH_UNSIGNED_LONG_LONG_AT_STACKADDR_W, LONGLONG_NBYTES, OS_WORD),
	x(OC_PUSH_UNSIGNED_LONG_LONG_AT_STACKADDR_L, LONGLONG_NBYTES, OS_LONG),
	x(OC_PUSH_UNSIGNED_LONG_LONG_AT_STACKADDR_Q, LONGLONG_NBYTES, OS_QUAD),
#endif

	x(OC_PUSH_UNSIGNED_BYTE_AT_STACKADDR_B, STACKW_SIZE, OS_BYTE),
	x(OC_PUSH_UNSIGNED_BYTE_AT_STACKADDR_W, STACKW_SIZE, OS_WORD),
	x(OC_PUSH_UNSIGNED_BYTE_AT_STACKADDR_L, STACKW_SIZE, OS_LONG),
	x(OC_PUSH_UNSIGNED_BYTE_AT_STACKADDR_Q, STACKW_SIZE, OS_QUAD),

	x(OC_PUSH_UNSIGNED_SHORT_AT_STACKADDR_B, STACKW_SIZE, OS_BYTE),
	x(OC_PUSH_UNSIGNED_SHORT_AT_STACKADDR_W, STACKW_SIZE, OS_WORD),
	x(OC_PUSH_UNSIGNED_SHORT_AT_STACKADDR_L, STACKW_SIZE, OS_LONG),
	x(OC_PUSH_UNSIGNED_SHORT_AT_STACKADDR_Q, STACKW_SIZE, OS_QUAD),

	x(OC_PUSH_UNSIGNED_LONG_AT_STACKADDR_B, STACKW_SIZE, OS_BYTE),
	x(OC_PUSH_UNSIGNED_LONG_AT_STACKADDR_W, STACKW_SIZE, OS_WORD),
	x(OC_PUSH_UNSIGNED_LONG_AT_STACKADDR_L, STACKW_SIZE, OS_LONG),
	x(OC_PUSH_UNSIGNED_LONG_AT_STACKADDR_Q, STACKW_SIZE, OS_QUAD),
/* ----------------------- */

/* ----------------------- */
#if WANT_LL
	x(OC_PUSH_UNSIGNED_LONG_LONG_AT_ADDR_B, LONGLONG_NBYTES, OS_BYTE),
	x(OC_PUSH_UNSIGNED_LONG_LONG_AT_ADDR_W, LONGLONG_NBYTES, OS_WORD),
	x(OC_PUSH_UNSIGNED_LONG_LONG_AT_ADDR_L, LONGLONG_NBYTES, OS_LONG),
	x(OC_PUSH_UNSIGNED_LONG_LONG_AT_ADDR_Q, LONGLONG_NBYTES, OS_QUAD),
#endif

	x(OC_PUSH_UNSIGNED_BYTE_AT_ADDR_B, STACKW_SIZE, OS_BYTE),
	x(OC_PUSH_UNSIGNED_BYTE_AT_ADDR_W, STACKW_SIZE, OS_WORD),
	x(OC_PUSH_UNSIGNED_BYTE_AT_ADDR_L, STACKW_SIZE, OS_LONG),
	x(OC_PUSH_UNSIGNED_BYTE_AT_ADDR_Q, STACKW_SIZE, OS_QUAD),

	x(OC_PUSH_UNSIGNED_SHORT_AT_ADDR_B, STACKW_SIZE, OS_BYTE),
	x(OC_PUSH_UNSIGNED_SHORT_AT_ADDR_W, STACKW_SIZE, OS_WORD),
	x(OC_PUSH_UNSIGNED_SHORT_AT_ADDR_L, STACKW_SIZE, OS_LONG),
	x(OC_PUSH_UNSIGNED_SHORT_AT_ADDR_Q, STACKW_SIZE, OS_QUAD),

	x(OC_PUSH_UNSIGNED_LONG_AT_ADDR_B, STACKW_SIZE, OS_BYTE),
	x(OC_PUSH_UNSIGNED_LONG_AT_ADDR_W, STACKW_SIZE, OS_WORD),
	x(OC_PUSH_UNSIGNED_LONG_AT_ADDR_L, STACKW_SIZE, OS_LONG),
	x(OC_PUSH_UNSIGNED_LONG_AT_ADDR_Q, STACKW_SIZE, OS_QUAD),
/* ----------------------- */
#if WANT_LL
	x(OC_PUSH_SIGNED_LONG_LONG_AT_ADDR_B, LONGLONG_NBYTES, OS_BYTE),
	x(OC_PUSH_SIGNED_LONG_LONG_AT_ADDR_W, LONGLONG_NBYTES, OS_WORD),
	x(OC_PUSH_SIGNED_LONG_LONG_AT_ADDR_L, LONGLONG_NBYTES, OS_LONG),
	x(OC_PUSH_SIGNED_LONG_LONG_AT_ADDR_Q, LONGLONG_NBYTES, OS_QUAD),
#endif

	x(OC_PUSH_SIGNED_BYTE_AT_ADDR_B, STACKW_SIZE, OS_BYTE),
	x(OC_PUSH_SIGNED_BYTE_AT_ADDR_W, STACKW_SIZE, OS_WORD),
	x(OC_PUSH_SIGNED_BYTE_AT_ADDR_L, STACKW_SIZE, OS_LONG),
	x(OC_PUSH_SIGNED_BYTE_AT_ADDR_Q, STACKW_SIZE, OS_QUAD),

	x(OC_PUSH_SIGNED_SHORT_AT_ADDR_B, STACKW_SIZE, OS_BYTE),
	x(OC_PUSH_SIGNED_SHORT_AT_ADDR_W, STACKW_SIZE, OS_WORD),
	x(OC_PUSH_SIGNED_SHORT_AT_ADDR_L, STACKW_SIZE, OS_LONG),
	x(OC_PUSH_SIGNED_SHORT_AT_ADDR_Q, STACKW_SIZE, OS_QUAD),

	x(OC_PUSH_SIGNED_LONG_AT_ADDR_B, STACKW_SIZE, OS_BYTE),
	x(OC_PUSH_SIGNED_LONG_AT_ADDR_W, STACKW_SIZE, OS_WORD),
	x(OC_PUSH_SIGNED_LONG_AT_ADDR_L, STACKW_SIZE, OS_LONG),
	x(OC_PUSH_SIGNED_LONG_AT_ADDR_Q, STACKW_SIZE, OS_QUAD),

	x(OC_PUSH_FLOAT_AT_ADDR_B, FLOAT_NBYTES, OS_BYTE),
	x(OC_PUSH_FLOAT_AT_ADDR_W, FLOAT_NBYTES, OS_WORD),
	x(OC_PUSH_FLOAT_AT_ADDR_L, FLOAT_NBYTES, OS_LONG),
	x(OC_PUSH_FLOAT_AT_ADDR_Q, FLOAT_NBYTES, OS_QUAD),

	x(OC_PUSH_DOUBLE_AT_ADDR_B, DOUBLE_NBYTES, OS_BYTE),
	x(OC_PUSH_DOUBLE_AT_ADDR_W, DOUBLE_NBYTES, OS_WORD),
	x(OC_PUSH_DOUBLE_AT_ADDR_L, DOUBLE_NBYTES, OS_LONG),
	x(OC_PUSH_DOUBLE_AT_ADDR_Q, DOUBLE_NBYTES, OS_QUAD),

#if WANT_LDBL
	x(OC_PUSH_LONG_DOUBLE_AT_ADDR_B, LDOUBLE_NBYTES, OS_BYTE),
	x(OC_PUSH_LONG_DOUBLE_AT_ADDR_W, LDOUBLE_NBYTES, OS_WORD),
	x(OC_PUSH_LONG_DOUBLE_AT_ADDR_L, LDOUBLE_NBYTES, OS_LONG),
	x(OC_PUSH_LONG_DOUBLE_AT_ADDR_Q, LDOUBLE_NBYTES, OS_QUAD),
#endif
/* ----------------------- */

/* ----------------------- */
	x(OC_INSERT_SIGNED_BITFIELD, -STACKW_SIZE, OS_OTHER),
	x(OC_INSERT_UNSIGNED_BITFIELD, -STACKW_SIZE, OS_OTHER),
/* ----------------------- */
	x(OC_EXTRACT_SIGNED_BITFIELD, 0, OS_OTHER),
	x(OC_EXTRACT_UNSIGNED_BITFIELD, 0, OS_OTHER),
/* ----------------------- */

	x(OC_RET, 0, OS_ZERO),
	x(OC_RET_WORD, -STACKW_SIZE, OS_ZERO),
#if WANT_LL
	x(OC_RET_LONG_LONG, -(LONGLONG_NBYTES), OS_ZERO),
#endif
	x(OC_RET_FLOAT, -(FLOAT_NBYTES), OS_ZERO),
	x(OC_RET_DOUBLE, -(DOUBLE_NBYTES), OS_ZERO),
#if WANT_LDBL
	x(OC_RET_LONG_DOUBLE, -(LDOUBLE_NBYTES), OS_ZERO),
#endif
	x(OC_RET_STRUCT, 0, OS_LONG),

	x(OC_PUSH_STRUCTRET_ADDR, STACKW_SIZE, OS_ZERO),

	x(OC_MULTI_ARROW, -(2 * STACKW_SIZE), OS_ZERO),

	x(OC_UNRESOLVED_JUMP, 0, OS_OTHER),
	x(OC_JUMP, 0, OS_OTHER),
	x(OC_JUMP_IF_NON_ZERO, -STACKW_SIZE, OS_OTHER),
	x(OC_JUMP_IF_ZERO, -STACKW_SIZE, OS_OTHER),

	x(OC_SWITCH_ON_TABLE, -STACKW_SIZE, OS_OTHER),
	x(OC_SWITCH_ON_CHAIN_B, -STACKW_SIZE, OS_OTHER),
	x(OC_SWITCH_ON_CHAIN_W, -STACKW_SIZE, OS_OTHER),
	x(OC_SWITCH_ON_CHAIN_L, -STACKW_SIZE, OS_OTHER),
	x(OC_SWITCH_ON_CHAIN_Q, -STACKW_SIZE, OS_OTHER),

	x(OC_PUSH_WORD_RETVAL, STACKW_SIZE, OS_ZERO),
#if WANT_LL
	x(OC_PUSH_LONG_LONG_RETVAL, LONGLONG_NBYTES, OS_ZERO),
#endif
	x(OC_PUSH_FLOAT_RETVAL, FLOAT_NBYTES, OS_ZERO),
	x(OC_PUSH_DOUBLE_RETVAL, DOUBLE_NBYTES, OS_ZERO),
#if WANT_LDBL
	x(OC_PUSH_LONG_DOUBLE_RETVAL, LDOUBLE_NBYTES, OS_ZERO),
#endif
#if WANT_TYPE_PUSHED
	x(OC_CALL_INDIRECT, -STACKW_SIZE, OS_OTHER),
#else
	x(OC_CALL_INDIRECT, -STACKW_SIZE, OS_BYTE),
#endif
	x(OC_TRAP, 0, OS_ZERO),
	x(OC_NOP, 0, OS_ZERO),

	/*  These two are only generated by ci_make_crt0(), thus they are
	 *  not recognised in the disassembler or ci_code_opcode().
	 */
	x(OC_SETJMP, 0, OS_ZERO),
	x(OC_LONGJMP, 0, OS_ZERO),

	x(OC_DUP, STACKW_SIZE, OS_ZERO),
	x(OC_DUP_BACK_ONE, 0, OS_ZERO),
	x(OC_RESERVE_SLOT, STACKW_SIZE, OS_ZERO),
	x(OC_POP, -STACKW_SIZE, OS_ZERO),

	x(OC_CVT_TO_BOOL, 0, OS_ZERO),

#if WANT_LL
	x(OC_BITWISE_AND_LONG_LONG, -LONGLONG_NBYTES, OS_ZERO),
#endif
	x(OC_BITWISE_AND, -STACKW_SIZE, OS_ZERO),
#if WANT_LL
	x(OC_BITWISE_XOR_LONG_LONG, -LONGLONG_NBYTES, OS_ZERO),
#endif
	x(OC_BITWISE_XOR, -STACKW_SIZE, OS_ZERO),
#if WANT_LL
	x(OC_BITWISE_OR_LONG_LONG, -LONGLONG_NBYTES, OS_ZERO),
#endif
	x(OC_BITWISE_OR, -STACKW_SIZE, OS_ZERO),
#if WANT_LL
	x(OC_BITWISE_NOT_LONG_LONG, 0, OS_ZERO),
#endif
	x(OC_BITWISE_NOT, 0, OS_ZERO),
	x(OC_LOGICAL_NOT, 0, OS_ZERO),
#if WANT_LL
	x(OC_LSHIFT_LONG_LONG, -LONGLONG_NBYTES, OS_ZERO),
#endif
	x(OC_LSHIFT, -STACKW_SIZE, OS_ZERO),
#if WANT_LL
	x(OC_RSHIFT_LONG_LONG, -LONGLONG_NBYTES, OS_ZERO),
#endif
	x(OC_RSHIFT, -STACKW_SIZE, OS_ZERO),

#if WANT_LL
	x(OC_MOD_LONG_LONG, -LONGLONG_NBYTES, OS_ZERO),
#endif
	x(OC_MOD, -STACKW_SIZE, OS_ZERO),

/* ----------------------- */
#if WANT_LL
	x(OC_LONG_LONG_UNARY_MINUS, 0, OS_ZERO),
#endif
	x(OC_UNARY_MINUS, 0, OS_ZERO),
	x(OC_FLOAT_UNARY_MINUS, 0, OS_ZERO),
	x(OC_DOUBLE_UNARY_MINUS, 0, OS_ZERO),
#if WANT_LDBL
	x(OC_LONG_DOUBLE_UNARY_MINUS, 0, OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_MUL_SIGNED_LONG_LONG, -LONGLONG_NBYTES, OS_ZERO),
#endif
	x(OC_MUL_SIGNED, -STACKW_SIZE, OS_ZERO),
	x(OC_MUL_FLOATS, -(FLOAT_NBYTES), OS_ZERO),
	x(OC_MUL_DOUBLES, -(DOUBLE_NBYTES), OS_ZERO),
#if WANT_LDBL
	x(OC_MUL_LONG_DOUBLES, -(LDOUBLE_NBYTES), OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_DIV_SIGNED_LONG_LONG, -LONGLONG_NBYTES, OS_ZERO),
#endif
	x(OC_DIV_SIGNED, -STACKW_SIZE, OS_ZERO),
	x(OC_DIV_FLOATS, -(FLOAT_NBYTES), OS_ZERO),
	x(OC_DIV_DOUBLES, -(DOUBLE_NBYTES), OS_ZERO),
#if WANT_LDBL
	x(OC_DIV_LONG_DOUBLES, -(LDOUBLE_NBYTES), OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_CHKDIV_SIGNED_LONG_LONG, -LONGLONG_NBYTES, OS_ZERO),
#endif
	x(OC_CHKDIV_SIGNED, -STACKW_SIZE, OS_ZERO),
	x(OC_CHKDIV_FLOATS, -(FLOAT_NBYTES), OS_ZERO),
	x(OC_CHKDIV_DOUBLES, -(DOUBLE_NBYTES), OS_ZERO),
#if WANT_LDBL
	x(OC_CHKDIV_LONG_DOUBLES, -(LDOUBLE_NBYTES), OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_ADD_LONG_LONG, -LONGLONG_NBYTES, OS_ZERO),
#endif
	x(OC_ADD, -STACKW_SIZE, OS_ZERO),
	x(OC_ADD_FLOATS, -(FLOAT_NBYTES), OS_ZERO),
	x(OC_ADD_DOUBLES, -(DOUBLE_NBYTES), OS_ZERO),
#if WANT_LDBL
	x(OC_ADD_LONG_DOUBLES, -(LDOUBLE_NBYTES), OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_SUB_LONG_LONG, -LONGLONG_NBYTES, OS_ZERO),
#endif
	x(OC_SUB, -STACKW_SIZE, OS_ZERO),
	x(OC_SUB_FLOATS, -(FLOAT_NBYTES), OS_ZERO),
	x(OC_SUB_DOUBLES, -(DOUBLE_NBYTES), OS_ZERO),
#if WANT_LDBL
	x(OC_SUB_LONG_DOUBLES, -(LDOUBLE_NBYTES), OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_LONG_LONG_IS_EQUAL, -((2 * LONGLONG_NBYTES) - STACKW_SIZE), OS_ZERO),
#endif
	x(OC_IS_EQUAL, -STACKW_SIZE, OS_ZERO),
	x(OC_FLOAT_IS_EQUAL, -((2 * FLOAT_NBYTES) - STACKW_SIZE), OS_ZERO),
	x(OC_DOUBLE_IS_EQUAL, -((2 * DOUBLE_NBYTES) - STACKW_SIZE), OS_ZERO),
#if WANT_LDBL
	x(OC_LONG_DOUBLE_IS_EQUAL, -((2 * LDOUBLE_NBYTES) - STACKW_SIZE), OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_LONG_LONG_NOT_EQUAL, -((2 * LONGLONG_NBYTES) - STACKW_SIZE), OS_ZERO),
#endif
	x(OC_NOT_EQUAL, -STACKW_SIZE, OS_ZERO),
	x(OC_FLOAT_NOT_EQUAL, -((2 * FLOAT_NBYTES) - STACKW_SIZE), OS_ZERO),
	x(OC_DOUBLE_NOT_EQUAL, -((2 * DOUBLE_NBYTES) - STACKW_SIZE), OS_ZERO),
#if WANT_LDBL
	x(OC_LONG_DOUBLE_NOT_EQUAL, -((2 * LDOUBLE_NBYTES) - STACKW_SIZE), OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_LONG_LONG_LESS_SIGNED, -((2 * LONGLONG_NBYTES) - STACKW_SIZE), OS_ZERO),
#endif
	x(OC_LESS_SIGNED, -STACKW_SIZE, OS_ZERO),
	x(OC_FLOAT_LESS, -((2 * FLOAT_NBYTES) - STACKW_SIZE), OS_ZERO),
	x(OC_DOUBLE_LESS, -((2 * DOUBLE_NBYTES) - STACKW_SIZE), OS_ZERO),
#if WANT_LDBL
	x(OC_LONG_DOUBLE_LESS, -((2 * LDOUBLE_NBYTES) - STACKW_SIZE), OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_LONG_LONG_GREATER_SIGNED, -((2 * LONGLONG_NBYTES) - STACKW_SIZE), OS_ZERO),
#endif
	x(OC_GREATER_SIGNED, -STACKW_SIZE, OS_ZERO),
	x(OC_FLOAT_GREATER, -((2 * FLOAT_NBYTES) - STACKW_SIZE), OS_ZERO),
	x(OC_DOUBLE_GREATER, -((2 * DOUBLE_NBYTES) - STACKW_SIZE), OS_ZERO),
#if WANT_LDBL
	x(OC_LONG_DOUBLE_GREATER, -((2 * LDOUBLE_NBYTES) - STACKW_SIZE), OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_LONG_LONG_LESS_OR_EQUAL_SIGNED, -((2 * LONGLONG_NBYTES) - STACKW_SIZE), OS_ZERO),
#endif
	x(OC_LESS_OR_EQUAL_SIGNED, -STACKW_SIZE, OS_ZERO),
	x(OC_FLOAT_LESS_OR_EQUAL, -((2 * FLOAT_NBYTES) - STACKW_SIZE), OS_ZERO),
	x(OC_DOUBLE_LESS_OR_EQUAL, -((2 * DOUBLE_NBYTES) - STACKW_SIZE), OS_ZERO),
#if WANT_LDBL
	x(OC_LONG_DOUBLE_LESS_OR_EQUAL, -((2 * LDOUBLE_NBYTES) - STACKW_SIZE), OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_LONG_LONG_GREATER_OR_EQUAL_SIGNED, -((2 * LONGLONG_NBYTES) - STACKW_SIZE), OS_ZERO),
#endif
	x(OC_GREATER_OR_EQUAL_SIGNED, -STACKW_SIZE, OS_ZERO),
	x(OC_FLOAT_GREATER_OR_EQUAL, -((2 * FLOAT_NBYTES) - STACKW_SIZE), OS_ZERO),
	x(OC_DOUBLE_GREATER_OR_EQUAL, -((2 * DOUBLE_NBYTES) - STACKW_SIZE), OS_ZERO),
#if WANT_LDBL
	x(OC_LONG_DOUBLE_GREATER_OR_EQUAL, -((2 * LDOUBLE_NBYTES) - STACKW_SIZE), OS_ZERO),
#endif
/* ----------------------- */
#if WANT_LL
	x(OC_MUL_UNSIGNED_LONG_LONG, -LONGLONG_NBYTES, OS_ZERO),
#endif
	x(OC_MUL_UNSIGNED, -STACKW_SIZE, OS_ZERO),
#if WANT_LL
	x(OC_DIV_UNSIGNED_LONG_LONG, -LONGLONG_NBYTES, OS_ZERO),
#endif
	x(OC_DIV_UNSIGNED, -STACKW_SIZE, OS_ZERO),
#if WANT_LL
	x(OC_CHKDIV_UNSIGNED_LONG_LONG, -LONGLONG_NBYTES, OS_ZERO),
#endif
	x(OC_CHKDIV_UNSIGNED, -STACKW_SIZE, OS_ZERO),
#if WANT_LL
	x(OC_LONG_LONG_LESS_UNSIGNED, -((2 * LONGLONG_NBYTES) - STACKW_SIZE), OS_ZERO),
#endif
	x(OC_LESS_UNSIGNED, -STACKW_SIZE, OS_ZERO),
#if WANT_LL
	x(OC_LONG_LONG_GREATER_UNSIGNED, -((2 * LONGLONG_NBYTES) - STACKW_SIZE), OS_ZERO),
#endif
	x(OC_GREATER_UNSIGNED, -STACKW_SIZE, OS_ZERO),
#if WANT_LL
	x(OC_LONG_LONG_LESS_OR_EQUAL_UNSIGNED, -((2 * LONGLONG_NBYTES) - STACKW_SIZE), OS_ZERO),
#endif
	x(OC_LESS_OR_EQUAL_UNSIGNED, -STACKW_SIZE, OS_ZERO),
#if WANT_LL
	x(OC_LONG_LONG_GREATER_OR_EQUAL_UNSIGNED, -((2 * LONGLONG_NBYTES) - STACKW_SIZE), OS_ZERO),
#endif
	x(OC_GREATER_OR_EQUAL_UNSIGNED, -STACKW_SIZE, OS_ZERO),

#if WANT_LDBL
	x(OC_CVT_UNSIGNED_LONG_TO_LONG_DOUBLE, LDOUBLE_NBYTES - STACKW_SIZE, OS_ZERO),
	x(OC_CVT_SIGNED_LONG_TO_LONG_DOUBLE, LDOUBLE_NBYTES - STACKW_SIZE, OS_ZERO),
	x(OC_CVT_DOUBLE_TO_LONG_DOUBLE, LDOUBLE_NBYTES - DOUBLE_NBYTES, OS_ZERO),
	x(OC_CVT_FLOAT_TO_LONG_DOUBLE, LDOUBLE_NBYTES - FLOAT_NBYTES, OS_ZERO),
	x(OC_CVT_LONG_DOUBLE_TO_DOUBLE, -(LDOUBLE_NBYTES - DOUBLE_NBYTES), OS_ZERO),
	x(OC_CVT_LONG_DOUBLE_TO_FLOAT, -(LDOUBLE_NBYTES - FLOAT_NBYTES), OS_ZERO),
	x(OC_CVT_LONG_DOUBLE_TO_LONG, -(LDOUBLE_NBYTES - STACKW_SIZE), OS_ZERO),
	x(OC_CVT_LONG_DOUBLE_TO_BOOL, -(LDOUBLE_NBYTES - STACKW_SIZE), OS_ZERO),
#endif

#if WANT_LL
	x(OC_CVT_UNSIGNED_LONG_TO_LONG_LONG, LONGLONG_NBYTES - STACKW_SIZE, OS_ZERO),
	x(OC_CVT_SIGNED_LONG_TO_LONG_LONG, LONGLONG_NBYTES - STACKW_SIZE, OS_ZERO),
	x(OC_CVT_DOUBLE_TO_LONG_LONG, LONGLONG_NBYTES - DOUBLE_NBYTES, OS_ZERO),
	x(OC_CVT_FLOAT_TO_LONG_LONG, LONGLONG_NBYTES - FLOAT_NBYTES, OS_ZERO),
	x(OC_CVT_UNSIGNED_LONG_LONG_TO_DOUBLE, DOUBLE_NBYTES - LONGLONG_NBYTES, OS_ZERO),
	x(OC_CVT_SIGNED_LONG_LONG_TO_DOUBLE, DOUBLE_NBYTES - LONGLONG_NBYTES, OS_ZERO),
	x(OC_CVT_UNSIGNED_LONG_LONG_TO_FLOAT, FLOAT_NBYTES - LONGLONG_NBYTES, OS_ZERO),
	x(OC_CVT_SIGNED_LONG_LONG_TO_FLOAT, FLOAT_NBYTES - LONGLONG_NBYTES, OS_ZERO),
	x(OC_CVT_UNSIGNED_LONG_LONG_TO_LONG, STACKW_SIZE - LONGLONG_NBYTES, OS_ZERO),
	x(OC_CVT_SIGNED_LONG_LONG_TO_LONG, STACKW_SIZE - LONGLONG_NBYTES, OS_ZERO),
	x(OC_CVT_LONG_LONG_TO_BOOL, STACKW_SIZE - LONGLONG_NBYTES, OS_ZERO),
#endif

#if WANT_LDBL && WANT_LL
	x(OC_CVT_UNSIGNED_LONG_LONG_TO_LONG_DOUBLE, LDOUBLE_NBYTES - LONGLONG_NBYTES, OS_ZERO),
	x(OC_CVT_SIGNED_LONG_LONG_TO_LONG_DOUBLE, LDOUBLE_NBYTES - LONGLONG_NBYTES, OS_ZERO),
	x(OC_CVT_LONG_DOUBLE_TO_LONG_LONG, LONGLONG_NBYTES - LDOUBLE_NBYTES, OS_ZERO),
#endif

	x(OC_CVT_UNSIGNED_LONG_TO_DOUBLE, DOUBLE_NBYTES - STACKW_SIZE, OS_ZERO),
	x(OC_CVT_SIGNED_LONG_TO_DOUBLE, DOUBLE_NBYTES - STACKW_SIZE, OS_ZERO),
	x(OC_CVT_FLOAT_TO_DOUBLE, DOUBLE_NBYTES - FLOAT_NBYTES, OS_ZERO),
	x(OC_CVT_UNSIGNED_LONG_TO_FLOAT, FLOAT_NBYTES - STACKW_SIZE, OS_ZERO),
	x(OC_CVT_SIGNED_LONG_TO_FLOAT, FLOAT_NBYTES - STACKW_SIZE, OS_ZERO),
	x(OC_CVT_DOUBLE_TO_FLOAT, -(DOUBLE_NBYTES - FLOAT_NBYTES), OS_ZERO),
	x(OC_CVT_FLOAT_TO_LONG, -(FLOAT_NBYTES - STACKW_SIZE), OS_ZERO),
	x(OC_CVT_DOUBLE_TO_LONG, -(DOUBLE_NBYTES - STACKW_SIZE), OS_ZERO),
	x(OC_CVT_FLOAT_TO_BOOL, -(FLOAT_NBYTES - STACKW_SIZE), OS_ZERO),
	x(OC_CVT_DOUBLE_TO_BOOL, -(DOUBLE_NBYTES - STACKW_SIZE), OS_ZERO)
#ifdef WANT_ENUM
};
#endif

#ifdef WANT_ENUM
#define NUM_OPCODES	((int)OC_CVT_DOUBLE_TO_BOOL + 1)		

typedef enum opcode_e opcode_t;

#define STACKW_NBYTES	((int)sizeof(stackword_t)
#define FLOAT_NBYTES	((int)sizeof(float))
#define DOUBLE_NBYTES	((int)sizeof(double))
#if WANT_LDBL
#define LDOUBLE_NBYTES	((int)sizeof(long_double_t))
#endif
#if WANT_LL
#define LONGLONG_NBYTES	((int)sizeof(long_long_t))
#endif

#define FLOAT_NSLOTS	(FLOAT_NBYTES / sizeof(stackword_t))
#define DOUBLE_NSLOTS	(DOUBLE_NBYTES / sizeof(stackword_t))
#if WANT_LDBL
#define LDOUBLE_NSLOTS	(LDOUBLE_NBYTES / sizeof(stackword_t))
#endif
#if WANT_LL
#define LONGLONG_NSLOTS	(LONGLONG_NBYTES / sizeof(stackword_t))
#endif

#if WANT_LL
#define LONGLONG_FORM_OFFSET	-1
#endif
#define BYTE_FORM_OFFSET	0
#define SHORT_FORM_OFFSET	1
#define LONG_FORM_OFFSET	2
#define QUAD_FORM_OFFSET	3
#define FLOAT_FORM_OFFSET	3
#define DOUBLE_FORM_OFFSET	4
#if WANT_LDBL
#define LDOUBLE_FORM_OFFSET	5
#endif

#define N_OPCODE_SIZES		4

#define BYTE_FORM(o)		((opcode_t)((int)(o) + BYTE_FORM_OFFSET))
#define SHORT_FORM(o)		((opcode_t)((int)(o) + SHORT_FORM_OFFSET))
#define LONG_FORM(o)		((opcode_t)((int)(o) + LONG_FORM_OFFSET))
#define QUAD_FORM(o)		((opcode_t)((int)(o) + QUAD_FORM_OFFSET))

#define MAX_BYTE	0xff
#define MAX_WORD	0xffff
#define MAX_LONG	0xffffffff
#endif

#undef x
#undef WANT_OPINFO
#undef WANT_LABADDRS
#undef WANT_ENUM

