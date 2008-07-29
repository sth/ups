/* ao_symscan.c - do initial symbol table scan */

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

/* @(#)ao_symscan.c	1.3 20 Jun 1995 (UKC) */
char ups_ao_symscan_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#ifdef AO_TARGET

#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#ifdef AO_ELF
#	include "elfstab.h"
#else
#	include <a.out.h>
#	ifndef OS_RISCOS
#		include <stab.h>
#	endif
#endif

#include <local/ukcprog.h>
#include <mtrprog/hash.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "st.h"
#include "ao_syms.h"
#include "ao_symload.h"
#include "ao_symscan.h"
#include "ao_symread.h"
#include "ao_elfsym.h"
#include "ao_symcb.h"
#include "util.h"
#include "state.h"  /* RCB: for demangling_enabled() */

#if HAVE_CPLUS_DEMANGLE
#include <demangle.h>
#if HAVE_CPLUS_DEMANGLE_NORET
#define sunpro_cplus_demangle cplus_demangle_noret
#else
#define sunpro_cplus_demangle cplus_demangle
#endif
#endif

/* Options passed to gnu_cplus_demangle (in 2nd parameter). */

#define DMGL_NO_OPTS	0		/* For readability... */
#define DMGL_PARAMS	(1 << 0)	/* Include function args */
#define DMGL_ANSI	(1 << 1)	/* Include const, volatile, etc */

#define DMGL_AUTO	(1 << 8)
#define DMGL_GNU	(1 << 9)
#define DMGL_LUCID	(1 << 10)
#define DMGL_ARM	(1 << 11)
#define DMGL_HP		(1 << 12)
#define DMGL_EDG	(1 << 13)
/* If none of these are set, use 'current_demangling_style' as the default. */
#define DMGL_STYLE_MASK (DMGL_AUTO|DMGL_GNU|DMGL_LUCID|DMGL_ARM)

extern char *gnu_cplus_demangle PROTO((const char *mangled, int options));

static hf_t *lookup_hf PROTO((hf_t *headers, int id));
static void build_symset PROTO((char *aout_symset, char *ofile_symset));
static bool is_fortran_void PROTO((const char *cptr));
static void demangle_g77_name PROTO((char *name, int func));
static void adjust_fil_vars_addr_base PROTO((fil_t *flist, long delta));
static void adjust_funcs_addr_base PROTO((func_t *funclist, long delta));
static fil_t *add_sol_fil PROTO((stf_t *stf, alloc_pool_t *ap, fil_t *sfiles,
				 fil_t *sofil, const char *name));
#ifdef AO_ELF
static func_t *note_ofile_function PROTO((alloc_pool_t *ap,
					  alloc_pool_t *tmp_ap, stf_t *stf,
					  const char *path,
					  fil_t *solfil, unsigned sline_offset,
					  const char *namestr, int symno,
					  funclist_t** p_fl));
#endif

void display_message PROTO((const char *mesg));

char
bump_str(sr, p_s)
Symrec *sr;
const char **p_s;
{
  char c;

  if (**p_s == '\\')
    *p_s = get_cont_symstring(sr);
  c = **p_s;
  (*p_s)++;
  if (**p_s == '\\')
    *p_s = get_cont_symstring(sr);
  return c;
}

#ifndef ST_TE
/*  Deduce language of source file from name.
 *  *.c is C, *.f is f77, *.f90 is f90, anything else is unknown.
 *
 *  BUG: dubious way of getting language.
 */
language_t
srctype(name)
const char *name;
{
	char *suf;

	if ((suf = strrchr(name, '.')) == NULL)
		return LANG_UNKNOWN;
	else
		++suf;

	if (strcmp(suf, "c") == 0)
		return LANG_C;

	if (strcmp(suf, "f") == 0 || strcmp(suf, "F") == 0)
		return LANG_F77;

	if (strcmp(suf, "f90") == 0 || strcmp(suf, "F90") == 0)
		return LANG_F90;

	if (strcmp(suf, "cc") == 0 || strcmp(suf, "C") == 0 ||
	    strcmp(suf, "CC") == 0 || strcmp(suf, "c++") == 0 ||
	    strcmp(suf, "cpp") == 0 || strcmp(suf, "cxx") == 0 ||
	    strcmp(suf, "H") == 0 || strcmp(suf, "hh") == 0)
	  return LANG_CC;	

	return LANG_UNKNOWN;
}

/*  Called when we have finished with an stf structure.  Create space
 *  for the map and copy the stuff to it.
 */
void
wrapup_stf(stf, orig_fmap, mapsize)
stf_t *stf;
hf_t **orig_fmap;
int mapsize;
{
	hf_t **fmap;
	int i;

	if (mapsize == 0)
		panic("mapsize 0 in wrapup_stf");

	fmap = (hf_t **)alloc(stf->stf_symtab->st_apool,
			      mapsize * sizeof(hf_t *));
	memcpy((char *)fmap, (char *)orig_fmap, mapsize * sizeof(hf_t *));

	for (i = 0; i < mapsize; ++i) {
		stf_t *hstf;

		hstf = fmap[i]->hf_stf;

		if (hstf->stf_fmap == NULL) {
			hstf->stf_fmap = fmap;
			hstf->stf_mapsize = mapsize;
		}
	}
}

void
reverse_stf_funclist(stf)
stf_t *stf;
{
	funclist_t *fl, *next, *newlist;

	newlist = NULL;
	for (fl = stf->stf_fil->fi_funclist; fl != NULL; fl = next) {
		next = fl->fl_next;
		fl->fl_next = newlist;
		newlist = fl;
	}

	stf->stf_fil->fi_funclist = newlist;
}

/*  Look up the header file entry with id id in the headers list headers.
 *
 *  Used when we find a N_EXCL symbol meaning a header file excluded
 *  because it has already been encountered.
 */
static hf_t *
lookup_hf(headers, id)
hf_t *headers;
int id;
{
	hf_t *hf;

	for (hf = headers; hf != NULL; hf = hf->hf_next)
		if (hf->hf_id == id)
			return hf;
	panic("id not found in lookup_hf");
	/* NOTREACHED */
	return NULL;	/* to keep gcc happy */

}

/*  Parse a name (NAME in the grammar).
 *  If save is non zero, return a pointer to a saved copy of the name,
 *  otherwise return NULL.  Names are saved via an alloc() on ap.
 *
 *  (IDE) added 'is_fortran' to get correct names generated by GNU Fortran
 *  (g77) in the typedefs it generates.
 */
const char *
parse_name(sr, p_s, ap, func, compiler, is_fortran)
Symrec *sr;
const char **p_s;
alloc_pool_t *ap;
int func;
Compiler_type compiler;
bool is_fortran;
{
	register const char *s, *sub;
	char *name;
	register int dots = 0;
/* RCB ADD: */
        int in_template = 0;
	size_t len = 0;

	s = *p_s;


	/*  Gcc 2.3.3 seems to put out spaces in names, so skip them.
	 */
	while (isspace(*s))
		++s;


	/*  The test for ':' is needed because gcc 2.3.1 puts out symbol
	 *  table entries containing empty names (i.e. nothing before the
	 *  ':') for unnamed enums.  SunOS f77 emits `block.data' as a
	 *  symbol, so we allow dots in names.  I don't remember why `$'
	 *  is allowed (Fortran ?).
	 */
	/*  RGA The test for '?' is needed because Pure-link puts out bad */ 
	/*  names apparently */
	if (*s != ':' && *s != '?' && !isalpha(*s) && *s != '_' && *s != '$' && *s != '.')
		panic("bad name in parse_name");

	while (    in_template
		|| isalnum(*s)
		|| *s == '_'
		|| *s == '$'
		|| *s == '.'
		|| *s == '<'    /*  RCB, for C++ templates */
		|| *s == '>'
		|| *s == '\\'
		|| (*s == ' ' && is_fortran)
		|| (*s == '*' && is_fortran)
	)
	{
	  len++;
	  if ( *s == '<')
	    in_template++;
	  else if ( *s == '>')
	    in_template--;
	  else if (*s == '.')
	    dots++;
	  else if ( *s == '\\')
	  {
	      char *s1, *s2;
	      int new_len;

	      len--;
	      s1 = (char *)get_cont_symstring(sr);
	      new_len = strlen(s1);
	      s2 = alloc(ap, s - *p_s + new_len + 1);
	      memcpy(s2, *p_s, len);
	      *p_s = s2;
	      memcpy(s2+len, s1, new_len);
	      s2[len+new_len] = 0;
	      s = s2+len-1;
	  }
	  ++s;
	}

	/* RGA SC4 static data is of the form $prefix.damangled_func_name.static_data */
	/* RCB gnu static members may be of the form mangled_class_name.name
	*/
	if (compiler != CT_GNU_CC && dots )
	  for (sub = s; sub != *p_s; sub--)
	    if (*sub == '.')
	    {
	      sub++;
	      len = s - sub;
	      *p_s = sub;
	      break;
	    }

	if (ap != NULL)
	{
	  demangle_name((char *)*p_s, len, ap, &name, func == 1, compiler);
	  len = strlen(name);
	}
	else 
	{
	  name = NULL;
	  len = 0;
	}

	if (is_fortran
	    && demangling_enabled(0, 0)
	    && func == 1
	    && compiler == CT_GNU_CC)
	{
	  demangle_g77_name(name, func);
	}

	*p_s = s;
	return name;
}

static void
demangle_g77_name(name, func)
char *name;
int func;	/* 0 => function, 1 => variable */
{
    int len;
    char *u;

    if (name == NULL)
	return;
    if ((u = strchr(name, '_')) == NULL)
	return;
    len = strlen (name);
    if (len > 3 && strcmp (name+len-2,"__") == 0)
    {
	/* Remove double underscore : functions - always,
	   variables - if name contains an underscore */
	if ((func == 0) || (u < name+len-2))
	    *(name+len-2) = '\0';
    }
    else if (*(name+len-1) == '_')
    {
	/* GNU f77 may add trailing '_' to globals. */
	*(name+len-1) = '\0';
    }
}

