/* target.h - public header file for target.c */

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


/* @(#)target.h	1.6 24 May 1995 (UKC) */

#define TARGET_H_INCLUDED

#ifndef SYMTAB_H_INCLUDED
typedef struct symtab_s symtab_t;
#endif

/*  Various states of of the target process.
 */
typedef enum tstateen {
	TS_STOPPED,		/* stopped (bpt, signal or just attached) */
	TS_HALTED,		/* stopped by fatal signal (can't continue) */
	TS_CORE,		/* debugging from a core file */
	TS_NOTR,		/* not run yet, and no core file */
	TS_RUNNING,		/* running (transient - see ui_menu.c */
	TS_SEARCHING		/* stopped, and searching for a symbol */
} tstate_t;

/*  Type for a target opcode.  Used in xp_tswap().  Must be large
 *  enough for all target types on all architectures.
 */
typedef unsigned long xp_opcode_t;

/*  What state signal handling is in for a given signal.
 */
typedef enum sigstateen {
	SGH_DEFAULT,	/* SIG_DFL - default */
	SGH_IGNORED,	/* SIG_IGN - signal is ignored */
	SGH_CAUGHT	/* signal is caught */
} sigstate_t;

/*  Why the process stopped after xp_cont() or xp_start().
 */
typedef enum stopresen {
	SR_SIG,		/* got a signal */
	SR_BPT,		/* hit a breakpoint */
	SR_WPT,		/* hit a watchpoint */
	SR_DIED,	/* process exited */
	SR_SSTEP,	/* stopped after a single step */
	SR_USER,	/* user requested stops */
	SR_FAILED	/* couldn't restart target (breakpoint problems) */
} stopres_t;

typedef union {
	float f;
	double d;
#if HAVE_LONG_DOUBLE
	long double ld;
#endif
} fpval_t;

/*  Register type for xp_readreg().  Positive values are general
 *  purpose registers.
 *
 *  Not all of these are valid for all architectures.  For example,
 *  the MIPS chip has no condition codes, and the VAX is the only
 *  machine with an argument pointer (UPSREG_AP) distinct from the
 *  frame pointer.
 */
#define UPSREG_PC			(-1)	/* program counter */
#define UPSREG_SP			(-2)	/* stack pointer */
#define UPSREG_AP			(-3)	/* argument ptr (VAX only) */
#define UPSREG_FP			(-4)	/* frame pointer */
#define UPSREG_CONDITION_CODES		(-5)	/* condition codes */
#define UPSREG_FP_CONDITION_CODES	(-6)	/* float condition codes */

#define REG_MIN_REGNO			(-6)

/*  Offset of PC from breakpoint instruction when we stop.
 */
#ifdef ARCH_SUN3
#define BPT_PC_OFFSET			2
#else
#ifdef ARCH_386
#define BPT_PC_OFFSET			1
#else
#define BPT_PC_OFFSET			0
#endif
#endif

typedef struct {
	int si_signo;
	taddr_t si_fp;
	char *si_predata;	/* generic preamble data (like func preamble) */
} Siginfo;

#ifdef SYMTAB_H_INCLUDED
/*  Element of the linked list describing the stack.
 */
struct stack_s {
	func_t *stk_func;
	taddr_t stk_pc;			/* saved program counter */
	taddr_t stk_fp;			/* frame pointer */
	taddr_t stk_sp;			/* stack pointer (only for $sp) */
	taddr_t stk_ap;			/* argument pointer (VAX a.out only) */
	fil_t *stk_fil;
	int stk_lnum;			/* source lnum matching stk_pc */
	Siginfo *stk_siginfo;		/* signal that caused func call, or 0 */
	bool stk_user_changed_vars;	/* user changed vars display */
	bool stk_bad;			/* stack corrupted after this frame */
	struct stack_s *stk_inner;	/* Stack describing next frame in */
	struct stack_s *stk_outer;	/* Stack describing next frame out */
	char *stk_data;			/* Additional target-specific data */
};
#endif

typedef struct breakpoint_s breakpoint_t;

typedef struct watchpoint_s watchpoint_t;

typedef struct stack_s Stack;

