/* ao_dwftype.c - type related functions */

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

/*
 * Notes
 * -----
 * "base" means the underlying type, e.g. for "char **" it is "char *".
 */

char ups_ao_dwftype_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#if WANT_DWARF

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <mtrprog/hash.h>
#include <limits.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "ci_parse.h"
#include "ci_types.h"
#include "st.h"
#include "ao_dwarf.h"
#include "ao_syms.h"
#include "ao_dwfname.h"
#include "ao_dwftype.h"
#include "ao_dwfutil.h"

#ifndef DW_ATE_UTF
#define DW_ATE_UTF 0x10
#endif
#ifndef DW_ATE_UCS
#define DW_ATE_UCS 0x11
#endif
#ifndef DW_ATE_ASCII
#define DW_ATE_ASCII 0x12
#endif


static void
dwf_copy_type PROTO((type_t *dst, type_t *src));
static type_t *
dwf_type_from_dtype PROTO((dtype_t *dt));
static type_t *
dwf_get_void_type PROTO((alloc_pool_t *ap, symtab_t *st));
static type_t *
dwf_try_resolve_base_type PROTO((Dwarf_Debug dbg, Dwarf_Die die,
				 alloc_pool_t *ap, stf_t *stf,
				 dtype_t *dt));
static int
dwf_guess_ae_alignment PROTO((aggr_or_enum_def_t *ae));
static type_t *
dwf_fixup_type PROTO((dtype_t *dt, dtype_t *dtypes));

/*
 * Determine the UPS typecode_t value from DWARF encoding etc.
 * This is for a DIE with tag DW_TAG_base_type, i.e. a basic type.
 *
 * For DW_ATE_xxx see .../lib/libdwarf/dwarf.h
 *
 * dwarfTODO: TY_VOID ??
 * dwarfTODO: Fortran TY_LOGICAL, TY_CHARACTER, TY_FVOID  (IS_FORTRAN() ?)
 * dwarfTODO: like the rest of UPS this assumes sizeof(char) == 1, probably
 * need TY_BYTE for DWARF basic type 'one byte unsigned'.
 */
typecode_t
dwf_get_typecode(dbg, die)
Dwarf_Debug dbg;
Dwarf_Die die;
{
    int nbytes;
    int encoding;

    nbytes = dwf_get_number(dbg, die, DW_AT_byte_size);
    encoding = dwf_get_number(dbg, die, DW_AT_encoding);
    switch (encoding) {

    case DW_ATE_address:	/* dwarfTODO: reasonable assumptions ? */
	if (sizeof(void *) == sizeof(unsigned int))
	    return TY_UINT;
	else if (sizeof(void *) == sizeof(unsigned long))
	    return TY_ULONG;
	else {
	    errf("DWARF %d byte address not handled, int assumed", nbytes);
	    return TY_INT_ASSUMED;
	}

    case DW_ATE_boolean:
	return TY_BOOLEAN;

    case DW_ATE_signed_char:
	return TY_CHAR;

    case DW_ATE_unsigned_char:
	return TY_UCHAR;

    case DW_ATE_complex_float:
	if (nbytes == (2 * sizeof(float)))
	    return TY_COMPLEX;
	else if (nbytes == (2 * sizeof(double)))
	    return TY_DBLCOMP;
	else
	    errf("DWARF %d byte complex float not handled", nbytes);
	break;

    case DW_ATE_float:
	if (nbytes == sizeof(float))
	    return TY_FLOAT;
	else if (nbytes == sizeof(double))
	    return TY_DOUBLE;
#if HAVE_LONG_DOUBLE
	else if (nbytes == sizeof(long double))
	    return TY_LONGDOUBLE;
#endif
	else
	    errf("DWARF %d byte float not handled", nbytes);
	break;

    case DW_ATE_signed:
	if (nbytes == sizeof(int))
	    return TY_INT;
	else if (nbytes == sizeof(long))
	    return TY_LONG;
#if HAVE_LONG_LONG
	else if (nbytes == sizeof(long long))
	    return TY_LONGLONG;
#endif
	else if (nbytes == sizeof(short))
	    return TY_SHORT;
	else if (nbytes == sizeof(char))
	    return TY_CHAR;
	else {
	    errf("DWARF %d byte signed not handled, int assumed", nbytes);
	    return TY_INT_ASSUMED;
	}

    case DW_ATE_unsigned:
    case DW_ATE_UTF: // char16_t, char32_t
	if (nbytes == sizeof(unsigned int))
	    return TY_UINT;
	else if (nbytes == sizeof(unsigned long))
	    return TY_ULONG;
#if HAVE_LONG_LONG
	else if (nbytes == sizeof(unsigned long long))
	    return TY_ULONGLONG;
#endif
	else if (nbytes == sizeof(unsigned short))
	    return TY_USHORT;
	else if (nbytes == sizeof(char))
	    return TY_UCHAR;
	else {
	    errf("DWARF %d byte unsigned not handled, int assumed", nbytes);
	    return TY_INT_ASSUMED;
	}

    case DW_ATE_imaginary_float:	/* DWARF 3 */
	errf("DWARF %d byte imaginary float not handled", nbytes);
	break;

    } /* switch */

    return TY_NOTYPE;
}