/*  Return a pointer to a munged saved copy of fname.  Ext is true
 *  if name is an external symbol generated by the linker.  These
 *  symbols are expected to have an underscore prepended - if it
 *  is there it is stripped, otherwise the returned name is
 *  enclosed in brackets (e.g. "[name]").
 *
 *  If modsepc is not NUL, and it occurs in the name, we split the name
 *  into a module name and function name at the seperator character.
 *
 *  We use alloc() to get the space.
 *
 *  NOTE: this routine is not used on the DECstation 3100.
 */
void
parse_fname(ap, name, modsepc, ext, p_modname, p_fname)
alloc_pool_t *ap;
const char *name;
int modsepc;
bool ext;
const char **p_modname, **p_fname;
{
	const char *modname;
	char *fname;

	if (ext && *name != '_') {
		modname = NULL;
		fname = alloc(ap, strlen(name) + 3);
		sprintf(fname, "[%s]", name);
	}
	else {
		const char *cptr;
		char *pos;
		size_t len;

		if (ext)
			name++;

		for (cptr = name; *cptr != '\0' && *cptr != ':'; cptr++)
			;

		len = cptr - name;
		fname = alloc(ap, len + 1);
		memcpy(fname, name, len);
		fname[len] = '\0';

		if (modsepc == '\0' || (pos = strchr(fname, modsepc)) == NULL) {
			modname = NULL;
		}
		else {
			modname = fname;
			*pos++ = '\0';
			fname = pos;
		}
	}

	*p_modname = modname;
	*p_fname = fname;
}
#endif /* !ST_TE */

fsyminfo_t *
make_fsyminfo(ap, symno)
alloc_pool_t *ap;
int symno;
{
	fsyminfo_t *fs;

	fs = (fsyminfo_t *)alloc(ap, sizeof(fsyminfo_t));
	fs->fs_initial_lno_fil = NULL;
	fs->fs_initial_sline_offset = 0;
	fs->fs_symno = symno;
	fs->fs_cblist = 0;
#if WANT_DWARF
	fs->fs_low_pc = 0;
	fs->fs_high_pc = 0;
	fs->fs_frame_base = NULL;
#endif
	return fs;
}

/*  Allocate a new stf_t structure with alloc() and fill in the fields.
 */
stf_t *
make_stf(ap, name, st, symno, language, addr)
alloc_pool_t *ap;
const char *name;
symtab_t *st;
int symno;
language_t language;
taddr_t addr;
{
#ifdef AO_ELF
	int ar;
#endif
	stf_t *stf;

	stf = (stf_t *)alloc(ap, sizeof(stf_t));
	stf->stf_name = name;
	stf->stf_language = language;
	stf->stf_compiler_type = ao_compiler(NULL, FALSE, CT_UNKNOWN);
	stf->stf_symtab = st;
	stf->stf_symno = symno;
	stf->stf_addr = addr;
	stf->stf_flags = 0;
#ifdef AO_ELF
	stf->stf_objpath_hint = NULL;
	stf->stf_objname = NULL;
	stf->stf_obj_mtime = (time_t)0;
	stf->stf_global_prefix = NULL;
	stf->stf_symio = NULL;
#endif
#ifdef ARCH_CLIPPER
	stf->stf_addrlist = NULL;
#endif
#ifdef ST_TE
#else
	stf->stf_snlist = NULL;
	stf->stf_ftypes = NULL;
	stf->stf_fmap = NULL;
	stf->stf_mapsize = 0;
#endif
#if WANT_DWARF
	stf->stf_dw_dbg = NULL;
	stf->stf_cu_hdr_offset = (off_t)0;
	stf->stf_cu_die_offset = (off_t)0;
	stf->stf_dtypes = NULL;
	stf->stf_last_dt = NULL;
	stf->stf_parent = NULL;
#endif
#ifdef AO_ELF
	stf->stf_objpath_hint = NULL;
	stf->stf_objname = NULL;
	stf->stf_obj_mtime = 0;
	stf->stf_global_prefix = NULL;
	stf->stf_symio = NULL;

	for (ar = 0; ar < AR_NTYPES; ++ar) {
		stf->stf_range[ar].base = 0;
		stf->stf_range[ar].lim = 0;
	}
#endif
	return stf;
}

/*  Allocate a new fil_t structure with alloc() and fill in the fields.
 */
fil_t *
ao_make_fil(stf, parblock, path_hint, next)
stf_t *stf;
block_t *parblock;
const char *path_hint;
fil_t *next;
{
	fil_t *fil;
	alloc_pool_t *ap;

	ap = stf->stf_symtab->st_apool;

	fil = ci_make_fil(ap, stf->stf_name, (char *)stf,
			  ci_make_block(ap, parblock), next);

	fil->fi_path_hint = path_hint;
	fil->fi_source_path = (char *)path_hint;
	fil->fi_language = stf->stf_language;
	fil->fi_symtab = stf->stf_symtab;

	return fil;
}

bool
find_sol_fil(sfiles, path_hint, name, p_fil)
fil_t *sfiles;
const char *path_hint, *name;
fil_t **p_fil;
{
	fil_t *fil;
	bool abspath;

	abspath = *name == '/';

	for (fil = sfiles; fil != NULL; fil = fil->fi_next) {
	  if ((abspath || same_string(fil->fi_path_hint, path_hint)) &&
	      strcmp(fil->fi_name, name) == 0) {
			*p_fil = fil;
			return TRUE;
		}
	}

	return FALSE;
}

static fil_t *
add_sol_fil(stf, ap, sfiles, sofil, name)
stf_t *stf;			/* RGA */
alloc_pool_t *ap;
fil_t *sfiles, *sofil;
const char *name;
{
	fil_t *fil;
	const char *path_hint;

	path_hint = (*name != '/') ? sofil->fi_path_hint : NULL;

	if (find_sol_fil(sfiles, path_hint, name, &fil))
		return fil;

	fil = ci_make_fil(ap, alloc_strdup(ap, name), (char *)stf,
			  ci_make_block(ap, (block_t *)NULL), sofil->fi_next);

	fil->fi_path_hint = path_hint;
	fil->fi_source_path = (char *)path_hint;
	fil->fi_language = srctype(fil->fi_name_only);

	/*  The only thing these entries are used for is displaying
	 *  source code, but we need fi_symtab because things like
	 *  open_source_file() go via it to get an alloc pool.
	 */
	fil->fi_symtab = sofil->fi_symtab;

	fil->fi_flags |= FI_DONE_VARS | FI_DONE_TYPES;

	sofil->fi_next = fil;

	return fil;
}

#ifndef ST_TE
#define SYMSET_SIZE	256

/*  Build the map of interesting symbols for scan_symtab.  We could do this
 *  at compile time with some effort, but it's much less hassle to do it at
 *  run time like this.
 */
static void
build_symset(aout_symset, ofile_symset)
char *aout_symset, *ofile_symset;
{
	static int common_syms[] = {
		N_BCOMM, N_STSYM, N_GSYM, N_LCSYM, N_FUN, N_SOL,
#ifdef N_BINCL
		N_BINCL, N_EXCL, N_EINCL,
#endif
#ifdef N_XLINE
		N_XLINE,
#endif
#ifdef AO_ELF
		N_ILDPAD,
		N_UNDF,
#endif
	};
	static int aout_only_syms[] = {
		N_SO,
		N_TEXT, N_TEXT | N_EXT, N_BSS | N_EXT, N_DATA | N_EXT,
		N_DATA,
#ifdef AO_ELF
		N_OPT,
#endif
#ifdef N_MAIN
		N_MAIN
#endif
	};
	int i;

	memset(ofile_symset, '\0', SYMSET_SIZE);
	for (i = 0; i < sizeof(common_syms) / sizeof(common_syms[0]); ++i)
		ofile_symset[common_syms[i]] = TRUE;

	memcpy(aout_symset, ofile_symset, SYMSET_SIZE);
	for (i = 0; i < sizeof(aout_only_syms) / sizeof(aout_only_syms[0]); ++i)
		aout_symset[aout_only_syms[i]] = TRUE;
}

bool
parse_number(stf, sr, p_s, p_val)
stf_t *stf;
Symrec *sr;
const char **p_s;
int *p_val;
{
	const char *s;
	int res;

	s = *p_s;

	if (*s == '\\')
	  s = get_cont_symstring(sr);

	if (!isdigit(*s))
		return FALSE;

	res = 0;
	if (stf->stf_compiler_type ==  CT_GNU_CC && s[-1] == '/' && s[-2] == ':')
	  bump_str(sr, &s);	/* RGA - g++ puts out these... */
	if (*s == '0') {
		bump_str(sr, &s);
		if (*s == 'x' || *s == 'X') {
			bump_str(sr, &s);
			for (;;) {
				if (isdigit(*s))
					res = res * 16 + bump_str(sr, &s) - '0';
				else if (*s >= 'A' && *s <= 'F')
					res = res * 16 + bump_str(sr, &s) - 'A' + 10;
				else if (*s >= 'a' && *s <= 'f')
					res = res * 16 + bump_str(sr, &s) - 'a' + 10;
				else
					break;
			}
		}
		else {
		    while (*s >= '0' && *s <= '7')
		      res = res * 8 + bump_str(sr, &s) - '0';
		}
	      }
	else {
		while (isdigit(*s))
		  res = res * 10 + bump_str(sr, &s) - '0';
	}
	if (*s == '\\')
	  s = get_cont_symstring(sr);
	*p_s = s;
	*p_val = res;
	return TRUE;
}

bool
parse_typenum(stf, sr, assume_paren, p_s, p_fnum, p_tnum)
stf_t *stf;
Symrec *sr;
bool assume_paren;
const char **p_s;
int *p_fnum, *p_tnum;
{
	const char *s;
	bool ok;

	s = *p_s;
	if (assume_paren == TRUE || *s == '(') {
	  if (assume_paren == FALSE)
		  bump_str(sr, &s);
		ok = (parse_number(stf, sr, &s, p_fnum) &&
		      bump_str(sr, &s) == ',' &&
		      parse_number(stf, sr, &s, p_tnum) &&
		      bump_str(sr, &s) == ')');
	}
	else {
		*p_fnum = 0;
		ok = parse_number(stf, sr, &s, p_tnum);
	}

	if (ok)
		*p_s = s;
	return ok;
}

