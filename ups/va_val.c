/* va_val.c - display of variable values */

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


/* @(#)va_val.c	1.39 04 Jun 1995 (UKC) */
char ups_va_val_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <local/ukcprog.h>
#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "st.h"
#include "data.h"
#include "va.h"
#include "va_priv.h"
#include "state.h"

static void show_string PROTO((const char *iptr, size_t ilen,
			       char *optr, size_t olen,
			       int delim, language_t language));
static vlong get_fieldval PROTO((bool bits_big_endian, vlong word,
			       bitfield_t *bf, int is_signed));
static void note_badaddr PROTO((char *buf, size_t bufsize, language_t language, 
                                taddr_t addr));
static void vtbl_to_string PROTO(( char *buf, size_t bufsize, taddr_t addr,
				type_t *type, vformat_t format));

/*  Read and display a C NUL terminated string.
 *
 *  This routine first tries to read the string in a big chunk.
 *  If that fails it tries to read it a character at a time.  If
 *  we don't do this we might wrongly say bad address for a string
 *  that starts inside the address space, but extends beyond it.
 */
void
read_and_show_C_string(addr, rbuf, rbufsize, obuf, obufsize) 
taddr_t addr;
char *rbuf;
size_t rbufsize;
char *obuf;
size_t obufsize;
{
	size_t len;
	target_t *xp;

	xp = get_current_target();

	if (dread(xp, addr, rbuf, rbufsize) == 0) {
		len = rbufsize;
	}
	else {
		int i;

		for (i = 0; i < rbufsize; ++i) {
			if (i > 0 && rbuf[i - 1] == '\0')
				break;
			if (dread(xp, addr + i, rbuf + i, 1) != 0)
				break;
		}
		len = i;
	}
			
	if (len > 0)
		show_string(rbuf, len, obuf, obufsize, '"', LANG_C);
	else
		note_badaddr(obuf, obufsize, LANG_C, addr);
}

static void
show_string(iptr, ilen, optr, olen, delim, language)
const char *iptr;
size_t ilen;
char *optr;
size_t olen;
int delim;
language_t language;
{
	static char escapes[] = "\nn\rr\tt\bb\ff\\\\''\"\"";
	const char *ilim, *olim, *cptr;
	int ch;

	olim = optr + olen - 2;		/* one for delim, one for NUL */
	ilim = iptr + ilen;
	*optr++ = delim;
	
	while (iptr < ilim) {
		ch = *iptr++ & 0xff;
		if (ch >= ' ' && ch != '\\' && ch != delim && ch < 127) {
			if (optr >= olim)
				break;
			*optr++ = ch;
		}
		else if (ch == 0) {
			if (delim == '"' || optr >= olim - 1) {
				*optr++ = delim;
				break;
			}
			*optr++ = '\\';
			*optr++ = '0';
		}
		else if ((cptr = strchr(escapes, ch)) != NULL) {
			if (optr >= olim - 1)
				break;
			*optr++ = '\\';
			*optr++ = cptr[1];
		}
		else {
			if (optr >= olim - 4)
				break;
			sprintf(optr, "\\%03o", ch);
			optr += 4;
		}
	}
	
	if (iptr == ilim && optr < olim && (delim != '"' || language != LANG_C))
		*optr++ = delim;
	
	*optr++ = '\0';
}

void
enumval_to_string(buf, bufsize, val, type)
char *buf;
size_t bufsize;
vlong val;
type_t *type;
{
	enum_member_t *em;

	if (type->ty_aggr_or_enum == NULL)
		em = NULL;
	else
		em = type->ty_aggr_or_enum->ae_enum_members;

	for (; em != NULL; em = em->em_next) {
		if (val == em->em_val) {
			strncpy(buf, em->em_name, bufsize - 1);
			buf[bufsize - 1] = '\0';
			break;
		}
	}
	if (em == NULL)
		strnf(buf, bufsize - 1, "<%ld>", val);
}

