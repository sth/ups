/* symtab.h - internal ups symbol table format */

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


/* @(#)symtab.h	1.28 04 Jun 1995 (UKC) */

/*  Define a symbol to indicate that we've been #included.
 *  This is so other prototype files like srcwin.h can include
 *  prototypes involving fil_t * etc only if they've been
 *  defined.
 */
#define SYMTAB_H_INCLUDED

/*  This structure is given a tag but not defined here.  It is never
 *  defined in the C interpreter (which doesn't use a symbol table struct).
 *  In ups it is defined in st.h.
 */
typedef struct symtab_s symtab_t;

/*  Languages
 */
typedef enum languageen {
	LANG_C,		/* C */
 	LANG_CC,	/* C++ */
	LANG_F77,	/* FORTRAN 77 */
	LANG_F90,	/* Fortran 90 */
	LANG_UNKNOWN	/* Unknown language */
} language_t;

#define IS_FORTRAN(language)  ((language) == LANG_F77 || (language) == LANG_F90)

/* RGA moved from ao_syms.h */
typedef enum {
	CT_UNKNOWN,
	CT_GNU_CC,
	CT_CC,
	CT_CLCC,
	CT_EPCF90,
	CT_GNU_F77,
	CT_F77
} Compiler_type;

/*  Variable code types.
 *  (IDE) For the C interpreter these must be in the range 0..255
 */
typedef enum typecodeen {

/* derivations */
	DT_PTR_TO = 0,		/* pointer to type ty_base */
	DT_FUNC_RETURNING,	/* function returning type ty_base */
	DT_ARRAY_OF,	 	/* array [ty_dim] of ty_base */

/* seperator between derivations and base types */
	TY_NOTYPE,

/* Type couldn't be determined */
	TY_UNKNOWN,

/* Parse internal */
	PI_SAME_AS,	/* Defined to be same as ty_base (RCB) */

/* Types for the C interpreter */
	TY_IDENTIFIER,	/* "type" used for stashing varnames when parsing */
	TY_ELLIPSIS, 	/* temporary indicator that a function is variadic */
	TY_UNSIGNED,
	TY_SIGNED,

/* C aggregate types etc */
	TY_STRUCT,	/* structure */
	TY_UNION,	/* union */
	TY_ENUM,	/* enumeration */
	TY_U_STRUCT,	/* undefined structure */
	TY_U_UNION,	/* undefined union */
	TY_U_ENUM,	/* undefined enum */
	TY_BITFIELD,	/* bitfield */

/* seperator between aggregate and basic types */
	TY_BASIC_TYPES,

/* Base types (common to C and Fortran */
	TY_VOID,	/* void */
	TY_CHAR,	/* character */
	TY_UCHAR,	/* unsigned character */
	TY_SHORT,	/* short integer */
	TY_USHORT,	/* unsigned short */
	TY_INT,		/* integer */
	TY_UINT,	/* unsigned int */
	TY_LONG,	/* long integer */
	TY_ULONG,	/* unsigned long */
	TY_FLOAT,	/* floating point */
	TY_DOUBLE,	/* double word */
	TY_INT_ASSUMED,	/* unknown - int assumed */

/* Non-standard C types */
	TY_LONGLONG,
	TY_ULONGLONG,
	TY_LONGDOUBLE,
	TY_BOOLEAN,	/* RGA for gcc */

/* FORTRAN base types */
	TY_COMPLEX,	/* Fortran complex */
	TY_DBLCOMP,	/* Fortran double complex */
	TY_LOGICAL,	/* Fortran logical */
	TY_CHARACTER,	/* Fortran character */
	TY_FVOID, 	/* fortran void */

/* Maximum type number */
	TY_MAXTYPE

} typecode_t;

/*  Bit masks used by the C interpeter symbol table reading and writing code.
 */
#define TC_CODEMASK	0x7f
#define TC_HAS_TYPEDEF	0x80

#define ISDERIV(code)		((long)(code) < (long)TY_NOTYPE)

#define IS_BASIC_TYPE(code)	((long)(code) > (long)TY_BASIC_TYPES)

