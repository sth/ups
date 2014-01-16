/* va_menu.c - code handling actions initiated from the va menu */

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


/* @(#)va_menu.c	1.34 04 Jun 1995 (UKC) */
char ups_va_menu_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <local/wn.h>
#include <local/obj/obj.h>
#include <local/ukcprog.h>
#include <mtrprog/alloc.h>
#include <mtrprog/hash.h>

#include "objtypes.h"
#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "state.h"
#include "data.h"
#include "va.h"
#include "menudata.h"
#include "va_priv.h"
#include "ui.h"
#include "obj_stack.h"
#include "obj_wpt.h"
#include "tdr.h"
#include "st.h"
#include "ao_syms.h"
#include "ao_symload.h"
#include "ci.h"
#include "expr.h" /* RCB, for show_static_member() */

static char *make_pattern PROTO((const char *string, int len));
static int match PROTO((const char *pattern, const char *string, int len));
static void alloc_special PROTO((const char *pattern, int name_len,
				 typecode_t typecode, vformat_t format));
static const char *skip_comment PROTO((const char *format));
static int parse_identifier PROTO((const char **p_format, const char **start));
static vformat_t parse_format PROTO((const char **p_format));
static void parse_format_specifier PROTO((const char **format_spec));
static void parse_format_string PROTO((void));
static int match_types PROTO((typecode_t var_type, typecode_t format_type));
static vformat_t check_for_special_format PROTO((const char *name,
						 typecode_t typecode));
static taddr_t get_dv_addr PROTO((objid_t par, var_t *v, ilist_t *ilist,
				  bool *p_decl_may_have_changed));
static taddr_t get_vtbl PROTO((taddr_t addr, type_t *type));
static taddr_t get_vtbl_based_addr PROTO((taddr_t addr, type_t* type,
					  int offset));
static void dump_dv_to_outwin PROTO((dvar_t *dv));
static void do_var PROTO((objid_t obj, int cmd, char *arg));

ALLOC_NEW_FREE(static,dvar_t,dvar,dv_nextfree)

/*
**    Code to select the default format of some variables via an enviroment
**    variable.
**    By Russ Browne ( russ@amc.com )
**
**    The syntax for UPS_FORMATS is
**
**      UPS_FORMATS format [; format ]
**
**	ups_format_string ::= format_specifier [ ; ups_format_string ] [;]
**
**	format_specifier ::= type_identifier : format | "auto"
**
**	type_identifier ::= [ sign_specifer ] [ type_name ] [ identifier ]
**
**	sign_specifier ::= "unsigned"
**	type_name ::= "char" | "short" | "long" | "int"
**	identifier ::= < Any string up to a ';' a  ',' a ':' or white space
**
**	format ::= "UDEC" | "UDML"  | "SDEC" | "SDML" |
**		   "UHEX" | "SHEX" | "UOCT" | "SOCT" |
**		   "UBIN" | "ASCII" | "STRING"
**
**	In the type_identifier:
**
**	    if both the sign_specifier and type_name are omitted, the
**	    requested format applies to all non-derived variables of
**	    whose name matches the identifier.
**
**	    if the type name is omitted, the requested format applies
**	    to all matching variables which are signed/unsigned according
**	    to the sign specifer.
**
**	    if the sign_specifier is omitted and the type_specifier is
**	    used, the sign_specifier defaults to signed.  Thus it is
**	    redundent to specify signed unless one wants to specify
**	    a format for all variables of a given name regardless of size.
**	      ** Since this sound useless, the "signed" option is commented out **
**
**	    The identifier may contain the wild cards '*', '?', '[' and ']',
**	    which have the same meaning as in shells such as sh, csh, bash.
**	    If the identifier is omitted, it defaults to "*".    Thus, the
**	    format specifier "unsigned : UHEX" may be used to request that
**	    all unsigned values may be formatted as unsiged hex integers.
**
**	The special format string "auto" has the effect of automating the
**	"Save Format" window, so that anytime a format is changed the
**	change is recorded and applied to all variables of the same name
**	and type.   Note that "auto" is a C reserved word so it cannot
**	be the name of a variable.
**
*/

typedef struct _SPECIAL_FORMAT
{
    struct _SPECIAL_FORMAT* next;
    vformat_t format;
    typecode_t typecode;
    char* pattern;
}SPECIAL_FORMAT;

static int save_format = FALSE;	/* Automatically save format changes */
static int got_special_formats = FALSE;
SPECIAL_FORMAT *specials;

/*
**   We prepend one character to to input pattern string
**   and set the top two bits as follows:
*/
#define  NO_WILD    (0<<5)	/* First special char is '\0' */
#define  WILD_ANY   (1<<5)	/* First special char is '*' */
#define  WILD_ONE   (2<<5)	/* First special char is '?' */
#define  WILD_BUNCH (3<<5)	/* First special char is '[' */
#define  WILD_LONG  (4<<5)	/* First special char is > 31 chars away */
#define  WILD_MASK  (7<<5)
/*
** The lower 5 bits contain the number of non-special chars until the
** first special char.  Each special character in the pattern is
** then overwritten in the same fashion with to indicate the type
** distance to the next special character.
**
*/
static char* make_pattern( string, len)
const char* string;
int len;
{
    char* pattern = (char*)malloc(len+ 2 + (len>>5) );
    char* mark_len = pattern;
    char* P = mark_len+1;
    int l;

    if (pattern == NULL)
    {
	panic("out of memory for special format\n");
	return 0;
    }
    strncpy(P, string, len);
    P[len] = '\0';

    for (l=0; l <= 31 ; P++)
    {
	int type = 0;
	switch (*P)
	{
	case '\0':
	    type = NO_WILD;
	    break;
	    
	case '*':
	    type = WILD_ANY;
	    break;
	case '?':
	    type = WILD_ONE;
	    break;
	case '[':
	{
	     char *start = P;
	     while (*P && *P != ']') P++, len--;
	     if (*P)
	     {
		type = WILD_BUNCH;
		*start = P-start;
	     }
	}
	break;
	default:
	    if (l == 31)
	    {
		char* A = P+strlen(P);
		char* B = A+1;
		while (A >= P)
		    *B-- = *A--;
		type = WILD_LONG;
	    } else
	    {
	        l++;
	        continue;	/* for loop */
	    }
	}
	*mark_len = type | l;
	l = 0;
	mark_len = P;
	if (type == NO_WILD)
	    break; /* from for */
    }
    return pattern;
}

