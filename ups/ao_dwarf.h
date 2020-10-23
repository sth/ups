/* ao_dwarf.h - DWARF symbol table definitions */

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

#include <libelf.h>
#if HAVE_LIBDWARF_LIBDWARF_H
#include <libdwarf/libdwarf.h>
#include <libdwarf/dwarf.h>
#else
#include <libdwarf.h>
#include <dwarf.h>
#endif

void	dwf_handle_error (Dwarf_Error *err, Dwarf_Ptr errarg);
void	dwf_error (char *msg, int dwarf_code, Dwarf_Die die, Dwarf_Error err);
void	dwf_fatal_error (char *msg, int dwarf_code, Dwarf_Die die, Dwarf_Error err);

bool	dwf_count_globals (Dwarf_Debug dbg, int *count);

Dwarf_Die	dwf_die_at_offset(Dwarf_Debug dbg, Dwarf_Off offset);
Dwarf_Die	dwf_die_at_attribute(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id);
Dwarf_Off	dwf_offset_of_die(Dwarf_Debug dbg, Dwarf_Die die);
Dwarf_Off	dwf_cu_offset_of_die(Dwarf_Debug dbg, Dwarf_Die die);

Dwarf_Die	dwf_cu_die(Dwarf_Debug dbg);
Dwarf_Die	dwf_next_die(Dwarf_Debug dbg, Dwarf_Die die);
Dwarf_Die	dwf_child_die(Dwarf_Debug dbg, Dwarf_Die die);
#define DW_STF_CU_DIE(stf) dwf_die_at_offset((stf)->stf_dw_dbg, (stf)->stf_cu_die_offset)

bool		dwf_has_attribute(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id);

Dwarf_Half	dwf_get_tag(Dwarf_Debug dbg, Dwarf_Die die);
char *		dwf_get_string(Dwarf_Debug dbg, alloc_pool_t *ap, Dwarf_Die die, Dwarf_Half id);
bool		dwf_get_name(Dwarf_Debug dbg, alloc_pool_t *ap, Dwarf_Die die,
			     char **p_name, char **p_mangled);
Dwarf_Addr	dwf_get_address(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id, Dwarf_Addr base);
Dwarf_Off	dwf_get_offset(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id);
Dwarf_Off	dwf_get_cu_ref(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id);
Dwarf_Unsigned	dwf_get_udata(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id);
Dwarf_Signed	dwf_get_sdata(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id);
Dwarf_Signed	dwf_get_number(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id);
bool		dwf_get_flag(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id);
bool		dwf_get_opt_flag(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id);
Dwarf_Locdesc **dwf_get_loclist(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id, Dwarf_Signed *p_len);

#else

typedef char * Dwarf_Debug;
typedef char * Dwarf_Error;
typedef char * Dwarf_Die;

#endif /* WANT_DWARF */

#define DWARF_DEBUG_TYPEDEFED 1