/*  Opaque handles on C interpreter objects
 */
typedef struct { int ei_dummy; } *expr_id_t;
typedef struct { int si_dummy; } *statement_id_t;
typedef struct { int ii_dummy; } *initialiser_id_t;

#if !defined(CI_H_INCLUDED)
typedef struct lexinfo_s lexinfo_t;
#endif

/*  Member of enum.
 */
typedef struct enum_member_s {
	expr_id_t em_expr_id;
	const char *em_name;
	long em_val;
	struct aggr_or_enum_def_s *em_enum;	/* ptr back to parent enum struct */
	struct enum_member_s *em_next;
	lexinfo_t *em_lexinfo;
} enum_member_t;

typedef enum { AE_COMPLETE, AE_INCOMPLETE } ae_is_complete_t;

/*  Aggregate (struct, union) or enum definition.
 *  Whether the aggregate is a struct or a union is recorded in the type_t
 *  structure that points to this, which is pointed back to by ae_type.
 *
 *  We have a nasty special case due to the SunOS header file scheme:
 *  if ae_type is NULL then ae_sublist is a pointer to a sublist that
 *  should be interpolated at this point in the list.  This will recurse
 *  if there are nested #includes.
 */
typedef struct aggr_or_enum_def_s {
	const char *ae_tag;
	ae_is_complete_t ae_is_complete;
	int ae_size;
	int ae_alignment;
	struct type_s *ae_type;
	struct aggr_or_enum_def_s *ae_next;
	lexinfo_t *ae_lexinfo;
	union {
		struct var_s *aeu_aggr_members;
		enum_member_t *aeu_enum_members;
		struct aggr_or_enum_def_s *aeu_sublist;
	} ae_u;
} aggr_or_enum_def_t;

#define ae_aggr_members		ae_u.aeu_aggr_members
#define ae_enum_members		ae_u.aeu_enum_members
#define ae_sublist		ae_u.aeu_sublist

typedef enum {
	DT_CONSTANT,		/* Compile-time bounds */
	DT_FPREL,		/* Runtime bound, stored at fp+offset */
	DT_APREL,		/* Runtime bound, stored at ap+offset */
	DT__UNKNOWN		/* Runtime bound, not known */
} Dimtype;

/*  Array size description.  For C, the type of the range must be int
 *  and di_low must be zero. 
 *
 *  If di_ltype or di_htype is not DT_CONSTANT, then the corresponding range
 *  field (di_low or di_high) is dynamic.  This is the case with FORTRAN
 *  parameter arrays with subranges defined by expressions.
 */
typedef struct dim_s {
	Dimtype di_ltype;	/* meaning of di_low */
	Dimtype di_htype;	/* meaning of di_high */
	long di_low;		/* lower bound, or where same can be found */
	long di_high;		/* upper bound, or where same can be found */
	expr_id_t di_high_expr_id;	/* see ci_parse.y */
	struct type_s *di_type;	/* type of an element */
} dim_t;

/*  Bitfield description.
 */
typedef struct bitfield_s {
	struct type_s *bf_type;
	expr_id_t bf_expr_id;
	short bf_offset;	/* within a word, hence 0..31 */
	short bf_width;
} bitfield_t;

typedef struct identifier_s {
	const char *id_name;
	lexinfo_t *id_lexinfo;
	bool id_lparen_follows;
	bool id_semi_follows;	/* Redeclaration fix : Dibyendu 8/1/99 */
} identifier_t;

typedef struct identifier_list_s {
	identifier_t *idl_id;
	struct identifier_list_s *idl_next;
} identifier_list_t;

typedef enum { FDT_IDLIST, FDT_TYPELIST } params_type_t;

/*  Structure representing a "function returning" derivation.
 */
typedef struct funcret_s {
	params_type_t fr_params_type;
	int fr_nparams;
	bool fr_is_variadic;
	bool fr_is_old_style;
	struct var_s *fr_params;
	identifier_list_t *fr_idlist;
} funcret_t;

/*  Type qualifiers.  At present just a bit mask.
 */
typedef unsigned qualifiers_t;

