/* ao_dwarf.c - functions connecting UPS to 'libdwarf' */

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

char ups_ao_dwarf_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#if WANT_DWARF
#include <local/ukcprog.h>
#include <limits.h>
#include <stdlib.h>

#include "ao_dwarf.h"

/*
 * Create a DWARF error message string.
 */
static char *
dwf_error_msg (char *msg, int dwarf_code, Dwarf_Die die, Dwarf_Error err)
{
    char *die_txt;
    char *die_name = NULL;
    Dwarf_Off offset = 0;
    Dwarf_Off rel_offset = 0;
    Dwarf_Error dummy;

    if (die != (Dwarf_Die)0) {
	dwarf_diename(die, &die_name, &dummy);
	dwarf_dieoffset(die, &offset, &dummy);
	dwarf_die_CU_offset(die, &rel_offset, &dummy);
    }
    if (die_name == NULL)
	die_txt = strf("(DIE <%ld><%ld>)", (long)offset, (long)rel_offset);
    else
	die_txt = strf("(DIE %s <%ld><%ld>)", die_name, (long)offset, (long)rel_offset);
    if (dwarf_code == DW_DLV_ERROR) {
	char * errmsg = dwarf_errmsg(err);
	Dwarf_Unsigned myerr = dwarf_errno(err);
	return strf("\bDWARF error : %s - %s %s (%ld)", msg, die_txt, errmsg, (long)myerr);
    }

    if (dwarf_code == DW_DLV_NO_ENTRY)
	return strf("\bDWARF error :  %s - no entry %s", msg, die_txt);

    if (dwarf_code == DW_DLV_OK)
	return strf("\bDWARF error : %s - OK %s", msg, die_txt);

    /* 'dwarf_code' not valid */
    return strf("\bDWARF error :  %s code = %d", msg, die_txt, dwarf_code);
}

/*
 * Error handler for 'libdwarf'
 *
 * This does not seem to get called, possibly because individual calls
 * all pass an address for an error descriptor (Dwarf_Error).
 */
void
dwf_handle_error (err, errarg)
Dwarf_Error *err;
Dwarf_Ptr errarg;
{
    char *s = dwf_error_msg("", DW_DLV_ERROR, NULL, *err);
    panic(s);
}

/*
 * Report a bad return from a libdwarf call.
 */
void
dwf_error (char *msg, int dwarf_code, Dwarf_Die die, Dwarf_Error err)
{
    char *s = dwf_error_msg(msg, dwarf_code, die, err);
    errf(s);
    free(s);
}

/*
 * Report a bad return from a libdwarf call then quit.
 *
 * dwarfTODO: don't quit for now
 */
void
dwf_fatal_error (char *msg, int dwarf_code, Dwarf_Die die, Dwarf_Error err)
{
    char *s = dwf_error_msg(msg, dwarf_code, die, err);
    errf(s);
#if WANT_DEBUG
    fprintf(stderr, "%s", s);
#endif
    free(s);
}

/*
 * Get the DIE of the current compilation unit.
 *
 * Unless you are inside a dwarf_next_cu_header() loop you should be
 * calling dwf_stf_cu_die() (in ao_dwfsyms.c).
 */
Dwarf_Die
dwf_cu_die(Dwarf_Debug dbg)
{
    int rv;
    Dwarf_Die cu_die = 0;
    Dwarf_Error err;

    if ((rv = dwarf_siblingof(dbg, NULL, &cu_die, &err)) != DW_DLV_OK)
	dwf_fatal_error("dwarf_siblingof(0)", rv, NULL, err);
    return cu_die;
}

/*
 * Get the next DIE at this level after 'die' ("sibling" in libdwarf terms)
 *
 * If 'die' is 0 return the first DIE in the current compilation unit,
 * but for this you should be calling dwf_cu_die().
 *
 * Returns 0 if there isn't a next/first DIE.
 */
Dwarf_Die
dwf_next_die(Dwarf_Debug dbg, Dwarf_Die die)
{
    int rv;
    Dwarf_Die next_die;
    Dwarf_Error err;

    if ((rv = dwarf_siblingof(dbg, die, &next_die, &err)) == DW_DLV_OK)
	return next_die;
    else if (rv == DW_DLV_NO_ENTRY)
	return (Dwarf_Die)0;
    dwf_fatal_error("dwarf_siblingof(die)", rv, die, err);
    return (Dwarf_Die)0;
}

