/* trun_j.c - target control when we know where the jumps are */

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


/* @(#)trun_j.c	1.32 24 May 1995 (UKC) */
char ups_trun_j_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#include <local/wn.h>
#include <local/ukcprog.h>
#include <local/obj/obj.h>
#include <mtrprog/utils.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "as.h"
#include "st.h"
#include "breakpoint.h"
#include "watchpoint.h"
#include "trun.h"
#include "data.h"
#include "exec.h"
#include "obj_stack.h"
#include "obj_signal.h"
#include "obj_bpt.h"
#include "obj_wpt.h"
#include "srcbuf.h"
#include "dx.h"
#include "expr.h"

#define MAXVARLEN	128

static void continue_target PROTO((target_t *xp, stopres_t *p_stopres));
static int jump_needed PROTO((jump_t *j));
static jump_t *get_lno_jumps PROTO((target_t *xp, func_t *f, lno_t *lno,
				    bool want_calls,
				    taddr_t *p_addr, taddr_t *p_nextaddr,
                                    taddr_t *p_alt_addr,
				    jumptype_t *p_alt_jumptype));
static jump_t *get_lno_jumps_patched PROTO((target_t *xp, func_t *f, lno_t *lno,
					    bool want_calls,
					    taddr_t *p_addr, taddr_t *p_nextaddr,
					    taddr_t *p_alt_addr,
					    jumptype_t *p_alt_jumptype,
					    int orig_jumps));
static stopres_t cont_target_over_sigs PROTO((target_t *xp));
static void bump_target PROTO((target_t *xp, rtype_t rtype,
						stopres_t *p_stopres));
static void bump_target_patched PROTO((target_t *xp, rtype_t rtype,
				       stopres_t *p_stopres));

static void null_ofunc PROTO((const char *s));

static void
null_ofunc(s)
const char *s;
{
}

/*  If we can't remove a breakpoint at the end of a next, step,
 *  we store in Stuck_bp so that next time dx_run_target() is invoked it can
 *  have another go at clearing it.
 */
static breakpoint_t *Stuck_bp = NULL;

static void
continue_target(xp, p_stopres)
target_t *xp;
stopres_t *p_stopres;
{
	static int recursive=-1;
	stopres_t stopres;
	int stop, sig;
	breakpoint_t *bp;
	taddr_t orig_fp;
	bool check_wps;
	int emulated_wps, refresh_interval;
	time_t next_refresh;
	cont_type_t cont_type = CT_CONT;

	orig_fp = 0;	/* but may be changed below */

	check_wps = FALSE;
	emulated_wps = 0;
	refresh_interval = get_watchpoint_refresh_interval();
	next_refresh = time(NULL) + refresh_interval;

	if (target_process_exists(xp)) {
		if (uninstall_all_breakpoints(xp) != 0)
		  return;
		if ((emulated_wps = install_all_watchpoints(xp)) < 0)
		  return;
		if (install_all_breakpoints(xp) != 0)
		  return;
	}

	recursive++;
	do {
	  /*	  if ( recursive ){*/
	  /*               if (uninstall_all_watchpoints(xp) < 0)*/
	  /*	       return;}*/
		if (target_process_exists(xp)) {
			stopres = xp_restart_child(xp, get_restart_sig(xp),
						      cont_type);
		}
		else {
			stopres = dx_start(xp);
			if (xp_get_state(xp) != TS_STOPPED)
				break;
		}
		
		switch(stopres) {
		case SR_DIED:
		case SR_USER:
		case SR_FAILED:
			stop = TRUE;
			break;
		case SR_SSTEP:
			/*  This should never happen, but it seems to
			 *  because of an oddity with signals, ptrace and
			 *  instructions which branch to themselves.
			 */
			stop = FALSE;
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
					stop = FALSE; /* to satisfy gcc */
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
			/* RCB: Don't execute breakpoint code if recursive;
			 *      let the top level call do it.
			 */
			if ( recursive )
			    stop = TRUE;
			else
			{
			    stop = execute_bp_code(bp, xp_getreg(xp, UPSREG_FP),
						       xp_getreg(xp, UPSREG_AP));
			    stopres = xp_get_stopres(xp);
			    if (stopres != SR_BPT)
				    stop = TRUE;
			}
			break;
		case SR_WPT:
			stop = check_all_watchpoints(xp) != NULL;
			break;
		default:
			panic("stopres botch in ct");
			stop = FALSE; /* to satisfy gcc */
		}

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
	} while (!stop);
	
	/*  If we installed watchpoints, remove them.
	 */
	if (target_process_exists(xp))
		uninstall_all_watchpoints(xp);

	*p_stopres = stopres;
	recursive--;
}

/*  Return TRUE if a jump is needed.  The only jumps that aren't needed
 *  are calls to a known address where the known address is within a
 *  function for which we don't have symbol table information.
 *
 *  You might think we could also skip branches to a destination within
 *  the line, but we can't because these jumps might jump over the
 *  breakpoint we have set.
 */
static int
jump_needed(j)
register jump_t *j;
{
	func_t *f;

	if (j->ju_type == JT_CALL && j->ju_dstaddr != 0) {
		f = addr_to_func(j->ju_dstaddr);
		return f != NULL && FU_LNOS(f) != NULL;
	}

	return TRUE;
}

