/* xc_symparse.c - unpack C interpreter file symbol table information */

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


/* @(#)xc_symparse.c	1.3 04 Jun 1995 (UKC) */
char ups_xc_symparse_c_rcsid[] = "$Id$";

#include <string.h>
#include <stdlib.h>

#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "xc_symparse.h"
#include "ci.h"

struct symdesc_s {
	const char *sd_iptr;
	const char *sd_typeptr;
	alloc_pool_t *sd_apool;
	const char *sd_strings;
	size_t sd_strings_size;
	type_t *sd_typetab;
	int sd_ntypes;
	char **sd_varaddrs;
	taddr_t sd_data_addr;
};

static const char *get_string PROTO((symdesc_t *sd));
static type_t *get_type PROTO((symdesc_t *sd));
static void unpack_type PROTO((symdesc_t *sd, type_t *type));
static aggr_or_enum_def_t *unpack_aggr PROTO((symdesc_t *sd, type_t *type));
static enum_member_t *unpack_enumlist PROTO((symdesc_t *sd,
					     aggr_or_enum_def_t *ae));
static block_t *unpack_block PROTO((symdesc_t *sd, block_t *par));
static block_t *unpack_blocklist PROTO((symdesc_t *sd, block_t *par));
static aggr_or_enum_def_t *unpack_aggrlist PROTO((symdesc_t *sd));
static typedef_t *unpack_typedeflist PROTO((symdesc_t *sd));
static var_t *unpack_varlist PROTO((symdesc_t *sd));
static dim_t *unpack_dim PROTO((symdesc_t *sd));
static funcret_t *unpack_funcret PROTO((symdesc_t *sd));
static bitfield_t *unpack_bitfield PROTO((symdesc_t *sd));

#define COPY(sd, v)	\
		(memcpy((char *)&(v), (sd)->sd_iptr, sizeof(v)), \
		 (sd)->sd_iptr += sizeof(v))

static const char *
get_string(sd)
symdesc_t *sd;
{
	int offset;

	COPY(sd, offset);

	if (offset == -1)
		return NULL;

	if (offset < 0 || offset >= sd->sd_strings_size)
		panic("bad offset in os");

	return alloc_strdup(sd->sd_apool, sd->sd_strings + offset);
}

static type_t *
get_type(sd)
symdesc_t *sd;
{
	int type_index;

	COPY(sd, type_index);

	if (type_index == -1)
		return NULL;

	if (type_index < 0 || type_index >= sd->sd_ntypes)
		panic("bad type index in it");

	return &sd->sd_typetab[type_index];
}

symdesc_t *
make_symdesc(ap, typeptr, strings, strings_size, ntypes,
						varaddrs, data_addr, iptr)
alloc_pool_t *ap;
const char *typeptr;
const char *strings;
size_t strings_size, ntypes;
char **varaddrs;
taddr_t data_addr;
const char *iptr;
{
	symdesc_t *sd;
	type_t *typetab;
	int i;

	sd = (symdesc_t *)e_malloc(sizeof(symdesc_t));

	typetab = (type_t *)alloc(ap, ntypes * sizeof(type_t));

	sd->sd_iptr = typeptr;
	sd->sd_typeptr = NULL;
	sd->sd_apool = ap;
	sd->sd_strings = strings;
	sd->sd_strings_size = strings_size;
	sd->sd_typetab = typetab;
	sd->sd_ntypes = ntypes;
	sd->sd_varaddrs = varaddrs;
	sd->sd_data_addr = data_addr;

	for (i = 0; i < ntypes; ++i)
		unpack_type(sd, &typetab[i]);

	if (sd->sd_iptr != strings)
		panic("typeptr botch in ms");

	sd->sd_iptr = iptr;
	sd->sd_typeptr = typeptr;

	return sd;
}

void
free_symdesc(sd)
symdesc_t *sd;
{
	if (sd->sd_iptr != sd->sd_typeptr)
		panic("types botch in fs");

	free((char *)sd);
}

