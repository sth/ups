/* ao_pt_uarea.c - process manipulation for a.out files using ptrace (part 3) */

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


/* @(#)ao_pt2.c	1.2 04 Jun 1995 (UKC) */
char ups_ao_pt_uarea_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#if AO_USE_PTRACE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>	/* needed for user.h */

#ifdef __STDC__
#include <unistd.h>
#endif

#if defined(OS_ULTRIX) || defined(ARCH_MIPS)
/*  The Ultrix system header files <setjmp.h> and <machine/pcb.h> clash
 *  over the #defines for some symbols, so we #undef them.  Hmmm ...
 */
#undef JB_S0
#undef JB_S1
#undef JB_S2
#undef JB_S3
#undef JB_S4
#undef JB_S5
#undef JB_S6
#undef JB_S7
#undef JB_S8
#undef JB_SP
#undef JB_PC
#undef NJBREGS
#endif

#include <sys/param.h>
#include <signal.h>
#ifdef OS_ULTRIX
#include <sys/dir.h>		/* VAX needs this for user.h */
#endif
/*
** Some system include files have a func_t typedef.
** This is to avoid colision with our func_t (RCB)
*/
#define func_t __func_t
#include <sys/user.h>
#undef func_t

#ifndef OS_LINUX /* RGA linux merge */
#include <machine/reg.h>
#endif
#include <errno.h>

#if HAVE_ASM_DEBUGREG_H
#include <asm/debugreg.h>
#endif

#ifndef DR_CONTROL
#ifdef OS_LINUX
#include <linux/debugreg.h>	/* for DR_CONTROL on Debian */
#endif
#endif

#ifdef ARCH_VAX
#include <machine/frame.h>
#endif
#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <local/arg.h>

#ifdef ARCH_MIPS
#include "mips_frame.h"
#endif
#include "ups.h"
#include "ui.h"
#include "ci.h"
#include "symtab.h"
#include "target.h"
#include "breakpoint.h"
#include "obj_bpt.h"	/* BUG: need this for recalculate_bpt_addrs */
#include "mreg.h"
#include "dx.h"
#include "as.h"
#include "ao.h"
#include "ao_syms.h"
#include "ao_asm.h"
#include "ao_text.h"
#include "ao_shlib.h"
#include "ao_core.h"
#include "ao_stack.h"
#include "ao_target.h"
#include "ao_ptrace.h"
#include "ao_aout.h"
#include "debug.h"
#include "tdr.h"
#include "state.h"

#if defined(OS_ULTRIX) && defined(ARCH_MIPS)
#define MIPS_ULTRIX
#endif

#ifdef ARCH_SUN4
#include <machine/cpu.h>

#ifdef CPU_ARCH
static int sparc_u_offset PROTO((void));
int gethostid PROTO((void));
#endif

#endif


/*  Return the offset into the u area of member.
 *  Used when doing PEEKUSER ptrace requests.
 */
#ifdef ARCH_MIPS
#define U_OFFSET(member)    ((char *)((int)&((struct user *)NULL)->member / 4))
#else
#define U_OFFSET(member)    ((char *)&((struct user *)NULL)->member)
#endif

#ifdef OS_SUNOS_4
#define u_tsize		u_procp->p_tsize
#define u_dsize		u_procp->p_dsize
#define u_ssize		u_procp->p_ssize
#endif /* OS_SUNOS_4 */


#ifdef UAREA_REGS
int get_uarea_reg PROTO((iproc_t *ip, int ri, taddr_t *p_value));
int set_uarea_reg PROTO((iproc_t *ip, int ri, taddr_t value));
#endif


#if defined(UAREA_REGS) || defined(USERCORE)
int reg_to_uarea_index PROTO((int regno));
#ifdef ARCH_BSDI386
typedef int (*get_uarea_word_func_t)PROTO((int arg, int offset));
void set_uarea_reg_offsets PROTO((ureg_t *ur,
				  get_uarea_word_func_t get_uarea_word_func,
				  int arg));
#else
void set_uarea_reg_offsets PROTO((ureg_t *ur));
#endif
#endif /* UAREA_REGS or USERCORE */


#ifdef ARCH_CLIPPER
int get_clipper_fp_reg PROTO((iproc_t *ip, int regno, taddr_t *p_val));
#endif

