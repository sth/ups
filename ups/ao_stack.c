/* ao_stack.c - build a stack trace by examining the target */

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


/* @(#)ao_stack.c	1.3 24 May 1995 (UKC) */
char ups_ao_stack_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#ifdef AO_TARGET

#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ARCH_MIPS
#	ifdef ARCH_CLIPPER
#		include <machine/machparam.h>
#		include <machine/vmparam.h>	/* for USRSTACK */
#	else
#	ifdef OS_SUNOS_5
#		include <sys/reg.h>
#	ifdef OS_SUNOS_V9
#               include <sys/privregs.h>
#	endif
#		include <sys/frame.h>
#		include <sys/ucontext.h>
#	else 
#	ifndef OS_LINUX
#		include <machine/frame.h>
#	endif
#	endif
#	endif /* !ARCH_CLIPPER */
#endif /* !ARCH_MIPS */

#if HAVE_SYS_REG_H
#include <sys/reg.h>
#endif
#if HAVE_MACHINE_REG_H
#include <machine/reg.h>
#endif

#ifdef ARCH_BSDI386
#include <machine/vmparam.h>	/* for USRSTACK */
#include <sys/param.h>
#include <sys/user.h>
#endif

#include <local/ukcprog.h>
#include <mtrprog/alloc.h>
#include <mtrprog/hash.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "data.h"
#include "st.h"
#include "mreg.h"
#include "ao_regs.h"
#include "ao_syms.h"
#include "ao_target.h"
#include "ao_ptrace.h"
#include "ao_stack.h"
#include "ao_text.h"

#ifdef ARCH_MIPS
#include "mips_frame.h"
#endif /* ARCH_MIPS */

#ifndef ARCH_MIPS
static Stack *discard_junk_frames PROTO((target_t *xp, Stack *stk));
#endif

/*  FRAGILE CODE
 *
 *  Build a stack trace from a core file or process.
 *
 *  If ups breaks on when you get a new release of an OS, this is the
 *  place to look.  This function has lots of machine and compiler
 *  dependent assumptions about stack layout (especially when signals
 *  are involved).
 *
 *  Note that the stk_outer links are not set by this function.
 */

#if (defined ARCH_SUN4)

/* ARCH_SUN4 */
Stack *
ao_get_stack_trace(xp)
target_t *xp;
{
	static int non_frame_functions = -1;
#ifdef OS_SUNOS_5
	static const char sighandler[] = "sigacthandler";
#else
	static const char sighandler[] = "_sigtramp";
#endif
	taddr_t pc, savpc, fp;
	Stack *stk, *last;
	func_t *f;
	struct frame fbuf;

	pc = xp_getreg(xp, UPSREG_PC);
	fp = xp_getreg(xp, UPSREG_FP);
	savpc = xp_getreg(xp, 31);

	if (non_frame_functions == -1)
	  non_frame_functions = user_wants_non_frame_functions();

	for (stk = last = NULL; ; last = stk) {
		fil_t *fil;
		int lnum;
	
		if ((f = addr_to_func(pc)) == NULL)
			f = make_badfunc();

		if (strcmp(f->fu_demangled_name, sighandler) == 0 && last != NULL) {
			ao_preamble_t *pr;
			int signo;
#ifdef OS_SUNOS_5
			struct ucontext ucbuf;
			struct regs *regs;

			if (dread(xp, fp + sizeof(struct frame),
				  (char *)&ucbuf, sizeof(ucbuf)) != 0)
				break;
			regs = (struct regs *)&ucbuf.uc_mcontext.gregs;
			pc = regs->r_pc;
			
			signo = fbuf.fr_arg[0];
#else
			struct { int signo, code; taddr_t scp; } sigargs;
			struct sigcontext scbuf;

			/*  From looking at stack dumps ...
			 */
			if (dread(xp, fp + 0x40, (char *)&sigargs,
							sizeof(sigargs)) != 0)
				break;
			signo = sigargs.signo;
			
			if (dread(xp, sigargs.scp, (char *)&scbuf,
							sizeof(scbuf)) != 0)
				break;
			pc = (taddr_t)scbuf.sc_pc;
#endif /* OS_SUNOS_5 */
			
			last->stk_siginfo = make_siginfo(signo, 0);
			last->stk_siginfo->si_fp = fp;

			if (dread(xp, fp, (char *)&fbuf, sizeof(fbuf)) != 0)
				break;

			fp = (taddr_t)fbuf.fr_savfp;
			
			if ((f = addr_to_func(pc)) == NULL)
				f = make_badfunc();

			if (!get_preamble(f, &pr))
				break;
			
		 /* Quantify and Purify insert non frame pointer functions */
		 /* into the code, so skip the HAS_FP check - RGA */

			if (non_frame_functions || (f->fu_flags & FU_NO_FP) == 0)
				savpc = fbuf.fr_savpc;
		}

		/*  The Sun 4 is unusual in that saved pc values point
		 *  to the address of the call instruction, rather than
		 *  the one after the call.  Thus we don't have to offset
		 *  the pc to get the right line number.
		 */
		addr_to_fil_and_lnum(f, pc, &fil, &lnum, FALSE);

		stk = make_stk(f, pc, fil, lnum, last);
		stk->stk_fp = stk->stk_ap = fp;
		stk->stk_sp = fp;

		/*  If we are not the innermost function, adjust stk_pc so
		 *  that it points to the location where the called function
		 *  will jump to on return.
		 */
		if (last != NULL && last->stk_siginfo == NULL)
			stk->stk_pc += 8;

		if (fp == TADDR_NULL)
			break;

		/*  Get the next stack frame
		 *
		 *  The pc > f->fu_addr test is to cover the case where
		 *  a signal has interrupted a function before the frame
		 *  setup preamble has executed.
		 */

#ifndef OS_SUNOS_4
		/* RGA - causes incomplete stacks on SunOS 4 */
 		if (!get_preamble(f, (ao_preamble_t **)NULL))
			break;
#endif /* OS_SUNOS_4 */
		

		 /* RGA: Quantify and Purify insert non frame pointer */
		 /* functions into the code, so skip the HAS_FP check if */
		 /* non_frame_functions requested. Doesn't work for */
		 /* all targets, so this is an option. */

		if ((non_frame_functions || (f->fu_flags & FU_NO_FP) == 0) &&
		    pc > f->fu_addr) {
			if (dread(xp, fp, (char *)&fbuf, sizeof(fbuf)) != 0)
				break;
			
			fp = (taddr_t)fbuf.fr_savfp;
			pc = savpc;
			savpc = fbuf.fr_savpc;
		}
		else {
			if (last == NULL) {
				pc = xp_getreg(xp, 15);
			}
			else {
				/*  We can't cope with non frame pointer
				 *  function that calls another function.
				 */
				if (last->stk_siginfo == NULL)
					break;
				pc = savpc;
				savpc = fbuf.fr_savpc;
			}
		}
	}

	if (fp != TADDR_NULL && stk != NULL)
		stk->stk_bad = TRUE;

	return discard_junk_frames(xp, stk);
}
/* ARCH_SUN4 */