static int match(pattern, string, len)
const char* pattern;
const char* string;
int  len;	/* strlen(string) */
{
    unsigned char T;
    char L;

    for (;;)
    {
	T = (unsigned char) *pattern++;

	if (T < (unsigned char)~WILD_MASK)
	{
	    /* NO_WILD CASE */
	    return (T == len && !strcmp(pattern, string));
	}
	L = T & ~WILD_MASK;

	if ( len < L || strncmp(pattern, string, L) )
	    return 0;

	string += L;
	pattern += L;
	len -= L;
	switch ( T & WILD_MASK )
	{
	case WILD_ANY:	/* match '*' */
	  {
	    int l;
	    for (l = len; l >= 0; l--)
		if ( match(pattern, string+l, len-l) )
		    return 1;
	    return 0;
	  }
	case WILD_ONE:		/* mactch '?' with anything but '\0' */
	    if (len==0)
	       return 0;
	    string++;
	    len--;
	    break;
	
	case WILD_BUNCH:	/* match "[stuf]" */
	  {
	     const char* nextp = pattern + *pattern;	/* get end of [] expression */
	     char cstart;
	     char cend;
	     int  result = 0;
	     char s = *string++;
	     len --;

	     if (*++pattern == '^')	/* check for NOT */
		 result = 1, pattern++;

	     while ( pattern < nextp && (cstart = *pattern++) )
	     {
		cend = cstart;
		if (*pattern == '-' && pattern[1] != ']')
		    cend = *++pattern;

		if ( cstart <= s && s <= cend)
		{
		    result = !result;
		    break;	/* from while */
		}
	     }
	     if (!result)
		 return 0;
	     pattern = nextp;
	     break;
	  }

	}
    }
}

static void alloc_special(pattern, name_len, typecode, format)
const char *pattern;
int name_len;
typecode_t typecode;
vformat_t format;
{
    /*
    **   If we were the least bit concerned about memory use,
    **   we would scan the current list of special formats
    **   to see if the new format string is already on the list.
    **   We don't bother with that.
    **   We also don't bother to free the malloc'ed data on termination.
    */
    SPECIAL_FORMAT* special = (SPECIAL_FORMAT*)malloc(sizeof(SPECIAL_FORMAT));
    if (special == NULL)
    {
	panic("out of memory for special format\n");
	return;
    }

    special->pattern = make_pattern( pattern, name_len);
    special->format = format;
    special->typecode = typecode;

    special->next = specials;
    specials = special;
}

static const char* skip_comment( format)
const char* format;
{
     char C;

     while (   ((C = *format)== ' ' )
	    || C == '\t' )
	format++;

     if (C  != '/'  || format[1] != '*')
	return format;

     format+=2;
     while ((C=*format) && (C !='*' || format[1] != '/') )
	format++;
     if (C)
	format+=2;

     while (   ((C = *format)== ' ' )
	    || C == '\t' )
	format++;
     return format;

}
/*
**    scan for a C identifier, possibly including the wild card character '*'
*/
static int parse_identifier(p_format, start)
const char** p_format;
const char** start;
{
    const char* format = *p_format;
    const char* end;
    char C;

    /*   Skip white space and comments */

    format = skip_comment(format);

    *start = format;
    while (    (C = *format) > ' '
	    &&  C != ',' 
	    &&  C != ';'
	    &&  C != ':'
	    &&  C != '/'
	    )
	format++;
    end = format;

    format = skip_comment(format);

    *p_format = format;
    return end - *start;

}

static vformat_t parse_format(p_format)
const char** p_format;
{
     static struct FNAME { vformat_t format; const char* name; } fnames[] =
     {
	{ DF_SDEC,	"SDEC" }, 
	{ DF_SDEC,	"SDML" }, 
	{ DF_UDEC,	"UDEC" },
	{ DF_UDEC,	"UDML" },
	{ DF_SOCT,	"SOCT" },
	{ DF_UOCT,	"UOCT" },
	{ DF_SHEX,	"SHEX" },
	{ DF_UHEX,	"UHEX" },
	{ DF_ASCII,	"ASCII" },
	{ DF_STRING,	"STRING" },
	{ DF_UBIN,	"UBIN" },
     };
     const char* start;
     int i = sizeof(fnames)/sizeof(fnames[0]);

     int name_len = parse_identifier(p_format, &start);

/*  AMC Change:   don't match zero length name */
     if ( name_len == 0 )
	return DF_NONE;

     while (i--) if (!strncmp(start, fnames[i].name, name_len) )
     {
/*  AMC Change:   don't match ambiguous name name */
	 /*
	 **   Check for ambiguous names
	 */
	 int j = i;
         while (j--) if (!strncmp(start, fnames[j].name, name_len) )
	 {
	    errf( "Ambiguous format type %s in $UPS_FORMATS", start);
	    return DF_NONE;
	 }
	 return fnames[i].format;
     }

     /*
     **   Need error handling here
     */
     errf( "Unknown format type %s", start);
     return DF_NONE;
}


static void parse_format_specifier(format_spec)
const char** format_spec;
{
     const char* format = *format_spec;
     const char* identifier = "*";
     int len = 1;
     typecode_t typecode = TY_NOTYPE;
     vformat_t dv_format;
     int i;


     for (i = 0; i < 3; i++)
     {
	const char *start;
	int name_len = parse_identifier(&format, &start);

	if (name_len == 0) break;

	if ( i == 0)
	{
	    /*
	    **    check for "auto", "signed" and "unsigned"
	    */
	    if (!strncmp(start, "auto", sizeof("auto")-1) )
	    {
		save_format = TRUE;
		while (*format && *format != ';') format++;
		return;
	    } else
	    if (!strncmp(start, "unsigned", sizeof("unsigned")-1) )
	    {
		typecode = TY_UNSIGNED;
	    } else
	    {
		/* Advance to look for type */
		i=1;
	    }
	}
	if ( i == 1)
	{
	    /*
	    **    check for "char", "short" etc
	    */
	    if (!strncmp(start, "char", sizeof("char")-1) )
	    {
		typecode = (typecode == TY_UNSIGNED)
			   ? TY_UCHAR : TY_CHAR;
	    } else
	    if (!strncmp(start, "short", sizeof("short")-1) )
	    {
		typecode = (typecode == TY_UNSIGNED)
			   ? TY_USHORT : TY_SHORT;
	    } else
	    if (!strncmp(start, "long", sizeof("long")-1) )
	    {
		if ( sizeof(int) == sizeof(long) )
		    typecode = (typecode == TY_UNSIGNED)
				? TY_UINT : TY_INT;
		else
		    typecode = (typecode == TY_UNSIGNED)
				? TY_ULONG : TY_LONG;
	    } else
	    if (!strncmp(start, "int", sizeof("int")-1) )
	    {
		typecode = (typecode == TY_UNSIGNED)
			   ? TY_UINT : TY_INT;
	    } else
	    if (!strncmp(start, "double", sizeof("double")-1) )
	    {
		typecode = TY_NOTYPE;
	    } else
	    {
		i = 2;
	    }
	}
	if ( i == 2)
	{
	    identifier = start;
	    len = name_len;
	}
     }
     if ( *format == ':')
	format++;
     dv_format = parse_format(&format);
/*  AMC Change: Don't save DX_NONE */
     if ( dv_format != DF_NONE )
        alloc_special(identifier, len, typecode, dv_format);
     *format_spec = format;
}

