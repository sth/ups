/* o_globals.c - definition and initialisation of obj globals */

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


/* @(#)o_globals.c	1.10 8/11/92 (UKC) */
char obj_globals_rcsid[] = "$Id$";

#include <local/wn.h>

#include <local/ukcprog.h>
#include "obj.h"
#include "o_priv.h"

/*  The object type table. This is a static - the public table
 *  header Odesc points at element 1 of this table. This is so
 *  we can use Odesc[-1] to store the type of the root object.
 *
 *  See o_obj.h for a description of the fields in odescst.
 */
static struct odescst Descs[MAX_OBJTYPES+1] = {
	{ 0, 0, 20, 0, OB_NL_AFTER|OB_FIXED_SIZE, NULL, NULL, NULL, NULL }
};

/*  The public pointer to the object type table Desc above
 */
struct odescst *Odesc = Descs + 1;

/* The format typechar table, with the default entries for
 * 's' and '*'. Note that '*' is a reserved format char - it
 * is used to print literals (see def.c).
 */
struct formatst Formats[MAX_FORMATS] = {
	{ 's', no_edit, s_draw, NULL },
	{ '*', no_edit, l_draw, NULL },
};

/*  Number of defined formats - increased if user defines new formats
 */
int Num_formats = 2;

/*  Window for displaying objects
 */
window_t Obj_wn = -1;

/*  The ancestor of all objects (analogous to / in the file system)
 */
struct objst Rootobj;	/* depend on fields of this being set to 0 */

/*  Obj change callback function (set via obj_set_callback_and_data).
 */
obj_change_callback_t obj__Change_callback = NULL;
char *obj__Change_callback_data;

