/* bp.c - low level breakpoint list handling */

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


/* @(#)bp.c	1.21 04 Jun 1995 (UKC) */
char ups_bp_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <stdlib.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "breakpoint.h"
#include "dx.h"

struct breakpoint_s {
	struct breakpoint_s *bp_next;	/* next entry in the list */
	struct breakpoint_s *bp_prev;	/* previous entry in the list */
	target_t *bp_xp;	/* process that bpt is (will be) inserted in */
	bool bp_installed;	/* installed (or would be if enabled) */
	bool bp_enabled;	/* is breakpoint active? */
	taddr_t bp_addr;	/* the text address of the breakpoint */
	xp_opcode_t bp_code;	/* the opcode that has been replaced */
	long bp_user_data;	/* user data - not used by us */
	bool bp_solib_event;	/* is breakpoint in dynamic linker ?*/
};

static int set_enabled PROTO((target_t *xp, breakpoint_t *bp, bool enabled));

static breakpoint_t Bphead = { &Bphead, &Bphead };

breakpoint_t *
dx_add_breakpoint(xp, addr)
target_t *xp;
taddr_t addr;
{
	breakpoint_t *bp;

	bp = (breakpoint_t *)e_malloc(sizeof(breakpoint_t));

	bp->bp_next = Bphead.bp_next;
	bp->bp_prev = &Bphead;
	Bphead.bp_next->bp_prev = bp;
	Bphead.bp_next = bp;

	bp->bp_xp = 0;
	bp->bp_enabled = TRUE;
	bp->bp_installed = FALSE;
	bp->bp_addr = addr;
	bp->bp_user_data = 0;
	bp->bp_solib_event = FALSE;
	return bp;
}

void
set_breakpoint_data(breakpoint, data)
breakpoint_t *breakpoint;
long data;
{
	((breakpoint_t *)breakpoint)->bp_user_data = data;
}

long
get_breakpoint_data(breakpoint)
breakpoint_t *breakpoint;
{
	return ((breakpoint_t *)breakpoint)->bp_user_data;
}

/*  Remove a breakpoint. Uninstall it first if necessary.
 */
int
dx_remove_breakpoint(xp, bp)
target_t *xp;
breakpoint_t *bp;
{
	if (bp->bp_xp != 0)
		if (uninstall_breakpoint(bp) != 0)
			return -1;

	bp->bp_prev->bp_next = bp->bp_next;
	bp->bp_next->bp_prev = bp->bp_prev;

	free((char *)bp);

	return 0;
}

/*  Return non zero if breakpoint is installed.
 */
int
breakpoint_is_installed(bp)
breakpoint_t *bp;
{
	return bp->bp_xp != 0;
}

/*  Install a breakpoint (e.g. write TRAP opcode into target).
 */
int
install_breakpoint(bp, xp)
breakpoint_t *bp;
target_t *xp;
{
	if (bp->bp_user_data && breakpoint_is_disabled(bp->bp_user_data))
	  return 0;

	if (bp->bp_enabled)
	{
	  if (xp_tswap(xp, bp->bp_addr, (xp_opcode_t)0,
		       &bp->bp_code) != 0)
	  {
	    errf("\bCan't install breakpoint: %s", get_errno_str());
	    /* RGA	    return -1;*/
	  }
	  
	  bp->bp_xp = xp;
	}

	bp->bp_installed = TRUE;
	
	return 0;
}

/*  Uninstall a breakpoint.
 */
int
uninstall_breakpoint(bp)
breakpoint_t *bp;
{
	if (bp->bp_enabled && bp->bp_xp != 0) {
		if (xp_tswap(bp->bp_xp, bp->bp_addr, bp->bp_code,
			     (xp_opcode_t *)NULL) != 0) {
			errf("\bCan't uninstall breakpoint: %s", get_errno_str());
	    /* RGA	    return -1;*/
		}
		
		bp->bp_xp = 0;
	}

	bp->bp_installed = FALSE;
	
	return 0;
}

void
change_breakpoint_address(xp, bp, addr)
target_t *xp;
breakpoint_t *bp;
taddr_t addr;
{
	if (bp->bp_xp != 0)
		panic("cbp called on installed breakpoint");
	
	bp->bp_addr = addr;
}

int
dx_enable_breakpoint(xp, bp)
target_t *xp;
breakpoint_t *bp;
{
	return set_enabled(xp, bp, TRUE);
}

int
dx_disable_breakpoint(xp, bp)
target_t *xp;
breakpoint_t *bp;
{
	return set_enabled(xp, bp, FALSE);
}

