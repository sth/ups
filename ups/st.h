/* st.h - ups symbol table access routines */

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


/* @(#)st.h	1.8 04 Jun 1995 (UKC) */

/*  This file defines the generic interface to the symbol table used
 *  by ups.  Some of the routines are involve a call (directly or
 *  indirectly) via the symbol table type dependent routines from
 *  the st_ops_t structure.  The different symbol table types are:
 *
 *	- a.out symbol table, with #ifdefs in the code to handle
 *	  the various formats.
 *
 *	- C interpreter symbol table.
 *
 *	- an interface to the symbol table routines inside gdb.
 *
 *  The st_*.c files are machine independent - there are no #includes
 *  of system header files relating to executable symbol tables.
 */

#define ST_H_INCLUDED

typedef int (*matchfunc_t)PROTO((const char *name, const char *pat));

typedef struct st_ops_s {
	int (*so_read_text)PROTO((symtab_t *st,
				  taddr_t addr, char *buf, size_t nbytes));
	int (*so_get_min_bpt_addr)PROTO((func_t *f, taddr_t *p_addr));
	taddr_t (*so_get_addr_lim)PROTO((func_t *f));
	
	taddr_t (*so_funcptr_to_addr)PROTO((symtab_t *st, taddr_t addr));
	taddr_t (*so_addr_to_funcptr)PROTO((symtab_t *st, taddr_t addr));

	bool (*so_fil_may_have_matching_globals)PROTO((fil_t *fil,
					const char *pat, matchfunc_t matchf));
	void (*so_close_symtab_data)PROTO((symtab_t *st));

	bool (*so_cblocks_match)PROTO((char *data, char *newdata,
						  bool same_func));
	void (*so_set_cblock_data)PROTO((char **p_data, char *data,
					 bool append));
	void (*so_free_cblock_data)PROTO((char *data));
	bool (*so_cblock_has_var)PROTO((char *data, const char *name));
	var_t *(*so_read_cblock_vars)PROTO((symtab_t *st, char *data,
								taddr_t addr));

	func_t *(*so_ensure_fu)PROTO((func_t *f));
	fil_t *(*so_get_fi)PROTO((symtab_t *st));
	lno_t *(*so_get_fu_lnos)PROTO((func_t *f));
	block_t *(*so_get_fu_blocks)PROTO((func_t *f));
	var_t *(*so_get_fi_vars)PROTO((fil_t *fil));
	macro_t *(*so_get_fi_macros)PROTO((fil_t *fil));

	const char *(*so_disassemble_instruction)PROTO((func_t *f, taddr_t addr,
						        const char *text,
							const char **p_buf));
	struct jump_s *(*so_get_jumps)PROTO((func_t *f, taddr_t addr,
					     const char *text, size_t len,
					     bool want_calls));
} sym_ops_t;

#define st_read_text(st, addr, buf, nbytes) \
	(st->st_ops->so_read_text)(st, addr, buf, nbytes)
#define st_get_min_bpt_addr(f, p_addr) \
	(f->fu_symtab->st_ops->so_get_min_bpt_addr)(f, p_addr)
#define st_get_addr_lim(f) \
	(f->fu_symtab->st_ops->so_get_addr_lim)(f)
#define st_funcptr_to_addr(st, addr) \
	(st->st_ops->so_funcptr_to_addr)(st, addr)
#define st_addr_to_funcptr(st, addr) \
	(st->st_ops->so_addr_to_funcptr)(st, addr)
#define st_fil_may_have_matching_globals(fil, pat, matchf) \
	(fil->fi_symtab->st_ops->so_fil_may_have_matching_globals)(fil, pat, \
								    matchf)
#define st_close_symtab_data(st) \
	(st->st_ops->so_close_symtab_data)(st)

#define st_cblocks_match(cb, newdata, same_func) \
	(cb->cb_symtab->st_ops->so_cblocks_match)(cb->cb_data, newdata, \
								same_func)
#define st_set_cblock_data(cb, data, append) \
	(cb->cb_symtab->st_ops->so_set_cblock_data)(&cb->cb_data,  data, append)
#define st_free_cblock_data(cb) \
	(cb->cb_symtab->st_ops->so_free_cblock_data)(cb->cb_data)
#define st_cblock_has_var(cb, name) \
	(cb->cb_symtab->st_ops->so_cblock_has_var)(cb->cb_data, name)
#define st_read_cblock_vars(cb) \
	(cb->cb_symtab->st_ops->so_read_cblock_vars)(cb->cb_symtab, \
						     cb->cb_data, cb->cb_addr)

#define st_ensure_fu(f) \
	(f->fu_symtab->st_ops->so_ensure_fu)(f)
#define st_get_fi(st) \
	(st->st_ops->so_get_fi)(st)
#define st_get_fu_lnos(f) \
	(f->fu_symtab->st_ops->so_get_fu_lnos)(f)
#define st_get_fu_blocks(f) \
	(f->fu_symtab->st_ops->so_get_fu_blocks)(f)
