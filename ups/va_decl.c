/* va_decl.c - variable declaration construction */

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


/* @(#)va_decl.c	1.30 04 Jun 1995 (UKC) */
char ups_va_decl_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <local/ukcprog.h>
#include <mtrprog/alloc.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "target.h"
#include "data.h"
#include "va.h"
#include "va_priv.h"
#include "state.h"

ilist_t *new_ilist PROTO((void));
static const char *derivname PROTO((typecode_t deriv, typecode_t last,
				    bool fake_array));
static void dim_to_s PROTO((int val, int known, char *buf));
static void set_il_indices PROTO((ilist_t *il, type_t *type));
static const char *fortran_basetype_name PROTO((type_t *type));
static const char *c_mkdecl PROTO((const char *name, type_t *type, 
                                   class_t class, ilist_t *ilist, int ilevel, 
                                   bool hide_ptr, bool want_typedefs,
				   short *ul_start));
static const char *fortran_mkdecl PROTO((const char *name, type_t *type, 
					 ilist_t *ilist, short *ul_start));

ALLOC_NEW_FREELIST(extern,ilist_t,ilist,il_next)

/*  Calculate the address of the bracketed part of the variable declaration.
 */
taddr_t
dvar_addr(dv)
dvar_t *dv;
{
	ilist_t *ilist;
	type_t *type;
	int i;
	taddr_t addr;
	bool is_fortran_var, is_fortran_pointer, is_fortran_pointer_to_array;
	
	ilist = dv->dv_ilist;
	addr = dv->dv_addr;
	type = dv->dv_var->va_type;
	is_fortran_var = IS_FORTRAN(dv->dv_var->va_language);
	is_fortran_pointer = FALSE;
	is_fortran_pointer_to_array = FALSE;

	for (i = dv->dv_ilevel; i > 0; --i) {
		int aindex, low;
		size_t basesize;
		typecode_t deriv;
		int offset;
	
		deriv = type->ty_code;

		if (deriv != DT_ARRAY_OF && deriv != DT_PTR_TO)
		{
		  errf("bad deriv in da - probably bad format hint for `%s'",
		       dv->dv_var && dv->dv_var->va_name ?
		       dv->dv_var->va_name : "(unknown)");
		  dv->dv_flags |= DVF_BAD_FORMAT_HINT;
		  return addr;
		}

                if (deriv == DT_PTR_TO) {
                        if (addr == 0)
                                return 0;
                        
                        if (dread(get_current_target(), addr,
                                  (char *)&addr, sizeof(addr)) != 0)
                                return BAD_ADDR;
		}

		if (deriv == DT_PTR_TO && is_fortran_var) {
                        if (addr == 0)
                                return 0;
                        
			aindex = 0;
			low = 0;
			basesize = 0;
			
			is_fortran_pointer = TRUE;

			/* RGA attempt to fix fortran pointer to arrays */
			if (type->ty_base->ty_code == DT_ARRAY_OF &&
			    type->ty_base->ty_base)
			{
			  is_fortran_pointer_to_array = TRUE;
			  aindex = ilist->il_index;
			  low = ilist->il_low;
			  basesize = dynamic_type_size(type->ty_base->ty_base,
						       ilist->il_next);
			}
		}
		else {
			if (deriv == DT_ARRAY_OF && !ilist->il_low_known)
				return BAD_ADDR;

			aindex = ilist->il_index;
			low = ilist->il_low;
			
			if (is_fortran_pointer && deriv == DT_ARRAY_OF) {
				basesize = ilist->il_basesize;
			}
			else {
				basesize = dynamic_type_size(type->ty_base,
							     ilist->il_next);
			}
			
			ilist = ilist->il_next;
		}

		type = type->ty_base;
		
		if (basesize == UNKNOWN_SIZE)
			return BAD_ADDR;

		offset = aindex;
		if (deriv == DT_ARRAY_OF || is_fortran_pointer_to_array)
			offset -= low;
		addr += offset * basesize;
	}

	return addr;
}

