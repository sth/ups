/* cx_wsym.c - write out debugging symbol table information */

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


/* @(#)cx_wsym.c	1.4 20 Jun 1995 (UKC) */
char ups_cx_wsym_c_rcsid[] = "$Id$";

#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <local/ukcprog.h>
#include <mtrprog/hash.h>

#include "ups.h"
#include "symtab.h"
#include "cx_symio.h"
#include "cx_wsym.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#endif

struct syminfo_s {
	alloc_pool_t *si_apool;
	hashtab_t *si_ht;	/* type->index hash table */
	type_t **si_types;	/* type array filled in by write_types */
	size_t si_ntypes;		/* # types currently in hash table */
	FILE *si_fp;		/* Symbol table file */
	ebuf_t *si_eb;		/* String output buffer */
};

static int write_aggr PROTO((syminfo_t *si, aggr_or_enum_def_t *ae));
static int write_enum_members PROTO((syminfo_t *si, enum_member_t *emlist));
static int write_aggrlist PROTO((syminfo_t *si, aggr_or_enum_def_t *aelist));
static int write_typedeflist PROTO((syminfo_t *si, typedef_t *tdlist));
static int write_blocklist PROTO((syminfo_t *si, block_t *blocklist));
static int type_to_index PROTO((syminfo_t *si, type_t *type));
static int write_varlist PROTO((syminfo_t *si, var_t *varlist));
static const char *enter_type PROTO((hashtab_t *ht, const char *arg,
				     hash_key_t key, size_t keylen,
				     hashvalues_t *hv));
static int write_type PROTO((syminfo_t *si, type_t *type));
static int write_dim PROTO((syminfo_t *si, dim_t *dim));
static int write_funcret PROTO((syminfo_t *si, funcret_t *fr));
static int write_bitfield PROTO((syminfo_t *si, bitfield_t *bf));

#define WRITEVAL(si, v) 	fp_write((si)->si_fp, (char *)&(v), sizeof(v))

syminfo_t *
make_syminfo(ap, fp, eb)
alloc_pool_t *ap;
FILE *fp;
ebuf_t *eb;
{
	syminfo_t *si;

	si = (syminfo_t *)alloc(ap, sizeof(syminfo_t));

	si->si_ht = hash_create_tab(ap, 1500);
	si->si_ntypes = 0;
	si->si_types = NULL;
	si->si_fp = fp;
	si->si_eb = eb;
	si->si_apool = ap;

	return si;
}

int
write_block(si, bl)
syminfo_t *si;
block_t *bl;
{
	if (WRITEVAL(si, bl->bl_start_lnum) != 0 ||
	    WRITEVAL(si, bl->bl_end_lnum) != 0 ||
	    write_aggrlist(si, bl->bl_aggr_or_enum_defs) != 0 ||
	    write_typedeflist(si, bl->bl_typedefs) ||
	    write_varlist(si, bl->bl_vars) != 0 ||
	    write_blocklist(si, bl->bl_blocks) != 0)
		return -1;
	
	return 0;
}

static int
write_aggr(si, ae)
syminfo_t *si;
aggr_or_enum_def_t *ae;
{
	int name_offset;

	name_offset = note_string(si->si_eb, ae->ae_tag);

	if (WRITEVAL(si, name_offset) != 0 ||
	    WRITEVAL(si, ae->ae_is_complete) != 0 ||
	    WRITEVAL(si, ae->ae_size) != 0 ||
	    WRITEVAL(si, ae->ae_alignment) != 0)
		return -1;

	switch (ae->ae_type->ty_code) {
	case TY_STRUCT:
	case TY_UNION:
		return write_varlist(si, ae->ae_aggr_members);

	case TY_ENUM:
		return write_enum_members(si, ae->ae_enum_members);

	case TY_U_ENUM:
		if (ae->ae_enum_members != NULL)
			panic("enum_botch in wa");
		return 0;

	case TY_U_STRUCT:
	case TY_U_UNION:
		if (ae->ae_aggr_members != NULL)
			panic("aggr botch in wa");
		return 0;

	default:
		panic("ae botch in wa");
		return -1;	/* to satisfy gcc */
	}
}

static int
write_enum_members(si, emlist)
syminfo_t *si;
enum_member_t *emlist;
{
	enum_member_t *em;
	int count;

	count = 0;
	for (em = emlist; em != NULL; em = em->em_next)
		++count;

	if (WRITEVAL(si, count) != 0)
		return -1;

	for (em = emlist; em != NULL; em = em->em_next) {
		int name_offset;

		name_offset = note_string(si->si_eb, em->em_name);

		if (WRITEVAL(si, name_offset) != 0 ||
		    WRITEVAL(si, em->em_val) != 0)
			return -1;
	}

	return 0;
}