/*
 * Get the first child of a DIE.
 * Returns 0 if there isn't a child DIE.
 */
Dwarf_Die
dwf_child_die(Dwarf_Debug dbg, Dwarf_Die die)
{
    int rv;
    Dwarf_Die child_die;
    Dwarf_Error err;

    if ((rv = dwarf_child(die, &child_die, &err)) == DW_DLV_OK)
	return child_die;
    else if (rv == DW_DLV_NO_ENTRY)
	return (Dwarf_Die)0;
    dwf_fatal_error("dwarf_child", rv, die, err);
    return (Dwarf_Die)0;
}

/*
 * Return the DIE at the given offset.
 */
Dwarf_Die
dwf_die_at_offset(Dwarf_Debug dbg, Dwarf_Off offset)
{
    int rv;
    Dwarf_Die die = 0;
    Dwarf_Error err;

    if ((rv = dwarf_offdie(dbg, offset, &die, &err)) != DW_DLV_OK)
	dwf_fatal_error("dwarf_offdie", rv, NULL, err);
    return die;
}

/*
 * Return the DIE referenced by the attribute 'id' in 'die'.
 */
Dwarf_Die
dwf_die_at_attribute(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id)
{
    Dwarf_Off offset;

    offset = dwf_get_offset(dbg, die, id);
    return dwf_die_at_offset(dbg, offset);
}

/*
 * Return the offset of a DIE.
 */
Dwarf_Off
dwf_offset_of_die(Dwarf_Debug dbg, Dwarf_Die die)
{
    int rv;
    Dwarf_Off offset = 0;
    Dwarf_Error err;

    if ((rv = dwarf_dieoffset(die, &offset, &err)) != DW_DLV_OK)
	dwf_fatal_error("dwarf_dieoffset", rv, die, err);
    return offset;
}

/*
 * Return the CU-relative offset of a DIE.
 */
Dwarf_Off
dwf_cu_offset_of_die(Dwarf_Debug dbg, Dwarf_Die die)
{
    int rv;
    Dwarf_Off offset = 0;
    Dwarf_Error err;

    if ((rv = dwarf_die_CU_offset(die, &offset, &err)) != DW_DLV_OK)
	dwf_fatal_error("dwarf_dieoffset", rv, die, err);
    return offset;
}

/*
 * Return the tag, e.g. DW_TAG_subprogram, from a DIE.
 */
Dwarf_Half
dwf_get_tag(Dwarf_Debug dbg, Dwarf_Die die)
{
    int rv;
    Dwarf_Half tag = 0;
    Dwarf_Error err;

    if ((rv = dwarf_tag(die, &tag, &err)) != DW_DLV_OK)
	dwf_fatal_error("dwarf_tag", rv, die, err);
    return tag;
}

/*
 * Does the DIE have the attribute specified by 'id' ?
 * Return TRUE if attribute present.
 */
bool
dwf_has_attribute(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id)
{
    int rv;
    Dwarf_Error err;
    Dwarf_Bool has_it = 0;

    if ((rv = dwarf_hasattr(die, id, &has_it, &err)) != DW_DLV_OK)
	dwf_fatal_error("dwarf_hasattr", rv, die, err);
    return (bool)has_it;
}

/*
 * Get string attribute value from a DIE.
 *
 * 'id' is DW_AT_xxx as defined in <dwarf.h>
 * 'ap' is an UPS alloc_pool_t, use NULL if not required.
 * It is an error to ask for an attribute that is not present.
 */
