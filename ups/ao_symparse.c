/* ao_symparse.c - code to parse a type string */

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


/* @(#)ao_symparse.c	1.7 04 Jun 1995 (UKC) */
char ups_ao_symparse_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#ifdef AO_TARGET

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef AO_ELF
#	include "elfstab.h"
#else
#	include <a.out.h>
#	ifndef OS_RISCOS
#		include <stab.h>
#	endif
#endif

#include <local/ukcprog.h>
#include <mtrprog/strcache.h>
#include <mtrprog/hash.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "ci_util.h"  /* RCB: for ci_set_sizeof_bool() */
#ifdef ARCH_MIPS
#include "mips_frame.h"
#endif
#include "target.h"
#include "st.h"
#include "ao_syms.h"
#include "ao_symload.h"
#include "ao_symread.h"
#include "ao_symparse.h"
#include "ao_symscan.h"

/*  This file parses the symbol table grammar given in the 4.3bsd manual
 *  page dbx(5) and information in the document "SPARCworks 3.0.x Debugger
 *  Interface" (on the SPARCworks CDROM).
 *
 *  An updated version of the above document is "Stabs Interface Manual, Version
 *  4.0 (Febuary, 1999).  Known as READMEs/stabs.ps on the SunPro5 distrubution.
 *
 *  Information on stabs format for gcc was obtained from:
 *   THE "STABS" DEBUG FORMAT
 *      Julia Menapace, Jim Kingdon, David MacKenzie
 *      Copyright (C) 1992, 1993 Free Software Foundation, Inc.
 *      http://www.cygnus.com/misc/gnu-win32/stabs.html
 *
 *  The routine names are the same as the non terminal names in the grammer.
 *  Above each routine is a description of the part of the grammar that
 *  the routine parses.  "Comments" in these grammar fragments are
 *  bracketed with '{' and '}'.
 *
 *  We don't parse the whole grammar - constructs that we don't parse
 *  are marked with the comment NI (not implemented).  We deviate from
 *  the grammar in some cases - these are marked with DEV.
 *  Partially implemented constructs are marked with PI.
 *
 *  The Sun C compiler generates type numbers (given as INTEGER in the grammar)
 *  of the form " '(' Filenum ',' Typenum ')' ".
 */


#ifndef ST_TE
struct Ftype {
	int tnum;
	bool have_type;
	union {
		type_t type;
		struct {
			bool is_undef_tag;
			bool maybe_better_def_exists;
			int symno;
			int offset;
		} defpos;
	} u;
	Ftype *next;
};

/*  Function prototypes.
 */

static void notimp PROTO((const char *s));
static int parse_signed_num PROTO((stf_t *stf, Symrec *sr, const char **p_s));
static type_t *TypeDef PROTO((stf_t *stf, Symrec *sr, const char **p_s,
							int tnum, bool eval));
static aggr_or_enum_def_t *EnumList PROTO((stf_t *stf, symtab_t *st, Symrec *sr,
					   const char **p_s, type_t *type));
static void Enum PROTO((stf_t *stf, symtab_t *st, const char **p_s,
			aggr_or_enum_def_t *ae, Symrec *sr));
static aggr_or_enum_def_t *Record PROTO((stf_t *stf, Symrec *sr, const char **p_s,
					 bool is_struct, bool eval, type_t *type));
static var_t *GnuBaseClass PROTO((stf_t *stf, Symrec *sr, const char **p_s,
			   bool eval, Compiler_type compiler));
static void GnuStructMethods PROTO((stf_t *stf, Symrec *sr, const char **p_s));
static var_t *SunProBaseClass PROTO((stf_t *stf, Symrec *sr, const char **p_s,
			   bool eval, Compiler_type compiler));
static void SunProStructMethods PROTO((stf_t *stf, Symrec *sr, const char **p_s));
static void SkipPastSemi PROTO((stf_t *stf, Symrec *sr, const char **p_s));
static void SkipToSplChar PROTO((stf_t *stf, Symrec *sr, const char **p_s, const char* chars));
static var_t* SunProStaticMembers PROTO((stf_t *stf, Symrec *sr, const char **p_s,
			   var_t* next, Compiler_type compiler));
static var_t* SunProVectorTable PROTO((stf_t *stf, Symrec *sr, const char **p_s,
			   var_t* next, Compiler_type compiler));
static var_t *Field PROTO((stf_t *stf, Symrec *sr, const char **p_s,
			   int is_struct, var_t *next, bool eval,
			   Compiler_type compiler, bool *terminate));
static dim_t *Subrange PROTO((stf_t *stf, Symrec *sr, const char **p_s,
					bool eval, bool want_subrange_type));

static void get_basic_type PROTO((stf_t *stf, language_t language, symtab_t *st,
				  const Symrec *sr, dim_t *dim, type_t *type));
static int get_def PROTO((stf_t *stf, Symrec *sr, symio_t *symio, int tnum,
			  int min, int max, Ftype **p_ft));
static type_t *get_type PROTO((stf_t *stf, Symrec *sr, symio_t *symio,
			       Ftype *ft, int tnum, bool have_undef_tag,
			       int min, int max));
static type_t *tnum_to_type PROTO((stf_t *stf, Symrec *sr,
				   symio_t *symio, int tnum,
				   bool have_undef_tag));
static type_t *lookup_tnum PROTO((stf_t *stf, int tnum));
static Ftype *insert_ftype PROTO((stf_t *stf, int tnum));
static void fix_f90_pointers PROTO((alloc_pool_t *ap, aggr_or_enum_def_t *ae));
static void get_basic_integer PROTO((stf_t *stf, alloc_pool_t *ap,
				     const Symrec *sr,
				     language_t language,
				     const char **p_s, type_t *type));
static void get_basic_float PROTO((stf_t *stf, alloc_pool_t *ap,
				   const Symrec *sr,
				   const char **p_s, type_t *type));
static void get_typename_str PROTO((const Symrec *sr, 
                                    const char **p_typename, size_t *p_len));
static const char *make_typename PROTO((alloc_pool_t *ap, const char *name, 
					size_t len));
static int parse_subrange_signed_num PROTO((stf_t *stf, Symrec *sr,
					    const char **p_s));
static void make_undef_type PROTO((stf_t *stf, const char *tag, 
				   typecode_t utypecode, type_t *type));
static void get_subrange_dim PROTO((stf_t *stf, Symrec *sr, const char **p_s,
				    Dimtype *p_dimtype, 
                                    long *p_val));
static Ftype *note_def_pos PROTO((stf_t *stf, int tnum, int symno, int offset,
				  bool is_undef_tag));
static Ftype *add_ftype_to_list PROTO((stf_t *stf, int tnum, bool have_type, 
                                         Ftype *prevft));
static void lookup_ftype PROTO((stf_t *stf, int tnum, Ftype **p_ft, 
                                Ftype **p_prevft));

static type_t* get_safe_type_ptr PROTO((void));

static void patch_ftypes PROTO((stf_t *stf));

/*  The address of this variable is used as a special value for ty_typedef
 *  to signal that it is a basic type not a user defined typedef.
 */
static typedef_t Is_basic_type;

/*  If **p_s is ch, move *p_s on, otherwise abort.  Used for checking
 *  and swallowing required terminals in the grammar.
 */
void
scheck(sr, p_s, ch)
Symrec *sr;
const char **p_s;
int ch;
{
	if (**p_s != ch)
	{
		errf("syntax error in symtab : got '%c' expected '%c'",
		     **p_s, (char)ch);
		field_scheck(sr, p_s, ch);
	}
	bump_str(sr, p_s);
}

int
field_scheck(sr, p_s, ch)
     Symrec *sr;
     const char **p_s;
     int ch;
{
  int ret = 1;
  if (**p_s == '\\')
    *p_s = get_cont_symstring(sr);
  if (**p_s != ch)	
  {
    const char *s1;
    s1 = strchr(*p_s, ch);
    if (s1)
      *p_s = s1;
    else
    {
/*      errf("syntax error in symtab");*/
      *p_s += strlen(*p_s) - 2;
      ret = 0;
    }
  }
  bump_str(sr, p_s);
  return ret;
}

/*  Oops - we encountered a construct which we haven't implemented in the
 *  grammar.
 */
static void
notimp(s)
const char *s;
{
	errf("%s not implemented", s);
}

/*  Return the symbol number for the definition of type number tnum
 *  in file stf.
 *  Search the symbols between min and lim.
 */
static bool
get_def(stf, sr, symio, match_tnum, min, max, p_ft)
stf_t *stf;
Symrec *sr;
symio_t *symio;
int match_tnum, min, max;
Ftype **p_ft;
{
	int symno;
	bool got_match;
	ao_stdata_t *ast;
	Symrec symrec;

	ast = AO_STDATA(stf->stf_symtab);

	got_match = FALSE;

	for (symno = next_unmarked_sym(symio, min);
	     symno <= max;
	     symno = next_unmarked_sym(symio, symno + 1)) {
		nlist_t nm;
		const char *s, *base, *pos, *end;
		char *s2;
		int len;

		getsym(symio, symno, &nm);

		switch (nm.n_type) {
		case N_GSYM:
		case N_LCSYM:
		case N_LSYM:
		case N_PSYM:
#ifdef N_ROSYM
		case N_ROSYM:
#endif
#ifdef N_ISYM					/* RCB, for SC5 */
		case N_ISYM:
		case N_ESYM:
#endif
		case N_RSYM:
		case N_SSYM:
		case N_STSYM:
		case N_FUN:
			break;

		default:
			mark_sym(symio, symno);
			continue;
		}
		
		base = s = symstring(symio, symno);
		/* RGA '!' check for SC4 */
		if (base == NULL || *base == '!' || *base == '<' ||
		    *base == '>' || *base == '=') 
			continue;

		s2 = 0;
		symrec.symio = symio;
		symrec.symno = symno;
		/* RGA SC4.2 folds lines at 400. */
		if ((pos = strchr(s, '=')) != NULL)
		{
		  len = strlen(s);
		  end = s + len - 1;
		  if (*end == '\\')
		  {
		    s2 = e_malloc(len + 1);
		    memcpy(s2, s, len + 1);
		    base = s = s2;
		  }
		  while (*end == '\\')
		  {
		    char *s1;
		    int new_len;
		    
		    s1 = (char *)get_cont_symstring(&symrec);
		    mark_sym(symio, symrec.symno);
		    new_len = strlen(s1);
		    s2 = e_realloc(s2, len + new_len);
		    base = s = s2;
		    memcpy(s2 + len - 1, s1, new_len);
		    s2[len + new_len - 1] = 0;
		    len += new_len - 1;
		    end = s + len - 1;
		    }
		}

		while ((pos = strchr(s, '=')) != NULL) {
			int fnum, tnum=-1;
			const char *matchpos;
			Ftype *ft;
			bool is_undef_tag;

			/*  Definitions of the form "=x" are unresolved tag
			 *  definitions.
			 */
			is_undef_tag = pos[1] == 'x';
			
			if (pos[-1] == ')') {
				while (pos >= base && *pos != '(')
					--pos;
				matchpos = pos;

				pos++;
				Typenum(stf, sr, TRUE, &pos, &fnum, &tnum);
				scheck(sr, &pos, '=');
			}
			else {
				while (pos >= base && isdigit(pos[-1]))
					--pos;
				matchpos = pos;

				if ( isdigit(pos[0]))
				{
				    fnum = stf->stf_fnum;
				    tnum = parse_num(stf, sr, &pos);
				}
			}
			
			if ( tnum >= 0) 
			    ft = note_def_pos(stf->stf_fmap[fnum]->hf_stf,
					  tnum, symno, matchpos - base,
					  is_undef_tag);
			
			if (tnum == match_tnum && fnum == stf->stf_fnum) {
				/*  Found the type.  We don't return
				 *  immediately because we want to finish
				 *  calling note_def_pos for all the
				 *  type definitions in the string.
				 */
				*p_ft = ft;

				/*  Keep on looking if all we have found
				 *  is an undef tag.  We will normally find
				 *  a real entry later on.
				 */
				got_match = !is_undef_tag;
			}
			s = pos + 1;
		}

		if (s2)
		  free(s2);
		mark_sym(symio, symno);

		if (got_match)
			return TRUE;
	}

	if (*p_ft != NULL)
		return TRUE;

	/* TODO: complain */
	return FALSE;
}

