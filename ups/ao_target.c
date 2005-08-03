/* ao_target.c - process manipulation for a.out files using target */

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


/* @(#)ao_target.c	1.1 24/5/95 (UKC) */
char ups_ao_target_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#ifdef AO_TARGET

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/mman.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#if HAVE_MACHINE_REG_H
#include <machine/reg.h>
#endif

#ifdef OS_SUNOS_4
#include <machine/param.h>	/* for PAGESIZE */
#endif

#ifndef PAGESIZE
#ifdef OS_LINUX
#include <asm/param.h>		/* EXEC_PAGESIZE */
#endif
#endif

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <mtrprog/hash.h>

#include "ups.h"
#include "ci.h"
#include "symtab.h"
#include "target.h"
#include "mreg.h"
#include "dx.h"
#include "breakpoint.h"
#include "st.h"
#include "ao_regs.h"
#include "ao_syms.h"
#include "ao_core.h"
#include "ao_target.h"
#include "ao_text.h"
#include "ao_stack.h"
#include "ao_shlib.h"
#include "obj_bpt.h"
#include "ao_ptrace.h"
#include "ao_aout.h"
#include "ao_procfs.h"
#include "ao_elflib.h"
#include "obj_signal.h"
#include "state.h"


static int ao_initialise PROTO((ebuf_t *usage_eb, char **argv));
static void ao_show_target_driver_info PROTO((bool name_only));
static bool ao_match PROTO((const char *textpath, text_block_t *tb, 
                            size_t nbytes));
static int ao_init_from_textfile PROTO((target_t *xp, int textfd, 
                                        const char *corepath, 
                                        bool user_gave_core, 
                                        const char **p_cmdline));
static int ao_is_attached PROTO((target_t *xp));
static int ao_get_addrsize PROTO((target_t *xp));
static tstate_t ao_get_state PROTO((target_t *xp));
static int ao_get_lastsig PROTO((target_t *xp));
static stopres_t ao_get_stopres PROTO((target_t *xp));
static int ao_read_fpval PROTO((target_t *xp, taddr_t addr, int num_bytes, 
                                fpval_t *p_val));
static int ao_readreg PROTO((target_t *xp, int regno, taddr_t *p_val));
static int ao_setreg PROTO((target_t *xp, int regno, taddr_t value));
static int ao_readdreg PROTO((target_t *xp, int regno, taddr_t *p_val));
static int ao_setdreg PROTO((target_t *xp, int regno, taddr_t value));
static taddr_t ao_get_retaddr_after_sig PROTO((target_t *xp));
static taddr_t ao_get_retaddr_after_jsr PROTO((target_t *xp, int rlink_reg));
static void ao_set_base_sp PROTO((target_t *xp, taddr_t sp));
static taddr_t ao_get_base_sp PROTO((target_t *xp));
static int ao_read_fpreg PROTO((target_t *xp, int regno, int num_bytes, 
                                fpval_t *p_val));
static sigstate_t ao_get_sigstate PROTO((target_t *xp, int signo));
static int ao_tswap PROTO((target_t *xp, taddr_t addr, xp_opcode_t opcode, 
                           xp_opcode_t *p_old_opcode));
static int ao_read_data PROTO((target_t *xp, taddr_t addr, char *buf, 
                               size_t nbytes));
static int ao_write_data PROTO((target_t *xp, taddr_t addr, const char *buf, 
                                size_t nbytes));
static int ao_read_text_from_process PROTO((target_t *xp, taddr_t addr, 
                                            char *buf, size_t nbytes));
static int ao_load_library PROTO((target_t *xp, char *arg));
static stopres_t ao_restart_child_internal PROTO((target_t *xp, int sig, 
						  cont_type_t ctype));
static stopres_t ao_restart_child PROTO((target_t *xp, int sig, 
                                         cont_type_t ctype));
static bool ao_do_debug_command PROTO((target_t *xp, const char *cmd, 
                                       size_t len, const char *arg));
static int ao_call_func PROTO((target_t *xp, char *cma, taddr_t addr, 
                               taddr_t *args, size_t *argsizes, int nargs, 
                               taddr_t *p_res, typecode_t restype,
			       const char **p_mesg));
static void ao_detach PROTO((target_t *xp));
static func_t *ao_decode_plt PROTO((target_t *xp, taddr_t pc));
static void ao_cancel_lastsig PROTO((target_t *xp));
static int ao_set_protection PROTO((target_t *xp, taddr_t addr, size_t size,
				    prot_type_t ptype));
static int ao_install_watchpoint PROTO((target_t *xp, taddr_t addr,
					size_t size));
static int ao_uninstall_watchpoint PROTO((target_t *xp, taddr_t addr,
					size_t size, int watchnum));

#if AO_USE_PROCFS
#define ps_create_child			procfs_create_child
#define ps_attach_to_process		procfs_attach_to_process
#define ps_continue			procfs_continue
#define ps_single_step			procfs_single_step
#define ps_detach_from_process		procfs_detach_from_process
#define ps_kill				procfs_kill
#define ps_stop				procfs_stop

#define ps_setreg			procfs_setreg
#define ps_set_all_regs			procfs_set_all_regs
#define ps_read_fpreg			procfs_read_fpreg
#define ps_readreg			procfs_readreg
#define ps_get_sigstate			procfs_get_sigstate
#define ps_load_library			procfs_load_library

#define ps_read_data			procfs_read_data
#define ps_write_data			procfs_write_data
#define ps_read_text_from_process	procfs_read_data
#define ps_write_text			procfs_write_data
#define ps_get_last_attach_pid		procfs_get_last_attach_pid

#define ps_install_watchpoint           procfs_install_watchpoint
#define ps_uninstall_watchpoint         procfs_uninstall_watchpoint
#endif


#if AO_USE_PTRACE
#define ps_create_child			ptrace_create_child
#define ps_attach_to_process		ptrace_attach_to_process
#define ps_continue			ptrace_continue
#define ps_single_step			ptrace_single_step
#define ps_detach_from_process	        ptrace_detach_from_process
#define ps_kill				ptrace_kill
#define ps_stop				ptrace_stop