/*
 * Make a 'block_t' structure and fill in some values.
 */
block_t *
dwf_make_block(dbg, die, ap, stf, parent, f)
Dwarf_Debug dbg;
Dwarf_Die die;
alloc_pool_t *ap;
stf_t *stf;
block_t *parent;	/* Parent block list. */
func_t *f;		/* Function containing the block. */
{
    block_t *bl;
    int start_lnum = 0, end_lnum = 0;
    fil_t *fil;
    fsyminfo_t *fs;
    taddr_t low_pc, high_pc;

    /*
     * Get address range of block.
     *
     * dwarfTODO: "gcc -g -O" emits DW_AT_ranges
     */
    if (dwf_has_attribute(dbg, die, DW_AT_ranges)) {
	low_pc = 0;
	high_pc = 0;
    } else {
       low_pc  = dwf_get_address(dbg, die, DW_AT_low_pc, 0) + stf->stf_addr;
	high_pc = dwf_get_address(dbg, die, DW_AT_high_pc, low_pc) + stf->stf_addr;
    }

    /*
     * We usually drop the first 'lno' of a function (the preamble)
     * so addr_to_fil_and_lnum() cannot find function address.
     *
     * 'high_pc' often seems to be the PC of the next statement,
     * taking off 1 seems to give a better line number.
     */
    fs = AO_FSYMDATA(f);
    if (low_pc == fs->fs_low_pc)
	start_lnum = f->fu_lexinfo->lx_lnum;
    else
	addr_to_fil_and_lnum(f, low_pc, &fil, &start_lnum, FALSE);
    if (high_pc == fs->fs_high_pc)
	addr_to_fil_and_lnum(f, high_pc, &fil, &end_lnum, FALSE);
    else
	addr_to_fil_and_lnum(f, high_pc - 1, &fil, &end_lnum, FALSE);

    bl = ci_make_block(ap, parent);
    bl->bl_start_lnum = start_lnum;
    bl->bl_end_lnum = end_lnum;
    return bl;
}

/*
 * Make a 'dim_t' to hold details of a subrange (array dimension).
 *
 * dwarfTODO: cope with non-constant bounds
 */
dim_t *
dwf_make_subrange(dbg, die, ap, stf)
Dwarf_Debug dbg;
Dwarf_Die die;
stf_t *stf;
alloc_pool_t *ap;
{
    dtype_t *dt;
    dim_t *dim;
    off_t type_offset;
    Dwarf_Signed count = 0, low, high;

    /*
     * Default bounds.
     */
    if (IS_FORTRAN(stf->stf_language))
	low = high = 1;
    else
	low = high = 0;

    /*
     * Get lower and upper bounds.
     *
     * Need to use the right signed/unsigned call because dwf_get_number()
     * assumes bounds are signed but GCC puts 256 in a DW_FORM_data1.
     */
    if (IS_FORTRAN(stf->stf_language)) {
	/*
	 * Language can have negative array bounds.
	 */
	if (dwf_has_attribute(dbg, die, DW_AT_lower_bound))
	    low = dwf_get_sdata(dbg, die, DW_AT_lower_bound);
	if (dwf_has_attribute(dbg, die, DW_AT_upper_bound))
	    high = dwf_get_sdata(dbg, die, DW_AT_upper_bound);
	else if (dwf_has_attribute(dbg, die, DW_AT_count)) {
	    count = dwf_get_number(dbg, die, DW_AT_count);
	    high = low + count - 1;
	}
    } else {
	/*
	 * Language array bounds must be >= 0.
	 */
	if (dwf_has_attribute(dbg, die, DW_AT_lower_bound))
	    low = dwf_get_udata(dbg, die, DW_AT_lower_bound);
	if (dwf_has_attribute(dbg, die, DW_AT_upper_bound))
	    high = dwf_get_udata(dbg, die, DW_AT_upper_bound);
	else if (dwf_has_attribute(dbg, die, DW_AT_count)) {
	    count = dwf_get_number(dbg, die, DW_AT_count);
	    high = low + count - 1;
	}
    }

    /*
     * dwarfTODO: cope with other than DT_CONSTANT
     */
    dim = (dim_t *)alloc(ap, sizeof(dim_t));
    dim->di_low = low;
    dim->di_ltype = DT_CONSTANT;
    dim->di_high = high;
    dim->di_htype = DT_CONSTANT;
    dim->di_high_expr_id = NULL;
    dim->di_type = NULL;

    dt = dwf_make_dtype(dbg, die, ap, stf, DT_IS_RANGE, &(dim->di_type), NULL);

    /*
     * If can find the type then use it, else save its offset
     * for dwf_fixup_types() to sort out.
     *
     * This is the type of the subrange, so will usually be an integer
     * type.
     */
    if (dwf_has_attribute(dbg, die, DW_AT_type)) {
	type_offset = dwf_get_cu_ref(dbg, die, DW_AT_type);
	if ((dim->di_type = dwf_find_type(stf, type_offset)) == NULL)
	    dt->dt_base_offset = type_offset;
    }

    return dim;
}