static Ftype *
note_def_pos(stf, tnum, symno, offset, is_undef_tag)
stf_t *stf;
int tnum, symno, offset;
bool is_undef_tag;
{
	Ftype *ft, *prevft;
	bool already_noted;

	lookup_ftype(stf, tnum, &ft, &prevft);

	already_noted = ft != NULL;

	if (!already_noted)
		ft = add_ftype_to_list(stf, tnum, FALSE, prevft);

	if (!already_noted ||
	    (!ft->have_type && !is_undef_tag &&
	     ft->u.defpos.maybe_better_def_exists)) {
		ft->u.defpos.maybe_better_def_exists = is_undef_tag;
		ft->u.defpos.is_undef_tag = is_undef_tag;
		ft->u.defpos.symno = symno;
		ft->u.defpos.offset = offset;
	}

	return ft;
}

static Ftype *
add_ftype_to_list(stf, tnum, have_type, prevft)
stf_t *stf;
int tnum;
bool have_type;
Ftype *prevft;
{
	Ftype *ft;

	ft = (Ftype *)alloc(stf->stf_symtab->st_apool, sizeof(Ftype));

	ft->tnum = tnum;
	ft->have_type = FALSE;

	if (prevft == NULL) {
		ft->next = stf->stf_ftypes;
		stf->stf_ftypes = ft;
	}
	else {
		if (prevft->tnum <= tnum)
			panic("prev tnum order botch in aftl");
		
		ft->next = prevft->next;
		prevft->next = ft;
	}

	if (ft->next != NULL && ft->next->tnum >= tnum)
		panic("next tnum order botch in aftl");

	return ft;
}

static type_t *
get_type(stf, sr, symio, ft, tnum, have_undef_tag, min, max)
stf_t *stf;
Symrec *sr;
Ftype *ft;
symio_t *symio;
int tnum;
bool have_undef_tag;
int min, max;
{
	Symrec symrec;
	const char *s;
	char *end, *s2;
	type_t *type_id;
	size_t len;

	if (ft != NULL) {
		if (ft->have_type)
			panic("have_type botch in gt");

	}
	else {
		if (!get_def(stf, sr, symio, tnum, min, max, &ft))
			return NULL;
	}

	if (ft->u.defpos.maybe_better_def_exists) {
		get_def(stf, sr, symio, tnum, min, max, &ft);

		/*  Whether we found anything or not, we now know
		 *  there's nothing better.
		 */
		ft->u.defpos.maybe_better_def_exists = FALSE;
	}

	if (ft->u.defpos.is_undef_tag && have_undef_tag) {
		
		/*  The best definition of this type is an xsfoo style entry,
		 *  and the instance of TypeId() that called us is already
		 *  looking at one of those (possibly this very entry).
		 *  Thus we don't want to call TypeId() again - we might
		 *  recurse for ever.  Just return NULL and have TypeId()
		 *  construct an undefined type entry.
		 */
		return NULL;
	}

	symrec.symio = symio;
	symrec.symno = ft->u.defpos.symno;
	s = symstring(symio, symrec.symno);
	s2 = 0;
	len = strlen(s);
	if (ft->u.defpos.offset >= len) /* RCB: changed > to >= */
	{
	  end = (char *)s + len - 1;
	  /* RGA SC4.2 folds lines at 400.  */
	  if (*end == '\\')
	  {
	    s2 = e_malloc(len + 1);
	    memcpy((void *)s2, s, len + 1);
	    s = s2;
	  }
	  while (*end == '\\')
	  {
	    char *s1;
	    size_t new_len;
	    
	    s1 = (char *)get_cont_symstring(&symrec);
	    mark_sym(symio, symrec.symno);
	    new_len = strlen(s1);
	    s2 = (char *)e_realloc((void *)s2, len + new_len);
	    s = s2;
	    memcpy((void *)(s2 + len - 1), s1, new_len);
	    s2[len + new_len - 1] = 0;
	    len += new_len - 1;
	    end = (char *)s + len - 1;
	  }
	}
	s += ft->u.defpos.offset;

	type_id = TypeId(stf, &symrec, &s, TRUE, *(s - 1));
	if (s2)
	  free(s2);
	return type_id;
}

static void
lookup_ftype(stf, tnum, p_ft, p_prevft)
stf_t *stf;
int tnum;
Ftype **p_ft, **p_prevft;
{
	Ftype *ft, *prevft;

	prevft = NULL;

	for (ft = stf->stf_ftypes; ft != NULL; ft = ft->next) {
		if (ft->tnum == tnum)
			break;

		if (ft->tnum < tnum) {
			ft = NULL;
			break;
		}
		
		prevft = ft;
	}

	*p_ft = ft;

	if (p_prevft != NULL)
		*p_prevft = prevft;
}

static void
patch_ftypes(stf)
stf_t *stf;
{
   if ( stf->stf_flags & STF_NEED_PATCH)
   {
	Ftype *ft;
	/*
	**   Search for any types marked "same as" during the parse
	**   and copy the true definition.
	*/
	for (ft = stf->stf_ftypes; ft != NULL; ft = ft->next) {
		if ( ft->have_type && ft->u.type.ty_code == PI_SAME_AS)
		{
			type_t* type = ft->u.type.ty_base;
			while ( type->ty_code == PI_SAME_AS)
				type = type->ty_base;
			ft->u.type = *type;
		}
	}
	stf->stf_flags &= ~STF_NEED_PATCH;
   }
}

void wrapup_types(stf)
stf_t *stf;
{
	int i;
	for ( i = 0; i < stf->stf_mapsize; i++)
	    patch_ftypes(stf->stf_fmap[i]->hf_stf);
}

static type_t *
tnum_to_type(stf, sr, symio, tnum, have_undef_tag)
stf_t *stf;
Symrec *sr;
symio_t *symio;
int tnum;
bool have_undef_tag;
{
	Ftype *ft;
	type_t *type;

	lookup_ftype(stf, tnum, &ft, (Ftype **)NULL);

	if (ft != NULL && ft->have_type) {
		type = &ft->u.type;
	}
	else {
		stf_t *search_stf;
#ifdef AO_ELF
		search_stf = stf->stf_fmap[0]->hf_stf;
#else
		/* RCB: gcc version 2.8 or later under sun 4 */
		if (stf->stf_compiler_type == CT_GNU_CC)
		    search_stf = stf->stf_fmap[0]->hf_stf;
		else
		    search_stf = stf;
#endif
		type = get_type(stf, sr, symio, ft, tnum, have_undef_tag,
				stf->stf_symno, search_stf->stf_symlim - 1);
	}

	return type;
}

static Ftype *
insert_ftype(stf, tnum)
stf_t *stf;
int tnum;
{
	Ftype *prevft, *ft;

	lookup_ftype(stf, tnum, &ft, &prevft);

	if (ft != NULL) {
		if (ft->have_type)
		{
/*			panic("type already exists in insert_type");*/
		  return ft;
		}
	}
	else {
		ft = add_ftype_to_list(stf, tnum, TRUE, prevft);
	}

	ci_init_type(&ft->u.type, TY_NOTYPE);
	ft->have_type = TRUE;

	return ft;
}

static type_t *
lookup_tnum(stf, tnum)
stf_t *stf;
int tnum;
{
	Ftype *ft;

	lookup_ftype(stf, tnum, &ft, (Ftype **)NULL);

	return (ft != NULL && ft->have_type) ? &ft->u.type : NULL;
}

/*  Class:
 *	'c' = Constant ';'	{ NI }
 *	Variable
 *	Procedure		{ PI }
 *	Parameter
 *	NamedType
 *	'X' ExportInfo		{ NI }
 *
 *   All the ones that we implement resolve to a type letter giving the
 *   class of the variable followed by a TypeId.
 */
