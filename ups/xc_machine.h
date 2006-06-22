/* xc_machine.h - header file for xc_machine.c */

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


/* @(#)xc_machine.h	1.1 22/12/93 (UKC) */

#define XC_MACHINE_H_INCLUDED

#define WANT_TYPE_PUSHED 1

typedef unsigned char textword_t;
typedef unsigned long stackword_t;
typedef unsigned long textaddr_t;
typedef stackword_t (*libfunc_addr_t)PROTO((stackword_t, ...));

typedef struct {
	struct codefile_s *er_file;
	bool er_indirect;
	textaddr_t er_addr;
} extref_t;

typedef struct callback_list_s {
	stackword_t cl_func;
	int cl_nargs;
	struct callback_list_s *cl_next;
} callback_list_t;

typedef struct codefile_s {
	extref_t **cf_funcrefs;
	char **cf_varaddrs;
	const char *cf_path;
	long cf_offset;
	bool cf_loaded;
	textword_t *cf_text_and_data;
	char *cf_data_copy;
	size_t cf_text_size;
	size_t cf_data_size;
	size_t cf_bss_size;
	textaddr_t *cf_funcaddrs;
	int cf_num_funcs;

	/*  Debugging information
	 */
	const char *cf_srcpath;
	const char **cf_globalnames;
	int cf_nglobalnames;
} codefile_t;

typedef struct reg_reloc_s {
	long rr_addr;		/* relative to cf_text */
	int rr_regno;
	struct reg_reloc_s *rr_next;
} reg_reloc_t;

typedef struct funcmap_s {
	const char *fm_name;
	int fm_file_index;
	textaddr_t fm_addr;
} funcmap_t;

#define SETJMP_NWORDS	 4

typedef union {
	unsigned char d_uchar;
	char d_char;
	unsigned short d_ushort;
	short d_short;
	unsigned int d_ulong;
	int d_long;
	float d_float;
	stackword_t d_word;
#if WANT_LDBL
	stackword_t d_words[LDOUBLE_NSLOTS];
#else
	stackword_t d_words[DOUBLE_NSLOTS];
#endif
	double d_double;
#if WANT_LL
	long_long_t d_longlong;
	ulong_long_t d_ulonglong;
#endif
#if WANT_LDBL
	long_double_t d_longdouble;
#endif
} func_retval_t;

/*  Setjmp structure - must change setjmp.h if you change this.
 */
typedef struct {
	codefile_t *ms_cf;
	textword_t *ms_pc;
        stackword_t *ms_fp;
	stackword_t *ms_sp;
	stackword_t ms_stackwords[SETJMP_NWORDS];
} machine_state_t;

typedef struct mem_s {
	union {
		double mu_align;
		struct {
			struct mem_s *mu_next;
			struct mem_s *mu_prev;
			size_t mu_size;
			int mu_start_marker;
		} s;
	} u;
} mem_t;

#define m_next		u.s.mu_next
#define m_prev		u.s.mu_prev
#define m_size		u.s.mu_size
#define m_start_marker	u.s.mu_start_marker

struct machine_s {
	alloc_pool_t *ma_apool;
	unsigned long *ma_opcounts;

	/*  External data and bss.
	 */
	char *ma_data;
	char *ma_data_copy;
	size_t ma_data_size;
	size_t ma_bss_size;

	/*  Only used when linking from .ocx files.
	 */
	int (*ma_loadfunc)PROTO((alloc_pool_t *ap, const char *cwd,
					   codefile_t *cftab, codefile_t *cf));
	const char *ma_cwd;
	codefile_t *ma_entry_codefile;
	textaddr_t ma_entry_point;
	textword_t *ma_argc_pos;
	textword_t *ma_argv_pos;
	textword_t *ma_envp_pos;
	codefile_t *ma_cftab;		/* for translating func addrs */
	size_t ma_cftab_size;

	callback_list_t *ma_callbacks;
	callback_list_t *ma_free_callbacks;

	/*  These are set once and never changed.
	 */
	unsigned long (*ma_get_regaddr_proc)PROTO((char *arg, int regno));
	char *ma_get_regaddr_proc_arg;
	reg_reloc_t *ma_reg_relocs;
	libfunc_addr_t *ma_libfuncs;
	stackword_t *ma_maxsp;
	stackword_t *ma_initial_sp;
	stackword_t *ma_minsp;
	size_t ma_stack_size;