#define QU_VOLATILE	0x0001
#define QU_CONST	0x0002
#define QU_PACKED	0x0004

/*  Structure defining a type.  The type representation here is internal
 *  to ups and is not based on any standard.
 */
typedef struct type_s {
	const char *ty_name;		/* name (optional except for basic type) */
	typecode_t ty_code;		/* derivation or base type */
	qualifiers_t ty_qualifiers;	/* type qualifiers */
	long ty_size;			/* size in bytes of this type (or -1) */
	struct type_s *ty_base;		/* type that this type is derived from */
	struct typedef_s *ty_typedef;	/* non NULL if this type is defed */
	lexinfo_t *ty_lexinfo;
	union {
		dim_t *tyu_dim;				/* DT_ARRAY_OF */
		aggr_or_enum_def_t *tyu_aggr_or_enum;	/* TY_STRUCT/TY_UNION/TY_ENUM */
		bitfield_t *tyu_bitfield;		/* TY_BITFIELD */
		identifier_t *tyu_identifier;		/* TY_IDENTIFIER (pseudo) */
		funcret_t *tyu_funcret;			/* DT_FUNC_RETURNING */
	} ty_u;
} type_t;

#define ty_aggr_or_enum	ty_u.tyu_aggr_or_enum
#define ty_dim		ty_u.tyu_dim
#define ty_bitfield	ty_u.tyu_bitfield
#define ty_identifier	ty_u.tyu_identifier
#define ty_funcret	ty_u.tyu_funcret

/*  Class types
 */
typedef enum classen {
	CL_NOCLASS,	/* out of band value */
	CL_EXTREF,	/* referenced extern variable (ci_compile.c etc) */
	CL_DECL,	/* declaration only */
	CL_AUTO,	/* automatic variable */
	CL_EXT,		/* external symbol */
	CL_STAT,	/* static */
	CL_REG,		/* register variable */
	CL_MOS,		/* member of structure */
	CL_ARG,		/* function argument passed by value */
	CL_REF,		/* function argument passed by reference */
	CL_MOU,		/* member of union */
	CL_MOE,		/* member of enumeration */
	CL_LSTAT,	/* local static */
	CL_FUNC,	/* function */
	CL_LFUNC,	/* static function */
	CL_TYPEDEF,	/* typedef */
	CL_TAGNAME	/* struct/union tag */
} class_t;

/*
 * Extended address description, abstracted from DWARF location description.
 *
 * dwarfTODO: this was a quick kludge to get things working; the proper
 * solution is probably to use what is in the 'C' interpreter.
 */
typedef enum {
    OP_ADDR,		/* Address. */
    OP_REGISTER,	/* Register number. */
    OP_U_OFFSET,	/* Unsigned offset. */
    OP_CFA_RELATIVE,	/* Relative to canonical frame address. */
    OP_FP_RELATIVE,	/* Relative to frame pointer. */
    OP_SP_RELATIVE,	/* Relative to stack pointer. */
} vaddr_op_t;
typedef struct vaddr_s {
	struct vaddr_s *v_next;		/* Next description. */
	taddr_t v_low_pc;		/* Low PC address. */
	taddr_t v_high_pc;		/* High PC address. */
	vaddr_op_t v_op;		/* Operation. */
	union {
	    taddr_t vu_addr;		/* Address or unsigned offset. */
	    long vu_const;		/* Constant. */
	    unsigned char vu_byte;	/* e.g. register number */
	} v_u;
} vaddr_t;
#define v_addr		v_u.vu_addr
#define v_register	v_u.vu_byte
#define v_u_offset	v_u.vu_addr
#define v_const		v_u.vu_const
#define v_offset	v_u.vu_const

/*  Variable, also used for struct/union members and function parameters.
 */
typedef struct var_s {
	const char *va_name;	/* variable name */
	class_t va_class;	/* class of var (see CL_ defines above) */
	language_t va_language;	/* language (C, FORTRAN etc) */
	short va_flags;		/* flags - see below */
	type_t *va_type;	/* variable type */
	taddr_t va_addr;	/* variable address */
	struct var_s *va_next;	/* next variable */
	lexinfo_t *va_lexinfo;  /* lexical information */
	vaddr_t *va_location;   /* variable location */
} var_t;