type_t *
Class(stf, sr, p_s, p_class)
stf_t *stf;
Symrec *sr;
const char **p_s;
class_t *p_class;
{
	const char *s;
	char prior_chr;
	class_t class;
	type_t *rtype = NULL;

	s = *p_s;
	switch(*s) {

	/* 'c' '=' Constant */
	case 'c':
		notimp("Constants");
		class = CL_NOCLASS;	/* to satisfy gcc */
		break;

	/* Variable */
	case '(':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		class = CL_AUTO;
		--s;
		break;
	case 'a':
		/*  The `a' is part of the array spec rather than a class
		 *  letter (epcf90 is the only compiler I have seen do this).
		 */
		class = CL_AUTO;

		/* RGA gcc emits these, but don't advance s */
		if (stf->stf_compiler_type != CT_GNU_CC)
		  --s;

		break;
	case 'r':
		class = CL_REG;
		break;
	case 'S':
		class = CL_STAT;
		break;
	case 'V':
		class = CL_LSTAT;
		break;
	case 'G':
		class = CL_EXT;
		break;

	/*  Procedure
	 *
	 *  DEV: 'P' should be in this group according to the grammar
	 *       see the comment later in this function.
	 */
	case 'Q':
	case 'I':
		notimp("Q/I functions");
		class = CL_NOCLASS;	/* to satisfy gcc */
		break;
	case 'F':
		class = CL_FUNC;
		break;
	case 'f':
		class = CL_LFUNC;
		break;
	case 'J':
		notimp("Internal functions");
		class = CL_NOCLASS;	/* to satisfy gcc */
		break;

	/* Parameter
	 *
	 *  DEV: the grammar says 'P' introduces a global procedure,
	 *       but gcc uses it for parameters, so we just silently
	 *       take it to mean a parameter.
	 */
	case 'P':
	case 'p':
		class = CL_ARG;
		break;
	case 'v':
		class = CL_REF;
		break;

	/* Namedtype */
	case 't':
		class = CL_TYPEDEF;
		break;
	case 'T':
		/*  Gcc now emits 'Tt' for structure definitions with
		 *  no tag of the form `typedef struct { ... } foo_t'.
		 */
		if (s[1] == 't') {
		        bump_str(sr, &s);
			class = CL_TYPEDEF;
			break;
		}

		class = CL_TAGNAME;
		break;

	/* 'X' ExportInfo */
	case 'X':
		/*  This has been seen in a SunOS 4.0 f77 a.out file.
		 *  I don't think we need to worry about it, so just
		 *  ignore the symbol (setting class to NOCLASS
		 *  indicates to our caller that this symbol is junk).
		 */
		class = CL_NOCLASS; 
		break;

	case 'Y':		/* RGA added for SC4.2 */
		class = CL_NOCLASS;
		rtype = NULL;
		break;

	default:
		errf("\bunknown type class letter in symbol table");
		class = CL_NOCLASS; /* to satisfy gcc */
	}

	prior_chr = *s == '\\' ? *(s-1) : *s;
	bump_str(sr, &s);
	if (prior_chr != 'Y')
		rtype = TypeId(stf, sr, &s, TRUE, prior_chr);
	if (rtype == NULL)
		return rtype;

	/*  (IDE) Was this to fix a bug ?
	 *  Modern compilers support passing struct/union by value.
	 *  Still works on Solaris 2 so keep for OS_SUNOS.
	 */
#ifdef OS_SUNOS
	if ((class == CL_ARG) && (rtype->ty_code == TY_STRUCT || rtype->ty_code == TY_UNION))
		class = CL_REF; 
#endif
	*p_s = s;

	/*  The basic types (int, char etc) get entries in the symbol
	 *  table that look like user typedefs.  We don't want to
	 *  clutter up the typedef lists with these entries.
	 */
	if ((class == CL_TYPEDEF) && rtype && (rtype->ty_typedef == NULL))
		*p_class = CL_NOCLASS;
	else
		*p_class = class;
	return rtype;
}

/*  Parse a number (INTEGER in the grammar).  No sign allowed.
 */
int
parse_num(stf, sr, p_s)
stf_t *stf;
Symrec *sr;
const char **p_s;
{
	int res;

	if (!parse_number(stf, sr, p_s, &res))
		panic("bad number in parse_num");

	return res;
}

/*  Parse a possibly signed number.
 */
static int
parse_signed_num(stf, sr, p_s)
stf_t *stf;
Symrec *sr;
const char **p_s;
{
	const char *s;
	int num;
	bool neg;

	s = *p_s;

	neg = *s == '-';
	if (neg)
	        bump_str(sr, &s);

	num = parse_num(stf, sr, &s);

	*p_s = s;
	return neg ? -num : num;
}

/* Typenum:
 *	'(' INTEGER ':' INTEGER ')'	{ DEV, Sun C symbol tables }
 *	INTEGER				{ f77 and all VAX symbol tables }
 */
void
Typenum(stf, sr, assume_paren, p_s, p_fnum, p_tnum)
stf_t *stf;
Symrec *sr;
bool assume_paren;
const char **p_s;
int *p_fnum, *p_tnum;
{
	if (!parse_typenum(stf, sr, assume_paren, p_s, p_fnum, p_tnum))
		panic("bad typenum in symbol table");
}

/*  Special values returned by parse_subrange_signed_num() - they just
 *  have to be values that do not otherwise appear in the get_basic_type()
 *  table.
 */
enum {
	SR_MINUS_2E63 = 0xacafe,	/* 01000000000000000000000 */
	SR_2E63_MINUS_1 = 0xaface,	/*  0777777777777777777777 */
	SR_2E64_MINUS_1 = 0xaceace	/* 01777777777777777777777 */
};


static void
get_basic_type(stf, language, st, sr, dim, type)
stf_t *stf;
language_t language;
symtab_t *st;
const Symrec *sr;
dim_t *dim;
type_t *type;
{
	typedef struct {
		long ct_low;
		long ct_high;
		typecode_t ct_code;
		int ct_size;
	} c_typetab_t;
	typedef struct {
		const char *ft_name;
		typecode_t ft_code;
		int ft_size;
	} f77_typetab_t;
		
	static c_typetab_t c_typetab[] = {
		{ 0x80000000,	0x7fffffff,	TY_INT,		sizeof(long)  },
		{ -128,		127,		TY_CHAR,	sizeof(char)  },
		{ 0,		127,		TY_CHAR,	sizeof(char)  },
		{ -32768,	32767,		TY_SHORT,	sizeof(short) },
		{ 0,		255,		TY_UCHAR,	sizeof(char)  },
		{ 0,		65535,		TY_USHORT,	sizeof(short) },
		{ 0,		-1,		TY_UINT,	sizeof(long)  },
#if HAVE_LONG_LONG
		{ SR_MINUS_2E63,SR_2E63_MINUS_1,TY_LONGLONG,	sizeof(long long)},
		{ 0,		SR_2E64_MINUS_1,TY_ULONGLONG,	sizeof(long long)},
#else
		{ SR_MINUS_2E63,SR_2E63_MINUS_1,TY_LONGLONG,	2*sizeof(long)},
		{ 0,		SR_2E64_MINUS_1,TY_ULONGLONG,	2*sizeof(long)},
#endif
		{ 4,		0,		TY_FLOAT,	sizeof(float) },
		{ 8,		0,		TY_DOUBLE,	sizeof(double)},
#if HAVE_LONG_DOUBLE
 		{ 12,		0,		TY_LONGDOUBLE,	sizeof(long double)},
 		{ 16,		0,		TY_LONGDOUBLE,	sizeof(long double)},
#endif
 		{ 0,		1,		TY_BOOLEAN,	sizeof(long)},
	};
#define N_C_TYPES	(sizeof c_typetab / sizeof *c_typetab)
	static f77_typetab_t f77_typetab[] = {
		{ "integer*2",		TY_SHORT,	sizeof(short)	   },
		{ "integer*4",		TY_INT,		sizeof(int)	   },
		{ "real",		TY_FLOAT,	sizeof(float)	   },
		{ "double precision",	TY_DOUBLE,	sizeof(double)	   },
		{ "complex",		TY_COMPLEX,	2 * sizeof(float)  },
		{ "double complex",	TY_DBLCOMP,	2 * sizeof(double) },
		{ "logical",		TY_LOGICAL,	sizeof(int)	   },
		{ "logical*1",		TY_LOGICAL,	sizeof(char)	   },
		{ "logical*2",		TY_LOGICAL,	sizeof(short)	   },
		{ "logical*4",		TY_LOGICAL,	sizeof(int)	   },
		{ "char",		TY_CHARACTER,	sizeof(char)	   },
		{ "void",		TY_FVOID,	0	   	   },
	};
#define N_F77_TYPES	(sizeof f77_typetab / sizeof *f77_typetab)

	c_typetab_t *ct;
	const char *line;
	size_t len;

	get_typename_str(sr, &line, &len);

	type->ty_base = NULL;
	type->ty_typedef = NULL;

	if (IS_FORTRAN(language)) {
		f77_typetab_t *ft;

		for (ft = f77_typetab; ft < f77_typetab + N_F77_TYPES; ++ft) {
			if (strncmp(ft->ft_name, line, len) == 0 &&
						ft->ft_name[len] == '\0') {
				const char *name;
				
				type->ty_code = ft->ft_code;
				type->ty_size = ft->ft_size;

				switch (ft->ft_code) {
				case TY_CHARACTER: name = "character"; break;
				case TY_INT:	   name = "integer";   break;
				default:	   name = ft->ft_name; break;
				}
				
				type->ty_name = name;
				return;
			}
		}
		if (stf->stf_compiler_type != CT_GNU_CC)
			panic("unknown f77 basic type line in symtab");
	}

	if ((language != LANG_C) && (language != LANG_CC)
			&& (stf->stf_compiler_type != CT_GNU_CC))
		panic("unknown lang in gbt");

	type->ty_name = make_typename(st->st_apool, line, len);

	if (dim == NULL) {
		if (!((strcmp(type->ty_name, "void") == 0)
		   || (strcmp(type->ty_name, "VOID") == 0)/* RGA for clcc */
		   || (strcmp(type->ty_name, "va") == 0))) /* RGA for clcc */
/*			panic("unknown special type in gbt");*/
		type->ty_code = TY_VOID;
		type->ty_size = 0;
		return;
	}

	for (ct = c_typetab; ct < c_typetab + N_C_TYPES; ++ct) {
		if (dim->di_low == ct->ct_low && dim->di_high == ct->ct_high) {
			type->ty_code = ct->ct_code;
			type->ty_size = ct->ct_size;
			return;
		}
	}

/*	panic("unknown size range");*/
	type->ty_code = TY_VOID;
	type->ty_size = 0;
	errf("\bunknown size range");
}

/*  Return a safe pointer to an "undefined type"
*/
static type_t*
get_safe_type_ptr()
{
    static type_t save_type;
    if ( save_type.ty_size == 0 )
    {
	/* First time, set up the safe type object incase anybody looks at it */
	save_type.ty_code = TY_INT_ASSUMED;
	save_type.ty_size = -1;
    }
    return &save_type;
}

/*  TypeId:
 *	Typenum
 *	Typenum '=' TypeDef
 *	Typenum '=' TypeAttrs TypeDef 	{ NI }
 *
 */
