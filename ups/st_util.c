/* st_util.c - generic symbol table utility routines */

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


/* @(#)st_util.c	1.5 04 Jun 1995 (UKC) */
char ups_st_util_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#ifdef __STDC__
#include <unistd.h>
#endif

#include <local/ukcprog.h>
#include <local/arg.h>
#include <mtrprog/strcache.h>
#include <mtrprog/alloc.h>
#include <mtrprog/hash.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "target.h"
#include "state.h"
#include "st.h"
#include "obj_bpt.h"
#include "srcbuf.h"
#include "srcwin.h"
#include "srcpath.h"
#include "ui.h"
#include "tdr.h"
#ifdef AO_ELF
#include "ao_syms.h"
#include "ao_elfsym.h"
#endif

/*  Element in the linked list of global variable names and 
 *  addresses for a symbol table.
 */
struct addrlist_s {
	const char *al_name;
	taddr_t al_addr;
	var_t *al_var;
	struct addrlist_s *al_next;
};

#define LD_STRCMP 0
#define LD_STRNCMP 1
#define LD_STRSTR 2

typedef struct loadlist_s {
	const char *ld_load;
	int str_style;
	int load_requested;
	struct loadlist_s *ld_next;
} loadlist_t;

typedef struct noloadlist_s {
	const char *name;
	int loaded;
	struct noloadlist_s *ld_next;
} noloadlist_t;

typedef struct name_s{
  const char *name;
} name_t;

static int ncmp PROTO((const void *name1p, const void *name2p));
static noloadlist_t *NoLoadlist = NULL, *NoLoadlist_tail;
static block_t *make_rootblock PROTO((void));
static loadlist_t *Loadlist = NULL, *Loadlist_tail;
static int Noload_flag = 0;

void
free_library_noload_list()
{
  noloadlist_t *ld, *ld1;

  for (ld = NoLoadlist; ld != NULL; )
  {
    ld1 = ld->ld_next;
    free((char *)ld->name);
    free(ld);
    ld = ld1;
  }
  NoLoadlist = NULL;
}

void
add_to_noload_list(name, loaded)
const char *name;
int loaded;
{
        noloadlist_t *ld;


	/* adding a new entry, so check if it's already in the list */
	for (ld = NoLoadlist; ld != NULL; ld = ld->ld_next)
	  if (!ld->loaded && ld->name && !strcmp(ld->name, name))
	  {
	    ld->loaded = loaded;
	    return;
	  }

        ld = (noloadlist_t *)e_malloc(sizeof(noloadlist_t));
	ld->name = strsave(name);
	ld->loaded = loaded;
        ld->ld_next = NULL;

        if (NoLoadlist == NULL)
                NoLoadlist = ld;
        else
                NoLoadlist_tail->ld_next = ld;
        NoLoadlist_tail = ld;
}

static int
ncmp(name1p, name2p)
const void *name1p, *name2p;
{
  const name_t *name1 = name1p;
  const name_t *name2 = name2p;
  if (!name1 || !name2)
    return 1;
  return strcmp(name1->name,  name2->name);
}

int
get_noload_list(names)
const char ***names;
{
        noloadlist_t *ld;
	const char **ptr;
	int i;
	name_t *n, *nroot;

	for (i = 0, ld = NoLoadlist; ld != NULL; ld = ld->ld_next)
	  if (!ld->loaded && ld->name)
	    i++;
	if (i == 0)
	  return 0;
	nroot = n = malloc(sizeof(name_t*)*(i+1));
	for (ld = NoLoadlist; ld != NULL; ld = ld->ld_next)
	  if (!ld->loaded && ld->name)
	  {
	    n->name = ld->name;
	    n++;
	  }
	n->name = 0;
	qsort((char *)nroot, i, sizeof(name_t *), ncmp);
	*(names) = ptr = malloc(sizeof(const char **)*(i+1));
	for (n = nroot; n->name && ptr; n++, ptr++)
	  *ptr = n->name;
	*ptr = 0;
	free(nroot);
	return 1;
}

