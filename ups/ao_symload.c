/* ao_symload.c - load file and function symbols on demand */

/*  Copyright 1995 Mark Russell, University of Kent at Canterbury.
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


/* @(#)ao_symload.c	1.9 05 Jun 1995 (UKC) */
char ups_ao_symload_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#ifdef AO_TARGET

#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef AO_ELF
#	include "elfstab.h"
#else
#	include <a.out.h>
#	ifndef OS_RISCOS
#		include <stab.h>
#	endif
#endif

#include <local/wn.h>
#include <local/ukcprog.h>
#include <mtrprog/strcache.h>
#include <mtrprog/hash.h>
#include <local/edit/edit.h>

#include "ups.h"
#include "symtab.h"
#include "st.h"
#include "ci.h"
#include "ui.h"
#include "srcbuf.h"
#include "srcpath.h"
#include "ao_syms.h"
#include "ao_symload.h"
#include "ao_symread.h"
#include "ao_symparse.h"
#include "ao_symscan.h"
#include "ao_elfsym.h"
#include "util.h"
#include "cursors.h"

static double compiler_version;

static void get_fi_compiler PROTO((fil_t *fil));
#if defined(OS_SUNOS) || defined(OS_LINUX)
static stf_t *id_to_hdrstf PROTO((stf_t *stf, taddr_t id, int symno));
#endif
static void get_fi_types PROTO((fil_t *fil, bool is_header));
static aggr_or_enum_def_t *match_tag PROTO((aggr_or_enum_def_t *ae, 
                                            const char *tag));
static aggr_or_enum_def_t *fix_undef_aggr PROTO((aggr_or_enum_def_t *uae, 
                                                 const char *arg));
static void fix_params PROTO((func_t *f));
static int backup_lnum_to_curly PROTO((func_t *f, fil_t *fil, int orig_lnum));
static taddr_t xbrac_addr PROTO((func_t *f, unsigned long val));
static void get_lno_symrange PROTO((func_t *f, int *p_start, int *p_lim));
#if !defined(AO_ELF)
static void fixup_initial_lnos PROTO((func_t *f));
#endif
#ifdef AO_ELF
static bool static_addrs_relative PROTO((Compiler_type compiler_type));
#endif

void display_message PROTO((const char *mesg));

void
push_typedefs_and_aggrs(ap, hdrbl, bl)
alloc_pool_t *ap;
block_t *hdrbl, *bl;
{
	if (hdrbl->bl_aggr_or_enum_defs != NULL) {
		aggr_or_enum_def_t *ae;

		ae = (aggr_or_enum_def_t *)alloc(ap,
						 sizeof(aggr_or_enum_def_t));
		ae->ae_type = NULL;
		ae->ae_sublist = hdrbl->bl_aggr_or_enum_defs;
		ae->ae_next = bl->bl_aggr_or_enum_defs;
		bl->bl_aggr_or_enum_defs = ae;
	}

	if (hdrbl->bl_typedefs != NULL) {
		typedef_t *td;

		td = (typedef_t *)alloc(ap, sizeof(typedef_t));
		td->td_type = NULL;
		td->td_sublist = hdrbl->bl_typedefs;
		td->td_next = bl->bl_typedefs;
		bl->bl_typedefs = td;
	}
}

#if defined(OS_SUNOS) || defined(OS_LINUX)
static stf_t *
id_to_hdrstf(stf, id, symno)
stf_t *stf;
taddr_t id;
int symno;
{
	hf_t **p_hf, **maxhf;
	stf_t *hdrstf;

	maxhf = stf->stf_fmap + stf->stf_mapsize;
	hdrstf = NULL;
	for (p_hf = stf->stf_fmap; p_hf < maxhf; ++p_hf) {
		if ((*p_hf)->hf_id == id) {
			if (symno != -1 && (*p_hf)->hf_stf->stf_symno != symno)
				continue;
			hdrstf = (*p_hf)->hf_stf;
			break;
		}
	}
	
	if (hdrstf == NULL)
		panic("hdr botch in ght");

	return hdrstf;
}
#endif

static
void 
get_fi_compiler(fil)
fil_t *fil;
{

  stf_t *stf, *stf_next;
  int symno, min_symno, max_symno;
  /* cursor_t old_cursor; */
  static int checked = 0;
  static Compiler_type sym;
  static char *c_str = NULL;
  symtab_t *st;
  Symrec symrec;
  nlist_t nm;
  const char *c;

#ifdef AO_ELF

  if (!elf_scan_then_setup_symio(fil, &symrec.symio))
    return;
  if (symrec.symio == NULL)	/* RGA added */
    return;
#else

  int func_symno, back;
  funclist_t *fl;

  symrec.symio = AO_STDATA(fil->fi_symtab)->st_text_symio;
#endif
	
  min_symno = 0;
  stf = AO_FIDATA(fil);
  if (fil->fi_next)
  {
    stf_next = AO_FIDATA(fil->fi_next);
    if (stf_next)
      min_symno = stf_next->stf_symno;
  }

  st = stf->stf_symtab;
  if (!checked)
  {
    char *c2;

    c_str = (char *)getenv("UPS_COMPILER");
    if (c_str)
      for (c2 = c_str; *c2; c2++)
	*c2 = tolower(*c2);
    if (c_str)
    {
      if (!strcmp(c_str, "cc"))
	sym = CT_CC;
      else
	if (!strcmp(c_str, "clcc"))
	  sym = CT_CLCC;
	else
	  if (!strcmp(c_str, "gcc"))
	    sym = CT_GNU_CC;
	  else
	    if (!strcmp(c_str, "f77"))
	      sym = CT_F77;
	    else
	      if (!strcmp(c_str, "f90"))
		sym = CT_EPCF90;
	      else
		sym = CT_UNKNOWN;
      ao_compiler(NULL, TRUE, sym);
    }
    checked = 1;
  }
  if (c_str)
  {
    stf->stf_compiler_type = sym;
    return;
  }

  stf->stf_compiler_type =
    (stf->stf_language == LANG_C || stf->stf_language == LANG_CC)
      ? CT_CC : (stf->stf_language == LANG_UNKNOWN) ? CT_UNKNOWN : CT_F77;

  /*  old_cursor = wn_get_window_cursor(WN_STDWIN);*/
  /*  set_bm_cursor(WN_STDWIN, CU_WAIT);*/

#ifdef AO_ELF
  max_symno = stf->stf_symno + 8;
  for (symno = 0; symno < max_symno; symno++)
  {
    getsym(symrec.symio, symno, &nm);

    if (nm.n_type == N_OPT)
    {
      c = symstring(symrec.symio, symno);

      if (strcmp(c, "gcc2_compiled.") == 0) 
	stf->stf_compiler_type = CT_GNU_CC;
      else 
	switch (stf->stf_language)
	{
	case LANG_F77:
	  stf->stf_compiler_type = CT_F77;
	  break;
	case LANG_F90:
	  stf->stf_compiler_type = CT_EPCF90;
	  break;
	case LANG_C:
	case LANG_CC:
	  if (strncmp(c, "clcc_", 5) == 0)
	    stf->stf_compiler_type = CT_CLCC;
	  else
	    stf->stf_compiler_type = CT_CC;
	  break;
	default:
	  if (strncmp(c, "clcc_", 5) == 0)
	  {
	    stf->stf_compiler_type = CT_CLCC;
	    stf->stf_language = LANG_CC;
	  }
	  else
	    stf->stf_compiler_type = CT_UNKNOWN;
	  break;
	}
      break;
    }
  }
  
#else

  /* RGA This is gross - there must be a better non */
  /* heuristic way doing this */

  fl = fil->fi_funclist;
/*  printf("%s\n",fil->fi_name);*/
  func_symno = (fl != NULL) ? AO_FSYMDATA(fl->fl_func)->fs_symno : -1;
  max_symno = stf->stf_symno + 8;
  if (max_symno > stf->stf_symlim)
    max_symno = stf->stf_symlim;
  if (min_symno > stf->stf_symno - 150)
    min_symno = stf->stf_symno - 150;
  if (min_symno < stf->stf_symno - 250)
    min_symno = stf->stf_symno - 250;
  if (min_symno < 0)
    min_symno = 0;
  for (back = 0,symno = stf->stf_symno; symno >= 0; back ? symno-- : symno++)
  {
    if (back && symno <= min_symno)
      break;

    getsym(symrec.symio, symno, &nm);
/*    if (nm.n_type == N_TEXT || nm.n_type == 0x3c) */
    {
      c = symstring(symrec.symio, symno);
      if (!c)
	continue;
/*      printf("\t%d: %s\n",symno,c);*/
      if (nm.n_type == N_TEXT)
      {
	if (back)
	{
	  if (*c == 'g' && !strncmp("cc_com", c + 1, 6)) /* gcc */
	  {
	    stf->stf_compiler_type = CT_GNU_CC;
	    break;
	  }
	  else
	    if (!strncmp("___gnu", c, 6))
	    {			/* "___gnu_compiled_cplusplus" presumably */
				/* but use LANG_CC to figure this out */
	      stf->stf_compiler_type = CT_GNU_CC;
	      break;
	    }
	    else
	      if (*c == 'c' && !strncmp("lcc_", c + 1, 4)) /* just look at the */
		/* beginning for clcc*/
	      {
		stf->stf_compiler_type = CT_CLCC;
		break;
	      }
	}
      }
    }
    if (!back && (symno >= max_symno || symno == func_symno))
    {
      back = 1;
      symno = stf->stf_symno;
    }
  }
#endif
    /*    wn_define_cursor(WN_STDWIN, old_cursor);*/
}