type_t *
TypeId(stf, sr, p_s, eval, prior_chr)
stf_t *stf;
Symrec *sr;
const char **p_s;
bool eval;
int prior_chr;
{
    stf_t *nstf;
    typecode_t utypecode;
    int fnum, tnum, save_symno;
    bool have_typenum, is_named_type, is_typedefed;
    Compiler_type compiler;
    const char *s, *tag;
    alloc_pool_t *ap;
    type_t *rtype;
    bool is_fortran;

    s = *p_s;
    ap = stf->stf_symtab->st_apool;

/*	is_named_type = s[-1] == 'T';*/
/*	is_typedefed = s[-1] == 't';*/
    is_named_type = prior_chr == 'T';
    is_typedefed = prior_chr == 't';
    have_typenum = isdigit(*s) || *s == '(';
    compiler = stf->stf_compiler_type;
    is_fortran = IS_FORTRAN(stf->stf_language);

    if (have_typenum) {
	Typenum(stf, sr, FALSE, &s, &fnum, &tnum);
	if (fnum >= stf->stf_mapsize)
	{
	    /* RGA for yacc garbage */
	    errf ("fnum %d out of range %d in TypeId", fnum, stf->stf_mapsize);
	    rtype = NULL;
	    nstf = stf;
	    tnum = -1;
	    utypecode = TY_NOTYPE;
	    tag = 0;
	    *p_s = s;
	    return rtype;
	}
	nstf = stf->stf_fmap[fnum]->hf_stf;

	if (*s == '=' && s[1] == 'x' && char_to_utypecode(s[2], &utypecode)) {
	    bump_str(sr, &s);
	    bump_str(sr, &s);
	    bump_str(sr, &s);
	    tag = parse_name(sr, &s, ap, 0, compiler, is_fortran);
	    if (stf->stf_compiler_type == CT_GNU_CC)
	        field_scheck(sr, &s, ':');
	    else
	        scheck(sr, &s, ':');
	}
	else {
	    utypecode = TY_INT_ASSUMED;
	    tag = NULL;
	}
	rtype = lookup_tnum(nstf, tnum);
    }
    else {
	rtype = NULL;
	nstf = stf;
	tnum = -1;
	utypecode = TY_NOTYPE;		/* to satisfy gcc */
	tag = 0;			/* to satisfy gcc */
    }

    if (is_named_type && rtype != NULL)
	return rtype;
    save_symno = sr->symno;

    if (have_typenum) {
	if (*s == '=') {
	    bump_str(sr, &s);
	    
	    if (*s == '@' || *s == '#')
	        bump_str(sr, &s);

	    if (rtype == NULL)
		rtype = TypeDef(nstf, sr, &s, tnum, eval);
	    else if (!is_typedefed)
		(void) TypeDef(nstf, sr, &s, tnum, FALSE);
	}
	else if (eval || !rtype)  /*RGA*/ {
	    rtype = tnum_to_type(nstf, sr, sr->symio, tnum, tag != NULL);
		
	    if (rtype == NULL) {
		Ftype *ft;

		ft = insert_ftype(nstf, tnum);
		rtype = &ft->u.type;
		make_undef_type(nstf, tag, utypecode, rtype);
	    }
	}
    }
    else {
	rtype = TypeDef(nstf, sr, &s, -1, eval);
    }

    *p_s = s;

    if (!eval)
	return NULL;

    while ( rtype->ty_code == PI_SAME_AS)
	rtype = rtype->ty_base;

    if (rtype->ty_typedef == &Is_basic_type) {
	is_typedefed = FALSE;
	rtype->ty_typedef = NULL;
    }

    if (is_named_type || is_typedefed) {
	const char *name;

	name = symstring(sr->symio, save_symno);

	if (is_typedefed) {
	  if (stf->stf_language == LANG_CC &&
	      (name[0] != ':' && name[0] != '?' &&
	       !isalpha(name[0]) && name[0] != '_' &&
	       name[0] != '$' && name[0] != '.'))
	  {
	      /* RGA added return of dummy type for C++ */
	      rtype = NULL;
	      nstf = stf;
	      tnum = -1;
	      utypecode = TY_NOTYPE;	
	      tag = 0;
	      *p_s = s;
	      return rtype;
	  }
	  else
		ci_make_typedef (ap,
			 parse_name (sr, &name, ap, 0, compiler, is_fortran),
			 rtype); /* RGA */
	}
	else {
	    switch(rtype->ty_code) {
	    case TY_U_STRUCT:
	    case TY_U_UNION:
	    case TY_U_ENUM:
	    case TY_STRUCT:
	    case TY_UNION:
	    case TY_ENUM:
		break;
	    default:
		/* RGA added return of dummy type for C++ */
		rtype = NULL;
		nstf = stf;
		tnum = -1;
		utypecode = TY_NOTYPE;	
		tag = 0;
		*p_s = s;
		return rtype;
	    } /* switch */

	    tag = parse_name(sr, &name, ap, 0, compiler, is_fortran); /* RGA */
	    if (*tag == '\0')
		tag = NULL;
		
	    rtype->ty_aggr_or_enum->ae_tag = tag;
	}
    }

    return rtype;
}

static void
make_undef_type(stf, tag, utypecode, type)
stf_t *stf;
const char *tag;
typecode_t utypecode;
type_t *type;
{
	type = ci_make_undef_type(stf->stf_symtab->st_apool,
				  tag, utypecode, type);
				
	if (utypecode == TY_U_STRUCT || utypecode == TY_U_UNION ||
	    utypecode == TY_U_ENUM) {
		aggr_or_enum_def_t *ae;
		block_t *bl;

		/*  BUG: struct might not be in
		 *       outermost block.
		 */
		ae = type->ty_aggr_or_enum;
		bl = stf->stf_fil->fi_block;

		ae->ae_next = bl->bl_aggr_or_enum_defs;
		bl->bl_aggr_or_enum_defs = ae;
	}
}

/*  See SDI page 79.
 */
static void
get_basic_integer(stf, ap, sr, language, p_s, type)
stf_t *stf;
alloc_pool_t *ap;
const Symrec *sr;
language_t language;
const char **p_s;
type_t *type;
{
	const char *s;
	bool is_signed;
	typecode_t typecode = TY_NOTYPE;
	int width, offset, nbits;
	const char *typename;
	size_t len;

	s = *p_s;

	is_signed = *s == 's';

	if (!is_signed && *s != 'u')
		panic("unknown value for signed flag in gbi");
	bump_str((Symrec*)sr, &s);

	/*  Handle the Display (char/bool) flag.
	 */
	if (*s == 'c' || *s == 'b' || *s == 'v') /* RGA added 'v' */
	{
		if ( *s == 'b')         /* RCB */
		    typecode = TY_BOOLEAN;
	        bump_str((Symrec*)sr, &s);
	}

	width = parse_num(stf, (Symrec*)sr, &s);
	scheck((Symrec*)sr, &s, ';');

	offset = parse_num(stf, (Symrec*)sr, &s);
	scheck((Symrec*)sr, &s, ';');

	nbits = parse_num(stf, (Symrec*)sr, &s);

	if (nbits != width * 8 || offset != 0)
		panic("width/nbits mismatch or offset!=0 in gbi");

	/*  Can't use a switch here because of problems with duplicate
	 *  cases when two types are the same size.
	 */
	if ( typecode == TY_BOOLEAN)   /* RCB */
	{
	    ci_set_sizeof_bool(width);
	}
	else if (width == sizeof(char)) {
		if (IS_FORTRAN(language))
			typecode = TY_CHARACTER;
		else
			typecode = is_signed ? TY_CHAR : TY_UCHAR;
	}
	else if (width == sizeof(short)) {
		typecode = is_signed ? TY_SHORT : TY_USHORT;
	}
	else if (width == sizeof(int)) {
		typecode = is_signed ? TY_INT : TY_UINT;
	}
	else if (width == sizeof(long)) {
		typecode = is_signed ? TY_LONG : TY_ULONG;
	}
	else if (width == 2 * sizeof(long)) {
		typecode = is_signed ? TY_LONGLONG : TY_ULONGLONG;
	}
	else if (width == 0) {
		typecode = TY_VOID;
	}
	else {
		panic("unknown width in gbi");
		typecode = TY_NOTYPE; /* to satisfy gcc */
	}



	get_typename_str(sr, &typename, &len);
	type->ty_name = make_typename(ap, typename, len);

	if (typecode == TY_INT && IS_FORTRAN(language) &&
	    strcmp(type->ty_name, "logical") == 0) {
		typecode = TY_LOGICAL;
	}

	type->ty_size = width;
	type->ty_code = typecode;

	*p_s = s;
}

/*  See SDI page 84.
 */
static void
get_basic_float(stf, ap, sr, p_s, type)
stf_t *stf;
alloc_pool_t *ap;
const Symrec *sr;
const char **p_s;
type_t *type;
{
	static typecode_t fptypes[] = {
		TY_NOTYPE,
		TY_FLOAT,	/* 1, NF_SINGLE */
		TY_DOUBLE,	/* 2, NF_DOUBLE */
		TY_COMPLEX,	/* 3, NF_COMPLEX */
		TY_DBLCOMP,	/* 4, NF_COMPLEX16 */
		TY_NOTYPE,	/* 5, NF_COMPLEX32 */
		TY_LONGDOUBLE,	/* 5, NF_LDOUBLE */
	};
	enum { NFPTYPES = sizeof fptypes / sizeof *fptypes };
	const char *s, *typename;
	size_t len;
	int format, nbytes;

	s = *p_s;

	format = parse_num(stf, (Symrec*)sr, &s);
	scheck((Symrec*)sr, &s, ';');
	nbytes = parse_num(stf, (Symrec*)sr, &s);

	type->ty_code = (format >= NFPTYPES) ? TY_NOTYPE : fptypes[format];

	if (type->ty_code == TY_NOTYPE)
		panic("unknown fp type in gbf");

	type->ty_size = nbytes;

	get_typename_str(sr, &typename, &len);
	type->ty_name = make_typename(ap, typename, len);

	*p_s = s;
}

static void
get_typename_str(sr, p_line, p_len)
const Symrec *sr;
const char **p_line;
size_t *p_len;
{
	const char *cptr;

	if ((*p_line = symstring(sr->symio, sr->symno)) == NULL)
		panic("NULL line in gbt");

	if ((cptr = strchr(*p_line, ':')) == NULL)
		panic("missing ':' in gbt");

	*p_len = cptr - *p_line;
}