#ifdef ARCH_SUN3
static void convert_68881_reg PROTO((unsigned *rwords, bool is_double,
							fpval_t *p_val));
#endif

#ifdef ARCH_BSDI386
static int get_uarea_word_with_ptrace PROTO((int pid, int offset));
#endif


#if defined(UAREA_REGS) || defined(USERCORE)

#ifdef ARCH_LINUX386
static 
int
do_register_translation(regno)
int regno;
{
  static int regmap[] = {
	  EAX, ECX, EDX, EBX, UESP, EBP, ESI, EDI,
	  EIP, EFL, CS, SS, DS, ES, FS, GS, ORIG_EAX
	};
  return regmap[regno];
}
#endif


/*
 *  Convert machine independent register number regno to u area register number.
 */
int
reg_to_uarea_index(regno)
int regno;
{
	switch(regno) {
	case UPSREG_PC:
		return UR_PC;
	case UPSREG_SP:
		return UR_SP;
	case UPSREG_AP:
#ifdef ARCH_VAX
		return UR_AP;
	case UPSREG_CONDITION_CODES:
		return UR_PSL;
#endif
	case UPSREG_FP:
		return UR_FP;
	default:
		if (regno < 0 || regno >= N_UAREA_GREGS)
			panic("bad regno in xp_getreg");
#ifdef ARCH_LINUX386
		return do_register_translation(regno);
#else
		return regno;
#endif
	}
	/* NOTREACHED */
}

#ifdef ARCH_BSDI386
void
set_uarea_reg_offsets(ur, get_uarea_word, arg)
ureg_t *ur;
get_uarea_word_func_t get_uarea_word;
int arg;
{
	static int trapregs[N_UREGS] = {
		tEAX, tECX, tEDX, tEBX, tESP, tEBP, tESI, tEDI, tEIP
	};
#ifdef bsdi
	static int syscallregs[N_UREGS] = {
		sEAX, sECX, sEDX, sEBX, sESP, sEBP, sESI, sEDI, sEIP
	};
#endif
	taddr_t addr;
	int flags, offset, i, *regmap;

	flags = (*get_uarea_word)(arg, (int)U_OFFSET(u_pcb.pcb_flags));
#if defined(bsdi)
	regmap = ((flags & FM_TRAP) != 0) ? trapregs : syscallregs;
	addr = (*get_uarea_word)(arg, (int)U_OFFSET(u_kproc.kp_proc.p_regs));
#elif defined(KINFO_PROC_SIZE)
	{
	    /* FreeBSD 5.x */
	    /* Untested : will normally use 'ptrace()' to get registers. */
	    struct proc *paddr;
	    regmap = trapregs;
	    paddr = (struct proc *)(*get_uarea_word)(arg, (int)U_OFFSET(u_kproc.ki_paddr));
	    addr = (*get_uarea_word)(arg, (int)(paddr->p_md.md_regs));
	}
#else
	regmap = trapregs;
	addr = (*get_uarea_word)(arg, (int)U_OFFSET(u_kproc.kp_proc.p_md.md_regs));
#endif

	offset = addr - USRSTACK;

	for (i = 0; i < N_UREGS; ++i)
		ur[i].ur_uaddr = offset + 4 * regmap[i];
}
#endif /* ARCH_BSDI386 */
#endif /* UAREA_REGS or USERCORE */


#ifdef UAREA_REGS

#ifndef ARCH_BSDI386
/*  Set the offsets into the u area for the registers.
 *  Called once on startup.
 *  Not static because we also need this information set up for core
 *  files - thus we are called from core.c
 */