bool
char_to_utypecode(c, p_typecode)
int c;
typecode_t *p_typecode;
{
	switch (c) {
	case 's':
		*p_typecode = TY_U_STRUCT;
		break;
	case 'u':
		*p_typecode = TY_U_UNION;
		break;
	case 'e':
		*p_typecode = TY_U_ENUM;
		break;
	default:
		return FALSE;
	}

	return TRUE;
}

/*  BUG: this is gross, and wrong to boot.  The number of basic types
 *       varies between f77 compilers.  See the comment about this
 *       in get_fi_vars().
 */
#define N_FIXED_FORTRAN_TYPES	9

bool
symtab_name_to_sn(snlist, name, prefix, fname, use_allnames, p_sn)
snlist_t *snlist;
const char *name;
const char *prefix;
const char *fname;
bool use_allnames;
snlist_t **p_sn;
{
  snlist_t *sn;
  const char *c;

  for (sn = snlist; sn != NULL; sn = sn->sn_next)
  {
    if (strcmp(sn->sn_name, name) == 0)
    {
      /* RGA for SC4, local statics of the same name in different
	 functions are distinguished by the prefix and mangled function
	 name in the sn_symtab_name.
	 */
      if (use_allnames == TRUE)
      {
	int prefix_len, fname_len;

	prefix_len = strlen(prefix);
	fname_len = strlen(fname);
	c = strchr(sn->sn_symtab_name, '.');
	c++;
	if (c)
	{
	  /* RGA FIXME: first few letters of prefix do not allway match the
	     global prefix, so offset by 3 until we get a better solution.
	     */
	  if ((prefix_len == c - sn->sn_symtab_name) &&
	      strncmp(sn->sn_symtab_name+3, prefix+3, prefix_len-3) == 0 &&
	      strncmp(c, fname, fname_len) == 0)
	  {
	    *p_sn = sn;
	    return TRUE;
	  }
	}
      }
      else
      {
	*p_sn = sn;
	return TRUE;
      }
    }
  }

  return FALSE;
}
bool
symtab_sym_name_to_sn(snlist, name, p_sn, justone)
snlist_t *snlist;
const char *name;
snlist_t **p_sn;
bool justone;
{
	const char* end = strchr(name, ':');
	int len = end ? end-name : strlen(name);

	snlist_t *sn;

	for (sn = snlist; sn != NULL; sn = sn->sn_next) {
          if (   strncmp(sn->sn_symtab_name, name, len) == 0
	      && strlen(sn->sn_symtab_name) == len) {
			*p_sn = sn;
			return TRUE;
		}
		if ( justone)
		   break;
	}

	return FALSE;
}

/*  Do a prescan of a symbol table.  We don't load in all of the symbol
 *  table on startup as that costs a lot in startup time for big symbol
 *  tables.  Instead we load enough to get us going (basically function
 *  and global variable names and addresses) and pull other bits in as
 *  needed.  This is a big win in normal use because the average debugger
 *  run touches only a small number of functions and global variables.
 *  Most of the symbol table is never read in at all.
 *
 *  The Sun C compiler has a system for reducing symbol table size by
 *  only including header file symbol table information once.  We have
 *  to do a fair amount of bookkeeping to resolve the header file
 *  references.
 *
 *  For most things that we load in this pass (like functions, global
 *  variables etc) we record the symbol number range of the object.
 *  This is so we can pull in more information when needed (e.g. the
 *  local variables of a function, the globals of a source file).
 */
