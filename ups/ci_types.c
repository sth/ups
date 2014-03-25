/* ci_types.c - type checking etc for the C parser */

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


/* @(#)ci_types.c	1.18 04 Jun 1995 (UKC) */
char ups_ci_types_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <stdlib.h>

#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "ci_parse.h"
#include "ci_util.h"
#include "ci_types.h"

static const char *nametype_to_string PROTO((nametype_t nametype));
static bool params_same PROTO((funcret_t *fr1, funcret_t *fr2));
static char *param_types PROTO((type_t *type));
static const char *qualifiers_to_string PROTO((qualifiers_t qualifiers));
static typecode_t flags_to_typecode PROTO((unsigned typeflags));
static unsigned typecode_to_flag PROTO((typecode_t typecode,
							bool alltypes_ok));
static const char *typeflag_to_name PROTO((unsigned flag));
static void complain_about_bad_type PROTO((unsigned flags));

#define CHAR_SIZE	sizeof(char)
#define SHORT_SIZE	sizeof(short)
#define INT_SIZE	sizeof(int)
#define LONG_SIZE	sizeof(long)
#define FLOAT_SIZE	sizeof(float)
#define DOUBLE_SIZE	sizeof(double)
#define ENUM_SIZE	sizeof(typecode_t)	/* or any enum */
#define POINTER_SIZE	sizeof(int *)
/*
 * If 'long long' or 'long double' support has been requested for the
 * interpreter then use the types decided in <ci.h>, otherwise use
 * values from <ifdefs.h>.  This is to allow non-interpreter code to
 * call some routines here when debugging "long long" variables in
 * programs.
 */
#if WANT_LL
#define LONG_LONG_SIZE	sizeof(long_long_t)
#elif HAVE_LONG_LONG
#define LONG_LONG_SIZE	SIZEOF_LONG_LONG
#else
#define LONG_LONG_SIZE	sizeof(long)
#endif
#if WANT_LDBL
#define LONG_DBL_SIZE	sizeof(long_double_t)
#elif HAVE_LONG_DOUBLE
#define LONG_DBL_SIZE	SIZEOF_LONG_DOUBLE
#else
#define LONG_DBL_SIZE	sizeof(double)
#endif

#define CHAR_ALIGN	CHAR_SIZE
#define SHORT_ALIGN	SHORT_SIZE
#define INT_ALIGN	INT_SIZE
#define LONG_ALIGN	LONG_SIZE
#define POINTER_ALIGN	POINTER_SIZE
#define FLOAT_ALIGN	FLOAT_SIZE
#define DOUBLE_ALIGN	FLOAT_SIZE	/* BUG: wrong (esp. on SPARC) */
#define LONG_LONG_ALIGN	LONG_LONG_SIZE	/* XXX: guess */
#define LONG_DBL_ALIGN	LONG_DBL_SIZE	/* XXX: guess */

#define WORD_ALIGN	LONG_ALIGN
#define MAX_ALIGN	DOUBLE_ALIGN	/* XXX: or LONG_DBL_ALIGN ? */

#define TF_TYPE_DONE	0x0001
#define TF_VOID		0x0002
#define TF_CHAR		0x0004
#define TF_SHORT	0x0008
#define TF_INT		0x0010
#define TF_LONG		0x0020
#define TF_FLOAT	0x0040
#define TF_DOUBLE	0x0080
#define TF_SIGNED	0x0100
#define TF_UNSIGNED	0x0200
#if WANT_LL
#define TF_LONG_LONG	0x0400
#endif
#if WANT_LDBL
#define TF_LONG_DBL	0x0800
#endif
#define TF_ENUM		0x1000
#define TF_STRUCT	0x2000
#define TF_UNION	0x4000

#define TF_MAX_MASK	TF_UNION

#define TF_ENUM_OR_AGGR	(TF_ENUM | TF_STRUCT | TF_UNION)