void
set_uarea_reg_offsets(ur)
ureg_t *ur;
{
#ifdef ARCH_VAX
#define RUADDR(rno)	(ctob(UPAGES) + (rno * 4))
	static taddr_t rmap[] = {
		RUADDR(R0),  RUADDR(R1),  RUADDR(R2),  RUADDR(R3),
		RUADDR(R4),  RUADDR(R5),  RUADDR(R6),  RUADDR(R7),
		RUADDR(R8),  RUADDR(R9),  RUADDR(R10), RUADDR(R11),
		RUADDR(AP),  RUADDR(FP),  RUADDR(SP),  RUADDR(PC),
		RUADDR(PS),
	};
#undef RUADDR
	taddr_t *rptr;

	rptr = rmap;
	while (rptr < rmap + N_UREGS)
		ur++->ur_uaddr = *rptr++;
#endif /* ARCH_VAX */
#ifdef ARCH_MIPS
	int i;

	for (i = 0; i < NGP_REGS + NFP_REGS; ++i)
		ur[i].ur_uaddr = i;

	if (i != UR_FP)
		panic("regno botch in suro");

	/*  This ureg is the frame pointer `register'.  We evaluate this
	 *  on the demand, so there is no u area address.  For safety,
	 *  make sure that any attempt to use it causes an error.
	 */
	ur[UR_FP].ur_uaddr = 20000;

	ur[UR_PC].ur_uaddr = PC;
#endif /* ARCH_MIPS */
#ifdef ARCH_CLIPPER
	int i;

	for (i = 0; i < 16; ++i)
		ur[i].ur_uaddr = (int)U_OFFSET(u_pcb.pcb_regs[i]);
	ur[UR_PC].ur_uaddr = (int)U_OFFSET(u_pcb.pcb_cxt.cxt_pc);
#endif
#ifdef ARCH_LINUX386

	/* RGA seems like we should use a regmap translation here
	 * so that ur[i].ur_uaddr = 4*regmap[i];. However this causes
	 * ptrace_wait_for_target() to return SR_SSTEP instead of SR_BPT
	 * on startup, so working around this by doing the register
	 * translation in do_register_translation() defined above.
	 * This is called from reg_to_uarea_index.
	 */

	int i;

	for (i = 0; i <= 16; i++)
	  	  	ur[i].ur_uaddr = 4*i;
#endif
}
#endif /* !ARCH_BSDI386 */


/*  Set *p_val to the current value of u area register ri.
 *  ri is a general purpose register number, or one of UR_PC, UR_AP, UR_FP
 *  and UR_SP.  See mreg.h
 */
int
get_uarea_reg(ip, ri, p_val)
iproc_t *ip;
int ri;
taddr_t *p_val;
{
	ureg_t *ur;

	ur = ip->ip_ptrace_info->uregs + ri;
	if (!ur->ur_is_current) {
		errno = 0;
#ifdef ARCH_MIPS
		if (ri == UR_FP) {
			taddr_t sp, pc;

			if (get_uarea_reg(ip, UR_SP, &sp) != 0)
				return -1;
			if (get_uarea_reg(ip, UR_PC, &pc) != 0)
				return -1;
			ur->ur_value = sp + get_frame_size(pc);
		}
		else {
			ur->ur_value = std_ptrace(PTRACE_PEEKUSER, ip->ip_pid,
							(char *)ur->ur_uaddr, 0);
		}
#else
		ur->ur_value = std_ptrace(PTRACE_PEEKUSER, ip->ip_pid,
							(char *)(int)ur->ur_uaddr, 0);
#endif
		if (errno != 0)
			return -1;
		ur->ur_is_current = TRUE;
	}
	*p_val = ur->ur_value;
	return 0;
}

int
set_uarea_reg(ip, ri, value)
iproc_t *ip;
int ri;
taddr_t value;
{
	ureg_t *ur;

	ur = ip->ip_ptrace_info->uregs + ri;
	errno = 0;
	e_ptrace(PTRACE_POKEUSER, ip->ip_pid, (char *)(int)ur->ur_uaddr, (int)value);
	ur->ur_is_current = FALSE;
	return (errno == 0) ? 0 : -1;
}