static void
unpack_type(sd, type)
symdesc_t *sd;
type_t *type;
{
	typecode_t typecode;
	unsigned char codech;

	COPY(sd, codech);
	typecode = (typecode_t)(codech & TC_CODEMASK);

	switch (typecode) {
	case TY_STRUCT:
	case TY_U_STRUCT:
	case TY_UNION:
	case TY_U_UNION:
	case TY_ENUM:
	case TY_U_ENUM:
		type->ty_code = typecode;
		COPY(sd, type->ty_size);
		type->ty_base = NULL;

		unpack_aggr(sd, type);
		break;

	case TY_VOID:
	case TY_CHAR:
	case TY_UCHAR:
	case TY_SHORT:
	case TY_USHORT:
	case TY_INT:
	case TY_UINT:
	case TY_LONG:
	case TY_ULONG:
#if WANT_LL
	case TY_LONGLONG:
	case TY_ULONGLONG:
#endif
	case TY_FLOAT:
	case TY_DOUBLE:
#if WANT_LDBL
	case TY_LONGDOUBLE:
#endif
	case TY_INT_ASSUMED:
		*type = *ci_code_to_type(typecode);
		break;

	case DT_PTR_TO:
	case DT_ARRAY_OF:
	case DT_FUNC_RETURNING:
		type->ty_size = -1;
		type->ty_code = typecode;
		type->ty_base = get_type(sd);

		switch (typecode) {
		case DT_PTR_TO:
			COPY(sd, type->ty_qualifiers);
			break;
		case DT_ARRAY_OF:
			type->ty_dim = unpack_dim(sd);
			break;
		case DT_FUNC_RETURNING:
			type->ty_funcret = unpack_funcret(sd);
			break;
		default:
			panic("bad typecode in dt");
		}

		break;

	case TY_BITFIELD:
		type->ty_code = typecode;
		type->ty_size = -1;
		type->ty_bitfield = unpack_bitfield(sd);
		break;

	default:
		panic("bad type in ut");
	}

	if ((codech & TC_HAS_TYPEDEF) != 0) {
		ci_make_typedef(sd->sd_apool, get_string(sd), type);
	}
	else {
		type->ty_typedef = NULL;
	}
}

static dim_t *
unpack_dim(sd)
symdesc_t *sd;
{
	dim_t *dim;

	dim = (dim_t *)alloc(sd->sd_apool, sizeof(dim_t));
	dim->di_ltype = DT_CONSTANT;
	COPY(sd, dim->di_htype);
	dim->di_low = 0;
	COPY(sd, dim->di_high);
	dim->di_type = ci_code_to_type(TY_INT);

	return dim;
}

static funcret_t *
unpack_funcret(sd)
symdesc_t *sd;
{
	funcret_t *fr;

	fr = (funcret_t *)alloc(sd->sd_apool, sizeof(funcret_t));
	COPY(sd, fr->fr_params_type);
	COPY(sd, fr->fr_nparams);
	COPY(sd, fr->fr_is_variadic);
	COPY(sd, fr->fr_is_old_style);
	fr->fr_params = unpack_varlist(sd);

	return fr;
}

static bitfield_t *
unpack_bitfield(sd)
symdesc_t *sd;
{
	bitfield_t *bf;
#if 1
	typecode_t typecode;
#endif

	bf = (bitfield_t *)alloc(sd->sd_apool, sizeof(bitfield_t));
#if 0
	COPY(sd, bf->bf_type->ty_code);
#else
	/* Fix : Dibyendu : 25/12/98 */
	COPY(sd, typecode);
	bf->bf_type = ci_code_to_type(typecode);
#endif
	COPY(sd, bf->bf_offset);
	COPY(sd, bf->bf_width);

	return bf;
}

static aggr_or_enum_def_t *
unpack_aggr(sd, type)
symdesc_t *sd;
type_t *type;
{
	aggr_or_enum_def_t *ae;

	ae = (aggr_or_enum_def_t *)alloc(sd->sd_apool,
						sizeof(aggr_or_enum_def_t));

	ae->ae_tag = get_string(sd);
	COPY(sd, ae->ae_is_complete);
	COPY(sd, ae->ae_size);
	COPY(sd, ae->ae_alignment);

	ae->ae_type = type;
	type->ty_aggr_or_enum = ae;

	switch (ae->ae_type->ty_code) {
	case TY_STRUCT:
	case TY_UNION:
		ae->ae_aggr_members = unpack_varlist(sd);
		break;

	case TY_ENUM:
		ae->ae_enum_members = unpack_enumlist(sd, ae);
		break;

	case TY_U_ENUM:
		ae->ae_enum_members = NULL;
		break;

	case TY_U_STRUCT:
	case TY_U_UNION:
		ae->ae_aggr_members = NULL;
		break;

	default:
		panic("ae botch in ua");
	}

	ae->ae_lexinfo = NULL;

	return ae;
}