int
ci_type_alignment(type)
type_t *type;
{
	switch(ci_effective_typecode(type->ty_code)) {
	case TY_U_ENUM:
	case TY_ENUM:
	case DT_PTR_TO:
		return WORD_ALIGN;
	case TY_STRUCT:
	case TY_UNION:
		return type->ty_aggr_or_enum->ae_alignment;
	case DT_ARRAY_OF:
		return ci_type_alignment(type->ty_base);
	case TY_LONG:
	case TY_ULONG:
		return LONG_ALIGN;
	case TY_LONGLONG:
	case TY_ULONGLONG:
		return LONG_LONG_ALIGN;
	case TY_BITFIELD:
		if (type->ty_size > 0)
		    return type->ty_size;
		else if (type->ty_base)
		    return ci_type_alignment(type->ty_base);
		return INT_ALIGN;
	case TY_INT:
	case TY_UINT:
	case TY_BOOLEAN:
	case TY_INT_ASSUMED:
		return INT_ALIGN;
	case TY_SHORT:
	case TY_USHORT:
		return SHORT_ALIGN;
	case TY_CHAR:
	case TY_UCHAR:
		return CHAR_ALIGN;
	case TY_FLOAT:
		return FLOAT_ALIGN;
	case TY_DOUBLE:
		return DOUBLE_ALIGN;
	case TY_LONGDOUBLE:
		return LONG_DBL_ALIGN;
	default:
		ci_panic("bad type in ta");
		return 0; /* to satisfy gcc */
	}
}

optype_t
ci_typecode_to_cvt_op(typecode)
typecode_t typecode;
{
	switch (ci_effective_typecode(typecode)) {
		case TY_CHAR:	return OP_CVT_TO_CHAR;
		case TY_UCHAR:	return OP_CVT_TO_UCHAR;
		case TY_SHORT:	return OP_CVT_TO_SHORT;
		case TY_USHORT:	return OP_CVT_TO_USHORT;
		case TY_INT:	return OP_CVT_TO_INT;
		case TY_U_ENUM:	return OP_CVT_TO_INT;
		case TY_ENUM:	return OP_CVT_TO_INT;
		case TY_UINT:	return OP_CVT_TO_UINT;
		case TY_LONG:	return OP_CVT_TO_LONG;
		case TY_ULONG:	return OP_CVT_TO_ULONG;
#if WANT_LL
		case TY_LONGLONG:	return OP_CVT_TO_LONG_LONG;
		case TY_ULONGLONG:	return OP_CVT_TO_ULONG_LONG;
#endif
		case TY_FLOAT:	return OP_CVT_TO_FLOAT;
		case TY_VOID:	return OP_CVT_TO_VOID;
		case TY_DOUBLE:	return OP_CVT_TO_DOUBLE;
#if WANT_LDBL
		case TY_LONGDOUBLE:	return OP_CVT_TO_LONG_DOUBLE;
#endif
		case DT_PTR_TO:	return OP_CVT_TO_PTR;
		case TY_BOOLEAN:return OP_CVT_TO_INT;
		default:
			ci_panic("bad type in pc");
			return OP_LAST; /* to satisfy gcc */
	}
}

taddr_t
ci_align_addr(addr, alignment)
taddr_t addr;
int alignment;
{
	int pad;

	pad = alignment - addr % alignment;
	return (pad == alignment) ? addr : addr + pad;
}
	
taddr_t
ci_align_addr_for_type(addr, type)
taddr_t addr;
type_t *type;
{
	int alignment;

	alignment = (type != NULL) ? ci_type_alignment(type) : MAX_ALIGN;
	return ci_align_addr(addr, alignment);
}

static void
complain_about_bad_type(flags)
unsigned flags;
{
	unsigned mask;
	int i, count;
	const char *typeword[20];
	char *s;

	if ((flags & (TF_SIGNED | TF_INT)) == (TF_SIGNED | TF_INT))
		flags &= ~TF_INT;

	count = 0;
	for (mask = 1; mask <= TF_MAX_MASK; mask <<= 1) {
		if (flags & mask)
			typeword[count++] = typeflag_to_name(mask);
	}

	if (count == 2) {
		diagf(ET_ERROR, (lexinfo_t *)NULL,
		      "Both `%s' and `%s' given in declaration",
						typeword[0], typeword[1]);
		return;
	}

	if (count < 3)
		panic("count botch in cabt");
	
	s = strf("`%s'", typeword[0]);

	for (i = 1; i < count - 1; ++i) {
		char *new;

		new = strf("%s, `%s'", s, typeword[i]);
		free(s);
		s = new;
	}
	
	diagf(ET_ERROR, (lexinfo_t *)NULL,
	      "%s and `%s' given in declaration", s, typeword[i]);
	free(s);
}

