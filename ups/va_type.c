/* va_type.c - type handling routines */

/*  Copyright 1991 Mark Russell, University of Kent at Canterbury.
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


/* @(#)va_type.c	1.21 04 Jun 1995 (UKC) */
char ups_va_type_c_rcsid[] = "$Id$";

#include <stdlib.h>
#include <mtrprog/ifdefs.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "st.h"
#include "data.h"
#include "va.h"
#include "va_priv.h"
#include "ci.h"
#include "state.h"

static void get_dim PROTO((target_t *xp, taddr_t fp, taddr_t ap, 
                           Dimtype dimtype, int offset, int *p_val, 
                           bool *p_val_known, bool *p_decl_may_have_changed));
static void set_il_range PROTO((ilist_t *il, bool is_associated,
				int low, int high, size_t basesize, 
                                bool *p_decl_may_have_changed));

int
typesize(type)
type_t *type;
{
	return dynamic_type_size(type, (ilist_t *)NULL);
}

/*  Return the size in bytes of type.
 *
 *  This routine just calls ci_typesize, except for arrays, where it
 *  checks for variable size arrays.
 */
int
dynamic_type_size(type, il)
type_t *type;
ilist_t *il;
{
	int size;

	if (type->ty_size != UNKNOWN_SIZE) {
		size = type->ty_size;
        }
	else if (type->ty_code == DT_ARRAY_OF) {
		dim_t *dim;
		int basesize, high, low, size_is_variable;

		dim = type->ty_dim;
		size_is_variable = dim->di_ltype != DT_CONSTANT ||
			           dim->di_htype != DT_CONSTANT;
                
		if (size_is_variable) {
			if (il == NULL || !il->il_low_known || !il->il_high_known)
				return UNKNOWN_SIZE;
			low = il->il_low;
			high = il->il_high;
		}
		else {
			low = dim->di_low;
			high = dim->di_high;
		}
                
		basesize = dynamic_type_size(type->ty_base,
				    (il != NULL) ? il->il_next
						 : (ilist_t *)NULL);

		if (high <= low)
                        high = low + 1;
		
		if (basesize == UNKNOWN_SIZE)
			size = UNKNOWN_SIZE;
		else
			size = ((high + 1) - low) * basesize;
		
		if (!size_is_variable)
			type->ty_size = size;
	}
	else {
		size = ci_typesize((lexinfo_t *)NULL, type);
        }

	if (size < 0 && size != UNKNOWN_SIZE)
		panic("non positive type size in typesize");

	return size;
}

void
fix_if_fortran_array_pointer(type, addr, ilist, p_decl_may_have_changed)
type_t *type;
taddr_t addr;
ilist_t *ilist;
bool *p_decl_may_have_changed;
{
	enum { ARRAY_ADDR, ARRAY_CLEN, ARRAY_ELSIZE, ARRAY_FLAGS, ARRAY_DIMS };
	enum { DIM_LOW, DIM_HIGH, DIM_BASESIZE, DIM_NITEMS};
	static taddr_t *addrs;
	static size_t bufsize;
	size_t addrbytes, elsize;
	ilist_t *il;
        bool is_associated;
	int rank, ndims;
	
	if (type->ty_code != DT_PTR_TO)
		return;
	type = type->ty_base;

	rank = 0;
	for (; type->ty_code == DT_ARRAY_OF; type = type->ty_base)
		++rank;
	
	if (rank == 0)
		return;

        ndims = rank;

        /*  Character variables are represented as DT_ARRAY_OF TY_CHARACTER,
         *  but there is no dimension information for this DT_ARRAY_OF.
         */
        if (type->ty_code == TY_CHARACTER)
                --ndims;

        addrbytes = (ARRAY_DIMS + ndims * DIM_NITEMS) * sizeof(taddr_t);

	if (addrbytes > bufsize) {
		if (bufsize != 0)
			free(addrs);
		bufsize = addrbytes;
		addrs = (taddr_t *)e_malloc(bufsize);
	}

	if (dread(get_current_target(), addr, (char *)addrs, addrbytes) != 0) {
		errf("Can't read array size information: %s", get_errno_str());
		return;
	}

        is_associated = addrs[ARRAY_ADDR] != 0 &&
		        addrs[ARRAY_ADDR] != EPCF90_UNASSIGNED_PATTERN;

	/*  A scalar character variable has just [addr, len] rather than
	 *  [addr, xxx, elsize, flags].
	 */
	elsize = addrs[(ndims == 0) ? ARRAY_CLEN : ARRAY_ELSIZE];
        
        il = ilist;
	for (--ndims; ndims >= 0; --ndims) {
		taddr_t *dim;

		dim = &addrs[ARRAY_DIMS + ndims * DIM_NITEMS];
			
		set_il_range(il, is_associated,
			     (int)dim[DIM_LOW], (int)dim[DIM_HIGH],
			     (size_t)(dim[DIM_BASESIZE] * elsize),
			     p_decl_may_have_changed);
		
		il = il->il_next;
	}

	if (type->ty_code == TY_CHARACTER) {
		set_il_range(il, is_associated, 1, (int)elsize, (size_t)1,
			     p_decl_may_have_changed);
	}
	
	if (*p_decl_may_have_changed && is_associated) {
		for (il = ilist; il != NULL; il = il->il_next) {
                        if (il->il_index < il->il_low ||
                            il->il_index >= il->il_high) {
                                il->il_index = il->il_low;
                        }
                }
	}
}