typedef struct {
	/*  These fields are set by make_target()
	 */
	alloc_pool_t *xp_apool;
	const char *xp_textpath;	/* Path to (main) text file */
	taddr_t xp_entryaddr;		/* Program entry point address */
	struct func_s *xp_mainfunc;	/* Initial function */
	struct func_s *xp_initfunc;	/* _init function before mainfunc */
	long xp_modtime;		/* Last modified time of text file */
	struct xp_ops_s *xp_ops;	/* Target functions */

	bool xp_words_big_endian;
	bool xp_bits_big_endian;

	/*  These fields are set up by the target dependent init routines.
	 */
	tstate_t xp_state;
	char *xp_data;			/* Private target data */
 	symtab_t *xp_symtabs;

	bool xp_new_dynamic_libs;	/* RGA set if dynamic libs added */
	bool xp_new_dynamic_libs_loaded; /* RGA set if dynamic libs loaded */
	bool xp_hit_solib_event;	/* RGA if bpt in dynamic linker hit */
	bool xp_restore_bpts;		/* RGA restore old bpts after attach */
	bool xp_target_updated;   /* RGA reread main symtab */
	bool xp_target_name_changed;    /* RGA when target name has been edited */
} target_t;

typedef union text_block_s {
	char buf[512];
	long words;		/* force long alignmemt */
} text_block_t;

/*  How to continue the target in xp_restart_child().
 */
typedef enum cont_type_e {
	CT_STEP,		/* single step */
	CT_CONT			/* continue to signal, breakpoint or exit */
} cont_type_t;

/*  Protection types for xp_set_protection().
 */
typedef enum prot_type_e {
	PT_NONE,		/* no access */
	PT_RDONLY,		/* read access */
	PT_WRONLY,		/* write access */
	PT_RDWR,		/* read and write access */
	PT_ALL			/* full access */
} prot_type_t;

/*  Types of the routines.
 */
