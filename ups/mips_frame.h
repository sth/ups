/* mips_frame.h - MIPS stack frame info */

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


/* @(#)mips_frame.h	1.2 4/7/91 (UKC) */

/*  Information about the MIPS virtual stack frame.
 *
 *  These fields are copied from the PDR (see <syms.h>)
 */
typedef struct framest {
	long fr_frame_size;		/* Stack frame size */
	long fr_reg_offset;		/* Offset from fp to registers */
	long fr_reg_mask;		/* Saved integer registers mask */
	short fr_pcreg;			/* Register used to save the pc */
	short fr_spreg;			/* Register used for sp (usually r29) */
} frame_t;

#define MIPS_SP_REGNO		29	/* r29 is used as the sp on the MIPS */
#define MIPS_SAVEDPC_REGNO	31	/* r31 is used to save the pc */

#ifdef SYMTAB_H_INCLUDED
frame_t *get_frame_info PROTO((func_t *f));
#endif

int get_frame_size PROTO((unsigned long addr));