void
scan_symtab(st, path, stf, p_flist, p_mainfunc_name)
symtab_t *st;
const char *path;
stf_t *stf;
func_t **p_flist;
const char **p_mainfunc_name;
{
    static int first_call = TRUE;
    static char aout_symset[SYMSET_SIZE], ofile_symset[SYMSET_SIZE];
    char *symset;
    snlist_t *sn;
    block_t *rootblock;
    fil_t *solfil;
    symio_t *symio;
    Symrec symrec;
    nlist_t nm;
    hf_t *headers, *hf;
#ifdef AO_ELF
    off_t file_offset, next_file_offset;
    const char* next_global_prefix = NULL;
#endif
    Compiler_type next_compiler = CT_UNKNOWN;
    bool          set_compiler = FALSE;
    fil_t        *last_fil = NULL;

    hf_t **fmap, **istack;
    ao_stdata_t *ast;
    func_t *curfunc, *flist;
    funclist_t* new_functlist;
    funclist_t** e_new_functlist;
    const char *name = 0, *mainfunc_name, *path_hint, *cptr;
    char *cptr2, *end;
    int symno;
    unsigned sline_offset;
    int isp, istack_size, mapsize, max_mapsize, nsyms, doing_header;
    alloc_pool_t *ap, *tmp_ap;
    bool seen_func, doing_ofile;
#ifdef OS_ULTRIX
    int last_data_symno, have_unused_datasym;
    nlist_t data_nm;
    const char *lastname;
#endif

    doing_ofile = stf != NULL;

    if (first_call) {
	build_symset(aout_symset, ofile_symset);
	first_call = FALSE;
    }

    ast = AO_STDATA(st);
    flist = NULL;
    curfunc = NULL;
    new_functlist = NULL;
    e_new_functlist = &new_functlist;
    rootblock = get_rootblock();
    doing_header = 0;
    headers = NULL;

#ifdef OS_ULTRIX
    /*  FRAGILE CODE
     *
     *  The Ultrix C compiler has a charming habit of correcting
     *  itself over symbol addresses.  A symbol table frequently
     *  has an N_DATA symbol with one address, followed soon
     *  after by an N_STSYM for the same symbol name with a slightly
     *  different address.  The N_DATA address is the correct one.
     *
     *  To cope with this, we remember the symbol number of N_DATA
     *  symbols, and correct the N_STSYM address if necessary.
     *
     *  The compiler also tends to put out N_DATA symbols immediately
     *  after N_STSYM symbols, but in these cases the addresses
     *  seem to always be the same.
     */
    have_unused_datasym = FALSE;
    last_data_symno = 0; /* to satisfy gcc */
#endif

#ifdef AO_ELF
    file_offset = next_file_offset = 0;
#endif

    max_mapsize = 32;
    fmap = (hf_t **) e_malloc(max_mapsize * sizeof(hf_t *));

    isp = 0;
    istack_size = 8;
    istack = (hf_t **) e_malloc(istack_size * sizeof(hf_t *));

    symno = -1;
    path_hint = NULL;
    seen_func = 0; /* to satisfy gcc */
    ap = st->st_apool;
    tmp_ap = alloc_create_pool();
    mainfunc_name = NULL;
    solfil = NULL;
    sline_offset = 0;

    if (doing_ofile) {
#ifdef AO_ELF
	symio = stf->stf_symio;
#else
	panic("doing_ofile set for non-ELF file in ss");
	symio = NULL;	/* to satisfy gcc */
#endif

	symset = ofile_symset;
	stf->stf_fnum = 0;
	fmap[0] = (hf_t *) alloc(ap, sizeof(hf_t));
	fmap[0]->hf_stf = stf;
	fmap[0]->hf_id = -1;
	fmap[0]->hf_next = NULL;
	mapsize = 1;
    }
    else {
	symset = aout_symset;
	symio = ast->st_text_symio;

	mapsize = 0; /* for lint */
    }

    nsyms = get_symio_nsyms(symio);

    for (symno = findsym(symio, symno + 1, &nm, symset);;
	 symno = next_unmarked_sym(symio, symno + 1)) {

	symno = findsym(symio, symno, &nm, symset);

	if (symno >= nsyms)
	    break;

	cptr2 = 0;
	switch(nm.n_type) {
	    const char *symtab_name;
	    language_t lang;
#ifdef AO_ELF
	    bool has_debug_syms;
#endif

#ifdef AO_ELF
	case N_ILDPAD:
	    next_file_offset += nm.n_value;
	    break;
	case N_UNDF:
	    file_offset = next_file_offset;
	    next_file_offset = file_offset + nm.n_value;

	    add_extra_string_offset(symio, symno, file_offset);
	    break;
#endif

#ifdef N_MAIN
	case N_MAIN:
	    mainfunc_name = symstring(symio, symno);
	    break;
#endif

	case N_SO:
	    solfil = NULL;
	    sline_offset = 0;

	    if (curfunc != NULL) {
		    AO_FSYMDATA(curfunc)->fs_symlim = symno;
		    curfunc = NULL;
	    }
	    if (stf != NULL)
		    stf->stf_symlim = symno;

	    name = symstring(symio, symno);

	    /*  4.0 cc puts paths ending in '/' just before
	     *  the source files that follow.
	     */
	    if (!name || *name == '\0' || name[strlen(name) - 1] == '/') {
		    path_hint = name;
		    break;
	    }

	    if (stf != NULL) {
		    wrapup_stf(stf, fmap, mapsize);
		    reverse_stf_funclist(stf);
	    }

#ifndef AO_ELF
	    if (strcmp(name, "libg.s") == 0) {
		    stf = NULL;
		    break;
	    }
#endif

	    if (name && *name == '.' && *(name+1) == '/') /* RGA */
		    name += 2;
	    lang = srctype(name);
#ifdef OS_SUNOS_4
	    /*  F77 files can be compiled with the f90 compiler,
	     *  so try and work out from the symbol table which
	     *  was used.
	     */
	    if (IS_FORTRAN(lang)) {
		const char *s;

		s = symstring(symio, symno + 1);

		if (strncmp(s, "byte:", 5) == 0) {
		    lang = LANG_F90;
		}
		else if (strncmp(s, "integer*2:", 10) == 0) {
		    lang = LANG_F77;
		}
	    }
#endif

	    stf = make_stf(ap, name, st, symno, lang, nm.n_value);
#ifdef AO_ELF
	    stf->stf_symio = symio;
#endif
	    last_fil = st->st_sfiles = ao_make_fil(stf, rootblock, path_hint, st->st_sfiles);
#ifndef OS_LINUX
	    if (!path_hint && lang == LANG_CC)
		    /* RGA g++ puts out empty names so hide such files  */
		    st->st_sfiles->fi_flags |= FI_HIDE_ENTRY;
#endif

	    set_compiler = next_compiler != CT_UNKNOWN;

	    /* Assumes: N_OPT before N_SO */
	    if ( set_compiler )
	    {
		    stf->stf_compiler_type = next_compiler;
		    st->st_sfiles->fi_flags |= FI_FOUND_COMPILER;
		    next_compiler = CT_UNKNOWN;
#ifdef AO_ELF
		    stf->stf_global_prefix = next_global_prefix;
		    next_global_prefix = NULL;
#endif
	    }

	    doing_header = 0;
	    stf->stf_fil = st->st_sfiles;

	    if (isp != 0)
	    {
		    errf_ofunc_t oldf;

		    oldf = errf_set_ofunc(display_message);
		    errf("unmatched N_BINCL symbol in %s (%s)", path, name);
		    errf_set_ofunc(oldf);
		    while (isp > 0)
			    istack[--isp]->hf_stf->stf_symlim = symno;
	    }

	    stf->stf_fnum = 0;
	    fmap[0] = (hf_t *) alloc(ap, sizeof(hf_t));
	    fmap[0]->hf_stf = stf;
	    fmap[0]->hf_id = -1;
	    fmap[0]->hf_next = NULL;
	    mapsize = 1;
	    path_hint = NULL;
	    seen_func = FALSE;
	    symset[N_SLINE] = TRUE;
#ifdef OS_ULTRIX
	    have_unused_datasym = FALSE;
#endif
	    break;

	case N_SOL:
	    /*  Names are relative to the last N_SO, so we need one.
	     */
	    if (stf == NULL)
		    break;

	    name = symstring(symio, symno);

	    if (name && *name == '.' && *(name+1) == '/')
		    name += 2;
	    solfil = add_sol_fil(stf, ap, st->st_sfiles, stf->stf_fil, name); 

	    if (solfil == stf->stf_fil)
		    solfil = NULL;

	    /*
	     * If we were able to determine the compiler,
	     * set FI_FOUND_COMPILER flag
	     */
	    if (solfil && set_compiler) {
		    solfil->fi_flags |= FI_FOUND_COMPILER;
	    }

	    if (curfunc != NULL && solfil != NULL &&
		  (solfil->fi_funclist == NULL ||
		   solfil->fi_funclist->fl_func != curfunc))
	    {
		add_to_fil_funclist(ap, solfil, curfunc);
	    }

	    /* RGA Insight puts out these - ignore them */
	    if (strcmp(name, "NoName") == 0)
		break;

	    /*
	     * Make ups work better with cfront (c++).
	     * Is it a something other than a header file ?
	     */
	    if (solfil && name != NULL &&
	        !strstr(name, ".h") &&
	        !strstr(name, ".H") &&
	        !strstr(name, ".hh") &&
	        !strstr(name, ".idl") &&
	        !strstr(name, ".icon"))
	    {
		/*
		 * Not a header.
		 */
		if (solfil->fi_language == LANG_UNKNOWN)
		    solfil->fi_flags |= FI_RENAMED_OTHER;

		if (!(st->st_sfiles->fi_flags & FI_RENAMED_OTHER) &&
		    (!strrchr(name, '/') ||
		     (st->st_sfiles->fi_language != srctype(stf->stf_name) &&
		      strrchr(stf->stf_name, '.'))))
		  /* ignore bogus files with no extension */
		{
		    if (st->st_sfiles->fi_language == LANG_C &&
			solfil->fi_language == LANG_CC)
		    {
			/*
			 * C++ -> C, so looks like it is out of cfront.
			 * Use first file renamed by cfront.
			 */
			st->st_sfiles->fi_flags |= FI_RENAMED_OTHER;
			st->st_sfiles->fi_name = solfil->fi_name;
			st->st_sfiles->fi_name_only = solfil->fi_name_only;
			st->st_sfiles->fi_language = LANG_CC;
		    }
		    else if ((st->st_sfiles->fi_language != solfil->fi_language) &&
			   (solfil->fi_language != LANG_UNKNOWN))
		    {
			/*
			 * Some other pre-processor, e.g. f2c.
			 */
			st->st_sfiles->fi_name = solfil->fi_name;
			st->st_sfiles->fi_name_only = solfil->fi_name_only;
			st->st_sfiles->fi_language = solfil->fi_language;
		    }
		    doing_header = 0;
		}
		else
		    doing_header = 0;
	    }
	    else
	    {
		/* Is a header. */
		if (solfil != NULL)
		    solfil->fi_flags |= FI_HEADER;
		doing_header = 1;
	    }
	    break;

#ifdef AO_ELF
	case N_OPT:
	    /* RCB:  Gnu outputs the N_OPT before the
	    **  N_SO, so we have to save the data
	    **  until we get the N_SO record.
	    */

	    cptr = symstring(symio, symno);

	    if (cptr== NULL)
		break;

	    lang = LANG_UNKNOWN;
	    elf_handle_optsym(ap, cptr, &lang,
			      &next_global_prefix,
			      &next_compiler,
			      &has_debug_syms);
	    if (stf && !set_compiler)
	    {
		/* Case for N_SO before N_OPT ? */
		stf->stf_compiler_type = next_compiler;
		stf->stf_global_prefix = next_global_prefix;
		stf->stf_obj_mtime = nm.n_value;
		next_compiler = CT_UNKNOWN;
		next_global_prefix = NULL;
		if (lang != LANG_UNKNOWN)
		{
		    /* This should be a better bet than the file extension */
		    stf->stf_language = lang;
		    if (last_fil != NULL)
		    {
			last_fil->fi_language = lang;
			last_fil = NULL;
		    }
		}
	    }
	    break;
#endif

	case N_SLINE:
	    if (!seen_func && stf != NULL)
		    stf->stf_flags |= STF_LNOS_PRECEDE_FUNCS;
	    symset[N_SLINE] = FALSE;
	    break;

#ifdef N_XLINE
	case N_XLINE:
	    sline_offset = nm.n_desc << 16;
	    break;
#endif

#ifdef N_BINCL
	case N_BINCL:
	case N_EXCL:
	    if (stf == NULL)
		    panic("header outside source file in ss");

	    if (mapsize == max_mapsize) {
		    max_mapsize *= 2;
		    fmap = (hf_t **) e_realloc((char *)fmap,
				    max_mapsize * sizeof(hf_t *));
	    }

	    if (nm.n_type == N_EXCL) {
		    hf = lookup_hf(headers, (int)nm.n_value);
		    fmap[mapsize++] = hf;
		    break;
	    }

	    if (isp == istack_size) {
		    istack_size *= 2;
		    istack = (hf_t **) e_realloc((char *)istack,
				    istack_size * sizeof(hf_t *));
	    }

	    istack[isp] = (hf_t *) alloc(ap, sizeof(hf_t));

	    istack[isp]->hf_next = headers;
	    headers = istack[isp++];

	    name = alloc_strdup(ap, symstring(symio, symno));
	    headers->hf_stf = make_stf(ap, name, st, symno,
				       st->st_sfiles->fi_language,
				       nm.n_value);

	    /* RCB: Copy the known compiler info to the header file */
	    headers->hf_stf->stf_compiler_type = stf->stf_compiler_type;

	    if (nm.n_type == N_EXCL) {
		    headers->hf_stf->stf_fil = NULL;
	    }
	    else {
		    headers->hf_stf->stf_fil =
				    ao_make_fil(headers->hf_stf,
					        (block_t *)NULL,
					        (const char *)NULL,
					        (fil_t *)NULL);
		    if (st->st_sfiles->fi_flags &= FI_FOUND_COMPILER)
			headers->hf_stf->stf_fil->fi_flags |= FI_FOUND_COMPILER;
	    }

	    doing_header = 0;
	    headers->hf_stf->stf_fnum = mapsize;
	    headers->hf_id = nm.n_value;
	    fmap[mapsize++] = headers;
	    break;

	case N_EINCL:
	    if (isp == 0)
		    panic("unmatched N_EINCL");

	    --isp;
	    istack[isp]->hf_stf->stf_symlim = symno;

	    if (!doing_ofile)
		    reverse_stf_funclist(stf);
	    break;
#endif /* N_BINCL */

	case N_STSYM:
	case N_GSYM:
	case N_LCSYM:
	    /*  We shouldn't get here before stf exists,
	     *  but lets not core dump if we do.
	     */
	    if (stf == NULL)
		    break;

	    if (IS_FORTRAN(stf->stf_language) &&
		symno - stf->stf_symno <= N_FIXED_FORTRAN_TYPES)
		    break;

	    cptr = symstring(symio, symno);

	    /* RGA have seen this with SC4 with templates */
	    while (*cptr==';')
	      cptr++;

	    /* RGA have seen this with SunOS/centerline */
	    if (*cptr == 0 || (*cptr  && *cptr==' '))
		break;

	    /* RGA have seen this with SC4.2 with templates */
	    if (*cptr != ':' && *cptr != '?' && !isalpha(*cptr) &&
		*cptr != '_' &&
		*cptr != '$' && *cptr != '.')
		break;

	    symrec.symio = symio;
	    symrec.symno = symno;
#if 0
	/* RGA SC4.2 folds lines at 400. Can't find a define for */
	/* this, so using an ugly number at present... */
	      {
		size_t len;

		len = strlen(cptr);
		end = (char *)cptr + len - 1;
		if (len == 401 && *end == '\\')
		{
		  cptr2 = e_malloc(len + 1);
		  memcpy((void *)cptr2, cptr, len + 1);
		  cptr = cptr2;
		}
		if (len == 401)
		  while (*end == '\\')
		  {
		    char *s1;
		    size_t new_len;

		    s1 = (char *)get_cont_symstring(&symrec);
		    mark_sym(symio, symrec.symno);
		    new_len = strlen(s1);
		    cptr2 = (char *)e_realloc((void *)cptr2, len + new_len);
		    cptr = cptr2;
		    memcpy((void *)(cptr2 + len - 1), s1, new_len);
		    cptr2[len + new_len - 1] = 0;
		    len += new_len - 1;
		    end = (char *)cptr + len - 1;
		    if (new_len != 401)
		      break;
		  }
	      }
#endif
#if 1
	    /* Check for continuation characters in the name */
	    {
		size_t len;

		len = strlen(cptr);
		end = (char *)cptr + len - 1;
		if (*end == '\\')
		{
		    cptr2 = e_malloc(len + 1);
		    memcpy((void *)cptr2, cptr, len + 1);
		    cptr = cptr2;
		}
		while (*end == '\\')
		{
		    char *s1;
		    size_t new_len;

		    s1 = (char *)get_cont_symstring(&symrec);
		    /* symno = symrec.symno;   No!  */
		    mark_sym(symio, symrec.symno);
		    new_len = strlen(s1);
		    cptr2 = (char *)e_realloc((void *)cptr2, len + new_len);
		    cptr = cptr2;
		    memcpy((void *)(cptr2 + len - 1), s1, new_len);
		    cptr2[len + new_len - 1] = 0;
		    len += new_len - 1;
		    end = (char *)cptr + len - 1;
		}
	    }
#endif
	    symtab_name = cptr;

#ifdef AO_ELF
	    if (doing_ofile) {
		if (symtab_sym_name_to_sn(stf->stf_snlist,
				      symtab_name, &sn, FALSE)) {
			sn->sn_symno = symno;
			break;
		}

		/*  The Sun cc (SPARCompiler 3.0) leaves a
		 *  few symbols out of the .stab.index
		 *  section in the linked file, so don't
		 *  complain about extra symbols - just
		 *  silently add them.  These symbols always
		 *  seem to be local static variables in an
		 *  inner block scope, but I can't reproduce
		 *  this with a small test case.
		 */
	    }

	    /*  SPARCompiler 3.0 sometimes emit duplicate
	     *  symbols, of which the first seems to be bogus.
	     *  For example, there are two entries for
	     *  file_string in the symbol table for
	     *  test/C/core/f.c.
	     *
	     *  Just ignore the first one.
	     */
	    if (symtab_sym_name_to_sn(stf->stf_snlist,
					  symtab_name, &sn, TRUE)) {
		    sn->sn_symno = symno;
		    break;
	    }

#endif

	    sn = push_symname(&symrec,ap, stf, cptr, symno);

#ifdef OS_ULTRIX
	    sn->sn_addr = 0;

	    if (have_unused_datasym) {
		lastname = symstring(symio, last_data_symno);

		if (*lastname == '_' &&
		    strcmp(lastname + 1, sn->sn_name) == 0) {
			getsym(symio, last_data_symno, &data_nm);
			sn->sn_addr = data_nm.n_value;
		}
		have_unused_datasym = FALSE;
	    }
#endif
	    break;

	case N_FUN:
	    name = symstring(symio, symno);

	    /*  Some compilers (e.g. gcc) put read only strings
	     *  in the text segment, and generate N_FUN symbols
	     *  for them.  We're not interested in these here.
	     */
	    if ((cptr = strchr(name, ':')) == NULL ||
		(cptr[1] != 'F' && cptr[1] != 'f'))
		    break;

#if defined(OS_SUNOS)
	    /* (IDE) Caused problems with C++ SC 4.2 on Sun. */
#else
	    /*  (IDE) On FreeBSD 3.0 getting two N_FUN for each
	     *  function - 1st has PSYM and SLINE after it, 2nd
	     *  repeats the PSYMs and has all other types.
	     *   RCB: Added test for nm.n_value!=0
	     */
	    if ((curfunc != NULL)
		&& (curfunc->fu_addr == nm.n_value)
		&& nm.n_value!=0 )
	    {
		    stf->stf_flags |= STF_FUNCS_REPEATED;
		    break;
	    }
#endif

	    /*  Close previous function.
	     */
	    if (curfunc != NULL) {
		    AO_FSYMDATA(curfunc)->fs_symlim = symno;
		    curfunc = NULL;
	    }

	    seen_func = TRUE;

#ifdef AO_ELF
	    if (doing_ofile) {
		    curfunc = note_ofile_function(ap, ap, stf,
						  path, solfil,
						  sline_offset,
						  name, symno,
						  e_new_functlist);
		      
		    if ( *e_new_functlist )
			e_new_functlist = & (*e_new_functlist)->fl_next;
			
		    break;
	    }
#endif
/*#ifdef OS_SUNOS
	    add_function_to_symtab
		  (st, &flist, name,
		   (st->st_sfiles->fi_language == LANG_CC && headers &&
		    headers->hf_stf) ? headers->hf_stf->stf_fil:
		   st->st_sfiles, solfil, cptr[1] == 'f', FALSE,
		   symno, nm.n_value);
#else*/
	    add_function_to_symtab(st, &flist, name,
				   st->st_sfiles, solfil,
				   cptr[1] == 'f', FALSE,
				   symno, nm.n_value);
/*#endif*/
	    AO_FSYMDATA(flist)->fs_initial_sline_offset = sline_offset;

	    /* RGA */
	    if (doing_header && flist->fu_language == LANG_CC)
		    flist->fu_flags |= FU_NOTHEADER;

	    curfunc = flist;

	    if (is_fortran_void(cptr))
		    curfunc->fu_type = ci_code_to_type(TY_VOID);
	    break;

	case N_TEXT:
	case N_TEXT | N_EXT:
	    name = symstring(symio, symno);

	    /*  Some compilers put N_TEXT symbols out with object
	     *  file names, often with the same addresses as real
	     *  functions.  We don't want these.
	     *
	     *  We also don't want N_TEXT symbols which immediately
	     *  follow an N_FUN symbol with the same address.
	     */
	    if (nm.n_type == N_TEXT) {
		if (*name != '_') {
#ifndef AO_ELF
		    /* RCB: Try to improve on gross
		    ** get_fi_compiler algorithm
		    */
		    if (next_compiler == CT_UNKNOWN)
		    {
			if (strncmp(name, "gcc_com",7) == 0) 
			    next_compiler = CT_GNU_CC;
			else if (strncmp(name, "gcc2_com",8) == 0) 
			    next_compiler = CT_GNU_CC;
			else if (strncmp(name, "clcc_",5) == 0) 
			    next_compiler = CT_CLCC;
		    }
#endif
		    /*  A .o file symbol is definitely
		     *  the end of the current function,
		     *  if any.
		     */
		    if (curfunc != NULL) {
			    AO_FSYMDATA(curfunc)->fs_symlim = symno;
			    curfunc = NULL;
		    }
		    break;
		}
		if ((curfunc != NULL) && (curfunc->fu_addr == nm.n_value))
			break;
	    }

	    add_function_to_symtab(st, &flist, name,
				   (fil_t *)NULL, (fil_t *)NULL,
				   (nm.n_type & N_EXT) == 0, TRUE, 
				   symno, nm.n_value);

	    /* RGA */
	    if (doing_header && flist->fu_language == LANG_CC)
		flist->fu_flags |= FU_NOTHEADER;

	    /*  You'd expect that we'd close down the current
	     *  function here, but some C compilers put out
	     *  N_TEXT symbols in the middle of the N_SLINE
	     *  symbols for a function.
	     *
	     *  Thus we leave curfunc alone, and rely on an N_SO or
	     *  a .o file N_TEXT to terminate the current
	     *  function.
	     */
	    break;

	case N_DATA:
#ifdef OS_ULTRIX
	    last_data_symno = symno;
	    have_unused_datasym = TRUE;
	    break; /* ? */
#endif
#ifdef ARCH_CLIPPER
	    {
		/*  The Clipper C compiler puts out an N_LCSYM symbol
		 *  with the wrong address for static globals, then
		 *  later puts out an N_DATA with the right address.
		 *  This we keep a list of N_DATA symbols for each
		 *  file so we can check the addresses later.
		 *
		 *  Note that we can't just stick the symbol in the
		 *  global addresses list, as we may have static
		 *  globals with the same name in different files.
		 */
		char *class_name, *name_start;
		int class_len, name_len;

		name = symstring(symio, symno);
		if (*name != '_')
			break;
		name_start = name+1;
		if (get_mangled_name_info
		    (0, name+1, &name_start, &name_len,
		     &class_name, &class_len))
		  name_start[name_len] = 0;
		else
		  name_start = name+1;

		insert_global_addr(ap, &stf->stf_addrlist,
				   name_start, (taddr_t)nm.n_value);
	    }
	    break; /* ? */
#endif
	case N_BSS | N_EXT:
	case N_DATA | N_EXT:
	    {
		char *name_start;

		name = symstring(symio, symno);
		if (*name != '_')
			break;
		++name;
		name_start = (char *)name;
#if 1
		/*  We only want local variables
		**  here if they look like gnu vector tables
		**  Skipping this for other local variables can
		**  save a lot of memory.
		*/
		if (nm.n_type == N_DATA )
		{
		    if (     name[0] != '_' 
			 ||  name[1] != 'v'
			 ||  name[2] != 't'
			 ||( name[3] != '$' && name[3] != '.' ) )
			break;
		}
#endif

		insert_global_addr(ap, &st->st_addrlist, name_start,
				   ast->st_base_address +
						  (taddr_t)nm.n_value);
	    }
	    break;

	case N_BCOMM:
	    symno = push_common_block(st, stf, curfunc, symio, symno);
	    break;

	default:
	    panic("unexpected symbol in init_syms");

	} /* switch nm.ntype */

	if (cptr2)
	    free((void *)cptr2);

    } /* for symno ... */

    if (curfunc != NULL) {
	AO_FSYMDATA(curfunc)->fs_symlim = symno;
	curfunc = NULL;
    }

    if (stf != NULL) {
	wrapup_stf(stf, fmap, mapsize);

	if (!doing_ofile)
	    reverse_stf_funclist(stf);
	else if (new_functlist)
	    stf->stf_fil->fi_funclist = new_functlist;

	stf->stf_symlim = symno;
    }

    free((char *)fmap);
    free((char *)istack);
    alloc_free_pool(tmp_ap);

    if (!doing_ofile)
	*p_flist = flist;

    if (p_mainfunc_name != NULL)
	*p_mainfunc_name = mainfunc_name;
}