static int
set_enabled(xp, bp, enabled)
target_t *xp;
breakpoint_t *bp;
bool enabled;
{
	if (bp != NULL) {
		if (enabled) {
			bp->bp_enabled = TRUE;

			if (bp->bp_xp == 0 && bp->bp_installed) {
				if (install_breakpoint(bp, xp) != 0)
					return -1;
			}
		}
		else {
			if (bp->bp_xp != 0)
				if (uninstall_breakpoint(bp) != 0)
					return -1;

			bp->bp_enabled = FALSE;
		}
	}
	else {
		for (bp = Bphead.bp_next; bp != &Bphead; bp = bp->bp_next) {
			if (set_enabled(xp, bp, enabled) != 0)
				return -1;
		}
	}

	return 0;
}

/*  Install the solib breakpoints if it exists.
 */
void
install_solib_event_breakpoint(xp)
target_t *xp;
{
	breakpoint_t *bp;

	for (bp = Bphead.bp_next; bp != &Bphead; bp = bp->bp_next) {
		if (bp->bp_solib_event == TRUE && bp->bp_installed == FALSE)
		{
		  install_breakpoint(bp, xp);
		  return;
		}
	}

	return;
}

/*  Uninstall any breakpoints installed in xp.
 */
int
uninstall_all_breakpoints(xp)
target_t *xp;
{
	breakpoint_t *bp;

	for (bp = Bphead.bp_next; bp != &Bphead; bp = bp->bp_next) {
		if (bp->bp_xp == xp)
			uninstall_breakpoint(bp);
/* RGA			if (uninstall_breakpoint(bp))*/
/*				return -1;*/
	}

	return 0;
}

/*  Install any uninstalled breakpoints.
 */
int
install_all_breakpoints(xp)
target_t *xp;
{
	breakpoint_t *bp;

	for (bp = Bphead.bp_next; bp != &Bphead; bp = bp->bp_next) {
		if (bp->bp_xp == 0)
			install_breakpoint(bp, xp);
/* RGA			if (install_breakpoint(bp, xp))*/
/*				return -1;*/
	}
	
	return 0;
}

/*  Test for installing all uninstalled breakpoints.
 */
bool
can_install_all_breakpoints(xp)
target_t *xp;
{
	breakpoint_t *bp;

	for (bp = Bphead.bp_next; bp != &Bphead; bp = bp->bp_next) {
		if (bp->bp_xp == 0)
		  if (xp_tswap(xp, bp->bp_addr, (xp_opcode_t)0,
			       &bp->bp_code) != 0) 
				return FALSE;
		else
		{		/* uninstall */
		  if (xp_tswap(xp, bp->bp_addr, bp->bp_code,
			       (xp_opcode_t *)NULL) != 0) 
		    errf("Can't test uninstall breakpoint: %s", get_errno_str());
		}
	}
	
	return TRUE;
}

/*  If there is a breakpoint installed at addr in xp, return the breakpoint.
 *  Otherwise, return 0.
 */
breakpoint_t *
get_breakpoint_at_addr(xp, addr)
target_t *xp;
taddr_t addr;
{
	breakpoint_t *bp;

	for (bp = Bphead.bp_next; bp != &Bphead; bp = bp->bp_next)
		if (bp->bp_xp == xp && bp->bp_addr == addr)
			return bp;

	return NULL;
}

/*  Return the breakpoint corresponding to address addr, even if breakpoint
 *  is not currently installed.
 *
 *  If there is no breakpoint at addr, return 0.
 */
breakpoint_t *
dx_addr_to_breakpoint(xp, addr)
target_t *xp;
taddr_t addr;
{
	breakpoint_t *bp;

	for (bp = Bphead.bp_next; bp != &Bphead; bp = bp->bp_next)
		if (bp->bp_addr == addr)
			return bp;

	return NULL;
}

taddr_t
dx_get_breakpoint_addr(xp, bp)
target_t *xp;
breakpoint_t *bp;
{
	return bp->bp_addr;
}

/*  Run through the breakpoint list marking breakpoints as not installed.
 *  Called when the target process exits.
 */
void
mark_breakpoints_as_uninstalled(xp)
target_t *xp;
{
	breakpoint_t *bp;

	for (bp = Bphead.bp_next; bp != &Bphead; bp = bp->bp_next) {
		if (bp->bp_installed) {
			bp->bp_xp = 0;
			bp->bp_installed = FALSE;
		}
	}
}

void
set_breakpoint_as_solib_event(bp)
breakpoint_t *bp;
{
	bp->bp_solib_event = TRUE;
}

bool
breakpoint_is_solib_event(bp)
breakpoint_t *bp;
{
	return bp->bp_solib_event;
}
