/* ao_pt_regs.c - use ptrace to get/set registers directly, not via u-area */

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


/* @(#)ao_pt_regs.c	1.2 04 Jun 1995 (UKC) */
char ups_ao_pt_regs_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#if AO_USE_PTRACE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifdef __STDC__
#include <unistd.h>
#endif

#include <sys/param.h>
#include <errno.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <mtrprog/hash.h>
#include <local/arg.h>

#include "ups.h"
#include "ui.h"
#include "ci.h"
#include "symtab.h"
#include "target.h"
#include "breakpoint.h"
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
#ifdef AO_ELF
#include "ao_elfcore.h"
#endif
#include "debug.h"
#include "tdr.h"
#include "state.h"


/*
 *  Macros for getting/setting registers in a 'regs' structure.
 */

#ifdef ARCH_SUN3
#define IS_FLOAT_REG(regno)	((regno) >= 16)
#define PC_REG(sr)		((sr)->sr_regs.r_pc)
#define SP_REG(sr)		((sr)->sr_regs.r_sp)
#define FRAME_REG(sr)		((sr)->sr_regs.r_areg[6])
#define INTEGER_REG(sr, regno)	((sr)->sr_regs.r_dreg[regno])
#endif

#ifdef ARCH_SUN4
#define IS_FLOAT_REG(regno)	((regno) >= 32 && (regno) < 64)
#define FLOAT_REG_OFFSET	32
#define PC_REG(sr)		((sr)->sr_regs.r_pc)
#define SP_REG(sr)		((sr)->sr_regs.r_sp)
#define FRAME_REG(sr)		((sr)->sr_rwindow.rw_fp)
#define INTEGER_REG(sr, regno)	((&(sr)->sr_regs.r_y)[regno])
#endif

#if (defined ARCH_LINUX386)

#define PC_REG(pr)		((pr)->regs.eip)
#define SP_REG(pr)		((pr)->regs.esp)
#define FRAME_REG(pr)		((pr)->regs.ebp)
#define INTEGER_REG(pr, regno)	((&(pr)->regs.ebx)[x86_gcc_register(regno)])

#elif (defined ARCH_FREEBSD386)

/* /usr/include/machine/reg.h */
#define PC_REG(pr)		((pr)->regs.r_eip)
#define SP_REG(pr)		((pr)->regs.r_esp)
#define FRAME_REG(pr)		((pr)->regs.r_ebp)
#if (defined __FreeBSD__) && __FreeBSD__ > 3
#define INTEGER_REG(pr, regno)	((&(pr)->regs.r_fs)[x86_gcc_register(regno)])
#else
#define INTEGER_REG(pr, regno)	((&(pr)->regs.r_es)[x86_gcc_register(regno)])
#endif

#elif (defined ARCH_BSDI386)

#define PC_REG(pr)		((pr)->regs.r_eip)
#define SP_REG(pr)		((pr)->regs.r_esp)
#define FRAME_REG(pr)		((pr)->regs.r_ebp)
#define INTEGER_REG(pr, regno)	((&(pr)->regs.r_es)[x86_gcc_register(regno)])

#endif


#ifdef OS_SUNOS
static int set_sun_regval PROTO((sunregs_t *sr, int pid,
						int regno, taddr_t val));
#endif

#ifdef ARCH_SUN3
static void convert_68881_reg PROTO((unsigned *rwords, bool is_double,
							fpval_t *p_val));
#endif


#if AO_HAS_PTRACE_REGS

static taddr_t get_ptrace_reg PROTO((ptrace_regs_t *pr, int pid, int regno));
static int set_ptrace_reg PROTO((ptrace_regs_t *pr, int pid, int regno, taddr_t val));
static int ptrace_update_pt_regs PROTO((target_t *xp));

#if AO_HAS_PTRACE_DREGS
static taddr_t get_ptrace_dreg PROTO((ptrace_regs_t *pr, int pid, int regno));
static int set_ptrace_dreg PROTO((ptrace_regs_t *pr, int pid, int regno, taddr_t val));
static int ptrace_update_pt_dregs PROTO((target_t *xp));
#endif

#ifdef OS_SUNOS
/*  Return the current value of Sun register regno.
 *  regno is a machine independent register number.
 */
