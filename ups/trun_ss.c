/* trun_ss.c - machine target execution control using single stepping */

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


/* @(#)trun_ss.c	1.22 24 May 1995 (UKC) */
char ups_trun_ss_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <signal.h>
#include <stdio.h>

#include <local/wn.h>
#include <local/ukcprog.h>
#include <local/obj/obj.h>
#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "st.h"
#include "breakpoint.h"
#include "watchpoint.h"
#include "trun.h"
#include "exec.h"
#include "obj_stack.h"
#include "obj_signal.h"
#include "obj_bpt.h"
#include "obj_wpt.h"
#include "dx.h"

/*  Target execution control.  Sit in a loop continuing the target process
 *  xp until we should stop, at which point we refresh the display.
 *
 *  rtype is the way in which we should run the target - RT_CONT, RT_STEP, or
 *  RT_NEXT.
 *
 *  If stop_bp is zero, then we should stop at any breakpoint we hit.  Otherwise
 *  all breakpoints other than stop_bp are ignored.
 *
 *  Warning: this is a hairy namei() type loop, full of breaks and continues.
 */
stopres_t
dx_run_target_ss(xp, rtype)
target_t *xp;
rtype_t rtype;
{
	stopres_t stopres;
	cont_type_t cont_type;
	breakpoint_t *bp, *tmp_bp;
	lno_t *lno;
	bool stepped_into_func, stop, bp_wants_stop, at_right_stack_level;
	bool plt_stop, check_wps;
	int sig, orig_lnum;
	taddr_t orig_fp, jsr_fp, last_sp, jsr_sp, sp;
	taddr_t last_pc, pc, retaddr;
	func_t *orig_f, *last_f, *f;
	breakpoint_t *stop_bp;
	int emulated_wps, refresh_interval;
	time_t next_refresh;

	cont_type = (rtype == RT_CONT) ? CT_CONT : CT_STEP;
	tmp_bp = 0;
	jsr_sp = 0;
	jsr_fp = 0;
	stop_bp = NULL;
	plt_stop = FALSE;
	check_wps = FALSE;
	emulated_wps = 0;
	refresh_interval = get_watchpoint_refresh_interval();
	next_refresh = time(NULL) + refresh_interval;

	if (target_process_exists(xp)) {
		taddr_t atol_pc;	/* pc to give to addr_to_lnum */

		sp = xp_getreg(xp, UPSREG_SP);
		if (rtype == RT_CONT) {
			pc = atol_pc = xp_getreg(xp, UPSREG_PC);
			orig_fp = xp_getreg(xp, UPSREG_FP);
			orig_f = f = addr_to_func(pc);
		}
		else {
			taddr_t fp;

			fp = xp_getreg(xp, UPSREG_FP);
			get_current_func(&f, &orig_fp, &pc, &atol_pc);
			orig_f = f;

			if ((f->fu_flags & FU_NO_FP) == 0 && fp != orig_fp) {
				stop_bp = dx_addr_to_breakpoint(xp, pc);
				if (stop_bp == NULL) {
					tmp_bp = dx_add_breakpoint(xp, pc);
					stop_bp = tmp_bp;
				}
				if (!breakpoint_is_installed(stop_bp)) {
					if (install_breakpoint(stop_bp, xp, TRUE) != 0)
/*	RGA					panic("can't install stop_bp");*/
						errf("can't install stop_bp");
				}
				jsr_fp = orig_fp;
				cont_type = CT_CONT;
			}
		}

		if (orig_f != NULL && addr_to_lno(orig_f, atol_pc, &lno, FALSE))
			orig_lnum = lno->ln_num;
		else
			orig_lnum = 0;

		if (rtype == RT_CONT &&
		    (emulated_wps = install_all_watchpoints(xp)) < 0)
			return SR_FAILED;
	}
	else {
		/*  No process.  Set sp so that the test "current_sp >= old_sp"
		 *  always succeeds.
		 */
		sp = 0;
		orig_fp = 0;

		/*  The following lines were added because gcc complained
		 *  about them being used unintialised.  I'm not sure if
		 *  these are the right values - this code has not been
		 *  used since these were added.
		 */
		orig_lnum = 0;
		orig_f = f = NULL;
		pc = 0;
	}
	
	bp = 0; /* to satisfy gcc */
	sig = 0; /* to satisfy gcc */

	for(;;) {
		/*  Start, continue or single step the target
		 */
		if (!target_process_exists(xp)) {
			if (rtype != RT_CONT)
				panic("bad rtype in dx_run_target");
			stopres = dx_start(xp);
			if (!target_process_exists(xp) || stopres == SR_WPT || stopres == SR_BPT)
				break;
			sig = 0;
			if (rtype == RT_CONT &&
			    (emulated_wps = install_all_watchpoints(xp)) < 0) {
				stopres = SR_FAILED;
				break;
			}
		}
		else {
			int res;

			if (cont_type == CT_CONT)
				res = install_all_breakpoints(xp);
			else
			{
			  res = uninstall_all_breakpoints(xp);
			  install_solib_event_breakpoint(xp);
			}
			
			if (res != 0) {
				stopres = SR_FAILED;
				break;
			}
			
			stopres = xp_restart_child(xp,
						      get_restart_sig(xp),
						      cont_type);
		}
		
		/*  Set stop to indicate whether we should stop now. 
		 *  If we aren't stopping, refresh the display if necessary.
		 */
		switch(stopres) {
		case SR_DIED:
		case SR_USER:
		case SR_FAILED:
			stop = TRUE;
			break;
		case SR_SSTEP:
			stop = FALSE;	/* but see later */

			/*  If we have arrived at the location of a breakpoint,
			 *  we execute the code associated with it, as a
			 *  next or step to a line with a breakpoint before
			 *  it conceptually executes the code of the breakpoint.
			 *
			 *  Note that we use addr_to_breakpoint here rather
			 *  the get_breakpoint_at_addr because we act on
			 *  a breakpoint even if it's not installed.
			 */
			bp = dx_addr_to_breakpoint(xp, xp_getreg(xp, UPSREG_PC));
			if (bp != NULL) {
				bp_wants_stop = execute_bp_code(bp,
							xp_getreg(xp, UPSREG_FP),
						     xp_getreg(xp, UPSREG_AP));
				stopres = xp_get_stopres(xp);
				if (breakpoint_is_solib_event(bp))
				  xp->xp_hit_solib_event = TRUE;
				else
				  xp->xp_hit_solib_event = FALSE;
				check_for_dynamic_libs_sstep(xp, &stopres);
				if (stopres == SR_DIED)
					stop = TRUE;
			}
			break;
		case SR_SIG:
			sig = xp_get_lastsig(xp);
			if (emulated_wps && sig == SIGSEGV) {
				if (deactivate_all_watchpoints(xp) < 0) {
					stopres = SR_FAILED;
					stop = TRUE;
				}
				else {
					xp_cancel_lastsig(xp);
					cont_type = CT_STEP;
					check_wps = TRUE;
					continue;
				}
			}
			else {
			if (sig_kills_target_by_default(sig) &&
					xp_get_sigstate(xp, sig) == SGH_DEFAULT)
				stop = TRUE;
			else
				stop = sig_stops_target(sig);
			if (!stop && sig_causes_refresh(sig))
				refresh_target_display(xp, stopres, FALSE);
			}
			break;
		case SR_BPT:
			bp = get_breakpoint_at_addr(xp, xp_getreg(xp, UPSREG_PC));
			if (bp == NULL)
				panic("bpt botch in rt");
			bp_wants_stop = execute_bp_code(bp,
							xp_getreg(xp, UPSREG_FP),
							xp_getreg(xp, UPSREG_AP));
			stopres = xp_get_stopres(xp);
			if (breakpoint_is_solib_event(bp))
			  xp->xp_hit_solib_event = TRUE;
			else
			  xp->xp_hit_solib_event = FALSE;
			check_for_dynamic_libs_sstep(xp, &stopres);
			if (stopres == SR_DIED) {
				stop = TRUE;
				break;
			}

			if (stop_bp == 0)
				stop = bp_wants_stop;
			else if (bp != stop_bp)
				stop = FALSE;
			else if (rtype == RT_STEP || rtype == RT_NEXT)
				stop = plt_stop;
			else
				stop = xp_getreg(xp, UPSREG_FP) >= orig_fp;

			plt_stop = FALSE;
			break;
		case SR_WPT:
			stop = check_all_watchpoints(xp) != NULL;
			break;
		default:
			panic("stopres botch in rt");
			stop = 0; /* to satisfy gcc */
		}

		/*  Get the simple cases out of the way.
		 */
		if (stop)
			break;

		if (check_wps) {
			if (check_all_watchpoints(xp)) {
				stopres = SR_WPT;
				break;
			}
			else if (activate_all_watchpoints(xp) < 0) {
				stopres = SR_FAILED;
				break;
			}

			if (refresh_interval >= 0 && time(NULL) > next_refresh) {
				refresh_target_display(xp, stopres, FALSE);
				next_refresh = time(NULL) + refresh_interval;
			}

			cont_type = CT_CONT;
			check_wps = FALSE;
		}

		if (rtype == RT_CONT)
			continue;
		
		/*  At this point, the following assertions hold:
		 *
		 *	We aren't stopping
		 *	rtype is RT_NEXT or RT_STEP
		 *	stopres is SR_SSTEP or SR_BPT or SR_SIG.
		 *
		 *  First, check the above.
		 */
		if (stop || (rtype != RT_NEXT && rtype != RT_STEP) ||
			    (stopres != SR_SSTEP &&
			     stopres != SR_BPT && stopres != SR_SIG))
			panic("assertion failed in dx_run_target");


		/*  Find out some things about the target state.
		 */
		last_f = f;
		last_pc = pc;
		last_sp = sp;
		pc = xp_getreg(xp, UPSREG_PC);
		sp = xp_getreg(xp, UPSREG_SP);

		/*  If we have nexted or stepped off the end of main,
		 *  we are back in start(), so finish things off by
		 *  doing a cont.  We test for this by seeing if the
		 *  stack pointer is below the stack pointer of main.
		 */
		if (sp > xp_get_base_sp(xp)) {
			cont_type = CT_CONT;
			continue;
		}
		
		f = addr_to_func(pc);

		if (f == NULL || !addr_to_lno(f, pc, &lno, FALSE))
			lno = NULL;
		
		/*  If we hit stop_bp and we are at the right stack level,
		 *  remove and zero stop_bp.
		 */
		if (jsr_fp != 0)
			at_right_stack_level = xp_getreg(xp, UPSREG_FP) >= jsr_fp;
		else
			at_right_stack_level = sp >= jsr_sp;
		if (stopres == SR_BPT && bp == stop_bp && at_right_stack_level) {
			uninstall_breakpoint(stop_bp);
			stop_bp = 0;
			jsr_sp = 0;
			if (tmp_bp != 0) {
				dx_remove_breakpoint(xp, tmp_bp);
				tmp_bp = 0;
			}
			cont_type = CT_STEP;
		}
		 
		/*  If we aren't at the top level, we can just carry on.
		 */
		if (stop_bp != 0)
			continue;
		
		/*  Have we just stepped into a function?
		 *  The test is complicated by recursion - we may have
		 *  changed functions even though f hasn't changed.
		 */
		stepped_into_func = sp < last_sp && (f != last_f || pc <= last_pc);

		/*  If we have stepped into a function, next over it if we
		 *  are nexting or can't step through it.
		 */
		if (stopres == SR_SIG &&
		    xp_get_sigstate(xp, sig) == SGH_CAUGHT) {
			retaddr = xp_get_retaddr_after_sig(xp);
		}
		else if (rtype == RT_STEP &&
			 stepped_into_func &&
                         f != NULL &&
			 (f->fu_flags & FU_PLT) != 0) {
			if ((f = xp_decode_plt(xp, pc)) != NULL) {
				get_min_bpt_addr(f, &retaddr, FALSE);
				plt_stop = TRUE;
			}
			else {
				stop = TRUE;
				stopres = SR_FAILED;
				break;
			}
		}
		else if (stepped_into_func && (rtype == RT_NEXT ||
					       f == NULL || 
					       (f->fu_flags & FU_NOSYM) ||
					       !open_source_file
					       (f->fu_fil, FALSE, FALSE))) {
			retaddr = xp_get_retaddr_after_jsr(xp, 0);
		}
		else {
			retaddr = 0;
		}
		
		if (retaddr != 0) {
			stop_bp = dx_addr_to_breakpoint(xp, retaddr);
			if (stop_bp == NULL) {
				tmp_bp = dx_add_breakpoint(xp, retaddr);
				stop_bp = tmp_bp;
			}
			if (!breakpoint_is_installed(stop_bp)) {
				if (install_breakpoint(stop_bp, xp, TRUE) != 0)
/*	RGA				panic("can't install stop_bp");*/
					errf("can't install stop_bp");
			}
			jsr_sp = sp;
			cont_type = CT_CONT;
			continue;
		}

		/*  We are at the top level, nexting or stepping.
		 *  If we are at a known source line, and the line number
		 *  or function is different from the originals, we stop.
		 */
		if (lno != NULL && (f != orig_f || lno->ln_num != orig_lnum)) {
			if (lno->ln_addr != pc) {
				if (f == orig_f)
					orig_lnum = lno->ln_num;
				continue;
			}
			stop = TRUE; /* unnecessary, but tidy */
			break;
		}
	}

	/*  If we installed watchpoints, remove them.
	 */
	if (target_process_exists(xp))
		uninstall_all_watchpoints(xp);

	/*  If we added a temporary breakpoint ourself, remove it.
	 */
	if (tmp_bp != 0)
		dx_remove_breakpoint(xp, tmp_bp);

	return stopres;
}
