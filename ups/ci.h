/* ci.h - public header file for the ci_* package */

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


/* @(#)ci.h	1.20 04 Jun 1995 (UKC) */

#define CI_H_INCLUDED

typedef struct { int pi_dummy; } *parse_id_t;
typedef struct machine_s machine_t;

typedef enum { MA_IGNORE, MA_WARNING_ONLY, MA_DEFAULT } ci_message_action_t;

#if WANT_LDBL
/*  FIX: 'long double' treated as 'double'
 */
typedef double long_double_t;
#endif

#if WANT_LL
/*  FIX: 'long long' does not work
 */
#if HAVE_LONG_LONG
typedef long long long_long_t;
typedef unsigned long long ulong_long_t;
#else
typedef long long_long_t;
typedef unsigned long ulong_long_t;
#endif
#endif

/*  Lexical information - currently a filename and line number.
 *
 *  Used for recording where things are in the source for better
 *  error reporting.
 */
struct lexinfo_s {
	const char *lx_filename;
	int lx_lnum;		/* Line number */
	int lx_cnum;		/* Column number */
};

#ifndef SYMTAB_H_INCLUDED
typedef struct lexinfo_s lexinfo_t;
#endif

typedef bool (*ci_report_error_func_t)PROTO((lexinfo_t *lx, const char *mesg));

const char *ci_translate_escape PROTO((const char *s, int *p_res));

#ifdef SYMTAB_H_INCLUDED
type_t *ci_code_to_type PROTO((typecode_t code));
enum_member_t *ci_make_enum_member PROTO((alloc_pool_t *ap,
					  const char *name, long val));
aggr_or_enum_def_t *ci_make_aggr_or_enum_def PROTO((alloc_pool_t *ap,
						    const char *tag,
						    typecode_t typecode,
						    type_t *type));
func_t *ci_make_func PROTO((alloc_pool_t *ap, const char *name,
				taddr_t addr, symtab_t *symtab,
				fil_t *fil, func_t *next));
fil_t *ci_make_fil PROTO((alloc_pool_t *ap, const char *name, char *data,
			  block_t *block, fil_t *next));
var_t *ci_make_var PROTO((alloc_pool_t *ap, const char *name, class_t class,
						type_t *type, taddr_t addr));
macro_t *ci_define_macro PROTO((alloc_pool_t *ap, macro_t *macrolist,
				int lnum, const char *name, const char *value));
void ci_undef_macro PROTO((alloc_pool_t *ap, macro_t *macrolist,
			   int lnum, const char *name));
var_t *ci_push_vars PROTO((var_t *v, var_t *list));
type_t *ci_make_undef_type PROTO((alloc_pool_t *ap, const char *tag,
					typecode_t typecode, type_t *type));
type_t *ci_make_bitfield_type PROTO((alloc_pool_t *ap, type_t *type,
				     int bit_offset, int bit_width));
type_t *ci_make_type PROTO((alloc_pool_t *ap, typecode_t code));
funcret_t *ci_make_funcret PROTO((alloc_pool_t *ap, params_type_t params_type,
                                  int nparams, bool is_variadic));
typedef_t *ci_make_typedef PROTO((alloc_pool_t *ap,
				  const char *name, type_t *type));
void ci_init_type PROTO((type_t *type, typecode_t typecode));
block_t *ci_make_block PROTO((alloc_pool_t *ap, block_t *parent));
long ci_typesize PROTO((lexinfo_t *lx, type_t *type));
const char *ci_basetype_name PROTO((type_t *type));
char *ci_qualified_basetype_name PROTO((type_t *type));
void ci_make_baseclass_name PROTO(( var_t* v));
char *ci_type_to_english PROTO((type_t *type, bool resolve_typedefs));
char *ci_type_to_decl PROTO((type_t *type, bool resolve_typedefs));
aggr_or_enum_def_t *ci_apply_to_aelist PROTO((
	       aggr_or_enum_def_t *aelist,
	       aggr_or_enum_def_t *(*func)(aggr_or_enum_def_t *ae, const char *farg),
	       const char *arg));
#endif

void ci_add_message_action PROTO((const char *pat, ci_message_action_t action));
void ci_free_all_message_actions PROTO((void));

typedef enum {
	CI_DATA,
	CI_DIRECT_LIBFUNC,
	CI_INDIRECT_LIBFUNC,
	CI_UNDEFINED
} ci_nametype_t;

typedef ci_nametype_t (*ci_getaddrproc_t)PROTO((const char *name,
						unsigned long *p_addr));

/*  BUG: not sure this needs to be here.  Too tired to get this straight now.
 */
typedef struct linkinfo_s linkinfo_t;


#ifdef SYMTAB_H_INCLUDED
typedef int (*ci_resolve_name_func_t)PROTO((const char *name, var_t **p_v));

parse_id_t ci_parse_file PROTO((const char *filename,
				block_t *block, unsigned long flags,
				ci_report_error_func_t report_error_func,
				ci_resolve_name_func_t resolve_name_func,
				const char *(*getline_func)(char *arg),
				char *getline_arg));

/*  We complain about a return statement in the special function $start
 *  with this message.  We put the message here so that the error  output
 *  function can special case it.  We do all this for the benifit of ups,
 *  where we want to give an error message if the user puts a return
 *  statement in breakpoint code (as it won't do what they expect).
 */