/*  Flags describing a variable.
 */
#define VA_HIDE_PTR		0x0001	/* hide outer ptr type (st_skim.c) */
#define VA_IS_CI_VAR		0x0002	/* var is private to C interpreter */
#define VA_HAS_INITIALISER	0x0004	/* var is initialised */
#define VA_EXT_RELOC		0x0008	/* use external relocation (ci) */
#define VA_TODO			0x0010	/* tmp marker flag (ci_compile.c) */
#define VA_ON_VLIST		0x0020	/* on tx_vlist (ci_compile.c) */
#define VA_BASECLASS		0x0040	/* Is base class for a g++ class (RCB) */
#define VA_STATIC_MEMBER	0x0080	/* Static member of a g++ class (RCB) */
#define VA_VTBLPTR		0x0100	/* Is virtual table pointer (RCB) */
#define VA_VTBLBASED		0x0200  /* Offset to object is in the vtbl (RCB) */

/*  Macro information.
 */
typedef struct macro_value_s {
	int mav_start_lnum;		/* first line # */
	int mav_end_lnum;		/* last line # */
	const char *mav_value;		/* value */
	struct macro_value_s *mav_next;	/* next value */
} macro_value_t;

typedef struct macro_s {
	const char *ma_name;		/* macro name */
	macro_value_t *ma_values;	/* macro value list */
	struct macro_s *ma_next;	/* next macro */
} macro_t;

/*  Element in the list of functions attached to a source file.
 */
typedef struct funclist_s {
	struct func_s *fl_func;
	struct funclist_s *fl_next;
} funclist_t;

/*  source file structure
 */
typedef struct fil_s {
	const char *fi_name;		/* file name */
	const char *fi_name_only;	/* file name start place in fi_name */
	const char *fi_path_hint;	/* possible directory of source file */
        char *fi_source_path;		/* directory of displayed source file */
	language_t fi_language;		/* programming language of the file */
	short fi_flags;
	symtab_t *fi_symtab;		/* symbol table */
	char *fi_data;			/* symtab type dependent data */
	struct Srcbuf *fi_srcbuf;	/* handle on source text */
	struct block_s *fi_block;	/* vars and defs with file scope */
	funclist_t *fi_funclist;	/* list of functions in this file */
	macro_t *fi_macros;		/* macro definitions */
	struct fil_s *fi_next;		/* next file */
} fil_t;

#define FI_DONE_VARS		(1<<0)
#define FI_DONE_TYPES		(1<<1)
#define FI_DOING_TYPES		(1<<2)
#define FI_HIDE_ENTRY		(1<<3)
#define FI_FOUND_COMPILER	(1<<4)	/* RGA */
#define FI_DUPLICATE		(1<<5)	/* RGA used for duplicate files */
#define FI_DONE_MATCH		(1<<6)	/* RGA already matched source */
#define FI_NEEDS_RESCAN		(1<<7)	/* RGA rescan after reload/rematch */
#define FI_RENAMED_OTHER	(1<<8)	/* RGA file renamed to unknown lang */
#define FI_HEADER		(1<<9)	/* RGA from  N_SOL symbol */
#define FI_DONE_MACROS		(1<<10)	/* macro information loaded */

#define FI_VARS(fil)	(((fil)->fi_flags & FI_DONE_VARS) \
						? (fil)->fi_block->bl_vars \
					        : st_get_fi_vars(fil))

#define FI_MACROS(fil)  (((fil)->fi_flags & FI_DONE_MACROS) \
						? (fil)->fi_macros \
					        : st_get_fi_macros(fil))

/* line number structure.
 */
typedef struct lno_s {
	taddr_t ln_addr;	/* text offset */
	fil_t *ln_fil;		/* source file */
	int ln_num;		/* line number */
	int ln_col;		/* column number (NYI) */
	struct lno_s *ln_next;	/* next line */
} lno_t;

/*  We have the same kind of special case here as for aggr_or_enum_def_t:
 *  SunOS header files mean we have to handle sublists.
 */