#ifdef ARCH_CLIPPER
int
get_clipper_fp_reg(xp, regno, p_val)
target_t *xp;
int regno;
taddr_t *p_val;
{
	ureg_t *ur;

	if (ip->ip_ptrace_info->fpreg_sp == 0) {
		static unsigned short text[] = {
			0xb420,		/* saved0 */
			0x0000,		/* second word of saved0 */
			0x0900,		/* BPT */
			0x0000,		/* nop */
		};
		taddr_t pc, saved_restart_pc, saved_sp, sp, new_sp, word1, word2;
		int errno1, errno2, i;

		saved_sp = sp = xp_getreg(xp, UPSREG_SP);

		/*  Back off to a word aligned place at least four halfwords
		 *  before the current pc.
		 */
		saved_restart_pc = ip->ip_restart_pc;
		pc = (ip->ip_restart_pc - 8) & ~3;

		word1 = std_ptrace(PTRACE_PEEKTEXT, ip->ip_pid, (char *)pc, 0);
		errno1 = errno;
		word2 = std_ptrace(PTRACE_PEEKTEXT, ip->ip_pid, (char *)(pc + 4), 0);
		errno2 = errno;

		std_ptrace(PTRACE_POKETEXT, ip->ip_pid, (char *)pc, *(int *)text);

		/*  At this point we can still back out.  Beyond this point
		 *  we have trodden on the text, so any errors are fatal.
		 */
		if (errno != 0 || errno1 != 0 || errno2 != 0) {
			errf("Can't insert fp register grabbing code (%m)");
			return -1;
		}

		e_ptrace(PTRACE_POKETEXT, ip->ip_pid,
						(char *)(pc + 4), *(int *)&text[2]);

		/*  Ensure that the stack pointer is eight byte aligned
		 */
		if (sp & 7) {
			sp &= ~7;
			if (ao_setreg(xp, UPSREG_SP, sp) != 0)
				panic("can't set sp in gifr");
		}
		
		e_ptrace(PTRACE_CONT, ip->ip_pid, (char *)pc, 0);
		ptrace_wait_for_target(ip);

		new_sp = xp_getreg(xp, UPSREG_SP);

		/*  Make sure we've hit the breakpoint, and that the sp
		 *  has been moved by the saved0.
		 */
		if (ip->ip_stopres != SR_BPT && ip->ip_stopres != SR_SSTEP)
			panic("ws botch in gcfr");

		e_ptrace(PTRACE_POKETEXT, ip->ip_pid, (char *)pc, word1);
		e_ptrace(PTRACE_POKETEXT, ip->ip_pid, (char *)(pc + 4), word2);

		/*  Sometimes the pc doesn't advance.  I don't know why.
		 */
		if (ip->ip_restart_pc == pc) {
			errf("Error running fp regno grabbing code");
			ip->ip_restart_pc = saved_restart_pc;
			return -1;
		}

		if (ip->ip_restart_pc != pc + 4)
			panic("pc botch in gcfr");
		if (new_sp != sp - 4 * 16)
			panic("sp botch in gcfr");

		ip->ip_restart_pc = saved_restart_pc;

		ip->ip_ptrace_info->fpreg_sp = sp - 8 * 8;	/* 8 eight byte doubles */
		ao_setreg(xp, UPSREG_SP, saved_sp);

		for (i = 0; i < 16; ++i)
			ip->ip_ptrace_info->fp_uregs[i].ur_is_current = FALSE;
	}

	ur = &ip->ip_ptrace_info->fp_uregs[regno];
	if (!ur->ur_is_current) {
		if (ao_read_data(xp, ip->ip_ptrace_info->fpreg_sp + regno * 4,
					    (char *)&ur->ur_value, 4) != 0) {
			errf("Can't read fp register value from the stack (%m)");
			return -1;
		}
		ur->ur_is_current = TRUE;
	}

	*p_val = ur->ur_value;
	return 0;
}
#endif /* ARCH_CLIPPER */


#ifdef ARCH_BSDI386
static int
get_uarea_word_with_ptrace(pid, offset)
int pid, offset;
{
	return e_ptrace(PTRACE_PEEKUSER, pid, (char *)offset, 0);
}
#endif /* ARCH_BSDI386 */


/*  Update the stored machine register values.  This is called after the
 *  target has been run and has thus changed the register values.
 *  For the VAX, we just mark our cached values as invalid.
 *
 *  The pc is a special case, as when we hit a breakpoint some machines
 *  report the pc as pointing after the trap opcode.  Thus if we are at
 *  a breakpoint we adjust the pc value.  The VAX kernel seems to do
 *  this for us.
 *
 *  Return TRUE if the target stopped because it hit a breakpoint.
 */