void
free_library_load_list()
{
  loadlist_t *ld, *ld1, **tail_ld_ptr, **last_ld_ptr;

  for (last_ld_ptr = &Loadlist, tail_ld_ptr = &Loadlist_tail,
	 Loadlist_tail = 0, ld = Loadlist; ld != NULL; )
  {
    ld1 = ld->ld_next;
    if (!ld->load_requested)
    {
      *last_ld_ptr = ld->ld_next;
      free((char*)ld->ld_load);
      free(ld);
    }
    else	/* keep these */
    {
      *tail_ld_ptr = *last_ld_ptr = ld;
      last_ld_ptr = &ld->ld_next;
    }
    ld = ld1;
  }
}

void
add_to_load_path(path, noload, load_requested)
const char *path;
int noload;
int load_requested;
{
        loadlist_t *ld;
	char *buf;
	const char *path_ptr = path;

	if (load_requested)
	{
	  for (ld = Loadlist; ld != NULL; ld = ld->ld_next)
	    if (strstr(path_ptr, ld->ld_load))
	    {
	      ld->load_requested = 1;
	      return;
	    }
	}

        ld = (loadlist_t *)e_malloc(sizeof(loadlist_t));
	ld->load_requested = load_requested;
	if (*path_ptr == '.' && *(path_ptr+1) == '/')
	{
	  const char *source_path;

	  source_path = hold_default_source_path(NULL, FALSE); /* get */
	  path_ptr = (const char *)e_malloc(strlen(source_path) + strlen(path));
	  sprintf((char *)path_ptr, "%s%s", source_path, path+1);
	}

	buf = strsave(path_ptr);

	if (*path_ptr == '*')
	{
	  ld->str_style = LD_STRSTR;
	  if (path_ptr[strlen(path_ptr)-1] == '*')
	  buf[strlen(path_ptr)-1] = 0;
	  ld->ld_load = strsave(buf + 1);
	}
	else
	  if (path_ptr[strlen(path_ptr)-1]  == '*')
	  {
	    ld->str_style = LD_STRNCMP;
	    buf[strlen(path_ptr)-1] = 0;
	    ld->ld_load = strsave(buf);
	  }
	  else
	  {
	    ld->str_style = LD_STRCMP;
	    ld->ld_load = strsave(path_ptr);
	  }
	free(buf);
	if (path_ptr != path)
	  free((char *)path_ptr);
	if (!load_requested)
	  Noload_flag = noload;
        ld->ld_next = NULL;

        if (Loadlist == NULL)
                Loadlist = ld;
        else
                Loadlist_tail->ld_next = ld;
        Loadlist_tail = ld;
}


int user_wants_library_loaded(name)
     char *name;
{
  loadlist_t *ld;

  if (!Loadlist)
    return(1);			/* default - load all libraries */

  for (ld = Loadlist; ld != NULL; ld = ld->ld_next)
  {
    switch (ld->str_style)
    {
    case LD_STRCMP:
      if (!strcmp(name, ld->ld_load))
	return(ld->load_requested ? 1 : Noload_flag ? 0 : 1);
      break;
    case LD_STRNCMP:
      if (!strncmp(name, ld->ld_load, strlen(ld->ld_load)))
	return(ld->load_requested ? 1 : Noload_flag ? 0 : 1);
      break;
    case LD_STRSTR:
      if (strstr(name, ld->ld_load))
	return(ld->load_requested ? 1 : Noload_flag ? 0 : 1);
      break;
    }
  }
  return(Noload_flag ? 1 : 0);
      
}

void
adjust_addrlist_addr_offset(addrlist, delta)
addrlist_t *addrlist;
long delta;
{
	addrlist_t *al;

	for (al = addrlist; al != NULL; al = al->al_next)
		al->al_addr += delta;
}

/*  Insert a new entry at the front of the globals address map.
 */
void
insert_global_addr(ap, p_addrlist, name, addr)
alloc_pool_t *ap;
addrlist_t **p_addrlist;
const char *name;
taddr_t addr;
{
	addrlist_t *al;

	al = (addrlist_t *)alloc(ap, sizeof(addrlist_t));
	al->al_name = name;
	al->al_addr = addr;
	al->al_var = NULL;
	al->al_next = *p_addrlist;
	*p_addrlist = al;
}

/*  Look up the address of global variable name.  Return the
 *  address, or 0 if name is not found.
 */
taddr_t
find_addr_in_addrlist(addrlist, name)
addrlist_t *addrlist;
const char *name;
{
	addrlist_t *al;

	for (al = addrlist; al != NULL; al = al->al_next)
		if (arg_match(al->al_name, name))
			return al->al_addr;

	return 0;
}