#define st_get_fi_vars(fil) \
	(fil->fi_symtab->st_ops->so_get_fi_vars)(fil)
#define st_get_fi_macros(fil) \
	(fil->fi_symtab->st_ops->so_get_fi_macros)(fil)

#define st_disassemble_instruction(f, addr, text, len) \
	(f->fu_symtab->st_ops->so_disassemble_instruction)(f, addr, text, len)
#define st_get_jumps(f, addr, text, len, want_calls) \
	(f->fu_symtab->st_ops->so_get_jumps)(f, addr, text, len, want_calls)

typedef struct cblist_s cblist_t;	/* Defined in st_cb.c */
typedef struct functab_s functab_t;	/* Defined in st_fmap.c */
typedef struct addrlist_s addrlist_t;	/* Defined in st_util.c */
typedef struct Module Module;		/* Defined in st_module.c */

typedef enum symtab_type_e {
	ST_NONE,
	ST_STABS,
	ST_DWARF,
	ST_ThirdEye		/* probably obsolete */
} symtab_type_t;

/*  A generic symbol table.
 */
struct symtab_s {
	alloc_pool_t *st_apool;	/* Alloc pool of this symtab */
	const char *st_path;	/* Path of file from which symtab was read */

	fil_t *st_sfiles;	/* List of source files */
	cblist_t *st_cblist;	/* List of FORTRAN common blocks */
	Module *st_modules;	/* List of modules (currently only f90) */
	functab_t *st_functab;	/* Addr --> func mapping table */
	addrlist_t *st_addrlist;/* List of addresses of globals */
	func_t *st_funclist;	/* List of functions */

	char *st_data;		/* Pointer to symtab type dependent data */
	sym_ops_t *st_ops;	/* Symtab type dependent operations */
	long st_modtime;	/* RGA for checking if reload reqd */
	int st_eclipsed;	/* RGA hide if eclipsed by a relink */
	char *st_func_hv;	/* RGA so hash table can be free'd for new target */
};

typedef struct common_block_s common_block_t;

/*  Function prototypes.  BUG: I'm not sure they belong here.
 *  Also, typesize is defined in va_*.c and used in st_*.c - dubious.
 */
block_t *get_fu_blocks PROTO((func_t *f));
lno_t *get_fu_lnos PROTO((func_t *f));
int typesize PROTO((type_t *type));
func_t *addr_to_func PROTO((taddr_t addr));
int find_global_by_name PROTO((const char *name, fil_t *fil, func_t *f,
			       common_block_t *cblock,
			       bool exact, func_t **p_f, func_t **p_f1,
			       var_t **p_v, var_t **p_v1,
			       common_block_t **p_cb,
			       fil_t **p_fil, fil_t **p_fil1,
			       bool use_demangled_name));
int resolve_untyped_name PROTO((const char *name, var_t **p_v));
int find_func_by_name PROTO((const char *name, func_t **p_f, func_t **p_f1,
			     bool use_demangled_name));

func_t *name_and_fil_to_func PROTO((const char *name, fil_t *fil,
				    bool demangled_name));
fil_t *name_to_fil PROTO((const char *name));
fil_t *name_and_st_to_fil PROTO((const char *name, symtab_t *st));
int find_and_show_matching_globals PROTO((fil_t *fil, const char *fi_name,
					  int fi_name_len, char *name));

var_t *get_cblock_vars PROTO((common_block_t *cb));
const char *get_cblock_name PROTO((common_block_t *cb));
func_t *get_cblock_func PROTO((common_block_t *cb));
const char *get_cblock_funcname PROTO((common_block_t *cb));
const symtab_t *get_cblock_symtab PROTO((common_block_t *cb));
fil_t *get_cblock_fil PROTO((common_block_t *cb));
void iterate_over_common_blocks PROTO((void (*func)(common_block_t *cb)));
common_block_t *name_to_cblock PROTO((const char *name, func_t *f));
common_block_t *name_and_st_to_cblock PROTO((const char *name, func_t *f,
                                             symtab_t *st));

void iterate_over_all_modules PROTO((void (*func)(Module *module)));

typedef int (*iof_func_t)PROTO((func_t *f, taddr_t addr,
						char *arg1, char *arg2));
int iterate_over_functions PROTO((symtab_t *st, iof_func_t func,
						char *arg1, char *arg2));
void iterate_over_vars_of_block PROTO((block_t *block,
				       void (*func)(var_t *v, char *args),
				       char *args));

taddr_t get_addr_lim_via_functab PROTO((func_t *f));
void iterate_over_source_files PROTO((void (*func)PROTO((fil_t *fil, bool arg)),
							 bool arg));

void set_highlighted_line PROTO((fil_t *fil, int lnum));
int get_mangled_name_info PROTO((int var, char *name, char **name_start,
				 int *name_len, char **class_name,
				 int *class_len)); /* RGA */
void demangle_name PROTO((char *name, int len, alloc_pool_t *alloc_id,
			  char **ptr, int func, Compiler_type compiler));