static void
get_fi_types(fil, is_header)
fil_t *fil;
bool is_header;
{
	stf_t *stf;
	symtab_t *st;
	Symrec symrec;
	int func_symno;
	funclist_t *fl;
	block_t *bl;
	
	if (fil->fi_flags & FI_DOING_TYPES)
		panic("dup fil in gft");
	if (fil->fi_flags & FI_DONE_TYPES)
		return;
	
#ifdef AO_ELF
	if (!elf_scan_then_setup_symio(fil, &symrec.symio))
		return;
#else
	symrec.symio = AO_STDATA(fil->fi_symtab)->st_text_symio;
#endif

	fil->fi_flags |= FI_DOING_TYPES;
 	if (!is_header && !(fil->fi_flags & FI_FOUND_COMPILER))	/* RGA */
 	{
 	  get_fi_compiler(fil);
 	  fil->fi_flags |= FI_FOUND_COMPILER;
 	}
	
	bl = fil->fi_block;
	stf = AO_FIDATA(fil);
	st = fil->fi_symtab;
	fl = fil->fi_funclist;
	func_symno = (fl != NULL) ? AO_FSYMDATA(fl->fl_func)->fs_symno : -1;

	/*  Note that we skip the 0th symbol in the source file, which
	 *  is the N_SO or N_BINCL symbol.  This is essential for N_BINCL -
	 *  if we didn't skip it we would recurse forever.
	 */
	for (symrec.symno = stf->stf_symno + 1; ; ++symrec.symno) {
		nlist_t nm;
		const char *name, *s;
		class_t class;
		type_t *type;
		bool push_aggr;
		aggr_or_enum_def_t *ae;

		/*  Skip over the symbols of functions - we don't
		 *  want type information from functions.
		 */
		while (symrec.symno == func_symno) {
			symrec.symno = AO_FSYMDATA(fl->fl_func)->fs_symlim;
			fl = fl->fl_next;
			func_symno = (fl != NULL) ? AO_FSYMDATA(fl->fl_func)->fs_symno : -1;
		}

		if (symrec.symno >= stf->stf_symlim)
			break;
		
		getsym(symrec.symio, symrec.symno, &nm);
#if defined(OS_SUNOS) || defined(OS_LINUX)
		if ((nm.n_type == N_BINCL || nm.n_type == N_EXCL)
							   && nm.n_value != 0) {
			stf_t *hdrstf;
			int match_symno;

			/*  If the symbol is N_BINCL we insist on the current
			 *  symbol number for hdrstf->stf_symno for a match
			 *  in id_to_hdrstf().  This is because you can have
			 *  different header files with the same id, and if
			 *  two such header files are #included in the same
			 *  source file we could pick up the wrong one if
			 *  we went just by the id.  Picking the wrong one
			 *  is disastrous for an N_BINCL because we set
			 *  symno below to hdrstf->stf_symlim.
			 */
			match_symno = (nm.n_type == N_BINCL) ? symrec.symno
							     : -1;
			hdrstf = id_to_hdrstf(stf, nm.n_value, match_symno);

			get_fi_types(hdrstf->stf_fil, TRUE);
			if (nm.n_type == N_BINCL)
				symrec.symno = hdrstf->stf_symlim - 1;
		}
#endif
		if (nm.n_type != N_LSYM
#ifdef N_ISYM					/* RCB, for SC5 */
                    && nm.n_type != N_ISYM
                    && nm.n_type != N_ESYM
#endif
		)
			continue;

		name = symstring(symrec.symio, symrec.symno);

		/*  Skip junk symbols, and ones that aren't TypeDefs (see st_parse.c)
		 */
		for (s = name; isalnum(*s) || *s == '_' || *s == '$'; ++s)
			;
		if (*s != ':' || (s[1] != 'T' && s[1] != 't'))
			continue;

		++s;
		if (!(symrec.symno % 100)) /* RGA check every 100th symbol */
		  if (!stop_pressed(0, 0) &&
		      user_wants_stop(TRUE)) /* RGA peek in queue */
		    stop_pressed(1, 0);	/* set */

		type = Class(stf, &symrec, &s, &class);

		if (!type)	/* RGA for C++ */
		  continue;

		switch(class) {
		case CL_TYPEDEF:
			type->ty_typedef->td_next = bl->bl_typedefs;
			bl->bl_typedefs = type->ty_typedef;

			ae = type->ty_aggr_or_enum;
			
			/*  Unnamed aggregate types don't get a CL_TAGNAME
			 *  entry, so if we get one that hasn't already been
			 *  typedefed, fall through to the aggregate pushing
			 *  code.
			 */  
			switch (type->ty_code) {
			case TY_STRUCT:
			case TY_UNION:
			case TY_ENUM:
				push_aggr = type->ty_typedef == NULL &&
					    ae->ae_tag == NULL;
				break;
			default:
				push_aggr = FALSE;
				break;
			}

			break;
			
			
		case CL_TAGNAME:
			/*  Incomplete types get pushed by make_undef_type.
			 */
			ae = type->ty_aggr_or_enum;
			push_aggr = ae->ae_is_complete == AE_COMPLETE;
			break;
			
		case CL_NOCLASS:
			push_aggr = FALSE;
			ae = NULL; /* to satisfy gcc */
			break;
			
		default:
			panic("bad class in gft");
			push_aggr = FALSE;	/* to satisfy gcc */
			ae = NULL;		/* to satisfy gcc */
			
		}

		if (push_aggr) {
		  aggr_or_enum_def_t *ae1;
/*	RGA		if (ae->ae_next != NULL)
			panic("ae botch in gft");*/

		  /* RGA break infinte loop in ci_apply_to_aelist() */
		  /* and keep going. Seen in some SC4 C++ targets */
			if (ae->ae_next != NULL || 
			    ae == bl->bl_aggr_or_enum_defs)
			{
			  ae->ae_next = NULL;
			  continue;
			}
			

		  for (ae1 = bl->bl_aggr_or_enum_defs;
		       ae1 != NULL && ae1 != ae;
		       ae1 = ae1->ae_next);
		  /* RGA break infinte loop in ci_apply_to_aelist() */
		  /* and keep going. Seen in some SC4 C++ targets */
		    if (ae1 && ae1 == ae)
		    {
		      ae->ae_next = NULL;
		      continue;
		    }

			ae->ae_next = bl->bl_aggr_or_enum_defs;
			bl->bl_aggr_or_enum_defs = ae;
		}
			
	}

	wrapup_types(stf);

	if (!is_header) {
		int i;
		
		for (i = 1; i < stf->stf_mapsize; ++i) {
			stf_t *hstf;

			hstf = stf->stf_fmap[i]->hf_stf;
			push_typedefs_and_aggrs(st->st_apool,
						hstf->stf_fil->fi_block, bl);
		}

		ci_apply_to_aelist(bl->bl_aggr_or_enum_defs, fix_undef_aggr,
				   (char *)bl->bl_aggr_or_enum_defs);
	}

	fil->fi_flags &= ~FI_DOING_TYPES;
	fil->fi_flags |= FI_DONE_TYPES;

#if WANT_DEBUG
dump_header(fil->fi_name);
dump_typedef_t(fil->fi_block->bl_typedefs, 0, TRUE);
dump_trailer();
#endif
}