void
scan_addrlist(st, matchf, pat, addf, addf_arg)
symtab_t *st;
matchfunc_t matchf;
const char *pat;
void (*addf)PROTO((char *, var_t *));
char *addf_arg;
{
	addrlist_t *al;

	for (al = st->st_addrlist; al != NULL; al = al->al_next) {
		if (!(*matchf)(al->al_name, pat))
			continue;

		if (al->al_var == NULL) {
			var_t *v;

			v = ci_make_var(st->st_apool, al->al_name, CL_EXT,
					ci_code_to_type(TY_INT_ASSUMED),
					al->al_addr);
			v->va_language = LANG_UNKNOWN;
			al->al_var = v;
		}
		
		(*addf)(addf_arg, al->al_var);
	}
}

/*   Find the address of a global.  Return 0 if not found (this will happen
 *   for an extern with no definition).
 */
taddr_t
lookup_global_addr(st, name)
symtab_t *st;
const char *name;
{
	return find_addr_in_addrlist(st->st_addrlist, name);
}

/*  Find the name of a global from the address
**  ( Returns the un-demangled symtab name)
*/

const char* lookup_global_name(addr, offset)
taddr_t addr;
size_t* offset;
{
    taddr_t best_addr = 0;
    const char* best_name = NULL;
    target_t *xp = get_current_target();
    symtab_t *st = NULL;
    while ( xp_next_symtab(xp, st, FALSE, &st))
    {
	addrlist_t *al;

	for (al = st->st_addrlist; al != NULL; al = al->al_next)
	{
	    if ( al->al_addr == addr)
	    {
		*offset = 0;
		return al->al_name;
	    }
	    if ( al->al_addr > best_addr
		&& al->al_addr < addr )
	    {
		best_addr = al->al_addr;
		best_name = al->al_name;
	    }
	}
    }
    *offset = addr-best_addr;
    return best_name;
}

symtab_t *
make_symtab(ap, path, sfiles, funclist, ops, data)
alloc_pool_t *ap;
const char *path;
fil_t *sfiles;
func_t *funclist;
sym_ops_t *ops;
char *data;
{
	symtab_t *st;
	func_t *f;
	fil_t *fil;
	
	st = (symtab_t *)alloc(ap, sizeof(symtab_t));
	
	st->st_apool = ap;
	st->st_path = alloc_strdup(ap, path);
	st->st_sfiles = sfiles;
	st->st_cblist = NULL;
	st->st_modules = NULL;
	st->st_functab = NULL;
	st->st_addrlist = NULL;
	st->st_funclist = funclist;
	st->st_data = data;
	st->st_ops = ops;
	st->st_modtime = 0;
	st->st_eclipsed = 0;
	st->st_func_hv = 0;

	for (f = funclist; f != NULL; f = f->fu_next)
		f->fu_symtab = st;

	for (fil = sfiles; fil != NULL; fil = fil->fi_next)
		fil->fi_symtab = st;

	return st;
}

/*  Close down a symbol table.  This is called if we rerun a
 *  target and find that it uses a different shared library
 *  from the last run (rare, but could happen if the
 *  environment variable LD_LIBRARY_PATH is changed).
 *  If we extended ups to allow multiple binaries in the
 *  same run this routine would also get called.
 *
 *  Free all the resources and memory associated with this
 *  symbol table (the memory part is made easy by alloc()).
 */
void
close_symtab(st)
symtab_t *st;
{
	fil_t *fil;

	/*  Delete any breakpoints from this symbol table
	 */
	remove_matching_breakpoints(st, NULL);

	/*  Close any source files we have open.
	 */
	for (fil = st->st_sfiles; fil != NULL; fil = fil->fi_next) {
		if (fil->fi_srcbuf != NULL)
			srcbuf_destroy(fil->fi_srcbuf);
	}
	
	free_cblist_info(st->st_cblist);

	st_close_symtab_data(st);
	if (st->st_func_hv)
	  free(st->st_func_hv);

	/*  Free all storage associated with this symtab.
	 *  Vars, functions, common blocks, the lot.
	 */
	alloc_free_pool(st->st_apool);
}

block_t *
get_rootblock()
{
	static block_t *block = NULL;

	if (block == NULL)
		block = make_rootblock();
	
	return block;
}