static void parse_format_string()
{
    const char *format = getenv("UPS_FORMATS");
    got_special_formats = TRUE;
    if (format == NULL)
	return;

    while (*format)
    {
	parse_format_specifier( &format);
	while (*format && *format != ';') format++;
	if (*format) format++;
    }
}

static int
match_types(var_type, format_type)
typecode_t var_type;
typecode_t format_type;
{
    if (format_type == TY_NOTYPE )
	return TRUE;

    if (format_type == TY_UNSIGNED )
	return (    var_type == TY_UCHAR 
		 || var_type == TY_USHORT
		 || var_type == TY_UINT
		 || var_type == TY_ULONG );

    return ( var_type == format_type );
}
/*
**    Check for a list of variables to format specially
**	in the environment variable UPS_FORMATS
*/
static vformat_t check_for_special_format (name, typecode)
const char *name;
typecode_t typecode;
{
    SPECIAL_FORMAT* special_format;
    int len;

    /*
    **   First time, check the enviroment, and parse the list
    */
    if  (!got_special_formats)
	parse_format_string();

    if (specials == NULL)
	return DF_NONE;

    /*
    **    Scan the list for a match
    **       If found, return the type specified in the environment
    */
    len = strlen(name);
    special_format = specials;
    do
    {
	if (   match_types(typecode, special_format->typecode)
	    && match(special_format->pattern, name, len ) )
	    return special_format->format;

	    special_format = special_format->next;
    } while (special_format != NULL);

    return DF_NONE;
}

static taddr_t
get_dv_addr(par, v, ilist, p_decl_may_have_changed)
objid_t par;
var_t *v;
ilist_t *ilist;
int *p_decl_may_have_changed;
{
	taddr_t addr, pc, fp, ap, sp, cfa;
	target_t *xp;

	xp = get_current_target();

	if (v->va_location) {
		func_t *f = get_stack_func(par, &pc, &fp, &ap, &sp, &cfa);
		stf_t *stf = AO_FIDATA(f->fu_fil);
		vaddr_t *vaddr;

		while (stf->stf_parent)
			stf = stf->stf_parent;

		for (vaddr = v->va_location; vaddr; vaddr = vaddr->v_next) {
			if (pc - stf->stf_addr >= vaddr->v_low_pc &&
			    pc - stf->stf_addr < vaddr->v_high_pc ) break;
		}

		if (vaddr == NULL) {
			errf("no location match in gda");
			addr = BAD_ADDR;
		} else {
			switch (vaddr->v_op) {
			case OP_ADDR:
				addr = vaddr->v_addr;
				break;
			case OP_REGISTER:
				addr = get_reg_addr(xp, par, (int)vaddr->v_register,
						    (size_t)dynamic_type_size(v->va_type,
									      ilist));
				break;
			case OP_CFA_RELATIVE:
				addr = cfa + vaddr->v_offset;
				break;
			case OP_FP_RELATIVE:
				addr = fp + vaddr->v_offset;
				break;
			case OP_SP_RELATIVE:
				addr = sp + vaddr->v_offset;
				break;
			default:
				panic("unknown op in gda");
			}
		}
	} else {
		switch (v->va_class) {
		case CL_REF:
			(void) get_stack_func(par, &pc, &fp, &ap, &sp, &cfa);
                
			if (dread(xp, ap+v->va_addr, (char *)&addr, sizeof(addr)) != 0)
				addr = BAD_ADDR;
                
			break;
		case CL_ARG:
			(void) get_stack_func(par, &pc, &fp, &ap, &sp, &cfa);
			addr = ap + v->va_addr;
			break;
		case CL_AUTO:
	                get_stack_func(par, &pc, &fp, &ap, &sp, &cfa);
			addr = fp + v->va_addr;
                
			break;
		case CL_MOS:
		case CL_MOU:
			addr = var_or_expr_addr(par);
			if ( v->va_flags & VA_VTBLBASED)
			{
			    /* SC5 virtual base class.  True offset
			    ** is in the vtable.
			    */
			    type_t* type = 0;
			    switch (get_object_type(par))
			    {
			    case OT_VAR:
				type = deref_aggr((dvar_t*)par);
				break;
			    case OT_EXPR:
				type = get_expr_type(par);
				if ( type->ty_code == DT_PTR_TO)
				    type = type->ty_base;
				break;
			    default:
				panic("bad parent type in get_dv_addr");
			    }
			    addr = get_vtbl_based_addr(addr, type, v->va_addr);
			}
			else
			{
			    addr = addr + v->va_addr;
			}
			break;
		case CL_EXT:
		case CL_STAT:
		case CL_LSTAT:
			addr = v->va_addr;
			break;
		case CL_REG:
			addr = get_reg_addr(xp, par, (int)v->va_addr,
					    (size_t)dynamic_type_size(v->va_type,
								      ilist));
			break;
		default:
			panic("unknown class in gda");
		}
	}

	if (addr != BAD_ADDR) {
		if (v->va_language == LANG_F90) {
			/*  This is a frig for epcf90, based on poking around
			 *  in the symbol table.  For passed-by-reference
			 *  array pointers it seems to emit a pointer to
			 *  one word *before* the start of the dope vector.
			 */
			if (v->va_class == CL_REF &&
			    v->va_type->ty_code == DT_PTR_TO &&
			    v->va_type->ty_base->ty_code == DT_ARRAY_OF)
				addr += sizeof(taddr_t);
			
			fix_if_fortran_array_pointer(v->va_type, addr, ilist,
						     p_decl_may_have_changed);
		}
                
		if (v->va_language == LANG_F77) {
			fix_if_f77_dynamic_array(v->va_type, fp, ap, ilist,
						 p_decl_may_have_changed);
		}
	}
                
	return addr;
}

void delete_variable(obj)
     objid_t obj;
{
  do_expr(obj, MR_VAR_DELETE, 0);
}