void
addr_to_string(buf, bufsize, addr, type, format)
char *buf;
size_t bufsize;
taddr_t addr;
type_t *type;
vformat_t format;
{
	func_t *f;
	char rbuf[128];

	if (addr == 0)
		strcpy(buf, "NULL");
	
	if (format == DF_STRING) {
		read_and_show_C_string(addr, rbuf, sizeof(rbuf), buf, bufsize);
	}
	else if (type->ty_base->ty_code == DT_FUNC_RETURNING) {
		target_t *xp;

		/*  Pointer to a function
		 */

		/*  BUG: the target might not use zero to represent a NULL
		 *       function pointer.
		 */
		if (addr == 0) {
			strcpy(buf, "NULL");
			return;
		}

		xp = get_current_target();
		addr = st_funcptr_to_addr(xp_main_symtab(xp), addr);
		
		if ((f = addr_to_func(addr)) == NULL)
			strnf(buf, bufsize - 1, "(%x)()", (int)addr);
		else if (f->fu_addr == addr) {
			if (f->fu_language == LANG_CC)
			    strnf(buf, bufsize - 1, "%s", f->fu_demangled_name);
			else
			    strnf(buf, bufsize - 1, "%s()", f->fu_name);
		} else
			strnf(buf, bufsize - 1, "(%s+%lx)()", f->fu_name,
							   addr - f->fu_addr);
	}
	else {
		int_to_string(buf, bufsize, (vlong)addr, format);
	}
}

static void
vtbl_to_string(buf, bufsize, addr, type, format)
char *buf;
size_t bufsize;
taddr_t addr;
type_t *type;
vformat_t format;
{
    size_t offset;
    const char* name = lookup_global_name(addr, &offset);
    if ( name )
    {
	char* dmangle = demangle_name_3((char *)name);
	if ( dmangle)
	{
	    strncpy(buf, dmangle, bufsize-1);
	    free(dmangle);
	}
	else
	    strncpy(buf, name, bufsize-1);
	buf[bufsize-1] = 0;

	/* If address was not an exact match, add in the offset
	**   ( This happens for SC4 when multiple inheritance is used )
	*/
	if ( offset)
	{
	   int len = strlen(buf);
	   int maxcp = bufsize-1-len;
	   if ( maxcp > 0 )
	   {
	       char offset_buff[12];
	       sprintf(offset_buff,"+0x%lx",(unsigned long)offset);
	       strncpy(buf+len, offset_buff, maxcp);
	   }
	}
    } else
	addr_to_string(buf, bufsize, addr, type, format);
}

#if HAVE_LONG_LONG
/* Compiler supports 'long long', does 'sprintf()' ? */
#if defined LONG_LONG_FMT
# define VLONG_FMT LONG_LONG_FMT
# define sprintf_ll sprintf
#else
# define VLONG_FMT ""
#endif
#else
/* Compiler does not support 'long long'. */
# define VLONG_FMT "l"
# define sprintf_ll sprintf
#endif

/*
**  Some compilers handle long longs fine but can't print them.
**  This routine simulates sprintf(buf, fmt, n) for that case. (RCB)
*/
#if !defined sprintf_ll
static void sprintf_ll(char* buf, const char* fmt, vlong n)
{
#define MAX_PTEN (1+(sizeof(unsigned long long)*8*3)/10)
#define MAX_PEIGHT (1+sizeof(unsigned long long)*8/3)
    static unsigned long long powers_of_ten[MAX_PTEN];
    static unsigned long long powers_of_eight[MAX_PEIGHT];
    unsigned long long* power_table;
    int base;
    int max_power;
    int exp;


    unsigned long long nn = (unsigned long long)n;
    unsigned long long power;
    while (*fmt != '%')
       *buf++ = *fmt++;
    switch(*++fmt)
    {
    case 'd':
	if ( n < 0 )
	{
	    nn = -n;
	    *buf++ = '-';
	}
	/* Fall into unsigned case */

    case 'u':
	base = 10;
	power_table = powers_of_ten;
	max_power = MAX_PTEN;
	break;
    case 'x':
	if ( nn > 0xffffffff)
	    sprintf(buf, "%lx%08lx", (unsigned long)(nn>>32), (unsigned long)nn);
	else
	    sprintf(buf, "%lx", (unsigned long)nn);
	return;

    case 'o':
	base = 8;
	power_table = powers_of_eight;
	max_power = MAX_PEIGHT;
	break;
    }

    if ( power_table[0] == 0)
    {
       /* First time, calculate all the powers of the base */
       power = 1;
       exp =0;
       while (exp < max_power)
       {
	    power_table[exp++] = power;
	    power *= base;
       }

    }

    /* Compute the first power of base larger than n */
    exp = 0;
    do  exp++;
    while ( exp < max_power && power_table[exp] <= nn);

    /* Now form the decimal or octal value */
    do
    {
	int digit;
	power = power_table[--exp];
	digit = nn / power;
	*buf++ = '0' + digit;
	nn -= power*digit;
    }while(exp);
    *buf = 0;
}
#endif

