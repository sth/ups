/* dx.h - prototypes for routines common to ao_* and xc_* target types */

/*  Copyright 1994 Mark Russell, University of Kent at Canterbury.
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


/* @(#)dx.h	1.1 16 Apr 1994 (UKC) */

/*  These routines are an implementation of the high level process control
 *  routines using low level routines provided by both the C interpreter
 *  and the native a.out file back ends.
 */

/*  Process control
 */
stopres_t dx_start PROTO((target_t *xp));
stopres_t dx_step PROTO((target_t *xp));
stopres_t dx_next PROTO((target_t *xp));
stopres_t dx_cont PROTO((target_t *xp));

/*  Breakpoints.
 */
breakpoint_t *dx_add_breakpoint PROTO((target_t *xp, taddr_t addr));
int dx_remove_breakpoint PROTO((target_t *xp, breakpoint_t *bp));
int dx_enable_breakpoint PROTO((target_t *xp, breakpoint_t *bp));
int dx_disable_breakpoint PROTO((target_t *xp, breakpoint_t *bp));
breakpoint_t *dx_addr_to_breakpoint PROTO((target_t *xp, taddr_t addr));
taddr_t dx_get_breakpoint_addr PROTO((target_t *xp, breakpoint_t *bp));

/*  Watchpoints.
 */
watchpoint_t *dx_add_watchpoint PROTO((target_t *xp, taddr_t addr,
				       size_t size));
int dx_remove_watchpoint PROTO((target_t *xp, watchpoint_t *wp));
void dx_enable_watchpoint PROTO((target_t *xp, watchpoint_t *wp));
void dx_disable_watchpoint PROTO((target_t *xp, watchpoint_t *wp));