snlist_t *push_symname(sr,ap,  stf, sym_name, symno)
Symrec *sr;
alloc_pool_t *ap;
stf_t *stf;
const char *sym_name;
int symno;
{
    /*
    **  RCB: Radically re-wrote to put all calls to
    **       demangle, or to elf_name_from_symtab_name,
    **	     here, instead of in the callers to this routine.
    **
    **	Enter:  sym_name = name as it appears in the symbol table
    **
    **    Create a snlist_t for this symbol.  Determine the
    **	   sn_name = the name as it is known to the user.
    **
    **	Link the snlist_t into the stf, and return it.
    **
    */
    char* cptr;
    const char* end_name = sym_name;
    snlist_t *sn = (snlist_t *)alloc(ap, sizeof(snlist_t));
    sn->sn_symno = symno;
#ifdef OS_ULTRIX
    sn->sn_addr = 0;
#endif

    /* get the end of the name */
    /* ( This would fail if a continuation character occured in
    ** the name.  But the N_DATA records for which this routine
    ** is called tend to be short and do not have continuation chars. )
    */
    parse_name(sr, &end_name, NULL, 0, stf->stf_compiler_type,
	       IS_FORTRAN(stf->stf_language));

    /* copy the name */
    cptr = (char*)alloc(ap, end_name-sym_name+1);
    strncpy(cptr,sym_name,end_name-sym_name);
    cptr[end_name-sym_name] = 0;
    sn->sn_symtab_name = cptr;
#ifdef AO_ELF
    sym_name = elf_name_from_symtab_name
			  (stf->stf_global_prefix, sym_name);
#endif
    /* parse the name again for demangling */
    sn->sn_name = parse_name(sr, &sym_name, ap, 0, stf->stf_compiler_type,
			     IS_FORTRAN(stf->stf_language));

    sn->sn_next = stf->stf_snlist;
    stf->stf_snlist = sn;
    return sn;
}
#endif /* !ST_TE */