type_t *
ci_get_dn_basetype(dn)
declaration_t *dn;
{
	if ((dn->dn_typeflags & TF_TYPE_DONE) == 0) {
		typecode_t typecode;

		switch (typecode = flags_to_typecode(dn->dn_typeflags)) {
		case TY_NOTYPE:
			complain_about_bad_type(dn->dn_typeflags);
			dn->dn__basetype = NULL;
			break;
		case TY_STRUCT:
			break;
		default:
			dn->dn__basetype = ci_code_to_type(typecode);
		}

		dn->dn_typeflags |= TF_TYPE_DONE;
	}

	return dn->dn__basetype;
}
	
static const char *
typeflag_to_name(flag)
unsigned flag;
{
	switch (flag) {
		case TF_VOID:		return "void";
		case TF_CHAR:		return "char";
		case TF_SHORT:		return "short";
		case TF_INT:		return "int";
		case TF_LONG:		return "long";
#if WANT_LL
		case TF_LONG_LONG:	return "long long";
#endif
		case TF_FLOAT:		return "float";
		case TF_DOUBLE:		return "double";
#if WANT_LDBL
		case TF_LONG_DBL:	return "";		/* XXX: see call */
#endif
		case TF_SIGNED:		return "signed";
		case TF_UNSIGNED:	return "unsigned";
		case TF_ENUM:		return "enum";
		case TF_STRUCT:		return "struct";
		case TF_UNION:		return "union";
		default:		panic("bad flag in ttn");
	}

	return NULL;	/* to satisfy gcc */
}

static typecode_t
flags_to_typecode(typeflags)
unsigned typeflags;
{
	switch (typeflags) {
	case TF_VOID:
		return TY_VOID;

	case TF_CHAR:
	case TF_SIGNED | TF_CHAR:
		/* BUG: `char' and `signed char' are distinct types.
		 */
		return TY_CHAR;

	case TF_UNSIGNED | TF_CHAR:
		return TY_UCHAR;
	
	case TF_SHORT:
	case TF_SIGNED | TF_SHORT:
	case TF_SHORT | TF_INT:
	case TF_SIGNED | TF_SHORT | TF_INT:
		return TY_SHORT;
	
	case TF_UNSIGNED | TF_SHORT:
	case TF_UNSIGNED | TF_SHORT | TF_INT:
		return TY_USHORT;

	case TF_INT:
	case TF_SIGNED:
	case TF_SIGNED | TF_INT:
		return TY_INT;
	
	case TF_UNSIGNED:
	case TF_UNSIGNED | TF_INT:
		return TY_UINT;
	
	case TF_LONG:
	case TF_SIGNED | TF_LONG:
	case TF_LONG | TF_INT:
	case TF_SIGNED | TF_LONG | TF_INT:
		return TY_LONG;

#if WANT_LL
	case TF_LONG_LONG:
	case TF_SIGNED | TF_LONG_LONG:
	case TF_LONG_LONG | TF_INT:
	case TF_SIGNED | TF_LONG_LONG | TF_INT:
		return TY_LONGLONG;
#endif

	case TF_UNSIGNED | TF_LONG:
	case TF_UNSIGNED | TF_LONG | TF_INT:
		return TY_ULONG;
	
#if WANT_LL
	case TF_UNSIGNED | TF_LONG_LONG:
	case TF_UNSIGNED | TF_LONG_LONG | TF_INT:
		return TY_ULONGLONG;
#endif

	case TF_FLOAT:
		return TY_FLOAT;

	case TF_DOUBLE:
#if WANT_LDBL
		return TY_DOUBLE;
#endif
	case TF_LONG | TF_DOUBLE:
#if WANT_LDBL
	case TF_LONG_DBL:
		return TY_LONGDOUBLE;
#else
		/* BUG: 'long double' and 'double' are distinct types 
		 */
		return TY_DOUBLE;
#endif
	
	case TF_ENUM:
	case TF_STRUCT:
	case TF_UNION:
		return TY_STRUCT;	/* see ci_get_dn_basetype() */
	
	case 0:
		diagf(ET_WARNING, (lexinfo_t *)NULL, "Type defaults to int");
		return TY_INT;

	default:
		return TY_NOTYPE;
	}
}