#define ps_setreg			ptrace_setreg
#define ps_setdreg			ptrace_setdreg
#define ps_set_all_regs			ptrace_set_all_regs
#define ps_read_fpreg			ptrace_read_fpreg
#define ps_readreg			ptrace_readreg
#define ps_readdreg			ptrace_readdreg
#define ps_get_sigstate			ptrace_get_sigstate
#define ps_load_library			ptrace_load_library

#define ps_read_data			ptrace_read_data
#define ps_write_data			ptrace_write_data
#define ps_read_text_from_process	ptrace_read_text_from_process
#define ps_write_text			ptrace_write_text
#define ps_get_last_attach_pid		ptrace_get_last_attach_pid

#define ps_install_watchpoint           ptrace_install_watchpoint
#define ps_uninstall_watchpoint         ptrace_uninstall_watchpoint
#endif

#ifdef AO_ELF
#define ps_next_symtab			elf_next_symtab
#else
#define ps_next_symtab			aout_next_symtab
#endif


xp_ops_t Ao_ops = {
	NULL, ao_initialise, ao_show_target_driver_info,
	ao_match, ao_init_from_textfile, ps_next_symtab,
	dx_start, dx_step, dx_next, dx_cont, ps_kill,
	ps_create_child, ao_restart_child,
	ao_set_base_sp, ao_get_base_sp,
	dx_add_breakpoint, dx_remove_breakpoint,
	dx_enable_breakpoint, dx_disable_breakpoint,
	dx_addr_to_breakpoint, dx_get_breakpoint_addr,
	dx_add_watchpoint, dx_remove_watchpoint,
	dx_enable_watchpoint, dx_disable_watchpoint,
	ao_install_watchpoint, ao_uninstall_watchpoint,
	ao_is_attached, ao_detach, ao_get_addrsize,
	ao_get_state, ao_get_lastsig, ao_get_stopres, ao_get_sigstate,
	ao_get_stack_trace, ao_get_reg_addr, ao_get_signal_tag,
	ao_read_fpval, ao_read_fpreg, ao_readreg, ao_setreg,
	ao_readdreg, ao_setdreg,
	ao_get_retaddr_after_jsr, ao_get_retaddr_after_sig,
	ao_tswap,
	ao_read_data, ao_write_data, ao_read_text_from_process,
	ao_load_library, ao_call_func, NULL, ao_do_debug_command,
	ao_decode_plt, ao_cancel_lastsig, ao_set_protection
};

static bool Disable_native = TRUE;
static long Page_size = 0;

static int
ao_initialise(usage_eb, argv)
ebuf_t *usage_eb;
char **argv;
{
	Disable_native = extract_bool_arg(usage_eb, argv, "-nonative");
#if defined(_SC_PAGESIZE)
	Page_size = sysconf(_SC_PAGESIZE);
#elif defined(PAGESIZE)
	Page_size = PAGESIZE;
#elif defined(EXEC_PAGESIZE)
	Page_size = EXEC_PAGESIZE;
#else
#error Unable to determine the page size of this system
#endif
	return 0;
}

static void
ao_show_target_driver_info(name_only)
bool name_only;
{
	if (Disable_native)
		return;
	
	if (name_only) {
		fputs("\tNative support for ", stdout);
		
#ifdef THIS_ARCH
		printf("%s ", THIS_ARCH);
#endif
		
#ifdef AO_ELF
		fputs("ELF ", stdout);
#endif
		
		puts("object files");
	}
}

static bool
ao_match(textpath, tb, nbytes)
const char *textpath;
text_block_t *tb;
size_t nbytes;
{
	return !Disable_native;
}

static int
ao_init_from_textfile(xp, textfd, corepath, user_gave_core, p_cmdline)
target_t *xp;
int textfd;
const char *corepath;
bool user_gave_core;
const char **p_cmdline;
{
	taddr_t data_addr;
	int lastsig, ret;
	iproc_t *ip;
	Coredesc *co;
	int pid;

	if (!open_textfile(textfd, xp->xp_textpath, &data_addr))
		return -1;

	if (!open_corefile(xp->xp_apool, corepath, xp->xp_textpath,
			   user_gave_core, data_addr, &co, &lastsig))
		return -1;

	if (user_gave_core && is_number(corepath)) {
		pid = atoi(corepath);
	}
	else {
        	pid = 0;
	}

	ip = (iproc_t *)alloc(xp->xp_apool, sizeof(iproc_t));
	ip->ip_apool = xp->xp_apool;
	ip->ip_pid = 0;
	ip->ip_addrsize = 32;
	ip->ip_core = co;
	ip->ip_lastsig = lastsig;
	ip->ip_base_sp = (taddr_t)~0;	/* Fixed later for non-ELF */
	
	xp->xp_data = (char *)ip;

#ifdef AO_ELF
#if AO_USE_PROCFS
	procfs_init(ip);
#else
	ip->ip_ptrace_info = NULL;
#if defined(AO_AOUT_SHLIBS)
	ip->ip_shlibs_ap = NULL;
	ip->ip_shlibs = NULL;
#endif
#endif
	
	ip->ip_solib_addrs = NULL;
	
	ret = scan_main_elf_symtab(xp->xp_apool, xp->xp_textpath, textfd,
				   xp->xp_modtime, &ip->ip_solibs,
				   &ip->ip_solib_addrs, &ip->ip_addrsize,
				   &xp->xp_entryaddr, &xp->xp_mainfunc,
				   &xp->xp_initfunc, xp->xp_target_updated, pid);
	xp->xp_target_updated = FALSE;
	if (!ret)
	  return -1;

	if (co != NULL)
		elf_get_core_shlib_info(xp->xp_apool, ip);

#else
	ip->ip_ptrace_info = NULL;
	
	if (!aout_scan_main_symtab(xp->xp_textpath, textfd,
				   &ip->ip_main_symtab, &ip->ip_symtab_cache,
				   &xp->xp_entryaddr, &xp->xp_mainfunc))
		return -1;
	
	ip->ip_shlib_symtabs = NULL;

	if (co != NULL)
		load_shared_library_symtabs(xp, FALSE);
#endif

	if (is_number(corepath) &&
	    ps_attach_to_process(xp, atoi(corepath)) != 0)
		return -1;

	if (co != NULL && *p_cmdline == NULL)
		*p_cmdline = core_get_cmdline(co);

#ifdef ARCH_386
	x86_gcc_register_init(ip->ip_addrsize);
#endif

	return 0;
}