/*
 * Make a member of an enumerated type.
 */
enum_member_t *
dwf_make_enumerator(dbg, die, ap, stf, ae)
Dwarf_Debug dbg;
Dwarf_Die die;
stf_t *stf;
alloc_pool_t *ap;
aggr_or_enum_def_t *ae;
{
    char *name;
    long value;
    enum_member_t *em;

    /*
     * Get the name and value.
     */
    name = dwf_get_string(dbg, ap, die, DW_AT_name);
    value = dwf_get_number(dbg, die, DW_AT_const_value);

    /*
     * Make the enumerator and link into the type definition.
     */
    em = ci_make_enum_member(ap, name, value);
    em->em_lexinfo = dwf_make_lexinfo(dbg, die, ap, stf);
    em->em_enum = ae;
    em->em_next = ae->ae_enum_members;
    ae->ae_enum_members = em;

    return em;
}

/*
 * Make a 'var_t' to hold details of a variable (hint = CL_AUTO).
 * Also used for struct members (CL_MOS), union members (CL_MOU),
 * and routine parameters (CL_ARG).
 */
var_t *
dwf_make_variable(dbg, die, ap, stf, p_vars, dw_level, class_hint, f)
Dwarf_Debug dbg;
Dwarf_Die die;
stf_t *stf;
alloc_pool_t *ap;
var_t **p_vars;
int dw_level;
class_t class_hint;	/* CL_AUTO, CL_MOS, CL_MOU or CL_ARG */
func_t *f;
{
    Dwarf_Die spec_die;
    var_t *v;
    dtype_t *dt = NULL;
    type_t *type = NULL;
    off_t type_offset;
    char *name = NULL;
    class_t class = CL_NOCLASS;
    vaddr_t *vaddr = NULL;
    taddr_t addr = 0;
    fsyminfo_t *fs = NULL;
    vaddr_t *frame_base = NULL;

    /*
     * Find the frame base address.
     */
    if (f) {
	fs = (fsyminfo_t *)f->fu_symdata;
        frame_base = fs->fs_frame_base;
    }

    /*
     * If there is an address it is a definition.
     * Union members don't have addresses.
     */
    if (dwf_has_attribute(dbg, die, DW_AT_location)) {
	vaddr = dwf_get_location(dbg, ap, die, DW_AT_location, frame_base);
    } else if (dwf_has_attribute(dbg, die, DW_AT_data_member_location)) {
	vaddr = dwf_get_location(dbg, ap, die, DW_AT_data_member_location, frame_base);
    } else if (class_hint != CL_MOU)
	return (var_t *)NULL;

    /*
     * Get the variable's name and type.
     * Only base classes should not have a name.
     */
    spec_die = dwf_find_spec_die(dbg, die);
    if (dwf_has_attribute(dbg, spec_die, DW_AT_name))
	name = dwf_get_string(dbg, ap, spec_die, DW_AT_name);
    type_offset = dwf_get_cu_ref(dbg, spec_die, DW_AT_type);
    type = dwf_find_type(stf, type_offset);

    /*
     * Is this a bitfield structure member ?
     */
    if (dwf_has_attribute(dbg, spec_die, DW_AT_bit_offset)) {
	/*
	 * If this is a bitfield then we make a 'type_t' for the bitfield
	 * and use it as the type of this structure member.  If we don't
	 * have the base type make a 'dtype_t' for later fixup.
	 */
	dtype_t *dtb;
	type_t *base = type;
	int bit_offset = dwf_get_number(dbg, spec_die, DW_AT_bit_offset);
	int bit_size = dwf_get_number(dbg, spec_die, DW_AT_bit_size);
	int ups_bit_offset, nbytes = 0;

	/*
	 * Get size in bytes - use attribute or base type.
	 *
	 * dwarfTODO: fails if no DW_AT_byte_size or base type not known.
	 */
	if (dwf_has_attribute(dbg, spec_die, DW_AT_byte_size))
	    nbytes = dwf_get_number(dbg, spec_die, DW_AT_byte_size);
	else if (base != NULL)
	    nbytes = base->ty_size;
	else
	    errf("\bbase size of bitfield %s not known", name);

	/*
	 * UPS offset is from LSB, DWARF is from MSB.
	 */
	ups_bit_offset = (nbytes * 8) - (bit_offset + bit_size);
	type = ci_make_bitfield_type(ap, base, ups_bit_offset, bit_size);
	type->ty_size = nbytes;

	if (base == NULL) {
	    /*
	     * Cannot find type, save for later.
	     */
	    dtb = dwf_make_dtype(dbg, die, ap, stf, DT_IS_BITFIELD, &(type->ty_bitfield->bf_type), NULL);
	    dtb->dt_base_offset = type_offset;
	}

    } else {
	/*
	 * Not a bitfield, do the usual...
	 */
	if (type == NULL) {
	    /*
	     * Cannot find type, save for later.
	     * dt->dt_p_type gets set after calling ci_make_var() below.
	     */
	    dt = dwf_make_dtype(dbg, die, ap, stf, DT_IS_VAR, NULL, NULL);
	    dt->dt_base_offset = type_offset;
	}
    }


    /*
     * Determine the class and address.
     * dwarfTODO: finish this
     */
    if (vaddr == NULL) {

	if ((class = class_hint) != CL_MOU)
	    type = NULL;
	addr = 0;

    } else if (vaddr->v_op == OP_CFA_RELATIVE ||
	       vaddr->v_op == OP_FP_RELATIVE ||
	       vaddr->v_op == OP_SP_RELATIVE ) {

	if (class_hint == CL_ARG)
	    class = CL_ARG;
	else if (class_hint == CL_AUTO)
	    class = CL_AUTO;
	addr = vaddr->v_offset;

    } else if (vaddr->v_op == OP_REGISTER) {

	class = CL_REG;
	addr = vaddr->v_register; /* dwarfTODO: correct ? */

    } else if (vaddr->v_op == OP_ADDR) {

	if (dwf_get_opt_flag(dbg, die, DW_AT_external))
	    class = CL_EXT;
	else if (dwf_get_opt_flag(dbg, spec_die, DW_AT_external))
	    class = CL_EXT;
	else if (dw_level == 1)
	    class = CL_STAT;
	else
	    class = CL_LSTAT;
	addr = vaddr->v_addr = vaddr->v_addr + stf->stf_addr;

    } else if (vaddr->v_op == OP_U_OFFSET) {

	class = CL_NOCLASS;
	addr = vaddr->v_u_offset;
	vaddr = NULL;

    }

    /*
     * Make the 'var_t' and (optionally) add it to the list of variables
     * in the block.
     */
    v = ci_make_var(ap, name, class, type, addr);
    v->va_language = stf->stf_language;
    v->va_lexinfo = dwf_make_lexinfo(dbg, spec_die, ap, stf);
    v->va_location = vaddr;
    v->va_next = *p_vars;
    *p_vars = v;
    if (dt)
	dt->dt_p_type = &(v->va_type);

    return v;
}