ilist_t *
dup_ilist(old)
ilist_t *old;
{
	ilist_t *last, *newlist, *il;

	newlist = last = NULL;
	for (; old != NULL; old = old->il_next) {
		il = new_ilist();
		*il = *old;
		if (last != NULL)
			last->il_next = il;
		else
			newlist = il;
		last = il;
	}
	if (last != NULL)
		last->il_next = NULL;
	return newlist;
}

static const char *
derivname(deriv, last, fake_array)
typecode_t deriv, last;
int fake_array;
{
	int needb;

	needb = (deriv == DT_FUNC_RETURNING || deriv == DT_ARRAY_OF) && last == DT_PTR_TO;
	switch(deriv) {
	case DT_PTR_TO:
		if (fake_array)
			return needb ? "(%s){%s}" : "%s{%s}";
		else
			return needb ? "*(%s)" : "*%s";
	case DT_FUNC_RETURNING:
		return needb ? "(%s)()" : "%s()";
	case DT_ARRAY_OF:
		return needb ? "(%s)[%s]" : "%s[%s]";
	default:
		panic("bad deriv in derivname");
	}
	/* NOTREACHED */
	return NULL;	/* to keep gcc happy */
}

/*  Marker for start of var name.  Any character that can't occur in a
 *  declaration.
 */
#define MARKER_CH	'@'

static const char *
c_mkdecl(name, type, class, ilist, ilevel, hide_ptr, want_typedefs, ul_start)
const char *name;
type_t *type;
class_t class;
ilist_t *ilist;
int ilevel;
bool hide_ptr, want_typedefs;
short *ul_start;
{
#       define MAX_NAME_LEN 200   /* Truncate names at this length */
#       define MAX_TYPE_LEN 100    /* Truncate types at this length */
	static char bufs[2][512];
	typecode_t deriv, last, bt;
	int cur, level, fake_array;
	char ibuf[12];
	bitfield_t *bf;
	
	cur = 0;
	strncpy(bufs[cur], name, MAX_NAME_LEN);
	if ( strlen(name) > MAX_NAME_LEN)
	    strcpy(bufs[cur]+MAX_NAME_LEN,"...");
	if (ul_start)		/* RGA */
	  bufs[cur][0] = MARKER_CH;
	last = TY_NOTYPE;
	
	level = 0;
	for (;;) {
		const char *fmt;

		if (level == ilevel) {
			sprintf(bufs[1-cur], "%c%s%c",
						C_AOPEN, bufs[cur], C_ACLOSE);
			cur = 1 - cur;
		}

		deriv = type->ty_code;
		if (!ISDERIV(deriv))
			break;

		fake_array = (deriv == DT_PTR_TO && level < ilevel);
		if (deriv == DT_ARRAY_OF || fake_array) {
			if (ilevel == -1) {
				dim_t *dim;

				dim = type->ty_dim;
				
				if (dim->di_htype == DT_CONSTANT)
					sprintf(ibuf, "%ld", dim->di_high + 1);
				else
					*ibuf = '\0';
			}
			if (level < ilevel) {
				sprintf(ibuf, "%d", ilist->il_index);
				ilist = ilist->il_next;
			}
			else
				*ibuf = '\0';
		}

		fmt = derivname(deriv, last, fake_array);
		fmt = hide_ptr ? "%s" : derivname(deriv, last, fake_array);
		
		sprintf(bufs[1-cur], fmt, bufs[cur], ibuf);
		cur = 1 - cur;
		
		last = deriv;
		type = type->ty_base;
		++level;
	}

	bt = type->ty_code;
	if (bt == TY_BITFIELD) {
		bf = type->ty_bitfield;
		if (bf->bf_type)
		    sprintf(bufs[1 - cur], "%s %s:%d",
			    ci_basetype_name(bf->bf_type), bufs[cur], bf->bf_width);
		else
		    sprintf(bufs[1 - cur], "int %s:%d",
			    bufs[cur], bf->bf_width);
	}
	else {
		const char *btname;

		if (want_typedefs && type->ty_typedef != NULL &&
		    (bt == TY_STRUCT || bt == TY_UNION || bt == TY_ENUM
		     || bt == TY_NOTYPE)) {
			btname = type->ty_typedef->td_name;
		}
		else {
			btname = ci_basetype_name(type);
		}

		if ( strlen(btname) > MAX_TYPE_LEN)
		{
		   strncpy(bufs[1 - cur],btname,MAX_TYPE_LEN);
		   sprintf(bufs[1 - cur]+MAX_TYPE_LEN, "... %s", bufs[cur]);
		}
		else
		   sprintf(bufs[1 - cur], "%s %s", btname, bufs[cur]);
	}
	cur = 1 - cur;

	if (class == CL_REG && hide_ptr)
		class = CL_AUTO;

	if (class == CL_STAT || class == CL_LSTAT || class == CL_REG) {
		sprintf(bufs[1 - cur], "%s %s",
			(class == CL_REG) ? "register" : "static",
			bufs[cur]);
		cur = 1 - cur;
	}
	if (ul_start)
	{
	  *ul_start = strchr(bufs[cur], MARKER_CH) - bufs[cur];
	  bufs[cur][*ul_start] = *name;
	}
	return bufs[cur];
}