void demangle_name_2 PROTO((char *name, int len, alloc_pool_t *alloc_id,
			  char **ptr, int func, fil_t *fil));
char* demangle_name_3 PROTO((char* name));
const char *macro_expand_word PROTO((fil_t *fil, int lnum,
				     const char *w, int wl));
char *macro_expand_string PROTO((fil_t *fil, int lnum, const char *s));

/*  From st_fmap.c
 */
int addrcmp PROTO((func_t *f1, func_t *f2));
void adjust_functab_text_addr_base PROTO((functab_t *functab,
					  func_t *funclist, long delta));
funclist_t *new_flist PROTO((void));
void free_flist_list PROTO((funclist_t *fl));
bool addr_and_functab_to_func PROTO((functab_t *functab, taddr_t addr,
				     func_t **p_func));
void make_funcinfo PROTO((alloc_pool_t *ap, func_t *flist, int flist_len,
			  taddr_t first_addr, taddr_t last_addr,
			  long addr_offset,
			  func_t **p_funclist, functab_t **p_functab));
void delete_duplicate_funcs PROTO((fil_t *fil));

/*  From st_cb.c
 */
void free_cblist_info PROTO((cblist_t *cblist));
void global_and_cblist_to_var PROTO((cblist_t *cblist, const char *name,
				     func_t *f, common_block_t **p_cblock,
				     fil_t **p_fil, var_t **p_var));
void iterate_over_cblist PROTO((cblist_t *cblist,
				void (*func)(common_block_t *cb)));
void add_common_block PROTO((symtab_t *st, fil_t *fil, func_t *f,
			     const char *name, bool empty, char *data));
void finish_common_blocks PROTO((symtab_t *st));

/*  From st_module.c
 */
void add_module_function PROTO((symtab_t *st, const char *modname, func_t *f));
void iterate_over_modules PROTO((Module *modules,
				 void (*func)(Module *module)));
const char *get_module_name PROTO((Module *module));
funclist_t *get_module_funclist PROTO((Module *module));
size_t get_module_max_namelen PROTO((Module *module));
Module *get_next_module PROTO((Module *module));


/*  From st_util.c
 */
symtab_t *make_symtab PROTO((alloc_pool_t *ap, const char *name,
			     fil_t *sfiles, func_t *funclist,
			     sym_ops_t *ops, char *data));
void adjust_addrlist_addr_offset PROTO((addrlist_t *addrlist, long delta));
void insert_global_addr PROTO((alloc_pool_t *ap, addrlist_t **p_addrlist,
			       const char *name, taddr_t addr));
taddr_t find_addr_in_addrlist PROTO((addrlist_t *addrlist, const char *name));
void scan_addrlist PROTO((symtab_t *st, matchfunc_t matchf, const char *pat,
			  void (*addf)(char *, var_t *), char *addf_arg));
taddr_t lookup_global_addr PROTO((symtab_t *st, const char *name));
const char* lookup_global_name PROTO(( taddr_t addr, size_t* offset)); /* RCB */
void close_symtab PROTO((symtab_t *st));
block_t *get_rootblock PROTO((void));

bool addr_to_lno PROTO((func_t *f, taddr_t text_addr, lno_t **p_lno,
			bool same_file));
int rbrac_addr_to_lnum PROTO((func_t *f, taddr_t text_addr));
taddr_t lnum_and_fil_to_addr PROTO((func_t *f, fil_t *fil, int lnum));
void addr_to_fil_and_lnum PROTO((func_t *f, taddr_t addr,
				 fil_t **p_fil, int *p_lnum, bool same_file));
int get_min_bpt_addr PROTO((func_t *f, taddr_t *p_addr, bool same_file));
int map_lnum_to_addr PROTO((func_t *f, fil_t *fil, int lnum, taddr_t *p_addr));
bool open_source_file PROTO((fil_t *fil, bool rematch, bool reload));
bool have_source_file PROTO((fil_t *fil));
void set_use_srcpath_only_flag PROTO((void));
void get_matching_func_bp_list PROTO((const char *funcname,
				      func_bp_list_t **funchead));
void do_free_func_bp_list_list PROTO((func_bp_list_t *funchead));
void free_library_load_list PROTO((void));
int get_noload_list PROTO((const char ***names));
void add_to_noload_list PROTO((const char *name, int loaded));
void free_library_noload_list PROTO((void));

void add_to_load_path PROTO((const char *path, int noload, int load_requested));
int user_wants_library_loaded PROTO((char *name));
void show_file_and_target_dates PROTO((fil_t *f, symtab_t *fu_symtab));
int find_duplicate_file PROTO((fil_t *fil, fil_t **fil1));
int construct_source_path_used PROTO((fil_t *fil, char *buff));
void restore_cached_symtab_breakpoints PROTO((void));
void save_matching_breakpoints PROTO((symtab_t *st, fil_t *fil, char *handle));
void collapse_srcfiles PROTO((void));
bool scan_elf_dot_o_file_symtab PROTO((fil_t *fil));
char *get_temp_state_filename PROTO((void));
void mark_duplicate_files PROTO((void));