/*
 * Copy a type
 */
static void
dwf_copy_type(dst, src)
type_t *dst;
type_t *src;
{
    memcpy(dst, src, sizeof(type_t));
}

/*
 * Get the type out of a 'dtype_t'
 *
 * If the type is set (DT_IS_TYPE/DT_IS_TYPEDEF) then use it,
 * otherwise try the type pointer.
 */
static type_t *
dwf_type_from_dtype(dt)
dtype_t *dt;
{
    if (dt == NULL)
	return (type_t *)NULL;
    if (dt->dt_type)
	return dt->dt_type;
    else if (dt->dt_p_type)
	return *(dt->dt_p_type);
    return (type_t *)NULL;
}

/*
 * Fake a 'void' type - for things like "void *" DWARF may just have a
 * DW_TAG_pointer_type with no base type, implying 'void'
 */
static type_t *dwf_get_void_type(ap, st)
alloc_pool_t *ap;
symtab_t *st;
{
    ao_stdata_t *ast = AO_STDATA(st);
    if (ast->st_dw_void_type == NULL) {
	ast->st_dw_void_type = ci_make_type(ap, TY_VOID);
	ast->st_dw_void_type->ty_size = 0;
	ast->st_dw_void_type->ty_name = "void";
    }
    return ast->st_dw_void_type;
}

/*
 * Make a 'dtype_t' structure.
 * Used to keep track of DWARF type and typedef information.
 * 'ptr' is to associated data.
 */