static block_t *
make_rootblock()
{
	static struct {
		const char *regname;
		int regno;
	} regtab[] = {
		{ "$pc",	UPSREG_PC	},
		{ "$fp",	UPSREG_FP	},
		{ "$sp",	UPSREG_SP	},
	};
#ifdef PURIFY
	static alloc_pool_t *save_apool;
#endif
	alloc_pool_t *ap;
	type_t *type;
	block_t *block;
	int i;

	ap = alloc_create_pool();
#ifdef PURIFY
	save_apool = ap;
#endif

	block = ci_make_block(ap, (block_t *)NULL);

	type = ci_code_to_type(TY_INT);

	for (i = 0; i < sizeof regtab / sizeof *regtab; ++i) {
		var_t *v;

		v = ci_make_var(ap, regtab[i].regname, CL_REG, type,
						     (taddr_t)regtab[i].regno);

		/*  This is pretty bogus.  The problem is that the register
		 *  values for these registers are stored in a data
		 *  structure in our address space, and we don't have an
		 *  address in the target to hand.  Thus we pass back
		 *  (in get_reg_addr) the address in our address space,
		 *  and set the flag below to make it look there.
		 */
		v->va_flags |= VA_IS_CI_VAR;

		v->va_next = block->bl_vars;
		block->bl_vars = v;
	}

	return block;
}

void
iterate_over_vars_of_block(block, func, args)
block_t *block;
void (*func)PROTO((var_t *v, char *c_args));
char *args;
{
	block_t *bl;
	var_t *v;

	if (block != NULL) {
		for (bl = block->bl_blocks; bl != NULL; bl = bl->bl_next)
			iterate_over_vars_of_block(bl, func, args);
		for (v = block->bl_vars; v != NULL; v = v->va_next)
			(*func)(v, args);
	}
}

/*  Return an lno pointer given a function and an address.
 *  Return NULL if no line number information is available.
 *
 *  This function searches for the first lno that has an address larger
 *  than text_addr, and returns the one before that.
 */
bool
addr_to_lno(f, text_addr, p_ln, same_file)
func_t *f;
taddr_t text_addr;
lno_t **p_ln;
bool same_file;
{
	lno_t *ln, *last;
	taddr_t min_addr;

	if ((f->fu_flags & FU_BAD) != 0)
		return FALSE;

	if (get_min_bpt_addr(f, &min_addr, same_file) != 0 ||
	    text_addr < min_addr)
		return FALSE;

	last = NULL;
	for (ln = FU_LNOS(f); ln != NULL; ln = ln->ln_next) {
		if (same_file == TRUE && ln->ln_fil != f->fu_fil)
			continue;	/* RGA skip itializiers etc in other files */
		if (ln->ln_addr > text_addr)
			break;
		last = ln;
	}

	if (last == NULL)
		return FALSE;

	*p_ln = last;
	return TRUE;
}

void
addr_to_fil_and_lnum(f, addr, p_fil, p_lnum, same_file)
func_t *f;
taddr_t addr;
fil_t **p_fil;
int *p_lnum;
bool same_file;
{
	lno_t *lno;
	
	if (addr_to_lno(f, addr, &lno, same_file)) {
		*p_fil = lno->ln_fil;
		*p_lnum = lno->ln_num;
	}
	else {
		*p_fil = NULL;
		*p_lnum = 0;
	}
}

/*  Return the lnum that matches address text_addr.
 *
 *  This function is used to matching RBRAC addresses to line numbers.
 *  Some compilers emit several lnos for a for loop, all with the same
 *  source line number.  For the end of a block we want a source line
 *  number near the end of the loop, hence the odd way this routine works.
 */
int
rbrac_addr_to_lnum(f, text_addr)
func_t *f;
taddr_t text_addr;
{
	int max_lnum;
	lno_t *ln;
	taddr_t min_addr;

	if (get_min_bpt_addr(f, &min_addr, FALSE) != 0 || text_addr < min_addr)
		return 0;

	max_lnum = 0;
	for (ln = FU_LNOS(f); ln != NULL; ln = ln->ln_next) {
		if (ln->ln_addr > text_addr)
			break;
		if (ln->ln_num > max_lnum)
			max_lnum = ln->ln_num;
	}
	return max_lnum;
}

/*  Map line number lnum to an address, given that the line is within
 *  function f.
 *
 *  Return 0 if there is no line number information, 1 if we can't find
 *  the line number.
 */
