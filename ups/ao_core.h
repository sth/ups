/* ao_core.h - header file for ao_core.c */

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


/* @(#)ao_core.h	1.3 24 May 1995 (UKC) */

#define AO_CORE_H_INCLUDED

typedef struct {
	taddr_t base;
	taddr_t lim;
	off_t file_offset;
} Core_segment;

typedef struct Coredesc Coredesc;

bool open_corefile PROTO((alloc_pool_t *ap, const char *corename,
		         const char *textname, bool user_gave_core,
			 taddr_t data_addr, Coredesc **p_co, int *p_lastsig));

bool core_dread PROTO((Coredesc *co, taddr_t addr, voidptr buf, size_t nbytes));
bool core_read PROTO((Coredesc *co, off_t offset, voidptr buf, size_t nbytes));
bool core_readstr PROTO((Coredesc *co, taddr_t addr, char *buf, size_t buflen));

char *core_getregs PROTO((Coredesc *co));
const char *core_get_cmdline PROTO((Coredesc *co));