dtype_t *
dwf_make_dtype(dbg, die, ap, stf, dt_is, p_type, x_type)
Dwarf_Debug dbg;
Dwarf_Die die;
alloc_pool_t *ap;
stf_t *stf;
dt_is_t dt_is;
type_t **p_type;
type_t *x_type;
{
    dtype_t *dt;

    /*
     * dwarfTODO: perhaps dwf_offset_of_die() would be better ?
     */
    dt = (dtype_t *)alloc(ap, sizeof(dtype_t));
    dt->dt_offset = dwf_cu_offset_of_die(dbg, die);
    dt->dt_base_offset = (off_t)0;
    dt->dt_is = dt_is;
    dt->dt_p_type = p_type;
    dt->dt_type = x_type;
    dt->dt_next = NULL;

    /*
     * Put at end of list - dwf_fixup_types() relies on this.
     */
    if (stf->stf_dtypes == NULL)
	stf->stf_dtypes = dt;
    if (stf->stf_last_dt != NULL)
	stf->stf_last_dt->dt_next = dt;
    stf->stf_last_dt = dt;

    return dt;
}

/*
 * Try to set the base type in a 'dtype_t'.
 * If the DIE has a DW_AT_type then try to resolve it; if cannot then save
 * for dwf_fixup_types().
 * If no DW_AT_type then assume 'void'.
 */
static type_t *
dwf_try_resolve_base_type(dbg, die, ap, stf, dt)
Dwarf_Debug dbg;
Dwarf_Die die;
alloc_pool_t *ap;
stf_t *stf;
dtype_t *dt;
{
    off_t base_offset;
    type_t *base = NULL;

    /*
     * If there is a reference to another type then look it up.
     */
    if (dwf_has_attribute(dbg, die, DW_AT_type)) {
	/*
	 * If can find the base type then save it, else save its offset
	 * for dwf_fixup_types() to sort out.
	 */
	base_offset = dwf_get_cu_ref(dbg, die, DW_AT_type);
	if ((base = dwf_find_type(stf, base_offset)) != NULL)
	    *(dt->dt_p_type) = base;
	else
	    dt->dt_base_offset = base_offset;
    } else {
	/*
	 * If there is no DW_AT_type attribute this is a "void *" or similar.
	 */
	*(dt->dt_p_type) = dwf_get_void_type(ap, stf->stf_symtab);
    }
    return base;
}

/*
 * Make a 'type_t' for a derived type.
 * Also creates the corresponding 'dtype_t'.
 */
dtype_t *
dwf_make_type(dbg, die, ap, stf, typecode)
Dwarf_Debug dbg;
Dwarf_Die die;
stf_t *stf;
alloc_pool_t *ap;
typecode_t typecode;
{
    dtype_t *dt;
    type_t *type;

    /*
     * Make a 'type_t' and 'dtype_t'
     * If the type has a byte size then save it.
     * If the type has a name then save it (derived types
     * usually do not).
     */
    type = ci_make_type(ap, typecode);
    type->ty_lexinfo = dwf_make_lexinfo(dbg, die, ap, stf);

    if (typecode == TY_NOTYPE)
	type->ty_size = 0;
    else if (dwf_has_attribute(dbg, die, DW_AT_byte_size))
	type->ty_size = dwf_get_number(dbg, die, DW_AT_byte_size);

    if (dwf_has_attribute(dbg, die, DW_AT_name))
	type->ty_name = dwf_get_string(dbg, ap, die, DW_AT_name);

    dt = dwf_make_dtype(dbg, die, ap, stf, DT_IS_TYPE, &(type->ty_base), type);
    dwf_try_resolve_base_type(dbg, die, ap, stf, dt);

    return dt;
}

/*
 * Make a 'type_t' for a class/struct type.
 * Also creates the corresponding 'dtype_t'.
 */
dtype_t *
dwf_make_struct_type(dbg, die, ap, stf, typecode, bl)
Dwarf_Debug dbg;
Dwarf_Die die;
stf_t *stf;
alloc_pool_t *ap;
typecode_t typecode;
block_t *bl;
{
    return dwf_make_ae_type(dbg, die, ap, stf, typecode, bl);
}

/*
 * Make a 'type_t' for an aggregate type (class/struct/union/enum).
 * Also creates the corresponding 'dtype_t'.
 */