static aggr_or_enum_def_t *
match_tag(ae, tag)
aggr_or_enum_def_t *ae;
const char *tag;
{
	if (ae->ae_is_complete == AE_COMPLETE &&
	    ae->ae_tag != NULL &&
	    strcmp(ae->ae_tag, tag) == 0)
		return ae;
	
	return NULL;
}

static aggr_or_enum_def_t *
fix_undef_aggr(uae, arg)
aggr_or_enum_def_t *uae;
const char *arg;
{
	aggr_or_enum_def_t *aelist;
	type_t *utype, *type;
	aggr_or_enum_def_t *ae;

	aelist = (aggr_or_enum_def_t *)arg;

	switch (uae->ae_type->ty_code) {
	case TY_U_STRUCT:
	case TY_U_UNION:
	case TY_U_ENUM:
		if (uae->ae_tag == NULL)
			ae = NULL;
		else
			ae = ci_apply_to_aelist(aelist, match_tag, uae->ae_tag);
		break;
	default:
		ae = NULL;
		break;
	}

	if (ae == NULL)
		return NULL;

	utype = uae->ae_type;
	type = ae->ae_type;

	utype->ty_code = type->ty_code;
	utype->ty_aggr_or_enum = ae;

	if (utype->ty_typedef != NULL && type->ty_typedef == NULL) {
		type->ty_typedef = utype->ty_typedef;
		type->ty_typedef->td_type = type;
	}

	return NULL;
}

/*  This routine is an optimisation - it is intended to avoid loading
 *  the globals of a file if none of them match according to the supplied
 *  comparison function.  This is to avoid demand loading of all the
 *  globals of every file when the user gives us a non-existent name.
 */
bool
ao_fil_may_have_matching_globals(fil, pat, matchf)
fil_t *fil;
const char *pat;
matchfunc_t matchf;
{
#ifndef ST_TE
	/*  Only worth trying to save time if the globals aren't already loaded.
	 */
	if ((fil->fi_flags & FI_DONE_VARS) == 0) {
		snlist_t *sn;

		sn = AO_FIDATA(fil)->stf_snlist;
		for (; sn != NULL; sn = sn->sn_next) {
			if ((*matchf)(sn->sn_name, pat))
				return TRUE;
		}

		return FALSE;	/* Definitely has none matching */
	}
#endif
	return TRUE;	/* Dunno - it might have */
}

/*  NOTE: this routine is not used on the DECstation 3100.
 */