taddr_t
lnum_and_fil_to_addr(f, fil, lnum)
func_t *f;
fil_t *fil;
int lnum;
{
	lno_t *ln;

	if ((f->fu_flags & FU_NOSYM) || FU_LNOS(f) == NULL)
		return 0;
	
	for (ln = FU_LNOS(f); ln != NULL; ln = ln->ln_next) {
	  if ((f->fu_language == LANG_CC ||
	       ln->ln_fil->fi_flags & FI_RENAMED_OTHER)
	      && ln->ln_num == lnum) /* RGA */
	    return ln->ln_addr;
	  else
	    if (ln->ln_num == lnum &&
		(ln->ln_fil == fil || (ln->ln_fil->fi_data == fil->fi_data)))
	      /* RGA after a reattach, fils are different, but fi_data 
		 point to the same data.
		 */
	      return ln->ln_addr;
	}
	
	return 1;
}

#ifdef ARCH_MIPS
/*  There is no DS3100 version of get_startup_code() - the stuff is
 *  set up by skim_te_symtab() in st_te.c.
 */
#endif /* ARCH_MIPS */

/*  Determine the minimum displacement into the function code at which a
 *  breakpoint can be placed. This should be after the registers have
 *  been saved if they are saved. Return the address.
 *
 *  If the function has line number information, just return the address
 *  of the first line.
 */
int
get_min_bpt_addr(f, p_addr, same_file)
func_t *f;
taddr_t *p_addr;
bool same_file;
{
	if (FU_LNOS(f) != NULL) {
	  lno_t *ln;

	  for (ln = FU_LNOS(f); ln != NULL; ln = ln->ln_next)
	  {			/* RGA skip itializiers etc in other files */
	    if (same_file == FALSE || ln->ln_fil == f->fu_fil) 
	    {
	      *p_addr = ln->ln_addr;
	      return 0;
	    }
	  }
	}
	return st_get_min_bpt_addr(f, p_addr);
}

/*  Set *p_addr to the address corresponding to line lnum in function f.
 *  Return 0 for success, -1 and an error message otherwise.
 */
int
map_lnum_to_addr(f, fil, lnum, p_addr)
func_t *f;
fil_t *fil;
int lnum;
taddr_t *p_addr;
{
	taddr_t addr, min_addr;

	if ((addr = lnum_and_fil_to_addr(f, fil, lnum)) == 1) {
		errf("No executable code at line %d of %s", lnum,
		     f->fu_fil->fi_name_only);
		return -1;
	}
	
	if (addr == 0) {
		errf("No line number information for %s", f->fu_demangled_name);
		return -1;
	}
	
	if (get_min_bpt_addr(f, &min_addr, FALSE) != 0)
		return -1;

	*p_addr = (addr > min_addr) ? addr : min_addr;
	return 0;
}

/*  Open the source file for fil if not already open.
 */
