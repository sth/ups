/* ao_stack.h - public header file for ao_stack.c */

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


/* @(#)ao_stack.h	1.1 22/12/93 (UKC) */

Stack *ao_get_stack_trace PROTO((target_t *xp));
taddr_t ao_get_reg_addr PROTO((target_t *xp, Stack *stk, int reg));
const char *ao_get_signal_tag PROTO((target_t *xp, int signo,
				     const siginfo_t *siginfo));