#ifdef AO_ELF
static func_t *
note_ofile_function(ap, tmp_ap, stf, path, solfil, sline_offset, namestr, symno, p_fl)
alloc_pool_t *ap, *tmp_ap;
stf_t *stf;
const char *path;
fil_t *solfil;
unsigned sline_offset;
const char *namestr;
int symno;
funclist_t** p_fl;
{
	fil_t *fil = stf->stf_fil;
	funclist_t* fl = NULL;
	funclist_t** plast_fl=&fil->fi_funclist;
	func_t *f = NULL;
	const char *modname, *fname;
	int modsepc;

	/*  RCB:  Added p_fl parameter.  When funct is found, we remove
	 *        it from the original functlist and pass it to the caller,
	 *	  who puts it in a list sorted by symno.  This is because
	 *	  the list as originally built is not sorted in any useful
	 *	  order.
	 */
	modsepc = (fil->fi_language == LANG_F90) ? '$' : '\0';

	parse_fname(tmp_ap, namestr, modsepc, FALSE, &modname, &fname);

	for (fl = fil->fi_funclist; fl != NULL; fl = fl->fl_next)
	{
	    if (strcmp(fl->fl_func->fu_name, fname) == 0)
	    {
	      fsyminfo_t *fs;

	      f = fl->fl_func;
	      fs = AO_FSYMDATA(f);
	      if (fs->fs_symno == 0)
	      {
		fs->fs_symno = symno;
		fs->fs_initial_lno_fil = (solfil != NULL) ? solfil : fil;
		fs->fs_initial_sline_offset = sline_offset;

		if (solfil != NULL)
		  add_to_fil_funclist(ap, solfil, f);
		break;
	      }
	    }
	    plast_fl = &fl->fl_next;
	}
	if ( fl)
	{
	    *plast_fl = fl->fl_next;
	    fl->fl_next = NULL;
	}
	*p_fl = fl;
	return f;
}
#endif /* AO_ELF */

void
add_to_fil_funclist(ap, fil, f)
alloc_pool_t *ap;
fil_t *fil;
func_t *f;
{
	funclist_t *fl;

	fl = (funclist_t *)alloc(ap, sizeof(funclist_t));
	fl->fl_func = f;
	fl->fl_next = fil->fi_funclist;
	fil->fi_funclist = fl;
}

void
add_function_to_symtab(st, p_flist, namestr, fil, solfil,
		       is_static, is_textsym, symno, addr)
symtab_t *st;
func_t **p_flist;
const char *namestr;
fil_t *fil, *solfil;
bool is_static, is_textsym;
int symno;
taddr_t addr;
{
	fsyminfo_t *fs;
	func_t *f;
	const char *modname, *fname;
	int modsepc;
	Compiler_type compiler;

	modsepc = (fil != NULL && fil->fi_language == LANG_F90) ? '$' : '\0';

	parse_fname(st->st_apool, namestr, modsepc,
		    is_textsym && !is_static, &modname, &fname);

	fs = make_fsyminfo(st->st_apool, symno);

	/*  I put this in as a workaround for a bug (with a fixme
	 *  comment.  Now I can't remember why it was needed.
	 *
	 *  TODO: find out why it was needed, and maybe do something
	 *        better.
	 */
	fs->fs_symlim = 1;

	fs->fs_initial_lno_fil = (solfil != NULL) ? solfil : fil;

	compiler = ao_compiler(fil, FALSE, CT_UNKNOWN);
	if (fil && fil->fi_language == LANG_CC && solfil && compiler != CT_GNU_CC)
	{
	  /* RGA 11/20/00 check clcc case */
	  f = ci_make_func
	    (st->st_apool, fname, addr, st,
	     (compiler == CT_CLCC && solfil->fi_language == LANG_UNKNOWN) ?
	     fil : solfil, *p_flist);
	  if (!strcmp(f->fu_demangled_name, "__STATIC_CONSTRUCTOR")) 
	  {			 /* static ctor */
	    f->fu_fil = fil;
	    f->fu_language = (fil != NULL) ? fil->fi_language : LANG_UNKNOWN;
	  }
	}
	else
	  f = ci_make_func(st->st_apool, fname, addr, st, fil, *p_flist);
	f->fu_symdata = (char *)fs;
	f->fu_predata = NULL;

	if (fil == NULL) {
		f->fu_flags |= FU_NOSYM | FU_DONE_BLOCKS | FU_DONE_LNOS;
	}
	else {
		add_to_fil_funclist(st->st_apool, fil, f);

		if (solfil != NULL)
			add_to_fil_funclist(st->st_apool, solfil, f);
	}

	if (is_static)
		f->fu_flags |= FU_STATIC;

	if (modname != NULL)
		add_module_function(st, modname, f);

	*p_flist = f;
}


static bool
is_fortran_void(cptr)
const char *cptr;
{
	/*  BUG: should look at the type properly.
	 *       There is no guarantee that type 11 is always void.
	 */
	return strcmp(cptr, ":F11") == 0;
}

/*  Adjust the addresses of all the global variables associated
 *  with source files in flist.  Called when a shared library
 *  mapping address changes across runs of the target.
 */
static void
adjust_fil_vars_addr_base(flist, delta)
fil_t *flist;
long delta;
{
	fil_t *fil;
	var_t *v;

	for (fil = flist; fil != NULL; fil = fil->fi_next) {
		if (AO_FIDATA(fil) == NULL)
			continue;

		AO_FIDATA(fil)->stf_addr += delta;

		if (fil->fi_flags & FI_DONE_VARS) {
			for (v = fil->fi_block->bl_vars;
			     v != NULL;
			     v = v->va_next) {
				v->va_addr += delta;
				if (v->va_location &&
				    v->va_location->v_op == OP_ADDR)
				   v->va_location->v_addr += delta;
			}
		}
	}
}

/*  Adjust the addresses of all the functions in funclist.  Called when
 *  a shared library mapping address changes across runs of the target.
 */
static void
adjust_funcs_addr_base(funclist, delta)
func_t *funclist;
long delta;
{
	func_t *f;
	fsyminfo_t *fs;
   
	for (f = funclist; f; f = f->fu_next) {
		fs = AO_FSYMDATA(f);
	   
#if WANT_DWARF
		fs->fs_low_pc += delta;
		fs->fs_high_pc += delta;
#endif
	}
}

/*  Deal with a change in the text offset of a symbol table.  This may
 *  be necessary when re-running the target as shared libraries may be
 *  mapped at different addresses.  It's also necessary when we have
 *  preloaded symbol tables with a nominal offset of zero.
 *
 *  We adjust the following:
 *
 *	function and line number addresses
 *	symbol table address to text file offset
 *	addresses of global variables
 *
 *  We don't change breakpoint addresses here - we do that by removing
 *  and recreating all breakpoints just after starting the target.
 */
void
change_base_address(st, new_addr)
symtab_t *st;
taddr_t new_addr;
{
	long delta;
	ao_stdata_t *ast;

	ast = AO_STDATA(st);
	delta = new_addr - ast->st_base_address;

	if (delta != 0) {
		adjust_addrlist_addr_offset(st->st_addrlist, delta);
		ast->st_addr_to_fpos_offset += delta;
		adjust_functab_text_addr_base(st->st_functab,
					      st->st_funclist, delta);
		adjust_fil_vars_addr_base(st->st_sfiles, delta);
		adjust_funcs_addr_base(st->st_funclist, delta);
		ast->st_base_address = new_addr;
#if WANT_DWARF
		ast->st_dw_base_address = ast->st_dw_base_address + delta;
#endif
	}
}


#endif /* AO_TARGET */


