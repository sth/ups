/* mreg.h - machine dependent register information */

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


/* @(#)mreg.h	1.13 24 May 1995 (UKC) */

#ifdef ARCH_SUN3
#define N_SUN_GREGS		14
#endif

#ifdef ARCH_SUN4
#define N_SUN_GREGS		32
#endif

#ifdef ARCH_BSDI386
/*  The first eight values are the general register numbers used in the
 *  symbol table.  These are mapped to a u area offset by the trapregs
 *  and syscallregs arrays in set_uarea_reg_offsets in proc.c.
 *
 *  UR_FP, UR_SP and UR_PC are also mapped by the *regs arrays - don't
 *  change their values without changing the arrays.
 */
#define N_UAREA_GREGS		8
#define UR_SP			4
#define UR_FP			5
#define UR_PC			N_UAREA_GREGS
#define N_UREGS			(N_UAREA_GREGS + 1)
#endif

#ifdef OS_LINUX
#define N_UAREA_GREGS		17
#define UR_SP			15
#define UR_FP			5
#define UR_PC			12
#define N_UREGS			17
#define UPAGES 1
#define ctob(x)	(0x1000*(x))
#define USRSTACK 0xc0000000
#endif

#ifdef ARCH_CLIPPER
#define N_UAREA_GREGS		14	/* Clipper has r0..r13 as general regs */

#define UR_FP		(N_UAREA_GREGS)		/* fp is r14 */
#define UR_SP		(N_UAREA_GREGS + 1)	/* sp is r15 */
#define UR_PC		(N_UAREA_GREGS + 2)
#define N_UREGS		(N_UAREA_GREGS + 3)
#endif

#ifdef ARCH_MIPS
#define N_UAREA_GREGS		NGP_REGS + NFP_REGS	/* from ptrace.h */

#define UR_SP		MIPS_SP_REGNO		/* from mips_frame.h */
#define UR_FP		N_UAREA_GREGS		/* we fake this on the fly */
#define UR_PC		(N_UAREA_GREGS + 1)
#define N_UREGS		(N_UAREA_GREGS + 2)
#endif

#ifdef ARCH_VAX
#define N_UAREA_GREGS		12	/* VAX has r0..r11 as general registers */

#define UR_AP	N_UAREA_GREGS
#define UR_FP	(N_UAREA_GREGS + 1)
#define UR_SP	(N_UAREA_GREGS + 2)
#define UR_PC	(N_UAREA_GREGS + 3)
#define UR_PSL	(N_UAREA_GREGS + 4)
#define N_UREGS	(N_UAREA_GREGS + 5)
#endif /* ARCH_VAX */

/*  Some architectures (currently VAX and MIPS under Ultrix) have
 *  the registers accessible via ptrace READU calls in the u area.
 *
 *  We try to abstract out some common code for these, and define
 *  UAREA_REGS for #ifdefs.
 *
 *  In recent versions of some systems the registers are accessible
 *  through special 'ptrace()' calls, but we still need ureg_t.
 */
#if defined(OS_BSD44) || defined(ARCH_VAX) || defined(ARCH_MIPS) || \
    defined(ARCH_CLIPPER) || defined(OS_LINUX)
#define UAREA_REGS
#endif

#ifdef UAREA_REGS
typedef struct uregst {
	short ur_uaddr;
	short ur_is_current;
	taddr_t ur_value;
} ureg_t;
#endif /* UAREA_REGS */


/*  Linux exposes the x86 debug registers via the user area.
 */
#ifdef ARCH_LINUX386
#define UAREA_DREGS
#define N_UDREGS 8
#endif


#if AO_HAS_PTRACE_REGS
#undef UAREA_REGS
#if HAVE_MACHINE_REG_H
#include <machine/reg.h>
#else
#include <sys/user.h>
#endif

#ifndef OS_SUNOS
typedef struct ptraceregst {
#if HAVE_MACHINE_REG_H
	struct reg regs;
	struct fpreg fpregs;
#else
	struct user_regs_struct regs;
	struct user_fpregs_struct fpregs;
#endif
	bool need_fpregs;
#if AO_HAS_PTRACE_DREGS
	struct dbreg dbregs;
#endif
} ptrace_regs_t;
#endif
#endif /*AO_HAS_PTRACE_REGS*/ 


#ifdef OS_SUNOS_4
typedef struct sunregsst {
	struct regs sr_regs;
#ifdef ARCH_SUN4
	struct rwindow sr_rwindow;	/* MUST be just after sr_regs - see gsr */
#endif
#if defined(ARCH_SUN3) || defined(ARCH_SUN4) 
	struct fpu sr_fpu;
	bool sr_need_fpu;
#endif
#ifdef ARCH_SUN3
	enum { FPT_SOFT, FPT_68881, FPT_FPA } sr_fptype;
#endif
} sunregs_t;

taddr_t get_sun_regval PROTO((sunregs_t *sr, int pid, int reg));
#endif /*OS_SUN_4*/