static unsigned
typecode_to_flag(typecode, alltypes_ok)
typecode_t typecode;
bool alltypes_ok;
{
	switch (ci_effective_typecode(typecode)) {
	case TY_VOID:
		return TF_VOID;
	case TY_CHAR:
		return TF_CHAR;
	case TY_SHORT:
		return TF_SHORT;
	case TY_BOOLEAN:
	case TY_INT:
		return TF_INT;
	case TY_LONG:
		return TF_LONG;
#if WANT_LL
	case TY_LONGLONG:
		return TF_LONG_LONG;
#endif
	case TY_FLOAT:
		return TF_FLOAT;
	case TY_DOUBLE:
		return TF_DOUBLE;
#if WANT_LDBL
	case TY_LONGDOUBLE:
		return TF_LONG_DBL;
#endif
	case TY_SIGNED:
		return TF_SIGNED;
	case TY_UNSIGNED:
		return TF_UNSIGNED;
	case TY_STRUCT:
	case TY_U_STRUCT:
		return TF_STRUCT;
	case TY_UNION:
	case TY_U_UNION:
		return TF_UNION;
	case TY_ENUM:
	case TY_U_ENUM:
		return TF_ENUM;
	default:
		if (alltypes_ok)
			return TF_TYPE_DONE;
		panic("bad typecode in ctt");	
		return 0;	/* to satisfy gcc */
	}
}

declaration_t *
ci_make_declaration(class, basetype, qualifiers, declarators)
class_t class;
type_t *basetype;
qualifiers_t qualifiers;
declarator_t *declarators;
{
	unsigned typeflags;
	declaration_t *dn = NEW(declaration_t);

	if (basetype == NULL) {
		basetype = ci_code_to_type(TY_INT);
		typeflags = 0;
	}
	else {
		typeflags = typecode_to_flag(basetype->ty_code, TRUE);
	}

	dn->dn_class = class;
	dn->dn__basetype = basetype;
	dn->dn_typeflags = typeflags;
	dn->dn_dup_typeflags = 0;
	dn->dn_qualifiers = qualifiers;
	dn->dn_declarators = declarators;
	dn->dn_next = NULL;

	return dn;
}

void
ci_add_type_specifier(dn, modtype)
declaration_t *dn;
type_t *modtype;
{
	unsigned flag;

	flag = typecode_to_flag(modtype->ty_code, FALSE);

#if WANT_LL
	/* Fudge for 'long long'. */
	if (((dn->dn_typeflags & TF_LONG) != 0) && (flag == TF_LONG)) {
	    /* Note - clear TF_LONG bit. */
	    dn->dn_typeflags &= (~TF_LONG);
	    flag = TF_LONG_LONG;
	}
#endif

	if ((dn->dn_typeflags & flag) != 0 &&
					(dn->dn_dup_typeflags & flag) == 0) {
		diagf(ET_WARNING, (lexinfo_t *)NULL,
		      "Duplicate use of `%s' in declaration",
					typeflag_to_name(flag));
		dn->dn_dup_typeflags |= flag;
	}

	dn->dn_typeflags |= flag;
}

/*  Return the size in bytes of type.
 */
long
ci_typesize(lx, type)
lexinfo_t *lx;
type_t *type;
{
	if (type->ty_size == -1) {
		long size;

		switch(ci_effective_typecode(type->ty_code)) {
		case DT_PTR_TO:
			size = POINTER_SIZE;
			break;
		case DT_ARRAY_OF:
			size = (type->ty_dim->di_high + 1) *
						ci_typesize(lx, type->ty_base);
			break;
		case DT_FUNC_RETURNING:
/* RGA			diagf(ET_ERROR, lx, "Sizeof <function> undefined");
			size = -1;*/
			size = 0;
			break;
		case TY_VOID:
			diagf(ET_ERROR, lx, "Sizeof void undefined");
			size = -1;
			break;
		case TY_U_STRUCT:
		case TY_U_UNION:
/*	RGA		diagf(ET_ERROR, lx,
			      "Sizeof <%s> undefined", ci_basetype_name(type));
			size = -1;*/
			size = 0;
			break;
		case TY_STRUCT:
		case TY_UNION:
			size = type->ty_aggr_or_enum->ae_size;
			break;
		case TY_U_ENUM:
		case TY_ENUM:
			size = ENUM_SIZE;
			break;
		case TY_LONG:
		case TY_ULONG:
			size = LONG_SIZE;
			break;
		case TY_LONGLONG:
		case TY_ULONGLONG:
			size = LONG_LONG_SIZE;
			break;
		case TY_INT:
		case TY_UINT:
		case TY_BITFIELD:
		case TY_BOOLEAN:
			size = INT_SIZE;
			break;
		case TY_SHORT:
		case TY_USHORT:
			size = SHORT_SIZE;
			break;
		case TY_CHAR:
		case TY_UCHAR:
			size = CHAR_SIZE;
			break;
		case TY_FLOAT:
			size = FLOAT_SIZE;
			break;
		case TY_DOUBLE:
			size = DOUBLE_SIZE;
			break;
#if WANT_LDBL
		case TY_LONGDOUBLE:
			size = LONG_DBL_SIZE;
			break;
#endif
		default:
/*			ci_panic("unknown typecode in ts");*/
/*			size = -1;*/ /* to satisfy gcc */
			errf("unknown typecode in ts");
			size = 0;
	}
		type->ty_size = size;
	}
	return type->ty_size;
}