/*  Construct a value string for an integer type variable using the format.
 */
void
int_to_string(buf, bufsize, n, format)
char *buf;
size_t bufsize;
vlong n;
vformat_t format;
{
	switch(format) {
/* This test is really "ANSI compliant C pre-processor ?" */
#if HAVE_LONG_LONG
 	case DF_NONE:
 	case DF_SDEC:
 		sprintf_ll(buf, "%" VLONG_FMT "d", n);
 		break;
 	case DF_UDEC:
 		sprintf_ll(buf, "%" VLONG_FMT "u", n);
 		break;
 	case DF_SOCT:
 		if (n >= 0)
 			sprintf_ll(buf, "0" "%" VLONG_FMT "o", n);
 		else
 			sprintf_ll(buf, "-0" "%" VLONG_FMT "o", -n);
 		break;
 	case DF_UOCT:
 		sprintf_ll(buf, "0" "%" VLONG_FMT "o", n);
 		break;
 	case DF_SHEX:
 		if (n >= 0)
 			sprintf_ll(buf, "0x" "%" VLONG_FMT "x", n);
 		else
 			sprintf_ll(buf, "-0x" "%" VLONG_FMT "x", -n);
 		break;
 	case DF_UHEX:
 	case DF_STRING:
 		sprintf_ll(buf, "0x" "%" VLONG_FMT "x", n);
 		break;
#else
 	case DF_NONE:
 	case DF_SDEC:
 		sprintf_ll(buf, "%ld", n);
 		break;
 	case DF_UDEC:
 		sprintf_ll(buf, "%lu", n);
 		break;
 	case DF_SOCT:
 		if (n >= 0)
 			sprintf_ll(buf, "0%lo", n);
 		else
 			sprintf_ll(buf, "-0%lo", -n);
 		break;
 	case DF_UOCT:
 		sprintf_ll(buf, "0%lo", n);
 		break;
 	case DF_SHEX:
 		if (n >= 0)
 			sprintf_ll(buf, "0x%lx", n);
 		else
 			sprintf_ll(buf, "-0x%lx", -n);
 		break;
 	case DF_UHEX:
 	case DF_STRING:
 		sprintf_ll(buf, "0x%lx", n);
 		break;
#endif
	case DF_UBIN:
	    {
		int i, start_i;
		char *cptr;

		for (i = 0; n > 0; ++i)
			n <<= 1;
		if (n == 0) {
			strcpy(buf, "0b0");
			break;
		}
		strcpy(buf, "0b");
		start_i = i;
		for (cptr = buf + 2; i < sizeof(n) * 8; n <<= 1, ++i) {
			if (i > start_i && i % 4 == 0)
				*cptr++ = ' ';
			*cptr++ = (n < 0) ? '1' : '0';
		}
		*cptr = '\0';
		break;
	    }
	case DF_ASCII:
		{
			size_t count;
			char obuf[sizeof(n) + 1], *optr;

			optr = obuf;
			if ((unsigned)n <= 0xff) {
				*optr = (char)n;
				count = 1;
			}
			else {
				memcpy(optr, (char *)&n, sizeof(n));
				count = sizeof(n);
				while (*optr == '\0') {
					++optr;
					--count;
				}
			}
			optr[count] = '\0';
			show_string(optr, count, buf, bufsize - 1, '\'', LANG_C);
		}
		break;
	default:
		panic("unknown format in df");
	}
}