static void
dim_to_s(val, known, buf)
int val, known;
char *buf;
{
	if (known)
		sprintf(buf, "%d", val);
	else
		strcpy(buf, "[?]");
}

static const char *
fortran_mkdecl(name, vartype, ilist, ul_start)
const char *name;
type_t *vartype;
ilist_t *ilist;
short *ul_start;
{
	static char bufs[2][256];
	int cur;
        bool need_brac;
        const char *pointer_tag;
	type_t *type, *target_type;
	ilist_t *il;
	bool have_ilist;
	
	cur = 0;
	bufs[cur][0] = '\0';

	need_brac = FALSE;
        pointer_tag = "";
	il = ilist;
	have_ilist = ilist != NULL;
        
	target_type = vartype;
	for (type = target_type; ISDERIV(type->ty_code); type = type->ty_base) {

                if (type->ty_code == DT_PTR_TO) {
                        if (*pointer_tag != '\0')
                                panic("dup DT_PTR_TO botch in fm");
                        pointer_tag = ", pointer ::";
                        target_type = type->ty_base;
                }
                else if (type->ty_code == DT_ARRAY_OF) {
                        int ch;

			if (!have_ilist)
				break;
			
                        if (type->ty_base->ty_code == TY_CHARACTER)
                                break;

                        ch = (type == target_type) ? FORTRAN_ACLOSE : ',';
                        sprintf(bufs[1 - cur], "%d%c%s",
                                il->il_index, ch, bufs[cur]);
                        cur = 1 - cur;
                        
                        need_brac = TRUE;
                        il = il->il_next;
                }
                else {
			panic("unknown type derivation in fortran_mkdecl");
                }
        }
                
	if (need_brac)
		sprintf(bufs[1 - cur], " %c%s", FORTRAN_AOPEN, bufs[cur]);
	else
		sprintf(bufs[1 - cur], " %s", bufs[cur]);
	cur = 1 - cur;

	il = ilist;
	need_brac = FALSE;
        
        target_type = vartype;
	for (type = target_type; ISDERIV(type->ty_code); type = type->ty_base) {
                dim_t *dim;
                int ch;
		bool low_known, high_known;
		long low, high;

                if (type->ty_code == DT_PTR_TO) {
                        target_type = type->ty_base;
                        continue;
                }
                
                if (type->ty_code != DT_ARRAY_OF)
                        panic("unknown deriv in fmd");
                
		if (type->ty_base->ty_code == TY_CHARACTER)
			break;

		dim = type->ty_dim;
		ch = (type == target_type) ? ')' : ',';

		if (have_ilist) {
			high = il->il_high;
			low = il->il_low;
			high_known = il->il_high_known;
			low_known = il->il_low_known;
		}
		else {
			high = dim->di_high;
			low = dim->di_low;
			high_known = dim->di_htype == DT_CONSTANT;
			low_known = dim->di_ltype == DT_CONSTANT;
		}
			
		
                if (high_known || low_known) {
                        char hdim[20];

                        dim_to_s(high, high_known, hdim);
        
                        if (low_known && low == 1) {
                                sprintf(bufs[1-cur], "%s%c%s",
                                        hdim, ch, bufs[cur]);
                        }
                        else {
                                char ldim[20];
                                
                                dim_to_s(low, low_known, ldim);
                                sprintf(bufs[1-cur], "%s:%s%c%s",
                                        ldim, hdim, ch, bufs[cur]);
                        }
                }
                else {
                        sprintf(bufs[1 - cur], ":%c%s", ch, bufs[cur]);
                }
                
		need_brac = TRUE;
		cur = 1 - cur;

		if (have_ilist)
			il = il->il_next;
	}

	sprintf(bufs[1 - cur], need_brac ? "%s(%s" : "%s %s", name, bufs[cur]);
	cur = 1 - cur;

        if (ISDERIV(type->ty_code)) {
                char lenstr[20];
                
		if (type->ty_code != DT_ARRAY_OF ||
                    type->ty_base->ty_code != TY_CHARACTER) {
			panic("botch in fortran_mkdecl");
                }

		if (have_ilist) {
			dim_to_s(il->il_high, il->il_high_known, lenstr);
		}
		else {
			dim_to_s(type->ty_dim->di_high,
				 type->ty_dim->di_htype == DT_CONSTANT, lenstr);
		}
	
		sprintf(bufs[1 - cur], "character*%s%s %s",
                        lenstr, pointer_tag, bufs[cur]);
	}
	else {
		sprintf(bufs[1 - cur], "%s%s %s",
                        fortran_basetype_name(type), pointer_tag, bufs[cur]);
	}
	if (ul_start)
		*ul_start = strlen (bufs[1 - cur]) - strlen(bufs[cur]);
	
	return bufs[1 - cur];
}

