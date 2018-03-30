/* exec.c - code for starting and managing target execution */

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


/* @(#)exec.c	1.43 04 Jun 1995 (UKC) */
char ups_exec_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <fcntl.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <local/wn.h>
#include <local/arg.h>
#include <local/obj/obj.h>

#include "ao_ifdefs.h"

#if HAVE_MACHINE_REG_H
#include <machine/reg.h>
#endif

#include "ups.h"
#include "symtab.h"
#include "va.h"
#include "ci.h"
#include "cursors.h"
#include "target.h"
#include "st.h"
#include "exec.h"
#include "mreg.h"
#include "dx.h"
#include "breakpoint.h"
#include "obj_stack.h"
#include "obj_target.h"
#include "obj_signal.h"
#include "obj_buildf.h"
#include "obj_bpt.h"
#include "obj_env.h"
#include "trun.h"
#include "printf.h"
#include "ui.h"
#include "reg.h"
#include "menudata.h"
#include "tdr.h"
#include "state.h"
#include "ao_target.h"
#include "ao_procfs.h"
#include "ao_ptrace.h"
#ifdef AO_ELF
#include "ao_elflib.h"
#endif

static stopres_t run_target PROTO((target_t *xp, rtype_t rtype));

#if AO_USE_PROCFS
#define ps_attach_to_process		procfs_attach_to_process
#define ps_kill				procfs_kill
#define ps_get_last_attach_pid		procfs_get_last_attach_pid
#define ps_continue			procfs_continue
#define ps_setreg			procfs_setreg
#endif


#if AO_USE_PTRACE
#define ps_attach_to_process		ptrace_attach_to_process
#define ps_kill				ptrace_kill
#define ps_get_last_attach_pid		ptrace_get_last_attach_pid
#define ps_continue			ptrace_continue
#define ps_setreg			ptrace_setreg
#endif

static bool Quit_on_exit = FALSE;