	/*  The current state of the machine.
	 */
	textword_t *ma_pc;
	stackword_t *ma_sp;
	stackword_t *ma_fp;
	codefile_t *ma_codefile;
	func_retval_t ma_retval;
	ci_exec_result_t ma_stopres;

	/*  Resources held by the currently executing machine.
	 */
	char *ma_fdtab;		/* File descriptors `owned' */
	size_t ma_fdtab_size;
	mem_t ma_mhead;	     /* Doubly linked circular list of malloc blocks */

	/*  Symbol table information.  Needed for debugging only, not for
	 *  execution.
	 */
	funcmap_t *ma_funcmap;
	size_t ma_funcmap_size;
};

#ifndef CI_H_INCLUDED
typedef struct machine_s machine_t;
#endif

#define XLOAD(ma, cf)	((*ma->ma_loadfunc)(ma->ma_apool, ma->ma_cwd, \
							    ma->ma_cftab, cf))

/*  The (unsigned) casts here are because unsigned char promotes to
 *  int under ANSI C, and the bitwise operators are (I think) not as
 *  well defined for int as for unsigned.  The casts also suppress
 *  buggy warning messages from the Ultrix C compiler.
 */
#define UBYTE(c)	((unsigned long)(unsigned char)c)
#define GETWORD(pc)	(UBYTE(*(pc)) | UBYTE((pc)[1] << 8))
#define GETLONG(pc)	(UBYTE(*(pc)) | UBYTE((pc)[1] << 8) | \
			 UBYTE((pc)[2] << 16) | UBYTE((pc)[3] << 24))
#if SIZEOF_LONG > 4
#define GETQUAD(pc)	(UBYTE(*(pc)) | UBYTE((pc)[1] << 8) | \
			 UBYTE((pc)[2] << 16) | UBYTE((pc)[3] << 24) | \
			 UBYTE((pc)[4] << 32) | UBYTE((pc)[5] << 40) | \
			 UBYTE((pc)[6] << 48) | UBYTE((pc)[7] << 56))
#else
#define GETQUAD(pc)	(0UL)
#endif
#define GETOPCODE(pc)	((opcode_t)GETWORD(pc))
#define PREVOPCODE(pc)	(GETOPCODE((pc) - 2))
#define READOPCODE(pc)	((pc) += 2, PREVOPCODE(pc))

#define PUTOPCODE(pc, op) \
	(pc)[0] = ((unsigned long)(op) & 0xff); \
	(pc)[1] = (((unsigned long)(op) >> 8 ) & 0xff);
#define WRITEOPCODE(pc, op) \
	*(pc)++ = ((unsigned long)(op) & 0xff); \
	*(pc)++ = (((unsigned long)(op) >> 8 ) & 0xff);

typedef struct {
	textword_t *su_text;
	size_t su_text_size;
	size_t su_start_addr;
	int su_argv_pos;
	int su_argc_pos;
	int su_envp_pos;
	size_t su_setjmp_addr;
	size_t su_longjmp_addr;
} startup_info_t;

machine_t *ci_create_machine PROTO((alloc_pool_t *ap, size_t stack_nwords,
				    codefile_t *cftab, size_t cftab_size,
				    codefile_t *entry_codefile,
				    size_t entry_point));

void ci_make_crt0 PROTO((alloc_pool_t *ap, int main_index, int exit_index,
			 startup_info_t *su));

void ci_write_text_long PROTO((textword_t *text, unsigned long val));

typedef int (*callback_t)PROTO((stackword_t a0, ...));

callback_t ci_make_callback PROTO((stackword_t cmpfunc, int nargs));

stackword_t ci_set_sig_handler PROTO((int sig, stackword_t func));

void ci_register_fd PROTO((machine_t *ma, int fd));
void ci_unregister_fd PROTO((machine_t *ma, int fd));
bool ci_fd_is_registered PROTO((machine_t *ma, int fd));
void ci_cause_exit PROTO((machine_t *ma));

voidptr ci_mem_malloc PROTO((machine_t *ma, size_t size));
void ci_mem_free PROTO((machine_t *ma, voidptr p));
voidptr ci_mem_realloc PROTO((machine_t *ma, voidptr p, size_t size));
