/* ao_aflist.h - header file for ao_aflist.c */

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


/* @(#)ao_aflist.h	1.1 24/5/95 (UKC) */

typedef struct Ambig_fil Ambig_fil;

fil_t *basename_to_maybe_ambig_fil PROTO((symtab_t *st, const char *name,
					  hashtab_t *ht, hashvalues_t *hv,
					  Ambig_fil **p_aflist));

void resolve_aflist PROTO((func_t *funclist, Ambig_fil *af));