void
do_menu_target_command(command)
int command;
{
/* RGA For a Start don't call xp_start(), use xp_cont() instead
   so that the first breakpoint works */

	target_t *xp;
	bool target_updated = FALSE;
	stopres_t stopres = SR_FAILED;	
        /* RGA as per Callum Gibson callum@bain.oz.au */
	char *ptr = 0;
	long k = 0, mtime;
        char *pid_str;
        char last_pid_str[16];
	int last_pid = 0, fd, i, new_target_name = 0;

	xp = get_current_target();

	switch(command) {
	case MR_TGT_CONT:
		stopres = xp_cont(xp);
		check_for_dynamic_libs(xp, &stopres);
		break;
	case MR_TGT_STEP:
		stopres = xp_step(xp);
		break;
	case MR_TGT_NEXT:
		stopres = xp_next(xp);
		break;
	case MR_TGT_STOP:
		errf("The target is not currently running");
		stopres = SR_FAILED;	/* no redisplay */
		break;
	case MR_TGT_ATTACH:
		set_bm_cursor(WN_STDWIN, CU_MENU);
		last_pid = ps_get_last_attach_pid();
		sprintf(last_pid_str, "%d", last_pid);
		if (prompt_for_string("filename", "Enter PID of process: ",
				      last_pid_str, &pid_str) != 0)
		  return;
		k = strtol(pid_str, &ptr, 0);
				/* fall through */
	case MR_TGT_START:
	case MR_TGT_INIT:	/* when changing target name */
		if (xp->xp_target_name_changed == TRUE)
		{
		  target_updated = TRUE;
		  new_target_name = 1;
		  reset_pathlist((char *)xp->xp_textpath);
		}

		if (!new_target_name && target_updated == FALSE)
		{
		  if ((check_target_name(xp->xp_textpath, &fd, &i, &mtime, FALSE)) == 0)
		  {
		    close(fd);
		    target_updated = xp->xp_modtime != mtime ? TRUE : FALSE;
		  }
		  else
		    return;
		}

		if (target_updated == FALSE)
		{
		  if (command == MR_TGT_INIT)
		    return;
		  else
		    if (command == MR_TGT_START)
		    {
		      restore_target_state(TRUE, TRUE);
		      stopres = xp_cont(xp);
		      check_for_dynamic_libs(xp, &stopres);
		      break;
		    }
		}
		
		if (target_updated == TRUE)
		{
#ifdef AO_ELF
		  reset_main_debug_address();
#endif /* AO_ELF */
		  xp->xp_restore_bpts = TRUE;
		}
		if (command == MR_TGT_START || command == MR_TGT_INIT || (k && ptr))
		{
		  if (command == MR_TGT_ATTACH)
		  {
		    if (xp_get_state(xp) != TS_NOTR)
		    {
		      save_target_state(xp, FALSE, TRUE, FALSE);
		      kill_or_detach_from_target(xp);
		    }
		    if (ps_attach_to_process(xp, k) == -1)
		      return;
		    if (xp->xp_new_dynamic_libs)
		    {
		      target_updated = TRUE;
#ifdef AO_ELF
		      reset_main_debug_address();
#endif /* AO_ELF */
		    }
		    else
		      restore_target_state(FALSE, TRUE);
		  }
		  if (target_updated || new_target_name)
		  {
		    if (command == MR_TGT_ATTACH)
		    {
		      if (xp_get_state(xp) == TS_NOTR)
			save_target_state(xp, FALSE, TRUE, FALSE);
		      kill_or_detach_from_target(xp);
		      sprintf(last_pid_str, "%ld", k);
		      if ((attach_to_target(xp, last_pid_str, last_pid == 0,
					    target_updated, TRUE,
					    TRUE)) == -1)
			return;
		      restore_target_state(FALSE, TRUE);
		      xp = get_current_target();
		    }
		    else
		    {
		      if ((attach_to_target(xp, "core", last_pid == 0,
					    target_updated, FALSE,
					    TRUE)) == -1)
			return;
		      restore_target_state(FALSE, TRUE);
		      xp = get_current_target();
		      if (new_target_name || command == MR_TGT_INIT)
		      {
			func_t *f;

			if ((f = xp_get_mainfunc(xp)) == NULL)
			{
			  errf("Can't find main");
			  return;
			}
			if (xp_get_state(xp) == TS_NOTR && f->fu_fil != NULL &&
			    FU_LNOS(f) != NULL)
			  show_source(f->fu_fil, FU_LNOS(f)->ln_num);
			src_window_redraw();
		      }
		      else
		      {
			if (xp->xp_restore_bpts == TRUE)
			{
			  restore_target_state(FALSE, TRUE);
			  xp->xp_restore_bpts = FALSE;
			}
			stopres = xp_cont(xp);
			check_for_dynamic_libs(xp, &stopres);
		      }
		      break;
		    }
		  }
		}
		else
		  if (!k && ptr && *ptr)
		  {
		    errf("\bInvalid entry: use blank string to reattach to same target");
		    return;
		  }
		  else
		  {
		    if (ps_attach_to_process(xp, k) == -1)
		      return;
		    if (xp->xp_new_dynamic_libs)
		    {
		      save_target_state(xp, FALSE, TRUE, FALSE);
		      kill_or_detach_from_target(xp);
		      attach_to_target(xp, "0", last_pid == 0, FALSE, TRUE,
				       TRUE);
		      restore_target_state(FALSE, TRUE);
		      xp = get_current_target();
		    }
		    else
		      restore_target_state(FALSE, TRUE);
		  }
		stopres = SR_SIG;
		break;
	case MR_TGT_DETACH:
	  	save_target_state(xp, FALSE, TRUE, FALSE);
		kill_or_detach_from_target(xp);
		stopres = SR_DIED;
		break;
	case MR_TGT_KILL:
	  	save_target_state(xp, FALSE, TRUE, FALSE);
		xp_kill(xp);
		stopres = SR_DIED;
		break;
	case MR_TGT_DONT_KILL:
		return;
	case MR_TGT_RESTART:
		do_menu_target_command(MR_TGT_KILL);
		do_menu_target_command(MR_TGT_START);
		break;
	case MR_TGT_DONT_RESTART:
		return;
	default:
		panic("bad cmd in dmtc");
		stopres = SR_FAILED;	/* to satisfy gcc */
	}

	if (command == MR_TGT_ATTACH || new_target_name)
	{
#ifdef AO_ELF
	  rescan_init_file();
	  rescan_dynamic_solibs(get_current_target(), FALSE);
#endif
	}
	refresh_target_display(xp, stopres, command == MR_TGT_START ||
                               command == MR_TGT_RESTART ||
			       command == MR_TGT_ATTACH);

	/* suggest raise window if started anything */
	run_done(stopres != SR_FAILED);
}