typedef struct xp_ops_s {
	/*  Initialisation.  No xp_*() #defines for these.
	 */
	int (*xo_preinitialise)PROTO((void));
	int (*xo_initialise)PROTO((ebuf_t *eb, char **argv));
	void (*xo_show_target_driver_info)PROTO((bool name_only));
 	bool (*xo_match)PROTO((const char *textpath, text_block_t *tb,
			       size_t nbytes));
	int (*xo_init_from_textfile)PROTO((target_t *xp, int textfd,
					   const char *corepath,
					   bool user_gave_core,
					   const char **p_cmdline));

	bool (*xo_next_symtab)PROTO((target_t *xp, symtab_t *st, bool load_new,
				     symtab_t **p_next_st));

	/*  High level process control.
	 */
	stopres_t (*xo_start)PROTO((target_t *xp));
	stopres_t (*xo_step)PROTO((target_t *xp));
	stopres_t (*xo_next)PROTO((target_t *xp));
	stopres_t (*xo_cont)PROTO((target_t *xp));
	void (*xo_kill)PROTO((target_t *xp));
	
	/*  Low level process control - routines used for both
	 *  xc_* and ao_* targets.
	 *
	 *  TODO: put these elsewhere.
	 */
	int (*xo_create_child)PROTO((target_t *xp,
				     const char **argv, const char **envp,
				     long rdlist, stopres_t *p_stopres));
	stopres_t (*xo_restart_child)PROTO((target_t *xp, int sig,
					    cont_type_t ctype));
	void (*xo_set_base_sp)PROTO((target_t *xp, taddr_t sp));
	taddr_t (*xo_get_base_sp)PROTO((target_t *xp));

	/*  Breakpoints.
	 *
	 *  QUERY: is the target_t parameter needed for these?
	 */
	breakpoint_t *(*xo_add_breakpoint)PROTO((target_t *xp, taddr_t addr));
	int (*xo_remove_breakpoint)PROTO((target_t *xp, breakpoint_t *bp));
	int (*xo_enable_breakpoint)PROTO((target_t *xp, breakpoint_t *bp));
	int (*xo_disable_breakpoint)PROTO((target_t *xp, breakpoint_t *bp));
	breakpoint_t *(*xo_addr_to_breakpoint)PROTO((target_t *xp,
						     taddr_t addr));
	taddr_t (*xo_get_breakpoint_addr)PROTO((target_t *xp,
						breakpoint_t *bp));
	
	/*  Watchpoints.
	 *
	 *  QUERY: is the target_t parameter needed for these?
	 */
	watchpoint_t *(*xo_add_watchpoint)PROTO((target_t *xp, taddr_t addr,
						 size_t size));
	int (*xo_remove_watchpoint)PROTO((target_t *xp, watchpoint_t *wp));
	void (*xo_enable_watchpoint)PROTO((target_t *xp, watchpoint_t *wp));
	void (*xo_disable_watchpoint)PROTO((target_t *xp, watchpoint_t *wp));
	int (*xo_install_watchpoint)PROTO((target_t *xp, taddr_t addr,
					   size_t size));
	int (*xo_uninstall_watchpoint)PROTO((target_t *xp, taddr_t addr,
					     size_t size, int watchnum));

	/*  Attaching to processes.
	 */
	bool (*xo_is_attached)PROTO((target_t *xp));
	void (*xo_detach)PROTO((target_t *xp));

	/*  Information.
	 */
	int (*xo_get_addrsize)PROTO((target_t *xp));
	tstate_t (*xo_get_state)PROTO((target_t *xp));
	int (*xo_get_lastsig)PROTO((target_t *xp));
	stopres_t (*xo_get_stopres)PROTO((target_t *xp));
	sigstate_t (*xo_get_sigstate)PROTO((target_t *xp, int sig));

	/*  Stack routines.
	 */
	Stack *(*xo_get_stack_trace)PROTO((target_t *xp));
	taddr_t (*xo_get_reg_addr)PROTO((target_t *xp, Stack *stk, int reg));
	const char *(*xo_get_signal_tag)PROTO((target_t *xp, int signo));

	/*  Reading and writing data.
	 */
	int (*xo_read_fpval)PROTO((target_t *xp, taddr_t addr,
				   int num_bytes, fpval_t *p_val));
	int (*xo_read_fpreg)PROTO((target_t *xp, int reg,
				   int num_bytes, fpval_t *p_val));
	int (*xo_readreg)PROTO((target_t *xp, int reg, taddr_t *p_regval));
	int (*xo_setreg)PROTO((target_t *xp, int reg, taddr_t regval));
	int (*xo_readdreg)PROTO((target_t *xp, int reg, taddr_t *p_regval));
	int (*xo_setdreg)PROTO((target_t *xp, int reg, taddr_t regval));
	taddr_t (*xo_get_retaddr_after_jsr)PROTO((target_t *xp,
						  int rlink_reg));
	taddr_t (*xo_get_retaddr_after_sig)PROTO((target_t *xp));
	int (*xo_tswap)PROTO((target_t *xp, taddr_t addr, xp_opcode_t opcode,
						    xp_opcode_t *p_old_opcode));
	int (*xo_read_data)PROTO((target_t *xp,
				  taddr_t addr, char *buf, size_t nbytes));
	int (*xo_write_data)PROTO((target_t *xp,
				   taddr_t addr, const char *buf, size_t nbytes));
	int (*xo_read_text_from_process)PROTO((target_t *xp,
				   taddr_t addr, char *buf, size_t nbytes));

	/*  Miscellaneous.
	 */
	int (*xo_load_library)PROTO((target_t *xp, char *arg));

	int (*xo_call_func)PROTO((target_t *xp, char *cma, taddr_t addr,
				  taddr_t *args, size_t *argsizes, int nargs,
				  taddr_t *p_res, typecode_t restype,
				  const char **p_mesg));
	void (*xo_handle_signal_change)PROTO((target_t *xp, int signo,
					      bool ignore, bool redraw,
					      bool stop));
	bool (*xo_do_debug_command)PROTO((target_t *xp, const char *cmd,
					  size_t cmdlen, const char *arg));
  	func_t *(*xo_decode_plt)PROTO((target_t *xp, taddr_t pc));
	void (*xo_cancel_lastsig)PROTO((target_t *xp));
	int (*xo_set_protection)PROTO((target_t *xp, taddr_t addr, size_t size,
					prot_type_t ptype));
} xp_ops_t;

#define xp_init_from_textfile(xp, textfd, corepath, user_gave_core, p_cmdline) \
	(xp->xp_ops->xo_init_from_textfile)(xp, textfd, corepath, \
						user_gave_core, p_cmdline)

#define xp_next_symtab(xp, st, load_new, p_next_st) \
	(xp->xp_ops->xo_next_symtab)(xp, st, load_new, p_next_st)

#define xp_start(xp) \
	(xp->xp_ops->xo_start)(xp)
#define xp_next(xp) \
	(xp->xp_ops->xo_next)(xp)
#define xp_step(xp) \
	(xp->xp_ops->xo_step)(xp)
#define xp_cont(xp) \
	(xp->xp_ops->xo_cont)(xp)
