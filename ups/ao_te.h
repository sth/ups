/* ao_te.h - public header file for ao_te.c */

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


/* @(#)ao_te.h	1.1 22/12/93 (UKC) */

#ifdef ST_TE
int skim_te_symtab PROTO((symtab_t *st, int fd,
			  off_t syms_offset, off_t addr_to_fpos_offset,
			  taddr_t first_addr, taddr_t last_addr,
			  block_t *rootblock,
			  bool *p_have_common_blocks, fil_t **p_sfiles,
			  functab_id_t *p_functab_id, cblist_id_t *p_cblist_id));
#endif