/*  Restart process xp.  Either continue or single step it, depending on ctype.
 *
 *  If the process was stopped at a breakpoint, we remove the breakpoint,
 *  single step the process and replace the breakpoint.  If ctype was CT_CONT
 *  and the single step stopped normally we than continue the process with
 *  a CT_CONT.
 *
 *  The intended effect is that breakpoints that are at the current pc value
 *  are ignored.
 *
 *  We return the reason why the process stopped.
 */
static stopres_t
ao_restart_child_internal(xp, sig, ctype)
target_t *xp;
int sig;
cont_type_t ctype;
{
	iproc_t *ip;
	breakpoint_t *bp;

	ip = GET_IPROC(xp);

	/*  Step over bpt if there is one at ip->ip_restart_pc.
	 */
	if ((bp = get_breakpoint_at_addr(xp, ip->ip_restart_pc)) != NULL) {
		if (uninstall_breakpoint(bp) != 0)
			return SR_FAILED;

		ps_single_step(xp, sig);
		
		if (install_breakpoint(bp, xp, FALSE) != 0)
			panic("can't install breakpoint in ao_cont");
		
		if (ip->ip_stopres != SR_SSTEP || ctype == CT_STEP)
			return ip->ip_stopres;
		
		sig = 0;
	}

	switch (ctype) {
	case CT_CONT:
		ps_continue(xp, (taddr_t)0, sig);
		break;
	case CT_STEP:
		ps_single_step(xp, sig);
		break;
	default:
		panic("bad ctype in arc");
	}

	return ip->ip_stopres;
}

static stopres_t
ao_restart_child(xp, sig, ctype)
target_t *xp;
int sig;
cont_type_t ctype;
{
  iproc_t *ip;
  int last_sig = sig;
  
  ip = GET_IPROC(xp);
  
  do
  {
    ao_restart_child_internal(xp, last_sig, ctype);
    last_sig = ip->ip_lastsig;
  }
  while (ip->ip_stopres == SR_SIG &&
	 !sig_stops_target(last_sig) &&
         xp_get_sigstate(xp, last_sig) != SGH_CAUGHT);

#ifdef AO_ELF
  if (ip->ip_stopres == SR_BPT)
  {
    breakpoint_t *bp;

    if ((bp = get_breakpoint_at_addr(xp, ip->ip_restart_pc)) != NULL &&
	breakpoint_is_solib_event(bp))
      xp->xp_hit_solib_event = TRUE;
    else
      xp->xp_hit_solib_event = FALSE;
  }
#endif

  return ip->ip_stopres;
}

/*  Return TRUE if we attached to process xp rather than starting it.
 */
static int
ao_is_attached(xp)
target_t *xp;
{
	iproc_t *ip;

	ip = GET_IPROC(xp);
	return ip->ip_pid && ip->ip_attached;
}

static int
ao_get_addrsize(xp)
target_t *xp;
{
	return GET_IPROC(xp)->ip_addrsize;
}

static tstate_t
ao_get_state(xp)
target_t *xp;
{
	iproc_t *ip;

	ip = GET_IPROC(xp);

	if (ip->ip_pid != 0)
		return sig_is_fatal(ip->ip_lastsig) ? TS_HALTED : TS_STOPPED;
	else
		return (ip->ip_core != NULL) ? TS_CORE : TS_NOTR;
}

/*  Return the signal that last stopped the target.
 */
static int
ao_get_lastsig(xp)
target_t *xp;
{
	return GET_IPROC(xp)->ip_lastsig;
}

/*  Return the reason why the process stopped.  This is the reason for the
 *  most recent stop.
 */
static stopres_t
ao_get_stopres(xp)
target_t *xp;
{
	return GET_IPROC(xp)->ip_stopres;
}

#ifdef ARCH_SUN3
static void
convert_68881_reg(rwords, is_double, p_val)
unsigned *rwords;
bool is_double;
fpval_t *p_val;
{
	int exponent;
	unsigned *w;

	exponent = ((rwords[0] >> 16) & 0x7fff) - ((1 << 14) - 1);

	w = (unsigned *)&p_val->d;

	/*  Transfer exponent and sign bit.
	 */
	w[0] = (exponent + ((1 << 10) - 1)) << 20;
	w[0] |= rwords[0] & 0x80000000;

	/*  Copy mantissa into first 32 bit word
	 */
	w[0] |= (rwords[1] & 0x7fffffff) >> 11;

	/*  Copy rest of mantissa (that will fit) into second 32 bit word.
	 */
	w[1] = ((rwords[1] & 0x7ff) << (32 - 11)) | (rwords[2] >> 11); 

	if (!is_double)
		p_val->f = p_val->d;
}
#endif
		
static int
ao_read_fpval(xp, addr, num_bytes, p_val)
target_t *xp;
taddr_t addr;
int num_bytes;
fpval_t *p_val;
{
#ifdef ARCH_SUN3
	unsigned rwords[3];

	if (ao_read_data(xp, addr, (char *)rwords, sizeof(rwords)) != 0)
		return -1;
	convert_68881_reg(rwords, (num_bytes == sizeof(double)), p_val);
	return 0;
#else
	switch (num_bytes) {
	    case sizeof(float):
		return ao_read_data(xp, addr, (char *)&p_val->f, num_bytes);
	    case sizeof(double):
		return ao_read_data(xp, addr, (char *)&p_val->d, num_bytes);
#if HAVE_LONG_DOUBLE && (SIZEOF_LONG_DOUBLE > SIZEOF_DOUBLE)
	    case sizeof(long double):
		return ao_read_data(xp, addr, (char *)&p_val->ld, num_bytes);
#endif
	}
#endif
	return -1;
}