#elif (defined ARCH_MIPS)

/* ARCH_MIPS */
Stack *
ao_get_stack_trace(xp)
target_t *xp;
{
	int lnum;
	bool stack_ok;
	Stack *stk, *last;
	taddr_t pc, sp;

	pc = xp_getreg(xp, UPSREG_PC);
	sp = xp_getreg(xp, UPSREG_SP);

	stack_ok = FALSE;

	for (stk = last = NULL; sp != 0; last = stk) {	/* BUG: need proper test */
		func_t *f;
		frame_t *fr;
		taddr_t fp, saved_pc;

		if ((f = addr_to_func(pc)) == NULL)
			break;

		fr = get_frame_info(f);

		/*  We take a frame with pcreg == 0 as the end of the stack.
		 *  This is from observation, not from any documentation.
		 */
		if (fr->fr_pcreg == 0) {
			stack_ok = TRUE;
			break;
		}

		if ((fr->fr_reg_mask & (1 << MIPS_SAVEDPC_REGNO)) == 0 &&
						strcmp(f->fu_demangled_name, "sigvec") == 0) {
			struct sigcontext *sc;
			int signo;

			/*  We set sc just so the address expressions below
			 *  work.  We never dereference it.  This isn't very
			 *  portable, but then neither is this function.
			 */
			sc = (struct sigcontext *)(sp + 9 * sizeof(taddr_t));

			if (xp_read_data(xp, (taddr_t)&sc->sc_pc,	
						(char *)&pc, sizeof(pc)) != 0)
				break;
			
			if ((f = addr_to_func(pc)) == NULL)
				break;
			fr = get_frame_info(f);

			if (xp_read_data(xp, (taddr_t)&sc->sc_regs[4],
						(char *)&signo, sizeof(signo)) != 0)
				break;
			if (signo == 0)
				signo = -1;
			if (last != NULL)
				last->stk_siginfo = make_siginfo(signo, 0);

			if (xp_read_data(xp,
					   (taddr_t)&sc->sc_regs[MIPS_SP_REGNO],
					   (char *)&sp, sizeof(sp)) != 0)
				break;

			if (xp_read_data(xp, (taddr_t)&sc->sc_regs[fr->fr_pcreg],
					   (char *)&saved_pc, sizeof(int)) != 0)
				break;
		}
		else
			saved_pc = 0;

		if (fr->fr_spreg != MIPS_SP_REGNO)
			panic("can't cope with spreg != SP_REG in bst");
		fp = sp + fr->fr_frame_size;

		lnum = addr_to_lnum(f, (last == NULL) ? pc : pc - 1);

		stk = make_stk(f, pc, lnum, last);
		stk->stk_fp = stk->stk_ap = fp;
		stk->stk_sp = sp;

		/*  We only drop the sp over the function's stack frame
		 *  if the function has had a chance to set up the frame.
		 */
		if (pc > f->fu_addr)
			sp += fr->fr_frame_size;

		if (pc > f->fu_addr &&
				    fr->fr_reg_mask & (1 << MIPS_SAVEDPC_REGNO)) {
			if (fr->fr_pcreg != MIPS_SAVEDPC_REGNO)
				panic("savpc botch in bst");
			if (dread(xp, fp + fr->fr_reg_offset,
							(char *)&pc, 4) != 0)
				break;
		}
		else {
			/*  The pc was not saved ... therefore this must
			 *  be a leaf function, which means that it must
			 *  be the innermost function on the stack (modulo
			 *  signals).
			 */
			if (saved_pc != 0)
				pc = saved_pc;
			else if (last == NULL)
				pc = xp_getreg(xp, fr->fr_pcreg);
			else
				panic("leaf func botch in bst");
		}
	}

	if (!stack_ok && stk != NULL)
		stk->stk_bad = TRUE;

	return stk;
}
/* ARCH_MIPS */