/*  Create an object describing the variable, and insert it in the display
 *  tree as a child of par.  Poscode must be OBJ_FIRST_CHILD or OBJ_LAST_CHILD.
 *  par is also needed to obtain the values of register variables.
 *
 *  If pos is OBJ_LAST_CHILD, we read this as "add this object after the
 *  last child of par which is an OT_VAR object".  This is to maintain
 *  the rule that variables always come before other things like block
 *  headers.
 */
objid_t
add_var_object(par, v, poscode, restoring)
objid_t par;
var_t *v;
int poscode;
bool restoring;
{
	static bool first_call = TRUE;
	static long initial_dv_flags;
	objid_t code;
	dvar_t *dv;
	fval_t fields[FN_VAR_LAST + 1];
	int junk;

	if (first_call) {
		const char *tdstr;

		tdstr = wn_get_default("WantTypedefs");
		if (tdstr != NULL && strcmp(tdstr, "no") == 0)
			initial_dv_flags = DVF_NO_TYPEDEFS;
		first_call = FALSE;
	}

	dv = new_dvar();
	/* Dibyendu 26/12/98 
	 * Unnamed bitfields do not set the name.
	 * This caused UPS to fail when debugging interpeter programs
	 * containing unnamed bitfields.
	 */
	if (v->va_name == NULL)
		v->va_name = "unnamed";
	dv->dv_var = v;
	if (v)			/* RGA for changing targets */
	  dv->dv_va_name = strdup(v->va_name);
	else
	   dv->dv_va_name = NULL;
	dv->dv_ilevel = default_level(v->va_type, v->va_language);
	dv->dv_ilist = make_ilist(v->va_type, v->va_language, dv->dv_ilevel);
	dv->dv_addr = get_dv_addr(par, v, dv->dv_ilist, &junk);
	dv->dv_flags = initial_dv_flags;
/* AMC Change:  Should have been removed when UPS FORMATS
*  Installed.  It is done over if check_for_special_format finds nothing.
*	dv->dv_format = default_format(v->va_type,
*				       get_type_at_level(v, dv->dv_ilevel));
*/
		
		
	/*
	**   Check if the user requested a non-default format for
	**   this variable, or has changed the format
	*/
        {
	    type_t *type  = get_type_at_level(v, dv->dv_ilevel);
	    typecode_t typecode = type->ty_code;

	    if (   ISDERIV(typecode)
		|| DF_NONE == (dv->dv_format = check_for_special_format
			       (v->va_name, typecode )))
	    dv->dv_format = default_format(v->va_type, type );
	}

	switch (poscode) {
	case OBJ_FIRST_CHILD:
		code = par;
		break;
	case OBJ_LAST_CHILD:
		code = get_code(par, OBJ_CHILD);
		while (code != NULL && get_object_type(code) == OT_VAR)
			code = get_code(code, OBJ_NEXT);
		if (code != NULL)
			poscode = OBJ_BEFORE;
		else {
			code = par;
			poscode = OBJ_LAST_CHILD;
		}
		break;
	default:
		panic("bad poscode in add_var_object");
		code = 0; /* to satisfy gcc */
	}
	new_object((objid_t)dv, OT_VAR, code, poscode);

	fields[FN_VAR_DECL] = (fval_t) strsave(mkdecl(dv));
	fields[FN_VAR_VALUE] = (fval_t) strsave(mkval(dv));
	fields[FN_VAR_LAST] = (fval_t) NULL;
	set_all_fields((objid_t)dv, fields, (fval_t)NULL);

	if (!restoring)
		maybe_restore_var_state(par, dv);
	
	note_var_change((objid_t)dv);
	
	return (objid_t)dv;
}

/*  Fully dereference var dv if it is an aggregate (struct or union).
 */
type_t *
deref_aggr(dv)
dvar_t *dv;
{
	var_t *v;
	type_t *type, *btype;
	int ilevel;

	v = dv->dv_var;
	btype = get_basetype(v->va_type);
	type = get_type_at_level(v, dv->dv_ilevel);
	ilevel = dv->dv_ilevel;
	if (btype->ty_code == TY_STRUCT || btype->ty_code == TY_UNION)
		for (; type->ty_code == DT_PTR_TO || type->ty_code == DT_ARRAY_OF;
								type = type->ty_base)
			change_dv_level(dv, CL_UP);
	if (dv->dv_ilevel != ilevel)
		redo_decl(dv);
	return type;
}

void
free_displayed_var(obj)
objid_t obj;
{
	free((char *)get_field_value(obj, FN_VAR_DECL));
	free((char *)get_field_value(obj, FN_VAR_VALUE));
	free_ilist_list(((dvar_t *)obj)->dv_ilist);
	free(((dvar_t *)obj)->dv_va_name);
	free_dvar((dvar_t *)obj);
}

/*  Update the displayed declaration and value after a change.
 */
void
redo_decl(dv)
dvar_t *dv;
{
	fval_t fields[FN_VAR_LAST + 1];
	char *oldval, *olddecl;

	olddecl = (char *) get_field_value((objid_t)dv, FN_VAR_VALUE);
	oldval = (char *) get_field_value((objid_t)dv, FN_VAR_DECL);
	fields[FN_VAR_DECL] = (fval_t) strsave(mkdecl(dv));
	fields[FN_VAR_VALUE] = (fval_t) strsave(mkval(dv));
	fields[FN_VAR_LAST] = (fval_t) NULL;
	set_all_fields((objid_t)dv, fields, (fval_t)NULL);
	free(oldval);
	free(olddecl);

	note_var_change((objid_t)dv);
}

vformat_t
mval_to_vformat(cmd)
int cmd;
{
	switch (cmd) {
	case MR_VAR_STRING:		return DF_STRING;
	case MR_VAR_SIGNED_DECIMAL:	return DF_SDEC;
	case MR_VAR_UNSIGNED_DECIMAL:	return DF_UDEC;
	case MR_VAR_SIGNED_OCTAL:	return DF_SOCT;
	case MR_VAR_UNSIGNED_OCTAL:	return DF_UOCT;
	case MR_VAR_SIGNED_HEX:		return DF_SHEX;
	case MR_VAR_UNSIGNED_HEX:	return DF_UHEX;
	case MR_VAR_UNSIGNED_BINARY:	return DF_UBIN;
	case MR_VAR_ASCII_BYTE:		return DF_ASCII;
	case MR_VAR_DEFAULT_FORMAT:	return DF_NONE;
	default:
		panic("unknown format char in mtf");
		return DF_SDEC;		/* to satisfy gcc */
	}
}