bool
open_source_file(fil, rematch, reload)
fil_t *fil;
bool rematch, reload;
{
  if (fil->fi_srcbuf == NULL) {
    fil->fi_srcbuf = srcpath_visit_file(fil->fi_symtab->st_apool,
					fil->fi_path_hint,
					fil->fi_name);
  }

  /* RGA start */
	
  if (fil->fi_srcbuf &&
      /*!(fil->fi_name && fil->fi_name[0] == '/') &&*/ /* RGA 8/14/97 */
      (!(fil->fi_flags & FI_DONE_MATCH) || rematch || reload))
  {
    int last_ln = 0, nlines, i, j, match = 0, file_access = -1, 
    old_path_found = 0, max_pathlen;
    funclist_t *fl;
    char *path_used, *opened_pl_path = NULL, *nbuf = NULL, *src_path, *c;
    Srcbuf *srcbuf = NULL, *opened_srcbuf = NULL;
    Pathentry *pl = NULL;
    FILE *fp = 0;

    if (rematch || reload)
    {
      if ((fp = fopen(get_temp_state_filename(), "w")) == NULL)
	errf("Can't create temp saved breakpoints file `%s'",
	     get_temp_state_filename());
      else
      {
	save_matching_breakpoints(NULL, fil, (char *)fp);
	remove_matching_breakpoints(NULL, fil);
      }
    }

    if (!reload)
    {
      for (fl = fil->fi_funclist; fl; fl = fl->fl_next)
      {
	FU_LNOS(fl->fl_func);
	if (!(fl->fl_func->fu_flags & FU_NOTHEADER))
        {
	  if (last_ln)
	    break;
	  else
             continue;
        }
	last_ln = fl->fl_func->fu_max_lnum;
      }
    }
    if (!(fil->fi_flags & FI_DONE_MATCH))
    {
      src_path = (char*)srcbuf_get_path(fil->fi_srcbuf);
      i = strlen(src_path);
      for (c = src_path + i -1; c && *c != '/'; c--, i--);
      fil->fi_source_path = alloc(fil->fi_symtab->st_apool, i);
      strncpy(fil->fi_source_path, src_path, i - 1);
      fil->fi_source_path[i - 1] = 0;
    }
    for (max_pathlen = 0, pl = get_source_path_root(); pl != NULL;
	 pl = pl->next)
      if (strlen(pl->path) > max_pathlen)
	max_pathlen = strlen(pl->path);
    if (!reload)
      for (pl = get_source_path_root(); pl != NULL; pl = pl->next)
      {
	if (nbuf != NULL)
	  free(nbuf);
	nbuf = e_malloc(max_pathlen + 1 + strlen(fil->fi_name) + 1);
	if (fil->fi_language == LANG_CC &&
	    ((rematch || reload) && !old_path_found && fil->fi_source_path))
	{
	  if (!strcmp(fil->fi_source_path, pl->path))
	    old_path_found = 1;
	  if (rematch || (reload && !old_path_found))
	    continue;
	}
	(void) sprintf(nbuf, "%s%s", pl->path, fil->fi_name);
	if ((file_access = access(nbuf, O_RDONLY)) != -1)
	{
	  path_used = (char *)pl->path;
	  srcbuf = srcpath_visit_file(fil->fi_symtab->st_apool,
				      pl->path,
				      fil->fi_name);
	  nlines = (long)srcbuf_nlines_in_file(srcbuf);
	  if (last_ln)
	  {
	    if (nlines >= last_ln)
	      j = nlines == last_ln ? last_ln - 1: last_ln;
	    else
	      j = 0;
	    if (j && !match)
	      for (i = 0; i < 2; i++, j--)
		if (strchr(srcbuf_get_file_line(srcbuf, j), '}'))
		{
		  match = 1;
		  break;
		}
	    if (match)
	    {
	      errf("\bUsing %s", nbuf);
	      fil->fi_source_path = path_used;
	      if (fil->fi_srcbuf)
		srcbuf_destroy(fil->fi_srcbuf);
	      fil->fi_srcbuf = srcbuf; 
	      fil->fi_flags |= FI_NEEDS_RESCAN;
	      break;
	    }
	    /* save info on first opened file */
	    /* in case it is the only match */
	    if (!opened_srcbuf)
	    {
	      opened_pl_path = (char *)pl->path;
	      opened_srcbuf = srcbuf;
	    }
	  }
	  else
	  {
	    if (srcbuf)
	      srcbuf_destroy(srcbuf);
	    break;
	  }
	}
      }
    if (pl == NULL)
    {
      if (opened_srcbuf)
      {
	(void) sprintf(nbuf, "%s%s", opened_pl_path, fil->fi_name);
	errf("\bUsing %s", nbuf);
	fil->fi_source_path = opened_pl_path;
	if (fil->fi_srcbuf)
	  srcbuf_destroy(fil->fi_srcbuf);
	fil->fi_srcbuf = opened_srcbuf; /* use first one that opened */
	fil->fi_flags |= FI_NEEDS_RESCAN;
      }
    }
    if (nbuf != NULL)
      free(nbuf);
    
    if (rematch || reload)
      if (!fil->fi_srcbuf)
      {
	if (rematch)
	  errf("Can't rematch source file `%s' in search path list",
	       fil->fi_name_only);
	else
	  errf("Can't reload source file `%s' in search path list",
	       fil->fi_name_only);
	return FALSE;
      }
    if (fil->fi_language != LANG_CC && reload && fil->fi_srcbuf)
    {
      fil->fi_flags |= FI_NEEDS_RESCAN;
      if (fil->fi_srcbuf)
	srcbuf_destroy(fil->fi_srcbuf);
      fil->fi_srcbuf = srcpath_visit_file(fil->fi_symtab->st_apool,
					  fil->fi_path_hint,
					  fil->fi_name);
    }
    fil->fi_flags |= FI_DONE_MATCH;
    if (rematch || reload)
      if (fp != NULL)
	restore_target_state(FALSE, TRUE);
  }
  /* RGA end */		
	
  return fil->fi_srcbuf != NULL;	
}

