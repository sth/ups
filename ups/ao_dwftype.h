/* ao_dwftype.h - routine definitions for ao_dwftype.c */

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

typecode_t	dwf_get_typecode PROTO((Dwarf_Debug dbg, Dwarf_Die cu_die));

enum_member_t *	dwf_make_enumerator PROTO((Dwarf_Debug dbg, Dwarf_Die die,
					   alloc_pool_t *ap, stf_t *stf,
					   aggr_or_enum_def_t *ae));
dim_t *		dwf_make_subrange PROTO((Dwarf_Debug dbg, Dwarf_Die die,
					 alloc_pool_t *ap, stf_t *stf));
var_t *		dwf_make_variable PROTO((Dwarf_Debug dbg, Dwarf_Die die,
					 alloc_pool_t *ap, stf_t *stf,
					 var_t **p_vars, int dw_level,
					 class_t class_hint, func_t *f));
block_t *	dwf_make_block PROTO((Dwarf_Debug dbg, Dwarf_Die cu_die,
				      alloc_pool_t *ap, stf_t *stf,
				      block_t *parent, func_t *f));
dtype_t *	dwf_make_dtype PROTO((Dwarf_Debug dbg, Dwarf_Die die,
				      alloc_pool_t *ap,  stf_t *stf,
				      dt_is_t dt_is, type_t **p_type,
				      type_t *x_type));
dtype_t *	dwf_make_type PROTO((Dwarf_Debug dbg, Dwarf_Die die,
				     alloc_pool_t *ap, stf_t *stf,
				     typecode_t typecode));
dtype_t *	dwf_make_struct_type PROTO((Dwarf_Debug dbg, Dwarf_Die die,
					    alloc_pool_t *ap, stf_t *stf,
					    typecode_t typecode, block_t *bl));
dtype_t *	dwf_make_ae_type PROTO((Dwarf_Debug dbg, Dwarf_Die die,
				        alloc_pool_t *ap, stf_t *stf,
				        typecode_t typecode, block_t *bl));
dtype_t *	dwf_make_typedef PROTO((Dwarf_Debug dbg, Dwarf_Die die,
					alloc_pool_t *ap, stf_t *stf,
					block_t *bl));

dtype_t *	dwf_lookup_dtype PROTO((dtype_t *dt, off_t offset));
type_t *	dwf_lookup_type PROTO((dtype_t *dt, off_t offset));
type_t *	dwf_find_type PROTO((stf_t *stf, off_t offset));
int		dwf_fixup_types PROTO((dtype_t *dt, int recursed));
void		dwf_finish_aggregate PROTO((dtype_t *dt));
void		dwf_finish_class PROTO((dtype_t *dt));

#endif /* WANT_DWARF */