const char *
deriv_to_string(typecode)
typecode_t typecode;
{
	switch(typecode) {
	case DT_ARRAY_OF:
		return "an array";
	case DT_PTR_TO:
		return "a pointer";
	case DT_FUNC_RETURNING:
		return "a function";
	default:
		panic("bad deriv code in dts");
		return NULL;	/* to keep gcc happy */
	}
	/* NOTREACHED */
}

const char *
var_getobjname(obj)
objid_t obj;
{
	static char *last = NULL;
	dvar_t *dv;
	const char *name;
	int count;

	dv = (dvar_t *)obj;

	name = dv->dv_var->va_name;

	count = 1;
	obj = get_code(get_code(obj, OBJ_PARENT), OBJ_CHILD);

	for (; obj != NULL; obj = get_code(obj, OBJ_NEXT)) {
		if (obj == (objid_t)dv)
			break;
		if (get_object_type(obj) != OT_VAR)
			continue;
		if (strcmp(name, ((dvar_t *)obj)->dv_var->va_name) == 0)
			++count;
	}

	if (last != NULL)
		free(last);

	if (count == 1) {
		last = NULL;
		return name;
	}

	last = strf("%d-%s", count, name);
	return last;
}

char *
var_format_obj(code)
objid_t code;
{
	const char *decl, *value;
	
	decl = get_field_value(code, FN_VAR_DECL);
	value = get_field_value(code, FN_VAR_VALUE);

	return strf("%-30s %s", decl, value);
}

void
do_var_or_expr(obj, cmd, arg)
objid_t obj;
int cmd;
char *arg;
{
	switch (get_object_type(obj)) {
	case OT_VAR:
		do_var(obj, cmd, arg);
		break;
	case OT_EXPR:
		do_expr(obj, cmd, arg);
		break;
	default:
		panic("unknown object type in dvoe");
		break;
	}
}

int
va_accelerator_action_from_type(type, fmt)
type_t *type;
vformat_t  fmt;
{
    int cmd;
    int issigned = 0;
    type_t* base_type;

    if (type == NULL)		/* RGA */
      return MR_VAR_ACCELERATOR_ACTION;

    if (type->ty_code == TY_BITFIELD)
      type = type->ty_bitfield->bf_type;

    /*
    **   Determine the most useful accelerator when a given
    **   data type is double clicked on
    */
    switch(type->ty_code) {
    case DT_PTR_TO:
    case DT_ARRAY_OF:
	for ( base_type = type;
	      base_type->ty_base;
	      base_type = base_type->ty_base)
	    continue;

	if (    IS_BASIC_TYPE(base_type->ty_code )
	     || base_type->ty_code == TY_ENUM)
	{
	  cmd = MR_VAR_DEREF;
	  break;
	}
	/* Else fall into stuct/union cases */
    case TY_U_STRUCT:
    case TY_U_UNION:
    case TY_STRUCT:
    case TY_UNION:
	cmd = MR_VAR_EXPAND_COMPLETELY;
	break;
    case TY_CHAR:
    case TY_SHORT:
    case TY_INT:
    case TY_LONG:
    case TY_LONGLONG:
	issigned = 1;
	/* Fall into unsigned cases */
    case TY_UCHAR:
    case TY_USHORT:
    case TY_UINT:
    case TY_ULONG:
    case TY_ULONGLONG:
	if ( fmt != DF_UHEX)
	   cmd = MR_VAR_UNSIGNED_HEX;
	else if ( issigned )
	   cmd = MR_VAR_SIGNED_DECIMAL;
	else
	   cmd = MR_VAR_UNSIGNED_DECIMAL;
	break;
    default:
	cmd = MR_VAR_ACCELERATOR_ACTION;
	break;
    }
   return cmd;
}

void
expand_statics(type)
type_t* type;
{
    var_t* v;
    const char* mesg = NULL;
    bool found  = FALSE;
    switch(type->ty_code) {
    case TY_STRUCT:
    case TY_UNION:
	break;
    case TY_U_STRUCT:
	mesg = "Can't expand statics for  undefined structure";
	break;
    case TY_U_UNION:
	mesg = "Can't expand statics for  undefined union";
	break;
    default:
	mesg = "Not a structure or union";
	break;
    }
    if (mesg != NULL) {
        errf(mesg);
        return;
    }

    for (v = type->ty_aggr_or_enum->ae_aggr_members;
		     v != NULL;
		     v = v->va_next)
    {
	if ( v->va_flags & VA_STATIC_MEMBER)
	{
	    found = TRUE;
	    show_static_member(type,v);
	}
    }
    if (!found && (ao_compiler(NULL, FALSE, CT_UNKNOWN) == CT_CLCC))
    {				/* RGA try globals (needed for CLCC) */
      func_t *cur_f;
      fil_t *fil = 0;
      taddr_t fp, pc, adjpc;
      char buff[200];
      const char *fi_name = 0;
      int fi_name_len = 0;

      sprintf(buff, "%s::*", type->ty_aggr_or_enum->ae_tag);
      get_current_func(&cur_f, &fp, &pc, &adjpc);
      if (cur_f)
	fil = cur_f->fu_fil;
      if (fil)
      {
	fi_name = fil->fi_name;
	fi_name_len = strlen(fil->fi_name);
      }
      found = find_and_show_matching_globals(fil, fi_name, fi_name_len, buff);
    }
    if ( !found)
	errf("No static members");
}