static jump_t *
get_lno_jumps(xp, f, lno, want_calls,
	      p_addr, p_nextaddr, p_alt_addr, p_alt_jumptype)
target_t *xp;
func_t *f;
lno_t *lno;
int want_calls;
taddr_t *p_addr, *p_nextaddr, *p_alt_addr;
jumptype_t *p_alt_jumptype;
{
        enum {
                ALL_WITHIN_LINE,
                WITHIN_LINE_OR_TO_ONE_ADDR,
                TO_MULTIPLE_PLACES
        } jump_disp;
	taddr_t addr, nextaddr, alt_addr;
	jumptype_t alt_jumptype;
	jump_t *jumps, *src, *dst, *res;
	char *text;
	size_t len;
	
	if (lno == NULL || f == NULL)
		panic("lno or f NULL in get_lno_jumps");
	
	addr = lno->ln_addr;
	
	if (lno->ln_next != NULL)
		nextaddr = lno->ln_next->ln_addr;
	else
		nextaddr = st_get_addr_lim(f);

	len = nextaddr - addr;
	text = e_malloc(len);
	
	if (st_read_text(f->fu_symtab, addr, text, len) != 0) {
		failmesg("Can't read text of", "function", f->fu_demangled_name);
		return NULL;
	}
	
	jumps = st_get_jumps(f, addr, text, len, want_calls);
	
	free(text);

	src = dst = jumps;
	jump_disp = ALL_WITHIN_LINE;
        alt_addr = 0;
	alt_jumptype = JT_END;	/* to satisfy gcc */
        
	while (src->ju_type != JT_END) {
		if (!jump_needed(src)) {
                        ++src;
                        continue;
                }
                
                if (src->ju_dstaddr == 0) {
                        jump_disp = TO_MULTIPLE_PLACES;
                }
                else if (src->ju_dstaddr < addr || src->ju_dstaddr > nextaddr) {
                        switch (jump_disp) {
                        case ALL_WITHIN_LINE:
                                alt_addr = src->ju_dstaddr;
				alt_jumptype = src->ju_type;
                                jump_disp = WITHIN_LINE_OR_TO_ONE_ADDR;
                                break;
                        case WITHIN_LINE_OR_TO_ONE_ADDR:
                                if (src->ju_dstaddr != alt_addr ||
				    src->ju_type != alt_jumptype)
                                        jump_disp = TO_MULTIPLE_PLACES;
                                break;
                        case TO_MULTIPLE_PLACES:
                                break;
                        }
		}
		
                *dst++ = *src++;
        }
        
        src->ju_type = JT_END;       

	*p_addr = addr;
	*p_nextaddr = nextaddr;

	if (jump_disp == TO_MULTIPLE_PLACES) {
		*p_alt_addr = 0;
		*p_alt_jumptype = JT_END;
		res = jumps;
	}
	else {
		*p_alt_addr = alt_addr;
		*p_alt_jumptype = alt_jumptype;
		res = src;	/* point res at a JT_END entry */
	}

	return res;
}

static jump_t *
get_lno_jumps_patched(xp, f, lno, want_calls,
		      p_addr, p_nextaddr, p_alt_addr, p_alt_jumptype, orig_jumps)
target_t *xp;
func_t *f;
lno_t *lno;
int want_calls;
taddr_t *p_addr, *p_nextaddr, *p_alt_addr;
jumptype_t *p_alt_jumptype;
int orig_jumps;
{
        enum {
                ALL_WITHIN_LINE,
                WITHIN_LINE_OR_TO_ONE_ADDR,
                TO_MULTIPLE_PLACES
        } jump_disp;
	taddr_t addr, nextaddr, alt_addr;
	jumptype_t alt_jumptype;
	jump_t *jumps, *src, *dst, *res;
	char *text;
	size_t len;

	if (lno == NULL || f == NULL)
		panic("lno or f NULL in get_lno_jumps_patched");
	
	addr = lno->ln_addr;
	
	if (lno->ln_next != NULL)
		nextaddr = lno->ln_next->ln_addr;
	else
		nextaddr = st_get_addr_lim(f);

	len = nextaddr - addr;
	text = e_malloc(len);
	
	if (st_read_text(f->fu_symtab, addr, text, len) != 0) {
		failmesg("Can't read text of", "function", f->fu_demangled_name);
		return NULL;
	}
	
	jumps = st_get_jumps(f, addr, text, len, want_calls);
	
	free(text);

	src = dst = jumps;
	jump_disp = ALL_WITHIN_LINE;
        alt_addr = 0;
	alt_jumptype = JT_END;	/* to satisfy gcc */
        
#ifdef ARCH_SUN4
	jump_disp = TO_MULTIPLE_PLACES;
	if (!orig_jumps)
	  for(;;) {
	    if (jump_needed(src)) {
	      if (src != dst)
		*dst = *src;
	      ++dst;
	    }
	    if (src->ju_type == JT_END)
	      break;
	    ++src;
	  }
#else
	while (src->ju_type != JT_END) {
		if (!jump_needed(src)) {
                        ++src;
                        continue;
                }
                
                if (src->ju_dstaddr == 0) {
                        jump_disp = TO_MULTIPLE_PLACES;
                }
                else if (src->ju_dstaddr < addr || src->ju_dstaddr > nextaddr) {
                        switch (jump_disp) {
                        case ALL_WITHIN_LINE:
                                alt_addr = src->ju_dstaddr;
				alt_jumptype = src->ju_type;
                                jump_disp = WITHIN_LINE_OR_TO_ONE_ADDR;
                                break;
                        case WITHIN_LINE_OR_TO_ONE_ADDR:
                                if (src->ju_dstaddr != alt_addr ||
				    src->ju_type != alt_jumptype)
                                        jump_disp = TO_MULTIPLE_PLACES;
                                break;
                        case TO_MULTIPLE_PLACES:
                                break;
                        }
		}
		
                *dst++ = *src++;
        }
        
        src->ju_type = JT_END;   

#endif /* not ARCH_SUN4 */

	*p_addr = addr;
	*p_nextaddr = nextaddr;

	if (jump_disp == TO_MULTIPLE_PLACES) {
		*p_alt_addr = 0;
		*p_alt_jumptype = JT_END;
		res = jumps;
	}
	else {
		*p_alt_addr = alt_addr;
		*p_alt_jumptype = alt_jumptype;
		res = src;	/* point res at a JT_END entry */
	}

	return res;
}