static const char *
nametype_to_string(nametype)
nametype_t nametype;
{
	switch(nametype) {
	case NT_VARNAME:
		return "variable";
	case NT_TAG:
		return "tag";
	case NT_ENUM_CONST:
		return "enum constant";
	case NT_TYPEDEF_NAME:
		return "typedef name";
	case NT_FUNCNAME:
		return "function name";
	default:
		return "<internal error - unknown type>";
	}
}

void
report_redecl(name, nametype, lx, prev_nametype, prev_lx)
const char *name;
nametype_t nametype;
lexinfo_t *lx;
nametype_t prev_nametype;
lexinfo_t *prev_lx;
{
	const char *nametype_str, *prev_nametype_str;
	char buf[50];

	nametype_str = nametype_to_string(nametype);
	if (nametype == prev_nametype)
		prev_nametype_str = "";
	else {
		(void) sprintf(buf, " as a %s", nametype_to_string(prev_nametype));
		prev_nametype_str = buf;
	}
	
	diagf(ET_ERROR, lx, "%s %s already declared%s at %s",
					nametype_str, name,
			     		prev_nametype_str,
					ci_lexinfo_to_string(prev_lx, lx));
}

static bool
params_same(fr1, fr2)
funcret_t *fr1, *fr2;
{
	var_t *v1, *v2;

	if (fr1->fr_is_old_style || fr2->fr_is_old_style)
		return TRUE;
	
	if (fr1->fr_nparams != fr2->fr_nparams)
		return FALSE;
	
	for (v1 = fr1->fr_params, v2 = fr2->fr_params; v1 != NULL;
					v1 = v1->va_next, v2 = v2->va_next)
		if (!ci_types_same(v1->va_type, v2->va_type))
			return FALSE;
	return TRUE;
}

bool
ci_types_same(t1, t2)
type_t *t1, *t2;
{
	for (; t1 != NULL && t2 != NULL; t1 = t1->ty_base, t2 = t2->ty_base) {

		if (t1->ty_code != t2->ty_code)
			return FALSE;
		switch (t1->ty_code) {
		case DT_PTR_TO:
			if (t1->ty_qualifiers != t2->ty_qualifiers)
				return FALSE;
			break;
		case DT_FUNC_RETURNING:
			if (!params_same(t1->ty_funcret, t2->ty_funcret))
				return FALSE;
			break;
		case DT_ARRAY_OF:
			{
				dim_t *d1 = t1->ty_dim;
				dim_t *d2 = t2->ty_dim;

				if (!ci_types_same(d1->di_type, d2->di_type))
					return FALSE;

				if (d1->di_ltype == DT_CONSTANT &&
				    d2->di_ltype == DT_CONSTANT &&
				    d1->di_low != d2->di_low)
					return FALSE;
				
				if (d1->di_htype == DT_CONSTANT &&
				    d2->di_htype == DT_CONSTANT &&
				    d1->di_high != d2->di_high)
					return FALSE;
				
				return TRUE;
			}
			break;
		case TY_STRUCT: case TY_U_STRUCT:
		case TY_UNION:	case TY_U_UNION:
		case TY_ENUM:	case TY_U_ENUM:
			if (t1->ty_aggr_or_enum != t2->ty_aggr_or_enum)
				return FALSE;
			break;
		case TY_VOID:
		case TY_BOOLEAN:
		case TY_CHAR:	case TY_UCHAR:
		case TY_SHORT:	case TY_USHORT:
		case TY_INT:	case TY_UINT:
		case TY_LONG:	case TY_ULONG:
#if WANT_LL
		case TY_LONGLONG:	case TY_ULONGLONG:
#endif
		case TY_FLOAT:	case TY_DOUBLE:	
#if WANT_LDBL
		case TY_LONGDOUBLE:
#endif
			break;
		default:
			ci_panic("bad type in ts");
		}
	}

	return t1 == NULL && t2 == NULL;
}