var_t *
ao_get_fi_vars(fil)
fil_t *fil;
{
	nlist_t nm;
	snlist_t *sn;
	class_t class;
	type_t *type;
	const char *s;
	Symrec symrec;
	symtab_t *st;
	ao_stdata_t *ast;
	stf_t *stf;
	var_t *v, *last;
	taddr_t addr;
	errf_ofunc_t oldf;
        cursor_t old_cursor;
	int rational_products_used = 0;

	if (fil->fi_flags & FI_DONE_VARS)
		return fil->fi_block->bl_vars;

#ifdef AO_ELF
	if (!elf_scan_then_setup_symio(fil, &symrec.symio))
		return NULL;
#else
	symrec.symio = AO_STDATA(fil->fi_symtab)->st_text_symio;
#endif
	
	oldf = errf_set_ofunc(display_message);
	errf("\bReading symbols of `%s'... ", fil->fi_name_only);
	/* This may take a while.  Put up a blocking cursor */
	old_cursor = wn_get_window_cursor(WN_STDWIN);
	set_bm_cursor(WN_STDWIN, CU_WAIT);
	indicate_target_menu_search(1);

	stf = AO_FIDATA(fil);

	/*  Needed for typedefs etc, as with get_fu_blocks().
	 */
	get_fi_types(fil, FALSE);

	fil->fi_block->bl_vars = last = NULL;
	st = fil->fi_symtab;
	ast = AO_STDATA(st);

#ifdef AO_ELF
	if (stf->stf_compiler_type == CT_CLCC)
		rational_products_used = rational_products_running();
#endif

	for (sn = stf->stf_snlist; sn != NULL; sn = sn->sn_next) {
		symrec.symno = sn->sn_symno;
		getsym(symrec.symio, symrec.symno, &nm);
#ifdef OS_ULTRIX
		if (sn->sn_addr != 0)
			nm.n_value = sn->sn_addr;
#endif
#ifdef ARCH_CLIPPER
		addr = find_addr_in_addrlist(stf->stf_addrlist, sn->sn_name);
		if (addr != NULL)
			nm.n_value = addr;
#endif
		s = symstring(symrec.symio, symrec.symno);

		/* RGA have seen this with SC4 with templates */
		while(*s==';')
		  s++;
		
		if(*s == 0 || *s==' ')
		  continue;

		parse_name(&symrec, &s, (alloc_pool_t *)NULL, 1,
			   stf->stf_compiler_type, IS_FORTRAN(stf->stf_language));

		/*  Ignore symbols with funny characters in their names.
		 *  For normally named symbols, parse_name will have
		 *  left us at the ':' that follows the name, so if
		 *  *s is not ':' then we have an oddly named symbol.
		 *
		 *  BUG: the reason for adding this code is that we
		 *  tripped over the basic type definition of
		 *  "logical*4" in an a.out produced by the SunOS 4.0
		 *  f77 compiler.  This is because we assume a fixed
		 *  number (N_FORTRAN_BASIC_TYPES) of basic types, and
		 *  the 4.0 f77 added a new one.  We need a better way
		 *  of deciding how many basic types there are, based
		 *  on the symbol table of the file.
		 */
		if (*s != ':')
			continue;
		++s;

		type = Class(stf, &symrec, &s, &class);

		/* RGA: from Russell Browne:
		   When compiled with SC4 as a C rather than C++ program, UPS
		   gets a bogus value for static globals out of the symbol table.
		   Is not a problem with other compilers.
		   SC5 does this for C++ also. (RCB)
		*/

		if (nm.n_value != 0
#ifdef AO_ELF
		    && !((stf->stf_compiler_type == CT_CC &&
			  (class == CL_EXT || class == CL_STAT)) ||
			  (stf->stf_compiler_type == CT_CLCC &&
			   rational_products_used &&
			   (class == CL_EXT || class == CL_STAT)))
#endif
			  )
			addr = (taddr_t)nm.n_value + ast->st_base_address;
		else
			addr = lookup_global_addr(st, sn->sn_symtab_name);

		if (addr != 0 && (class == CL_EXT || class == CL_STAT)) {
			v = ci_make_var(st->st_apool, sn->sn_name,
							class, type, addr);
			v->va_language = fil->fi_language;
			v->va_next = fil->fi_block->bl_vars;
			if (last != NULL)
				last->va_next = v;
			else
				fil->fi_block->bl_vars = v;
			last = v;
		}
	}
	if (last != NULL)
		last->va_next = NULL;
	fil->fi_flags |= FI_DONE_VARS;
	errf("\bReading symbols of `%s'... done", fil->fi_name_only);
	errf_set_ofunc(oldf);
        if (target_menu_search_disabled(0, 0) == FALSE)
        {
		bool stop;

		stop = stop_pressed(0, 0);
		indicate_target_menu_search(0);
		if (stop)
		    stop_pressed(1, 0);
				/* set it back again to break out of reading */
                                /* more source file symbols */
        }
	wn_define_cursor(WN_STDWIN, old_cursor);

#if WANT_DEBUG
dump_header(fil->fi_name_only);
dump_fil_t(fil, FALSE, FALSE);
dump_var_t(fil->fi_block->bl_vars, TRUE);
dump_trailer();
#endif
	return fil->fi_block->bl_vars;
}

macro_t *
ao_get_fi_macros(fil)
fil_t *fil;
{
	fil->fi_flags |= FI_DONE_MACROS;

	return fil->fi_macros;
}

/*  Many compilers give you two seperate symbol table entries for a
 *  register parameter - one for the stack copy of the parameter, one
 *  for the register.  Go through the vars of f zapping the stack
 *  symbols of any such registers.
 *
 *  Some compilers will also generate two symbol table entries for
 *  stack based parameters if the parameter has to be promoted when
 *  it is passed - the first is a parameter entry of type int and
 *  the second is for the stack copy and has the actual type given
 *  in the function prototype. As with the register case we zap the
 *  extra copy.
 *
 *  We use an n^2 algorithm as we don't expect functions to have more than
 *  about ten arguments.
 */
static void
fix_params(f)
func_t *f;
{
	register var_t *vreg, *varg;
	var_t *prev, *next, *head;
	int zapped_vreg;

	/*  We are only interested in the vars at the outermost scope.
	 */
	if (f->fu__blocks == NULL)
		return;

	head = f->fu__blocks->bl_vars;

	prev = NULL;
	for (vreg = head; vreg != NULL; vreg = next) {
		next = vreg->va_next;
		zapped_vreg = FALSE;
		if (vreg->va_class == CL_REG || vreg->va_class == CL_AUTO) {
			for (varg = head; varg != NULL; varg = varg->va_next){
				if (varg->va_class == CL_ARG &&
				    strcmp(vreg->va_name, varg->va_name) == 0) {
					vreg->va_next = varg->va_next;
					*varg = *vreg;
					if (prev != NULL)
						prev->va_next = next;
					else
						head = next;
					
					/*  Would free vreg at this point but
					 *  it was allocated by alloc() so
					 *  we can't - forget it.
					 */

					zapped_vreg = TRUE;
					break;
				}
			}
		}
		if (!zapped_vreg)
			prev = vreg;
	}

	f->fu__blocks->bl_vars = head;
}

#ifndef ST_TE
/*  This function is an attempt to improve the accuracy of block start line
 *  numbers.  Most compilers make the LBRAC for a block point to after the
 *  declarations of variables in the block, but we want the declarations
 *  included in the block.
 *
 *  The way we do this is to search back through the source lines, looking
 *  for the '{' character that starts a block.  We only go back a smallish
 *  number of lines - if we don't find anything by then we just return orig_lnum.
 *
 *  Note that we make no attempt to deal with #ifdefs, comments, strings etc.
 *  To do this right in general would require too much knowledge of C.
 *  It doesn't matter if we get this wrong - the result is just that our
 *  block numbers will be slightly off.
 *
 *  The so package numbers lines from zero up, whereas the symbol table
 *  numbers them from one up, so we have to adjust.
 *
 *  NOTE: this routine is not used on the DECstation 3100.
 */
static int
backup_lnum_to_curly(f, fil, orig_lnum)
func_t *f;
fil_t *fil;
int orig_lnum;
{
	const int lbrace = '{'; /* to avoid upsetting vi's '}' matching */
	Srcbuf *sb;
	const char *cptr;
	int lnum, min_lnum;
				/* RGA no blocking for C++ */
/*	if (f->fu_language == LANG_CC)*/
/*	  return orig_lnum;*/

	/*  The call to srcpath_file_exists() is to avoid an error message
	 *  in the case where we can't find the source file.
	 */
	if (fil == NULL ||
	    !srcpath_file_exists(fil->fi_path_hint, fil->fi_name) ||
	    !open_source_file(fil, FALSE, FALSE)) {
		return orig_lnum;
	}
	
	sb = fil->fi_srcbuf;

	if (orig_lnum > srcbuf_nlines_in_file(sb))
		return orig_lnum;
	
	/*  Make sure that we only go back a reasonable distance, and that we
	 *  don't go out of range.
	 */
	min_lnum = orig_lnum - 50;
	if (min_lnum < 1)
		min_lnum = 1;
	if (orig_lnum < 1)
		orig_lnum = 1;

	lnum = orig_lnum;

	/*  If the opening line doesn't have an lbrace, or has an
	 *  lbrace with nothing but whitespace after it, back up
	 *  a line.
	 *
	 *  Again, this code is not bulletproof (we'll get confused by
	 *  if the lbrace is inside a comment, for example), but it doesn't
	 *  matter much if it gets things wrong.
	 */
	cptr = strrchr(srcbuf_get_file_line(sb, lnum), lbrace);
	if (cptr != NULL) {
		int lastch;
		bool incomment;

		incomment = FALSE;
		lastch = *cptr++;
		for (; ; ++cptr) {
			if (incomment && lastch == '*' && *cptr == '/')
				incomment = FALSE;
			else if (!incomment && lastch == '/' && *cptr == '*')
				incomment = TRUE;
			else {
				if (!incomment && isalnum(*cptr))
					break;
				lastch = *cptr;
			}
			if (*cptr == '\0') {
				--lnum;
				break;
			}
		}
	}
	if (cptr == NULL || *cptr == '\0')
		--lnum;

	for (; lnum >= min_lnum; --lnum)
		if (strchr(srcbuf_get_file_line(sb, lnum), lbrace) != NULL)
			return lnum;
	return orig_lnum;
}

