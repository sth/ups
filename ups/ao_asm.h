/* ao_asm.h - public header file for ao_asm.c */

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


/* @(#)ao_asm.h	1.1 22/12/93 (UKC) */

jump_t *get_asm_jumps PROTO((taddr_t addr, const char *text, size_t len,
							bool want_calls));
taddr_t get_next_pc PROTO((target_t *xp, taddr_t pc));

const char *disassemble_one_instruction PROTO((taddr_t addr, const char *text,
							   const char **p_buf));

