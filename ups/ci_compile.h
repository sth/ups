/* ci_compile.h - header file for the C interpreter compilation routines */

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


/* @(#)ci_compile.h	1.10 16 Apr 1994 (UKC) */

#define IS_LOCAL_CLASS(class)	((class) == CL_AUTO || (class) == CL_ARG)

/*  Type of target area of relocation.  We need to distinguish between
 *  internal text and data because the byte orders are different (text
 *  is always little endian because we use a bytecode machine, but data
 *  is machine-endian).  This needs to be fixed some day.
 */
typedef enum {
	RT_TEXTLOC,		/* Internal text */
	RT_DATALOC,		/* Internal data */
	RT_EXTDATALOC		/* External data */
} loctype_t;

typedef struct funcreloc_s {
	loctype_t fr_loctype;
	taddr_t fr_location;
	struct funcreloc_s *fr_next;
} funcreloc_t;

typedef struct relocst {
	loctype_t rl_loctype;	/* text, data or extdata */
	taddr_t rl_location;	/* relative to start of area */
	union {
		var_t *rlu_var;		/* used on tx_var_relocs list */
		string_const_t *rlu_sc;	/* used on tx_string_relocs list */
	} rl_u;
	long rl_offset;
	struct relocst *rl_next;
} reloc_t;

#define rl_var		rl_u.rlu_var
#define rl_sc		rl_u.rlu_sc

typedef struct labelrefst {
	taddr_t lr_location;
	struct labelrefst *lr_next;
} labelref_t;

typedef struct labelst {
	taddr_t lb_addr;
	labelref_t *lb_refs;
} branch_label_t;

typedef struct label_listst {
	labeldesc_t ll_labeldesc;
	struct label_listst *ll_next;
} label_list_t;

typedef struct vlist_s {
	var_t *vl_var;
	struct vlist_s *vl_next;
} vlist_t;

typedef enum { LT_GOTO, LT_BREAK, LT_CONTINUE, LT_NLABEL_TYPES } label_type_t;

typedef struct textst {
	alloc_pool_t *tx_apool;
	textword_t *tx_text;
	taddr_t tx_pclim;
	taddr_t tx_pc;
	taddr_t tx_sp;
	taddr_t tx_max_sp;
	branch_label_t **tx_labels;
	lno_t *tx_lnos;
	lno_t *tx_last_lno;
	fil_t *tx_curfil;		/* File we are currently compiling */
	initlist_t *tx_initlist;
	reloc_t *tx_var_relocs;
	reloc_t *tx_string_relocs;
	funcreloc_t *tx_func_relocs;
	int tx_labellim;
	label_list_t *tx_label_lists[(int)LT_NLABEL_TYPES];
	var_t *tx_varlist;
	vlist_t *tx_vlist;
	
	func_t *tx_funclist;
	int tx_num_funcs;
	
	libfunc_t *tx_undef_funcs;
	int tx_num_undef_funcs;
	
	ci_report_error_func_t tx_report_error_func;
	ci_checkarg_proc_t tx_checkarg_proc;
	
	ci_regno_to_addr_proc_t tx_regno_to_addr_proc;
	bool tx_want_reg_relocs;
	reg_reloc_t *tx_reg_relocs;
	bool tx_have_proc_vars;
	
	unsigned long tx_flags;
	bool tx_had_error;
} text_t;

void ci_code_byte PROTO((text_t *tx, long byte));
void ci_code_word PROTO((text_t *tx, long word));
void ci_code_long PROTO((text_t *tx, long l));
void ci_code_quad PROTO((text_t *tx, long q));
void ci_code_opcode PROTO((text_t *tx, opcode_t opcode));
void ci_code_generic_opcode PROTO((text_t *tx, opcode_t byte_opcode,
				   stackword_t arg));
void ci_code_constpush PROTO((text_t *tx, long val));
labeldesc_t ci_code_jump_to_label PROTO((text_t *tx, opcode_t opcode,
							labeldesc_t labeldesc));
void ci_resolve_and_free_label PROTO((text_t *tx, labeldesc_t labeldesc));
void ci_add_var_reloc PROTO((text_t *tx, loctype_t loctype, taddr_t location,
			     var_t *v, long offset));
void ci_add_func_reloc PROTO((text_t *tx, loctype_t loctype, taddr_t location));
void ci_add_string_reloc PROTO((text_t *tx, loctype_t loctype, taddr_t location,
				string_const_t *sc, long offset));

int ci_funcname_to_index PROTO((text_t *tx, lexinfo_t *lx, const char *name));

taddr_t ci_libvarname_to_addr PROTO((text_t *tx, lexinfo_t *lx, const char *name));

func_t *ci_addr_to_func PROTO((func_t *funclist, taddr_t addr));