static taddr_t
xbrac_addr(f, val)
func_t *f;
unsigned long val;
{
#if defined(ARCH_386) && defined(OS_SUNOS)
	return val;
#else
#ifdef AO_ELF
	return f->fu_addr + val;
#else
	return AO_FIDATA(f->fu_fil)->stf_addr + val;
#endif /* !AO_ELF */
#endif /* !(ARCH_386 && OS_SUNOS) */
}

#ifdef AO_ELF
static bool
static_addrs_relative(compiler_type)
Compiler_type compiler_type;
{
	return compiler_type != CT_GNU_CC && compiler_type != CT_GNU_F77;
}
#endif /* AO_ELF */

block_t *
ao_get_fu_blocks(f)
func_t *f;
{
	nlist_t nm, extra_nm;
	class_t class;
	type_t *type;
	taddr_t addr;
	alloc_pool_t *ap;
	const char *name, *s;
	ao_stdata_t *ast;
	Symrec symrec;
	stf_t *stf;
	var_t *v;
	var_t **p_varlist, *varlists[MAX_BLOCK_LEVEL];
	aggr_or_enum_def_t **p_aelist, *aelists[MAX_BLOCK_LEVEL];
	typedef_t **p_tdlist, *tdlists[MAX_BLOCK_LEVEL];
	block_t *blocklists_tab[MAX_BLOCK_LEVEL + 1], **blocklists, *bl;
	bool skipthis, sane_bracs, build_level_0_block, seen_lbrac;
	errf_ofunc_t oldf;
	int symlim, extra_sym, level, lnum, next_lnum;
	unsigned short last_sline = 0;

	if (f->fu_flags & FU_DONE_BLOCKS)
		return f->fu__blocks;
	
	oldf = errf_set_ofunc(display_message);
	errf("\bReading symbols of `%s'... ", f->fu_demangled_name);

	ast = AO_STDATA(f->fu_symtab);
	
#ifdef AO_ELF
	if (!elf_scan_then_setup_symio(f->fu_fil, &symrec.symio))
		return NULL;
#else
	symrec.symio = ast->st_text_symio;
#endif
	
	stf = AO_FIDATA(f->fu_fil);

	/*  We want to use typedef names if possible for structures and
	 *  enums (see ci_basetype_name), so we need any type information
	 *  in the file.  Thus we load the file types.
	 */
	get_fi_types(f->fu_fil, FALSE);

	ap = f->fu_symtab->st_apool;
	
	blocklists = blocklists_tab + 1;
	blocklists[-1] = f->fu_fil->fi_block;

	level = 0;
	varlists[0] = NULL;
	varlists[1] = NULL;	/* RGA */
	aelists[0] = NULL;
	tdlists[0] = NULL;

	/* RGA add file types here so "add expr" can reference them */
	if (f->fu_fil->fi_block)
	{
	  aelists[0] = f->fu_fil->fi_block->bl_aggr_or_enum_defs;
	  tdlists[0] = f->fu_fil->fi_block->bl_typedefs;
	}
	blocklists[0] = NULL;
	bl = NULL;
	
	p_varlist = &varlists[level];
	p_aelist = &aelists[level];
	p_tdlist = &tdlists[level];

	symlim = AO_FSYMDATA(f)->fs_symlim;
	extra_sym = 0;
	build_level_0_block = FALSE;

	seen_lbrac = FALSE;
	sane_bracs = FALSE;	/* until proven otherwise by first lbrac */

	for (symrec.symno = AO_FSYMDATA(f)->fs_symno;
	     symrec.symno < symlim + extra_sym;
	     ++symrec.symno) {
		fil_t *fil = 0;
		
		if (symrec.symno == symlim)
			nm = extra_nm;
		else
			getsym(symrec.symio, symrec.symno, &nm);
		
		switch(nm.n_type) {
		case N_SLINE:
		  	last_sline = (unsigned)nm.n_desc;
			if (fil && bl && bl->bl_start_lnum == 0)
			{
			  lnum = backup_lnum_to_curly(f, fil, last_sline);
			  bl->bl_start_lnum = lnum;
			}
			if (fil && bl && bl->bl_end_lnum == 0)
			{
			  lnum = last_sline;
			  bl->bl_end_lnum = lnum;
			}
			break;

		case N_LBRAC:
			if (!seen_lbrac) {
#ifdef AO_ELF
				sane_bracs = nm.n_desc != 0;
#else
				sane_bracs = TRUE;
#endif
				seen_lbrac = TRUE;
			}

			addr_to_fil_and_lnum(f, xbrac_addr(f, nm.n_value),
					     &fil, &lnum, FALSE);

			if (fil == NULL) {
				fil = f->fu_fil;
				lnum = 0;
			}

			if (lnum > 0)
			  lnum = backup_lnum_to_curly(f, fil, lnum);

				/* RGA turn off blocking for C++ */
/*			if (stf->stf_language == LANG_CC)*/
/*			  break;*/

			if (sane_bracs) {
				block_t *par;

				par = bl;
				bl = ci_make_block(ap, par);
				++level;

				if (par == NULL) {
					bl->bl_vars = varlists[0];
					bl->bl_aggr_or_enum_defs = aelists[0];
					bl->bl_typedefs = tdlists[0];
				}
				else {
					bl->bl_next = par->bl_blocks;
					par->bl_blocks = bl;
				}

				bl->bl_start_lnum = lnum;
				bl->bl_end_lnum = lnum;

				p_varlist = &bl->bl_vars;
				p_aelist = &bl->bl_aggr_or_enum_defs;
				p_tdlist = &bl->bl_typedefs;
				
				break;
			}

			if (level == MAX_BLOCK_LEVEL)
				panic("block nesting too deep");

			
			bl = ci_make_block(ap, blocklists[level - 1]);
			bl->bl_start_lnum = lnum;
			bl->bl_end_lnum = lnum;

			bl->bl_vars = varlists[level];
			varlists[level] = NULL;
			bl->bl_typedefs = tdlists[level];
			tdlists[level] = NULL;
			bl->bl_aggr_or_enum_defs = aelists[level];
			aelists[level] = NULL;

			bl->bl_next = blocklists[level];
			blocklists[level] = bl;

			++level;
			blocklists[level] = NULL;
			varlists[level] = NULL;
			aelists[level] = NULL;
			tdlists[level] = NULL;
			
			p_varlist = &varlists[level];
			p_aelist = &aelists[level];
			p_tdlist = &tdlists[level];

			break;

		case N_RBRAC:
			lnum = rbrac_addr_to_lnum(f, xbrac_addr(f, nm.n_value));

				/* RGA turn off blocking for C++  */
/*			if (stf->stf_language == LANG_CC)*/
/*			  break;*/

/* RGA comment out */
/*#ifdef OS_SUNOS_5*/
			if (sane_bracs) {

				if (bl == NULL)
					panic("missing LBRAC");

				if (lnum > 0)
				  bl->bl_end_lnum = lnum;
				else
				  bl->bl_end_lnum = last_sline;

				--level;
				
				if (bl->bl_parent == NULL) {
					symlim = symrec.symno;	/* finished */
					break;
				}
				
				bl = bl->bl_parent;
				
				p_varlist = &bl->bl_vars;
				p_aelist = &bl->bl_aggr_or_enum_defs;
				p_tdlist = &bl->bl_typedefs;
				
				break;
			}
/* RGA comment out */
/*#endif*/
			
			if (level <= 0)
				panic("missing LBRAC");
			
			/*  The logically last RBRAC symbol of a function
			 *  (the one that takes us from level 1 to level 0)
			 *  sometimes appears earlier than we'd expect.
			 *  We arrange that we always process this symbol
			 *  last.
			 */
			if (level == 1 && symrec.symno < symlim - 1) {
				extra_nm = nm;
				extra_sym = 1;
				break;
			}

			blocklists[level - 1]->bl_blocks = blocklists[level];
			
			--level;
			p_varlist = &varlists[level];
			p_aelist = &aelists[level];
			p_tdlist = &tdlists[level];
			
			/*  FRAGILE CODE:
			 *
			 *  We subtract one from the addr_to_lnum() return value
			 *  here.  This is based only on looking at the results
			 *  on some code - not on any documentation.
			 */
			blocklists[level]->bl_end_lnum = lnum - 1;

			/*  FRAGILE CODE:
			 *
			 *  More heuristics to try and cope with bogus block
			 *  start lines.  If a block starts on the same line
			 *  as the following block at the same level, back up
			 *  one then go back to the next previous curly.
			 *
			 *  Note that bl_next points the lexically *previous*
			 *  block, because we push blocks as we find them.
			 */
			bl = blocklists[level]->bl_blocks; 
			if (bl == NULL)
				break;

			next_lnum = bl->bl_start_lnum + 1;
			for (; bl != NULL; bl = bl->bl_next) {
				if (next_lnum != 0 && bl->bl_start_lnum >= next_lnum)
					bl->bl_start_lnum =
					  backup_lnum_to_curly(f, f->fu_fil,
							       next_lnum - 1);
				next_lnum = bl->bl_start_lnum;
			}
			if (blocklists[level]->bl_start_lnum >= next_lnum)
				blocklists[level]->bl_start_lnum =
					  backup_lnum_to_curly(f, f->fu_fil, next_lnum - 1);

			break;

		case N_PSYM:
		case N_LSYM:
		case N_STSYM:
		case N_RSYM:
		case N_FUN:
		case N_LCSYM:
#ifdef N_ROSYM
		case N_ROSYM:
#endif
#ifdef N_ISYM					/* RCB, for SC5 */
		case N_ISYM:
		case N_ESYM:
#endif
			s = symstring(symrec.symio, symrec.symno);

			/*  The Sun 4 C compiler emits symbols for
			 *  what look like compiler-created temporary
			 *  variables.  These have names like '#tmp0'.
			 *  So skip any names that start with '#'.
			 */
			if (*s == '#' || *s == 0) /* RGA for gcc 2.8.0 */
				break;

			if (stf->stf_language == LANG_CC &&
			    (*s == '?' || *s == '('))
			  break;

		        if (stf->stf_compiler_type == CT_CC)
			  while (*s == '.')
			    ++s;

			name = parse_name (&symrec, &s, ap,
			                   nm.n_type == N_FUN ? 2 : 1,
					   stf->stf_compiler_type,
					   IS_FORTRAN(stf->stf_language));
			if (*s == ' ') /* RGA for C++  */
				break;
			field_scheck(&symrec, &s, ':');
			type = Class(stf, &symrec, &s, &class);

			if ((class == CL_FUNC || class == CL_LFUNC) &&
			    (symrec.symno == AO_FSYMDATA(f)->fs_symno) &&
			    (strcmp(name, f->fu_demangled_name) == 0)) {
				f->fu_type = type;
				break;
			}

			/*  Class() does the wrong thing with foo:P() (see
			 *  the comment there.  So skip N_FUN syms.
			 *
			 *  TODO: Have compiler_type field so we can fix
			 *  Class() and various other routines.
			 */
			if (nm.n_type == N_FUN)
				break;

			switch (class) {
			case CL_ARG:
			case CL_AUTO:
				if (nm.n_type == N_RSYM)
					class = CL_REG;
				skipthis = FALSE;
				break;
			case CL_REG:
			case CL_LSTAT:
			case CL_REF:
				skipthis = FALSE;
				break;
			case CL_TYPEDEF:
				type->ty_typedef->td_next = *p_tdlist;
				*p_tdlist = type->ty_typedef;
				skipthis = TRUE;
				break;
			case CL_TAGNAME:
				if (type->ty_aggr_or_enum) /* RGA added */
				{
				  type->ty_aggr_or_enum->ae_next = *p_aelist;
				  *p_aelist = type->ty_aggr_or_enum;
				}
				skipthis = TRUE;
				break;
			default:
				skipthis = TRUE;
				break;
			}
			
			/*  Gcc without the -O flag emits duplicate symbols
			 *  for parameters.  So if we are doing the outermost
			 *  block (where parameters occur) discard this variable
			 *  if we already have one with same name and address.
			 */
			if (level == 0 && (class == CL_AUTO || class == CL_ARG)) {
				for (v = *p_varlist; v != NULL;
							      v = v->va_next) {
					if (v->va_addr == nm.n_value &&
					      (v->va_class == CL_ARG ||
					       v->va_class == CL_REG) &&
					    strcmp(v->va_name, name) == 0) {
						skipthis = TRUE;
						break;
					}
				}
			}

			if (skipthis)
				break;

#ifdef ARCH_CLIPPER
			if (class == CL_REG && nm.n_value >= 16)
				nm.n_value = (nm.n_value - 16) * 2 + 16;
#endif

			/*  Attempt to work around bug in epcf90 symbol table
			 *  output.
			 */
			if (IS_FORTRAN(stf->stf_language) &&
			    class == CL_AUTO && (int)nm.n_value > 0x10000 &&
			    type->ty_code == DT_PTR_TO &&
			    type->ty_base->ty_code == DT_ARRAY_OF &&
			    type->ty_base->ty_base->ty_code == TY_CHARACTER) {
				class = CL_LSTAT;
			}
                        
                        if (IS_FORTRAN(stf->stf_language) &&
			    class == CL_AUTO &&
			    type->ty_code == DT_PTR_TO &&
                            type->ty_base->ty_code == DT_PTR_TO) {
				type = type->ty_base;
                                class = CL_REF;
                        }
			
			addr = nm.n_value;
#ifdef AO_ELF
			if (static_addrs_relative(stf->stf_compiler_type)) {
			  int ar;
			  snlist_t *sn;
				
			  switch (nm.n_type) {
			  case N_STSYM:
			    ar = AR_DATA;
			    break;
			  case N_LCSYM:
			    ar = AR_BSS;
			    break;
			  case N_ROSYM:
			    ar = AR_RODATA;
			    break;
			  default:
			    ar = -1;
			    break;
			  }

			  if (ar != -1 &&
			      symtab_name_to_sn
			      (stf->stf_snlist, name, 
			       stf->stf_global_prefix, f->fu_name,
			       class == CL_LSTAT &&
			       stf->stf_compiler_type == CT_CC &&
			       f->fu_fil->fi_language == LANG_CC, &sn)) {
			    name = sn->sn_name;
			    /* RGA for SC4 */
			    if (!addr &&
				stf->stf_compiler_type == CT_CC &&
				f->fu_fil->fi_language == LANG_CC) 
			      addr = lookup_global_addr
				(f->fu_fil->fi_symtab, sn->sn_symtab_name);
			    else
			      addr += stf->stf_range[ar].base;
			  }
			}
#endif
			
			if (class == CL_LSTAT &&
			    !(stf->stf_compiler_type == CT_CC &&
			      f->fu_fil->fi_language == LANG_CC))
			  addr += ast->st_base_address;

			v = ci_make_var(ap, name, class, type, addr);
			v->va_language = stf->stf_language;

			v->va_next = *p_varlist;
			*p_varlist = v;

#if IS_BIG_ENDIAN && defined(FIX_SHORT_PARAMS)
			if (v->va_class == CL_ARG) {
				switch (v->va_type->ty_code) {
				case TY_USHORT:
				case TY_SHORT:
					v->va_addr += sizeof(int)-sizeof(short);
					break;
				case TY_UCHAR:
				case TY_CHAR:
					v->va_addr += sizeof(int)-sizeof(char);
					break;
				default:
					/* No adjustment */
					break;
				}
			}
#endif
#ifdef ARCH_SUN4
			/*  Special case code for structures passed by value.
			 *  The Sun 4 C compiler copies these to the stack and
			 *  passes a pointer to this copy to the called
			 *  function.
			 *
			 *  In the symbol table for the called function we get
			 *  a normal symbol for the variable, followed by a
			 *  register symbol for it.  We spot the situation by
			 *  looking for register vars that are too big to go
			 *  in a register.
			 *
			 *  We push an extra level of indirection on to the
			 *  variable's type, and lose the symbol table entry
			 *  before this one if it refers to a variable of the
			 *  same name.
			 *
			 *  BUG: What if the passed pointer doesn't go in a
			 *       register?
			 */
			if (class == CL_REG &&
			    type != NULL && /* RGA added for gcc 2.8.0 */
			    type->ty_code != TY_FLOAT &&
			    type->ty_code != TY_DOUBLE &&
#if HAVE_LONG_LONG
			    type->ty_code != TY_LONGLONG &&	/* RCB */
			    type->ty_code != TY_ULONGLONG &&	/* RCB */
#endif
		            typesize(type) > sizeof(long)) {
				type = ci_make_type(ap, DT_PTR_TO);
				type->ty_base = v->va_type;
				v->va_type = type;
				v->va_flags |= VA_HIDE_PTR;

				if (v->va_next != NULL &&
				    strcmp(name, v->va_next->va_name) == 0)
					v->va_next = v->va_next->va_next;
			}
#endif
			break;
		}
	}

	if (level != 0)
	{
	  /* RCB: SC4 does this for optimized code.  When this
	  ** happens any data about local variables tends to
	  ** be wrong anyway, so throw it out. */
	  sane_bracs = FALSE;
	}

	/*  With some compilers, functions with parameters but no local
	 *  variables don't get an LBRAC symbol.  If this is the case
	 *  for this function, build a level 0 block for it.
	 */

				/* RGA no blocking for C++ */
/*	if (stf->stf_language == LANG_CC &&*/
/*	    (varlists[0] != NULL || varlists[1] != NULL))*/
/*	    build_level_0_block = TRUE;*/

 	if (build_level_0_block ||
	    (!sane_bracs && blocklists[0] == NULL && varlists[0] != NULL)) {
	  	bl = ci_make_block(ap, f->fu_fil->fi_block);
		bl->bl_start_lnum = (FU_LNOS(f) == NULL) ? 1 : f->fu__lnos->ln_num;
		bl->bl_end_lnum = f->fu_max_lnum;
		bl->bl_vars = varlists[0];
		bl->bl_typedefs = tdlists[0];
		bl->bl_aggr_or_enum_defs = aelists[0];
		blocklists[0] = bl;
	}

	f->fu__blocks = sane_bracs ? bl : blocklists[0];
	f->fu_flags |= FU_DONE_BLOCKS;

	/*  If a function has parameters, no outer level locals but some
	 *  inner locals, we can get a situation where the inner locals
	 *  are noted at one level too low.  Fix this if necessary.
	 */
	if (f->fu__blocks != NULL && f->fu__blocks->bl_blocks == NULL &&
						f->fu__blocks->bl_next != NULL) {
		f->fu__blocks->bl_blocks = f->fu__blocks->bl_next;
		f->fu__blocks->bl_next = NULL;
	}
	
	fix_params(f);
	errf("\bReading symbols of `%s'... done", f->fu_demangled_name);
	errf_set_ofunc(oldf);
#if WANT_DEBUG
dump_header(f->fu_demangled_name);
dump_fil_t(f->fu_fil, FALSE, FALSE);
dump_block_t(f->fu__blocks, 0, TRUE);
dump_trailer();
#endif
	return f->fu__blocks;
}