void
ci_show_type(type, what)
type_t *type;
const char *what;
{
	char *typestr;

	typestr = ci_type_to_english(type, FALSE);
	diagf(ET_MORE, (lexinfo_t *)NULL, "%s: %s", what, typestr);
	free(typestr);
}

bool
ci_is_signed_type(typecode)
typecode_t typecode;
{
	switch (typecode) {
	case TY_CHAR:
	case TY_SHORT:
	case TY_INT:
	case TY_LONG:
#if WANT_LL
	case TY_LONGLONG:
#endif
	case TY_ENUM:
	case TY_FLOAT:
	case TY_DOUBLE:
#if WANT_LDBL
	case TY_LONGDOUBLE:
#endif
	case TY_BOOLEAN:
		return TRUE;
	case TY_UCHAR:
	case TY_USHORT:
	case TY_UINT:
	case TY_ULONG:
#if WANT_LL
	case TY_ULONGLONG:
#endif
	case DT_PTR_TO:
	case DT_ARRAY_OF:
		return FALSE;
	default:
		ci_panic("bad type in ist");
		return FALSE;	/* to satisfy gcc */
	}
}

bool
ci_is_integral(typecode)
typecode_t typecode;
{
	switch(typecode) {
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
	case TY_BITFIELD:
	case TY_BOOLEAN:
	/* Dibyendu added following */
	case TY_ENUM:
		return TRUE;
	default:
		return FALSE;
	}
}

const char *
ci_basetype_name(type)
type_t *type;
{
	static char buf[4096];
	const char *tagtype, *tag;

	if (type->ty_base != NULL)
		ci_panic("ci_basetype_name called on derived type");

	strcpy(buf, qualifiers_to_string(type->ty_qualifiers));

	switch(type->ty_code) {
	case TY_U_STRUCT:
		tagtype = "undefined struct";
		break;
	case TY_STRUCT:
		tagtype = "struct";	
		break;

	case TY_UNION:
		tagtype = "union";	
		break;
	case TY_U_UNION:
		tagtype = "undefined union";
		break;

	case TY_ENUM:
		tagtype = "enum";
		break;
	case TY_U_ENUM:
		tagtype = "undefined enum";
		break;
	
	case TY_BITFIELD:
		return "bitfield";

	default:
		return type->ty_name;	
	}

	if ((tag = type->ty_aggr_or_enum->ae_tag) != NULL)
		(void) snprintf(buf, sizeof(buf), "%s %s", tagtype, tag);
	else
		(void) snprintf(buf, sizeof(buf), "(unnamed) %s", tagtype);
	return buf;
}

/*
**  Because of the recursive nature of the parser, the name of a
**  base class is not always set up when the definition of a
**  subclass is parsed.  This routine sets up the va_name field
**  for a base class to the class name, if it is known, or to a
**  safe fixed address otherwise.
**
**  We call this when the subclass definition is parsed and again
**  when we need it for display.
*/
void
ci_make_baseclass_name( v)
var_t* v;
{
    static const char* unset_name = "Unknown BaseClass";

    if (v->va_name == NULL || v->va_name == unset_name)
    {
	type_t* type = v->va_type;
	if ( type->ty_code == DT_PTR_TO )
	    type = type->ty_base;
	if (type->ty_typedef != NULL &&  type->ty_typedef->td_name != NULL)
	    v->va_name = type->ty_typedef->td_name;
	else if ( type->ty_code == TY_STRUCT)
	    v->va_name = type->ty_aggr_or_enum->ae_tag;
	if (  v->va_name == NULL)
	     v->va_name = unset_name;
    }
}

/*  Rewrite any parameters of type "array of T" to "pointer to T"
 */
void
ci_rewrite_func_params(fr)
funcret_t *fr;
{
	var_t *v;

	for (v = fr->fr_params; v != NULL; v = v->va_next) {
		if (v->va_type->ty_code == DT_ARRAY_OF)
			v->va_type = ci_make_pointer(v->va_type->ty_base, 0);
		else if (v->va_type->ty_code == DT_FUNC_RETURNING) /* Dibyendu 01/01/99 */
			v->va_type = ci_make_pointer(v->va_type, 0);
	}
}