#define xp_kill(xp) \
	(xp->xp_ops->xo_kill)(xp)

#define xp_set_base_sp(xp, sp) \
	(xp->xp_ops->xo_set_base_sp)(xp, sp)
#define xp_get_base_sp(xp) \
	(xp->xp_ops->xo_get_base_sp)(xp)
#define xp_create_child(xp, argv, envp, rdlist, p_stopres) \
	(xp->xp_ops->xo_create_child)(xp, argv, envp, rdlist, p_stopres)
#define xp_restart_child(xp, sig, ctype) \
	(xp->xp_ops->xo_restart_child)(xp, sig, ctype)

#define xp_add_breakpoint(xp, addr) \
	(xp->xp_ops->xo_add_breakpoint)(xp, addr)
#define xp_remove_breakpoint(xp, bp) \
	(xp->xp_ops->xo_remove_breakpoint)(xp, bp)
#define xp_enable_breakpoint(xp, bp) \
	(xp->xp_ops->xo_enable_breakpoint)(xp, bp)
#define xp_disable_breakpoint(xp, bp) \
	(xp->xp_ops->xo_disable_breakpoint)(xp, bp)
#define xp_addr_to_breakpoint(xp, addr) \
	(xp->xp_ops->xo_addr_to_breakpoint)(xp, addr)
#define xp_get_breakpoint_addr(xp, bp) \
	(xp->xp_ops->xo_get_breakpoint_addr)(xp, bp)

#define xp_add_watchpoint(xp, addr, size) \
	(xp->xp_ops->xo_add_watchpoint)(xp, addr, size)
#define xp_remove_watchpoint(xp, wp) \
	(xp->xp_ops->xo_remove_watchpoint)(xp, wp)
#define xp_enable_watchpoint(xp, wp) \
	(xp->xp_ops->xo_enable_watchpoint)(xp, wp)
#define xp_disable_watchpoint(xp, wp) \
	(xp->xp_ops->xo_disable_watchpoint)(xp, wp)
#define xp_install_watchpoint(xp, addr, size) \
	(xp->xp_ops->xo_install_watchpoint)(xp, addr, size)
#define xp_uninstall_watchpoint(xp, addr, size, watchnum) \
	(xp->xp_ops->xo_uninstall_watchpoint)(xp, addr, size, watchnum)

#define xp_is_attached(xp) \
	(xp->xp_ops->xo_is_attached)(xp)
#define xp_detach(xp) \
	(xp->xp_ops->xo_detach)(xp)

#define xp_get_addrsize(xp) \
	(xp->xp_ops->xo_get_addrsize)(xp)
#define xp_get_state(xp) \
	(xp->xp_ops->xo_get_state)(xp)
#define xp_get_lastsig(xp) \
	(xp->xp_ops->xo_get_lastsig)(xp)
#define xp_get_stopres(xp) \
	(xp->xp_ops->xo_get_stopres)(xp)
#define xp_get_sigstate(xp, sig) \
	(xp->xp_ops->xo_get_sigstate)(xp, sig)

#define xp_get_stack_trace(xp) \
	(xp->xp_ops->xo_get_stack_trace)(xp)
#define xp_get_signal_tag(xp, signo) \
	(xp->xp_ops->xo_get_signal_tag)(xp, signo)
#define xp_get_reg_addr(xp, stk, reg) \
	(xp->xp_ops->xo_get_reg_addr)(xp, stk, reg)

#define xp_read_fpval(xp, addr, num_bytes, p_val) \
	(xp->xp_ops->xo_read_fpval)(xp, addr, num_bytes, p_val)
#define xp_read_fpreg(xp, addr, num_bytes, p_val) \
	(xp->xp_ops->xo_read_fpreg)(xp, addr, num_bytes, p_val)
#define xp_readreg(xp, reg, p_regval) \
	(xp->xp_ops->xo_readreg)(xp, reg, p_regval)
#define xp_setreg(xp, reg, p_regval) \
	(xp->xp_ops->xo_setreg)(xp, reg, p_regval)
#define xp_readdreg(xp, reg, p_regval) \
	(xp->xp_ops->xo_readdreg)(xp, reg, p_regval)
#define xp_setdreg(xp, reg, p_regval) \
	(xp->xp_ops->xo_setdreg)(xp, reg, p_regval)
#define xp_get_retaddr_after_jsr(xp, rlink_reg) \
	(xp->xp_ops->xo_get_retaddr_after_jsr)(xp, rlink_reg)
