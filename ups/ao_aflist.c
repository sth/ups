/* ao_aflist.c - try to resolve ambiguous SunOS ELF .symtab file entries */

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


/* @(#)ao_aflist.c	1.2 04 Jun 1995 (UKC) */
char ups_ao_aflist_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#ifdef AO_ELF

#include <sys/types.h>
#include <string.h>

#include <local/ukcprog.h>
#include <mtrprog/hash.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "st.h"
#include "ao_aflist.h"
#include "ao_syms.h"
#include "ao_symscan.h"

typedef struct Fil_list {
	fil_t *fil;
	struct Fil_list *next;
} Fil_list;

struct Ambig_fil {
	const char *name;
	fil_t *dummy_fils;	/* Dummy fil structs from .symtab section */
	Fil_list *stab_fils;	/* All .stab.index fils with matching names */
	Ambig_fil *next;
};

static void
push_ambig_fil(alloc_pool_t *ap, Ambig_fil **p_aflist, const char *name)
{
	Ambig_fil *af;
	
	af = (Ambig_fil *)alloc(ap, sizeof(Ambig_fil));
	af->name = alloc_strdup(ap, name);
	af->dummy_fils = NULL;
	af->stab_fils = NULL;
	af->next = *p_aflist;
	*p_aflist = af;
}		

static void
push_stab_fil(alloc_pool_t *ap, Ambig_fil *af, fil_t *fil)
{
	Fil_list *fl;

	fl = (Fil_list *)alloc(ap, sizeof(Fil_list));
	fl->fil = fil;
	fl->next = af->stab_fils;
	af->stab_fils = fl;
}

static fil_t *
push_dummy_fil(alloc_pool_t *ap, Ambig_fil *af)
{
	fil_t *fil;
	stf_t *stf;
	
	/*  Add a dummy stf structure.  The only field we will use is
	 *  stf_range.
	 */
	stf = make_stf(ap, af->name, (symtab_t *)NULL, 0, LANG_C, 0);

	fil = ci_make_fil(ap, af->name, (char *)stf, (block_t *)NULL,
			  af->dummy_fils);
	stf->stf_fil = fil;
	
	af->dummy_fils = fil;

	return fil;
}

fil_t *
basename_to_maybe_ambig_fil(symtab_t *st, const char *name,
			    hashtab_t *ht, hashvalues_t *hv,
			    Ambig_fil **p_aflist)
{
	fil_t *matchfil;
	Ambig_fil *af;
	alloc_pool_t *ap;
	const char *path, *slash;
	
	ap = st->st_apool;
	
	for (af = *p_aflist; af != NULL; af = af->next) {
		if (strcmp(name, af->name) == 0)
			return push_dummy_fil(ap, af);
	}

	matchfil = NULL;

	/*  SC f77 puts path names in STT_FILE syms.  SC cc doesn't.
	 */
	slash = strrchr(name, '/');
	if (slash != NULL && slash[1] != '\0') {
		path = name;
		name = slash + 1;
	}
	else {
		path = NULL;
	}
		
	if (hash_lookup_key(ht, name, strlen(name), hv)) {
		int i;
		
		for (i = 0; i < hv->hv_nvalues; ++i) {
			fil_t *fil;

			fil = (fil_t *)hv->hv_values[i];

			if (path != NULL && strcmp(fil->fi_name, path) != 0)
				continue;
			
			if (matchfil != NULL) {
				if (af == NULL) {
					push_ambig_fil(ap, p_aflist, name);
					af = *p_aflist;
					push_stab_fil(ap, af, matchfil);
				}
				
				push_stab_fil(ap, af, fil);
			}
			else {
				matchfil = fil;
			}
		}
	}

	return (af == NULL) ? matchfil : push_dummy_fil(ap, af);
}

static void
find_smallest_delta(taddr_t addr, funclist_t *funclist,
		    off_t *p_smallest_delta, bool *p_in_between)
{
	funclist_t *fl;
	bool found_earlier, found_later;
	off_t smallest_delta;

	found_earlier = found_later = FALSE;
	smallest_delta = 0;
	
	for (fl = funclist; fl != NULL; fl = fl->fl_next) {
		off_t delta;

		delta = fl->fl_func->fu_addr - addr;
		
		if (delta > 0) {
			if (found_earlier)
				break;
			found_later = TRUE;
		}
		else {
			if (found_later)
				break;
			found_earlier = TRUE;
			delta = -delta;
		}

		if (smallest_delta == 0 || delta < smallest_delta)
			smallest_delta = delta;
	}

	*p_in_between = found_earlier && found_later;
	*p_smallest_delta = smallest_delta;
}