static int
write_aggrlist(si, aelist)
syminfo_t *si;
aggr_or_enum_def_t *aelist;
{
	aggr_or_enum_def_t *ae;
	int count;

	count = 0;
	for (ae = aelist; ae != NULL; ae = ae->ae_next)
		++count;

	if (WRITEVAL(si, count) != 0)
		return -1;

	for (ae = aelist; ae != NULL; ae = ae->ae_next) {
		int type_index;

		type_index = type_to_index(si, ae->ae_type);

		if (WRITEVAL(si, type_index) != 0)
			return -1;
	}

	return 0;
}

static int
write_typedeflist(si, tdlist)
syminfo_t *si;
typedef_t *tdlist;
{
	typedef_t *td;
	int count;

	count = 0;
	for (td = tdlist; td != NULL; td = td->td_next)
		++count;

	if (WRITEVAL(si, count) != 0)
		return -1;

	for (td = tdlist; td != NULL; td = td->td_next) {
		int type_index;

		type_index = type_to_index(si, td->td_type);

		if (WRITEVAL(si, type_index) != 0)
			return -1;
	}

	return 0;
}

static int
write_blocklist(si, blocklist)
syminfo_t *si;
block_t *blocklist;
{
	block_t *bl;
	int count;

	count = 0;
	for (bl = blocklist; bl != NULL; bl = bl->bl_next)
		++count;

	if (WRITEVAL(si, count) != 0)
		return -1;

	for (bl = blocklist; bl != NULL; bl = bl->bl_next) {
		if (write_block(si, bl) != 0)
			return -1;
	}

	return 0;
}

static int
type_to_index(si, type)
syminfo_t *si;
type_t *type;
{
	const char *ti;

	if (type == NULL)
		return -1;

	ti = hash_lookup(si->si_ht, (hash_key_t)&type, sizeof(type_t *));

	if (ti == NULL) {
		type_t **p_type;
		var_t *v;

		ti = (char *)si + si->si_ntypes++;

		p_type = (type_t **)alloc(si->si_apool, sizeof(type_t *));
		*p_type = type;

		hash_enter(si->si_ht, (hash_key_t)p_type, sizeof(type_t *), ti);

		/*  Recursively enter any types referred to by this type.
		 */
		switch (type->ty_code) {
		case DT_FUNC_RETURNING:
			for (v = type->ty_funcret->fr_params;
						v != NULL; v = v->va_next)
				type_to_index(si, v->va_type);
			type_to_index(si, type->ty_base);
			break;
			
		case DT_ARRAY_OF:
			type_to_index(si, type->ty_base);
			break;

		case DT_PTR_TO:
			type_to_index(si, type->ty_base);
			break;

		case TY_STRUCT:
		case TY_UNION:
			for (v = type->ty_aggr_or_enum->ae_aggr_members;
						v != NULL; v = v->va_next)
				type_to_index(si, v->va_type);
			break;

		default:
			break;
		}
	}

	return ti - (char *)si;
}

static int
write_varlist(si, varlist)
syminfo_t *si;
var_t *varlist;
{
	var_t *v;
	int count;

	count = 0;
	for (v = varlist; v != NULL; v = v->va_next)
		++count;

	if (WRITEVAL(si, count) != 0)
		return -1;

	for (v = varlist; v != NULL; v = v->va_next) {
		int name_offset, type_index, class_and_extflag;

		name_offset = note_string(si->si_eb, v->va_name);
		type_index = type_to_index(si, v->va_type);

		class_and_extflag = (int)v->va_class;
		if ((v->va_flags & VA_EXT_RELOC) != 0)
			class_and_extflag = -class_and_extflag;

		if (WRITEVAL(si, name_offset) != 0 ||
		    WRITEVAL(si, class_and_extflag) != 0 ||
		    WRITEVAL(si, v->va_addr) != 0 ||
		    WRITEVAL(si, type_index) != 0)
			return -1;
	}

	return 0;
}

static const char *
enter_type(ht, arg, key, keylen, hv)
hashtab_t *ht;
const char *arg;
hash_key_t key;
size_t keylen;
hashvalues_t *hv;
{
	const char *base, *ti;
	syminfo_t *si;

	si = (syminfo_t *)arg;

	if (keylen != sizeof(type_t *) || hv->hv_nvalues != 1)
		panic("hash botch in haf");

	base = (char *)si;
	ti = *hv->hv_values;

	if (ti < base || ti >= base + si->si_ntypes)
		panic("type botch in haf");

	memcpy((char *)&si->si_types[ti - base], (char *)key, keylen);

	return NULL;
}

