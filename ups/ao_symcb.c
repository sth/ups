/* ao_symcb.c - do initial symbol table scan */

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


/* @(#)ao_symcb.c	1.2 04 Jun 1995 (UKC) */
char ups_ao_symcb_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#ifdef AO_TARGET

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
#include "ao_symcb.h"
#include "ao_symread.h"
#include "ao_symscan.h"
#include "ao_symparse.h"

typedef struct cbstringst {
	const char *string;
	int symno;
} cbstring_t;

typedef struct ao_cbdata_s { 
	int nstrings;
	cbstring_t *strings;
	stf_t *stf;
	symio_t *symio;
} ao_cbdata_t;

static int get_cb_strings PROTO((ao_cbdata_t *acb, int symno));

int
push_common_block(st, stf, f, symio, symno)
symtab_t *st;
stf_t *stf;
func_t *f;
symio_t *symio;
int symno;
{
	nlist_t nm;
	bool empty;
	const char *name;
	ao_cbdata_t *acb;
	
	name = symstring(symio, symno++);

	getsym(symio, symno, &nm);
	empty = nm.n_type == N_ECOMM;

	acb = (ao_cbdata_t *)e_malloc(sizeof(ao_cbdata_t));

	acb->stf = stf;
	acb->symio = symio;
	
	if (empty) {
		acb->strings = NULL;
		acb->nstrings = 0;
	}
	else {
		symno = get_cb_strings(acb, symno);
	}

	add_common_block(st, stf->stf_fil, f, name, empty, (char *)acb);

	return symno;
}

bool
ao_cblocks_match(data, newdata, same_func)
char *data, *newdata;
bool same_func;
{
	ao_cbdata_t *acb, *new_acb;
	int i;

	acb = (ao_cbdata_t *)data;
	new_acb = (ao_cbdata_t *)newdata;

	if (acb->strings == NULL || same_func)
		return TRUE;

	if (acb->nstrings != new_acb->nstrings)
		return FALSE;

	for (i = 0; i < acb->nstrings; ++i) {
		if (strcmp(acb->strings[i].string,
			   new_acb->strings[i].string) != 0)
			return FALSE;
	}
	
	return TRUE;
}

/*  Copy the string pointers from the static vector returned by get_cb_strings.
 *
 *  If cb->strings not NULL, the strings are added to the end of the
 *  existing strings.
 */
void
ao_set_cblock_data(p_data, data, append)
char **p_data, *data;
bool append;
{
	cbstring_t *new_strings;
	int i, old_nstrings, new_nstrings;
	ao_cbdata_t *old_acb, *new_acb;

	old_acb = (ao_cbdata_t *)*p_data;
	new_acb = (ao_cbdata_t *)data;

	old_nstrings = append ? old_acb->nstrings : 0;
	new_nstrings = old_nstrings + new_acb->nstrings;

	new_strings = (cbstring_t *)e_malloc(new_nstrings * sizeof(cbstring_t));
	
	for (i = 0; i < old_nstrings; ++i)
		new_strings[i] = old_acb->strings[i];

	for (; i < new_nstrings; ++i)
		new_strings[i] = new_acb->strings[i - old_nstrings];

	if (old_acb != NULL)
		ao_free_cblock_data((char *)old_acb);

	new_acb->strings = new_strings;
	new_acb->nstrings = new_nstrings;
	
	*p_data = (char *)new_acb;
}

void
ao_free_cblock_data(data)
char *data;
{
	ao_cbdata_t *acb;

	acb = (ao_cbdata_t *)data;

	if (acb->strings != NULL)
		free((char *)acb->strings);

	free((char *)acb);
}

bool
ao_cblock_has_var(data, name)
char *data;
const char *name;
{
	cbstring_t *cbs, *lim;
	size_t len;
	ao_cbdata_t *acb;

	acb = (ao_cbdata_t *)data;

	cbs = acb->strings;
	lim = cbs + acb->nstrings;

	for (; cbs < lim; ++cbs) {
		len = strchr(cbs->string, ':') - cbs->string;
		if (strlen(name) == len &&
			     strncmp(cbs->string, name, len) == 0)
			return TRUE;
	}

	return FALSE;
}

/*  This routine points cb_strings at static data, as this common block
 *  may be identical to an already seen one.  If this one does turn out
 *  to be new or different, the strings are copied by add_and_free_cblock_info.
 */
static int
get_cb_strings(acb, symno)
ao_cbdata_t *acb;
int symno;
{
	static const char block_data[] = "block.data";
	static cbstring_t *strings;
	static int strings_size = 0;
	nlist_t nm;
	int nstrings;
	const char *string;

	if (strings_size == 0) {
		strings_size = 32;
		strings = (cbstring_t *)e_malloc((strings_size + 1) *
						 sizeof(cbstring_t));
	}

	nstrings = 0;
	for (;;) {
		getsym(acb->symio, symno, &nm);
		
		if (nm.n_type == N_ECOMM)
			break;
		
		string = symstring(acb->symio, symno);
		
		if (nm.n_type == N_FUN &&
		    strncmp(string, block_data, sizeof(block_data)-1) == 0) {
			++symno;
			continue;
		}
	
		if (nstrings >= strings_size) {
			strings_size *= 2;
			strings = (cbstring_t *) e_realloc((char *)strings,
					    strings_size * sizeof(cbstring_t));
		}
		
		if (string != NULL && strchr(string, '=') == NULL) {
			strings[nstrings].string = string;
			strings[nstrings].symno = symno;
			++nstrings;
		}

		++symno;
	}
	
	acb->strings = strings;
	acb->nstrings = nstrings;
	return symno;
}

var_t *
ao_get_cblock_vars(st, data, addr) 
symtab_t *st;
char *data;
taddr_t addr;
{
	var_t *v, *vlist;
	Symrec symrec;
	int i;
	const char *s, *name;
	type_t *type;
	class_t class;
	ao_cbdata_t *acb;
	nlist_t nm;

	acb = (ao_cbdata_t *)data;

	vlist = NULL;

#ifdef ST_TE
	panic("gcv NYI under ST_TE");
#endif

	symrec.symio = acb->symio;
	
	for (i = 0; i < acb->nstrings; ++i) {
		symrec.symno = acb->strings[i].symno;
		getsym(symrec.symio, symrec.symno, &nm);

		s = acb->strings[i].string;
		name = parse_name(&symrec, &s, st->st_apool, 0, CT_UNKNOWN, FALSE);
		scheck(&symrec, &s, ':');
		type = Class(acb->stf, &symrec, &s, &class);

		if (class != CL_NOCLASS && class != CL_FUNC) {
			v = ci_make_var(st->st_apool, name, class, type,
					addr + (taddr_t)nm.n_value);
			v->va_language = acb->stf->stf_language;
			v->va_next = vlist;
			vlist = v;
		}
	}
	
	return vlist;
}

#endif /* AO_TARGET */