char *
dwf_get_string(Dwarf_Debug dbg, alloc_pool_t *ap, Dwarf_Die die, Dwarf_Half id)
{
    int rv;
    Dwarf_Error err;
    Dwarf_Attribute attribute;
    Dwarf_Half form;
    char *s;
    char *str = NULL;

    if ((rv = dwarf_attr(die, id, &attribute, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_attr", rv, die, err);
	goto end;
    }
    if ((rv = dwarf_whatform(attribute, &form, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_whatform", rv, die, err);
	goto cleanup_attribute;
    }
    if ((rv = dwarf_formstring(attribute, &s, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_formstring", rv, die, err);
	goto cleanup_attribute;
    }
    if (ap)
	str = alloc_strdup(ap, s);
    else
	str = strdup(s);
    dwarf_dealloc(dbg, s, DW_DLA_STRING);
cleanup_attribute:
    dwarf_dealloc(dbg, attribute, DW_DLA_ATTR);
end:
    return str;
}

/*
 * Get an object's name(s) from a DIE - looks for normal and mangled names.
 * If 'p_name' is not NULL it is set to the name.
 * If 'p_mangled' is not NULL it is set to the mangled name.
 * Returns TRUE if either name was checked for and found.
 */
bool
dwf_get_name(Dwarf_Debug dbg, alloc_pool_t *ap, Dwarf_Die die,
	     char **p_name, char **p_mangled)
{
    bool found = FALSE;

    if (p_name != NULL) {
	*p_name = NULL;
	if (dwf_has_attribute(dbg, die, DW_AT_name)) {
	    *p_name = dwf_get_string(dbg, ap, die, DW_AT_name);
	    found = TRUE;
	}
    }
    if (p_mangled != NULL) {
	*p_mangled = NULL;
	if (dwf_has_attribute(dbg, die, DW_AT_linkage_name)) {
	    *p_mangled = dwf_get_string(dbg, ap, die, DW_AT_linkage_name);
	    found = TRUE;
	}
	else if (dwf_has_attribute(dbg, die, DW_AT_MIPS_linkage_name)) {
	    *p_mangled = dwf_get_string(dbg, ap, die, DW_AT_MIPS_linkage_name);
	    found = TRUE;
	}
    }
    return found;
}

/*
 * Get address attribute value from a DIE.
 *
 * 'id' is DW_AT_xxx as defined in <dwarf.h>, e.g. DW_AT_low_pc
 * It is an error to ask for an attribute that is not present.
 */
Dwarf_Addr
dwf_get_address(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id, Dwarf_Addr base)
{
    int rv;
    Dwarf_Error err;
    Dwarf_Addr addr = 0;

    if (id == DW_AT_low_pc) {
	if ((rv = dwarf_lowpc(die, &addr, &err)) != DW_DLV_OK) {
	    dwf_fatal_error("dwarf_lowpc", rv, die, err);
	    goto end;
	}
    } else {
	Dwarf_Attribute attribute;
	Dwarf_Half form;

	if ((rv = dwarf_attr(die, id, &attribute, &err)) != DW_DLV_OK) {
	    dwf_fatal_error("dwarf_attr", rv, die, err);
	    goto end;
	} else if ((rv = dwarf_whatform(attribute, &form, &err)) != DW_DLV_OK) {
	    dwf_fatal_error("dwarf_whatform", rv, die, err);
	    goto local_cleanup_attribute;
	} else {
	    if (form == DW_FORM_addr) {
		if ((rv = dwarf_formaddr(attribute, &addr, &err)) != DW_DLV_OK) {
		    dwf_fatal_error("dwarf_formaddr", rv, die, err);
		    goto local_cleanup_attribute;
		}
	    } else if ((form == DW_FORM_data1)
		       || (form == DW_FORM_data2)
		       || (form == DW_FORM_data4)
		       || (form == DW_FORM_data8)
		       || (form == DW_FORM_udata)) {
		Dwarf_Unsigned offset;

		if ((rv = dwarf_formudata(attribute, &offset, &err)) != DW_DLV_OK) {
		    dwf_fatal_error("dwarf_formudata", rv, die, err);
		    goto local_cleanup_attribute;
		}

		addr = base + offset;
	    } else {
		dwf_fatal_error("attribute form not address", 0, die, err);
		goto local_cleanup_attribute;
	    }
	}
local_cleanup_attribute:
	dwarf_dealloc(dbg, attribute, DW_DLA_ATTR);
    }
end:
    return addr;
}

/*
 * Get global offset attribute value from a DIE.
 *
 * It is an error to ask for an attribute that is not present.
 * 'id' is DW_AT_xxx as defined in <dwarf.h>, e.g. DW_AT_type
 */
Dwarf_Off
dwf_get_offset(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id)
{
    int rv;
    Dwarf_Error err;
    Dwarf_Off val = 0;
    Dwarf_Attribute attribute;
    Dwarf_Half form;

    if ((rv = dwarf_attr(die, id, &attribute, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_attr", rv, die, err);
	goto end;
    }
    if ((rv = dwarf_whatform(attribute, &form, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_whatform", rv, die, err);
	goto cleanup_attribute;
    }

    if ((form == DW_FORM_ref1)
	    || (form == DW_FORM_ref2)
	    || (form == DW_FORM_ref4)
	    || (form == DW_FORM_ref8)
	    || (form == DW_FORM_ref_udata)
	    || (form == DW_FORM_ref_addr)) {
	if ((rv = dwarf_global_formref(attribute, &val, &err)) != DW_DLV_OK) {
	    dwf_fatal_error("dwarf_global_formref", rv, die, err);
	    goto cleanup_attribute;
	}
    } else {
	dwf_fatal_error("attribute form not (global) ref", 0, die, err);
	goto cleanup_attribute;
    }

cleanup_attribute:
    dwarf_dealloc(dbg, attribute, DW_DLA_ATTR);
end:
    return val;
}

/*
 * Get CU-relative offset attribute value from a DIE.
 *
 * It is an error to ask for an attribute that is not present.
 * 'id' is DW_AT_xxx as defined in <dwarf.h>, e.g. DW_AT_type
 * This cannot get DW_FORM_ref_addr values.
 *
 * To use this to get a DIE do:
 *   dwf_die_at_offset(dbg, cu_header_offset + offset)
 */
Dwarf_Off
dwf_get_cu_ref(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id)
{
    int rv;
    Dwarf_Error err;
    Dwarf_Off val = 0;
    Dwarf_Attribute attribute;
    Dwarf_Half form;

    if ((rv = dwarf_attr(die, id, &attribute, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_attr", rv, die, err);
	goto end;
    }
    if ((rv = dwarf_whatform(attribute, &form, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_whatform", rv, die, err);
	goto cleanup_attribute;
    }

    if ((form == DW_FORM_ref1)
	    || (form == DW_FORM_ref2)
	    || (form == DW_FORM_ref4)
	    || (form == DW_FORM_ref8)
	    || (form == DW_FORM_ref_udata)) {
	if ((rv = dwarf_formref(attribute, &val, &err)) != DW_DLV_OK) {
	    dwf_fatal_error("dwarf_formref", rv, die, err);
	    goto cleanup_attribute;
	}
    } else {
	dwf_fatal_error("attribute form not ref", 0, die, err);
	goto cleanup_attribute;
    }

cleanup_attribute:
    dwarf_dealloc(dbg, attribute, DW_DLA_ATTR);
end:
    return val;
}

/*
 * Get a signed 1, 2, 4 or 8 byte value from an attribute.
 *
 * 'attribute' is the attribute
 * 'form' is DW_AT_xxx as defined in <dwarf.h>
 */
static Dwarf_Unsigned
dwf_get_s_data(Dwarf_Debug dbg, Dwarf_Attribute attribute)
{
    int rv;
    Dwarf_Error err;
    Dwarf_Signed val = 0;

    if ((rv = dwarf_formsdata(attribute, &val, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_formsdata", rv, NULL, err);
	goto end;
    }

end:
    return val;
}

/*
 * Get an unsigned 1, 2, 4 or 8 byte value from an attribute.
 *
 * 'attribute' is the attribute
 * 'form' is DW_AT_xxx as defined in <dwarf.h>
 */
static Dwarf_Unsigned
dwf_get_u_data(Dwarf_Debug dbg, Dwarf_Attribute attribute)
{
    int rv;
    Dwarf_Error err;
    Dwarf_Unsigned val = 0;

    if ((rv = dwarf_formudata(attribute, &val, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_formudata", rv, NULL, err);
	goto end;
    }

end:
    return val;
}

/*
 * Get an unsigned 1, 2, 4 or 8 byte attribute value from a DIE.
 *
 * 'id' is DW_AT_xxx as defined in <dwarf.h>, e.g. DW_AT_decl_line
 * It is an error to ask for an attribute that is not present.
 *
 * If not sure whether data is signed or unsigned then call dwf_get_number().
 */
Dwarf_Unsigned
dwf_get_udata(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id)
{
    int rv;
    Dwarf_Error err;
    Dwarf_Unsigned val = 0;
    Dwarf_Attribute attribute;
    Dwarf_Half form;

    if ((rv = dwarf_attr(die, id, &attribute, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_attr", rv, die, err);
	goto end;
    }
    if ((rv = dwarf_whatform(attribute, &form, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_whatform", rv, die, err);
	goto cleanup_attribute;
    }

    if ((form == DW_FORM_data1)
	    || (form == DW_FORM_data2)
	    || (form == DW_FORM_data4)
	    || (form == DW_FORM_data8)
	    || (form == DW_FORM_udata)) {
	val = dwf_get_u_data(dbg, attribute);
    } else {
	dwf_fatal_error("attribute form not udata", 0, die, err);
	goto cleanup_attribute;
    }

cleanup_attribute:
    dwarf_dealloc(dbg, attribute, DW_DLA_ATTR);
end:
    return val;
}

/*
 * Get a signed 1, 2, 4 or 8 byte attribute value from a DIE.
 *
 * It is an error to ask for an attribute that is not present.
 * 'id' is DW_AT_xxx as defined in <dwarf.h>, e.g. DW_AT_decl_line
 * dwf_get_number().
 *
 * If not sure whether data is signed or unsigned then call dwf_get_number().
 */
Dwarf_Signed
dwf_get_sdata(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id)
{
    int rv;
    Dwarf_Error err;
    Dwarf_Signed val = 0;
    Dwarf_Attribute attribute;
    Dwarf_Half form;

    if ((rv = dwarf_attr(die, id, &attribute, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_attr", rv, die, err);
	goto end;
    }
    if ((rv = dwarf_whatform(attribute, &form, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_whatform", rv, die, err);
	goto cleanup_attribute;
    }

    if ((form == DW_FORM_data1)
	    || (form == DW_FORM_data2)
	    || (form == DW_FORM_data4)
	    || (form == DW_FORM_data8)
	    || (form == DW_FORM_sdata)) {
	val = dwf_get_s_data(dbg, attribute);
    } else {
	dwf_fatal_error("attribute form not sdata", 0, die, err);
	goto cleanup_attribute;
    }

cleanup_attribute:
    dwarf_dealloc(dbg, attribute, DW_DLA_ATTR);
end:
    return val;
}

/*
 * Get numeric attribute value from a DIE.
 * The value is returned cast to Dwarf_Signed if it was unsigned.
 * Use 'dwf_get_udata()' to get large unsigned values.
 *
 * It is an error to ask for an attribute that is not present.
 * 'id' is DW_AT_xxx as defined in <dwarf.h>, e.g. DW_AT_decl_line
 *
 * If it is not clear from the "form" whether the data is signed or
 * unsigned then use the attribute to decide.
 */
Dwarf_Signed
dwf_get_number(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id)
{
    int rv;
    Dwarf_Error err;
    Dwarf_Signed sval = 0;
    Dwarf_Unsigned uval = 0;
    Dwarf_Attribute attribute;
    Dwarf_Half form;

    if ((rv = dwarf_attr(die, id, &attribute, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_attr", rv, die, err);
	goto end;
    }
    if ((rv = dwarf_whatform(attribute, &form, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_whatform", rv, die, err);
	goto cleanup_attribute;
    }

    /*
     * If it is not clear from the "form" whether the data is signed or
     * unsigned then use the attribute to decide.
     */
    if (form == DW_FORM_sdata) {
	/*
	 * Data is a signed value.
	 */
	sval = dwf_get_s_data(dbg, attribute);
    } else if (form == DW_FORM_udata) {
	/*
	 * Data is an unsigned value.
	 */ 
	uval = dwf_get_u_data(dbg, attribute);
	if (uval > LONG_MAX)
	    errf("\bdwf_get_number: value > LONG_MAX");
	sval = (Dwarf_Signed)uval;
    } else if ((form == DW_FORM_data1)
	    || (form == DW_FORM_data2)
	    || (form == DW_FORM_data4)
	    || (form == DW_FORM_data8)) {
	/*
	 * Signed/unsigned depending on tag.
	 * Taken from .../dwarfdump/print_die.c
	 */
	switch (id) {
	case DW_AT_ordering:
	case DW_AT_byte_size:
	case DW_AT_bit_offset:
	case DW_AT_bit_size:
	case DW_AT_inline:
	case DW_AT_language:
	case DW_AT_visibility: 
	case DW_AT_virtuality:
	case DW_AT_accessibility:
	case DW_AT_address_class:
	case DW_AT_calling_convention:
	case DW_AT_encoding:
	case DW_AT_identifier_case:
	case DW_AT_MIPS_loop_unroll_factor:
	case DW_AT_MIPS_software_pipeline_depth:
	case DW_AT_decl_column:
	case DW_AT_decl_file:
	case DW_AT_decl_line:
	case DW_AT_start_scope:
	case DW_AT_stride_size:
	case DW_AT_count:
	case DW_AT_stmt_list:
	case DW_AT_MIPS_fde:
	    uval = dwf_get_u_data(dbg, attribute);
	    sval = (Dwarf_Signed)uval;
	    break;

	case DW_AT_const_value:
	    /* dwarfTODO: presumably SGI fix not reqd. */
	    uval = dwf_get_u_data(dbg, attribute);
	    sval = (Dwarf_Signed)uval;
	    break;

	case DW_AT_upper_bound:
	case DW_AT_lower_bound:
	default:
	    sval = dwf_get_s_data(dbg, attribute);
	    break;

	} /* switch on 'id' */
    } else {
	dwf_fatal_error("attribute form not data", 0, die, err);
	goto cleanup_attribute;
    }

cleanup_attribute:
    dwarf_dealloc(dbg, attribute, DW_DLA_ATTR);
end:
    return sval;
}

/*
 * Get flag attribute value from a DIE.
 *
 * It is an error to ask for an attribute that is not present.
 * 'id' is DW_AT_xxx as defined in <dwarf.h>, e.g. DW_AT_external
 *
 * You should probably be calling dwf_get_opt_flag() as flag attributes
 * seem to be present only when set TRUE.
 */
bool
dwf_get_flag(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id)
{
    int rv;
    Dwarf_Error err;
    Dwarf_Bool val = 0;
    Dwarf_Attribute attribute;
    Dwarf_Half form;

    if ((rv = dwarf_attr(die, id, &attribute, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_attr", rv, die, err);
	goto end;
    }
    if ((rv = dwarf_whatform(attribute, &form, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_whatform", rv, die, err);
	goto cleanup_attribute;
    }

    if (form == DW_FORM_flag) {
	if ((rv = dwarf_formflag(attribute, &val, &err)) != DW_DLV_OK) {
	    dwf_fatal_error("dwarf_formflag", rv, die, err);
	    goto cleanup_attribute;
    	}
    } else if (form == DW_FORM_flag_present) {
	val = 1;
    } else {
	dwf_fatal_error("attribute form not flag", 0, die, err);
	goto cleanup_attribute;
    }

cleanup_attribute:
    dwarf_dealloc(dbg, attribute, DW_DLA_ATTR);
end:
    return (bool)val;
}

/*
 * Get optional flag attribute value from a DIE.
 *
 * Returns FALSE if the attribute is not present.
 * 'id' is DW_AT_xxx as defined in <dwarf.h>, e.g. DW_AT_external
 */
bool
dwf_get_opt_flag(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id)
{
    if (!dwf_has_attribute(dbg, die, id))
	return FALSE;
    return dwf_get_flag(dbg, die, id);
}

/*
 * Get a DWARF 'location list'
 * You should probably be calling dwf_get_location().
 */
Dwarf_Locdesc **
dwf_get_loclist(Dwarf_Debug dbg, Dwarf_Die die, Dwarf_Half id, Dwarf_Signed *p_count)
{
    int rv;
    Dwarf_Error err;
    Dwarf_Attribute attribute;
    Dwarf_Half form;
    Dwarf_Locdesc **loclist = NULL;

    if ((rv = dwarf_attr(die, id, &attribute, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_attr", rv, die, err);
	goto end;
    }
    if (dwarf_whatform(attribute, &form, &err) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_whatform", rv, die, err);
	goto end;
    }
    if ((rv = dwarf_loclist_n(attribute, &loclist, p_count, &err)) != DW_DLV_OK) {
	dwf_fatal_error("dwarf_loclist", rv, die, err);
	goto cleanup_attribute;
    }

cleanup_attribute:
    dwarf_dealloc(dbg, attribute, DW_DLA_ATTR);
end:
    return loclist;
}

#endif /* WANT_DWARF */

