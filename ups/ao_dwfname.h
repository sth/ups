/* ao_dwfname.h - routine definitions for ao_dwfname.c */

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

int		find_types_by_name PROTO((void));
int		set_find_types_by_name PROTO((int val));

#if WANT_DWARF

#ifdef DWARF_DEBUG_TYPEDEFED

type_t *	dwf_find_alt_type_defn PROTO((Dwarf_Debug dbg, Dwarf_Die die,
					      stf_t *stf));

typename_t *	dwf_save_typename PROTO((Dwarf_Debug dbg, Dwarf_Die die,
					 symtab_t *st, stf_t *stf));
typename_t *	dwf_find_typename PROTO((symtab_t *st, stf_t *stf,
					 const char *name));

#endif /* DWARF_DEBUG_TYPEDEFED */

#endif /* WANT_DWARF */