stopres_t
dx_next(xp)
target_t *xp;
{
	return run_target(xp, RT_NEXT);
}

stopres_t
dx_step(xp)
target_t *xp;
{
	return run_target(xp, RT_STEP);
}

stopres_t
dx_cont(xp)
target_t *xp;
{
	return run_target(xp, RT_CONT);
}

static stopres_t
run_target(xp, rtype)
target_t *xp;
rtype_t rtype;
{
	stopres_t stopres;
	
/*	indicate_target_running(); RGA */
	update_target_menu_state(TS_RUNNING, xp_is_attached(xp)); /* RGA instead */
#ifdef KNOW_ASM
	stopres = dx_run_target(xp, rtype);
#else
	stopres = dx_run_target_ss(xp, rtype);
#endif
/*	indicate_target_stopped(xp_get_state(xp)); RGA*/
	update_target_menu_state(xp_get_state(xp), xp_is_attached(xp));/* RGA instead */

	return stopres;
}

void
exec_to_lnum(f, fil, lnum)
func_t *f;
fil_t *fil;
int lnum;
{
	target_t *xp;
	taddr_t addr;
	breakpoint_t *bp, *tmp_bp;
	bool just_started, have_process;
	tstate_t tstate;
	stopres_t stopres;
	bool target_updated = FALSE;
	int fd, i;
	long mtime;
	
	xp = get_current_target();

	tstate = xp_get_state(xp);

	if (tstate == TS_HALTED) {
		errf("Can't continue target");
		return;
	}

	have_process = target_process_exists(xp);
	just_started = tstate != TS_STOPPED;
	if (!have_process)	/* may be a new target */
	  if ((check_target_name(xp->xp_textpath, &fd, &i,
				 &mtime, FALSE)) == 0)
	  {
	    close(fd);
	    target_updated = xp->xp_modtime != mtime ? TRUE : FALSE;
	  }

	if (map_lnum_to_addr(f, fil, lnum, &addr) != 0)
		return;

	bp = xp_addr_to_breakpoint(xp, addr);
	if (bp != NULL)
		tmp_bp = NULL;
	else
		tmp_bp = bp = xp_add_breakpoint(xp, addr);

	if (xp_disable_breakpoint(xp, (breakpoint_t *)NULL) != 0 ||
	    xp_enable_breakpoint(xp, bp) != 0) {
		stopres = SR_FAILED;
	}
	else {
		taddr_t fp;
		bool same_func;

		if (have_process) {
			func_t *cur_f;
			taddr_t pc, adjpc;
			
			get_current_func(&cur_f, &fp, &pc, &adjpc);
			same_func = f == cur_f;
		}
		else {
			fp = 0;
			same_func = FALSE;
		}
			
		for (;;) {
		  if (!have_process)
		  {
		    if (target_updated == TRUE)
		    {
#ifdef AO_ELF
		      reset_main_debug_address();
#endif /* AO_ELF */
		      xp->xp_restore_bpts = TRUE;
		    }
		    if (target_updated)
		    {
		      if ((attach_to_target(xp, "core", TRUE,
					    target_updated, FALSE,
					    TRUE)) == -1)
			break;
		      restore_target_state(FALSE, TRUE);
		      xp = get_current_target();
		      if (xp->xp_restore_bpts == TRUE)
		      {
			restore_target_state(FALSE, TRUE);
			xp->xp_restore_bpts = FALSE;
		      }
		    }
		  }
		  if (!target_updated && !have_process)
		    restore_target_state(TRUE, TRUE);
		  stopres = have_process ? xp_cont(xp) : xp_start(xp);
		  have_process = target_process_exists(xp);

			if (stopres != SR_BPT)
				break;

			if (xp->xp_hit_solib_event == TRUE)
			{
			  check_for_dynamic_libs(xp, &stopres);
			  continue;
			}

			if (!same_func || xp_getreg(xp, UPSREG_FP) >= fp)
				break;
		} 
	}

	if (tmp_bp != NULL)
		xp_remove_breakpoint(xp, tmp_bp);

	xp_enable_breakpoint(xp, (breakpoint_t *)NULL);

	refresh_target_display(xp, stopres, just_started);
	run_done(stopres != SR_FAILED);
}