static const char *
make_typename(ap, line, len)
alloc_pool_t *ap;
const char *line;
size_t len;
{
	typedef struct {
		const char *name;
		const char *alias;
	} alias_t;
	static alias_t atab[] = {
		{ "long int",			"long"			},
		{ "unsigned int",		"unsigned"		},
		{ "long unsigned int",		"unsigned long"		},
		{ "short int",			"short"			},
		{ "short unsigned int",		"unsigned short"	},
		{ "long long int",		"long long"		},
		{ "long long unsigned int",	"unsigned long long"	},
		{ "logical*4",			"logical"		},
		{ "real*4",			"real"			},
		{ "integer*4",			"integer"		},
	};
	alias_t *al;
	char *name;

	/*  Some compilers put rather cumbersome names for some C types in
	 *  the symbol table.  If the name is one of these, rewrite it in
	 *  the more usual C idiom, otherwise just take a copy of the name.
	 */
	for (al = atab; al < &atab[sizeof atab / sizeof *atab]; ++al) {
		if (strncmp(al->name, line, len) == 0 && al->name[len] == '\0')
			return al->alias;
	}

	name = alloc(ap, len + 1);
	strncpy(name, line, len);
	name[len] = '\0';

	return name;
}

/*  Typedef:
 *	TYPENUM				{ NI }
 *	Subrange
 *	Array				{ PI, DEV }
 *	Record
 *	'e' Enumlist
 *	'*' TypeId
 *	'S' Typeid			{ Set of TypeId (NI) }
 *	'd' Typeid			{ File of TypeId (NI) }
 *	ProcedureType			{ PI }
 *	'i' NAME ':' NAME ';'		{ NI }
 *	'o' NAME ';'			{ NI }
 *	'i' NAME ':' NAME ',' TypeId ';'{ NI }
 *	'o' NAME ',' TypeId		{ NI }
 */
static type_t *
TypeDef(stf, sr, p_s, tnum, eval)
stf_t *stf;
Symrec *sr;
const char **p_s;
int tnum, eval;
{
	Ftype *ft, dummy_ftype;
	aggr_or_enum_def_t *ae;
	dim_t *dim;
	typecode_t utypecode;
	int is_struct, junk, const_type = 0;
	const char *s, *s1, *tag;
	char prior_chr;
	alloc_pool_t *ap;
	Compiler_type compiler;
	bool is_fortran;

	s = *p_s;
	ae = NULL;
	ap = stf->stf_symtab->st_apool;
/* RGA */
	compiler = stf->stf_compiler_type;
	is_fortran = IS_FORTRAN(stf->stf_language);

	if (stf->stf_language == LANG_CC)
	{
	  if (*s == 'Y')
	    bump_str(sr, &s);
	  if ((*s == 'k' || *s == 'a') && isdigit(*(s+1)))
	    bump_str(sr, &s);
	}
/* RGA comment out this */
#ifdef AO_ELF
	/*  K and B are undocumented type letters which Sun cc 3.0 emits.
	 *
	 *  Note that we check for just the (f,n) form of typeids, and not
	 *  the single digit form emitted by gcc.  This is to avoid
	 *  looping on the `t19=19' that gcc emits for void.
	 */
/* RGA - this code causes infinite recursion for Centerline */
	if (!(stf->stf_compiler_type == CT_CLCC ||
	      stf->stf_compiler_type == CT_GNU_CC) &&
	    (*s == '(' || *s == 'k' || *s == 'B'))
	{
	  prior_chr = *(s - 1);
	  if (*s != '(')
	  {
	    prior_chr = *s == '\\' ? *(s-1) : *s;
	    bump_str(sr, p_s);
	  }
	  return TypeId(stf, sr, p_s, eval, prior_chr);
	}
#endif /* AO_ELF */

	if (tnum == -1) {
		ft = (Ftype *)alloc(ap, sizeof(Ftype));
		ft->tnum = -1;
		ft->u.type.ty_size = -1;
		ft->u.type.ty_typedef = NULL;
		ft->next = NULL;
	}
	else {
		ft = eval ? insert_ftype(stf, tnum) : &dummy_ftype ;
	}

	switch(*s) {

	/*  '*' TypeId */
	case '*':
	case '&':		/* RGA for g++ */
        	prior_chr = *s == '\\' ? *(s-1) : *s;
	        bump_str(sr, &s);
		ft->u.type.ty_code = DT_PTR_TO;
		ft->u.type.ty_qualifiers = 0;
		ft->u.type.ty_base = TypeId(stf, sr, &s, eval, prior_chr);
		break;

	/*  Typenum.  We assume this is a special case basic type (e.g. void).
	 */
	case '(':
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		/* RGA This is dubious. Have seen this for SC3, and */
		/* in "const char" for clcc, though without the 'k' */
		if (stf->stf_language == LANG_C || 
		    stf->stf_language == LANG_CC) 
		{
		  type_t *rtype = NULL;
		  const char *s_old = s;

		  if (*(s-1) == 'k')
		    const_type = 1;
		  rtype = TypeId(stf, sr, &s, eval, *(s - 1));
		  if (!rtype)
		  {
		    s = s_old;
		    Typenum(stf, sr, FALSE, &s, &junk, &junk);
		    get_basic_type(stf, stf->stf_language, stf->stf_symtab,
				   sr, (dim_t *)NULL, &ft->u.type);
		  }
		  else if (rtype == &ft->u.type)
		  {
		    if (ft->u.type.ty_code != TY_NOTYPE)
		       panic("non-void type defined as itself");
		    
		    ft->u.type.ty_code = TY_VOID;
		    ft->u.type.ty_size = 0;
		    ft->u.type.ty_name = make_typename(ap, "void", 4);
		  }
		  else
		  {
		    /*  RCB: This causes problems when *rtype itself is
		    **       not set up yet.  For example, if rtype is a
		    **	     pointer with ty_base still NULL, ty_base in
		    **	     the copy never gets set up.
		    **    ft->u.type = *rtype;
		    */
			ft->u.type.ty_code = PI_SAME_AS;
			ft->u.type.ty_base = rtype;
			stf->stf_flags |= STF_NEED_PATCH;
		    }

/*
 *		if (rtype && const_type)
 *		  {
 *		    char *const_name;
 *
 *		    const_name = e_malloc(strlen((ft->u.type).ty_name) + 7);
 *		    sprintf(const_name, "const %s", (ft->u.type).ty_name);
 *		    (ft->u.type).ty_name = const_name;
 *		  }
 */
		}
		else
		{
		  Typenum(stf, sr, FALSE, &s, &junk, &junk);
		  get_basic_type(stf, stf->stf_language, stf->stf_symtab,
				 sr, (dim_t *)NULL, &ft->u.type);
		}
		break;

	case 'b':
	        bump_str(sr, &s);
		get_basic_integer(stf, ap, sr, stf->stf_language, &s, &ft->u.type);
		break;
	case 'R':
	        bump_str(sr, &s);
		get_basic_float(stf, ap, sr, &s, &ft->u.type);
		break;

	/*  Subrange.  We assume this means a basic type.
	 */
	case 'r':
		dim = Subrange(stf, sr, &s, eval, FALSE);
		get_basic_type(stf, stf->stf_language, stf->stf_symtab,
			       sr, dim, &ft->u.type);

		/*  Signal to our caller (TypeId()) that this is not
		 *  a user typedef.
		 */
		ft->u.type.ty_typedef = &Is_basic_type;

		break;

	/*  Array.
	 *  The grammar gives < 'a' TypeId ';' TypeId >, but we currently
	 *  only cope with < 'a' Subrange ';' TypeId >.
	 */
	case 'a':
	        bump_str(sr, &s);
		ft->u.type.ty_code = DT_ARRAY_OF;
		ft->u.type.ty_dim = Subrange(stf, sr, &s, eval, TRUE);
		prior_chr = *s == '\\' ? *(s-1) : *s;
		scheck(sr, &s, ';');
		ft->u.type.ty_base = TypeId(stf, sr, &s, eval, prior_chr);
		break;

	/*  ProcedureType.
	 *  We only implement the < 'f' TypeId ';' > form.
	 *  Sun symbol tables seem to omit the ';'.
	 */
	case 'f':
		prior_chr = *s == '\\' ? *(s-1) : *s;
	        bump_str(sr, &s);
		ft->u.type.ty_code = DT_FUNC_RETURNING;
		ft->u.type.ty_funcret = ci_make_funcret(ap, FDT_IDLIST, 0,
							FALSE);
		ft->u.type.ty_base = TypeId(stf, sr, &s, eval, prior_chr);
		break;

	/*  Record.
	 */
	case 'Y':		/* RGA */
	        bump_str(sr, &s); /* drop thru */

	case 'c':		/* RGA C++ class*/
	case 'C':		/* RGA SC4.2 nested class*/
	case 's':
	case 'S':
	case 'u':
	case 'U':
		prior_chr = *s == '\\' ? *(s-1) : *s;
	        bump_str(sr, &s);
		if (!eval)
			errf("\bFound aggregate with eval FALSE");
		is_struct = (prior_chr == 'c' || prior_chr == 'C') ? 2 :
		  (prior_chr == 's' || prior_chr == 'S') ? 1 : 0;
		ft->u.type.ty_code = is_struct ? TY_STRUCT : TY_UNION;
		s1 = s; /* RCB*/
		ae = Record(stf, sr, &s, is_struct, eval, &ft->u.type);
		if (stf->stf_compiler_type == CT_GNU_CC &&
		    s1[-2]=='@' &&  /* RCB */
		    ae->ae_aggr_members == NULL)
		{
		  dim_t dim_dummy;

		  if ( *s != '-' )
		  {
		     *p_s = s;
		     return TypeDef(stf, sr, p_s, tnum, eval);
		  }

		  dim_dummy.di_low = 0;
		  dim_dummy.di_high = 1;
		  get_basic_type(stf, stf->stf_language, stf->stf_symtab,
				 sr, &dim_dummy, &ft->u.type);
		  
		  /*  Signal to our caller (TypeId()) that this is not
		   *  a user typedef.
		   */
		  ft->u.type.ty_typedef = &Is_basic_type;
		  /* RCB: size may be 1 or 4 */
		  ft->u.type.ty_size = parse_num(stf, sr, &s1)/8;
		  ci_set_sizeof_bool(ft->u.type.ty_size);

		}
		break;

	/*  'e' EnumList
	 */
				/* drop thru */
	case 'e':
	        bump_str(sr, &s);
		if (!eval)
			errf("\bFound enum with eval FALSE");
		ae = EnumList(stf, stf->stf_symtab, sr, &s, &ft->u.type);
		break;

	case 'x':
	        bump_str(sr, &s);
		if (!char_to_utypecode(bump_str(sr, &s), &utypecode))
		  panic("unknown undef typecode in TypeDef");
		tag = parse_name(sr, &s, ap, 0, compiler, is_fortran);
		if (stf->stf_compiler_type == CT_GNU_CC)
		  field_scheck(sr, &s, ':');
		else
		  scheck(sr, &s, ':');

		make_undef_type(stf, tag, utypecode, &ft->u.type);
		break;

	default:
	        bump_str(sr, &s);
		ft->u.type.ty_name = "UnKnOwN";
	  /*		panic("unknown character in TypeDef");*/
	}

	*p_s = s;
	return &ft->u.type;
}