taddr_t
expand_hierarchy(obj, type)
objid_t obj;
type_t* type;
{
    dvar_t* dv_vtbl = NULL;
    taddr_t vtbl_addr;

    var_t *v;

    while ( type->ty_code == DT_PTR_TO)
	type = type->ty_base;

    if ( type->ty_code != TY_STRUCT)
	return BAD_ADDR;
    for (v = type->ty_aggr_or_enum->ae_aggr_members;
	v != NULL;
	v = v->va_next)
    {
	if ( v->va_flags & VA_VTBLPTR)
	{
	    dv_vtbl = ( dvar_t*) find_var(obj, v);
	    if ( dv_vtbl == NULL)
	        dv_vtbl = (dvar_t*) add_var_object(obj, v, OBJ_FIRST_CHILD, FALSE);
	    if ( dv_vtbl->dv_addr != BAD_ADDR)
	    {
		target_t* xp = get_current_target();
		if (dread(xp, dv_vtbl->dv_addr, (char *)&vtbl_addr, sizeof(vtbl_addr)) != 0)
		    vtbl_addr = BAD_ADDR;
	    } else
		    vtbl_addr = BAD_ADDR;
	} else if ( v->va_flags & VA_BASECLASS)
	{
	    taddr_t sub_addr;
	    dvar_t* dv = ( dvar_t*) find_var(obj, v);
	    if ( dv == NULL)
	        dv = (dvar_t*) add_var_object(obj, v, OBJ_FIRST_CHILD, FALSE);
	    sub_addr = expand_hierarchy((objid_t)dv,deref_aggr(dv));


	    /* If the base class displayed the same vtbl as previously displayed on
	    ** this level, remove the initial display
	    */
	    if ( dv_vtbl && ( sub_addr == vtbl_addr) )
	    {
		remove_object((objid_t)dv_vtbl, OBJ_SELF);
		dv_vtbl = NULL;
	    }
	}
    }
    return vtbl_addr;
}
/* Get the address of the vtable for the object at the given address.
** Return BAD_ADDR if no vtable found or other error situations.
** In the case of multiple inheritance, returns the first vtable
** found, which should be the vtable for the first base class.
*/
static taddr_t
get_vtbl(addr, type)
taddr_t addr;
type_t* type;
{
    taddr_t vtbl_addr = BAD_ADDR;
    target_t* xp = get_current_target();

    var_t *v;

    while ( type->ty_code == DT_PTR_TO)
    {
	if (dread(xp, addr, (char *)&addr, sizeof(addr)) != 0)
	    return BAD_ADDR;
	type = type->ty_base;
    }

    if ( type->ty_code != TY_STRUCT)
	return BAD_ADDR;

    for (v = type->ty_aggr_or_enum->ae_aggr_members;
	v != NULL;
	v = v->va_next)
    {
	if ( v->va_flags & VA_VTBLPTR)
	{
	    if (dread(xp, addr+v->va_addr, (char *)&vtbl_addr, sizeof(vtbl_addr)) != 0)
		return BAD_ADDR;
	    break;
	}
	if ( v->va_flags & VA_BASECLASS)
	{
	    vtbl_addr = get_vtbl(addr+v->va_addr, v->va_type);
	}
    }
    return vtbl_addr;
}


/* Return the address of a struct member whose offset into the class
** is given by an entry in the vtable.  Offset is the entry in the vtable
** to look at, and may be ( actually, is) negative.  This mechanism
** is used by SC5 for virtual base classes.
*/
static taddr_t
get_vtbl_based_addr(addr, type, offset)
taddr_t addr;
type_t* type;
int     offset;
{
    taddr_t vtbl_addr = get_vtbl(addr, type);
    if ( vtbl_addr != BAD_ADDR)
    {
	  int true_offset;
          target_t* xp = get_current_target();
	  if ( dread(xp, vtbl_addr+offset, (char *)&true_offset, sizeof(true_offset)) == 0)
	        return  addr+true_offset;
    }
    return BAD_ADDR;
}

static void dump_dv_to_outwin(dv)
dvar_t* dv;
{
    char buff[80];
    var_t* v = dv->dv_var;
    type_t* type  = get_type_at_level(v, dv->dv_ilevel );
    taddr_t addr = addr = dvar_addr(dv);
    int length = 0;
    char grpchar = ' ';



    if (   type->ty_code == DT_PTR_TO)
    {
	/*  For a pointer, Get the address pointed to */
	taddr_t addr2;
	target_t* xp = get_current_target();
	if (dread(xp, addr, (char *)&addr2, sizeof(addr)) == 0)
	    addr = addr2;

	/* and the type pointed to */
	type = type->ty_base;
	if (   IS_BASIC_TYPE(type->ty_code)
	    || type->ty_code == TY_NOTYPE)    /* For void */
	    length = 256;
    } else if ( dv->dv_ilevel )
    {
	/*  If the object is an array element, dump to the end of the array */
	type_t* atype  = get_type_at_level(v, dv->dv_ilevel-1);
	if ( atype->ty_code == DT_ARRAY_OF)
	{
	    int array_size = ci_typesize(NULL, atype);
	    ilist_t *il = dv->dv_ilist;
	    int i = dv->dv_ilevel;
	    while (--i )
		il = il->il_next;
	    if ( il->il_low_known)
	        length = array_size
			 -ci_typesize(NULL, type)*(il->il_index-il->il_low);
	} else if ( dv->dv_format == DF_STRING)
	{
	    length = 256;
	}
    }

    if ( length <= 0)
    {
        length = ci_typesize(NULL, type);
	if (length <= 0 )
	    length = 16;
    }

    /*   Decide whether to group the data as bytes, shorts, or longs */
    if ( IS_BASIC_TYPE(type->ty_code))
    {
	switch (ci_typesize(NULL, type))
	{
	case 1:
	    grpchar = 'b'; break;
	case 2:
	case 10: /* for long doubles */
	    grpchar = 's'; break;
	case 4:
	case 8:
	    grpchar = 'l'; break;
	default:
	    break;
	}
    } else switch (type->ty_code)
    {
    case DT_PTR_TO:
    case DT_FUNC_RETURNING:
    case DT_ARRAY_OF:
    case TY_U_ENUM:
	grpchar = 'l';
	break;
    default:
        break;
    }

    sprintf(buff,"%%d%c %lx %d", grpchar,addr,length);
    dump_memory (buff);
}

/*  Process the return from a var menu.
 */