dtype_t *
dwf_make_ae_type(dbg, die, ap, stf, typecode, bl)
Dwarf_Debug dbg;
Dwarf_Die die;
stf_t *stf;
alloc_pool_t *ap;
typecode_t typecode;
block_t *bl;
{
    dtype_t *dt;
    type_t *type;
    aggr_or_enum_def_t *ae;
    char *name = NULL;

    /*
     * Make a 'type_t' and 'dtype_t'
     */
    type = ci_make_type(ap, typecode);
    dt = dwf_make_dtype(dbg, die, ap, stf, DT_IS_TYPE, &(type->ty_base), type);

    if (dwf_has_attribute(dbg, die, DW_AT_name))
	name = dwf_get_string(dbg, ap, die, DW_AT_name);

    /*
     * NOTE - ci_make_aggr_or_enum_def() does ci_init_type()
     */
    ae = ci_make_aggr_or_enum_def(ap, name, typecode, type);
    if (typecode == TY_ENUM)
	ae->ae_enum_members = NULL;
    else
	ae->ae_aggr_members = NULL;
    if (dwf_has_attribute(dbg, die, DW_AT_byte_size))
	type->ty_size = dwf_get_number(dbg, die, DW_AT_byte_size);
    type->ty_name = name;
    type->ty_lexinfo = dwf_make_lexinfo(dbg, die, ap, stf);
    ae->ae_lexinfo = type->ty_lexinfo;

    /*
     * Finish off undefined enum/struct/union here as will not be calling
     * dwf_finish_aggregate().
     */
    if ((typecode == TY_U_STRUCT) || (typecode == TY_U_UNION) || (typecode == TY_U_ENUM)) {
	ae->ae_size = type->ty_size;
	ae->ae_alignment = -1;
	ae->ae_is_complete = AE_COMPLETE;
    }

    /*
     * Link it in.
     */
    ae->ae_next = bl->bl_aggr_or_enum_defs;
    bl->bl_aggr_or_enum_defs = ae;

    return dt;
}

/*
 * Make a 'typedef_t'.
 * Also creates the corresponding 'dtype_t'.
 *
 * In 'type_t' there is a pointer back to the 'typedef_t' but with DWARF
 * the same type could be used by several typedefs.  Solve by creating a
 * dummy type, dwf_fixup_types() sorts it out.
 */
dtype_t *
dwf_make_typedef(dbg, die, ap, stf, bl)
Dwarf_Debug dbg;
Dwarf_Die die;
alloc_pool_t *ap;
stf_t *stf;
block_t *bl;
{
    dtype_t *dt, *base;
    type_t *type;
    typedef_t *td;
    off_t base_offset;
    char *name;

    /*
     * Get the name and make a dummy type.
     */
    name = dwf_get_string(dbg, ap, die, DW_AT_name);
    type = ci_make_type(ap, TY_NOTYPE);
    type->ty_lexinfo = dwf_make_lexinfo(dbg, die, ap, stf);
    type->ty_name = name;

    /*
     * Make a 'typedef' referring to the dummy type.
     * Link it into the typedefs in the block.
     */
    td = ci_make_typedef(ap, name, type);
    td->td_lexinfo = type->ty_lexinfo;
    dt = dwf_make_dtype(dbg, die, ap, stf, DT_IS_TYPEDEF, &(type->ty_base), type);
    td->td_next = bl->bl_typedefs;
    bl->bl_typedefs = td;

    /*
     * If can find the base type then use it, else save its offset
     * for dwf_fixup_types() to sort out.
     */
    if (dwf_has_attribute(dbg, die, DW_AT_type)) {
	base_offset = dwf_get_cu_ref(dbg, die, DW_AT_type);
	if ((base = dwf_lookup_dtype(stf->stf_dtypes, base_offset)) != NULL)
	    if (base->dt_base_offset == 0)
		type->ty_base = dwf_type_from_dtype(base);
	    else
		dt->dt_base_offset = base_offset;
	else
	    dt->dt_base_offset = base_offset;
    } else if (td->td_lexinfo) {
	/*
	 * A declaration without a type - assume a typedef of 'void'.
	 */
	type->ty_base = dwf_get_void_type(ap, stf->stf_symtab);
    } else {
	/*
	 * No type, not a definition, ...
	 */
	errf("\bDWARF typedef '%s' has no type", td->td_name);
    }

    /*
     * Set 'ty_typedef' to show it is typedef'd.
     */
    type->ty_typedef = td;

    return dt;
}

/*
 * Find a 'dtype_t' using an offset.
 * Start looking at the specified 'dt'.
 */
dtype_t *
dwf_lookup_dtype(dt, offset)
dtype_t *dt;
off_t offset;
{
    while (dt != NULL) {
	if (dt->dt_offset == offset)
	    return dt;
	dt = dt->dt_next;
    }
    return (dtype_t *)NULL;
}

/*
 * Find a 'type_t' using an offset.
 * Start looking at the specified 'dt'.
 */
type_t *
dwf_lookup_type(dt, offset)
dtype_t *dt;
off_t offset;
{
    return dwf_type_from_dtype(dwf_lookup_dtype(dt, offset));
}