int
ptrace_update_ua_regs(xp)
target_t *xp;
{
	taddr_t pc;
	breakpoint_t *bp;
	iproc_t *ip;
	ureg_t *ur;

	ip = GET_IPROC(xp);

	for (ur = ip->ip_ptrace_info->uregs;
             ur < ip->ip_ptrace_info->uregs + N_UREGS;
             ++ur)
		ur->ur_is_current = FALSE;

#ifdef ARCH_BSDI386
	set_uarea_reg_offsets(ip->ip_ptrace_info->uregs,
			      get_uarea_word_with_ptrace, ip->ip_pid);
#endif

#ifdef ARCH_CLIPPER
	ip->ip_ptrace_info->fpreg_sp = 0;
#endif

	if (get_uarea_reg(ip, UR_PC, &pc) != 0)
		panic("can't get pc in ur");
	bp = get_breakpoint_at_addr(xp, pc - BPT_PC_OFFSET);
	if (bp != NULL) {
		pc -= BPT_PC_OFFSET;
		ip->ip_ptrace_info->uregs[UR_PC].ur_value = pc;
	}

	ip->ip_restart_pc = pc;
	return bp != NULL;
}
#endif /* UAREA_REGS */


#ifdef UAREA_DREGS

/*  Set the offsets into the u area for the debug registers.
 *  Called once on startup.
 */
static void
set_uarea_dreg_offsets(ur)
ureg_t *ur;
{
	int i;

	for (i = 0; i < N_UDREGS; i++)
		ur[i].ur_uaddr = (int)U_OFFSET(u_debugreg[i]);

	return;
}


/*  Set *p_val to the current value of u area debug register ri.
 */
int
get_uarea_dreg(ip, ri, p_val)
iproc_t *ip;
int ri;
taddr_t *p_val;
{
	ureg_t *ur;

	ur = ip->ip_ptrace_info->udregs + ri;
	if (!ur->ur_is_current) {
		errno = 0;
		ur->ur_value = std_ptrace(PTRACE_PEEKUSER, ip->ip_pid,
					  (char *)(int)ur->ur_uaddr, 0);
		if (errno != 0)
			return -1;
		ur->ur_is_current = TRUE;
	}
	*p_val = ur->ur_value;
	return 0;
}


int
set_uarea_dreg(ip, ri, value)
iproc_t *ip;
int ri;
taddr_t value;
{
	ureg_t *ur;

	ur = ip->ip_ptrace_info->udregs + ri;
	errno = 0;
	e_ptrace(PTRACE_POKEUSER, ip->ip_pid, (char *)(int)ur->ur_uaddr, (int)value);
	ur->ur_is_current = FALSE;
	return (errno == 0) ? 0 : -1;
}


/*  Update the stored machine debug register values.  This is called after
 *  the target has been run and has thus changed the register values.
 *
 *  Return TRUE if the target stopped because it hit a watchpoint.
 */
int
ptrace_update_ua_dregs(xp)
target_t *xp;
{
	iproc_t *ip;
	ureg_t *ur;
	taddr_t status;

	ip = GET_IPROC(xp);

	for (ur = ip->ip_ptrace_info->udregs;
             ur < ip->ip_ptrace_info->udregs + N_UDREGS;
             ++ur)
		ur->ur_is_current = FALSE;

#ifdef ARCH_386
	if (get_uarea_dreg(ip, DR_STATUS, &status) != 0)
		panic("can't get status in udr");

	return (status & (DR_TRAP0|DR_TRAP1|DR_TRAP2|DR_TRAP3)) != 0;
#else
	panic("don't know how to check watchpoint status for this machine");
#endif
}

#endif /* UAREA_DREGS */


void
ptrace_init_pid(ip, pid, attached)
iproc_t *ip;
int pid;
bool attached;
{
	if (ip->ip_ptrace_info == NULL) {
		ip->ip_ptrace_info =
			(Ptrace_info *)e_malloc(sizeof(Ptrace_info));
	}
	
	ip->ip_core = NULL;
	ip->ip_pid = pid;
	ip->ip_attached = attached;

#if defined(UAREA_REGS) && !defined(ARCH_BSDI386)
	set_uarea_reg_offsets(ip->ip_ptrace_info->uregs);
#endif

#if defined(UAREA_DREGS)
	set_uarea_dreg_offsets(ip->ip_ptrace_info->udregs);
#endif

#ifdef ARCH_CLIPPER
	ip->ip_ptrace_info->fpreg_sp = 0;
#endif

#ifdef ARCH_SUN3
	ip->ip_ptrace_info->sunregs.sr_fptype = FPT_68881;	/* BUG: just for testing */
#endif

#ifdef OS_SUNOS
	/*  Purify doesn't seem to know that PTRACE_GETREGS sets
	 *  all of a regs structure.
	 */
	memset((char *)&ip->ip_ptrace_info->sunregs, '\0', sizeof(ip->ip_ptrace_info->sunregs));
#endif
}