static void
transfer_fil_lists(fil_t *dummy_fil, fil_t *stab_fil)
{
	funclist_t *fl, *next;
	stf_t *dummy_stf, *stab_stf;
	
	for (fl = dummy_fil->fi_funclist; fl != NULL; fl = next) {
		next = fl->fl_next;

		fl->fl_func->fu_fil = stab_fil;
		
		fl->fl_next = stab_fil->fi_funclist;
		stab_fil->fi_funclist = fl;
	}
	dummy_fil->fi_funclist = NULL; /* for neatness */

	dummy_stf = AO_FIDATA(dummy_fil);
	stab_stf = AO_FIDATA(stab_fil);

	if (stab_stf)		/* RGA  */
	  memcpy(stab_stf->stf_range, dummy_stf->stf_range,
		 sizeof(stab_stf->stf_range));
	
	/* TODO: variables */
}

static Fil_list *
find_closest_stab_fil(Ambig_fil *af, fil_t *dummy_fil)
{
	funclist_t *fl;
	Fil_list *bestsf;
	off_t smallest_delta;

	/*  If there are no functions to go by, just take the first on
	 *  the list.  This means that if there is no better information,
	 *  we just pair off the dummy_fils and stab_fils in order.
	 *  The orders of the .symtab file and .stab N_SO symbols correspond,
	 *  so this gets the right answer.
	 *
	 *  TODO: match up based on variable addresses.
	 */
	bestsf = af->stab_fils;

	if (bestsf == NULL || bestsf->next == NULL)
		return bestsf;
	
	smallest_delta = 0;

	for (fl = dummy_fil->fi_funclist; fl != NULL; fl = fl->fl_next) {
		Fil_list *sf, *prev;
		
		prev = NULL;
		
		for (sf = af->stab_fils; sf != NULL; sf = sf->next) {
			off_t delta;
			bool in_between;
			
			find_smallest_delta(fl->fl_func->fu_addr,
					    sf->fil->fi_funclist,
					    &delta, &in_between);

			if (in_between) {
				bestsf = sf;
				break;
			}

			if (delta != 0 &&
			    (smallest_delta == 0 || delta < smallest_delta)) {
				bestsf = sf;
				smallest_delta = delta;
			}
		}
		
	}

	return bestsf;
}

static void
resolve_dummy_fil(Ambig_fil *af, fil_t *df)
{
	Fil_list *sf;
	
	sf = find_closest_stab_fil(af, df);

	if (sf != NULL) {
		Fil_list *sf2, *prev;
			
		transfer_fil_lists(df, sf->fil);

		/*  We use the fi_symtab field here to communicate with
		 *  resolve_aflist() below.  Any unused struct pointer
		 *  field would do.
		 */
		df->fi_symtab = (symtab_t *)sf->fil;

		prev = NULL;
		for (sf2 = af->stab_fils; sf2 != sf; sf2 = sf2->next)
			prev = sf2;
			
		if (prev != NULL)
			prev->next = sf->next;
		else
			af->stab_fils = sf->next;
	}
	
	df->fi_name = NULL;	/* Mark as done for resolve_aflist() */
}
		
void
resolve_aflist(func_t *funclist, Ambig_fil *aflist)
{
	Ambig_fil *af, *new_aflist;
	func_t *f;

	new_aflist = NULL;
	
	for (af = aflist; af != NULL; af = af->next) {
		fil_t *df;

		/*  Search the ones with local functions first, as these
		 *  give us a handle to get a best match on.
		 */
		for (df = af->dummy_fils; df != NULL; df = df->fi_next) {
			if (df->fi_funclist != NULL)
				resolve_dummy_fil(af, df);
		}

		for (df = af->dummy_fils; df != NULL; df = df->fi_next) {
			if (df->fi_name != NULL)
				resolve_dummy_fil(af, df);
		}
	}

	for (f = funclist; f != NULL; f = f->fu_next) {
		fsyminfo_t *fs;
		fil_t *fil;

		fs = AO_FSYMDATA(f);
		fil = fs->fs_initial_lno_fil;

		if (fil != NULL && fil->fi_name == NULL)
			fs->fs_initial_lno_fil = (fil_t *)fil->fi_symtab;
	}
}

#endif /* AO_ELF */
