/* ao_dwfname.c - functions for the typename list */

/*  Copyright 2002 Ian Edwards
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

char ups_ao_dwfname_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#if WANT_DWARF

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <mtrprog/hash.h>
#include <limits.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "st.h"
#include "ao_dwarf.h"
#include "ao_syms.h"
#include "ao_dwftype.h"
#include "ao_dwfname.h"
#include "ao_dwfutil.h"


/*
 * Given a type declaration find its definition.
 *
 * A class/struct/union/enum type may only have a declaration in a CU and
 * no definition.  In UPS this will show up as e.g. "undefined struct xyz".
 * With C++ classes users really do want to be able to look at the members.
 * As there is no DWARF info to link the declaration back to the definition
 * in a different CU we have to go by the type (i.e. class) name.
 *
 * Where this occurs it seemed easiest just to go off and do the types in the
 * 'other' CU rather than extending the lookup/save/fixup done for each CU.
 *
 * We hope that duplicate names just means that the definition appears in
 * more than one CU.  This is not necessarily so, but preferring a definition
 * in the current 'stf' before any other helps.
 */
type_t *
dwf_find_alt_type_defn(dbg, die, stf)
Dwarf_Debug dbg;
Dwarf_Die die;
stf_t *stf;
{
    Dwarf_Die spec_die;
    char *name;
    typename_t *tn;
    symtab_t *st;
    alloc_pool_t *ap;

    if (!find_types_by_name())
	return (type_t *)NULL;

    /*
     * This DIE is a definition, return NULL so caller will
     * process it.
     */
    if (! dwf_has_attribute(dbg, die, DW_AT_declaration))
	return (type_t *)NULL;

    st = stf->stf_symtab;
    ap = st->st_apool;

    /*
     * Do we have a 'typename' entry for this type ?
     */
    spec_die = dwf_find_spec_die(dbg, die);
    if (! dwf_has_attribute(dbg, spec_die, DW_AT_name))
	return (type_t *)NULL;
    name = dwf_get_string(dbg, ap, spec_die, DW_AT_name);
    if ((tn = dwf_find_typename(st, stf, name)) == NULL)
	return (type_t *)NULL;

    /*
     * Load the types of the other CU.
     */
    if (stf != tn->tn_stf) {
	char *lib;
	bool load = TRUE;

	if (*(tn->tn_stf->stf_name) != '/') {
	    lib = normalise_path(strf("%s/%s", tn->tn_stf->stf_objpath_hint, tn->tn_stf->stf_name));
	    if (lib)
		load = user_wants_library_loaded(lib);
	    free(lib);
	}
	else
	    load = user_wants_library_loaded((char *)tn->tn_stf->stf_name);
	if (load)
	    dwf_do_cu_types(st, tn->tn_stf);
    }

    /*
     * Look for the type in the other CU.
     */
    return dwf_find_type(tn->tn_stf, tn->tn_offset);
}

/*
 * Make a 'type name' and save it.
 */
typename_t *
dwf_save_typename(dbg, die, st, stf)
Dwarf_Debug dbg;
Dwarf_Die die;
symtab_t *st;
stf_t *stf;
{
    typename_t *tn, *prev_tn;
    hashtab_t *ht;
    char *name;
    alloc_pool_t *ap;

    if ((ht = AO_STDATA(st)->st_type_names) == NULL)
	return (typename_t *)NULL;
    if (! dwf_has_attribute(dbg, die, DW_AT_name))
	return (typename_t *)NULL;
    ap = st->st_apool;

    /*
     * If type already defined elsewhere then add to list on '->tn_next'.
     */
    name = dwf_get_string(dbg, ap, die, DW_AT_name);
    if ((prev_tn = dwf_find_typename(st, stf, name)) != NULL) {
	if (prev_tn->tn_stf == stf) {
	    errf("Already saved typename %s in this CU <%ld>",
		 name, (long)stf->stf_cu_hdr_offset);
	    return prev_tn;
	}
    }

    /*
     * dwarfTODO: perhaps dwf_offset_of_die() would be better ?
     */
    tn = (typename_t *)alloc(ap, sizeof(typename_t));
    tn->tn_offset = dwf_cu_offset_of_die (dbg, die);
    tn->tn_name  = dwf_get_string(dbg, ap, die, DW_AT_name);
    tn->tn_stf = stf;
    if (prev_tn) {
	tn->tn_next = prev_tn->tn_next;
	prev_tn->tn_next = tn;
    } else {
	tn->tn_next = NULL;
	hash_enter(ht, name, strlen(name), (hash_value_t)tn);
    }
    return tn;
}

/*
 * Find a 'typename' given a name.  Prefer the current 'stf' to others.
 */
typename_t *
dwf_find_typename(st, stf, name)
symtab_t *st;
stf_t *stf;
const char *name;
{
    hashtab_t *ht;
    typename_t *tn, *tn2;

    if ((ht = AO_STDATA(st)->st_type_names) == NULL)
	return (typename_t *)NULL;
    if ((tn = (typename_t *)hash_lookup(ht, name, strlen(name))) == NULL)
	return tn;
    tn2 = tn;
    while (tn2 != NULL) {
	if (tn2->tn_stf == stf)
	    return tn2;
	tn2 = tn2->tn_next;
    }
    return tn;
}

/*
 * Query and optionally set 'resolve_by_name' flag.
 */
static int
dwf_types_by_name(int which)
{
    static int resolve_by_name = TRUE;

    if (which == 0)
	resolve_by_name = FALSE;
    else if (which == 1)
	resolve_by_name = TRUE;
    return resolve_by_name;
}

/*
 * Return value of 'resolve_by_name' flag.
 */
int
find_types_by_name()
{
    return dwf_types_by_name(-1);
}

/*
 * Set the 'resolve_by_name' flag FALSE if 'val' is zero, otherwise TRUE
 */
int
set_find_types_by_name(int val)
{
    return dwf_types_by_name(val ? 1 : 0);
}

#endif /* WANT_DWARF */

