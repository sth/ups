/* va_priv.h - private header file for the va_*.c files */

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


/* @(#)va_priv.h	1.15 04 Jun 1995 (UKC) */

/*  Constants describing the variable display formats
 */
typedef enum vformaten {
	DF_NONE,	/* No format, or enum */
	DF_SDEC,	/* Signed decimal */
	DF_UDEC,	/* Unsigned decimal */
	DF_SOCT,	/* Signed octal */
	DF_UOCT,	/* Unsigned octal */
	DF_SHEX,	/* Signed hex */
	DF_UHEX,	/* Unsigned hex */
	DF_ASCII,	/* Ascii character */
	DF_STRING,	/* String */
	DF_UBIN		/* Unsigned binary */
} vformat_t;

/*  The brackets that bound the editable parts of declarations in
 *  the various languages.
 */

#define C_AOPEN		'<'
#define C_ACLOSE	'>'

#define FORTRAN_AOPEN	'['
#define FORTRAN_ACLOSE	']'

/*  Macros which test for a character being the opening or closing
 *  delimiters of a subscript.
 *
 *  We test for a union of the characters used in FORTRAN and C.
 *  This scheme will probably break down if we add more languages.
 */
#define IS_LBRAC(c)	((c) == '[' || (c) == '{' || (c) == ',')
#define IS_RBRAC(c)	((c) == ']' || (c) == '}' || (c) == ',')

/*  Which way the level is going in change_level().
 */
enum leveldir { CL_UP, CL_DOWN };

/*  Size returned by typesize for an object whose size cannot be determined.
 *  Currently the only objects of this sort are FORTRAN dynamic arrays.
 */
#define UNKNOWN_SIZE	(-1)

/*  Impossible addresses returned by dvar_addr() in case of error
 */
#define BAD_ADDR	((taddr_t)0x80000000)

/*  Special value for pointers, integers etc used by epcf90 to mean
 *  unassigned.
 *
 *  BUG: We check this for LANG_F90 - we should be using CT_EPCF90.
 *       So far epcf90 is the only f90 compiler we've used.
 */
#define EPCF90_UNASSIGNED_PATTERN	0x81818181

typedef struct ilistst {
	bool il_low_known;
	bool il_high_known;
	int il_low;
	int il_high;
	size_t il_basesize;
	int il_index;
	struct ilistst *il_next;
} ilist_t;

typedef struct dvarst {
	var_t *dv_var;			/* symbol table info for var */
	taddr_t dv_addr;		/* address of variable */
	short dv_ul_start;		/* char where underlining starts */
	short dv_ilevel;		/* indirection level */
	ilist_t *dv_ilist;		/* list of array indexes */
	vformat_t dv_format;		/* display format */
	unsigned dv_flags;		/* flags - see below */
  	char *dv_va_name;		/* RGA copy of var name */
	struct dvarst *dv_nextfree;	/* for free list management only */
} dvar_t;

#define DVF_VAL_CHANGED		0x001	/* value changed since last time */
#define DVF_NO_TYPEDEFS		0x002	/* don't use typedefs in decls */
#define DVF_BAD_FORMAT_HINT	0x004	/* bad hint - restore old */

/*  Field numbers in a variable object.
 */
#define FN_VAR_DECL	0	/* the declaration */
#define FN_VAR_VALUE	1	/* the value */
#define FN_VAR_LAST	2

/*  'long long' support ?
 */
#if HAVE_LONG_LONG
typedef long long vlong;
typedef unsigned long long uvlong;
#else
typedef long vlong;
typedef unsigned long uvlong;
#endif

/*  Function prototypes.
 */

/*  va_type.c
 */
int dynamic_type_size PROTO((type_t *type, ilist_t *il));
type_t *get_basetype PROTO((type_t *type));
void fix_if_fortran_array_pointer PROTO((type_t *type, taddr_t addr,
					 ilist_t *ilist,
					 bool *p_decl_may_have_changed));