#elif (defined ARCH_386)

#if defined(tEBX) && !defined(EBX)
#define EBX tEBX
#define EBP tEBP
#endif

#if defined(RBX) && !defined(EBX)
#define EBX RBX
#define EBP RBP
#endif

#define EBXMASK (1u << EBX)
#define EBPMASK (1u << EBP)

#if defined(ARCH_LINUX386)

static bool ebx_valid  PROTO((target_t *xp, taddr_t ebx));

static bool
ebx_valid(xp, ebx)
target_t *xp;
taddr_t ebx;
{
	taddr_t fa;
	func_t *f;

	if (dread_addrval(xp, ebx + 8, &fa) != 0) return FALSE;

	f = addr_to_func(fa);

	if (f && strcmp(f->fu_name, "_dl_runtime_resolve") == 0) return TRUE;

	return FALSE;
}

#endif

/* ARCH_386 */
Stack *
ao_get_stack_trace(xp)
target_t *xp;
{
	int wordsize = xp_get_addrsize(xp) / 8;
	taddr_t pc, fp, sp;
	register Stack *stk;
	func_t *f;
	Stack *last;
	
	pc = xp_getreg(xp, UPSREG_PC);
	fp = xp_getreg(xp, UPSREG_FP);
	sp = xp_getreg(xp, UPSREG_SP);

	if (fp == (taddr_t)0) {
		fp = (taddr_t)-1;
	}
        
#ifdef DEBUG_STACK
	if (Debug_flags & DBFLAG_STACK)
		fprintf(stderr, "bst: pc=0x%x fp=0x%x sp=0x%x\n", pc, fp, sp);
#endif

	for (stk = last = NULL; fp != (taddr_t)0; last = stk) {
		taddr_t prevpc = (last == NULL) ? pc : (pc - 1);
		fil_t *fil;
		int lnum;

		if ((f = addr_to_func(prevpc)) == NULL) {
			f = make_badfunc();
		}
#if defined(ARCH_BSDI386)
		if (*f->fu_demangled_name == '[' && strcmp(f->fu_demangled_name, "[start]") == 0) {
			fp = NULL;
			break;
		}
#endif
#if defined(ARCH_LINUX386) || defined(ARCH_SOLARIS386)
		if ((strcmp(f->fu_demangled_name, "_entry") == 0) ||
		    (strcmp(f->fu_demangled_name, "_start") == 0) ||
		    (strcmp(f->fu_demangled_name, "__libc_start_main") == 0)) {
			fp = (taddr_t)0;
			break;
		}
#endif

		addr_to_fil_and_lnum(f, prevpc, &fil, &lnum, FALSE);

#ifdef DEBUG_STACK
		if (Debug_flags & DBFLAG_STACK)
			fprintf(stderr, "bst: pc=0x%x fname=%s fp=%x\n",
				pc, f->fu_demangled_name, fp);
#endif

		stk = make_stk(f, pc, fil, lnum, last);

                stk->stk_fp = stk->stk_ap = fp;
		stk->stk_sp = sp;

		if (!get_preamble(f, (ao_preamble_t **)NULL))
			break;

		/*  Get the next stack frame
		 */

		if (f->fu_symtab && st_unwind(xp, f->fu_symtab, &fp, &sp, &prevpc, &stk->stk_cfa)) {
			pc = prevpc;
		}
		else if ((f->fu_flags & FU_NO_FP) != 0 || pc < f->fu_addr) {
			ao_preamble_t *pr;
			int offset;
			int offsetlim;
			unsigned mask;
			int i;
			int ebxoffset = 0;
			int ebpoffset = 0;
			taddr_t possible_pc = 0;
			int possible_offset;

			if (!get_preamble(f, &pr))
				panic("can't get function preamble");

			offset = -(pr->pr_rsave_offset);

			for (mask = pr->pr_rsave_mask; mask; mask >>= 1)
				if (mask & 1u)
					offset += wordsize;

			offsetlim = offset + 1000;

			if (pr->pr_rsave_mask & EBXMASK) {
				ebxoffset = pr->pr_rsave_offset;
				for (i = 0; i < N_PUSHABLE_REGS; i++) {
					ebxoffset -= wordsize;
					if (pr->pr_regtab[i] == EBX)
						break;
				}
			}

			if (pr->pr_rsave_mask & EBPMASK) {
				ebpoffset = pr->pr_rsave_offset;
				for (i = 0; i < N_PUSHABLE_REGS; i++) {
					ebpoffset -= wordsize;
					if (pr->pr_regtab[i] == EBP)
						break;
				}
			}

			for (; offset < offsetlim; offset += 4) {
				func_t *cfunc;

				if (dread_addrval(xp, sp + offset, &pc) == 0 &&
				    (cfunc = addr_to_func(pc)) != NULL) {
					unsigned char s_buf[7];
					taddr_t ebx = 0;

					if (dread(xp, pc - 7, (char *)s_buf, sizeof(s_buf)) != 0)
						continue;

					if (ebxoffset)
						dread_addrval(xp, sp + offset + ebxoffset, &ebx);
					else
						ebx = xp_getreg(xp, 3);

#if defined(ARCH_LINUX386)
                                        if (wordsize == 4 && !ebx_valid(xp, ebx)) {
						int i;

						for (i = offset; i > offset - 20; i -= wordsize)
							if (dread_addrval(xp, sp + i, &ebx) == 0 && ebx_valid(xp, ebx))
								break;
                                        }
#endif
                                        
					if (ebpoffset) {
						if (dread_addrval(xp, sp + offset + ebpoffset, &fp) != 0 || !fp)
							fp = (taddr_t)-1;
					}

					if (s_buf[2] == 0xe8) {
						taddr_t cpc = pc + *(int *)(s_buf + 3);

						if (cpc == f->fu_addr)
							break;

						dread(xp, cpc, (char *)s_buf, 6);

						if (s_buf[0] == 0xff && s_buf[1] == 0x25) {
							if (wordsize == 4) {
								dread_addrval(xp, *(unsigned int *)(s_buf + 2), &cpc);
							}
							else {
								dread_addrval(xp, *(int *)(s_buf + 2) + cpc + 6, &cpc);
							}
						}
						else if (s_buf[0] == 0xff && s_buf[1] == 0xa3) {
							dread_addrval(xp, *(int *)(s_buf + 2) + ebx, &cpc);
						}

						if (cpc == f->fu_addr)
							break;
					}
					else if (s_buf[5] == 0xff &&
						 ((s_buf[6] & 0xf0) == 0xd0 ||
						  ((s_buf[6] & 0xf0) == 0x10 &&
						   (s_buf[6] & 0x07) != 0x04))) {
						/* call REG or call *REG */
						if (possible_pc == 0) {
							possible_pc = pc;
							possible_offset = offset;
						}
					}
					else if (s_buf[4] == 0xff &&
						 ((s_buf[5] & 0xf7) == 0x14 ||
						  ((s_buf[5] & 0xf0) == 0x50 &&
						   (s_buf[5] & 0x07) != 0x04))) {
						/* call *(REG*x) or call *NN(REG) */
						if (possible_pc == 0) {
							possible_pc = pc;
							possible_offset = offset;
						}
					}
					else if (s_buf[3] == 0xff &&
						 (s_buf[4] & 0xf7) == 0x54) {
						/* call *NN(REG*x) */
						if (possible_pc == 0) {
							possible_pc = pc;
							possible_offset = offset;
						}
					}
					else if (s_buf[1] == 0xff &&
						 ((s_buf[2] & 0xf7) == 0x15 ||
						  ((s_buf[2] & 0xf0) == 0x90 &&
						   (s_buf[2] & 0x07) != 0x04))) {
						/* call *NNNNNNNN or call *NNNNNNNN(REG) */
						if (possible_pc == 0) {
							possible_pc = pc;
							possible_offset = offset;
						}
					}
					else if (s_buf[0] == 0xff &&
						 (s_buf[1] & 0xf7) == 0x94) {
						/* call *NNNNNNNN(REG*x) */
						if (possible_pc == 0) {
							possible_pc = pc;
							possible_offset = offset;
						}
					}
				}
			}

			if (offset == offsetlim) {
				if (possible_pc) {
					pc = possible_pc;
					offset = possible_offset;
				}
				else {
					break;
				}
			}

			stk->stk_fp = stk->stk_ap = sp + offset;
			stk->stk_sp = sp;

			sp = sp + offset + 4;
		} else {
			if (dread_addrval(xp, stk->stk_fp, &fp) != 0)
				break;
			if (dread_addrval(xp, stk->stk_fp + wordsize, &pc) != 0)
				break;

			sp = stk->stk_fp + 2 * wordsize;

#ifdef DEBUG_STACK
			if (Debug_flags & DBFLAG_STACK) {
				fprintf(stderr,
					"bst: frame fp=0x%x: fp=0x%x pc=0x%x\n",
					fp, fbuf.fr_savfp, fbuf.fr_savpc);
			}
#endif
		}

#if defined(ARCH_FREEBSD386)
#elif defined(ARCH_BSDI386)
		/*  If the pc points at the signal trampoline code in the u
		 *  area, we assume that the target has taken a signal.
		 */
#define u ((struct user *)USRSTACK)
		if (pc >= (taddr_t)u->u_pcb.pcb_sigc &&
		    pc < (taddr_t)u->u_pcb.pcb_sigc + sizeof u->u_pcb.pcb_sigc){
#undef u
			struct { int signo, code; taddr_t scp; } sigargs;
			taddr_t scp;
			struct sigcontext scbuf;

			if (xp_read_data(xp, sp,
				     (char *)&sigargs, sizeof(sigargs)) != 0)
				break;
			if (xp_read_data(xp, sigargs.scp,
					    (char *)&scbuf, sizeof(scbuf)) != 0)
				break;

			stk->stk_siginfo = make_siginfo(sigargs.signo, 0);

			sp = scbuf.sc_sp;
			pc = scbuf.sc_pc;
			fp = scbuf.sc_fp;
		}
#endif
	}
	if ((fp != (taddr_t)0) && (stk != NULL))
		stk->stk_bad = TRUE;

	return discard_junk_frames(xp, stk);
}
/* ARCH_386 */

