/* ao_exec.h - generic interface to low level exec file info */

/*  Copyright 1995 Mark Russell, University of Kent at Canterbury.
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


/* @(#)ao_execinfo.h	1.1 24/5/95 (UKC) */

#define AO_EXECINFO_H_INCLUDED

typedef struct {
	taddr_t text_mem_addr;	/* Start addr in memory where text is loaded */
	size_t text_size;	/* #bytes of text */
	off_t text_addr_delta;	/* What to add to fu_addr etc to get mem addr */
	off_t debug_addr_delta; /* Delta from debug addresses to real addresses */

	off_t addr_to_fpos_offset; /* Delta from mem addr to file offset  */
	
	off_t file_syms_offset;
	int nsyms;
	off_t file_symstrings_offset;
	
	bool dynamic;		/* Is SunOS 4 shared lib (BUG: lose this) */

	taddr_t entry_addr;	/* Program entry point virtual address */
} Execinfo;