/*  NOTE: this routine is not used on the DECstation 3100.
 */
static void
get_lno_symrange(f, p_start, p_lim)
func_t *f;
int *p_start, *p_lim;
{
	stf_t *stf;

	if (f->fu_fil == NULL)
		panic("NULL fil in gls");
	stf = AO_FIDATA(f->fu_fil);

	if (stf->stf_flags & STF_LNOS_PRECEDE_FUNCS) {
		funclist_t *prev, *fl;
		
		prev = NULL;
		for (fl = f->fu_fil->fi_funclist; fl->fl_func != f;
							    fl = fl->fl_next) {
			if (fl == NULL)
				panic("prev botch in gfl");
			prev = fl;
		}

		if (prev != NULL)
			*p_start = AO_FSYMDATA(prev->fl_func)->fs_symno;
		else
			*p_start = stf->stf_symno;
		*p_lim = AO_FSYMDATA(f)->fs_symno;
	}
	else {
		*p_start = AO_FSYMDATA(f)->fs_symno;
		*p_lim = AO_FSYMDATA(f)->fs_symlim;
	}
}

lno_t *
ao_get_fu_lnos(f)
func_t *f;
{
  lno_t dummy_lno, *last;
  lno_t *lno = NULL;
  ao_stdata_t *ast;
  int max_lnum;
  symio_t *symio;
  taddr_t min_addr;
  fil_t *fil, *initial_fil;
  int symno, symno_start, symno_lim, bogus_last_lno;
  unsigned sline_offset, save_sline_offset;
  unsigned short u;
  Compiler_type compiler;

  if (f->fu_flags & FU_DONE_LNOS)
    return f->fu__lnos;

  ast = AO_STDATA(f->fu_symtab);

#ifdef AO_ELF
  if (!elf_scan_then_setup_symio(f->fu_fil, &symio))
    return NULL;
#else
  symio = ast->st_text_symio;
#endif
          
  /*  First grab all the N_SLINE symbols in the function - we possibly
   *  discard some of them later.
   */
  last = &dummy_lno;
  max_lnum = 0;
  get_lno_symrange(f, &symno_start, &symno_lim);

  if (st_get_min_bpt_addr(f, &min_addr) != 0)
    min_addr = f->fu_addr;

  fil = initial_fil = AO_FSYMDATA(f)->fs_initial_lno_fil;
  compiler = ao_compiler(fil, FALSE, CT_UNKNOWN);
  sline_offset = AO_FSYMDATA(f)->fs_initial_sline_offset;
  save_sline_offset = sline_offset;

  for (symno = symno_start; symno < symno_lim; ++symno) {
    nlist_t nm;
    taddr_t addr;
    const char *name;

    getsym(symio, symno, &nm);

    switch (nm.n_type) {
    case N_SLINE:
      break;

#ifdef N_XLINE
    case N_XLINE:
      sline_offset = nm.n_desc << 16;
      save_sline_offset = sline_offset;
      break;
#endif

    case N_SOL:
      name = symstring(symio, symno);

      if (name && *name == '.' && *(name+1) == '/') /* RGA */
	name += 2;

      if (!find_sol_fil(f->fu_symtab->st_sfiles,
			f->fu_fil->fi_path_hint, name,
			&fil)) {
	/*    RGA                     errf("Warning: can't find file entry for %s",*/
	/*                                    name);*/
	fil = f->fu_fil;
      }

      if (fil == initial_fil)
	sline_offset = save_sline_offset;
      else
	sline_offset = 0;
      /* RGA gcc puts out non zero values here which upset */
      /* the addr of the first lno */
      if (compiler == CT_GNU_CC)
	nm.n_value = 0;
      break;

    default:
      continue;
    }
                                    
#ifdef AO_ELF
    addr = f->fu_addr + nm.n_value;
#else
    addr = nm.n_value + ast->st_base_address;
#endif

    if ( lno && addr < min_addr)
      continue;
 
    /*              if (compiler == CT_GNU_CC && fil != f->fu_fil)*/
    /*                      continue;*/

    /* RGA use unsigned - for files over 32k ! */
    u = (unsigned)nm.n_desc;
    bogus_last_lno = compiler == CT_CLCC && lno && (max_lnum > (int)u | sline_offset);
    if (bogus_last_lno)
      max_lnum = 0;		/* RGA reset */
    else
      lno = (lno_t *)alloc(f->fu_symtab->st_apool,sizeof(lno_t));
    lno->ln_fil = fil;
    lno->ln_num = u | sline_offset;
    lno->ln_col = -1;
    lno->ln_addr = addr;

    if (lno->ln_num > max_lnum)
      max_lnum = lno->ln_num;

    if (!bogus_last_lno)
    {
      last->ln_next = lno;
      last = lno;
    }
  }

  last->ln_next = NULL;

  /* Get the first record.  Unless it was the only
  ** record, skip to the first one after the function prelude
  */
  lno = dummy_lno.ln_next;
  if  (   lno
          && lno->ln_addr < min_addr
	  && lno->ln_next )
    lno = lno->ln_next;
  f->fu__lnos = lno;
        
#if !defined(AO_ELF)
  fixup_initial_lnos(f);
#endif

  f->fu_max_lnum = max_lnum;
  f->fu_flags |= FU_DONE_LNOS;
#if WANT_DEBUG
dump_header(f->fu_demangled_name);
dump_fil_t(f->fu_fil, FALSE, FALSE);
dump_lno_t(f->fu__lnos, TRUE);
dump_trailer();
#endif
  return f->fu__lnos;
}
#endif /* !ST_TE */

