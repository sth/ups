/* st_cb.c - FORTRAN common block routines */

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


/* @(#)st_cb.c	1.22 04 Jun 1995 (UKC) */
char ups_st_cb_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <local/ukcprog.h>
#include <mtrprog/strcache.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "target.h"
#include "st.h"
#include "state.h"

/*  FORTRAN common block. 
 */
struct common_block_s {
	const char *cb_name;
	symtab_t *cb_symtab;
	fil_t *cb_fil;
	int cb_id;
	taddr_t cb_addr;
	func_t *cb_deffunc;
	bool cb_onedef;
	int cb_havevars;
	var_t *cb__vars;
	char *cb_data;
};

/*  List of common blocks that hangs off a func_t.
 */
struct cblist_s {
	common_block_t *cl_cblock;
	struct cblist_s *cl_next;
};

static common_block_t *search_cblist PROTO((cblist_t *cl, const char *name));

void
free_cblist_info(cl)
cblist_t *cl;
{
	for (; cl != NULL; cl = cl->cl_next) 
		st_free_cblock_data(cl->cl_cblock);
}

void
finish_common_blocks(st)
symtab_t *st;
{
	cblist_t *cl;
	common_block_t *cb, *lastcb;
	int same_count, id;

	same_count = 0;	/* just to keep lint happy */

	lastcb = NULL;
	id = 0;

	for (cl = st->st_cblist; cl != NULL; cl = cl->cl_next) {
		cb = cl->cl_cblock;

		cb->cb_onedef = FALSE;
		if (cb->cb_id != id) {
			const char *name;
			char *namecopy;
			size_t len;
			
			if (lastcb != NULL && same_count == 1)
				lastcb->cb_onedef = TRUE;
			same_count = 1;

			name = cb->cb_name;
			len = strlen(name);
			if (len > 4 && strcmp(&name[len - 3], ".i_") == 0) {
				namecopy = strsave(name);
				namecopy[len - 2] = 'u';
				name = namecopy;
			}
			else {
				namecopy = NULL;
			}
			
			cb->cb_addr = lookup_global_addr(st, name);

			if (namecopy != NULL)
				free(namecopy);
			
			id = cb->cb_id;
		}
		else {
			++same_count;
			cb->cb_addr = lastcb->cb_addr;
		}
		lastcb = cb;
	}

	if (lastcb != NULL && same_count == 1)
		lastcb->cb_onedef = TRUE;
}

void
add_common_block(st, fil, f, name, empty, data)
symtab_t *st;
fil_t *fil;
func_t *f;
const char *name;
bool empty;
char *data;
{
	static int id = 0;
	cblist_t *cl, *first_cl, *same_cl, *prev_cl;
	common_block_t *cb;
	int this_id;

	/*  Seen this one already?
	 */
	first_cl = prev_cl = NULL;
	for (cl = st->st_cblist; cl != NULL; cl = cl->cl_next) {
		if (strcmp(cl->cl_cblock->cb_name, name) == 0) {
			first_cl = cl;
			break;
		}
		prev_cl = cl;
	}
	this_id = (first_cl != NULL) ? first_cl->cl_cblock->cb_id : ++id;

	if (empty && first_cl != NULL) {
		cb = first_cl->cl_cblock;
	}
	else {
		same_cl = NULL;
		for (cl = first_cl; cl != NULL; cl = cl->cl_next) {
			cb = cl->cl_cblock;

			if (cb->cb_id != this_id)
				break;

			if (st_cblocks_match(cb, data, cb->cb_deffunc == f)) {
				same_cl = cl;
				break;
			}
		}

		if (same_cl != NULL) {
			cb = same_cl->cl_cblock;
		}
		else {
			/*  same_cl == NULL => this is a new common block
			 *  definition (first_cl == NULL), or a common block
			 *  definition which doesn't agree with an existing
			 *  one (legal in FORTRAN).
			 */
			cb = (common_block_t *)alloc(st->st_apool,
						     sizeof(common_block_t));
			cb->cb_name = strsave(name);
			cb->cb_symtab = st;
			cb->cb_fil = fil;
			cb->cb_addr = 0;
			cb->cb_data = NULL;
			cb->cb_id = this_id;
			cb->cb_deffunc = f;
			cb->cb_havevars = FALSE;
			cb->cb__vars = NULL;

			/*  Insert the new block in the global list.
			 */
			cl = (cblist_t *)alloc(st->st_apool, sizeof(cblist_t));
			cl->cl_cblock = cb;
			if (prev_cl == NULL) {
				cl->cl_next = st->st_cblist;
				st->st_cblist = cl;
			}
			else {
				cl->cl_next = prev_cl->cl_next;
				prev_cl->cl_next = cl;
			}
		}

		st_set_cblock_data(cb, data,
				   same_cl != NULL && cb->cb_deffunc == f);
	}
	
	/*  Add the common block to the list for this function.
	 */
	if (f != NULL) {
		cl = (cblist_t *) alloc(st->st_apool, sizeof(cblist_t));
		cl->cl_cblock = cb;
		cl->cl_next = f->fu_cblist;
		f->fu_cblist = cl;
	}
}