typedef struct typedef_s {
	const char *td_name;
	type_t *td_type;	/* the type_t structure that points to this */
	struct typedef_s *td_next;
	lexinfo_t *td_lexinfo;
	struct typedef_s *td_sublist;
} typedef_t;

typedef struct initlist_s {
	var_t *il_var;
	fil_t *il_fil;
	initialiser_id_t il_initialiser_id;
	struct initlist_s *il_next;
} initlist_t;

/*  Structure describing a block.
 */
typedef struct block_s {
	int bl_start_lnum;		/* first line # */
	int bl_end_lnum;		/* last line # */
	var_t *bl_vars;			/* vars declared at in this block */
	typedef_t *bl_typedefs;		/* typedefs declared in this block */
	aggr_or_enum_def_t *bl_aggr_or_enum_defs;
	initlist_t *bl_initlist;	/* used by the C interpreter */
	struct block_s *bl_next;	/* next block at this level */
	struct block_s *bl_blocks;	/* sub blocks */
	struct block_s *bl_parent;	/* parent block */
} block_t;

/*  Maximum block nesting level. RGA - doubled to 64 for C++
 */
#define MAX_BLOCK_LEVEL	64

/*  Structure defining a function.  As we only read the symbol table
 *  on demand, many of the fields are initially unset.
 *
 *  The fields with a double underscore (e.g. fu__blocks) should not
 *  be accessed directly.  Instead the corresponding macro (e.g. FU_BLOCKS(f))
 *  should be used.
 */
typedef struct func_s {
	/*  Always valid.
	 */
	short fu_flags;			/* flags, see below */
	const char *fu_name;		/* function name */
	const char *fu_demangled_name;  /* RGA for C++ name demangling */
	type_t *fu_type;
	taddr_t fu_addr;		/* function text area address */
	fil_t *fu_fil;			/* source file */
	language_t fu_language;		/* source language */
	symtab_t *fu_symtab; 		/* symbol table of this function */
	struct cblist_s *fu_cblist;	/* common blocks (FORTRAN only) */
	lexinfo_t *fu_lexinfo;		/* source location of start of func */
	struct func_s *fu_next;

	char *fu_predata;		/* target type dependent preamble data*/
	char *fu_symdata;		/* symtab type dependent data */

	/*  The following fields are used only by the C interpreter.
	 */
	struct statement_s *fu_statement; /* stm containing body of func */

	/*  Valid if FU_DONE_LNOS set
	 */
	lno_t *fu__lnos;		/* list of line numbers, if any */
	short fu_max_lnum;

	/*  Valid if FU_DONE_BLOCKS set
	 */
	block_t *fu__blocks;		/* block list */
} func_t;

typedef struct func_bp_list_s {
	func_t *fl_func;
	void *fl_bp;
	struct func_bp_list_s *fl_next;
} func_bp_list_t;

/*  Bits in fu_flags
 */
#define FU_NOSYM	  0x1	/* No symbol table info */
#define FU_DONE_LNOS	  0x2	/* Have processed line number info */
#define FU_DONE_BLOCKS	  0x4	/* Have processed block info */
#define FU_NO_FP	  0x8	/* Function doesn't set up frame pointer */
#define FU_STATIC	 0x10	/* Function is declared static */
#define FU_WEAK		 0x20	/* Function is an ELF weak symbol */
#define FU_BAD		 0x40	/* Bad entry in stack trace */
#define FU_DUPLICATE	 0x80	/* This is duplicate entry: delete it */
#define FU_DONE_ADDR	0x100	/* To distinguish overloaded C++ function addr */
#define FU_NOTHEADER    0x200   /* Function is not in header (C++ code) */
#define FU_PLT		0x400   /* Procedure linkage table */

/*  Macros for accessing fields which may need to be read in.
 */
#define FU_BLOCKS(f)	(((f)->fu_flags & FU_DONE_BLOCKS) ? (f)->fu__blocks \
							  : st_get_fu_blocks(f))

#define FU_LNOS(f)	(((f)->fu_flags & FU_DONE_LNOS) ? (f)->fu__lnos \
							: st_get_fu_lnos(f))