static char *
param_types(type)
type_t *type;
{
	funcret_t *fr;
	var_t *v;
	char *str, *new, *cptr;

	if (type->ty_code != DT_FUNC_RETURNING)
		ci_panic("bad type in pt");
	fr = type->ty_funcret;
	
	if (fr->fr_is_old_style)
		return strsave("()");

	if (fr->fr_nparams == 0)
		return strsave("(void)");

	str = strsave("");
	for (v = fr->fr_params; v != NULL; v = v->va_next) {
		cptr = ci_type_to_decl(v->va_type, FALSE);
		new = strf("%s, %s", str, cptr);
		free(cptr);
		free(str);
		str = new;
	}
	new = strf("(%s)", str + 2);
	free(str);
	return new;
}

static const char *
qualifiers_to_string(qualifiers)
qualifiers_t qualifiers;
{
	static char buf[50];

	strnf(buf, sizeof(buf), "%s%s%s",
			 (qualifiers & QU_CONST) ?    "const "    : "",
			 (qualifiers & QU_VOLATILE) ? "volatile " : "",
			 (qualifiers & QU_PACKED) ?   "packed "   : "");
	return buf;
}

char *
ci_qualified_basetype_name(type)
type_t *type;
{
    const char *s1 = qualifiers_to_string(type->ty_qualifiers);
    const char *s2 = ci_basetype_name(type);
    if (s1 && s2)
	return strf("%s%s", s1, s2);
    else
	return NULL;
}

char *
ci_type_to_decl(type, resolve_typedefs)
type_t *type;
bool resolve_typedefs;
{
	char *str, *new, *cptr;
	bool quit, last_was_ptr;
	
	str = strsave("");
	quit = last_was_ptr = FALSE;
	for (;;) {
		const char *asizestr;

		if (!resolve_typedefs && type->ty_typedef != NULL)
			break;

		switch(type->ty_code) {
		case DT_PTR_TO:
			new = strf("*%s%s",
					qualifiers_to_string(type->ty_qualifiers),
					str);
			break;
		case DT_FUNC_RETURNING:
			cptr = param_types(type);
			new = strf(last_was_ptr ? "(%s)%s" : "%s%s", str, cptr);
			free(cptr);
			break;
		case DT_ARRAY_OF:
			if (type->ty_dim->di_htype != DT_CONSTANT) {
				asizestr = "";
			}
			else {
				static char buf[30];

				strnf(buf, sizeof(buf), "%ld", 
				      type->ty_dim->di_high + 1);
				asizestr = buf;
			}
			new = strf(last_was_ptr ? "(%s)[%s]" : "%s[%s]",
							        str, asizestr);
			break;
		default:
			quit = TRUE;
			new = NULL;	/* to satisfy gcc */
			break;
		}
		if (quit)
			break;

		free(str);
		str = new;

		last_was_ptr = (type->ty_code == DT_PTR_TO);
		type = type->ty_base;
	}

	if (type->ty_code == TY_BITFIELD)
		new = strf("int %s:%d", str, type->ty_bitfield->bf_width);
	else {
		const char *name;

		if (!resolve_typedefs && type->ty_typedef != NULL)
			name = type->ty_typedef->td_name;
		else
			name = ci_basetype_name(type);
		new = strf("%s%s%s%s", qualifiers_to_string(type->ty_qualifiers),
			   name, *str == '\0' ? "" : " ", str);
	}
	free(str);
	return new;
}

char *
ci_type_to_english(type, resolve_typedefs)
type_t *type;
bool resolve_typedefs;
{
	bool quitloop;
	const char *space;
	char *str;

	str = strsave("");
	space = "";		/* no space the first time round the loop */

	for (quitloop = FALSE; !quitloop && type != NULL; type = type->ty_base){
		char *new;

		if (!resolve_typedefs && type->ty_typedef != NULL) {
			new = strf("%s%s%s", str, space,
						type->ty_typedef->td_name);
			quitloop = TRUE;
		}
		else {
			char *cptr;
			const char *params_str;

			switch (type->ty_code) {
			case DT_PTR_TO:
				new = strf("%s%s%spointer to", str, space,
					 qualifiers_to_string(type->ty_qualifiers));
				break;
			case DT_FUNC_RETURNING:
				cptr = param_types(type);
				params_str = (strcmp(cptr, "()") == 0) ? "" : cptr;
				new = strf("%s%sfunction%s returning",
							str, space, params_str);
				free(cptr);
				break;
			case DT_ARRAY_OF:
				if (type->ty_dim->di_htype != DT_CONSTANT)
					new = strf("%s%sarray[(unspecified size)] of",
							str, space);
				else
					new = strf("%s%sarray[%ld] of",
						   str, space,
						   type->ty_dim->di_high + 1);
				break;
			default:
				new = strf("%s%s%s", str, space,
							ci_basetype_name(type));
				break;
			}
		}
		free(str);
		str = new;
		space = " ";
	}
	return str;
}