static const char *
fortran_basetype_name(type)
type_t *type;
{
	static char *last = NULL;
	const char *tag;

	if (type->ty_base != NULL)
		panic("fortran_basetype_name called on derived type");

	if (type->ty_code != TY_STRUCT)
		return type->ty_name;
	
	/* GNU f77 produces typedef's in symbol table. */
	if ((type->ty_typedef != NULL) && (type->ty_typedef->td_name != NULL))
		return type->ty_typedef->td_name;

	if ((tag = type->ty_aggr_or_enum->ae_tag) == NULL)
		tag = "<UNKNOWN>";

	if (last != NULL)
		free(last);

	last = strf("type (%s)", tag);
	return last;
}

const char *
type_to_decl(name, type, class, language, want_typedefs)
const char *name;
type_t *type;
class_t class;
language_t language;
bool want_typedefs;
{
	switch (language) {
	case LANG_C:
	case LANG_CC:
	case LANG_UNKNOWN:
		return c_mkdecl(name, type, class, (ilist_t *)NULL, -1,
				FALSE, want_typedefs, 0);

	case LANG_F77:
	case LANG_F90:
		return fortran_mkdecl(name, type, (ilist_t *)NULL, 0);

	default:
		panic("unknown lang in ttd");
		return NULL;	/* to satisfy gcc */
	}
}

const char *
mkdecl(dv)
dvar_t *dv;
{
	var_t *v;

	v = dv->dv_var;
	
	switch(dv->dv_var->va_language) {
	case LANG_C:
	case LANG_CC:
	case LANG_UNKNOWN:
		/*
		**   Set up base class names.  Done here since
		**   the base class type_t may not be set up
		**   at the time the class definition is parsed.
		*/
		if ( v->va_flags & VA_BASECLASS)
		    ci_make_baseclass_name(v);
		return c_mkdecl(v->va_name, v->va_type, v->va_class,
				dv->dv_ilist, dv->dv_ilevel,
				(v->va_flags & VA_HIDE_PTR) != 0,
				(dv->dv_flags & DVF_NO_TYPEDEFS) == 0,
				&dv->dv_ul_start);
	case LANG_F77:
	case LANG_F90:
		return fortran_mkdecl(v->va_name, v->va_type, dv->dv_ilist,
				      &dv->dv_ul_start);
		
	default:
		panic("unknown language in mkdecl");
		return NULL;	/* to satisfy gcc */
	}
}

/*  return the default format for the variable value.
 */