bool
have_source_file(fil)
fil_t *fil;
{
	return fil->fi_srcbuf != NULL ||
	       srcpath_file_exists(fil->fi_path_hint, fil->fi_name);
}

void
show_file_and_target_dates(fil, fu_symtab)
fil_t *fil;
symtab_t *fu_symtab;
{
#ifdef OS_SUNOS_4
  time_t local_time;
#endif /* OS_SUNOS_4 */
  time_t file_time;
  struct tm *tm_ptr;
  char time_str[32];
  Outwin *ow;
  char buff[256];
  target_t *xp;
	  
  ow = get_or_create_outwin();

  if (fil && fil->fi_srcbuf)
  {
    srcbuf_get_buffer_modtime(fil->fi_srcbuf, (long *)&file_time);
    tm_ptr = localtime(&file_time);
#ifdef OS_SUNOS_4
    local_time = timelocal(tm_ptr);
    file_time += file_time - local_time;
    tm_ptr = localtime(&file_time);
#endif /* OS_SUNOS_4 */
    strcpy(time_str, (char*)asctime(tm_ptr));
    time_str[strlen(time_str) - 1] = 0; /* no '\n' */
    if (fil->fi_name && fil->fi_name[0] == '/')
      sprintf(buff, "%s for source %s\n", time_str, fil->fi_name);
    else
      sprintf(buff, "%s for source %s%s%s\n",
	      time_str,
	      fil->fi_source_path,
	      (fil->fi_source_path
	       [strlen(fil->fi_source_path)-1] == '/') ?
	      "":"/", fil->fi_name_only ? fil->fi_name_only : "(unknown)");
    outwin_insert(ow,  buff, strlen(buff));   
  }
  
  if (fu_symtab || (fil && fil->fi_symtab && fil->fi_symtab->st_modtime))
  {
    if (fil && fil->fi_symtab && fil->fi_symtab->st_modtime)
      file_time = (time_t)fil->fi_symtab->st_modtime;
    else
      file_time = fu_symtab->st_modtime;
    tm_ptr = localtime(&file_time);
#ifdef OS_SUNOS_4
    local_time = timelocal(tm_ptr);
    file_time += file_time - local_time;
    tm_ptr = localtime(&file_time);
#endif /* OS_SUNOS_4 */
    strcpy(time_str, (char*)asctime(tm_ptr));
    time_str[strlen(time_str) - 1] = 0; /* no '\n' */
    if (fil && fil->fi_symtab && fil->fi_symtab->st_modtime)
      sprintf(buff, "%s for shared library %s\n", time_str,
	      fil->fi_symtab->st_path);
    else
      sprintf(buff, "%s for shared library %s\n", time_str,
	      fu_symtab->st_path);
    outwin_insert(ow,  buff, strlen(buff));   
  }

  xp = get_current_target();
  file_time = (time_t)xp->xp_modtime;
  tm_ptr = localtime(&file_time);
#ifdef OS_SUNOS_4
  local_time = timelocal(tm_ptr);
  file_time += file_time - local_time;
  tm_ptr = localtime(&file_time);
#endif /* OS_SUNOS_4 */
  strcpy(time_str, (char*)asctime(tm_ptr));
  time_str[strlen(time_str) - 1] = 0; /* no '\n' */
  sprintf(buff, "%s for target %s\n", time_str, xp->xp_textpath);
  outwin_insert(ow,  buff, strlen(buff));   
}

void
restore_cached_symtab_breakpoints()
{
  restore_target_state(FALSE, TRUE);
}

void
save_matching_breakpoints(st, fil, handle)
     symtab_t *st;
     char *handle;
     fil_t *fil;
{
  save_matching_bpts(st, fil, handle);
}

void
collapse_srcfiles()
{
  collapse_files();
}

bool
scan_elf_dot_o_file_symtab(fil)
fil_t *fil;
{
#ifdef AO_ELF
  return elf_scan_dot_o_file_symtab(fil);
#else
  return FALSE;
#endif /* !AO_ELF */
}

char *
get_temp_state_filename()
{
  static char filename[32];

  if (!*filename)
    sprintf(filename, "/tmp/ups-state.%d", (int)getpid());
  return filename;
}