taddr_t
get_sun_regval(sr, pid, reg)
sunregs_t *sr;
int pid, reg;
{
	switch(reg) {
	case UPSREG_PC:
		return PC_REG(sr);
	case UPSREG_SP:
		return SP_REG(sr);
	case UPSREG_FP:
	case UPSREG_AP:
		return FRAME_REG(sr);
#ifdef ARCH_SUN4
	case UPSREG_CONDITION_CODES:
		return sr->sr_regs.r_psr;
#endif
	default:
#ifdef ARCH_SUN4
		if (reg == UPSREG_FP_CONDITION_CODES || (reg >= 32 && reg < 64)) {
			if (sr->sr_need_fpu) {
				e_ptrace(PTRACE_GETFPREGS, pid,
					 (char *)&sr->sr_fpu, 0);
				sr->sr_need_fpu = FALSE;
			}
			if (reg == UPSREG_FP_CONDITION_CODES)
				return sr->sr_fpu.fpu_fsr;
			return sr->sr_fpu.fpu_regs[reg - FLOAT_REG_OFFSET];
		}
#endif
		if (reg < 0 || reg >= N_SUN_GREGS)
			panic("bad reg in gsr");
#ifdef ARCH_SUN4
		if (reg == 0)
			return 0;
#endif
		return INTEGER_REG(sr, reg);
	}
}


static int
set_sun_regval(sr, pid, regno, val)
sunregs_t *sr;
int pid, regno;
taddr_t val;
{
#ifdef ARCH_SUN3
	/* BUG */
	if (regno >= 14) {
		errf("\bCan't handle 68881/fpa register floats yet");
		return -1;
	}
#endif
		
	switch(regno) {
	case UPSREG_PC:
		PC_REG(sr) = val;
		break;
	case UPSREG_FP:
	case UPSREG_AP:
		FRAME_REG(sr) = val;
		break;
	case UPSREG_SP:
		SP_REG(sr) = val;
		break;
	default:
#ifdef ARCH_SUN4
		if (IS_FLOAT_REG(regno)) {
			if (sr->sr_need_fpu) {
				e_ptrace(PTRACE_GETFPREGS, pid,
						       (char *)&sr->sr_fpu, 0);
				sr->sr_need_fpu = FALSE;
			}
			sr->sr_fpu.fpu_regs[regno - FLOAT_REG_OFFSET] = val;

			if (std_ptrace(PTRACE_SETFPREGS, pid,
						(char *)&sr->sr_fpu, 0) != 0) {
				sr->sr_need_fpu = TRUE;
				return -1;
			}
			return 0;
		}
#endif
		if (regno < 0 || regno >= N_SUN_GREGS)
			panic("bad regno in ssr");
		INTEGER_REG(sr, regno) = val;
		break;
	}

	errno = 0;
	e_ptrace(PTRACE_SETREGS, pid, (char *)sr, 0);
	if (errno != 0)
		return -1;

#ifdef ARCH_SUN4
	if (ptrace(PTRACE_WRITEDATA, pid, (char *)SP_REG(sr),
		          sizeof(sr->sr_rwindow), (char *)&sr->sr_rwindow) != 0)
		return -1;
#endif

	return 0;
}


#else


/*
 *  Return the current value of ptrace'd register 'regno', which is a
 *  machine independent register number.
 */
static taddr_t
get_ptrace_reg(pr, pid, regno)
ptrace_regs_t *pr;
int pid, regno;
{
	switch(regno) {
	case UPSREG_PC:
		return PC_REG(pr);
	case UPSREG_SP:
		return SP_REG(pr);
	case UPSREG_FP:
	case UPSREG_AP:
		return FRAME_REG(pr);
	default:
		if (regno < 0 || regno >= sizeof(pr->regs)/sizeof(PC_REG(pr)))
			panic("bad regno in gpr");
		return INTEGER_REG(pr, regno);
	}
}


static int
set_ptrace_reg(pr, pid, regno, val)
ptrace_regs_t *pr;
int pid, regno;
taddr_t val;
{
	switch(regno) {
	case UPSREG_PC:
		PC_REG(pr) = val;
		break;
	case UPSREG_FP:
	case UPSREG_AP:
		FRAME_REG(pr) = val;
		break;
	case UPSREG_SP:
		SP_REG(pr) = val;
		break;
	default:
		if (regno < 0 || regno >= sizeof(pr->regs)/sizeof(PC_REG(pr)))
			panic("bad regno in spr");
		INTEGER_REG(pr, regno) = val;
		break;
	}

	errno = 0;
#ifdef OS_LINUX
	e_ptrace(PTRACE_SETREGS, pid, 0, (int)&pr->regs);
#else
	e_ptrace(PTRACE_SETREGS, pid, (char *)&pr->regs, 0);
#endif
        if (errno != 0)
		return -1;

	return 0;
}