void
jump_to_lnum(f, fil, lnum)
func_t *f;
fil_t *fil;
int lnum;
{
	target_t *xp;
	taddr_t addr;
	breakpoint_t *bp, *tmp_bp;
	bool just_started, have_process;
	tstate_t tstate;
	stopres_t stopres;
	int orig_lnum, stops;
	fil_t *orig_fil;
	func_t *orig_f;
	taddr_t orig_fp;
	taddr_t orig_pc;
		
	xp = get_current_target();

	tstate = xp_get_state(xp);

	if (tstate == TS_HALTED) {
	  errf("Can't continue target");
	  return;
	}

	have_process = target_process_exists(xp);

	if (have_process == FALSE) {
	  errf("Target must be started in order to jump");
	  return;
	}

	stops = 0;
	just_started = tstate != TS_STOPPED;

	if (map_lnum_to_addr(f, fil, lnum, &addr) != 0)
	  return;

	get_current_func_2(&orig_f, &orig_fp, &orig_pc, &orig_fil, &orig_lnum);

loop:
	bp = xp_addr_to_breakpoint(xp, addr);
	if (bp != NULL)
	  tmp_bp = NULL;
	else
	  tmp_bp = bp = xp_add_breakpoint(xp, addr);

	if (xp_disable_breakpoint(xp, (breakpoint_t *)NULL) != 0 ||
	    xp_enable_breakpoint(xp, bp) != 0) {
		stopres = SR_FAILED;
	}
	else {
		taddr_t fp;
		bool same_func;
		func_t *cur_f;
		taddr_t pc, adjpc;
		
		get_current_func(&cur_f, &fp, &pc, &adjpc);
		same_func = f == cur_f;
		install_all_breakpoints(xp);
		update_target_menu_state(TS_RUNNING, xp_is_attached(xp));
			
		for (;;) {
		  stopres = have_process ? ps_continue(xp, addr, 0) :
		    xp_start(xp);
		  have_process = target_process_exists(xp);
		  
		  if (stopres == SR_USER)
		  {		
/* RGA return to original place if user presses stop */
/* if (map_lnum_to_addr(orig_f, orig_fil, orig_lnum, &addr) != 0)*/
/* return;*/
		    if (orig_f->fu_flags & FU_NOSYM)
		      stops++;
		    if (stops > 1)
		      break;
		    addr = orig_pc;
		    if (tmp_bp != NULL)
		      xp_remove_breakpoint(xp, tmp_bp);
		    f = orig_f;
		    goto loop;
		  }
		  if (stopres != SR_BPT)
		    break;
		  
		  if (!same_func || xp_getreg(xp, UPSREG_FP) >= fp)
		    break;
		} 
	}

	if (tmp_bp != NULL)
		xp_remove_breakpoint(xp, tmp_bp);

	xp_enable_breakpoint(xp, (breakpoint_t *)NULL);

	update_target_menu_state(TS_STOPPED, xp_is_attached(xp));
	refresh_target_display(xp, stopres, just_started);
}

void
refresh_target_display(xp, stopres, just_started)
target_t *xp;
stopres_t stopres;
bool just_started;
{
	bool old;
	objid_t obj_to_make_visible;

	if (stopres == SR_FAILED)
		return;

	save_var_display_state(just_started, FALSE);
	
	if (td_have_window())
		wn_updating_off(WN_STDWIN);
	old = td_set_obj_updating(OBJ_UPDATING_OFF);

	if (stopres == SR_DIED) {
		if (Quit_on_exit && just_started)
			re_set_exit_event_loop_flag();

		close_target_display();
		obj_to_make_visible = NULL;
	}
	else {
		if (just_started)
			restore_file_displays();
		
		obj_to_make_visible = rebuild_display(xp);
	}

	td_set_obj_updating(old);

	if (obj_to_make_visible != NULL)
		ensure_visible(obj_to_make_visible);

	if (td_have_window())
		wn_updating_on(WN_STDWIN);
}

/*  Start the target process running.
 */
