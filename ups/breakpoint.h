/* breakpoint.h - public header file for bp.c */

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


/* @(#)breakpoint.h	1.6 16 Apr 1994 (UKC) */

#define BREAKPOINT_H_INCLUDED

/*  The following routines are used only by the dx implementation routines.
 */
int breakpoint_is_installed PROTO((breakpoint_t *bp));
void set_breakpoint_data PROTO((breakpoint_t *breakpoint, long));
long get_breakpoint_data PROTO((breakpoint_t *breakpoint));
int install_breakpoint PROTO((breakpoint_t *bp, target_t *xp, bool force));
int uninstall_breakpoint PROTO((breakpoint_t *bp));
int install_all_breakpoints PROTO((target_t *xp));
bool can_install_all_breakpoints PROTO((target_t *xp));
int uninstall_all_breakpoints PROTO((target_t *xp));
breakpoint_t *get_breakpoint_at_addr PROTO((target_t *xp, taddr_t addr));
void mark_breakpoints_as_uninstalled PROTO((target_t *xp));
void set_breakpoint_as_solib_event PROTO((breakpoint_t *bp));
bool breakpoint_is_solib_event PROTO((breakpoint_t *bp));
void install_solib_event_breakpoint PROTO((target_t *xp));

/*  FIX: this routine is called from recalculate_bpt_addrs() in obj_bpt.c
 */
void change_breakpoint_address PROTO((target_t *xp, breakpoint_t *bp,
				      taddr_t addr));

