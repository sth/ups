/* ao_shlib.h - public header file for ao_shlib.c */

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


/* @(#)ao_shlib.h	1.2 24 May 1995 (UKC) */

int make_symtab_cache PROTO((const char *textpath, symtab_t **p_stcache));
symtab_t *get_symtab_cache_list PROTO((void));

bool aout_next_symtab PROTO((target_t *xp, symtab_t *st, bool load_new,
			     symtab_t **p_next_st));
			     
int load_shared_library_symtabs PROTO((target_t *xp, int adding_libs_only));
void unload_shared_library_symtabs PROTO((target_t *xp));
char *get_unloaded_lib PROTO((taddr_t addr));
void free_shlib_list PROTO((target_t *xp));