static vlong
get_fieldval(bits_big_endian, word, bf, is_signed)
bool bits_big_endian;
vlong word;
bitfield_t *bf;
int is_signed;
{
	int nbits;
	int shift;
	uvlong mask, val;

	nbits = sizeof(uvlong) * 8;

	if (bits_big_endian)
		shift = nbits - (bf->bf_offset + bf->bf_width);
	else
		shift = bf->bf_offset;

	mask = 0;
	mask = ~mask >> (nbits - bf->bf_width);
	val = (word >> shift) & mask;

	/*  If the bitfield is signed we must sign extend it ourself
	 *  if the most significant bit of the field is set.
	 */
	if (is_signed && bf->bf_width > 0 && (val & (1 << (bf->bf_width - 1))))
		val |= ~mask;
	
	return val;
}

int
is_signed_format(format)
vformat_t format;
{
	switch(format) {
	case DF_UDEC:
	case DF_UOCT:
	case DF_UHEX:
	case DF_UBIN:
		return FALSE;
	default:
		return TRUE;
	}
}

#define DREAD_VAR(xp, addr, var) \
			(dread(xp, addr, (char *)&var, sizeof(var)) == 0)

int
addr_to_number(val, addr, type)
vlong *val;
taddr_t addr;
type_t *type;
{
	target_t *xp = get_current_target();
	int ret = 0;
	value_t vl;
	
	switch (type->ty_size)
	{
	case 1:
		ret = DREAD_VAR(xp, addr, vl.vl_char);
		*val = vl.vl_char;
		break;
	case 2:
		ret = DREAD_VAR(xp, addr, vl.vl_short);
		*val = vl.vl_short;
		break;
	case 4:
		ret = DREAD_VAR(xp, addr, vl.vl_long);
		*val = vl.vl_long;
		break;
#if HAVE_LONG_LONG
	case 8:
		ret = DREAD_VAR(xp, addr, vl.vl_longlong);
		*val = vl.vl_longlong;
		break;
#endif
	}

	return ret;
}

/*  Construct a value string for the variable.
 */