/*  Get the value currently stored in register regno of process xp.
 *  Return 0 and set *p_val for success, -1 for failure.
 *
 *  regno is either a general purpose register, or one of REG_PC, REG_SP,
 *  REG_FP and REG_AP.  See mreg.h.
 */
static int
ao_readreg(xp, regno, p_val)
target_t *xp;
int regno;
taddr_t *p_val;
{
	return ps_readreg(GET_IPROC(xp), regno, p_val);
}

static int
ao_readdreg(xp, regno, p_val)
target_t *xp;
int regno;
taddr_t *p_val;
{
#ifdef ps_readdreg
	return ps_readdreg(GET_IPROC(xp), regno, p_val);
#else
	panic("don't know how to get a debug register value for this machine");
#endif
}

static int
ao_read_fpreg(xp, regno, num_bytes, p_val)
target_t *xp;
int regno;
int num_bytes;
fpval_t *p_val;
{
	return ps_read_fpreg(GET_IPROC(xp), regno, num_bytes, p_val);
}

static int
ao_setreg(xp, regno, value)
target_t *xp;
int regno;
taddr_t value;
{
	return ps_setreg(GET_IPROC(xp), regno, value);
}

static int
ao_setdreg(xp, regno, value)
target_t *xp;
int regno;
taddr_t value;
{
#ifdef ps_setdreg
	return ps_setdreg(GET_IPROC(xp), regno, value);
#else
	panic("don't know how to set a debug register value for this machine");
#endif
}

/*  Return the saved pc after a signal.  Only works immediately after a
 *  ao_cont(..., CT_STEP) which has returned SR_SIG for a signal which
 *  is caught.
 *
 *  Used to put a breakpoint after the return from the signal handler.
 */
static taddr_t
ao_get_retaddr_after_sig(xp)
target_t *xp;
{
#if defined(ARCH_SUN3) || defined(ARCH_VAX) || defined(ARCH_386)
	return xp_getreg(xp, UPSREG_PC);
#else
	panic("gras NYI");
	return 0;	/* to satisfy gcc */
#endif
}

/*  Return the saved pc after a jsr.  Must be called immediatly after a jsr.
 *  Used to put a breakpoint at the return address.
 */
static taddr_t
ao_get_retaddr_after_jsr(xp, rlink_reg)
target_t *xp;
int rlink_reg;
{
#if defined(ARCH_SUN3) || defined(ARCH_CLIPPER) || \
   defined(ARCH_BSDI386) || defined(ARCH_LINUX386) || defined(ARCH_SOLARIS386)
	taddr_t sp, retaddr;
	iproc_t *ip;

	ip = GET_IPROC(xp);
	
	sp = xp_getreg(xp, UPSREG_SP);
	retaddr = 0;
	
	if (ao_read_data(xp, sp, (char *)&retaddr, ip->ip_addrsize / 8) == -1)
		panic("bad sp in pgraj");
	return retaddr;
#else
#ifdef ARCH_VAX
	taddr_t sp;
	struct frame frbuf;

	sp = xp_getreg(xp, UPSREG_SP);
	if (ao_read_data(xp, sp, (char *)&frbuf, sizeof(frbuf)) == -1)
		panic("bad sp in pgraj");
	return frbuf.fr_savpc;
#else
#ifdef ARCH_MIPS
	return xp_getreg(xp, rlink_reg);
#else
/*	panic("graj NYI");  RGA */
	return 0;
#endif  /* !ARCH_MIPS */
#endif	/* !ARCH_VAX */
#endif  /* !ARCH_{SUN{3,386},CLIPPER} */
}

static void
ao_set_base_sp(xp, sp)
target_t *xp;
taddr_t sp;
{
	GET_IPROC(xp)->ip_base_sp = sp;
#if defined(AO_AOUT_SHLIBS)
	load_shared_library_symtabs(xp, FALSE);
	recalculate_bpt_addrs(xp);
#endif
}

static taddr_t
ao_get_base_sp(xp)
target_t *xp;
{
	return GET_IPROC(xp)->ip_base_sp;
}

static sigstate_t
ao_get_sigstate(xp, signo)
target_t *xp;
int signo;
{
	return ps_get_sigstate(GET_IPROC(xp), signo);
}

static int
ao_read_data(xp, addr, buf, nbytes)
target_t *xp;
taddr_t addr;
char *buf;
size_t nbytes;
{
	iproc_t *ip;
	
	ip = GET_IPROC(xp);
	
	if (target_process_exists(xp))
		return ps_read_data(ip, addr, buf, nbytes);

	if ((ip->ip_core != NULL &&
	     core_dread(ip->ip_core, addr, buf, nbytes)) ||
	    text_dread(xp, addr, buf, nbytes))
		return 0;

	return  -1;
}

static int
ao_write_data(xp, addr, buf, nbytes)
target_t *xp;
taddr_t addr;
const char *buf;
size_t nbytes;
{
	return ps_write_data(GET_IPROC(xp), addr, buf, nbytes);
}

static int
ao_read_text_from_process(xp, addr, buf, nbytes)
target_t *xp;
taddr_t addr;
char *buf;
size_t nbytes;
{
	return ps_read_text_from_process(GET_IPROC(xp), addr, buf, nbytes);
}

static int
ao_load_library(xp, arg)
target_t *xp;
char *arg;
{
	return ps_load_library(xp, arg);
}

/*  Breakpoint opcodes for the various machines.
 */
#ifdef ARCH_VAX
#define BPT				0x03		/* bpt */
#endif

#ifdef ARCH_386
#define BPT				0xcc		/* int $3 */
#endif

#ifdef ARCH_SUN3
#define BPT				0x4e4f		/* trap #15 */
#endif

#ifdef ARCH_CLIPPER
#define BPT				0x0900		/* what dbx uses */
#endif

#ifdef ARCH_SUN4
#define BPT				0x91d02001	/* What gdb uses */
#endif