#if defined(ARCH_SUN4) && defined(CPU_ARCH)
/*  The Sun 4m CPU has an extra 1024 byte field at the start of the
 *  u area.  This means we have to add or subtract this amount if we
 *  are running on a different CPU architecture than the one we were
 *  compiled on.
 */
static int
sparc_u_offset()
{
	static int offset;
	static bool need_offset = TRUE;

	if (need_offset) {
		int cputype;

		cputype = (gethostid() >> 24) & CPU_ARCH;

#ifdef SUN4M_ARCH
		offset = (cputype == SUN4M_ARCH) ? 0 : -1024;
#else
		offset = (cputype == 0x70) ? 1024 : 0;
#endif
		
		if ((Debug_flags & DBFLAG_MISC) != 0)
			printf("sparc_u_offset = %d\n", offset);
		
		need_offset = FALSE;
	}

	return offset;
}
#endif


/*  Return the current state of signal handling for signal sig in process
 *  xp.  Returns SGH_CAUGHT, SGH_IGNORED or SGH_DEFAULT.
 *
 *  You might think we could return the address of the signal handler for
 *  caught signals, but on the Suns at least the returned address is that
 *  of a library routine which handles all caught signals.
 */
#ifdef OS_LINUX

#define LINUX_STAT_FORMAT "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u " \
                          "%*u %*d %*d %*d %*d %*d %*d %*u %*u %*d %*u %*u " \
                          "%*u %*u %*u %*u %*u %*u %*d %*d %d %d %*u"

sigstate_t
ptrace_get_sigstate(ip, signo)
iproc_t *ip;
int signo;
{
	sigstate_t res;
	char filename[64];
	FILE *f;
   
	sprintf(filename, "/proc/%d/stat", ip->ip_pid);
	
	if ((f = fopen(filename, "r")) != NULL) {
		char buf[4096];

		if (fgets(buf, sizeof(buf), f)) {
			int mask = 1 << (signo - 1);
			int ignored;
			int caught;
		   
			if (sscanf(buf, LINUX_STAT_FORMAT, &ignored, &caught) == 2) {
				if ((ignored & mask) != 0)
					res = SGH_IGNORED;
				else if ((caught & mask) != 0)
					res = SGH_CAUGHT;
				else
					res = SGH_DEFAULT;
			}
			else {
				panic("Can't parse process status file");
			}
		}
		else {
			panic("Can't read process status file");
		}
	}
	else {
		panic("Can't open process status file");
	}

	return res;
}

#else

sigstate_t
ptrace_get_sigstate(ip, signo)
iproc_t *ip;
int signo;
{
	caddr_t uaddr;
	taddr_t handler;
	sigstate_t res;

	if (signo < 1 || signo >= NSIG)
		panic("signo botch in gss");

#if defined(ARCH_MIPS)
	uaddr = (char *)(SIG_BASE + signo);
#elif defined(ARCH_BSDI386)
	uaddr = U_OFFSET(u_sigacts.ps_sigact[signo]);
#elif defined(ARCH_SUN4) && defined(CPU_ARCH)
	uaddr = U_OFFSET(u_signal[signo]) + sparc_u_offset();
#else
	uaddr = U_OFFSET(u_signal[signo]);
#endif

#if defined(PTRACE_PEEKUSER)
	errno = 0;
	handler = e_ptrace(PTRACE_PEEKUSER, ip->ip_pid, (char *)uaddr, 0);
#else
	errf("ptrace_get_sigstate : UAREA desupported by OS, no signal state");
	return SGH_DEFAULT; /* guess */
#endif

	if (handler == (taddr_t)SIG_IGN)
		res = SGH_IGNORED;
	else if (handler == (taddr_t)SIG_DFL)
		res = SGH_DEFAULT;
	else
		res = SGH_CAUGHT;

	return res;
}

#endif /* OS_LINUX */

#endif /* AO_USE_PTRACE */