static void
set_il_range(il, is_associated, low, high, basesize, p_decl_may_have_changed)
ilist_t *il;
bool is_associated;
int low, high;
size_t basesize;
bool *p_decl_may_have_changed;
{
	if (is_associated) {
		if (!il->il_low_known || il->il_low != low ||
		    !il->il_high_known || il->il_high != high ||
		    il->il_basesize != basesize) {
			il->il_low = low;
			il->il_high = high;
			il->il_basesize = basesize;
			il->il_low_known = il->il_high_known = TRUE;

			*p_decl_may_have_changed = TRUE;
		}
	}
	else {
		if (il->il_low_known || il->il_high_known) {
			il->il_low_known = il->il_high_known = FALSE;
			*p_decl_may_have_changed = TRUE;
		}
	}
}
	
static void
get_dim(xp, fp, ap, dimtype, offset,
	p_val, p_val_known, p_decl_may_have_changed)
target_t *xp;
taddr_t fp, ap;
Dimtype dimtype;
int offset, *p_val;
bool *p_val_known;
bool *p_decl_may_have_changed;
{
	int val;
	bool val_known;
	taddr_t addr;

	switch (dimtype) {
	case DT_CONSTANT:
		*p_val = offset;
		*p_val_known = TRUE;
		return;

	case DT_FPREL:
		addr = fp + offset;
		break;

	case DT_APREL:
		addr = ap + offset;
		break;

	case DT__UNKNOWN:
		*p_val_known = FALSE;
		return;

	default:
		panic("unknown dimtype in get_dim");
		addr = 0;	/* to satisfy gcc */
	}

	val_known = dread(xp, addr, (char *)&val, sizeof(int)) == 0;

	if (val_known != *p_val_known || (val_known && val != *p_val))
		*p_decl_may_have_changed = TRUE;

	*p_val_known = val_known;
	
	if (val_known)
		*p_val = val;
}

void
fix_if_f77_dynamic_array(type, fp, ap, ilist, p_decl_may_have_changed)
type_t *type;
taddr_t fp, ap;
ilist_t *ilist;
bool *p_decl_may_have_changed;
{
	target_t *xp;
	ilist_t *il;

	xp = get_current_target();
	
	il = ilist;
	for (; type != NULL; type = type->ty_base) {
		dim_t *dim;
		
		if (type->ty_code != DT_ARRAY_OF)
			continue;
		dim = type->ty_dim;
		
		if (il == NULL)
			panic("ilist botch in fifda");

		get_dim(xp, fp, ap, dim->di_ltype, dim->di_low,
			&il->il_low, &il->il_low_known,
			p_decl_may_have_changed);

		get_dim(xp, fp, ap, dim->di_htype, dim->di_high,
			&il->il_high, &il->il_high_known,
			p_decl_may_have_changed);
		
		il = il->il_next;
	}
}

type_t *
get_basetype(type)
type_t *type;
{
	while (ISDERIV(type->ty_code))
		type = type->ty_base;
	return type;
}

type_t *
get_type_at_level(v, level)
var_t *v;
int level;
{
	type_t *type;

	for (type = v->va_type; type != NULL && --level >= 0; type = type->ty_base)
		;
	if (type == NULL)
		errf("level too high in get_type_at_level");
	/* RGA		panic("level too high in get_type_at_level");*/
	return type;
}

