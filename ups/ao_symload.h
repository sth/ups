/* ao_symload.h - public header file for ao_symload.c */

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


/* @(#)ao_symload.h	1.5 24 May 1995 (UKC) */

void push_typedefs_and_aggrs PROTO((alloc_pool_t *ap, block_t *hdrbl, 
                                    block_t *bl));
bool ao_fil_may_have_matching_globals PROTO((fil_t *fil, const char *pat, 
                                             matchfunc_t matchf));
var_t *ao_get_fi_vars PROTO((fil_t *fil));
block_t *ao_get_fu_blocks PROTO((func_t *f));
lno_t *ao_get_fu_lnos PROTO((func_t *f));
Compiler_type ao_compiler PROTO((fil_t *fil, bool compiler_found,
				 Compiler_type compiler));
				
void set_compiler_version(double ver);
double get_compiler_version();
