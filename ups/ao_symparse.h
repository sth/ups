/* ao_symparse.h - public header file for ao_symparse.c */

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


/* @(#)ao_symparse.h	1.3 24 May 1995 (UKC) */

#ifndef ST_TE
type_t *TypeId PROTO((stf_t *stf, Symrec *sr, const char **p_s,
		      int eval, int prior_chr));
void Typenum PROTO((stf_t *stf, Symrec *sr, bool assume_paren,
		    const char **p_s, int *p_fnum, int *p_tnum));
type_t *Class PROTO((stf_t *stf, Symrec *sr, const char **p_s,
		     class_t *p_class));
int parse_num PROTO((stf_t *stf, Symrec *sr, const char **p_s));
void scheck PROTO((Symrec *sr, const char **p_s, int ch));
int field_scheck PROTO((Symrec *sr, const char **p_s, int ch));
void wrapup_types PROTO((stf_t *stf));
#endif /* !ST_TE */