int
write_types(si, p_ntypes)
syminfo_t *si;
size_t *p_ntypes;
{
	hashvalues_t *hv;
	int res;
	type_t **p_type, **typetab, **lim;

	typetab = (type_t **)e_malloc(si->si_ntypes * sizeof(type_t *));
	lim = typetab + si->si_ntypes;

	hv = hash_make_hashvalues();
	si->si_types = typetab;
	hash_apply(si->si_ht, enter_type, (char *)si, hv);
	si->si_types = NULL;
	hash_free_hashvalues(hv);

	res = 0;
	for (p_type = typetab; p_type < lim; ++p_type) {
		if (write_type(si, *p_type) != 0) {
			res = -1;
			break;
		}
	}

	free((char *)typetab);

	*p_ntypes = si->si_ntypes;
	return res;
}

static int
write_type(si, type)
syminfo_t *si;
type_t *type;
{
	int base_index;
	unsigned char codech;

	codech = (char)type->ty_code;

	if ((typecode_t)(codech & TC_CODEMASK) != type->ty_code)
		panic("typecode botch");

	if (type->ty_typedef != NULL)
		codech |= TC_HAS_TYPEDEF;

	if (WRITEVAL(si, codech))
		return -1;

	switch (type->ty_code) {
	case TY_STRUCT:
	case TY_U_STRUCT:
	case TY_UNION:
	case TY_U_UNION:
	case TY_ENUM:
	case TY_U_ENUM:
		if (type->ty_base != NULL)
			panic("aggr base botch in wt");

		if (WRITEVAL(si, type->ty_size) != 0 ||
		    write_aggr(si, type->ty_aggr_or_enum) != 0)
			return -1;
		break;

	case TY_VOID:
	case TY_CHAR:
	case TY_UCHAR:
	case TY_SHORT:
	case TY_USHORT:
	case TY_INT:
	case TY_UINT:
	case TY_BOOLEAN:
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
		if (type->ty_base != NULL)
			panic("base botch in wt");

		/*  No need to write out the type name - the reader can
		 *  invent it.
		 */
		break;

	case DT_PTR_TO:
	case DT_ARRAY_OF:
	case DT_FUNC_RETURNING:
		base_index = type_to_index(si, type->ty_base);

		if (WRITEVAL(si, base_index) != 0)
			return -1;

		switch (type->ty_code) {
		case DT_PTR_TO:
			if (WRITEVAL(si, type->ty_qualifiers) != 0)
				return -1;
			break;
		case DT_ARRAY_OF:
			if (write_dim(si, type->ty_dim) != 0)
				return -1;
			break;
		case DT_FUNC_RETURNING:
			if (write_funcret(si, type->ty_funcret) != 0)
				return -1;
			break;
		default:
			panic("bad typecode in wdt");
		}
		break;

	case TY_BITFIELD:
		if (write_bitfield(si, type->ty_bitfield) != 0)
			return -1;
		break;

	default:
		panic("bad typecode in wt");
		break;
	}

	if (type->ty_typedef != NULL) {
		int name_offset;

		name_offset = note_string(si->si_eb, type->ty_typedef->td_name);

		if (WRITEVAL(si, name_offset) != 0)
			return -1;
	}

	return 0;
}

static int
write_dim(si, dim)
syminfo_t *si;
dim_t *dim;
{
	if (dim->di_ltype != DT_CONSTANT ||
	    dim->di_low != 0 ||
	    dim->di_type->ty_code != TY_INT)
		panic("dim botch in wd");

	if (WRITEVAL(si, dim->di_htype) != 0 ||
	    WRITEVAL(si, dim->di_high) != 0)
		return -1;

	return 0;
}

static int
write_funcret(si, fr)
syminfo_t *si;
funcret_t *fr;
{
	if (WRITEVAL(si, fr->fr_params_type) != 0 ||
	    WRITEVAL(si, fr->fr_nparams) != 0 ||
	    WRITEVAL(si, fr->fr_is_variadic) != 0 ||
	    WRITEVAL(si, fr->fr_is_old_style) != 0 ||
	    write_varlist(si, fr->fr_params) != 0)
		return -1;

	return 0;
}

static int
write_bitfield(si, bf)
syminfo_t *si;
bitfield_t *bf;
{
	if (WRITEVAL(si, bf->bf_type->ty_code) != 0 ||
	    WRITEVAL(si, bf->bf_offset) != 0 ||
	    WRITEVAL(si, bf->bf_width) != 0)
		return -1;

	return 0;
}