#else

/* NOT SUN4/MIPS/386 */
Stack *
ao_get_stack_trace(xp)
target_t *xp;
{
#ifdef OS_SUNOS
	struct sigframest {
		taddr_t sf_savfp;
		taddr_t sf_savpc;
		int sf_signo;
		int sf_code;
		taddr_t sf_scp;
	} sigframe;
#endif /* OS_SUNOS */
#ifdef ARCH_CLIPPER
	struct frame {
		taddr_t fr_savfp;
		taddr_t fr_savpc;
	};
	preamble_t *pr;
	bool must_find_fp;
#endif
	bool normal_frame;
	taddr_t pc, fp, sp;
#ifdef ARCH_VAX
	taddr_t ap;
#endif
	register Stack *stk;
	func_t *f;
	Stack *last;
	struct frame fbuf;
	
	pc = xp_getreg(xp, UPSREG_PC);
	fp = xp_getreg(xp, UPSREG_FP);
#ifdef ARCH_VAX
	ap = xp_getreg(xp, UPSREG_AP);
#endif
	sp = xp_getreg(xp, UPSREG_SP);

#ifdef DEBUG_STACK
	if (Debug_flags & DBFLAG_STACK)
		fprintf(stderr, "bst: pc=0x%x fp=0x%x sp=0x%x\n", pc, fp, sp);
#endif

#ifdef ARCH_CLIPPER
	must_find_fp = FALSE;
#endif
	for (stk = last = NULL; fp != (taddr_t)0; last = stk) {
		fil_t *fil;
		int lnum;
		normal_frame = TRUE;

		if ((f = addr_to_func(pc)) == NULL) {
#ifdef ARCH_CLIPPER
			if (last != NULL && last->stk_func == main_func) {
				fp = NULL;
				break;
			}
#endif /* ARCH_CLIPPER */
			f = make_badfunc();
		}
#if defined(ARCH_VAX) || defined(ARCH_CLIPPER) || defined(ARCH_BSDI386)
		if (*f->fu_demangled_name == '[' && strcmp(f->fu_demangled_name, "[start]") == 0) {
			fp = NULL;
			break;
		}
#endif
		addr_to_fil_and_lnum(f, (last == NULL) ? pc : (pc - 1),
				     &fil, &lnum, FALSE);

#ifdef OS_SUNOS
		if (strcmp(f->fu_demangled_name, "_sigtramp") == 0 && last != NULL) {
			int signo;
			taddr_t scp;
			struct sigcontext scbuf;

			if (dread(xp, last->stk_fp, (char *)&sigframe,
							sizeof(sigframe)) != 0)
				break;
			signo = sigframe.sf_signo;
			scp = sigframe.sf_scp;

			last->stk_siginfo = make_siginfo(signo, 0);

			if (dread(xp, scp, (char *)&scbuf, sizeof(scbuf)) != 0)
				break;
			pc = (taddr_t)scbuf.sc_pc;
			if ((f = addr_to_func(pc)) == NULL)
				f = make_badfunc();
			lnum = addr_to_lnum(f, pc);
		}
#endif /* OS_SUNOS */

#ifdef DEBUG_STACK
		if (Debug_flags & DBFLAG_STACK)
			fprintf(stderr, "bst: pc=0x%x fname=%s fp=%x\n",
				pc, f->fu_demangled_name, fp);
#endif

		stk = make_stk(f, pc, fil, lnum, last);

		/*  Get the next stack frame
		 */
#if defined(ARCH_SUN3)
		/*  Special case code for functions with no frame pointer
		 *  setup code on Suns.
		 *
		 *  The pc > f->fu_addr test is to cover the case where
		 *  a signal has interrupted a function before the frame
		 *  setup preamble has executed.
		 */
		if (!get_preamble(f, (ao_preamble_t **)NULL))
			break;
		
		if (!((f->fu_flags & FU_NO_FP) == 0 && pc > f->fu_addr) &&
				     (last == NULL || last->stk_siginfo != NULL)) {
			taddr_t offset;
			int res;
			struct sigcontext scbuf;

			if (last != NULL && last->stk_siginfo != NULL)
				sp = (taddr_t)scbuf.sc_sp;

			/*  A function with no stack setup code may still
			 *  push things on the stack, so the sp may point
			 *  a few words above the return address.
			 *
			 *  Without interpreting the code of the function,
			 *  we don't know how may words have been pushed,
			 *  so we punt and look down the stack until we
			 *  find an address that points inside a function.
			 *
			 *  BUG: this code will break if, say, a register
			 *  containing a text address has been pushed.
			 *  We should either own up and put "(probably)"
			 *  after the function name in the stack trace if
			 *  we had to use an offset, or verify that the
			 *  address we found points just after a function call.
			 */
			for (offset = 0; offset < 6 * sizeof(taddr_t); offset += sizeof(taddr_t)) {
				res = dread(xp, sp + offset, (char *)&pc,
								 sizeof(pc));
				if (res != 0 || addr_to_func(pc) != NULL)
					break;
			}
			stk->stk_fp = stk->stk_ap = sp + offset; /* for saved regs */
			stk->stk_sp = sp;
			if (res != 0)
				break;
			normal_frame = FALSE;
		}
#endif /* ARCH_SUN3 */

#ifdef ARCH_CLIPPER
		if (!get_preamble(f, (ao_preamble_t **)NULL))
			break;
		
		if ((f->fu_flags & FU_NO_FP) != 0) {
			unsigned short text[2];

			stk->stk_sp = sp;
			sp += pr->pr_frame_size;
			stk->stk_fp = stk->stk_ap = sp;
			if (dread(xp, sp, (char *)&pc, sizeof(pc)) != 0)
				break;
			sp += 4;

			/*  If the function pushed arguments on the
			 *  stack the pc should be pointing at the
			 *  code to drop the sp back over the pushed
			 *  arguments.  Check for this, and adjust
			 *  the sp accordingly if we find the instruction.
			 *
			 *  We're looking for addq $n, sp or addi $n, sp.
			 */
			if (xp_read_text(xp, pc,
					      (char *)text, sizeof(text)) != 0)
				break;
			if ((*text & 0xff0f) == 0x820f)
				sp += (*text >> 4) & 0xf;
			else if (*text == 0x83bf)
				sp += (short)text[1];
			
			normal_frame = FALSE;
			must_find_fp = TRUE;
		}
#endif

		if (normal_frame) {
#ifdef ARCH_CLIPPER
			if (must_find_fp) {
				taddr_t fpaddr;

				fpaddr = ao_get_reg_addr(xp, last->stk_outer, 14); /* fp */
				if (fpaddr != 0 && xp_read_data(xp,
						   fpaddr, (char *)&fp, 4) != 0)
					break;
			}
			must_find_fp = FALSE;
#endif
#ifdef ARCH_VAX
			stk->stk_ap = ap;
			stk->stk_fp = fp;
#else
			stk->stk_fp = stk->stk_ap = fp;
#endif
			stk->stk_sp = sp;
			if (dread(xp, fp, (char *)&fbuf, sizeof(fbuf)) != 0)
				break;
			sp = fp + sizeof(fbuf);

#ifdef DEBUG_STACK
			if (Debug_flags & DBFLAG_STACK) {
				fprintf(stderr,
					"bst: frame fp=0x%x: fp=0x%x pc=0x%x\n",
					fp, fbuf.fr_savfp, fbuf.fr_savpc);
			}
#endif

			fp = (taddr_t)fbuf.fr_savfp;
#ifdef ARCH_VAX
			ap = (taddr_t)fbuf.fr_savap;
#endif
			pc = fbuf.fr_savpc;
		}

#ifdef ARCH_CLIPPER
		/*  If the pc points into the u area, we assume that
		 *  the target has taken a signal, and that the pc is
		 *  in the signal trampoline code.
		 */
		if (pc >= (taddr_t)USRSTACK &&
				      pc < (taddr_t)USRSTACK + UPAGES * NBPG) {
			taddr_t scp;
			Siginfo *si;
			int signo;
			struct sigcontext scbuf;

			if (xp_read_data(xp, sp,
					     (char *)&scp, sizeof(scp)) != 0)
				break;
			if (xp_read_data(xp, scp,
					    (char *)&scbuf, sizeof(scbuf)) != 0)
				break;
			if (xp_read_data(xp, scp - 20,
					    (char *)&signo, sizeof(signo)) != 0)
				break;

			si = make_siginfo(signo, sizeof(preamble_t));
			si->si_fp = scp - 80; /* From looking at stack dumps */
			si->si_pr->pr_rsave_mask = (1 << 14) - 1; /* savew0 */
			si->si_pr->pr_rsave_offset = 0;
			stk->stk_siginfo = si;

			sp = scbuf.sc_sp;
			pc = scbuf.sc_pc;
		}
#endif
#ifdef ARCH_VAX
		/*  Check for signal generated stack frames on the VAX.
		 *
		 *  Assumption: if this is a callg frame rather than a calls
		 *  one, then this frame was generated by a signal.
		 *  Furthur assumptions come from poking around in VAX
		 *  stack dumps.
		 *
		 *  As the man says, you are not expected to understand this.
		 */
		if (!fbuf.fr_s) {
			taddr_t scp;
			int signo;
			struct sigcontext scbuf;

			/*  The hairiest bit of all: get the pc from
			 *  the arguments of the signal catching routine.
			 *  What we are getting is roughly
			 *
			 *	((struct sigcontext *)ap[3])->sc_pc
			 *
			 *  The 3 comes from the fact that scp is documented
			 *  in sigvec(2) as the third argument to a signal
			 *  catcher.  This would make you expect ap[2], but
			 *  VAX args are always shifted by one.
			 */
			if (dread(xp, ap + 3 * sizeof(taddr_t), (char *)&scp,
								sizeof(scp)) != 0)
				break;
			if (dread(xp, scp, (char *)&scbuf, sizeof(scbuf)) != 0)
				break;
			pc = scbuf.sc_pc;

			/*  The signal number is the first argument of the signal
			 *  catcher.
			 */
			if (dread(xp, ap + sizeof(taddr_t), (char *)&signo,
								sizeof(signo)) != 0)
				break;

			/*  Now skip a frame.
			 */
			if (dread(xp, fp, (char *)&fbuf, sizeof(fbuf)) != 0)
				break;
			fp = (taddr_t)fbuf.fr_savfp;
			ap = (taddr_t)fbuf.fr_savap;
			stk->stk_siginfo = make_siginfo(signo, 0);
		}
#endif /* ARCH_VAX */
	}
	if ((fp != (taddr_t)0) && (stk != NULL))
		stk->stk_bad = TRUE;

	return discard_junk_frames(xp, stk);
}
/* NOT SUN4/MIPS/386 */