#ifdef ARCH_MIPS
#define BPT				0x0000000d	/* break 0 */
#endif

#if defined(ARCH_VAX) || defined(ARCH_386)
#define PUT_OPCODE_IN_WORD(w, data)	(((w) & ~0xff) | (data & 0xff))
#define GET_OPCODE_FROM_WORD(w)		((w) & 0xff)
#endif

#if defined(ARCH_SUN3) || defined(ARCH_CLIPPER)
#define PUT_OPCODE_IN_WORD(w, data)	(((data) << 16) | ((w) & 0xffff))
#define GET_OPCODE_FROM_WORD(w)		(((w) >> 16) & 0xffff)
#endif

#if defined(ARCH_SUN4) || defined(ARCH_MIPS)
#define PUT_OPCODE_IN_WORD(w, data)	(data)
#define GET_OPCODE_FROM_WORD(w)		(w)
#endif

/*  Insert opcode into the text area at addr.  If p_old_opcode is
 *  non NULL we ignore the supplied opcode and insert the appropriate
 *  breakpoint opcode for the machine.  Otherwise insert opcode
 *  and set *p_old_opcode to the old opcode.
 *
 *  We treat it as a fatal error if we are installing a breakpoint
 *  and one is already there, or if we are installing other than
 *  a breakpoint and there isn't one already there.
 */
static int
ao_tswap(xp, addr, opcode, p_old_opcode)
target_t *xp;
taddr_t addr;
xp_opcode_t opcode, *p_old_opcode;
{
#if defined(ARCH_CLIPPER)
	int byte_shift;
	unsigned mask;
#endif
	int pid;
        unsigned long temp;
	xp_opcode_t old_opcode;
	iproc_t *ip;

	ip = GET_IPROC(xp);
	pid = ip->ip_pid;

	if (p_old_opcode != NULL)
		opcode = BPT;

#ifdef ARCH_CLIPPER
	if ((addr & 1) != 0)
		panic("unaligned addr passes to ao_tswap");
	byte_shift = (addr & 03) * 8;
	mask = 0xffff << byte_shift;
	addr &= ~03;
#endif
#ifdef ARCH_MIPS
	if ((addr & 3) != 0)
		panic("unaligned addr passed to ao_tswap");
#endif

	if (ps_read_text_from_process(ip, addr,
				      (char *)&temp, sizeof(temp)) != 0)
	       return -1;

#if defined(ARCH_CLIPPER)
	old_opcode = ((unsigned)temp & mask) >> byte_shift;
	temp = ((unsigned)temp & ~mask) | (opcode << byte_shift);
#else
	old_opcode = GET_OPCODE_FROM_WORD(temp);
	temp = PUT_OPCODE_IN_WORD(temp, opcode);
#endif

	if (ps_write_text(ip, addr, (char *)&temp, sizeof(temp)) != 0)
		return -1;
	
	if ((opcode == BPT) == (old_opcode == BPT)) {
		if (opcode == BPT)
/*	RGA		panic("duplicate breakpoint in ao_tswap");*/
			errf("duplicate breakpoint in ao_tswap");
		else
/*	RGA		panic("vanished breakpoint in ao_tswap");*/
			errf("vanished breakpoint in ao_tswap");
	}

	if (p_old_opcode != NULL)
		*p_old_opcode = old_opcode;
	
	return 0;
}

static void
ao_detach(xp)
target_t *xp;
{
	iproc_t *ip;

	ip = GET_IPROC(xp);
	
	if (!ip->ip_attached)
		panic("ao_detach called but not attached to xp");
	
	if (uninstall_all_breakpoints(xp) != 0)
		panic("can't uninstall breakpoints in xp");
	
	ps_detach_from_process(xp);
}

static bool
ao_do_debug_command(xp, cmd, len, arg)
target_t *xp;
const char *cmd;
size_t len;
const char *arg;
{
#ifdef AO_ELF
	static const char dumplibs[] = "dumplibs";
	
	if (len == sizeof(dumplibs) - 1 && memcmp(cmd, dumplibs, len) == 0) {
		dump_elf_libs(xp);
		return TRUE;
	}
#endif
	
	return FALSE;
}

static func_t *
ao_decode_plt(xp, pc)
target_t *xp;
taddr_t pc;
{
#ifdef AO_ELF
	return elf_decode_plt(xp, pc);
#else
	return NULL;
#endif
}

/*  Cancel any signal that stopped the target.
 */
static void
ao_cancel_lastsig(xp)
target_t *xp;
{
	GET_IPROC(xp)->ip_lastsig = 0;
}

static int
ao_set_protection(xp, addr, size, ptype)
target_t *xp;
taddr_t addr;
size_t size;
prot_type_t ptype;
{
	func_t *f;
	int prot;
	taddr_t args[3];
	size_t argsizes[3];
	taddr_t res;
	const char *msg;

	if (find_func_by_name("mprotect", &f, NULL, FALSE) < 0)
		return -1;

	addr = addr & ~(Page_size-1);
	if (size % Page_size)
		size = size + Page_size - (size % Page_size);

	switch (ptype) {
	case PT_NONE:
		prot = PROT_NONE;
		break;
	case PT_RDONLY:
		prot = PROT_READ;
		break;
	case PT_WRONLY:
		prot = PROT_WRITE;
		break;
	case PT_RDWR:
		prot = PROT_READ | PROT_WRITE;
		break;
	case PT_ALL:
		prot = PROT_READ | PROT_WRITE | PROT_EXEC;
		break;
        default:
		panic("bad protection type in ao_set_protection");
		prot = PROT_NONE; /* to satisfy gcc */
		break;
	}

	args[0] = addr;
	args[1] = size;
	args[2] = prot;

	argsizes[0] = 0;
	argsizes[1] = 0;
	argsizes[2] = 0;

	if (ao_call_func(xp, NULL, f->fu_addr, args, argsizes, 3, &res, TY_INT, &msg) < 0)
		return -1;

	return (int)res;
}

