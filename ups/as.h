/* as.h - header file for the assembler routines */

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


/* @(#)as.h	1.5 12/22/93 (UKC) */

/*  Type of a jump.
 */
typedef enum jumptypeen {
	JT_BRANCH,
	JT_CALL,
	JT_END
} jumptype_t;

/*  An element in the list of jumps.
 */
typedef struct jump_s {
	jumptype_t ju_type;	/* call or branch */
	taddr_t ju_addr;	/* instruction addr - 0 for end of jump array */
	taddr_t ju_dstaddr;	/* target of jump - 0 if unknown */
	bool ju_unconditional;	/* is the jump unconditional? */
} jump_t;

/*  Utility routines provided for the machine specific assembler routines.
 */
const char *addr_to_func_and_offset PROTO((taddr_t addr, int allow_offset));
const char *hex PROTO((int n));

/*  Public routines in as_mi.c
 */
void dump_as_assembler PROTO((const char *funcname, int want_source_lines));