#endif

#ifndef ARCH_MIPS
/*  Discard any frames outside the main function.
 *
 *  Must do it this way round as the main function may be called
 *  recursively.
 */
static Stack *
discard_junk_frames(xp, stk)
target_t *xp;
Stack *stk;
{
	func_t *main_func;

	main_func = xp_get_mainfunc(xp);

	if (main_func != NULL) {
		Stack *outer, *next;

		outer = stk;
		while (stk != NULL && !stk->stk_bad && stk->stk_func != main_func)
			stk = stk->stk_inner;
		
		if (stk != NULL && stk->stk_func == main_func) {
			for (; outer != stk; outer = next) {
				next = outer->stk_inner;
				destroy_stk(outer);
			}
		}
		stk = outer;
	}

	return stk;
}
#endif

taddr_t
ao_get_reg_addr(xp, stk, reg)
target_t *xp;
Stack *stk;
int reg;
{
	/*  The first place registers live in this frame could be saved is
	 *  the next frame in.
	 */
	stk = stk->stk_inner;
	
#ifdef ARCH_SUN4
	if (stk == NULL)
		return 0;
	else if (reg < 32) {
		taddr_t fp;

		if (reg < 8)
			panic("reg_addr: can't do global regs");

		if (reg < 16) {
			stk = stk->stk_inner;
			reg += 16;
		}

		/*  Integer register.  Just return the address in the
		 *  register window (as it appears on the stack).
		 *
		 *  On the SPARC, a called function conceptually saves
		 *  the caller's local and in registers on the stack,
		 *  as the first 16 words of it's stack frame (the
		 *  kernel hides the register window gunk from us).
		 */
		if (stk == NULL) {
			fp = xp_getreg(xp, UPSREG_SP);
		}
		else if (stk->stk_siginfo != NULL &&
			 stk->stk_siginfo->si_fp != 0) {
			fp = stk->stk_siginfo->si_fp;
		}
		else {
			if (!get_preamble(stk->stk_func,(ao_preamble_t **)NULL))
				return 0;
			    
			if ((stk->stk_func->fu_flags & FU_NO_FP) == 0) {
				fp = stk->stk_fp;
			}
			else {
				return ao_get_reg_addr(xp, stk, reg);
			}
		}
		
		return fp + (reg - 16) * 4;
	}
	else if (reg < 64) {
#define STF	(((unsigned)3 << 30) | (044 << 19))
#define NINST	32		/* Max 32 fp regs (paranoia) */
#define SPREG	14		/* SPARC reg %o6 */
		int text[NINST];
		int i;

		/*  Floating point register.  Search back through the
		 *  instructions for the floating point save.
		 */

		stk = stk->stk_outer;

		reg -= 32;

		if (st_read_text(stk->stk_func->fu_symtab,
				 stk->stk_pc - sizeof(text),
				 (char *)text, sizeof(text)) != 0) {
			return 0;	/* BUG: not right */
		}
		
		for (i = NINST - 1; i > 0; --i) {
			unsigned inst, val, expected;

			inst = text[i];
			val = inst & ~(((unsigned)1 << 12) - 1);
			expected = STF | (reg << 25) | (SPREG << 14) | (1 << 13);
			if (val == expected) {
				return stk->stk_inner->stk_fp +
					(inst & ((1 << 12) - 1));
			}
		}
	}
	else
		panic("reg_addr: reg > 63");

	return 0;
#else
    {
	ao_preamble_t *pr;
	taddr_t regaddr;
	unsigned rmask;
	Siginfo *si;
#ifdef ARCH_386
	int i;
#else
	unsigned mask;
#endif

#ifdef ARCH_SUN3
	/*  Registers 18 to 25 are the 68881 floating point registers.
	 */
	if (reg >= 18 && reg < 26) {
		reg -= 18;
		rmask = 0x80 >> reg;
		si = NULL;
		while (stk != NULL) {
			taddr_t fp;

			if (si == NULL &&
			    stk->stk_siginfo != NULL &&
			    stk->stk_siginfo->si_fp != 0) {
				si = stk->stk_siginfo;
				fp = si->si_fp;
				pr = si->si_pr;
			}
			else {
				fp = stk->stk_fp;
				if (!get_preamble(stk->stk_func, &pr))
					return 0;
				stk = stk->stk_inner;
			}
			
			if (pr->pr_fpreg_rsave_mask & rmask) {
				regaddr = fp + pr->pr_fpreg_rsave_offset;
				for (mask = 0x80; mask != rmask; mask >>= 1)
					if (pr->pr_fpreg_rsave_mask & mask)
						regaddr += 12;
				return regaddr;
			}
		}
		return 0;
	}
#endif /* ARCH_SUN3 */
#ifdef ARCH_MIPS
	/*  Registers 32 to 63 are the MIPS floating point registers.
	 */
	if (reg >= 32 && reg < 64) {
		rmask = 1 << (reg - 32);
		si = NULL;
		while (stk != NULL) {
			taddr_t fp;

			if (si == NULL && stk->stk_siginfo != NULL &&
						stk->stk_siginfo->si_fp != 0) {
				si = stk->stk_siginfo;
				fp = si->si_fp;
				pr = si->si_pr;
			}
			else {
				fp = stk->stk_fp;
				if (!get_preamble(stk->stk_func, &pr))
					return 0;
				stk = stk->stk_inner;
			}
			
			if (pr->pr_fpreg_rsave_mask & rmask) {
				regaddr = fp + pr->pr_fpreg_rsave_offset;
				for (mask = 1 << 31; mask != rmask; mask >>= 1)
					if (pr->pr_fpreg_rsave_mask & mask)
						regaddr -= 4;
				return regaddr;
			}
		}
		return 0;
	}
#endif /* ARCH_MIPS */
#ifdef ARCH_CLIPPER
	/*  Registers 16 to 32 are the Clipper fp registers.
	 */
	if (reg >= 16 && reg < 32) {
		if (reg & 1)
			panic("odd reg in gra");
		reg = (reg - 16) / 2;
		rmask = 1 << reg;
		si = NULL;
		while (stk != NULL) {
			taddr_t fp;

			if (si == NULL && stk->stk_siginfo != NULL &&
						stk->stk_siginfo->si_fp != 0) {
				si = stk->stk_siginfo;
				fp = si->si_fp;
				pr = si->si_pr;
			}
			else {
				fp = stk->stk_fp;
				if (!get_preamble(stk->stk_func, &pr))
					return 0;
				stk = stk->stk_inner;
			}
			if (pr->pr_fpreg_rsave_mask & rmask) {
				regaddr = fp + pr->pr_fpreg_rsave_offset;
				for (mask = 0x1; mask != rmask; mask <<= 1)
					if (pr->pr_fpreg_rsave_mask & mask)
						regaddr += 8;
				return regaddr;
			}
		}
		return 0;
	}
#endif
#ifdef ARCH_386_64
	/* Registers 17 to 32 are the SSE XMM registers which are
	 * used for fp values on x86-64 architectures.
	 */
	if (reg >= 17 && reg <= 32) {
		return 0;
	}
#endif
#ifdef ARCH_386
	reg = x86_gcc_register(reg);
#endif

	if (reg < 0 || reg > 31)
		panic("regno botch in gra");

	rmask = 1 << reg;
	si = NULL;
	while (stk != NULL) {
		taddr_t fp;

		if (si == NULL && stk->stk_siginfo != NULL &&
						stk->stk_siginfo->si_fp != 0) {
			si = stk->stk_siginfo;
			fp = si->si_fp;
			pr = (ao_preamble_t *)si->si_predata;
		}
		else {
			fp = stk->stk_fp;
			if (!get_preamble(stk->stk_func, &pr))
				return 0;
			stk = stk->stk_inner;
		}

		if (pr->pr_rsave_mask & rmask) {
			regaddr = fp + pr->pr_rsave_offset;
#ifdef ARCH_386
			for (i = 0; i < N_PUSHABLE_REGS; ++i) {
				regaddr -= 4;
				if (pr->pr_regtab[i] == reg)
					return regaddr;
			}
			panic("reg botch in gra");
			return 0;	/* to satisfy gcc */
#else
#ifdef ARCH_MIPS
			for (mask = 1 << 31; mask != rmask; mask >>= 1)
				if (pr->pr_rsave_mask & mask)
					regaddr -= 4;
#else
			for (mask = 0x1; mask != rmask; mask <<= 1)
				if (pr->pr_rsave_mask & mask)
					regaddr += 4;
#endif
			return regaddr;
#endif
		}
	}

	return 0;
    }
#endif /* !ARCH_SUN4 */
}

const char *
ao_get_signal_tag(xp, signo)
target_t *xp;
int signo;
{
	return (signo == -1) ? "<unknown signal number>" : signame(signo);
}

#endif /* AO_TARGET */