#if !defined(AO_ELF)
/*  FRAGILE CODE
 *
 *  Some compilers helpfully put out junk symbols at the start
 *  of functions.  We try to get rid of these so that add bpt
 *  puts the bpt at the line the user expects.
 *
 *  The rules for how many symbol to junk were guessed at by
 *  examining symbol tables.  To put it mildly, this is not
 *  a desirable way to do things.  This stuff is almost the
 *  most fragile code in the debugger - it is liable to break
 *  with every new release.
 *
 *  Of course, it's too much to ask that the first SLINE symbol
 *  for a function should correspond to the first line of the
 *  function, or that any of this stuff should be documented ...
 *
 *  Notice that we don't free lnos as we discard them.  This is
 *  because they were allocated via alloc().
 */
static void
fixup_initial_lnos(f)
func_t *f;
{
	lno_t *lno, *first;
	
	switch(f->fu_language) {
	case LANG_C:
	case LANG_CC:
		lno = f->fu__lnos;

#ifdef OS_ULTRIX
		if (lno != NULL)
			lno = lno->ln_next;
#else
		/*  Skip any symbols whose address is the same as the
		 *  function address.
		 */
		for (; lno != NULL && lno->ln_addr == f->fu_addr; lno = lno->ln_next)
			;
#endif

		/*  Skip all but the last of a set of lnos with the same
		 *  address.
		 */
		if (lno != NULL) {
			taddr_t base_addr;
			lno_t *next;

			base_addr = lno->ln_addr;
			for (;;) {
				next = lno->ln_next;
				if (next == NULL || next->ln_addr != base_addr)
					break;
				lno = next;
			}
		}

		f->fu__lnos = lno;
		break;
		
	case LANG_F77:
		/*  The f77 compiler on Ultrix 2.2 and 4.3BSD generates
		 *  lots of N_SLINE symbols with the function address as
		 *  the address.  Try rejecting all but the last of these
		 *  symbols.
		 */
		first = NULL;
		for (lno = f->fu__lnos; lno != NULL; lno = lno->ln_next)
			if (lno->ln_addr == f->fu_addr)
				first = lno;
		
		if (first != NULL) {
			for (lno = f->fu__lnos; lno != first; lno = lno->ln_next)
				;
			f->fu__lnos = first;
		}
		
		break;
	case LANG_F90:
	case LANG_UNKNOWN:
		break;
	default:
		panic("unknown language in gfl");
	}
}
#endif /* !AO_ELF */

#endif /* AO_TARGET */

Compiler_type
ao_compiler(fil, compiler_found, compiler)
fil_t *fil;
bool compiler_found;
Compiler_type compiler;
{
    stf_t *stf;
    static Compiler_type likely = CT_UNKNOWN;

    if (compiler_found == TRUE)
    {
	likely = compiler;
	return likely;
    }

    if (fil)
    {
	if (!(fil->fi_flags & FI_FOUND_COMPILER))
	{
	    get_fi_compiler(fil);
	    fil->fi_flags |= FI_FOUND_COMPILER;
	}

	stf = AO_FIDATA(fil);
	if (stf->stf_compiler_type != CT_UNKNOWN)  /* RCB */
	    likely = stf->stf_compiler_type;
    }
    return likely;
}

void set_compiler_version(double ver)
{
    if ( ver > compiler_version)
	compiler_version = ver;
}
double get_compiler_version()
{
    return compiler_version;
}