static stopres_t
cont_target_over_sigs(xp)
target_t *xp;
{
	stopres_t stopres;
	taddr_t lastsp, lastpc;
	int lastsig;

	lastsp = lastpc = 0;
	lastsig = 0;

	for (;;) {
		taddr_t sp, pc;
		int sig;

		stopres = xp_restart_child(xp, get_restart_sig(xp), CT_CONT);
		if (stopres != SR_SIG)
			break;

		sig = xp_get_lastsig(xp);
		pc = xp_getreg(xp, UPSREG_PC);
		sp = xp_getreg(xp, UPSREG_SP);

		if (pc == lastpc && sp == lastsp && sig == lastsig)
			break;
		lastpc = pc;
		lastsp = sp;
		lastsig = sig;

		if (sig_kills_target_by_default(sig) &&
				xp_get_sigstate(xp, sig) == SGH_DEFAULT)
			break;

		if (sig_causes_refresh(sig))
			refresh_target_display(xp, stopres, FALSE);
	}
	check_for_dynamic_libs(xp, &stopres);
	return stopres;
}

static void
bump_target(xp, rtype, p_stopres)
target_t *xp;
rtype_t rtype;
stopres_t *p_stopres;
{
	static char error_stackfile[] = "ups_stack.error";
	taddr_t addrlim;
	jump_t *jumps, *j;
	jumptype_t alt_jumptype;
	lno_t *lno, *old_lno;
	func_t *f, *old_f;
	taddr_t fp, orig_fp, adjusted_pc, pc, last_pc;
        taddr_t addr, nextaddr, bpt_addr, alt_addr;
	stopres_t stopres;
	bool off_end_of_main;
	breakpoint_t *bp, *tmp_bp;

	get_current_func(&f, &orig_fp, &pc, &adjusted_pc);

	/*  We want the adjusted pc (see get_current_func) for the
	 *  bpt_addr == pc test below.  This is to avoid a false
	 *  positive when the saved pc after a function call points
	 *  at the next source line.
	 */
	pc = adjusted_pc;

	if (f == NULL || !addr_to_lno(f, pc, &lno, FALSE)) {
		errf("Can't step in function with no line number information");
		return;
	}

	if (uninstall_all_breakpoints(xp) != 0)
		return;

	install_solib_event_breakpoint(xp);

	addrlim = st_get_addr_lim(f);
	jumps = get_lno_jumps(xp, f, lno, rtype == RT_STEP,
                              &addr, &nextaddr, &alt_addr, &alt_jumptype);

	if (jumps == NULL)
		return;
	
	bp = 0;
	off_end_of_main = FALSE;
	
	for(;;) {
                bool at_alt_addr;
		jumptype_t jumptype;
                
		/*  Find the first jump instruction after or at the pc.
		 */
		for (j = jumps; j->ju_type != JT_END && j->ju_addr < pc; ++j)
			;
		bpt_addr = j->ju_type != JT_END ? j->ju_addr : nextaddr;
		if (bpt_addr < pc) {
			dump_stack_to_file(xp, error_stackfile);
			panic("bpt addr botch");
		}

		/*  If we are not already at the jump, set a breakpoint
		 *  at the jump and continue the target to let it get there.
		 *  Check that we got to the breakpoint we were expecting.
		 */
		if (bpt_addr == pc) {
			stopres = SR_BPT;
			fp = xp_getreg(xp, UPSREG_FP);
                        at_alt_addr = FALSE;
			jumptype = j->ju_type;
		}
		else {
                        breakpoint_t *alt_bp;
                        
                        bp = dx_add_breakpoint(xp, bpt_addr);
			if (install_breakpoint(bp, xp) != 0) {
				stopres = SR_FAILED;
				break;
			}

			/*  If alt_addr is non-zero, it is known to be the
			 *  only jump destination outside the current line.
			 *
			 *  This means we can put a breakpoint there (as well
			 *  as at the next line), and ignore any jumps within
			 *  the line.  This is useful mainly for f90 array
			 *  assignments, although it breaks if there are
			 *  more than two jumps outside the line.
			 */
                        if (alt_addr == 0) {
                                alt_bp = NULL;
                        }
                        else {
                                alt_bp = dx_add_breakpoint(xp, alt_addr);

                                if (install_breakpoint(alt_bp, xp) != 0) {
                                        stopres = SR_FAILED;
                                        break;
                                }
                        }
                        
			for(;;) {
				stopres = cont_target_over_sigs(xp);
				if (stopres != SR_BPT) {
					fp = 0; /* to satisfy gcc */
					break;
				}
				fp = xp_getreg(xp, UPSREG_FP);
				if (fp >= orig_fp)
					break;
			}
			if (dx_remove_breakpoint(xp, bp) != 0)
				break;
			bp = 0;
			if (stopres != SR_BPT)
				break;
			pc = xp_getreg(xp, UPSREG_PC);

                        at_alt_addr = alt_bp != NULL && pc == alt_addr;

                        if (at_alt_addr) {
                                bpt_addr = alt_addr;
				jumptype = alt_jumptype;
			}
			else {
				jumptype = j->ju_type;
			}
                        
			if (pc != bpt_addr)
				panic("unexpected pc in bt");

                        if (alt_bp != NULL &&
                            dx_remove_breakpoint(xp, alt_bp) != 0)
                                break;

                        alt_bp = NULL;
		}


		/*  If we have reached the next line, we've finished the
		 *  next or step.
		 *
		 *  As we fell through to this line rather than jumping,
		 *  if there is a breakpoint at the line, execute the
		 *  code associated with the breakpoint.
		 */
		if (bpt_addr == nextaddr && (rtype == RT_STEP || fp >= orig_fp)) {
			breakpoint_t *bp_at_nextline;

			bp_at_nextline = dx_addr_to_breakpoint(xp, pc);
			if (bp_at_nextline != NULL) {
				execute_bp_code(bp_at_nextline, fp,
							xp_getreg(xp, UPSREG_AP));
				stopres = xp_get_stopres(xp);
			}
			break;
		}

                if (!at_alt_addr) {
                        /*  OK, we are now sitting at a jump instruction of
                         *  some sort.  Single step to execute that instruction
                         *  and find out where we jumped to.
                         */
                        stopres = xp_restart_child(xp, get_restart_sig(xp), CT_STEP);
                        if (stopres != SR_SSTEP)
                                break;
                }

		last_pc = pc;
		pc = xp_getreg(xp, UPSREG_PC);
		fp = xp_getreg(xp, UPSREG_FP);

		/*  If we are still at the same line and stack level, continue.
		 */
		if (pc >= addr && pc < nextaddr && fp <= orig_fp) {
			if (pc < last_pc)
				break;
			else
				continue;
		}
		
		/*  Off the line - find the new lno and func.
		 *  We save the old lno first because we want to check
		 *  that we've moved onto a different source line (you
		 *  can get multiple lnos with the same source line
		 *  number).
		 */
		old_lno = lno;
		old_f = f;
		if (pc == nextaddr && lno->ln_next != NULL) {
			lno = lno->ln_next;
		}
		else {
			if (pc < f->fu_addr || pc >= addrlim) {
				f = addr_to_func(pc);
				if (f == NULL)
					break;
				addrlim = st_get_addr_lim(f);
			}

			if (!addr_to_lno(f, pc, &lno, FALSE))
				lno = NULL;
			
			/*  If we don't know which lno we're at after
			 *  a branch, stop.
			 *
			 *  This can happen when returning to function
			 *  with no symbol table information (a RET
			 *  instruction is treated as a branch).
			 *
			 *  A call to a function usually puts the pc in
			 *  the function prologue, so we don't mind not
			 *  knowing which lno we're at after a call.
			 *  We move the pc onwards below in this case.
			 */
			if (jumptype != JT_CALL && lno == NULL) {
				off_end_of_main = fp == TADDR_NULL;
				break;
			}
		}

		/*  We're done if we're at a source line boundary with
		 *  a different source line number than the current lno.
		 *
		 *  Note that it is possible to jump from within a source
		 *  line to the start of the line.  One example of this is
		 *  the SPARC, where we jump over the code for the body of
		 *  a switch statement to the switch itself.  The compiler
		 *  emits two line number symbols referring the the source
		 *  line containing the switch, and we jump from the first
		 *  to the start of the second.
		 */
		if (lno != NULL) {
			bool on_boundary, new_func, new_lnum, new_level;
			
			on_boundary = pc == lno->ln_addr;
			new_func = f != old_f;
			new_lnum = lno->ln_num != old_lno->ln_num;
			new_level = fp != orig_fp;

			if (on_boundary && (new_func || new_lnum || new_level))
				break;
		}
		
		/*  If we have just stepped into a function, move the pc
		 *  up to the minimum breakpoint address and stop.
		 */
		if (jumptype == JT_CALL) {
			taddr_t call_bpt_addr;
			breakpoint_t *bp_at_start_of_func;

			if (get_min_bpt_addr(f, &call_bpt_addr, FALSE) != 0)
				panic("can't get min bpt addr");
			bp_at_start_of_func = dx_addr_to_breakpoint(xp, call_bpt_addr);

			if (pc == call_bpt_addr)
				stopres = SR_BPT;
			else {
				if (bp_at_start_of_func != 0) {
					bp = bp_at_start_of_func;
					tmp_bp = 0;
				}
				else {
					bp = dx_add_breakpoint(xp,
							       call_bpt_addr);
					tmp_bp = bp;
				}
				if (install_breakpoint(bp, xp) != 0)
					break;
				stopres = xp_restart_child(xp, 0, CT_CONT);
				if (tmp_bp != 0 && xp_remove_breakpoint(xp, tmp_bp) != 0)
					break;
				bp = 0;
			}
			if (stopres != SR_BPT)
				break;
			if (xp_getreg(xp, UPSREG_PC) != call_bpt_addr) {
				dump_stack_to_file(xp, error_stackfile);
				panic("pc escaped in bump_target");
			}
			if (bp_at_start_of_func != NULL) {
				execute_bp_code(bp_at_start_of_func,
							xp_getreg(xp, UPSREG_FP),
							xp_getreg(xp, UPSREG_AP));
				stopres = xp_get_stopres(xp);
			}
			break;
		}
		
		/*  Move focus to the new line.
		 */
		jumps = get_lno_jumps(xp, f, lno, rtype == RT_STEP,
                                      &addr, &nextaddr,
				      &alt_addr, &alt_jumptype);
		orig_fp = xp_getreg(xp, UPSREG_FP);
		if (jumps == NULL)
			break;
	}
        
	if (bp != 0 && xp_remove_breakpoint(xp, bp) != 0)
		Stuck_bp = bp;

	if (off_end_of_main)
		continue_target(xp, &stopres);
	
	*p_stopres = stopres;
	return;
}