void fix_if_f77_dynamic_array PROTO((type_t *type, taddr_t fp, taddr_t ap,
				     ilist_t *ilist,
				     bool *p_decl_may_have_changed));
type_t *get_type_at_level PROTO((var_t *v, int level));

/*  va_decl.c
 */
const char *mkdecl PROTO((dvar_t *dv));
taddr_t dvar_addr PROTO((dvar_t *dv));
int default_level PROTO((type_t *type, language_t language));
vformat_t default_format PROTO((type_t *vtype, type_t *type));
ilist_t *make_ilist PROTO((type_t *type, language_t language, int level));
ilist_t *dup_ilist PROTO((ilist_t *old));
void free_ilist_list PROTO((ilist_t *ilist));

/*  va_val.c
 */
const char *mkval PROTO((dvar_t *dv));
void int_to_string PROTO((char *buf, size_t buflen, vlong n, vformat_t format));
void enumval_to_string PROTO((char *buf, size_t buflen,
						vlong val, type_t *type));
void addr_to_string PROTO((char *buf, size_t buflen, taddr_t addr,
						type_t *type, vformat_t format));
size_t get_decl_len PROTO((size_t len));
void get_value_colors PROTO((int wn, bool val_changed, long *p_fg, long *p_bg));
const char *deriv_to_string PROTO((typecode_t typecode));
vformat_t mval_to_vformat PROTO((int cmd));
void read_and_show_C_string PROTO((taddr_t addr, char *rbuf, size_t rbufsize,
						 char *obuf, size_t obufsize));
#ifdef OBJ_H_INCLUDED
int var_or_expr_getwidth PROTO((objid_t obj, int fnum, fval_t fval));
#endif
int addr_to_number PROTO((vlong *val, taddr_t addr, type_t *type));
int is_signed_format PROTO((vformat_t format));

/*  va_menu.c
 */
#ifdef OBJ_H_INCLUDED
void update_var PROTO((objid_t obj, bool change_caused_by_edit));
void update_struct PROTO((objid_t par, bool change_caused_by_edit));
#endif
type_t *deref_aggr PROTO((dvar_t *dv));
void change_dv_level PROTO((dvar_t *dv, enum leveldir level));
void redo_decl PROTO((dvar_t *dv));
int va_accelerator_action_from_type PROTO(( type_t* type, vformat_t  fmt )); /* RCB */

/*  va_expr.c
 */
#ifdef OBJ_H_INCLUDED
void do_expr PROTO((objid_t obj, int command, char *arg));

void update_expr PROTO((objid_t obj, vformat_t new_format,
			bool change_caused_by_edit));

taddr_t var_or_expr_addr PROTO((objid_t obj));

void get_expr_info PROTO((objid_t obj, block_t **p_bl, const char **p_text,
			  vformat_t *p_vformat));
type_t *get_expr_type PROTO((objid_t obj));

objid_t restore_displayed_expr PROTO((objid_t par, block_t *bl,
				      const char *text, vformat_t vformat));

bool maybe_restore_var_state PROTO((objid_t par, dvar_t *dv));
#endif

/*  va_state.c
 */
#ifdef OBJ_H_INCLUDED
void note_var_change PROTO((objid_t obj));
taddr_t expand_hierarchy PROTO((objid_t obj, type_t* type));
void expand_statics PROTO(( type_t* type));
#endif
void note_format_change PROTO((dvar_t *dv));
bool get_num PROTO((char **p_line, const char *what, int *p_val));

/*  va_format.c
 */
#ifdef EOF
int write_format_state_to_file PROTO((const char *path, FILE *fp));
#endif
bool maybe_restore_format PROTO((dvar_t *dv));
const char *format_to_string PROTO((vformat_t format));
bool string_to_format PROTO((const char *str, vformat_t *p_format));
void get_variable_colors PROTO((int wn, long *p_bg, long *p_fg));
void update_format_state PROTO((symtab_t *oldst, symtab_t *newst));

/*  va_edit.c
 */
void show_dv_size PROTO((dvar_t* dv));

