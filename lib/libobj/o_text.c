/* o_text.c - routines to convert between ASCII text and objects */

/*  Copyright 1992 Mark Russell, University of Kent at Canterbury.
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


/* @(#)o_text.c	1.1 29/6/92 (UKC) */
char obj_text_rcsid[] = "$Id$";

#include <local/wn.h>

#include <string.h>

#include <local/ukcprog.h>

#include "obj.h"
#include "o_priv.h"
#include "o_mkobj.h"

static struct objst *name_to_obj PROTO(( struct objst *par, const char *name));

void
obj_to_vec(alloc_id, code, p_vec, p_ncomp)
alloc_id_t alloc_id;
objid_t code;
const char ***p_vec;
int *p_ncomp;
{
	int comp, ncomp;
	const char **vec;
	struct objst *obj, *baseobj;

	baseobj = code_to_obj(code);

	ncomp = 0;
	for (obj = baseobj; obj != &Rootobj; obj = obj->ob_parent) {
		if (obj == NULL)
			panic("par botch in otv");
		++ncomp;
	}
	
	vec = (const char **)alloc(alloc_id, ncomp * sizeof(const char *));

	comp = ncomp;
	for (obj = baseobj; obj != &Rootobj; obj = obj->ob_parent) {
		const char *name;

		name = (*Odesc[obj->ob_type].od_get_name)(obj->ob_code);
		vec[--comp] = alloc_strdup(alloc_id, name);
	}
	
	*p_vec = vec;
	*p_ncomp = ncomp;
}

static struct objst *
name_to_obj(par, name)
struct objst *par;
const char *name;
{
	struct objst *obj;
	const char *objname;

	for (obj = par->ob_child; obj != NULL; obj = obj->ob_next) {
		objname = (*Odesc[obj->ob_type].od_get_name)(obj->ob_code);

		if (strcmp(objname, name) == 0)
			return obj;
	}

	return NULL;
}

objid_t
vec_to_obj(vec, ncomp)
const char **vec;
int ncomp;
{
	struct objst *obj;
	int i;

	obj = &Rootobj;

	for (i = 0; i < ncomp; i++)
		if ((obj = name_to_obj(obj, vec[i])) == NULL) {
			return NULL;
	}
	
	return obj->ob_code;
}