#endif /* OS_SUNOS */


/*  Update the stored machine register values.  This is called after the
 *  target has been run and has thus changed the register values.
 *
 *  The pc is a special case, as when we hit a breakpoint some machines
 *  report the pc as pointing after the trap opcode.  Thus if we are at
 *  a breakpoint we adjust the pc value.
 *
 *  Return TRUE if the target stopped because it hit a breakpoint.
 */
static int
ptrace_update_pt_regs(xp)
target_t *xp;
{
	taddr_t pc;
	breakpoint_t *bp;
	iproc_t *ip;
#ifdef OS_SUNOS
	sunregs_t *sr;
#else
	ptrace_regs_t *pr;
#endif

	ip = GET_IPROC(xp);

#ifdef OS_SUNOS
	sr = &ip->ip_ptrace_info->sunregs;
	e_ptrace(PTRACE_GETREGS, ip->ip_pid, (char *)&sr->sr_regs, 0);
#ifdef ARCH_SUN4
	if (ptrace(PTRACE_READDATA, ip->ip_pid, (char *)SP_REG(sr),
		        sizeof(sr->sr_rwindow), (char *)&sr->sr_rwindow) != 0) {
		panic("rwindow read botch in pur");
	}
#endif
	pc = (taddr_t)PC_REG(sr);
	bp = get_breakpoint_at_addr(xp, pc - BPT_PC_OFFSET);
	if (bp != NULL) {
		pc -= BPT_PC_OFFSET;
		PC_REG(sr) = pc;
	}
#if defined(ARCH_SUN3) || defined(ARCH_SUN4)
	sr->sr_need_fpu = TRUE;
#endif

#else /* not OS_SUNOS */

	pr = &ip->ip_ptrace_info->ptrace_regs;
#ifdef OS_LINUX
        e_ptrace(PTRACE_GETREGS, ip->ip_pid, 0, (int)&pr->regs);
#else
        e_ptrace(PTRACE_GETREGS, ip->ip_pid, (char *)&pr->regs, 0);
#endif
	pc = (taddr_t)PC_REG(pr);
	bp = get_breakpoint_at_addr(xp, pc - BPT_PC_OFFSET);
	if (bp != NULL) {
		pc -= BPT_PC_OFFSET;
		PC_REG(pr) = pc;
	}
	pr->need_fpregs = TRUE;
#endif
	ip->ip_restart_pc = pc;
	return bp != NULL;
}

#if defined(OS_SUNOS)
void
ptrace_get_regtab(ip, sr)
iproc_t *ip;
sunregs_t *sr;
{
	*sr = ip->ip_ptrace_info->sunregs;
}

bool
ptrace_set_all_regs(ip, sr)
iproc_t *ip;
sunregs_t *sr;
{
 	if (std_ptrace(PTRACE_SETREGS, ip->ip_pid,
 		       (char *)&sr->sr_regs, 0) != 0) {
		return FALSE;
	}
	ip->ip_ptrace_info->sunregs = *sr;
	return TRUE;
}

#else

void
ptrace_get_regtab(ip, pr)
iproc_t *ip;
ptrace_regs_t *pr;
{
	*pr = ip->ip_ptrace_info->ptrace_regs;
}