static void
do_var(obj, cmd, arg)
objid_t obj;
int cmd;
char *arg;
{
	dvar_t *dv, *dv2;
	type_t *type;
	const char *mesg;
	var_t *v;
	bool hide_ptr;
	int oldstate, is_ptr_to_func;
	fval_t fields[FN_VAR_LAST + 1];

	dv = (dvar_t *)obj;
	v = dv->dv_var;

	hide_ptr = (v->va_flags & VA_HIDE_PTR) != 0;
	type = get_type_at_level(v, dv->dv_ilevel);

	if ( cmd == MR_VAR_ACCELERATOR_ACTION )
	  cmd = va_accelerator_action_from_type(type, dv->dv_format);


	switch(cmd) {
	case MR_VAR_STRING:
	case MR_VAR_SIGNED_DECIMAL:
	case MR_VAR_UNSIGNED_DECIMAL:
	case MR_VAR_SIGNED_OCTAL:
	case MR_VAR_UNSIGNED_OCTAL:
	case MR_VAR_SIGNED_HEX:
	case MR_VAR_UNSIGNED_HEX:
	case MR_VAR_UNSIGNED_BINARY:
	case MR_VAR_ASCII_BYTE:
	case MR_VAR_DEFAULT_FORMAT:
		dv->dv_format = mval_to_vformat(cmd);

		if (dv->dv_format == DF_NONE)
			dv->dv_format = default_format(v->va_type, type);

		oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
		redo_decl(dv);
		td_set_obj_updating(oldstate);
		note_format_change(dv);
		break;
		
	case MR_VAR_DEREF:
		if (hide_ptr || !ISDERIV(type->ty_code)) {
			errf("Can't indirect through %s", v->va_name);
			break;
		}

		if (type->ty_code == DT_PTR_TO && type->ty_base->ty_code == DT_FUNC_RETURNING) {
			/*  Function address
			 */
			errf("Can't indirect through a pointer to a function");
			break;
		}

		if (type->ty_code == DT_PTR_TO && type->ty_base->ty_code == TY_VOID) {
			errf("Can't indirect through a void *");
			break;
		}

		if (type->ty_code == DT_PTR_TO &&
				(type->ty_base->ty_code == TY_U_STRUCT ||
				 type->ty_base->ty_code == TY_U_UNION)) {
			errf("Can't indirect through a pointer to an undefined type");
			break;
		}

		do {
			change_dv_level(dv, CL_UP);
			type = type->ty_base;
		} while (type->ty_code == DT_ARRAY_OF);


		if (   ISDERIV(type->ty_code)
		    ||    DF_NONE
		       == (dv->dv_format = check_for_special_format( v->va_name, type->ty_code )))

		    dv->dv_format = default_format(v->va_type, type);

		oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
		redo_decl(dv);
		td_set_obj_updating(oldstate);

		note_format_change(dv);
		
		break;
	case MR_VAR_ADDRESS:
		if (hide_ptr || dv->dv_ilevel == 0) {
			errf("Can't decrease the indirection level of %s",
								v->va_name);
			break;
		}

		do {
			change_dv_level(dv, CL_DOWN);
			type = get_type_at_level(v, dv->dv_ilevel);
		} while (type->ty_code == DT_ARRAY_OF && dv->dv_ilevel > 0);

		for (; type->ty_code == DT_ARRAY_OF; type = type->ty_base)
			change_dv_level(dv, CL_UP);

		dv->dv_format = default_format(v->va_type, type);
		oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
		redo_decl(dv);
		td_set_obj_updating(oldstate);

		note_format_change(dv);

		break;
	case MR_VAR_EXPAND_LIKE_BEFORE:
		errf("elb NYI");
		break;
	case MR_VAR_EXPAND_COMPLETELY:
		is_ptr_to_func = FALSE;
		for (type = v->va_type; type != NULL; type = type->ty_base)
			if (type->ty_code == DT_FUNC_RETURNING)
				is_ptr_to_func = TRUE;

		if (is_ptr_to_func) {
			errf("Can't expand a pointer to a function");
			break;
		}
		type = deref_aggr(dv);

		switch(type->ty_code) {
		case TY_STRUCT:
		case TY_UNION:
			mesg = NULL;
			break;
		case TY_U_STRUCT:
			mesg = "Can't expand undefined structure %s";
			break;
		case TY_U_UNION:
			mesg = "Can't expand undefined union %s";
			break;
		default:
			mesg = "%s is not a structure or union";
			break;
		}
		if (mesg != NULL) {
			errf(mesg, v->va_name);
			break;
		}

		for (v = type->ty_aggr_or_enum->ae_aggr_members;
		     v != NULL;
		     v = v->va_next)
		{
		    /*
		    **   Skip static members, which we display as globals.
		    **   Skip vector tables, if thats what the user wants to see
		    **   the user should use the expand_hierarchy command.
		    */
		    if (   !( v->va_flags & (VA_STATIC_MEMBER|VA_VTBLPTR))
			&& find_var((objid_t)dv, v) == NULL)
				add_var_object((objid_t)dv, v, OBJ_FIRST_CHILD,
					       FALSE);
		}
		break;
	case MR_VAR_SHOW_HIERARCHY:
	        expand_hierarchy((objid_t)dv,deref_aggr(dv));
		break;
	case MR_VAR_SHOW_STATICS:
	        expand_statics(deref_aggr(dv));
		break;
	case MR_VAR_DUMP_TO_OUTWIN:
		dump_dv_to_outwin(dv);
		break;
	case MR_VAR_SHOW_SIZE:
		show_dv_size(dv);
		break;
	case MR_VAR_DUP:
		dv2 = new_dvar();
		*dv2 = *dv;
		if (dv->dv_va_name) dv2->dv_va_name = strdup(dv->dv_va_name);
		dv2->dv_ilist = dup_ilist(dv->dv_ilist);
/*
**		Auto increment the highest index when duplicating
**		a pointer.
*/

		if ( dv2->dv_ilevel > 0
		    && dv2->dv_ilist != NULL )
		{
		    /*  For strings, go 40 char's into the string. */
		    if (dv2->dv_format == DF_STRING
			&& dv2->dv_ilevel == 1 )
		        dv2->dv_ilist->il_index += 40;
		    else
		        dv2->dv_ilist->il_index++;
		}
		fields[FN_VAR_VALUE] = (fval_t) strsave(mkval(dv2));
		fields[FN_VAR_DECL] = (fval_t) strsave(mkdecl(dv2));
		fields[FN_VAR_LAST] = (fval_t) NULL;
		new_object((objid_t)dv2, OT_VAR, (objid_t)dv, OBJ_AFTER);
		set_all_fields((objid_t)dv2, fields, (fval_t)NULL);
		add_to_new_selection((objid_t)dv2);
		note_var_change((objid_t)dv);
		break;
	case MR_VAR_DELETE:
		note_var_change((objid_t)dv);
		remove_object((objid_t)dv, OBJ_DESCENDENTS);
		remove_object((objid_t)dv, OBJ_SELF);
		break;
	case MR_VAR_COLLAPSE:
		note_var_change((objid_t)dv);
		remove_object((objid_t)dv, OBJ_CHILDREN);
		break;
	case MR_VAR_COLLAPSE_COMPLETELY:
		note_var_change((objid_t)dv);
		remove_object((objid_t)dv, OBJ_DESCENDENTS);
		break;
	case MR_VAR_WANT_TYPEDEFS:
		dv->dv_flags &= ~DVF_NO_TYPEDEFS;
		redo_decl(dv);
		break;
	case MR_VAR_NO_TYPEDEFS:
		dv->dv_flags |= DVF_NO_TYPEDEFS;
		redo_decl(dv);
		break;
	case MR_VAR_ADD_WATCHPOINT:
		if (v->va_class == CL_REG && dv->dv_ilevel == 0) {
			errf("Cannot watch a register variable");
		}
		else {
			taddr_t addr = dvar_addr(dv);
			size_t size = typesize(get_type_at_level(v, dv->dv_ilevel));
			const char *decl = get_field_value((objid_t)dv, FN_VAR_DECL);

			add_watchpoint_object(addr, size, decl);
		}
		break;

	case MR_VAR_ACCELERATOR_ACTION:
		break;	/* Silently ignore when no quick change action defined */
	default:
		panic("bad cmd in dv");
	}
}