/*  Some architectures pass some function arguments in registers.
 */
#ifdef ARCH_MIPS
#define N_REG_ARGS	4
#define RETURN_REGNO	2
#endif /* ARCH_MIPS */

#ifdef ARCH_CLIPPER
#define N_REG_ARGS	2
#endif

#ifdef ARCH_386_64
#define N_REG_ARGS	6
#endif

#ifdef ARCH_SUN4
#define N_REG_ARGS	6
#define RETURN_REGNO	8	/* %o0 */
#define ALIGN_STACK(n)	((n) & ~(unsigned)07)
#endif

#ifndef N_REG_ARGS
#define N_REG_ARGS	0
#endif

#ifndef RETURN_REGNO
#define RETURN_REGNO	0
#endif

#ifndef ALIGN_STACK
#define ALIGN_STACK(n)	((n) & ~03)
#endif

#ifdef ARCH_386_64
static const int argregs[N_REG_ARGS] = { 5, 4, 1, 2, 8, 9 };
#endif

/*  Call a function in the target.
 *
 *  There must be no breakpoints installed when this routine is called
 */
static int
ao_call_func(xp, cma, addr, args, argsizes, nargs, p_res, restype, p_mesg)
target_t *xp;
char *cma;
taddr_t addr;
taddr_t *args;
size_t *argsizes;
int nargs;
taddr_t *p_res;
typecode_t restype;
const char **p_mesg;
{
	taddr_t saved_restart_pc;
	taddr_t sp, retpc;
	iproc_t *ip;
	int i;
#ifdef ARCH_VAX
	struct frame frame;
	taddr_t fp, ap, saved_sp, saved_r0, saved_r1;
	int psw;
	unsigned short regmask;
#endif
#ifdef OS_SUNOS
#if AO_USE_PROCFS
	static int *regs = NULL, *orig_regs;
	static size_t regbytes;
#else
	sunregs_t sunregs, orig_sunregs;
	sunregs_t *regs, *orig_regs;
#endif
#endif
#ifdef ARCH_SUN4
	int *oregs;
#endif
#ifdef ARCH_MIPS
	taddr_t regtab[NGP_REGS + NFP_REGS]; 	/* Not all used */
	static int saveregs = -1;
#endif /* ARCH_MIPS */
#ifdef ARCH_CLIPPER
	taddr_t saved_sp, saved_regs[N_REG_ARGS];
#endif
#if defined(ARCH_BSDI386) || defined(ARCH_LINUX386)
#if AO_HAS_PTRACE_REGS
	ptrace_regs_t ptrace_regs, orig_ptrace_regs;
	ptrace_regs_t *regs, *orig_regs; 
#else
	taddr_t saved_regs[N_UREGS];
#endif
#endif
	breakpoint_t *bp;
	taddr_t realargs[40];
	int wordsize;
	
	if (nargs > sizeof realargs / sizeof *realargs)
		panic("nargs too large in ao_call_func");

	*p_mesg = NULL;

	ip = GET_IPROC(xp);
	wordsize = xp_get_addrsize(xp) / 8;

#if defined(ARCH_SUN4) && !defined(OS_SUNOS_5)
	/*  We can only call a target function if we are currently stopped
	 *  at a breakpoint.  This is because on the SPARC you can't always
	 *  feed a pc value back to PTRACE_CONT - if for example the target
	 *  is stopped at a delay slot then the only thing that works is
	 *  the special pc value 1 meaning `restart from where you stopped'.
	 *  Calling a target function would change the `where you last stopped'
	 *  remembered state and we would be unable to subseqently continue
	 *  the target.
	 *
	 *  The condition should never be true, as target functions can
	 *  only be called from breakpoint code and we don't allow target
	 *  function calls in display area expressions.  Better safe than
	 *  sorry though.
	 */
	if (ip->ip_stopres != SR_BPT) {
	  /*		*p_mesg = "Can't call target function - target not stopped at breakpoint";*/
		return -1;
	}
#endif

#ifdef ARCH_MIPS
	/*  Save registers that the Assembly Language Guide
	 *  says are not preserved across function calls.
	 *  This is because we can call a target function
	 *  at an arbitrary point, and the surrounding code
	 *  will not be preprared for registers to change.
	 *
	 *  We save all registers expect r0 (wired to zero),
	 *  and r16..r23 (s0..s7).
	 *
	 *  Similarly, we save the non-preserved floating point
	 *  registers (f0..f19).
	 */
	
	if (saveregs == -1)
		saveregs = getenv("DONTSAVEREGS") == NULL;

	if (saveregs) {
		for (i = 1; i < 16; ++i)
			regtab[i] = xp_getreg(xp, i);
		for (i = 23; i < 32 + 20; ++i)
			regtab[i] = xp_getreg(xp, i);
		/*  Make the sp four byte aligned.  It probably already is.
		 */
		sp = regtab[29] & ~3;
	}
	else {
		sp = xp_getreg(xp, REG_SP) & ~3;
	}
#endif /* ARCH_MIPS */

#ifdef ARCH_VAX
	if (ao_read_text_from_process(xp, addr,
				      (char *)&regmask, sizeof(regmask)) != 0) {
		*p_mesg = "Can't read register mask from text";
		return -1;
	}
	addr += sizeof(regmask);

	fp = xp_getreg(xp, REG_FP);
	psw = xp_getreg(xp, REG_CONDITION_CODES) & 0xffff;

	/*  Get the values of the registers that won't be preserved by the
	 *  calls instruction.
	 */
	saved_r0 = xp_getreg(xp, 0);
	saved_r1 = xp_getreg(xp, 1);
	saved_sp = xp_getreg(xp, REG_SP);

	/*  Do the calls type stack pointer four byte alignment.
	 */
	sp = saved_sp & ~03;
#endif
#ifdef OS_SUNOS
#if AO_USE_PROCFS
	if (regs == NULL) {
		regbytes = sizeof(int) * procfs_get_reg_index(PROCFS_REG_NREGS);
		regs = e_malloc(regbytes);
		orig_regs = e_malloc(regbytes);
	}
	
	procfs_get_regtab(ip, regs);
	memcpy(orig_regs, regs, regbytes);
	sp = regs[procfs_get_reg_index(PROCFS_REG_SP)];
#else
	ptrace_get_regtab(ip, &sunregs);
	orig_sunregs = sunregs;
	regs = &sunregs;
	orig_regs = &orig_sunregs;
	sp = sunregs.sr_regs.r_sp;
#endif
#endif
#ifdef ARCH_CLIPPER
	for (i = 0; i < N_REG_ARGS; ++i)
		saved_regs[i] = xp_getreg(xp, i);
	sp = saved_sp = xp_getreg(xp, UPSREG_SP);
#endif
#if defined(ARCH_BSDI386) || defined(ARCH_LINUX386)
#if AO_HAS_PTRACE_REGS
	ptrace_get_regtab(ip, &ptrace_regs);
	orig_ptrace_regs = ptrace_regs;
	regs = &ptrace_regs;
	orig_regs = &orig_ptrace_regs;
	sp = xp_getreg(xp, UPSREG_SP);
#else
	for (i = 0; i < N_UREGS; ++i)
		if (get_uarea_reg(ip, i, &saved_regs[i]) != 0)
			panic("can't get reg in pcf");
	sp = xp_getreg(xp, UPSREG_SP);
#endif
#endif

	/*  Adjust the stack pointer to ensure that it points at a new
	 *  stack page. This is because some of the stack pages in the
	 *  target may have been protected in order to implement a
	 *  watchpoint and we don't want that to affect the function
	 *  we are about to call.
	 */
	sp = sp - Page_size;
	
	retpc = saved_restart_pc = ip->ip_restart_pc;

	/*  Make copies of the targets of pointers and arrays.
	 *
	 *  These are indicated by a non zero size.
	 */
	for (i = 0; i < nargs; ++i) {
		size_t size;

		size = argsizes[i];
		if (size > 0 && ci_is_ci_addr((machine_t *)cma,
							args[i], size)) {
			sp = ALIGN_STACK(sp - size);
			if (ps_write_data(ip, sp, (char *)args[i], size) != 0) {
				*p_mesg = "Can't write data to the stack";
				return -1;
			}
			realargs[i] = sp;
		}
		else
			realargs[i] = args[i];
	}

	/*  Push the arguments.  We want the last argument pushed first,
	 *  so work backwards through the array of arguments.
	 *
	 *  On some machines only some arguments go on the stack - the
	 *  rest go in registers.  We delay setting the registers until
	 *  we have successfully set the return breakpoint (see below).
	 *  
	 */
#ifdef ARCH_SUN4
	/*  Try to end up with the stack pointer eight byte aligned
	 *  (after the 0x5c subtraction below.
	 */
	if (nargs < N_REG_ARGS || nargs % 2 == 0)
		sp -= wordsize;
#endif
	for (i = nargs - 1; i >= N_REG_ARGS; --i) {
		sp -= wordsize;
		if (ps_write_data(ip, sp, (char *)&realargs[i], wordsize) != 0) {
			*p_mesg = "Can't write argument to the stack";
			return -1;
		}
	}
#ifdef ARCH_SUN4
	/*  The SPARC compilers expect to find the seventh parameter
	 *  (the first one not passed in a register) at 0x5c(fp).
	 */
	sp -= 0x5c;
#endif

#ifdef ARCH_VAX
	/*  Push the arg count.
	 */
	sp -= wordsize;
	if (ps_write_data(ip, sp, (char *)&nargs, sizeof(nargs)) != 0) {
		*p_mesg = "Can't write the arg count to the stack";
		return -1;
	}

	ap = sp;

	/*  Save the registers mentioned in the function's save mask.
	 */
	for (i = 11; i >= 0; --i) {
		if (regmask & (1 << i)) {
			taddr_t regval;

			regval = xp_getreg(xp, i);
			sp -= wordsize;
			if (ps_write_data(ip, sp, (char *)&regval, wordsize) != 0) {
				*p_mesg = "Can't push register value";
				return -1;
			}
		}
	}

	/*  Construct a calls stack frame.
	 *
	 *  We don't use the bitfields defined in <sys/frame.h> because they
	 *  just have a :1 for a field which is specified as "must be zero"
	 *  by the VAX architecture manual.
	 */
	frame.fr_handler = 0;
	(&frame.fr_handler)[1] = (1 << 29) | (regmask << 16) | psw;
	frame.fr_savap = xp_getreg(xp, UPSREG_AP);
	frame.fr_savfp = xp_getreg(xp, UPSREG_FP);
	frame.fr_savpc = retpc;

	/*  Push frame onto the stack.
	 */
	sp -= sizeof(frame);
	if (ps_write_data(ip, sp, (char *)&frame, sizeof(frame)) != 0) {
		*p_mesg = "Can't push stack frame";
		return -1;
	}
#endif /* ARCH_VAX */
#if defined(ARCH_386) || defined(ARCH_CLIPPER)
	sp -= wordsize;
	if (ps_write_data(ip, sp, (char *)&retpc, sizeof(retpc)) != 0) {
		*p_mesg = "Can't push return address";
		return -1;
	}
#endif

	/*  Put a breakpoint where we've arranged for the function to
	 *  return to, unless there is already a breakpoint installed there.
	 */
	if (get_breakpoint_at_addr(xp, retpc) != NULL) {
		bp = NULL;
	}
	else {
		bp = dx_add_breakpoint(xp, retpc);
		if (install_breakpoint(bp, xp, FALSE) != 0) {
			*p_mesg = "Can't insert breakpoint at return address";
			return -1;
		}
	}

	/*  Up to this point, we haven't been committed, as all we've been
	 *  doing is pushing things beyond the end of the stack.
	 *
	 *  Now we're reasonably confident that the call will succeed, so
	 *  sync the target registers with our idea of their values.
	 */
#ifdef ARCH_VAX
	ps_setreg(ip, UPSREG_AP, ap);
	ps_setreg(ip, UPSREG_FP, fp);
	ps_setreg(ip, UPSREG_SP, sp);
#endif /* ARCH_VAX */
#if defined(ARCH_386)
	ps_setreg(ip, UPSREG_SP, sp);
#endif
#ifdef ARCH_386_64
	/*  The first six integer argument go in registers, with no
	 *  space allocated for them on the stack.
	 *
	 *  BUG: we assume that all the arguments are integer (not
	 *       floating point).
	 */
	for (i = 0; i < nargs && i < N_REG_ARGS; ++i) {
		if (ps_setreg(ip, argregs[i], realargs[i]) != 0)
			panic("regno write failed in cf");
	}
#endif
#ifdef ARCH_MIPS
	/*  The first four integer arguments go in registers, but there
	 *  is space allocated for them on the stack.
	 *
	 *  BUG: we assume that all the arguments are integer (not
	 *       floating point).
	 */
	sp -= N_REG_ARGS * wordsize;
	if (ps_setreg(ip, MIPS_SP_REGNO, sp) != 0) {
		*p_mesg = "Can't set stack pointer for target function call";
		return -1;
	}
	
	for (i = 0; i < nargs && i < N_REG_ARGS; ++i) {
		if (ps_setreg(ip, 4 + i, realargs[i]) != 0)
			panic("regno write failed in cf");
	}

	/*  Arrange for the function to return to where we've set
	 *  the breakpoint.
	 */
	if (ps_setreg(ip, MIPS_SAVEDPC_REGNO, retpc) != 0)
		panic("regno write failed in cf");
#endif
#ifdef ARCH_CLIPPER
	if (ps_setreg(ip, UPSREG_SP, sp) != 0) {
		*p_mesg = "Can't set stack pointer for target function call";
		return -1;
	}
	for (i = 0; i < nargs && i < N_REG_ARGS; ++i) {
		if (ps_setreg(ip, i, realargs[i]) != 0)
			panic("regno write failed in cf");
	}
#endif
#ifdef ARCH_SUN4
#if AO_USE_PROCFS
	oregs = &regs[procfs_get_reg_index(PROCFS_REG_O0)];
#else
	oregs = &sunregs.sr_regs.r_o0;
#endif
	for (i = 0; i < nargs && i < N_REG_ARGS; ++i)
		oregs[i] = realargs[i];
	
	oregs[6] = sp;
	oregs[7] = retpc - 8;	/* Normal func return is jmp savpc + 8 */

	if (!ps_set_all_regs(ip, regs) != 0) {
		*p_mesg = "Can't set registers for target function call";
		return -1;
	}
#endif /* ARCH_SUN4 */

	/*  Jump to the start of the function.
	 */
	ps_continue(xp, addr, 0);
	
	/*  If we installed a breakpoint ourself, uninstall it.
	 */
	if (ip->ip_stopres != SR_DIED && bp != NULL) {
		if (dx_remove_breakpoint(xp, bp) != 0)
			return -1;
	}

	/*  If we didn't get back to where we expected, stop now.
	 */
	if (ip->ip_stopres != SR_BPT || ip->ip_restart_pc != retpc)
		return -1;

	*p_res = xp_getreg(xp, RETURN_REGNO);
#if defined(ARCH_386) && !defined(ARCH_386_64)
	if (restype == TY_LONGLONG || restype == TY_ULONGLONG)
		p_res[1] = xp_getreg(xp, 2);
#endif

	/*  Copy back any stuff we copied into the target's core.
	 */
	for (i = 0; i < nargs; ++i) {
		if (realargs[i] != args[i])
			ps_read_data(ip, realargs[i],
				     (char *)args[i], argsizes[i]);
	}

	/*  Restore the registers we saved.
	 */
#if defined(ARCH_VAX)
	ps_setreg(ip, UPSREG_PC, saved_restart_pc);
	ps_setreg(ip, UPSREG_SP, saved_sp);
	ps_setreg(ip, 0, saved_r0);
	ps_setreg(ip, 1, saved_r1);
#elif AO_HAS_PTRACE_REGS
	ps_set_all_regs(ip, orig_regs);
#elif defined(ARCH_MIPS)
	if (saveregs) {
		for (i = 1; i < 16; ++i) {
			if (ps_setreg(ip, i, regtab[i]) != 0)
				panic("can't restore registers in cf");
		}
		for (i = 24; i < 32 + 20; ++i) {
			if (ps_setreg(ip, i, regtab[i]) != 0)
				panic("can't restore registers in cf");
		}
	}
#elif defined(ARCH_CLIPPER)
	for (i = 0; i < N_REG_ARGS; ++i) {
		if (ps_setreg(ip, i, saved_regs[i]) != 0)
			panic("can't restore regs in cf");
	}
	if (ps_setreg(ip, UPSREG_SP, saved_sp) != 0)
		panic("can't restore sp in cf");
#elif defined(ARCH_BSDI386)
	for (i = 0; i < N_UREGS; ++i)
		if (set_uarea_reg(ip, i, saved_regs[i]) != 0)
			panic("can't restore regs in cf");
#elif defined(ARCH_LINUX386)
	for (i = 0; i < N_UREGS; ++i)
		if (i != ORIG_EAX && set_uarea_reg(ip, i, saved_regs[i]) != 0)
			panic("can't restore regs in cf");
#else
	ps_set_all_regs(ip, orig_regs);
#endif

	/*  Wait for target will have set ip_restart_pc to retpc, because
	 *  that is where the function returned.  We put this back.
	 */
	ip->ip_restart_pc = saved_restart_pc;

	return 0;
}

static int
ao_install_watchpoint(xp, addr, size)
target_t *xp;
taddr_t addr;
size_t size;
{
	return ps_install_watchpoint(GET_IPROC(xp), addr, size);
}

static int
ao_uninstall_watchpoint(xp, addr, size, watchnum)
target_t *xp;
taddr_t addr;
size_t size;
int watchnum;
{
	return ps_uninstall_watchpoint(GET_IPROC(xp), addr, size, watchnum);
}

#endif /* AO_TARGET */