bool
ptrace_set_all_regs(ip, pr)
iproc_t *ip;
ptrace_regs_t *pr;
{
#ifdef OS_LINUX
	if (std_ptrace(PTRACE_SETREGS, ip->ip_pid, 0, (char *)&pr->regs) != 0) {
#else
	if (std_ptrace(PTRACE_SETREGS, ip->ip_pid, (char *)&pr->regs, 0) != 0) {
#endif
		return FALSE;
	}

	ip->ip_ptrace_info->ptrace_regs = *pr;
	return TRUE;
}
#endif


#endif /* AO_HAS_PTRACE_REGS */


/*
 *  This maps 'regno' (the register number GCC puts in the symbol table)
 *  to the actual register.
 *  Originally copied from 'do_register_translation()' in ao_pt_uarea.c.
 *  FIX: where does this come from ?  Must be some way to get it out
 *  of the header files.
 */
int
x86_gcc_register(regno)
int regno;
{
#if (defined ARCH_LINUX386)
  static int regmap[] = {
	  EAX, ECX, EDX, EBX, UESP, EBP, ESI, EDI,
	  EIP, EFL, CS, SS, DS, ES, FS, GS, ORIG_EAX
	};
  return regmap[regno];
#elif (defined ARCH_FREEBSD386) && defined(z__FreeBSD__) && __FreeBSD__ > 3
  static int regmap[] = {
	/*  9     8     7     6     5    4      3     2 */
	  tECX, tEDX, tEBX, tISP, tEBP, tESI, tEDI, tDS,
	/*  12    14  13   16   1    0 */
	  tERR, tCS, tEIP, tESP, tES, tFS
	};
  return regmap[regno];
#elif (defined ARCH_FREEBSD386)
  static int regmap[] = {
	/*  9     8     7     6     5    4      3     2 */
	  tEAX, tECX, tEDX, tEBX, tESP, tEBP, tESI, tEDI,
	/*  12    14      13   16   1    0 */
	  tEIP, tEFLAGS, tCS, tSS, tDS, tES
	};
  return regmap[regno];
#else
  return regno;
#endif
}


/*
 * ptrace_update_regs
 */
int
ptrace_update_regs(xp)
target_t *xp;
{
#if AO_HAS_PTRACE_REGS
    return ptrace_update_pt_regs (xp);
#elif defined(UAREA_REGS)
    return ptrace_update_ua_regs (xp);
#else
    /*    panic ("don't know how to update register values for this machine");*/
    return 0;
#endif
}


#if AO_HAS_PTRACE_DREGS
/*
 *  Update the stored machine debug register values.  This is called after
 *  the target has been run and has thus changed the register values.
 *
 *  Return TRUE if the target stopped because it hit a watchpoint.
 */
static int
ptrace_update_pt_dregs(xp)
target_t *xp;
{
	iproc_t *ip;
	ptrace_regs_t *pr;
	taddr_t status;

	ip = GET_IPROC(xp);
	pr = &ip->ip_ptrace_info->ptrace_regs;

	e_ptrace(PTRACE_GETDREGS, ip->ip_pid, (char *)&pr->dbregs, 0);

	/* FreeBSD : .../sys/i386/i386/machdep.c
	 * dr6 is status register, dr7 is control register.
	 */
	return (pr->dbregs.dr6 & 0x0000000f);
}

/*
 *
 */
int
ptrace_set_dregs(ip)
iproc_t *ip;
{
	ptrace_regs_t *pr;
	taddr_t status;

	pr = &ip->ip_ptrace_info->ptrace_regs;

	errno = 0;
	std_ptrace(PTRACE_SETDREGS, ip->ip_pid, (char *)&pr->dbregs, 0);
	if (errno != 0)
		return -1;
	return 0;
}

/*
 *  Return the current value of ptrace'd debug register 'regno'.
 */
static taddr_t
get_ptrace_dreg(pr, pid, regno)
ptrace_regs_t *pr;
int pid, regno;
{
    /* return ((&(pr)->dbregs.dr0)[regno]); */
    return DBREG_DRX((&(pr)->dbregs),regno);
}

/*
 *  Set the value of ptrace'd debug register 'regno'.
 */
static int
set_ptrace_dreg(pr, pid, regno, val)
ptrace_regs_t *pr;
int pid, regno;
taddr_t val;
{
    DBREG_DRX((&(pr)->dbregs),regno) = val;

    errno = 0;
    e_ptrace(PTRACE_SETDREGS, pid, (char *)&pr->dbregs, 0);
    if (errno != 0)
	return -1;
    return 0;
}
#endif


/*
 * ptrace_update_dregs
 */
int
ptrace_update_dregs(xp)
target_t *xp;
{
#if AO_HAS_PTRACE_DREGS
    return ptrace_update_pt_dregs (xp);
#elif defined(UAREA_DREGS)
    return ptrace_update_ua_dregs (xp);
#else
    panic ("don't know how to update debug register values for this machine");
    return 0;
#endif
}


/*
 * ptrace_setreg
 */
int
ptrace_setreg(ip, regno, value)
iproc_t *ip;
int regno;
taddr_t value;
{
#if defined(OS_SUNOS)
	return set_sun_regval (&ip->ip_ptrace_info->sunregs,
			       ip->ip_pid, regno, value);
#elif AO_HAS_PTRACE_REGS
	return set_ptrace_reg (&ip->ip_ptrace_info->ptrace_regs,
			       ip->ip_pid, regno, value);
#elif defined(UAREA_REGS)
	return set_uarea_reg(ip, reg_to_uarea_index(regno), value);
#else
	/*	panic ("don't know how to set a register value for this machine");*/
	return 0;
#endif
}


/*
 * ptrace_readreg
 */
int
ptrace_readreg(ip, regno, p_val)
iproc_t *ip;
int regno;
taddr_t *p_val;
{
	/* If pid == 0 then reading a core file. */
	if (ip->ip_pid == 0) {
#ifdef ARCH_CLIPPER
		if (regno >= 16 && regno < 32) {
			errf("Sorry, can't get fp registers from core files on the Clipper");
			*p_val = 0;
			return 0;
		}
#endif
#ifdef AO_ELF
		return elf_get_core_reg(ip->ip_core, regno, p_val);
#else
		return aout_get_core_reg(ip->ip_core, regno, p_val);
#endif
	}

#if defined(OS_SUNOS)
	*p_val = get_sun_regval (&ip->ip_ptrace_info->sunregs,
				 ip->ip_pid, regno);
	return 0;
#elif AO_HAS_PTRACE_REGS
	*p_val = get_ptrace_reg (&ip->ip_ptrace_info->ptrace_regs,
				 ip->ip_pid, regno);
	return 0;
#elif defined(UAREA_REGS)
#ifdef ARCH_CLIPPER
	if (regno >= 16 && regno < 32)
		return get_clipper_fp_reg (ip, regno - 16, p_val);
#endif
	return get_uarea_reg (ip, reg_to_uarea_index(regno), p_val);
#else
	/*	panic ("don't know how to get a register value for this machine");*/
	return 0;
#endif
}


/*
 * ptrace_setdreg
 */
int
ptrace_setdreg(ip, regno, value)
iproc_t *ip;
int regno;
taddr_t value;
{
#if AO_HAS_PTRACE_DREGS
	return set_ptrace_dreg(&ip->ip_ptrace_info->ptrace_regs, ip->ip_pid, regno, value);
#elif defined(UAREA_DREGS)
	return set_uarea_dreg(ip, regno, value);
#else
	/*	panic ("don't know how to set a debug register value for this machine");*/
	return 0;
#endif
}


/*
 * ptrace_readdreg
 */
int
ptrace_readdreg(ip, regno, p_val)
iproc_t *ip;
int regno;
taddr_t *p_val;
{
	/* If pid == 0 then reading a core file. */
	if (ip->ip_pid == 0) {
		errf("Sorry, can't get debug registers from core files");
		*p_val = 0;
		return 0;
	}

#if AO_HAS_PTRACE_DREGS
	*p_val = get_ptrace_dreg(&ip->ip_ptrace_info->ptrace_regs, ip->ip_pid, regno);
	return 0;
#elif defined(UAREA_DREGS)
	return get_uarea_dreg(ip, regno, p_val);
#else
	/*	panic ("don't know how to get a debug register value for this machine");*/
	return 0;
#endif
}


/*
 *  ptrace_read_fpreg
 */
int
ptrace_read_fpreg(ip, regno, num_bytes, p_val)
iproc_t *ip;
int regno;
int num_bytes;
fpval_t *p_val;
{
#ifdef ARCH_SUN3
	sunregs_t *sr;

	/*  The f68881 has eight registers, which are numbered 18..25 in
	 *  the symbol table.
	 */
	regno -= 18;
	if (regno < 0 || regno >= 8)
		panic("bad regno in rf");
	
	ip = GET_IPROC(xp);
	sr = &ip->ip_ptrace_info->sunregs;
	if (sr->sr_need_fpu) {
		e_ptrace(PTRACE_GETFPREGS, ip->ip_pid, (char *)&sr->sr_fpu, 0);
		sr->sr_need_fpu = FALSE;
	}

	convert_68881_reg((unsigned *)sr->sr_fpu.f_fpstatus.fps_regs[regno].fp,
						(num_bytes == sizeof(double), p_val);
	return 0;
#else
 	switch (num_bytes) {
 	    case sizeof(float):
 		if (ptrace_readreg(ip, regno, (taddr_t *)&p_val->f) != 0)
 			return -1;
 		break;
 	    case sizeof(double):
 		{
		int regno2;
		taddr_t *buf = (taddr_t *)&p_val->d;

		regno2 = regno + 1;
#ifdef ARCH_BSDI386
		if (regno == 3)
			regno2 = 6;
#endif
		if (ptrace_readreg(ip, regno, buf) != 0 ||
		    ptrace_readreg(ip, regno2, buf+1) != 0)
			return -1;
		}
 		break;
 	    default:
 		panic("cannot get this size floating point from register");
 		break;
	}
	return 0;
#endif
}


#endif /* AO_USE_PTRACE */