/*  Enum:
 *	'e' EnumList ';'
 *
 *  EnumList:
 *	Enum
 *	EnumList Enum
 *
 *  Some machines omit the ';' so we don't insist on it.
 *
 *  TypeDef above has already skipped the 'e'.
 */
static aggr_or_enum_def_t *
EnumList(stf, st, sr, p_s, type)
stf_t *stf;
symtab_t *st;
Symrec *sr;
const char **p_s;
type_t *type;
{
	aggr_or_enum_def_t *ae;
	const char *s;

	s = *p_s;
	ae = ci_make_aggr_or_enum_def(st->st_apool, (const char *)NULL,
								TY_ENUM, type);
	ae->ae_is_complete = AE_COMPLETE;
	ae->ae_enum_members = NULL;
	for (;;) {
		if (*s == '\\')
			s = get_cont_symstring(sr);
		if (*s == ';' || *s == '\0')
			break;
		Enum(stf, st, &s, ae, sr);
	}
	if (*s != '\0')
		scheck(sr, &s, ';');
	*p_s = s;
	return ae;
}

/*  Enum:
 *	NAME ':' OrdValue ','
 *
 *  OrdValue:
 *	INTEGER
 */
static void
Enum(stf, st, p_s, ae, sr)
stf_t *stf;
symtab_t *st;
const char **p_s;
aggr_or_enum_def_t *ae;
Symrec *sr;
{
	const char *s, *name;
	int val;
	enum_member_t *em;
	Compiler_type compiler;

	s = *p_s;

	compiler = stf->stf_compiler_type;
	name = parse_name(sr, &s, st->st_apool, 0, compiler, FALSE);	/* RGA */
	if (*s == '\\')
	  s = get_cont_symstring(sr);
	field_scheck(sr, &s, ':');
	val = parse_signed_num(stf, sr, &s);
	if (*s == '\\')
	  s = get_cont_symstring(sr);
	scheck(sr, &s, ',');

	em = ci_make_enum_member(st->st_apool, name, val);
	em->em_enum = ae;

	em->em_next = ae->ae_enum_members;
	ae->ae_enum_members = em;

	*p_s = s;
}

/*  Record:
 *	's' ByteSize FieldList ';'
 *	'u' ByteSize FieldList ';'
 *
 *  TypeDef above has already skipped the 's' or 'u'.
 */
static aggr_or_enum_def_t *
Record(stf, sr, p_s, is_struct, eval, type)
stf_t *stf;
Symrec *sr;
const char **p_s;
int is_struct;
bool eval;
type_t *type;
{
	const char *s;
	aggr_or_enum_def_t *ae;
	typecode_t typecode;
	var_t *members, *last_members;
	Compiler_type compiler = CT_CC;
	bool terminate = FALSE;
	bool sun_pro =FALSE;

	if (stf->stf_compiler_type == CT_UNKNOWN)
	  compiler = ao_compiler(NULL, FALSE, CT_UNKNOWN);
	else
	  compiler = stf->stf_compiler_type;
	s = *p_s;
	typecode = is_struct ? TY_STRUCT : TY_UNION;
	ae = ci_make_aggr_or_enum_def(stf->stf_symtab->st_apool,
				      (const char *)NULL, typecode, type);
	ae->ae_is_complete = AE_COMPLETE;
	ae->ae_size = parse_num(stf, sr, &s);

	/* Get Base class info */
	if (is_struct == 2 || *(*p_s - 2) == 'Y') /* RGA SC3 class or struct */
	{
	  sun_pro = TRUE;
	  field_scheck(sr, &s, ';');
          members = SunProBaseClass(stf,sr,&s, eval, compiler);
	} else if ( *s == '!')
	{
	  bump_str(sr, &s);
          members = GnuBaseClass(stf,sr,&s, eval, compiler);
	} else
	{
	    members = NULL;
	}
	last_members = members;
	for (;;) {
		if (compiler == CT_GNU_CC && *s == '\\') {
			s = get_cont_symstring(sr);
		}

		/*  The test for '\0' is because epcf90 omits the terminating
		 *  semicolon on a structure definition.
		 */
		if (*s == ';' || *s == '\0' ||
		    (compiler == CT_GNU_CC && *s == '\\') || terminate == TRUE)
			break;
		/*  RGA prestest for strings that fail parse_name. Seen */
		/*  on SC3 C++ code */ 
		if (*s != ':' && *s != '?' && !isalpha(*s) && *s != '_' &&
		    *s != '$' && *s != '.' && *s != '!')
		  break;
		  
		members = Field(stf, sr, &s, is_struct, members,
				eval, compiler, &terminate);
		if (members)
		  last_members = members;
	}
	bump_str(sr, &s);
	if ( sun_pro)
	{
	    SunProStructMethods(stf, sr, &s);
	    last_members = SunProStaticMembers(stf, sr, &s, last_members, compiler);
	    SkipPastSemi(stf, sr, &s); /* Skip list of friends */
	    last_members = SunProVectorTable(stf, sr, &s, last_members, compiler);
	}
	ae->ae_aggr_members = last_members;

        if (stf->stf_language == LANG_F90)
                fix_f90_pointers(stf->stf_symtab->st_apool, ae);
        
	*p_s = s;
	return ae;
}

static void
fix_f90_pointers(ap, ae)
alloc_pool_t *ap;
aggr_or_enum_def_t *ae;
{
        taddr_t nextaddr;
        var_t *v;

        nextaddr = ae->ae_size;

        for (v = ae->ae_aggr_members; v != NULL; v = v->va_next) {
                size_t nbytes;
                
                nbytes = nextaddr - v->va_addr;
                        
                if (v->va_type->ty_code == TY_STRUCT &&
                    nbytes == sizeof(taddr_t) &&
                    v->va_type->ty_aggr_or_enum->ae_size > sizeof(taddr_t)) {
                        type_t *type;
                        
                        type = ci_make_type(ap, DT_PTR_TO);
                        type->ty_base = v->va_type;
                        v->va_type = type;
                }

                nextaddr = v->va_addr;
        }
}
/*
**   Special parse routines for G++
**     Added by Russ Browne (russ@amc.com)
*/
static var_t*
GnuBaseClass(stf,sr, p_s, eval, compiler)
stf_t *stf;
Symrec *sr;
const char **p_s;
bool eval;
Compiler_type compiler;
{
    /*
    **    Parse C++ base class as output by g++ -g
    **
    **    The input is of the form
    **
    **    ! Base_class_count, visibility_digit offset, type_num_of_base;
    **
    **	    the visibility and type repeats for each base class.
    **
    **     If there are virtual base classes, the offset for these
    **     is zero, and the base class information is followed by a 
    **     field of the form $vb<typenum>,typenum,offset
    **
    **     Ref: THE "STABS" DEBUG FORMAT
    **          Section GNU C++ Stabs
    */

    var_t *v;
    type_t *rtype;
    int virtual;
    int offset;

    var_t *next = NULL;
    const char* s = *p_s;
    int nbases = parse_num(stf, sr, &s);
    int nvirtual = 0;

    scheck(sr,&s, ',');
    while (nbases--)
    {
	virtual =  ( *s++ == '1');
	s++;	   /* don't care about public/private */
        offset = parse_num(stf, sr, &s);
	scheck(sr,&s, ',');

	rtype = TypeId(stf, sr, &s, eval, 0);
	if ( *s == '=' )
	{
	    abort();
	    while ( *s && *s != ';')
		s++;
	}
	scheck(sr,&s, ';');
	if ( !virtual)
	{
	    v = (var_t *) alloc(stf->stf_symtab->st_apool, sizeof(var_t));
	    v->va_addr = offset/8;
	    v->va_name = NULL;
	    v->va_language = stf->stf_language;
	    v->va_flags = VA_BASECLASS;
	    v->va_class = CL_MOS;
	    v->va_type = rtype?rtype:get_safe_type_ptr();
	    v->va_next = next;
	    ci_make_baseclass_name(v);
	    next = v;
	} else 
	    nvirtual++;
    }
    /*
    **  Now parse the virtual base information
    */
    while ( nvirtual-- && (*s == '$' || *s == '.') && !strncmp(s+1, "vb",2))
    {
	    field_scheck(sr, &s, ':'); /* Skip over name */
	    rtype = TypeId(stf, sr, &s, eval, 0);
	    scheck(sr,&s, ',');
            offset = parse_num(stf, sr, &s);

	    v = (var_t *) alloc(stf->stf_symtab->st_apool, sizeof(var_t));
	    v->va_addr = offset/8;
	    v->va_name = NULL;
	    v->va_language = stf->stf_language;
	    v->va_flags = VA_BASECLASS|VA_HIDE_PTR;
	    v->va_class = CL_MOS;
	    v->va_type = rtype?rtype:get_safe_type_ptr();
	    v->va_next = next;
	    ci_make_baseclass_name(v);
	    next = v;
            scheck(sr,&s, ';');
    }
    *p_s = s;
    return next;
}