stopres_t
dx_start(xp)
target_t *xp;
{
	const char **argv;
	long rdlist;
	stopres_t stopres;

	if (setup_args(&argv, &rdlist) != 0)
		return SR_DIED;

	reset_tty();

/*	indicate_target_running(); RGA */
	update_target_menu_state(TS_RUNNING, xp_is_attached(xp)); /* RGA instead */
	
	if (xp_create_child(xp, argv, get_environment(), rdlist,
			    &stopres) != 0)
		stopres = SR_DIED;

/*	indicate_target_stopped(xp_get_state(xp)); RGA */
	update_target_menu_state(xp_get_state(xp), xp_is_attached(xp));/* RGA instead */

	if (stopres == SR_DIED)
		return stopres;
	if (!(stopres == SR_BPT || stopres == SR_WPT))
		panic("xp not started properly in start_target");
			
	xp_set_base_sp(xp, xp_getreg(xp, UPSREG_SP));

	reinitialise_bpt_code_data();

	if (install_all_breakpoints(xp) != 0) {
		xp_kill(xp);
		return SR_DIED;
	}

	/*  If we have a breakpoint where the target is stopped, we
	 *  don't need to continue it.
	 */
	if (get_breakpoint_at_addr(xp, xp_getreg(xp, UPSREG_PC)) == NULL)
		stopres = xp_cont(xp);

	return stopres;
}

int
get_startup_stop_addrs(xp, p_main_addr, p_main_min_bpt_addr)
target_t *xp;
taddr_t *p_main_addr, *p_main_min_bpt_addr;
{
	func_t *mainfunc, *initfunc = 0;
	int ret;

	mainfunc = xp_get_mainfunc(xp);
	// We use the returned function to determine valid stack pointer addresses,
	// and _init() might be executed on a deeper stack than main. Having
	// the correct address for main() is more important than debugging
	// static initializers, for now.
	if (0 && mainfunc->fu_language == LANG_CC)
	  initfunc = xp_get_initfunc(xp);
	if (initfunc)
	{
	  *p_main_addr = initfunc->fu_addr;
	  ret = get_min_bpt_addr(initfunc, p_main_min_bpt_addr, FALSE);
	}
	else
	{
	  *p_main_addr = mainfunc->fu_addr;
	  ret = get_min_bpt_addr(mainfunc, p_main_min_bpt_addr, FALSE);
	}
	return ret;
}

stopres_t
xp_execto(xp, addr)
target_t *xp;
taddr_t addr;
{
	breakpoint_t *bp;
	int lastsig;
	const char *path;

	path = xp->xp_textpath;

	bp = xp_add_breakpoint(xp, addr);

	if (install_breakpoint(bp, xp, FALSE) != 0) {
		failmesg("Can't install initial breakpoint in", "object file",
			 path);

		if (xp_remove_breakpoint(xp, bp) != 0)
			panic("can't back out uninstalled bpt");

		if (!xp_is_attached(xp))
			xp_kill(xp);

		return SR_DIED;
	}

	lastsig = 0;
	do {
		xp_restart_child(xp, lastsig, CT_CONT);
	} while (xp_get_stopres(xp) == SR_SIG && xp_get_state(xp) == TS_STOPPED);

	if (xp_remove_breakpoint(xp, bp) != 0)
		panic("can't uninstall initial breakpoint");
	
	return xp_get_stopres(xp);
}