vformat_t
default_format(vtype, type)
type_t *vtype, *type;
{
	typecode_t typecode;

	typecode = type->ty_code;
	if (typecode == TY_BITFIELD)
		typecode = type->ty_bitfield->bf_type->ty_code;

	switch(typecode) {

	/*  The C interpreter preserves array types (rather than
	 *  having them turn into pointers) to make the copyin/copyout
	 *  pointer stuff work in breakpoint code.  Thus we can get
	 *  arrays types for display expressions.
	 */
	case DT_ARRAY_OF:
	case DT_PTR_TO:
		return DF_UHEX;

	case TY_CHAR:
	case TY_UCHAR:
		while (vtype != type && vtype->ty_base != type)
			vtype = vtype->ty_base;
		if (vtype->ty_code == DT_ARRAY_OF || vtype->ty_code == DT_PTR_TO)
			return DF_STRING;
		else
			return DF_ASCII;

	case TY_USHORT:
	case TY_UINT:
	case TY_ULONG:
	case TY_ULONGLONG:
		return DF_UDEC;

	case TY_ENUM:
	case TY_U_ENUM:
		return DF_NONE;
	
	default:
		return DF_SDEC;
	}
}

/*  Return the default level for displaying a variable of type type.
 */
int
default_level(type, language)
type_t *type;
language_t language;
{
	int level;

        level = 0;
        
        if (type->ty_code == DT_PTR_TO &&
            (IS_FORTRAN(language) ||
             (type->ty_base->ty_code == TY_CHAR ||
              type->ty_base->ty_code == TY_UCHAR))) {
                type = type->ty_base;
		++level;
	}

        while (type->ty_code == DT_ARRAY_OF) {
                type = type->ty_base;
                ++level;
        }

	return level;
}

/*  Set the ilist indices for an array.  Note that we set il_high and il_low
 *  even for a dynamic array, because di_{low,high} contain the stack offset
 *  of the place where the current array dimensions can be found for dynamic
 *  arrays.
 */
static void
set_il_indices(il, type)
ilist_t *il;
type_t *type;
{
	dim_t *dim;

	il->il_index = 0;
	il->il_basesize = 0;
	if (type->ty_code == DT_ARRAY_OF) {
		dim = type->ty_dim;
                
		il->il_low = dim->di_low;
		il->il_high = dim->di_high;
		il->il_low_known = dim->di_ltype == DT_CONSTANT;
		il->il_high_known = dim->di_htype == DT_CONSTANT;
		il->il_index = (il->il_low_known) ? dim->di_low : 0;
	}

}

void
change_dv_level(dv, dir)
dvar_t *dv;
enum leveldir dir;
{
	ilist_t *il, *last;
	type_t *type;
	int i;

	if (dir == CL_DOWN) {
		--dv->dv_ilevel;
		return;
	}

	il = dv->dv_ilist;
	last = NULL;
	type = dv->dv_var->va_type;

        if (IS_FORTRAN(dv->dv_var->va_language) && type->ty_code == DT_PTR_TO)
                type = type->ty_base;
        
	for (i = 0; i < dv->dv_ilevel; ++i) {
		type = type->ty_base;
		last = il;
		il = il->il_next;
	}
        
	if (il == NULL) {
		il = new_ilist();
		set_il_indices(il, type);
		if (last != NULL)
			last->il_next = il;
		else
			dv->dv_ilist = il;
		il->il_next = NULL;
	}
        
	++dv->dv_ilevel;
}

ilist_t *
make_ilist(type, language, level)
type_t *type;
language_t language;
int level;
{
	ilist_t *ilist, *il, *last;
	int i;

        if (IS_FORTRAN(language) && type->ty_code == DT_PTR_TO && level > 0) {
                type = type->ty_base;
                --level;
        }
            
	ilist = last = NULL;
            
	for (i = 0; i < level; i++) {
		while (type->ty_code != DT_ARRAY_OF &&
                       type->ty_code != DT_PTR_TO) {
			type = type->ty_base;
                }

		il = new_ilist();
		set_il_indices(il, type);

		if (last != NULL)
			last->il_next = il;
		else
			ilist = il;
		last = il;
		type = type->ty_base;
	}
	if (last != NULL)
		last->il_next = NULL;
	return ilist;
}