/*
 * Find a 'type_t' using an offset.
 * Look in all the types in the 'stf'.
 */
type_t *
dwf_find_type(stf, offset)
stf_t *stf;
off_t offset;
{
    return dwf_lookup_type(stf->stf_dtypes, offset);
}

/*
 * Struct/union alignment.
 * Returns current ae_alignment if not all types available.
 *
 * dwarfTODO: packed structures ?
 */
static int
dwf_guess_ae_alignment(ae)
aggr_or_enum_def_t *ae;
{
    var_t *v;
    int alignment, max_align = 0;

    if (ae->ae_is_complete != AE_COMPLETE)
	return ae->ae_alignment;
    if ((v = ae->ae_aggr_members) == NULL)
	return ae->ae_alignment;

    while (v != NULL) {
	if (v->va_type == NULL)
	    return ae->ae_alignment;
	switch(v->va_type->ty_code) {
	case TY_NOTYPE:
	    return ae->ae_alignment;
	case TY_U_STRUCT:
	case TY_U_UNION:
	case TY_U_ENUM:
	    alignment = 0;
	    break;
	default:
	    alignment = ci_type_alignment(v->va_type);
	    break;
	}
	if (alignment > max_align)
	    max_align = alignment;
	v = v->va_next;
    }
    return max_align;
}

/*
 * Fixup the types.
 *
 * When reading the type information we don't bother to follow forward
 * references, just record the 'base_offset'. Now, as the list is kept
 * in the order things are read, any forward references should be further
 * on in the list, never earlier.
 *
 * Also sort out any dummy types we created.
 * Also fix base class names in inherited classes.
 */
int
dwf_fixup_types(dt, recursed)
dtype_t *dt;
int recursed;
{
    dtype_t *start = dt;
    type_t *dest;
    int incomplete = 0;
    int bad_dummy = 0;

    /*
     * First resolve base types.
     */
    for (dt = start; dt; dt = dt->dt_next) {
	dwf_fixup_type(dt, start);
	if (dt->dt_base_offset != (off_t)0) {
	    incomplete++;
	    if (recursed == 0) {
		errf("\bDWARF type incomplete, offset <%ld>", (long)dt->dt_base_offset);
#if WANT_DEBUG
		fprintf(stderr, "level %d incomplete type, offset <%ld>\n", recursed, (long)dt->dt_base_offset);
#endif
	    }
	}
    }

    for (dt = start; dt; dt = dt->dt_next) {
	if (dt->dt_base_offset == (off_t)0) {
	    type_t *base;
	    qualifiers_t qual;
	    typedef_t *tdef;
	    lexinfo_t *lx;
	   
	    /*
	     * If this is a dummy type (ty_code == TY_NOTYPE) then fix it.
	     */
	    switch (dt->dt_is) {
	    case DT_IS_TYPE:
		dest = dt->dt_type;
		if (dest->ty_code != TY_NOTYPE)
		    break;
		/*
		 * Only qualified types should have got a dummy type.
		 */
		if (dt->dt_type->ty_qualifiers == 0) {
		    errf("\bUnqualified dummy DWARF type");
		    break;
		}
		/*
		 * Copy the base type, preserving the qualifiers and lexinfo.
		 */
		base = dt->dt_type->ty_base;
		qual = dest->ty_qualifiers;
		lx = dest->ty_lexinfo;
		if (base)
		    dwf_copy_type(dest, base);
		dest->ty_qualifiers = qual;
		if (lx)
		    dest->ty_lexinfo = lx;
		break;
		
	    case DT_IS_TYPEDEF:
		dest = dt->dt_type;
		if (dest->ty_code != TY_NOTYPE)
		    break;
		/*
		 * Copy the base type, preserving the 'typedef' pointer
		 * and lexinfo
		 */
		base = *(dt->dt_p_type);
		tdef = dest->ty_typedef;
		lx = dest->ty_lexinfo;
		dwf_copy_type(dest, base);
		dest->ty_typedef = tdef;
		if (lx)
		    dest->ty_lexinfo = lx;
		/*
		 * If the base type does not already contain a reference to
		 * a typedef then set it.  This is so that variables are
		 * shown with the typedef name even if the symbol table
		 * used the underlying type.
		 */
		if ((base->ty_typedef == NULL) && (base->ty_lexinfo != NULL))
		    base->ty_typedef = tdef;
		break;
	   
	    case DT_IS_VAR:
	    case DT_IS_RANGE:
	    case DT_IS_BITFIELD:
	       break;
	       
	    }
	}
    }

    /*
     * Look for struct/union to fix.
     * Also does base class names.
     */
    for (dt = start; dt; dt = dt->dt_next) {
	if ((dt->dt_base_offset == (off_t)0) && (dt->dt_is == DT_IS_TYPE)) {
	    dest = dt->dt_type;
	    if ((dest->ty_code == TY_STRUCT) || (dest->ty_code == TY_UNION)) {
		if (recursed == 0) {
		    aggr_or_enum_def_t *ae = dest->ty_aggr_or_enum;
		    var_t *v;
		    /*
		     * Should be able to work out struct alignment.
		     */
		    ae->ae_alignment = dwf_guess_ae_alignment(ae);
		    if (ae->ae_alignment <= 0)
			errf("No alignment on %s", ae->ae_tag ? ae->ae_tag : "NULL");
		    /*
		     * Should now have base class name.
		     */
		    v = ae->ae_aggr_members;
		    while (v != NULL) {
			if ((v->va_flags & VA_BASECLASS) && (v->va_type != NULL))
			    ci_make_baseclass_name(v);
			v = v->va_next;
		    }
		}
	    }
	}
    }

#if WANT_DEBUG
if (recursed == 0 && (incomplete + bad_dummy) > 0)
fprintf(stderr, "level %d - %d incomplete type(s), %d bad type(s)\n", recursed, incomplete, bad_dummy);
#endif
    return incomplete + bad_dummy;
}