static void
GnuStructMethods(stf,sr, p_s)
stf_t *stf;
Symrec *sr;
const char **p_s;
{
    /*
    **   The gnu compiler outputs a list of methods for
    **   the class.  We would need this to support calling
    **   a method from the inserted code.  For now,
    **   we only want to find the end of the data so the recursive parse
    **   of other classes can continue.
    **
    **   The format of the data is:
    **     
    **        <method_name::type_def=type_desc return_type(int);
    **  	{:arg_types;method_kind vtblinfo};>;"
    **	      The portion inside {} is repeated for each method of the same
    **	      name and a different arguement list.
    **	      The portion between <> is repeated for each method.
    **
    **    method_kind is indicated by three characters
    **        [0-2][A-D][.*?]
    **	      0  = private
    **	      1  = protected
    **	      2  = public
    **	      A  = normal
    **	      B  = const
    **	      C  = violatile
    **	      D  = const violatile
    **        .  = normal
    **        *  = virtual
    **        ?  = static
    **
    **  vtblinfo, if present, has the form
    **
    **        vtable_index;class_first_defining;
    **
    **
    **   Our algorithm is to look for the sequenct ;[0-2][A-D][.*?],
    **   skip to the next ".", '?', or '*'
    **   then to the next, or third next semicolon, and repeat
    **   until the next character is also a semicolon.
    **
    **    When called out of Field(), the initial method_name:: has
    **    already been parsed and *p_s points to the type_def.
    **
    **     Ref: THE "STABS" DEBUG FORMAT
    **          Section GNU C++ Stabs
    */
    const char* s = *p_s;
    do
    {
	/* Skip to the :' preceeding the arg_types */
	SkipToSplChar(stf, sr, &s, ":");
	if (s[0] == ':' && s[1] == ':')
	{
	   s+=2;
	   SkipToSplChar(stf, sr, &s, ":");
	}

	/* Look for ";[0-2][A-D][.*?]" */
	do SkipPastSemi(stf, sr, &s);
	while (!(    ((unsigned)(s[0]-'0')) <= 2
		&& ((unsigned)(s[1]-'A')) <= 3
		&& ((s[2] == '.') || (s[2] == '?') || (s[2] == '*'))));


	/* For a virtual method, skip past the vtbl index in type number of the
	** class first defining this method 
	*/
	if ( s[2] == '*')
	{
	    SkipPastSemi(stf, sr, &s);
	    SkipPastSemi(stf, sr, &s);
	}

	/* Go to the ':' that starts the next arguement list for a method
	** of the same name, or the';' that ends the methods of this name.
	*/
	SkipToSplChar(stf, sr, &s, ";:");
	if (*s == ';')
	{
	    bump_str(sr, &s);
	}

	/* Repeat until remaining char is also a ';' */
    }
    while (*s && *s != ';');

    /* The gnu type discription may end with ~%typeNum,
    ** giving the type number of the root type.  We want to
    ** skip that too.
    */
    if ( s[1] == '~'  && s[2] == '%')
    {
	s++;
	SkipToSplChar(stf, sr, &s, ";");
    }
    *p_s = s;
}

/*
**   Special parse routines for SunPro
**     Added by Russ Browne (russ@amc.com)
*/
static var_t*
SunProBaseClass(stf,sr, p_s, eval, compiler)
stf_t *stf;
Symrec *sr;
const char **p_s;
bool eval;
Compiler_type compiler;
{
    /* Parse base class info as output by the SC4 compiler
    **  Input for each base class is of the form
    **
    **       C<offset in bytes>(tnums)
    **
    **   For example
    **
    **     C0(0,24)C4(0,250);
    **
    **   An initial A, B, or C represents a normal base class
    **   An initial I, J, or K represents a virtual base class
    **
    **   Ref: SPARCworks debugger Interface, Chapter 6, Page 60
    **
    */
    var_t *next = NULL;
    var_t *v = NULL;
    type_t *rtype;
    const char* s = *p_s;
    bool sc5_vbase = FALSE;
    int virtual;
    int offset;
    while  (*s != ';')
    {
	if ( *s == 'C' || *s=='B' || *s=='A')
	   virtual = FALSE;
	else if ( *s == 'K' || *s == 'J' || *s == 'I')
	   virtual = TRUE;
	else
	{
	    field_scheck(sr, &s, ';');
	    break;
	}
	bump_str(sr, &s);
	offset = parse_num(stf, sr, &s);
	rtype = TypeId(stf, sr, &s, eval, 0);

	/* For the SC5 compiler, -offset is an index into the vtbl to
	** find the offset of the virtual base class.  Discard
	if (virtual &&  get_compiler_version() >= 5.0)
	    continue;
	** such base classes */

	v = (var_t *) alloc(stf->stf_symtab->st_apool, sizeof(var_t));
	v->va_addr = offset;
	v->va_name = NULL;
	v->va_language = stf->stf_language;
	v->va_flags = VA_BASECLASS;
	v->va_class = CL_MOS;
	v->va_type = rtype?rtype:get_safe_type_ptr();
	v->va_next = next;
	next = v;
	if ( virtual )
	{

	     /* If it is the SC5 compiler, the offset for the first
	     ** virtual base class will be 2, which is really offset
	     ** -8 into the vtbl.
	     ** For SC4, all offsets will be a multiple of 4. 
	     */
	     if ( offset&3)
		sc5_vbase = TRUE;
	     if (sc5_vbase)
	     {
		 v->va_flags |= VA_VTBLBASED;
		 v->va_addr = -4*offset;
	     }
	     else
	     {
		 type_t *btype = (type_t*)alloc(stf->stf_symtab->st_apool, sizeof(type_t));
		 btype->ty_code = DT_PTR_TO;
		 btype->ty_qualifiers = 0;
		 btype->ty_size = -1;
		 btype->ty_base = rtype;
		 v->va_type = btype;
		 v->va_flags |= VA_HIDE_PTR;
	     }
	}
	ci_make_baseclass_name(v);
    }
    *p_s = ++s;
    return v;
}
static void
SunProStructMethods(stf,sr, p_s)
stf_t *stf;
Symrec *sr;
const char **p_s;
{
    /*
    **   The SC4 compiler outputs a list of methods for
    **   the class.  We would need this to support calling
    **   a method from the inserted code.  We might also
    **   use this to remove the unnatural requirment that looking
    **   at a static member by clicking on its name only works
    **   in a non-Static member function.
    **
    **   For now, we only want to find the end of the list
    **
    **   Ref: SPARCworks debugger Interface, Chapter 6, Page 61
    */
    SkipPastSemi(stf,sr, p_s);
}
static void
SkipPastSemi(stf,sr, p_s)
stf_t *stf;
Symrec *sr;
const char **p_s;
{
    /* Skip to the next ';' and consume it */
    const char* s = *p_s;
    const char* end = strchr(s, ';');

    while(*s && end == NULL)
    {
	s = get_cont_symstring(sr);
	end = strchr(s, ';');
    }
    if ( end )
       s = end+1;
    if (*s == '\\')
         s = get_cont_symstring(sr);
    *p_s = s;
}
static void
SkipToSplChar(stf,sr, p_s, chars)
stf_t *stf;
Symrec *sr;
const char **p_s;
const char* chars;
{
    /* Skip to the next character in the string chars */
    const char* s = *p_s;
    const char* end = strpbrk(s, chars);
    while(*s && end == NULL)
    {
	s = get_cont_symstring(sr);
	end = strpbrk(s, chars);
    }
    if ( end )
       s = end;
    *p_s = s;
}
static var_t*
SunProStaticMembers(stf,sr, p_s, next, compiler)
stf_t *stf;
Symrec *sr;
const char **p_s;
var_t* next;
Compiler_type compiler;
{
    /*
    **   The SC4 compiler outputs a list of static members for
    **   the class.  Create a var_t for each of them. Get the names,
    **   demangle them, put the final component in the va_name field
    **   and the full name in the va_addr field.  Set the va_type field
    **   to something safe.
    **
    **   Ref: SPARCworks debugger Interface, Chapter 6, Page 61
    */

    const char* s = *p_s;


    if (*s == '\\')
      s = get_cont_symstring(sr);
    while(*s != ';')
    {
	var_t* v = (var_t *) alloc(stf->stf_symtab->st_apool, sizeof(var_t));
	const char* name = parse_name(sr, &s, stf->stf_symtab->st_apool,
				  0, compiler, FALSE);
	const char* ename = strrchr(name, ':');
	if ( ename)
	   v->va_name = ename+1;
	else
	   v->va_name = name;
	v->va_addr = (taddr_t)name;
	v->va_language = stf->stf_language;
	v->va_flags = VA_STATIC_MEMBER;
	v->va_class = CL_MOS;
	v->va_type = get_safe_type_ptr(); /* point to something safe */
	v->va_next = next;
	next = v;
	if (*s == '\\')
	  s = get_cont_symstring(sr);
	if ( *s == ' ')
	   bump_str(sr, &s);
    }
    bump_str(sr, &s);
    *p_s = s;
    return next;
}

static var_t*
SunProVectorTable(stf,sr, p_s, next, compiler)
stf_t *stf;
Symrec *sr;
const char **p_s;
var_t* next;
Compiler_type compiler;
{
    /*
    **   SC4 vector table info consists of two numbers.  The first
    **   is an algorithm number for accessing the vector table,
    **   which we ignore.  The second is offset in bytes of the
    **   virtual function table pointer in the structure.
    **
    **   Ref: SPARCworks debugger Interface, Chapter 6, Page 61
    */
    static type_t pVoidType[2];
    const char* s = *p_s;

    if ( !pVoidType[1].ty_base)
    {
       /* First time, create a type_t that says void* */
       pVoidType[0].ty_code = TY_VOID;
       pVoidType[1].ty_qualifiers = 0;
       pVoidType[0].ty_size = 0;
       pVoidType[0].ty_name = "void";
       pVoidType[1].ty_code = DT_PTR_TO;
       pVoidType[1].ty_size = sizeof(void*);
       pVoidType[1].ty_qualifiers = 0;
       pVoidType[1].ty_base = &pVoidType[0];
    }
    if (*s == '\\')
	s = get_cont_symstring(sr);
    if ( *s != ';')
    {
	int method = parse_num(stf, sr, &s);

        var_t *v =  (var_t *) alloc(stf->stf_symtab->st_apool, sizeof(var_t));
            v->va_name = "_vptr.";
            v->va_language = stf->stf_language;
            v->va_flags = VA_VTBLPTR;
            v->va_type = &pVoidType[1];
            bump_str(sr, &s);
            v->va_addr = parse_num(stf, sr, &s);
	    v->va_class = CL_MOS;
	    v->va_next = next;
	    next = v;

	(void) method;  /* Shut up unused var compiler warning */
    }

    bump_str(sr, &s);
    *p_s = s;
    return next;
}
	       
/* Field:
 *	NAME ':' TypeId ',' BitOffset ',' BitSize ';'
 */
