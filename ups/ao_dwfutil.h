/* ao_dwfutil.h - routine definitions for ao_dwfutil.c */

/*  Copyright (C) 2002 Ian Edwards
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

/* $Id$ */

#if WANT_DWARF

char *		normalise_path PROTO((char *path));

Dwarf_Die	dwf_find_spec_die PROTO((Dwarf_Debug dbg, Dwarf_Die die));
Dwarf_Die	dwf_stf_cu_die PROTO((stf_t *stf));

language_t	dwf_get_src_language PROTO((Dwarf_Debug dbg, Dwarf_Die cu_die));
Compiler_type	dwf_get_compiler_type PROTO((Dwarf_Debug dbg, Dwarf_Die cu_die));
void		dwf_get_cu_files PROTO((Dwarf_Debug dbg, Dwarf_Die cu_die,
					alloc_pool_t *ap, stf_t *stf));
stf_t *		dwf_lookup_file_by_fnum PROTO((stf_t *stf, int fnum));
stf_t *		dwf_lookup_file_by_name PROTO((stf_t *stf, char *name));
stf_t *		dwf_lookup_file_by_die PROTO((stf_t *stf, Dwarf_Die die));

func_t *	dwf_lookup_func_by_addr PROTO((stf_t *stf, taddr_t addr));

lexinfo_t *	dwf_make_lexinfo PROTO((Dwarf_Debug dbg, Dwarf_Die die,
					alloc_pool_t *ap, stf_t *stf));

vaddr_t *	dwf_get_location PROTO((Dwarf_Debug dbg, alloc_pool_t *ap,
					Dwarf_Die die, Dwarf_Half id));

#endif /* WANT_DWARF */