static common_block_t *
search_cblist(cl, name)
cblist_t *cl;
const char *name;
{
	for (; cl != NULL; cl = cl->cl_next) {
		if (st_cblock_has_var(cl->cl_cblock, name))
			return cl->cl_cblock;
	}
	return NULL;
}

void
iterate_over_cblist(cblist, func)
cblist_t *cblist;
void (*func)PROTO((common_block_t *cblock));
{
	cblist_t *cl;

	for (cl = cblist; cl != NULL; cl = cl->cl_next)
		(*func)(cl->cl_cblock);
}

func_t *
get_cblock_func(cb)
common_block_t *cb;
{
	return cb->cb_deffunc;
}

const char *
get_cblock_funcname(cb)
common_block_t *cb;
{
	return (cb->cb_onedef || cb->cb_deffunc == NULL)
					? NULL : cb->cb_deffunc->fu_name;
}

const char *
get_cblock_name(cb)
common_block_t *cb;
{
	return cb->cb_name;
}

const symtab_t *
get_cblock_symtab(cb)
common_block_t *cb;
{
	return cb->cb_symtab;
}

common_block_t *
name_to_cblock(name, f)
const char *name;
func_t *f;
{
	target_t *xp;
	symtab_t *st;

	xp = get_current_target();
	st = NULL;
	
	while (xp_next_symtab(xp, st, TRUE, &st)) {
		cblist_t *cl;
		
		for (cl = st->st_cblist; cl != NULL; cl = cl->cl_next) {
			common_block_t *cb;

			cb = cl->cl_cblock;
			
			if ((f == NULL || f == cb->cb_deffunc) &&
			    strcmp(cb->cb_name, name) == 0)
				return cb;
		}
	}

	return NULL;
}

common_block_t *
name_and_st_to_cblock(name, f, st)
const char *name;
func_t *f;
symtab_t *st;
{
	cblist_t *cl;
		
	for (cl = st->st_cblist; cl != NULL; cl = cl->cl_next) {
		common_block_t *cb;

		cb = cl->cl_cblock;
			
		if ((f == NULL || f == cb->cb_deffunc) &&
		    strcmp(cb->cb_name, name) == 0)
			return cb;
	}

	return NULL;
}

fil_t *
get_cblock_fil(cb)
common_block_t *cb;
{
	return cb->cb_fil;
}

void
global_and_cblist_to_var(cblist, name, f, p_cblock, p_fil, p_var)
cblist_t *cblist;
const char *name;
func_t *f;
common_block_t **p_cblock;
fil_t **p_fil;
var_t **p_var;
{
	common_block_t *cb;

	if (f == NULL || (cb = search_cblist(f->fu_cblist, name)) == NULL)
		cb = search_cblist(cblist, name);

	if (cb == NULL) {
		*p_var = NULL;
	}
	else {
		var_t *v;

		for (v = get_cblock_vars(cb); v != NULL; v = v->va_next) {
			if (strcmp(name, v->va_name) == 0) {
				*p_fil = cb->cb_fil;
				*p_var = v;
				break;
			}
		}

		*p_cblock = cb;
	}
}

var_t *
get_cblock_vars(cb)
common_block_t *cb;
{
	if (!cb->cb_havevars) {
		cb->cb__vars = st_read_cblock_vars(cb);
		cb->cb_havevars = TRUE;
	}

	return cb->cb__vars;
}
