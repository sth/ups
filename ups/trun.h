/* trun.h - target control header file */

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


/* @(#)trun.h	1.9 16 Apr 1994 (UKC) */

typedef enum rtypeen {
	RT_STEP,
	RT_NEXT,
	RT_CONT
} rtype_t;

/*  Define KNOW_ASM if we know the assembler for this machine.
 */
#if defined(ARCH_VAX) || defined(ARCH_SUN3) || defined(ARCH_MIPS) || defined(ARCH_CLIPPER) || defined(ARCH_SUN4)
#define KNOW_ASM
#endif

/*  Set FORGET_ASM to make ups forget any knowledge of assembler it might
 *  have had.
 */
#ifdef FORGET_ASM
#undef KNOW_ASM
#endif

#ifdef TARGET_H_INCLUDED
stopres_t dx_run_target PROTO((target_t *xp, rtype_t rtype));
stopres_t dx_run_target_ss PROTO((target_t *xp, rtype_t rtype));
#endif