#define xp_get_retaddr_after_sig(xp) \
	(xp->xp_ops->xo_get_retaddr_after_sig)(xp)
#define xp_tswap(xp, addr, opcode, p_old_opcode) \
	(xp->xp_ops->xo_tswap)(xp, addr, opcode, p_old_opcode)
#define xp_read_data(xp, addr, buf, nbytes) \
	(xp->xp_ops->xo_read_data)(xp, addr, buf, nbytes)
#define xp_write_data(xp, addr, buf, nbytes) \
	(xp->xp_ops->xo_write_data)(xp, addr, buf, nbytes)
#define xp_read_text_from_process(xp, addr, buf, nbytes) \
	(xp->xp_ops->xo_read_text_from_process)(xp, addr, buf, nbytes)

#define xp_load_library(xp, arg) \
	(xp->xp_ops->xo_load_library)(xp, arg)
#define xp_call_func(xp, ma, addr, args, argsizes, nargs, p_res, restype, p_mesg) \
	(xp->xp_ops->xo_call_func)(xp, ma, addr, args, argsizes, nargs, \
						  p_res, restype, p_mesg)
#define xp_handle_signal_change(xp, signo, ignore, redraw, stop) \
	(xp->xp_ops->xo_handle_signal_change)(xp, signo, ignore, redraw, stop)
#define xp_do_debug_command(xp, cmd, cmdlen, arg) \
	(xp->xp_ops->xo_do_debug_command)(xp, cmd, cmdlen, arg)
#define xp_decode_plt(xp, pc) \
	(xp->xp_ops->xo_decode_plt)(xp, pc)
#define xp_cancel_lastsig(xp) \
	(xp->xp_ops->xo_cancel_lastsig)(xp)
#define xp_set_protection(xp, addr, size, ptype) \
	(xp->xp_ops->xo_set_protection)(xp, addr, size, ptype)

int get_startup_stop_addrs PROTO((target_t *xp, taddr_t *main_addr,
				  taddr_t *main_min_bpt_addr));
const char *signame PROTO((int sig));
bool can_get_target_vars PROTO((target_t *xp));
bool target_process_exists PROTO((target_t *xp));
void kill_or_detach_from_target PROTO((target_t *xp));
stopres_t xp_execto PROTO((target_t *xp, taddr_t addr));
void load_one_library PROTO((target_t *xp, char *arg));
int make_target PROTO((const char *textpath, const char *corename,
		       bool user_gave_core, target_t **p_xp,
		       const char **p_cmdline, bool restore_bpts,
		       bool target_updated));

taddr_t xp_getreg PROTO((target_t *xp, int regno));

taddr_t xp_get_entryaddr PROTO((target_t *xp));

struct func_s *xp_get_mainfunc PROTO((target_t *xp));
void xp_set_mainfunc PROTO((target_t *xp, struct func_s *f));

struct func_s *xp_get_initfunc PROTO((target_t *xp));
void xp_set_initfunc PROTO((target_t *xp, struct func_s *f));

symtab_t *xp_main_symtab PROTO((target_t *xp));

Siginfo *make_siginfo PROTO((int signo, int preamble_nbytes));
void destroy_stk PROTO((Stack *stk));
#ifdef SYMTAB_H_INCLUDED
Stack *make_stk PROTO((func_t *f, taddr_t pc,
		       fil_t *fil, int lnum, Stack *last));
func_t *make_badfunc PROTO((void));
#endif

void indicate_target_running PROTO((void));
void indicate_target_stopped PROTO((tstate_t tstate));

int preinitialise_target_drivers PROTO((void));
int initialise_target_drivers PROTO((ebuf_t *usage_eb, char **argv));
void show_target_driver_info PROTO((bool name_only));
bool extract_bool_arg PROTO((ebuf_t *usage_eb, char **argv,
			     const char *flag));

xp_ops_t **get_target_drivers PROTO((void));
void target_stop PROTO((void));
int user_wants_non_frame_functions PROTO((void));
int attach_to_target PROTO((target_t *xp, char *pid_str, bool first_attach,
			    bool target_updated, bool user_gave_core,
			    bool new_target_name));
void rescan_dynamic_solibs PROTO((target_t *xp, bool check_only));
int check_target PROTO((const char *textpath, int *p_index, int *p_fd,
			long *p_mtime, bool check_textfile));