int
get_mangled_name_info(var, name, name_start, name_len,
		      class_name, class_len)
     int var;
     char *name;
     char **name_start;
     int *name_len;
     char **class_name;
     int *class_len;
{
  int ret = 0;
  char *t, *n, *start, *end;
  char num_buff[10];

  *class_name = NULL;
  *class_len = 0;
  *name_len = 0;
  start = name;

  if (demangling_enabled(0, 0))
  {
    if (var)
    {
      start = *name_start;
      t = strstr(start, "__");
      if (t && isdigit(*(t + 2)) && t == start)
      {
	ret = 1;
	t += 2;		/* skip "__" */
	while(isdigit(*t))
	  t++;
      }
      else
	t = start;
      *name_start = start = t;
    }
    else
      start = name;
    end = start + strlen(start);
    for (t = start; t && t < end;)
    {
      t = strstr(t, "__");
      if (t)
      {
	if (isdigit(*(t + 2)) || (isalpha(*(t + 2)) && isupper(*(t + 2))))
	{
	  for (*name_len = 0, n = start; n < t; (*name_len)++, n++);
	  if (t && isdigit(*(t + 2)))
	  {
	    if (*(t + 2) == '0')
	    {
	      ret = 2;		/* SC3 mangling */
	      if (isupper(*(t + 3)))
	      {
		*name_start = t + 5;
		if ((int)(*(t + 4) - 'A') > 25)
		  *name_len = (int)(*(t + 4) - 'a') + 26;
		else
		  *name_len = (int)(*(t + 4) - 'A');
		t = *name_start + *name_len;
	      }
	      else
	      {
		if (*(t + 3) == 'd' && isdigit(*(t + 4)))
		{
		  while (!isupper(*(t + 4)) && t + 5 < end) t++;
		  if ((int)(*(t + 4) - 'A') > 0)
		  {
		    if ((int)(*(t + 4) - 'A') > 25)
		      t += (int)(*(t + 4) - 'a') + 26 + 1;
		    else
		      t += (int)(*(t + 4) - 'A') + 1;
		  }
		}
		if ((int)(*(t + 4) - 'A') > 0)
		{
		  *class_name = t + 5;
		  if ((int)(*(t + 4) - 'A') > 25)
		    *class_len = (int)(*(t + 4) - 'a') + 26;
		  else
		    *class_len = (int)(*(t + 4) - 'A');
		  if (*class_len > end - *class_name)
		    *class_len = end - *class_name;
		  t = *class_name + *class_len;
		  while (isdigit(*t)) /* templates...just skip */
		    t++;
		  *name_start = t + 1;
		  if (*name_start > end)
		    *name_start = start;
		  if ((int)(*(t) - 'A') > 25)
		    *name_len = (int)(*(t) - 'a') + 26;
		  else
		    *name_len = (int)(*(t) - 'A');
		  if (*name_len > end - *name_start)
		    *name_len = end - *name_start;
		  if (*name_len < 0)
		    t = *name_start;
		  else
		  t = *name_start + *name_len;
		}
		else
		{
		  ret = 0;
		  break;
		}
	      }
	    }
	    else
	    {
	      for (*name_len = 0, n = start; n < t; (*name_len)++, n++);
	      if (isdigit(*(t + 2)))
	      {
		t += 2;		/* skip "__" */
		for (n = num_buff; (isdigit(*t)); t++, n++)
		  *n = *t;
		*(n++) = 0;
		*class_name = t;
		*class_len = atoi(num_buff);
		if (t + *class_len > end)
		{
		  *class_name = NULL;
		  *class_len = 0;
		  *name_len = end - *name_start;
		}
	      }
	      ret = 1;
	      break;
	    }
	  }
	  if (!ret)
	    ret = 1;
	  break;
	}
	else
	  ++t;
      }
    }
    if (!(*name_len))
      for(*name_len = 0, t = start; isalnum(*t) || *t == '_';
	  (*name_len)++, t++); 
    if (*name_len < 0)
      *name_len = 0;
  }
  return (ret);
}
#define CONSTRUCTOR 0
#define DESTRUCTOR 1
#define OPERATOR 2
#define OPERATOR_CAST 3

static struct 
{
  const char *mangle_str;
  int   mangle_str_len;
  const char *demangle_str;
  int demangle_str_len;
  int type;
} Mangle_buff[] =
{
  /*  Macro to define an entry in Mangle_buff */
#define MNGL(mgl, dmgl, type) { mgl, sizeof(mgl)-1, dmgl, sizeof(dmgl)-1,type }

  MNGL( "ct",  "",           CONSTRUCTOR),
  MNGL( "dt",  "",           DESTRUCTOR),
  MNGL( "op",  "operator ",   OPERATOR_CAST),
  MNGL( "as",  "operator=",  OPERATOR),
  MNGL( "eq",  "operator==", OPERATOR),
  MNGL( "pl",  "operator+",  OPERATOR),
  MNGL( "mi",  "operator-",  OPERATOR),
  MNGL( "ml",  "operator*",  OPERATOR),
  MNGL( "dv",  "operator/",  OPERATOR),
  MNGL( "md",  "operator%",  OPERATOR),
  MNGL( "er",  "operator^",  OPERATOR),
  MNGL( "ad",  "operator&",  OPERATOR),
  MNGL( "or",  "operator|",  OPERATOR),
  MNGL( "co",  "operator~",  OPERATOR),
  MNGL( "nt",  "operator!",  OPERATOR),
  MNGL( "gt",  "operator>",  OPERATOR),
  MNGL( "lt",  "operator<",  OPERATOR),
  MNGL( "apl", "operator+=", OPERATOR),
  MNGL( "ami", "operator-=", OPERATOR),
  MNGL( "amu", "operator*=", OPERATOR),
  MNGL( "aml", "operator*=", OPERATOR),
  MNGL( "adv", "operator/=", OPERATOR),
  MNGL( "gdv", "operator/=", OPERATOR),
  MNGL( "amd", "operator%=", OPERATOR),
  MNGL( "aer", "operator^=", OPERATOR),
  MNGL( "aad", "operator&=", OPERATOR),
  MNGL( "gad", "operator&=", OPERATOR),
  MNGL( "aor", "operator|=", OPERATOR),
  MNGL( "ls",  "operator<<", OPERATOR),
  MNGL( "rs",  "operator>>", OPERATOR),
  MNGL( "ars", "operator>>=",OPERATOR),
  MNGL( "als", "operator<<=",OPERATOR),
  MNGL( "ne",  "operator!=", OPERATOR),
  MNGL( "le",  "operator<=", OPERATOR),
  MNGL( "ge",  "operator>=", OPERATOR),
  MNGL( "rm",  "operator->*",OPERATOR),
  MNGL( "cm",  "operator,",  OPERATOR),
  MNGL( "mm",  "operator--", OPERATOR),
  MNGL( "pp",  "operator++", OPERATOR),
  MNGL( "oo",  "operator||", OPERATOR),
  MNGL( "aa",  "operator&&", OPERATOR),
  MNGL( "vc",  "operator[]", OPERATOR),
  MNGL( "cl",  "operator()", OPERATOR),
  MNGL( "rf",  "operator->", OPERATOR),
  MNGL( "nw",  "operator new",        OPERATOR),
  MNGL( "dl",  "operator delete",     OPERATOR)
};
#define NMANGLE_STRS (sizeof Mangle_buff / sizeof *Mangle_buff)

char* demangle_name_3(name)
char* name;
{
    /* demangle the input name, returning a malloc'ed string */
    char* dmangle = NULL;
    switch ( ao_compiler(NULL, FALSE, CT_UNKNOWN))
    {
    case CT_GNU_CC:
        dmangle = gnu_cplus_demangle(name, DMGL_GNU);
	break;
    case CT_CLCC:
        dmangle = gnu_cplus_demangle(name, DMGL_ARM);
	break;
    case CT_CC:
      {
#if HAVE_CPLUS_DEMANGLE
	    char buff[512];
	    const char* dmgl = buff;
    	    int ret = sunpro_cplus_demangle(name, buff, 512);
	    if ( ret == DEMANGLE_ESPACE)
		dmgl = name;
	    dmangle = (char*)malloc(strlen(name)+1);
	    strcpy(dmangle, dmgl);
#else
	    int name_len, class_len;
	    char* class_name, *name_start;
	    name_start = (char*)name;     /* cast away const */
	    if ( get_mangled_name_info(0, name_start, &name_start, &name_len,
		      &class_name, &class_len))
	    {
		dmangle = malloc(class_len+name_len+3);
		strncpy(dmangle,class_name,class_len);
		strcpy(dmangle+class_len,"::");
		strncpy(dmangle+class_len+2, name_start, name_len);
		dmangle[class_len+name_len+2] = 0;
	    }
#endif
	break;
      }
    default:
	break;
    }
    return dmangle;
}

void
demangle_name_2(name, len, alloc_id, ptr, func, fil)
     char *name;
     int len;
     alloc_pool_t *alloc_id;
     char **ptr;
     int func;
     fil_t *fil;
{
  /* Dibyendu : when debugging CX executables, demangle_name() does
   * not make sense - so avoid calling it. I am assuming that CT_UNKNOWN
   * is not a valid compiler type - and the default. Ideally CX/CG
   * should get their own type.
   * ao_compiler() is fooled by setting FI_FOUND_COMPILER flag when
   * loading a CX executable (see xc_text.c).
   */
  Compiler_type ct;

  ct = ao_compiler(fil, FALSE, CT_UNKNOWN);
  if (ct == CT_UNKNOWN)
     *ptr = alloc_strdup(alloc_id, name);
  else
     demangle_name(name, len, alloc_id, ptr, func, ct);

  /* GNU f77 likes appending underscores to names of globals. */
  if (ct == CT_GNU_CC && fil && IS_FORTRAN(fil->fi_language) && *ptr && **ptr)
     demangle_g77_name(*ptr, func);
}

/* RGA for Solaris, you can use the cplus_demangle() function in libC
   by changing the #ifdefs below. This will demangle templates and
   the like for SC4 and above. You need to extract dem.o cafe_dem.o
   from libC.a. E.g.

   ar x /opt/SUNWspro/SC4.0/lib/libC.a dem.o cafe_dem.o

   and add these files to the link, such as the AOOBJS list in
   ups/Makefile.
   */


extern  char* gnu_demangle_type PROTO((const char** mangled, int options));