static enum_member_t *
unpack_enumlist(sd, ae)
symdesc_t *sd;
aggr_or_enum_def_t *ae;
{
	enum_member_t first, *last;
	int i, count;

	COPY(sd, count);

	last = &first;
	for (i = 0; i < count; ++i) {
		enum_member_t *em;

		em = (enum_member_t *)alloc(sd->sd_apool,sizeof(enum_member_t));
		em->em_name = get_string(sd);
		COPY(sd, em->em_val);

		em->em_expr_id = NULL;
		em->em_enum = ae;
		em->em_lexinfo = NULL;

		last->em_next = em;
		last = em;
	}
	last->em_next = NULL;

	return first.em_next;
}

block_t *
unpack_one_block(sd)
symdesc_t *sd;
{
	block_t *bl;

	bl = unpack_block(sd, (block_t *)NULL);
	bl->bl_next = NULL;
	return bl;
}

static block_t *
unpack_block(sd, par)
symdesc_t *sd;
block_t *par;
{
	block_t *bl;

	bl = ci_make_block(sd->sd_apool, par);

	COPY(sd, bl->bl_start_lnum);
	COPY(sd, bl->bl_end_lnum);
	bl->bl_aggr_or_enum_defs = unpack_aggrlist(sd);
	bl->bl_typedefs = unpack_typedeflist(sd);
	bl->bl_vars = unpack_varlist(sd);
	/* bl->bl_blocks = unpack_blocklist(sd, par); */
	/* FIX: Dibyendu: 1 april 1999 */
	bl->bl_blocks = unpack_blocklist(sd, bl);
	bl->bl_parent = par;

	return bl;
}

static block_t *
unpack_blocklist(sd, par)
symdesc_t *sd;
block_t *par;
{
	block_t first, *last;
	int i, count;

	COPY(sd, count);

	last = &first;
	for (i = 0; i < count; ++i) {
		block_t *newbl;

		newbl = unpack_block(sd, par);
		last->bl_next = newbl;
		last = newbl;
	}
	last->bl_next = NULL;

	return first.bl_next;
}

static aggr_or_enum_def_t *
unpack_aggrlist(sd)
symdesc_t *sd;
{
	int i, count;
	aggr_or_enum_def_t first, *last;

	COPY(sd, count);

	last = &first;
	for (i = 0; i < count; ++i) {
		type_t *type;
		aggr_or_enum_def_t *ae;

		type = get_type(sd);

		ae = type->ty_aggr_or_enum;
		if (ae->ae_type != type)
			panic("aggr botch in ua");

		last->ae_next = ae;
		last = ae;
	}
	last->ae_next = NULL;

	return first.ae_next;
}

static typedef_t *
unpack_typedeflist(sd)
symdesc_t *sd;
{
	int i, count;
	typedef_t first, *last;

	COPY(sd, count);

	last = &first;
	for (i = 0; i < count; ++i) {
		type_t *type;
		typedef_t *td;

		type = get_type(sd);

		td = type->ty_typedef;
		if (td->td_type != type)
			panic("typedef botch in ut");

		last->td_next = td;
		last = td;
	}
	last->td_next = NULL;

	return first.td_next;
}

static var_t *
unpack_varlist(sd)
symdesc_t *sd;
{
	int i, count;
	var_t first, *last;

	COPY(sd, count);

	last = &first;
	for (i = 0; i < count; ++i) {
		const char *name;
		int class_and_extflag;
		class_t class;
		taddr_t addr;
		type_t *type;

		name = get_string(sd);
		COPY(sd, class_and_extflag);
		COPY(sd, addr);
		type = get_type(sd);

		if (class_and_extflag >= 0) {
			class = (class_t)class_and_extflag;
			
			if (class == CL_LSTAT || class == CL_STAT)
				addr += sd->sd_data_addr;
		}
		else {
			class = (class_t)-class_and_extflag;
			addr = (taddr_t)sd->sd_varaddrs[addr];
		}

		if (class != CL_DECL) {
			var_t *v;

			v = ci_make_var(sd->sd_apool, name, class, type, addr);

			last->va_next = v;
			last = v;
		}
	}
	last->va_next = NULL;

	return first.va_next;
}