static void
bump_target_patched(xp, rtype, p_stopres)
target_t *xp;
rtype_t rtype;
stopres_t *p_stopres;
{
	static char error_stackfile[] = "ups_stack.error";
	taddr_t addrlim;
	jump_t *jumps, *j, *last_j;
	jumptype_t alt_jumptype;
	lno_t *lno, *old_lno;
	func_t *f, *old_f;
	taddr_t fp, orig_fp, adjusted_pc, pc, last_pc;
        taddr_t addr, nextaddr, bpt_addr, alt_addr, dummy_addr;
	stopres_t stopres;
	bool off_end_of_main;
	breakpoint_t *bp, *tmp_bp, *tmp_bp1 = 0;
	int first = 1, out_range = 0, do_step_into = 0;
	char buff[MAXVARLEN + 1];
	func_bp_list_t *fl, *funchead;
	errf_ofunc_t oldf;
	fil_t *fil;
	int dummy_lnum;

#ifdef ARCH_SUN4
	static int checked = 0, sunos_step = 1;
	jumptype_t alt_jumptype1;
	func_t *orig_f;
        taddr_t alt_addr1;

	if (!checked)
	{
	  const char *env_str = NULL;

	  env_str = (char *)getenv("UPS_SUNOS_STEP");
	  if (env_str)
	  {
	    char *str;
	    int i;

	    str = malloc(strlen(env_str) + 1);
	    for (i = 0; i < strlen(env_str); i++)
	      if (islower(*(env_str+i)))
		str[i] = toupper(*(env_str+i));
	      else
		str[i] = *(env_str+i);
	    str[strlen(env_str)] = 0;
	    sunos_step = (!strcmp(str, "TRUE") || !strcmp(str, "ON") ||
			  !strcmp(str, "1")) ? 1 : 0;
	    if (!strcmp(str, "2"))
	      sunos_step = 2;
	    free(str);
	  }
	  checked = 1;
	}
#endif /* ARCH_SUN4 */

	funchead = 0;

retry:
	get_current_func(&f, &orig_fp, &pc, &adjusted_pc);

	/*  We want the adjusted pc (see get_current_func) for the
	 *  bpt_addr == pc test below.  This is to avoid a false
	 *  positive when the saved pc after a function call points
	 *  at the next source line.
	 */
	pc = adjusted_pc;

	if (f == NULL || !addr_to_lno(f, pc, &lno, FALSE)) {
		errf("Can't step in function with no line number information");
		return;
	}

	if (uninstall_all_breakpoints(xp) != 0)
		return;

	addrlim = st_get_addr_lim(f);
	jumps = get_lno_jumps_patched(xp, f, lno, rtype == RT_STEP,
				      &addr, &nextaddr, &alt_addr,
				      &alt_jumptype, 0);

	if (jumps == NULL)
		return;
	
	bp = 0;
	off_end_of_main = FALSE;
	
	for(;;) {
                bool at_alt_addr;
		jumptype_t jumptype;
                
		/*  Find the first jump instruction after or at the pc.
		 */
		for (j = jumps; j->ju_type != JT_END && j->ju_addr < pc; ++j)
			;
		/* RGA - Skip jumps to unknown destinations - Purify seems to
		   create these
		   */
		if (first && out_range && !j->ju_dstaddr && j->ju_type == JT_CALL)
		  for (last_j = j; j->ju_type == JT_CALL && !j->ju_dstaddr &&
		       j->ju_addr < nextaddr; ++j)
		    last_j = j;
/*		if (first && out_range && j->ju_type == JT_BRANCH)
		  for (; j->ju_type != JT_END && j->ju_addr <= pc; ++j);*/
		first = 0;

		bpt_addr = j->ju_type != JT_END ? j->ju_addr : nextaddr;
		if (bpt_addr < pc) {
			dump_stack_to_file(xp, error_stackfile);
			panic("bpt addr botch");
		}

	  get_min_bpt_addr(f, &dummy_addr, FALSE);
          addr_to_fil_and_lnum(f, dummy_addr, &fil, &dummy_lnum, FALSE);
		
#ifdef ARCH_SUN4
	  /* RGA: try to cope with stepping into a shared library or
	     stepping from one shared library into another. Do this by
	     trying to find the function name that is being stepped to,
	     then use this function as the next breakpoint address.
	     Obviously does not work for macros mapped to function names. */

	  if (sunos_step && !do_step_into && rtype == RT_STEP &&
	      f->fu_language != LANG_CC
/* &&
	      func_in_dynamic_lib(f->fu_symtab_id)*/
)
	  {
	    jump_t *orig_jumps, *jmp;
	    taddr_t addr1, nextaddr1, bpt_addr1;
	    int unmapped_func;

	    orig_jumps = get_lno_jumps_patched(xp, f, lno, rtype == RT_STEP,
					       &addr1, &nextaddr1,
					       &alt_addr1, &alt_jumptype1, 1);
	    for (unmapped_func = 0, jmp = jumps; jmp->ju_type != JT_END; ++jmp)
	      if (jmp->ju_type == JT_CALL)
	      { 
		if (jmp->ju_dstaddr == 0 || !addr_to_func(jmp->ju_dstaddr))
		{
		  unmapped_func = 1;
		  break;
		}
	      }
	    if (unmapped_func)
	    {
	      lno_t *l;
	      const char *c, *d;
	      size_t start, lim;
	      func_t *f_step, *f_step1;
	      int k, last_l, find_func;
	      
	      open_source_file(fil, FALSE, FALSE);
	      for (l = FU_LNOS(f), last_l = l ? l->ln_num - 1 : 0;
		   l != NULL; l = l->ln_next)
	      {
		if (l->ln_addr == pc)
		{
		  for (k = 1; l->ln_num - k >= last_l; k++)
		  {
		    c = (char *)srcbuf_get_file_line(fil->fi_srcbuf,
						     l->ln_num - k + 1);
		    d = c;
		    if (d)
		    {
		      while(d && *d)
		      {
			for(; d && *d && !isalpha(*d); d++);
			
			if (get_varname(f->fu_language, c, d, &start, &lim))
			{
			  (void) strncpy(buff, c + start, lim - start);
			  buff[lim - start] = '\0';
			  oldf = errf_set_ofunc(null_ofunc);
			  find_func = find_func_by_name(buff, &f_step, &f_step1, TRUE);
			  errf_set_ofunc(oldf);
			  if (!find_func &&
			      (sunos_step == 2 || fil != f_step->fu_fil))
			  {
			    orig_jumps = get_lno_jumps_patched
			      (xp, f, lno, 0, /* next */
			       &addr1, &nextaddr1, &alt_addr1, &alt_jumptype1, 1);
			    bpt_addr1 = nextaddr;
			    if (bpt_addr1 == pc) {
			      stopres = SR_BPT;
			      fp = xp_getreg(xp, UPSREG_FP);
			    }
			    else {
			      /* install a bpt at next line in case the f_step
				 is not called */
			      tmp_bp1 = dx_add_breakpoint(xp, bpt_addr1);
			      if (install_breakpoint(tmp_bp1, xp) != 0) {
				stopres = SR_FAILED;
				break;
			      }
			    }
			    orig_f = f;
			    f = f_step;
			    do_step_into = 1;
			    goto set_bpt;
			  }
			  d = c + lim;
			}
		      }
		    }
		  }
		}
		last_l = l->ln_num - 1;
	      }
	    }
	  }
#endif /* ARCH_SUN4 */

	    
		/*  If we are not already at the jump, set a breakpoint
		 *  at the jump and continue the target to let it get there.
		 *  Check that we got to the breakpoint we were expecting.
		 */
		if (bpt_addr == pc) {
			stopres = SR_BPT;
			fp = xp_getreg(xp, UPSREG_FP);
                        at_alt_addr = FALSE;
			jumptype = j->ju_type;
		}
		else {
                        breakpoint_t *alt_bp;
                        
                        bp = dx_add_breakpoint(xp, bpt_addr);
			if (install_breakpoint(bp, xp) != 0) {
				stopres = SR_FAILED;
				break;
			}

			/*  If alt_addr is non-zero, it is known to be the
			 *  only jump destination outside the current line.
			 *
			 *  This means we can put a breakpoint there (as well
			 *  as at the next line), and ignore any jumps within
			 *  the line.  This is useful mainly for f90 array
			 *  assignments, although it breaks if there are
			 *  more than two jumps outside the line.
			 */
                        if (alt_addr == 0) {
                                alt_bp = NULL;
                        }
                        else {
                                alt_bp = dx_add_breakpoint(xp, alt_addr);

                                if (install_breakpoint(alt_bp, xp) != 0) {
                                        stopres = SR_FAILED;
                                        break;
                                }
                        }
                        
			for(;;) {
				stopres = cont_target_over_sigs(xp);
				if (stopres != SR_BPT) {
					fp = 0; /* to satisfy gcc */
					break;
				}
				fp = xp_getreg(xp, UPSREG_FP);
				if (fp >= orig_fp)
					break;
			}
			if (dx_remove_breakpoint(xp, bp) != 0)
				break;
			bp = 0;
			if (stopres != SR_BPT)
				break;
			pc = xp_getreg(xp, UPSREG_PC);

                        at_alt_addr = alt_bp != NULL && pc == alt_addr;

                        if (at_alt_addr) {
                                bpt_addr = alt_addr;
				jumptype = alt_jumptype;
			}
			else {
				jumptype = j->ju_type;
			}
                        
			if (pc != bpt_addr)
				panic("unexpected pc in bt");

                        if (alt_bp != NULL &&
                            dx_remove_breakpoint(xp, alt_bp) != 0)
                                break;

                        alt_bp = NULL;
		}


		/*  If we have reached the next line, we've finished the
		 *  next or step.
		 *
		 *  As we fell through to this line rather than jumping,
		 *  if there is a breakpoint at the line, execute the
		 *  code associated with the breakpoint.
		 */
		if (bpt_addr == nextaddr && (rtype == RT_STEP || fp >= orig_fp)) {
			breakpoint_t *bp_at_nextline;

			bp_at_nextline = dx_addr_to_breakpoint(xp, pc);
			if (bp_at_nextline != NULL) {
				execute_bp_code(bp_at_nextline, fp,
							xp_getreg(xp, UPSREG_AP));
				stopres = xp_get_stopres(xp);
			}
			break;
		}

                if (!at_alt_addr) {
                        /*  OK, we are now sitting at a jump instruction of
                         *  some sort.  Single step to execute that instruction
                         *  and find out where we jumped to.
                         */
                        stopres = xp_restart_child(xp, get_restart_sig(xp), CT_STEP);
                        if (stopres != SR_SSTEP)
                                break;
                }

		last_pc = pc;
		pc = xp_getreg(xp, UPSREG_PC);
		fp = xp_getreg(xp, UPSREG_FP);

		/*  If we are still at the same line and stack level, continue.
		 */
		if (pc >= addr && pc < nextaddr && fp <= orig_fp) {
			if (pc < last_pc)
				break;
			else
				continue;
		}
		
		/*  Off the line - find the new lno and func.
		 *  We save the old lno first because we want to check
		 *  that we've moved onto a different source line (you
		 *  can get multiple lnos with the same source line
		 *  number).
		 */
		old_lno = lno;
		old_f = f;
		if (pc == nextaddr && lno->ln_next != NULL) {
			lno = lno->ln_next;
		}
		else {
			if (pc < f->fu_addr || pc >= addrlim) {
				f = addr_to_func(pc);
				if (f == NULL)
				{
				  first = 1;
				  if (out_range == 0)
				  {
				    out_range++;
/*				    rtype = RT_NEXT;*/
				    goto retry;
				  }
				  else
				    if (out_range == 1)
				    {
				      out_range++;
				      rtype = RT_NEXT;
				      goto retry;
				    }
				    else
				      break;
				}
				addrlim = st_get_addr_lim(f);
			}

			if (!addr_to_lno(f, pc, &lno, FALSE))
				lno = NULL;
			
			/*  If we don't know which lno we're at after
			 *  a branch, stop.
			 *
			 *  This can happen when returning to function
			 *  with no symbol table information (a RET
			 *  instruction is treated as a branch).
			 *
			 *  A call to a function usually puts the pc in
			 *  the function prologue, so we don't mind not
			 *  knowing which lno we're at after a call.
			 *  We move the pc onwards below in this case.
			 */
			if (jumptype != JT_CALL && lno == NULL) {
				off_end_of_main = fp == TADDR_NULL;
				break;
			}
		}

		/*  We're done if we're at a source line boundary with
		 *  a different source line number than the current lno.
		 *
		 *  Note that it is possible to jump from within a source
		 *  line to the start of the line.  One example of this is
		 *  the SPARC, where we jump over the code for the body of
		 *  a switch statement to the switch itself.  The compiler
		 *  emits two line number symbols referring the the source
		 *  line containing the switch, and we jump from the first
		 *  to the start of the second.
		 */
		if (lno != NULL) {
			bool on_boundary, new_func, new_lnum, new_level;
			
			on_boundary = pc == lno->ln_addr;
			new_func = f != old_f;
			new_lnum = lno->ln_num != old_lno->ln_num;
			new_level = fp != orig_fp;

			if (on_boundary && (new_func || new_lnum || new_level))
				break;
		}
		
		/*  If we have just stepped into a function, move the pc
		 *  up to the minimum breakpoint address and stop.
		 */
		if (jumptype == JT_CALL) {
			taddr_t call_bpt_addr;
			breakpoint_t *bp_at_start_of_func;

#ifdef ARCH_SUN4
set_bpt:
#endif
			if (get_min_bpt_addr(f, &call_bpt_addr, FALSE) != 0)
				panic("can't get min bpt addr");
			bp_at_start_of_func = dx_addr_to_breakpoint(xp, call_bpt_addr);

			if (pc == call_bpt_addr)
				stopres = SR_BPT;
			else {
				if (bp_at_start_of_func != 0) {
					bp = bp_at_start_of_func;
					tmp_bp = 0;
				}
				else {
					bp = dx_add_breakpoint(xp,
							       call_bpt_addr);
					tmp_bp = bp;
					if (!do_step_into)
					  tmp_bp1 = 0;
				}
				if (install_breakpoint(bp, xp) != 0)
					break;
				stopres = xp_restart_child(xp, 0, CT_CONT);
				if (tmp_bp1 != 0)
				  dx_remove_breakpoint(xp, tmp_bp1);
				if (tmp_bp != 0 && xp_remove_breakpoint(xp, tmp_bp) != 0)
					break;
				tmp_bp1 = 0;
				bp = 0;
			}
			if (stopres != SR_BPT)
				break;
			if (xp_getreg(xp, UPSREG_PC) != call_bpt_addr) {
/*  RGA can happen if the SUNOS_STEP puts a breakpoint which the target */
/* never gets to... */
/*	RGA			dump_stack_to_file(xp, error_stackfile);*/
/*	RGA			panic("pc escaped in bump_target");*/
			  ;
			}
			else
			  if (do_step_into)
			    errf("\bStepping into `%s'", buff);
			if (bp_at_start_of_func != NULL) {
				execute_bp_code(bp_at_start_of_func,
							xp_getreg(xp, UPSREG_FP),
							xp_getreg(xp, UPSREG_AP));
				stopres = xp_get_stopres(xp);
			}
			break;
		}
		
		/*  Move focus to the new line.
		 */
		if (do_step_into)
		{
		  get_current_func(&f, &orig_fp, &pc, &adjusted_pc);
		  addr_to_lno(f, pc, &lno, FALSE);
		}
		jumps = get_lno_jumps_patched(xp, f, lno, rtype == RT_STEP,
					      &addr, &nextaddr,
					      &alt_addr, &alt_jumptype, 0);
		orig_fp = xp_getreg(xp, UPSREG_FP);
		if (jumps == NULL)
			break;
	}
        
	if (bp != 0 && xp_remove_breakpoint(xp, bp) != 0)
		Stuck_bp = bp;

	if (tmp_bp1 != 0)
	  dx_remove_breakpoint(xp, tmp_bp1);
	if (off_end_of_main)
		continue_target(xp, &stopres);
	
	*p_stopres = stopres;
	if (funchead)
	{
	  oldf = errf_set_ofunc(null_ofunc);
	  for (fl = funchead; fl != NULL; fl = fl->fl_next)
	    if (fl->fl_bp)
	      dx_remove_breakpoint(xp, (breakpoint_t*)fl->fl_bp);
	  do_free_func_bp_list_list(funchead);
	  errf_set_ofunc(oldf);
	}
	return;
}