void
update_struct(par, change_caused_by_edit)
objid_t par;
int change_caused_by_edit;
{
	objid_t obj;

	obj = get_code(par, OBJ_CHILD);
	for (; obj != NULL; obj = get_code(obj, OBJ_NEXT))
		update_var(obj, change_caused_by_edit);
}

/*  Update the value of the variable obj.  If it is a structure, update
 *  any displayed members.
 *
 *  The change_caused_by_edit argument is non zero if the reason the address
 *  has changed is simply that the user has changed the display. This happens,
 *  for example when the user edits the index in an array of structures.
 *  We normally flag variable values that have changed, but we don't want
 *  to flag ones that have changed simply because of a user edit.
 */
void
update_var(obj, change_caused_by_edit)
objid_t obj;
bool change_caused_by_edit;
{
	int decl_may_have_changed, decl_changed, val_changed;
	dvar_t *dv;
	typecode_t btype;

	dv = (dvar_t *) obj;

	decl_may_have_changed = FALSE;
	dv->dv_addr = get_dv_addr(get_code(obj, OBJ_PARENT),
				  dv->dv_var,
				  dv->dv_ilist,
				  &decl_may_have_changed);

	if (decl_may_have_changed)
		decl_changed = change_field(obj, FN_VAR_DECL, mkdecl(dv));
	else
		decl_changed = FALSE;
	val_changed = change_field(obj, FN_VAR_VALUE, mkval(dv));

	if ((decl_changed || val_changed) && !change_caused_by_edit)
		dv->dv_flags |= DVF_VAL_CHANGED;
	else
		dv->dv_flags &= ~DVF_VAL_CHANGED;

	btype = get_basetype(dv->dv_var->va_type)->ty_code;
	if (btype == TY_STRUCT || btype == TY_UNION)
		update_struct(obj, change_caused_by_edit);
}

/*  Update the values of the variables which are the children of par.
 *  See update_var for the meanings of the other arguments.
 */
void
update_vars_of(par, change_caused_by_edit)
objid_t par;
int change_caused_by_edit;
{
	objid_t obj;

	obj = get_code(par, OBJ_CHILD);
	for (; obj != NULL; obj = get_code(obj, OBJ_NEXT)) {
		switch(get_object_type(obj)) {
		case OT_EXPR:
			update_expr(obj, DF_NONE, change_caused_by_edit);
			break;
		case OT_VAR:
			update_var(obj, change_caused_by_edit);
			break;
		case OT_BLOCK:
			update_vars_of(obj, change_caused_by_edit);
			break;
		}
	}
}
int match_name_start_in_obj(name, given_obj, obj_exists, obj_var)
char *name;
objid_t given_obj;
int *obj_exists;
int **obj_var;
{
  sel_t *sel;
  objid_t obj, var_obj;
  dvar_t *dv;
  type_t *type;
  const char *mesg;
  var_t *v, *sub_v;
  int is_ptr_to_func, found = 0, str_match;
  taddr_t addr;
  type_t *btype;

  *obj_exists = 0;
  *obj_var = 0;
  if (!can_get_target_vars(get_current_target()))
    return found;

  if (given_obj)
    obj = given_obj;
  else
  {
    sel = get_selection();
    obj = (objid_t)sel->se_code;
  }
  dv = (dvar_t *)obj;
  v = dv->dv_var;
  
  type = get_type_at_level(v, dv->dv_ilevel);
  
  
  is_ptr_to_func = FALSE;
  for (type = v->va_type; type != NULL; type = type->ty_base)
    if (type->ty_code == DT_FUNC_RETURNING)
      is_ptr_to_func = TRUE;
  
  if (is_ptr_to_func) {
    errf("Can't expand a pointer to a function");
    return found;
  }
  type = deref_aggr(dv);
  
  switch(type->ty_code) {
  case TY_STRUCT:
  case TY_UNION:
    mesg = NULL;
    break;
  case TY_U_STRUCT:
    mesg = "Can't expand undefined structure %s";
    break;
  case TY_U_UNION:
    mesg = "Can't expand undefined union %s";
    break;
  default:
    mesg = "%s is not a structure or union";
    break;
  }
  if (mesg != NULL) {
    errf(mesg, v->va_name);
    return found;
  }

  for (v = type->ty_aggr_or_enum->ae_aggr_members; v != NULL; v = v->va_next)
  {
    if (demangling_enabled(0, 0))
      str_match = !strcmp(v->va_name, name);     
    else
      str_match = !!strstr(v->va_name, name);
    if ( str_match && v->va_flags & VA_STATIC_MEMBER)
    {
	show_static_member(type,v);
	found = 1;
	break;
    } else
    /* RGA Note return value of get_member_of_aggr() changed from 2.45.2 */
    if (str_match && !get_member_of_aggr(obj, v->va_name, &sub_v, &addr, &btype))
    {
      var_obj = find_var((objid_t)dv, sub_v);
      if (var_obj)
	*obj_exists = 1;
      else
	var_obj = add_var_object((objid_t)dv, sub_v, OBJ_FIRST_CHILD, FALSE);
      *obj_var = (int *)var_obj;
      if (!given_obj)
      {
	clear_selection();
	select_object(var_obj, TRUE, OBJ_SELF);
	ensure_visible(var_obj);
      }
      found = 1;
      break;
    }
  }

  /* look for components of unions .... */
  if (!found) /* go one level up */
    for (v = type->ty_aggr_or_enum->ae_aggr_members; v != NULL; v = v->va_next)
      if ( (v->va_type->ty_code == TY_UNION || v->va_type->ty_code == TY_STRUCT) &&
	!get_member_of_aggr(obj, v->va_name, &sub_v, &addr, &btype))
    /* RGA Note return value of get_member_of_aggr() changed from 2.45.2 */
      {
	var_obj = find_var((objid_t)dv, sub_v);
	if (var_obj)
	  *obj_exists = 1;
	else
	  var_obj = add_var_object((objid_t)dv, sub_v, OBJ_FIRST_CHILD, FALSE);
	found = match_name_start_in_obj(name, var_obj, obj_exists, obj_var);
	if (found)
	{
	  clear_selection();
	  select_object(*obj_var, TRUE, OBJ_SELF);
	  ensure_visible(*obj_var);
	  break;
	}
	else
	  delete_variable(var_obj);
      }
  return found;
}