/*
 * Fixup type information in a single 'dtype_t' entry.
 * If the type information was resolved OK then return a pointer to the type.
 */
static type_t *
dwf_fixup_type(dt, dtypes)
dtype_t *dt;
dtype_t *dtypes;
{
    dtype_t *dbase;
    type_t *base;

    /*
     * Base type missing ?
     */
    if (dt->dt_base_offset != (off_t)0) {
	/*
	 * Look for the base type in our list.
	 */
	if ((dbase = dwf_lookup_dtype(dtypes, dt->dt_base_offset)) != NULL) {
	    /*
	     * Resolve the base type if possible.
	     */
	    if ((base = dwf_fixup_type(dbase, dtypes)) != NULL) {
		*(dt->dt_p_type) = base;
		dt->dt_base_offset = (off_t)0; /* mark it done */
	    }
	}
    }

    return (dt->dt_base_offset == (off_t)0) ? dt->dt_type : NULL;
}

/*
 * Finish up an aggregate type we have just got the members of.
 */
void
dwf_finish_aggregate(dt)
dtype_t *dt;
{
    var_t *v;
    aggr_or_enum_def_t *ae;
    class_t class;

    switch (dt->dt_type->ty_code) {
    case TY_ENUM:	class = CL_MOE; break; /* to satisfy gcc */
    case TY_STRUCT:	class = CL_MOS; break;
    case TY_UNION:	class = CL_MOU; break;
    default:		panic("botch in dwf_finish_aggregate()");
			class = CL_NOCLASS; break; /* to satisfy gcc */
    }
    ae = dt->dt_type->ty_aggr_or_enum;

    /*
     *  If there are no members demote to "undefined".
     */
    switch (dt->dt_type->ty_code) {
    case TY_ENUM:
	if (ae->ae_enum_members == NULL)
	    dt->dt_type->ty_code = TY_U_ENUM;
	break;
    case TY_STRUCT:
	if (ae->ae_aggr_members == NULL)
	    dt->dt_type->ty_code = TY_U_STRUCT;
	break;
    case TY_UNION:
	if (ae->ae_aggr_members == NULL)
	    dt->dt_type->ty_code = TY_U_UNION;
	break;
    default:
	panic("botch in dwf_finish_aggregate()");
	break;
    }

    if ((dt->dt_type->ty_code == TY_STRUCT)
	    || (dt->dt_type->ty_code == TY_UNION)) {
	/*
	 * Fix struct/union members.
	 */
	v = ae->ae_aggr_members;
	while (v != NULL) {
	    v->va_class = class;
	    v = v->va_next;
	}
    }
    ae->ae_size = dt->dt_type->ty_size;
    ae->ae_alignment = -1;
    ae->ae_is_complete = AE_COMPLETE;
}

/*
 * Finish up a C++ class type we have just got the members of.
 */
void
dwf_finish_class(dt)
dtype_t *dt;
{
    var_t *v;
    aggr_or_enum_def_t *ae;

    if (dt->dt_type->ty_code != TY_STRUCT)
	return;
    ae = dt->dt_type->ty_aggr_or_enum;

    /*
     * Fix class members.
     * If possible set base class name.
     */
    v = ae->ae_aggr_members;
    while (v != NULL) {
	if ((v->va_flags & VA_BASECLASS) && (v->va_type != NULL))
	    ci_make_baseclass_name(v);
	v = v->va_next;
    }
}

#endif /* WANT_DWARF */