stopres_t
dx_run_target(xp, rtype)
target_t *xp;
rtype_t rtype;
{
	stopres_t stopres;
	static int non_frame_functions = -1;

	if (non_frame_functions == -1)
	  non_frame_functions = user_wants_non_frame_functions();

	/*  Fall back to single-step style of target control if we don't
	 *  have a get_jumps() routine.
	 */
	if (xp_main_symtab(xp)->st_ops->so_get_jumps == NULL)
		return dx_run_target_ss(xp, rtype);

#ifdef OS_SUNOS
	/* RGA using single stepping for Sun shared libs */
	if (!non_frame_functions && rtype == RT_STEP)
		return dx_run_target_ss(xp, rtype);
#endif

	if (!target_process_exists(xp) && rtype != RT_CONT)
		panic("bad rtype in dx_run_target");

	if (Stuck_bp != 0) {
		if (xp_remove_breakpoint(xp, Stuck_bp) == 0)
			Stuck_bp = 0;
		else
			return SR_FAILED;
	}

	/*  Set stopres to anything other than SR_DIED or SR_SIG.
	 */
	stopres = SR_BPT;

	if (rtype == RT_CONT) {
		continue_target(xp, &stopres);
	}
	else {
	  if (non_frame_functions)
	    bump_target_patched(xp, rtype, &stopres);
	  else
	    bump_target(xp, rtype, &stopres);

		/*  TODO: get rid of this test (and xp_get_base_sp()).
		 *        Make bump_target() work out whether we have
		 *        stepped off the end of main() (it already does
		 *        under Solaris 2.
		 */
		if (stopres != SR_DIED) {
			taddr_t sp;

			sp = xp_getreg(xp, UPSREG_SP);
			if (sp > xp_get_base_sp(xp))
				continue_target(xp, &stopres);
		}
	}

	return stopres;
}
