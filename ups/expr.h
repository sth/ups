/* expr.h - header file for expr.c */

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


/* @(#)expr.h	1.7 04 Jun 1995 (UKC) */

bool get_varname PROTO((language_t language, const char *text, const char *pos,
			size_t *p_start, size_t *p_end));

func_t *lnum_to_func PROTO((fil_t *fil, int lnum));

#ifdef OBJ_H_INCLUDED
int display_local PROTO((objid_t par, func_t *f, int lnum, const char *name,
			 bool restoring, objid_t *p_var_obj, 
			 bool double_click));

bool show_local_expr PROTO((fil_t *fil, int lnum, const char *text));
#endif

#ifdef SRCWIN_H_INCLUDED
void show_var_from_typing_line PROTO((Srcwin *sw, const char *name));
int show_var PROTO((Srcwin *sw, fil_t *fil, int lnum, const char *wholename,
		     errf_ofunc_t out_f, int control_down, bool double_click));

#ifdef ST_H_INCLUDED
bool show_global PROTO((Srcwin *sw, fil_t *srcfil, func_t *srcfunc,
			common_block_t *srccblock,
			const char *name, bool want_errmesg,
			bool from_typing_line, bool restoring,
			objid_t *p_var_obj, bool undef_check,
			bool double_click));
void show_given_global PROTO((fil_t *fil, var_t *v));
#endif

#endif

bool show_static_member PROTO((type_t* type,var_t* v));