ci_exec_result_t
call_target_function(ma, addr, args, nwords, p_res, restype)
machine_t *ma;
taddr_t addr;
taddr_t *args;
int nwords;
taddr_t *p_res;
typecode_t restype;
{
	target_t *xp;
	int argno, retval, argslots, type_index;
	const char *mesg;
	size_t argsizes_buf[40], *argsizes;
#define MAX_ARGSIZES	(sizeof argsizes_buf / sizeof *argsizes_buf)

	xp = get_current_target();

	if (addr == STOP_ADDR)
		return STOP;
	if (addr == PRINTF_ADDR)
		return ups_printf(xp, ma, args, nwords);

	if (nwords > MAX_ARGSIZES)
		panic("nwords too large in ctf");

	type_index = nwords;
	argsizes = argsizes_buf;

	for (argno = 0; argno < nwords; argno += argslots, ++type_index) {
		type_t *type;
		int val, i;

		type = (type_t *)args[type_index];
		if (type == NULL) /* RGA avoid crash on parameters passed by value */
		{
		  val = 0;
		  argslots = 1;
		} else
		switch (type->ty_code) {
		case DT_PTR_TO:
			switch (type->ty_base->ty_code) {
			case TY_U_STRUCT:
			case TY_U_UNION:
				val = 0;
				break;
			default:
				val = ci_typesize((lexinfo_t *)NULL, type->ty_base);
				break;
			}
			argslots = 1;
			break;
		case DT_ARRAY_OF:
			val = ci_typesize((lexinfo_t *)NULL, type);
			argslots = 1;
			break;
		case TY_FLOAT:
			val = 0;
			argslots = sizeof(float) / sizeof(long);
			break;
		case TY_DOUBLE:
			val = 0;
			argslots = sizeof(double) / sizeof(long);
			break;
		default:
			val = 0;
			argslots = 1;
			break;
		}
		for (i = 0; i < argslots; i++)
			*argsizes++ = val;
	}

	retval = xp_call_func(xp, (char *)ma, addr,
			      args, argsizes_buf, nwords,
			      p_res, restype, &mesg);
	if (mesg != NULL) {
		func_t *f = addr_to_func(addr);

		if (f) 
			errf("Call of function %s failed: %s: %s",
			     f->fu_demangled_name, mesg, get_errno_str());
		else
			errf("Call of function at address 0x%lx failed: %s: %s",
			     addr, mesg, get_errno_str());
	}

	return (retval == 0) ? CI_ER_CONTINUE : CI_ER_INDIRECT_CALL_FAILED;
}

int
get_restart_sig(xp)
target_t *xp;
{
	int sig;
	
	sig = xp_get_lastsig(xp);
	return (sig != 0 && accept_signal(sig)) ? sig : 0;
}

void
check_for_dynamic_libs(xp, p_stopres)
     target_t *xp;
     stopres_t *p_stopres;
{
#ifdef AO_ELF
  if (*p_stopres == SR_BPT)
  {
    while (xp->xp_hit_solib_event == TRUE &&
	   can_install_all_breakpoints(xp) == TRUE)
    {
      rescan_dynamic_solibs(xp, FALSE);
      if (xp->xp_new_dynamic_libs_loaded == TRUE)
      {
#if AO_USE_PROCFS
	procfs_load_mmap_info(xp, FALSE);
#endif
#if AO_USE_PTRACE
#endif
	xp->xp_new_dynamic_libs_loaded = FALSE;
	xp->xp_hit_solib_event = FALSE;
      }
      *p_stopres = xp_cont(xp);
      if (*p_stopres != SR_BPT ||
	  user_wants_stop(TRUE)) /* RGA peek in queue */
      {
	xp->xp_new_dynamic_libs_loaded = FALSE;
	xp->xp_hit_solib_event = FALSE;
	break;
      }
    }
  }
#endif
}
void
check_for_dynamic_libs_sstep(xp, p_stopres)
     target_t *xp;
     stopres_t *p_stopres;
{
#ifdef AO_ELF
#if AO_USE_PROCFS
  elf_rescan_dynamic_solibs(xp, FALSE);
  if (xp->xp_new_dynamic_libs_loaded == TRUE &&
      can_install_all_breakpoints(xp) == TRUE)
  {
    procfs_load_mmap_info(xp, FALSE);
    xp->xp_new_dynamic_libs_loaded = FALSE;
    xp->xp_hit_solib_event = FALSE;
  }
#endif
#endif
}

int
check_target_name(textpath, p_fd, p_index, p_mtime, check_textfile)
const char *textpath;
int *p_fd, *p_index;
long *p_mtime;
bool check_textfile;
{
  int res;

  res = check_target(textpath, p_fd, p_index, p_mtime, check_textfile);
  if (res == 0 && check_textfile == TRUE)
  {
    target_t *xp;

    xp = get_current_target();
    if (strcmp(xp->xp_textpath, get_target_name()))
    {
      if (xp_get_state(xp) != TS_NOTR)
      {
	save_target_state(xp, FALSE, TRUE, FALSE);
	kill_or_detach_from_target(xp);
      }
      refresh_target_display(xp, SR_DIED, FALSE);
      xp->xp_target_name_changed = TRUE;
      xp->xp_textpath = (const char*)strdup(get_target_name());
    }
  }
  return res;
}

void
set_quit_on_exit(quit_on_exit)
bool quit_on_exit;
{
  Quit_on_exit = quit_on_exit;

  return;
}