const char *
mkval(dv)
dvar_t *dv;
{
	static const char f90_unassigned_str[] = "<unassigned>";
	var_t *v;
	value_t vl;
	taddr_t addr;
	type_t *type;
	ilist_t *il;
	static char buf[128], rbuf[128];
	bool want_hex;
	size_t len;
	target_t *xp;
	vlong val;


	xp = get_current_target();
	v = dv->dv_var;
	addr = dvar_addr(dv);
        
	if (dv->dv_flags & DVF_BAD_FORMAT_HINT)
		return "<Bad format hint>";

	if (addr == BAD_ADDR)
		return "<Address out of range>";

        if (dynamic_type_size(v->va_type, dv->dv_ilist) == 0)
                return "<Zero size object>";

	type = get_type_at_level(v, dv->dv_ilevel);

	if (type == NULL)	/* RGA */
                return "<Unreadable data>";

	if (type->ty_code == DT_PTR_TO) {
		if (!DREAD_VAR(xp, addr, vl.vl_addr)) {
			note_badaddr(buf, sizeof(buf), v->va_language, addr);
			return buf;
		}
		if ( v->va_flags & VA_VTBLPTR )
		    vtbl_to_string(buf, sizeof(buf), vl.vl_addr,
							type, dv->dv_format);

		else
		    addr_to_string(buf, sizeof(buf), vl.vl_addr,
							type, dv->dv_format);
		return buf;
	}

	if (ISDERIV(type))
		panic("unexpected type in mkval");

	if (addr == 0 && v->va_type->ty_code == DT_PTR_TO) {
                if (IS_FORTRAN(v->va_language) && dv->dv_ilevel >= 1)
                        return "<unassociated>";

                if (dv->dv_ilevel == 1 && dv->dv_ilist->il_index == 0)
                        return "*NULL";
        }

	if (addr != 0 && dv->dv_format == DF_STRING) {
		size_t ilen;

		if (v->va_class == CL_REG && type == v->va_type)
			ilen = sizeof(int);
		else
			ilen = sizeof(rbuf);

		read_and_show_C_string(addr, rbuf, ilen, buf, sizeof(buf));
		return buf;
	}
	
	*buf = '\0';
	want_hex = dv->dv_format == DF_SHEX || dv->dv_format == DF_UHEX;

	switch (type->ty_code) {
	case TY_CHARACTER:
		for (il = dv->dv_ilist; il->il_next != NULL; il = il->il_next)
			;
		
		len = il->il_high;
		
		if (len > sizeof(buf))
			len = sizeof(buf);

		if (dread(xp, addr, rbuf, len) == 0)
			show_string(rbuf, len, buf, sizeof(buf),
							'"', LANG_F90);
		break;

	case TY_BOOLEAN:
		if (addr_to_number(&val, addr, type))
		   strcpy(buf, val ? "true" : "false");
		break;

	case TY_LOGICAL:
		if (DREAD_VAR(xp, addr, vl.vl_logical)) {
			if (v->va_language == LANG_F90 &&
			    vl.vl_logical == EPCF90_UNASSIGNED_PATTERN)
				strcpy(buf, f90_unassigned_str);
			else
				strcpy(buf, vl.vl_logical ? ".true."
				       			  : ".false.");
		}
		break;

	case TY_COMPLEX:
		if (DREAD_VAR(xp, addr, vl.vl_float)) {
                        if (v->va_language == LANG_F90 &&
			    vl.vl_int == EPCF90_UNASSIGNED_PATTERN) {
                                strcpy(buf, f90_unassigned_str);
                                break;
                        }
			
			sprintf(buf, "(%s,", get_real(xp->xp_words_big_endian,
						      vl, want_hex, sizeof(float)));
		}
		
		if (DREAD_VAR(xp, addr + sizeof(float), vl.vl_float)) {
			sprintf(buf + strlen(buf), "%s)",
					       get_real(xp->xp_words_big_endian,
							vl, want_hex, sizeof(float)));
		}
		else {
			*buf = '\0';
		}
		break;
		
	case TY_DBLCOMP:
		if (DREAD_VAR(xp, addr, vl.vl_double)) {
			if (v->va_language == LANG_F90 &&
			    vl.vl_ints[0] == EPCF90_UNASSIGNED_PATTERN &&
			    vl.vl_ints[1] == EPCF90_UNASSIGNED_PATTERN) {
				strcpy(buf, f90_unassigned_str);
				break;
			}
			sprintf(buf, "(%s,", get_real(xp->xp_words_big_endian,
						      vl, want_hex, sizeof(double)));
		}
		
		if (DREAD_VAR(xp, addr + sizeof(double), vl.vl_double)) {
			sprintf(buf + strlen(buf), "%s)",
					       get_real(xp->xp_words_big_endian,
							vl, want_hex, sizeof(double)));
		}
		else {
			*buf = '\0';
		}
		
		break;
	
	case TY_CHAR:
	case TY_UCHAR:
		if (DREAD_VAR(xp, addr, vl.vl_char)) {
			if (is_signed_format(dv->dv_format))
				int_to_string(buf, sizeof(buf),
					      (vlong)vl.vl_char,
					      dv->dv_format);
			else
				int_to_string(buf, sizeof(buf),
					      (vlong)(vl.vl_char & (unsigned char)~0),
					      dv->dv_format);
		}
		break;
	case TY_USHORT:
	case TY_SHORT:
		if (DREAD_VAR(xp, addr, vl.vl_short)) {
			if (v->va_language == LANG_F90 &&
			    (unsigned short)vl.vl_short ==
			    (unsigned short)EPCF90_UNASSIGNED_PATTERN) {
                                strcpy(buf, f90_unassigned_str);
			}
                        else if (is_signed_format(dv->dv_format)) {
				int_to_string(buf, sizeof(buf),
					      (vlong)vl.vl_short,
					      dv->dv_format);
			}
			else
				int_to_string(buf, sizeof(buf),
					      (vlong)(vl.vl_short & (unsigned short)~0),
					      dv->dv_format);
		}
		break;

	case TY_INT:
	case TY_UINT:
		if (DREAD_VAR(xp, addr, vl.vl_int)) {
			if (v->va_language == LANG_F90 &&
			    vl.vl_int == EPCF90_UNASSIGNED_PATTERN) {
                                strcpy(buf, f90_unassigned_str);
			}
                        else if (is_signed_format(dv->dv_format)) {
				int_to_string(buf, sizeof(buf),
					      (vlong)vl.vl_int,
					      dv->dv_format);
			}
			else {
				int_to_string(buf, sizeof(buf),
					      (vlong)(vl.vl_int & (unsigned)~0),
					      dv->dv_format);
			}
		}
		break;
		
	case TY_LONG:
	case TY_ULONG:
		if (DREAD_VAR(xp, addr, vl.vl_long)) {
			int_to_string(buf, sizeof(buf),
				      (vlong)vl.vl_long, dv->dv_format);
		}
		break;

	case TY_LONGLONG:
	case TY_ULONGLONG:
#if HAVE_LONG_LONG
 		if (DREAD_VAR(xp, addr, vl.vl_longlong)) {
 			int_to_string(buf, sizeof(buf),
 				      (vlong)vl.vl_longlong, dv->dv_format);
 		}
#else
		strcpy(buf, "<long long not supported>");
#endif
		break;

	case TY_BITFIELD:
		if (addr_to_number(&val, addr, type->ty_bitfield->bf_type)) {
			typecode_t typecode;
			
			val = get_fieldval(xp->xp_bits_big_endian,
					   val, type->ty_bitfield,
					   is_signed_format(dv->dv_format));

			typecode = type->ty_bitfield->bf_type->ty_code;
			if ((typecode == TY_ENUM || typecode == TY_U_ENUM) &&
			    dv->dv_format == DF_NONE) {
				enumval_to_string(buf, sizeof(buf), val,
						  type->ty_bitfield->bf_type);
			}
			else {
				int_to_string(buf, sizeof(buf), val,
					      dv->dv_format);
			}
		}
		break;
		
	case TY_UNION:
	case TY_STRUCT:
	case TY_U_STRUCT:
	case TY_U_UNION:
		sprintf(buf, "*0x%lx", addr);
		break;
		
	case TY_FLOAT:
		if (dread_fpval(xp, addr, v->va_class == CL_REG, sizeof(float),
				(char *)&vl.vl_float) == 0) {
			if (v->va_language == LANG_F90 &&
			    vl.vl_int == EPCF90_UNASSIGNED_PATTERN) {
				strcpy(buf, f90_unassigned_str);
			}
			else {
				strcpy(buf, get_real(xp->xp_words_big_endian,
						     vl, want_hex, sizeof(float)));
			}
		}
		break;
		
	case TY_DOUBLE:
		if (dread_fpval(xp, addr, v->va_class == CL_REG, sizeof(double),
				(char *)&vl.vl_double) == 0) {
			if (v->va_language == LANG_F90 &&
			    vl.vl_ints[0] == EPCF90_UNASSIGNED_PATTERN &&
			    vl.vl_ints[1] == EPCF90_UNASSIGNED_PATTERN) {
				strcpy(buf, f90_unassigned_str);
			}
			else {
				strcpy(buf, get_real(xp->xp_words_big_endian,
						     vl, want_hex, sizeof(double)));
			}
		}
		break;
		
	case TY_LONGDOUBLE:
#if HAVE_LONG_DOUBLE
 		if (dread_fpval(xp, addr, v->va_class == CL_REG, sizeof(long double),
 				(char *)&vl.vl_longdouble) == 0) {
 		    strcpy(buf, get_real(xp->xp_words_big_endian,
 					 vl, want_hex, sizeof(long double)));
 		}
#else
		strcpy(buf, "<long double not supported>");
#endif
  		break;
	case TY_ENUM:
	case TY_U_ENUM:
		if (addr_to_number(&val, addr, type)) {
			if (dv->dv_format == DF_NONE) {
				enumval_to_string(buf, sizeof(buf), val, type);
			}
			else {
				int_to_string(buf, sizeof(buf), val,
					      dv->dv_format);
			}
		}
		break;

	default:
		strcpy(buf, "<unknown type>");
	}

	if (*buf == '\0')
		note_badaddr(buf, sizeof(buf), v->va_language, addr);
	
	return buf;
}

static void
note_badaddr(buf, bufsize, language, addr)
char *buf;
size_t bufsize;
language_t language;
taddr_t addr;
{
	if (addr == 0) {
		strcpy(buf, "NULL");
	}
	else if (language == LANG_F90 && addr == EPCF90_UNASSIGNED_PATTERN) {
		strnf(buf, bufsize, "<unassigned address>");
	}
	else {
		strnf(buf, bufsize, "<bad address 0x%lx>", addr);
	}
}