void
demangle_name(name, len, alloc_id, ptr, func, compiler)
     char *name;
     int len;
     alloc_pool_t *alloc_id;
     char **ptr;
     int func;
     Compiler_type compiler;
{
  char *class_name, *name_start, tmp = 0, tmp2;
  int class_len, name_len, mangle, index, gnu_destructor = 0;

  name_start = name;
  tmp2 = *(name+len);
  *(name+len)= 0;
  *ptr = NULL;

  if (func == 0 
      && !( compiler == CT_CC || compiler == CT_CLCC)
      && demangling_enabled(0, 0)
      )
  {
    char *result = gnu_cplus_demangle (name, DMGL_ANSI);
    *(name+len) = tmp2;
    if (result == NULL)
    {
      *(name+len) = tmp2;
      *ptr = alloc(alloc_id, len + 1);
      memcpy(*ptr, name, len);
      (*ptr)[len] = '\0';
    }
    else
    {
      len = strlen(result);
      *ptr = alloc(alloc_id, len + 1);
      memcpy(*ptr, result, len);
      (*ptr)[len] = '\0';
      free(result);
    }
    return;
  }
#if HAVE_CPLUS_DEMANGLE
  if (   compiler == CT_CC
      && demangling_enabled(0, 0) )
  {
    char buff[512];
    int ret;
    char* dmgl = buff;
    char* name1 = name;

    /* For field names and some variable names, the sunpro compiler puts an
    ** access level character in front of the mangled name cplus_demangle
    ** is prepared to demangle.  So if the demangle failed and name+1 looks
    ** like a Sunpro mangled name try name+1 instead.
    */
    if (        name[1] == '_'
	     && name[2] == '_'
	     && isupper(name[0])
	     && (name[3] == '0' || name[3] == '1') )
    {
	*dmgl++ = *name1++; /* copy first char incase demangle fails */
    }

    ret = sunpro_cplus_demangle(name1, dmgl, 511);
    /* sunpro's demangler should not report that a function
    ** is static, but it does
    */
    if (ret == 0 && !strncmp(buff, "static ", sizeof("static")))
	dmgl += sizeof("static");
    else if (   ret == DEMANGLE_ENAME )
    {
	dmgl = buff; /* back up to the copied char */
    }
    *(name+len) = tmp2; /* put back the nulled out character */

    if ( ret != DEMANGLE_ESPACE)
    {
      if ( func == 0 )
      {
          /* If there are parameters, cut them off
	  **  But watch out for operator()(...)
          */
          char* params = strchr(buff,'(');
	  if ( params && params[1] == ')' && params[2] == '(')
              params+=2;
          if ( params)
            *params = 0;
  
      }
      else
      {
	  /* cut off the function or class name if the demangler put it in */
	  char* n = strrchr(buff, ':');
	  if (n)
	     dmgl = n+1;
      }
      len = strlen(dmgl);
    }
    else
    {
	 /* Demangled name too long if here.  Just return the mangled name */
	 dmgl = name;
    }
    *ptr = alloc(alloc_id, len + 1);
    memcpy(*ptr, dmgl, len);
    (*ptr)[len] = '\0';
    return;
  }
#endif /* HAVE_CPLUS_DEMANGLE */

  if (*(name+1) == '$' && *name == '_' && *(name+2) == '_')
  {
    /* If the compiler is gnu, we shouldn't get here unless
    ** the compiler is incorrectly determined.  But handle
    ** that just in case
    */
    gnu_destructor = 1;
    *(name+1) = '_';
  }

  mangle = get_mangled_name_info(func==1, name, &name_start, &name_len,
				 &class_name, &class_len);
  if (gnu_destructor)
    *(name+1) = '$';
  if (mangle)
  {
    if (func == 1)		/* variable */
    {
      *ptr = alloc(alloc_id, name_len + 1);
      (void) strncpy(*ptr, name_start, name_len);
      (*ptr)[name_len] = '\0';
    }
    else
    if (mangle == 2 && !class_len) /* SC 3 mangling*/
    {
      if (!strncmp(name_start, "_STCON_", name_len))
      {
	*ptr = alloc(alloc_id, strlen("__STATIC_CONSTRUCTOR") + 1);
	(void) strcpy(*ptr, "__STATIC_CONSTRUCTOR");
      }
      else
	if (!strncmp(name_start, "_STDES_", name_len))
	{
	  *ptr = alloc(alloc_id, strlen("__STATIC_DESTRUCTOR") + 1);
	  (void) strcpy(*ptr, "__STATIC_DESTRUCTOR");
	}
	else
	{
	  *ptr = alloc(alloc_id, name_len + 1);
	  (void) strncpy(*ptr, name_start, name_len);
	  (*ptr)[name_len] = '\0';
	}
    }
    else
    {				/* function name */
      if (name_len <= len)
	tmp = *(name+name_len);
      if (mangle != 2)
	*(name+name_len) = 0;
      if (gnu_destructor)
      {
	*ptr = alloc(alloc_id, 2*class_len + 4);
	memcpy(*ptr, class_name, class_len);
	memcpy(*ptr+class_len, "::", 2);
	class_len += 2;
	*(*ptr+class_len) = '~';
	memcpy(*ptr+class_len+1, *ptr, class_len-2);
	(*ptr)[class_len+class_len-1] = 0;
      }
      else
	if (class_name && !strncmp(name, "__", 2))
	{
	  if (mangle == 1 && isdigit(*(name + 2)))
	  {			/* g++ constructor */
	    *ptr = alloc(alloc_id, 2*class_len + 3);
	    memcpy(*ptr, class_name, class_len);
	    memcpy(*ptr+class_len, "::", 2);
	    class_len += 2;
	    memcpy(*ptr+class_len, *ptr, class_len - 2);
	    (*ptr)[class_len+class_len-2] = 0;
	  }
	  else
	  {
	    const char* mangled_op =
	       (mangle == 2 )
	       ? class_name + class_len
	       : name+2;
	    for (index = 0; !*ptr && index < NMANGLE_STRS; index++)
	    {
	      if (!strncmp( mangled_op
			  , Mangle_buff[index].mangle_str
			  , Mangle_buff[index].mangle_str_len))
	      {
		switch (Mangle_buff[index].type)
		{
		case CONSTRUCTOR:
		  *ptr = alloc(alloc_id, 2*class_len + 3);
		  memcpy(*ptr, class_name, class_len);
		  memcpy(*ptr+class_len, "::", 2);
		  class_len += 2;
		  memcpy(*ptr+class_len, *ptr, class_len - 2);
		  (*ptr)[class_len+class_len-2] = 0;
		  break;
		case DESTRUCTOR:
		  *ptr = alloc(alloc_id, 2*class_len + 4);
		  memcpy(*ptr, class_name, class_len);
		  memcpy(*ptr+class_len, "::", 2);
		  class_len += 2;
		  *(*ptr+class_len) = '~';
		  memcpy(*ptr+class_len+1, *ptr, class_len-2);
		  (*ptr)[class_len+class_len-1] = 0;
		  break;
		case OPERATOR:
		  *ptr = alloc(alloc_id, class_len +
			       Mangle_buff[index].demangle_str_len + 3);
		  memcpy(*ptr, class_name, class_len);
		  memcpy(*ptr+class_len, "::", 2);
		  class_len += 2;
		  memcpy(*ptr+class_len,
			 Mangle_buff[index].demangle_str,
			 Mangle_buff[index].demangle_str_len);
		  (*ptr)[class_len+Mangle_buff[index].demangle_str_len] = 0;
		  break;
		case OPERATOR_CAST:
		 {
		   /* point to end of string parsed so far: */
		   const char* ptype = mangled_op+Mangle_buff[index].mangle_str_len;
		   /* Use the gnu demangler to demangle the type */
		   char* type = gnu_demangle_type(&ptype,DMGL_ANSI);
		   if ( type )
		      ptype = type;
		   else
		      ptype = "";

		   /* gnu doesn't know about SC 3.  Fix that: */
		   if ( mangle ==2 && isupper(*ptype))
		      ptype++;

		   *ptr = alloc(alloc_id, class_len +
			       Mangle_buff[index].demangle_str_len + 3 +strlen(ptype));
		   memcpy(*ptr, class_name, class_len);
		   memcpy(*ptr+class_len, "::", 2);
		   class_len += 2;
		   memcpy(*ptr+class_len,
			 Mangle_buff[index].demangle_str,
			 Mangle_buff[index].demangle_str_len);
		   (*ptr)[class_len+Mangle_buff[index].demangle_str_len] = 0;
		   strcat(*ptr,ptype);
		   if ( type )
		      free(type);
		 }
		}
	      }
	    }
	  }
	}
      if (!*ptr)
      {
	if (mangle == 2)		/* SC 3 mangling*/
	{
	  *ptr = alloc(alloc_id, class_len + name_len + 3);
	  memcpy(*ptr, class_name, class_len);
	  memcpy(*ptr + class_len, "::", 2);
	  class_len += 2;
	  memcpy(*ptr + class_len, name_start, name_len);
	  (*ptr)[class_len+name_len] = 0;
	}
	else
	{
	  if (class_name)
	  {
	    *ptr = alloc(alloc_id, class_len + name_len + 3);
	    memcpy(*ptr, class_name, class_len);
	    memcpy(*ptr+class_len, "::", 2);
	    class_len += 2;
	  }
	  else
	    *ptr = alloc(alloc_id, class_len + name_len + 1);
	  if (name_len <= len)
	    name[name_len] = tmp;
	  memcpy(*ptr+class_len, name, name_len);
	  (*ptr)[class_len+name_len] = 0;
	}
      }
      if (name_len <= len)
	*(name+name_len) = tmp;
    }
    *(name+len) = tmp2;
  }
  else
  {
    if (!strncmp(name, "__sti__", 7))
    {
      *ptr = alloc(alloc_id, strlen("__STATIC_CONSTRUCTOR") + 1);
      (void) strcpy(*ptr, "__STATIC_CONSTRUCTOR");
    }
    else
      if (!strncmp(name, "__std__", 7))
      {
	*ptr = alloc(alloc_id, strlen("__STATIC_DESTRUCTOR") + 1);
	(void) strcpy(*ptr, "__STATIC_DESTRUCTOR");
      }
      else
      {
	*(name+len) = tmp2;
	*ptr = alloc(alloc_id, len + 1);
	memcpy(*ptr, name, len);
	(*ptr)[len] = '\0';
      }
  }
}

int
demangling_enabled(set, reset)
     int set;
     int reset;
{
  static int enabled = 1;

  if (set)
    enabled = 1;
  if (reset)
    enabled = 0;
  return(enabled);
}