static var_t *
Field(stf, sr, p_s, is_struct, next, eval, compiler, terminate)
stf_t *stf;
Symrec *sr;
const char **p_s;
int is_struct;
var_t *next;
bool eval;
Compiler_type compiler;
bool *terminate;
{
	const char *s;
	char prior_chr;
	type_t *type, *type1;
	int width = 0, offset;
	var_t *v = NULL;

	s = *p_s;

	if ((*s == ':' && *(s+1) != '/'))
	  bump_str(sr, &s);
	if (compiler == CT_GNU_CC &&
	    *s != ':' && *s != '?' && !isalpha(*s) && *s != '_' &&
		*s != '$' && *s != '.' && *s != '!')
	{
	  field_scheck(sr, &s, ';');
	  *terminate = TRUE;
	  return NULL;
	}

	/* if (!(*s == '!' || *s == '$')) */
	{
	  v = (var_t *) alloc(stf->stf_symtab->st_apool, sizeof(var_t));
	  v->va_name = parse_name(sr, &s, stf->stf_symtab->st_apool,
				  1, compiler, FALSE); /* RGA */
	  v->va_language = stf->stf_language;
	  v->va_flags = 0;
	  v->va_class = is_struct ? CL_MOS : CL_MOU;
	  if (compiler == CT_GNU_CC )
	  {
	      if  ( *v->va_name == '$' || *v->va_name == '.' )
	      {
	          if ( !strncmp(v->va_name+1, "vf", 2))
	          {
		      /* "$vf<number>: is a gnu virtual function table.
		      **  we would like to show these and display the
		      **  table it points to by name.  This would indicate
		      **  which sub-class of a base class an object is a]
		      **  member of.  Gdb does this.
		      */
        	      v->va_name = (*v->va_name == '$') ? "_vptr$" : "_vptr.";
	              v->va_flags =  VA_VTBLPTR;
		      width = 32;
	          } else
	          {
		     v = NULL;  /* compiler generated symbol. Throw away */
	          }
	      }
	  } else if (!strcmp(v->va_name, "__vptr"))
	  {
	       v->va_flags =  VA_VTBLPTR; /* clcc virtual function table */
	  }
	}
	if (*s == '\0')
	{
	  *p_s = s;
	  return NULL;
	}

	if (*s == '\\')
	{
	  s = get_cont_symstring(sr);
	  if (v && !strcmp(v->va_name, "A")) /* RGA */
	  {
	    char *tmp_s, *orig_s;
	    const char * tmp_s2;

	    orig_s = tmp_s = e_malloc(strlen(s) + 2);
	    sprintf(tmp_s, "A%s", s);
	    tmp_s2 = (const char *)tmp_s;
	    v->va_name = parse_name(sr, &tmp_s2, stf->stf_symtab->st_apool,
				    1, compiler, FALSE);
	    s += tmp_s2 - orig_s - 1;
	    free(orig_s);
	  }
	  *p_s = s;
	}

	prior_chr = *s == '\\' ? *(s-1) : *s;
	if (!field_scheck(sr, &s, ':')
	    || *s == ':' )
	{
	    if  ( *s == ':')
	    {
		 s++;
		 GnuStructMethods(stf, sr, &s);
	    }
	    *p_s = s;
	    *terminate = TRUE;
	    return NULL;
	}
	if (*s == '/')
	  s += 2;

	type = TypeId(stf, sr, &s, eval, prior_chr);
	/*
	** RCB:  This can occur when the type is currently
	**       under construction by one of the callers of
	**       in the call stack.  Returning NULL causes early
	**       termination of the list of fields.
	**       Problem reported to user's list,
	**          May 9, 2000 by J.W.Armstrong@swansea.ac.uk
	**       
	** if (type && type->ty_code == TY_NOTYPE)
	** {
	**   *p_s = s;
	**   return NULL;
	** }
	*/
	if(*s == '=')		/* RGA have seen this from clcc */
	{
	  prior_chr = *s == '\\' ? *(s-1) : *s;
	  bump_str(sr, &s);
	  type1 = TypeId(stf, sr, &s, eval, prior_chr); /* just ignore it */
	}

	if (*s == ':' )   /*   g++ static member */
	{
	    v->va_flags = VA_STATIC_MEMBER;
	    s++;
	    v->va_addr = (taddr_t) parse_name(sr, &s, stf->stf_symtab->st_apool,
				    0, compiler, FALSE);
	    v->va_type = type?type:get_safe_type_ptr();
	    (void)field_scheck(sr,&s, ';');
	    *p_s = s;
	    v->va_next = next;
	    return v;

	}
	if (*s != ',')
	{
	  if (type == NULL || !field_scheck(sr, &s, ';'))
	  {
	    *p_s = s;
	    return NULL;
	  }
	  width = typesize(type) * 8;
	  offset = 0;
	}
	else
	  {
	    if (!field_scheck(sr, &s, ',') || !isdigit(*s)) 
	    {
	      *p_s = s;
	      return NULL;
	    }
	    
	    offset = parse_num(stf, sr, &s);
	    if (width == 0 && !field_scheck(sr, &s, ','))
	    {
	      *p_s = s;
	      return NULL;
	    }
#ifdef ARCH_CLIPPER
	    /*  The Clipper puts this in the symbol table for varargs entries:
	     *
	     *	va_arg0:p65=s4_nameless:1,0,-1;;
	     *
	     *  so allow -1 as a possible width.
	     */
	    width = parse_signed_num(stf, sr, &s);
	    if (width < 0) {
	      if (width != -1)
		panic("width botch in Field");
	      width = typesize(type) * 8;
	    }
#else
	    if (compiler == CT_GNU_CC)
	    {
	      if (width == 0 )
	          width = parse_signed_num(stf, sr, &s);
	      if ( width == 0)
	      {
		 /* RCB: g++, with the -gstabs option, represents
		 ** static members as having 0 width
		 */
	         v->va_flags = VA_STATIC_MEMBER;
		 offset = 0; /* ensure that va_addr gets 0 */
	      } else
	      if (width < 0)
		width = typesize(type) * 8;
	    }
	    else
	      width = parse_num(stf, sr, &s);
#endif
	    if (compiler == CT_GNU_CC)
	      s++;
	    else
	      if (!field_scheck(sr, &s, ';'))
	      {
		*p_s = s;
		return NULL;
	      }
	  }
	*p_s = s;
	if (!eval || !v)
		return NULL;

	v->va_addr = offset / 8;

	if (!type || (type && !type->ty_size)) /* RGA for clcc */
	  v->va_type = type?type:get_safe_type_ptr();
	else
	    /* added by Joseph M. Orost joe@echologic.com */
	  if (type && type->ty_code == TY_ENUM && width != typesize(type) * 8) {
	    v->va_type = ci_make_bitfield_type
	      (stf->stf_symtab->st_apool,
	       ci_code_to_type(TY_UINT), offset - v->va_addr * 8, width);
	    v->va_type->ty_bitfield->bf_type = type;
	  } else
	if (type && IS_BASIC_TYPE(type->ty_code) && width != typesize(type) * 8) {
		v->va_type = ci_make_bitfield_type(stf->stf_symtab->st_apool,
						   type,
						   offset - (int)v->va_addr * 8,
						   width);
	}
	else {
		v->va_type = type?type:get_safe_type_ptr();
	}

	v->va_next = next;
	return v;
}

/*  Subrange:
 *	'r' TypeId ';' INTEGER ';' INTEGER
 *
 *  We take the letters 'T', 'A' and 'J' as indicating a dynamic array.
 *  This is not documented in the dbx(5) manual page - it was determined
 *  by examining symbol tables produced by the SunOS 3.5 f77 compiler.
 *  Your mileage may vary.
 *
 *  Update [several years later]: See page 84 of the SPARCworks Debugger
 *  Interface document.
 */
static dim_t *
Subrange(stf, sr, p_s, eval, want_subrange_type)
stf_t *stf;
Symrec *sr;
const char **p_s;
bool eval, want_subrange_type;
{
	const char *s;
	char prior_chr;
	dim_t *dim;

	s = *p_s;
	prior_chr = *s == '\\' ? *(s-1) : *s;
	scheck(sr, &s, 'r');
	dim = (dim_t *) alloc(stf->stf_symtab->st_apool, sizeof(dim_t));

	if (want_subrange_type) {
		dim->di_type = TypeId(stf, sr, &s, eval, prior_chr);
	}
	else {
		int junk;

		Typenum(stf, sr, FALSE, &s, &junk, &junk);
	}
	scheck(sr, &s, ';');

	/* RCB: Have seen an extra ';' here from gnu */
	if ( *s == ';')
	    scheck(sr, &s, ';');

	get_subrange_dim(stf, sr, &s, &dim->di_ltype, &dim->di_low);
	scheck(sr, &s, ';');

	get_subrange_dim(stf, sr, &s, &dim->di_htype, &dim->di_high);

	*p_s = s;
	return dim;
}

static void
get_subrange_dim(stf, sr, p_s, p_dimtype, p_val)
stf_t *stf;
Symrec *sr;
const char **p_s;
Dimtype *p_dimtype;
long *p_val;
{
	const char *s;
	Dimtype dimtype;

	s = *p_s;

	switch (*s) {
	case 'T':
		dimtype = DT_FPREL;
		bump_str(sr, &s);
		break;
	case 'A':
		dimtype = DT_APREL;
		bump_str(sr, &s);
		break;
	case 'J':
		dimtype = DT__UNKNOWN;
		bump_str(sr, &s);
		break;
	default:
		dimtype = DT_CONSTANT;
		break;
	}

	*p_dimtype = dimtype;
	*p_val = (dimtype == DT__UNKNOWN) ?
	  0 : parse_subrange_signed_num(stf, sr, &s);
	*p_s = s;
}

/*  Like parse_signed_num, but special case the values emitted by gcc
 *  for the 64 bit types.  These do not fit in 32 bits, so we just use
 *  special values which get_basic_type() will recognise.
 */
static int
parse_subrange_signed_num(stf, sr, p_s)
stf_t *stf;
Symrec *sr;
const char **p_s;
{
	const char *s;

	s = *p_s;

	if (*s == '0' && (s[1] == '1' || s[1] == '7')) {
		const char *ends;
		int ndig;
		
		for (ends = s; isdigit(*ends); bump_str(sr, &ends))
			;
		ndig = ends - s;

		*p_s = ends;
		
		if (ndig == 23 && memcmp(s, "01000000000000000000000", 23) == 0)
			return SR_MINUS_2E63;

		if (ndig == 22 && memcmp(s,  "0777777777777777777777", 22) == 0)
			return SR_2E63_MINUS_1;
		
		if (ndig == 23 && memcmp(s, "01777777777777777777777", 23) == 0)
			return SR_2E64_MINUS_1;

		/*  Not a special number after all ...
		 */
		*p_s = s;
	}

	return parse_signed_num(stf, sr, p_s);
}

#endif /* !ST_TE */

#endif /* AO_TARGET */