void
ci_complain_if_types_differ(name, prev_type, prev_lexinfo, type, lexinfo)
const char *name;
type_t *prev_type;
lexinfo_t *prev_lexinfo;
type_t *type;
lexinfo_t *lexinfo;
{
	if (ci_types_same(prev_type, type))
		return;

	diagf(ET_ERROR, lexinfo,
	      "Declaration of `%s' conflicts with previous declaration at %s",
	      name, ci_lexinfo_to_string(prev_lexinfo, lexinfo));
	diagf(ET_MORE, (lexinfo_t *)NULL,
				"Old: %s", ci_type_to_english(prev_type, TRUE));
	diagf(ET_MORE, (lexinfo_t *)NULL,
				"New: %s", ci_type_to_english(type, TRUE));
}

bool
ci_complain_about_any_void_types(vlist)
var_t *vlist;
{
	bool complained;
	var_t *v;

	complained = FALSE;
	for (v = vlist; v != NULL; v = v->va_next) {
		typecode_t prevcode;
		type_t *type;

		prevcode = TY_NOTYPE;
		for (type = v->va_type; type->ty_base != NULL; type = type->ty_base)
			prevcode = type->ty_code;
		if (type->ty_code == TY_VOID && prevcode != DT_PTR_TO &&
						prevcode != DT_FUNC_RETURNING) {
			if (v->va_name != NULL)
				diagf(ET_ERROR, v->va_lexinfo,
					"Illegal type \"%s\" for `%s'",
					ci_type_to_english(v->va_type, TRUE),
					v->va_name);
			else
				diagf(ET_ERROR, v->va_lexinfo,
					"Illegal type \"%s\"",
					ci_type_to_english(v->va_type, TRUE));
			complained = TRUE;
		}
	}
	return complained;
}

const char *
ci_typecode_to_name(typecode)
typecode_t typecode;
{
    switch (typecode) {
	case DT_PTR_TO:		return "ptr";
	case DT_FUNC_RETURNING: return "function";
	case DT_ARRAY_OF: 	return "array";
	case TY_IDENTIFIER:	return "identifier";
	case TY_ELLIPSIS:	return "ellipsis";
	case TY_UNSIGNED:	return "unsigned";
	case TY_SIGNED:		return "signed";

	/* C aggregate types etc */
	case TY_STRUCT:		return "struct";
	case TY_UNION:		return "union";
	case TY_ENUM:		return "enum";
	case TY_U_STRUCT:	return "undefined struct";
	case TY_U_UNION:	return "undefined union";
	case TY_U_ENUM:		return "undefined enum";
	case TY_BITFIELD:	return "bitfield";

	/* Base types (common to C and Fortran */
	case TY_VOID:		return "void";
	case TY_CHAR:		return "char";
	case TY_UCHAR:		return "unsigned char";
	case TY_SHORT:		return "short";
	case TY_USHORT:		return "unsigned short";
	case TY_INT:		return "int";
	case TY_UINT:		return "unsigned int";
	case TY_LONG:		return "long";
	case TY_ULONG:		return "unsigned long";
	case TY_FLOAT:		return "float";
	case TY_DOUBLE:		return "double";
	case TY_INT_ASSUMED:	return "int (assumed)";

	/* Non-standard C types */
	case TY_LONGLONG:	return "long long";
	case TY_ULONGLONG:	return "unsigned long long";
	case TY_LONGDOUBLE:	return "long double";
	case TY_BOOLEAN:	return "boolean";

	/* FORTRAN base types */
	case TY_COMPLEX:	return "complex";
	case TY_DBLCOMP:	return "dblcomp";
	case TY_LOGICAL:	return "logical";
	case TY_CHARACTER:	return "character";
	case TY_FVOID:		return "fvoid";
	default:		return "unknown type";
    }
}