extern const char ci_Illegal_return_from_start_message[];

/* Flags for ci_compile().
 */
#define CI_MAKE_EXTERNS_TOPLEVEL 0x0001 /* Treat local extern decls like pcc */
#define CI_DONT_PANIC	0x0004	/* Error message rather than panic on botch */

typedef bool (*ci_checkarg_proc_t)PROTO((type_t *type, int nargs, int argn,
					 long *p_val));
typedef unsigned long (*ci_regno_to_addr_proc_t)PROTO((int regno));
typedef unsigned long (*ci_get_regaddr_proc_t)PROTO((char *arg, int regno));

func_t *ci_get_parse_id_funcs PROTO((parse_id_t parse_id));

linkinfo_t *ci_compile PROTO((const char *srcpath, parse_id_t parse_id,
			      ci_report_error_func_t report_error,
			      ci_checkarg_proc_t checkarg_proc,
			      ci_regno_to_addr_proc_t regno_to_addr_proc,
			      unsigned flags));

void ci_free_linkinfo PROTO((linkinfo_t *li));

machine_t *ci_make_machine_from_linkinfo
                                  PROTO((linkinfo_t *li,
					 const char *entry_name,
					 unsigned stack_size,
					 ci_getaddrproc_t getaddr,
					 ci_get_regaddr_proc_t get_regaddr_proc,
					 char *get_regaddr_proc_arg,
					 ci_report_error_func_t errfunc));

#define CI_CP_CHECKSP	      0x0001 /* Want runtime sp check instructions */
#define CI_CP_DONT_PANIC      0x0002 /* Error mesg rather than panic on botch */
#define CI_CP_CHECK_DIV	      0x0004 /* Want divide by 0 checking div instrs */
#define CI_CP_ONEFILE         0x0008 /* One .c file only - no external data */
#define CI_CP_WANT_REG_RELOCS 0x0010
#define CI_CP_WANT_SYMINFO    0x0020 /* Want symbol table information */

#endif /* SYMTAB_H_INCLUDED */

void ci_free_machine PROTO((machine_t *machine));
void ci_free_parse_id PROTO((parse_id_t parse_id));

typedef unsigned long ci_opcode_t;
ci_opcode_t ci_install_trap_instruction PROTO((machine_t *machine,
					       long location));
void ci_uninstall_trap_instruction PROTO((machine_t *machine, long location,
								ci_opcode_t opcode));

#ifdef EOF
int ci_disassemble_machine PROTO((parse_id_t parse_id, machine_t *machine,
								FILE *fp));
#endif

typedef enum {
	CI_ER_CONTINUE,		/* for indirect functions */
	CI_ER_EXITED,
	CI_ER_TRAP,
	CI_ER_LOADFAULT,
	CI_ER_READDATA_FAILED,
	CI_ER_WRITEDATA_FAILED,
	CI_ER_INDIRECT_CALL_FAILED,
	CI_ER_STACK_OVERFLOW,
	CI_ER_DIVISION_BY_ZERO,
	CI_ER_ARITHMETIC_EXCEPTION,
	CI_ER_BAD_MA_COUNT,
	CI_ER_MEM_BAD,
	CI_ER_MEM_OVERRUN,
	CI_ER_MEM_FREED,
	CI_ER_USER1,
	CI_ER_USER2,
	CI_ER_USER3
} ci_exec_result_t;

const char *ci_exec_result_to_string PROTO((ci_exec_result_t res));

typedef int (*ci_readproc_t)PROTO((unsigned long addr, void *buf,
								size_t nbytes));
typedef int (*ci_writeproc_t)PROTO((unsigned long addr, const void *buf,
								size_t nbytes));
#ifdef SYMTAB_H_INCLUDED
typedef ci_exec_result_t (*ci_indirect_call_proc_t)PROTO((machine_t *machine,
					     unsigned long addr,
					     unsigned long *args,
					     int nargs,
					     unsigned long *p_res,
					     typecode_t restype));
#endif /* SYMTAB_H_INCLUDED */

void ci_initialise_machine PROTO((machine_t *machine, bool reset_data,
				  bool want_opcounts,
				  char **argv, char **envp));

void ci_push_trap_frame PROTO((machine_t *machine));

void ci_catch_arithmetic_signals PROTO((void (*handler)(int sig)));
void ci_set_expecting_arithmetic_signals PROTO((bool expecting_signals));

#ifdef SYMTAB_H_INCLUDED
ci_exec_result_t ci_execute_machine PROTO((machine_t *machine,
			    unsigned long procfp, unsigned long procap,
			    unsigned long procsp, unsigned long proccfa,
			    ci_readproc_t readproc, ci_writeproc_t writeproc,
			    ci_indirect_call_proc_t indirect_call_proc));
#endif /* SYMTAB_H_INCLUDED */
int ci_get_exit_status PROTO((machine_t *ma));
void ci_reset_machine PROTO((machine_t *ma));
#ifdef EOF
int ci_dump_machine_stats PROTO((machine_t *ma, FILE *fp, const char *path));
#endif

bool ci_is_ci_addr PROTO((machine_t *machine, unsigned long addr, size_t len));
