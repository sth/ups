/* st_debug.h - header file for st_debug.c */

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


/* @(#)st_debug.h	1.1 04 Jun 1995 (UKC) */

#ifdef TARGET_H_INCLUDED
void debug_dump_symbols PROTO((target_t *xp, const char *name));
void debug_load_symbols PROTO((target_t *xp, const char *name));
#endif

#if WANT_DEBUG
void dump_header PROTO((const char *s));
void dump_trailer PROTO((void));
void dump_enum_member_t PROTO((enum_member_t *em, int level));
void dump_aggr_or_enum_def_t PROTO((aggr_or_enum_def_t *ae, int ty_code,
				    int level, bool follow));
void dump_type_t PROTO((type_t *t, int level, bool follow));
void dump_dim_t PROTO((dim_t *dim, int level));
void dump_typedef_t PROTO((typedef_t *td, int level, bool follow));
void dump_dtype_t PROTO((dtype_t *dt, int level, bool follow));
void dump_var_t PROTO((var_t *v, int level, bool follow));
void dump_func_t PROTO((func_t *f, bool follow));
void dump_lno_t PROTO((lno_t *lno, bool follow));
void dump_block_t PROTO((block_t *bl, int level, bool follow));
void dump_funclist_t PROTO((funclist_t *fl, bool follow));
void dump_stf_t PROTO((stf_t *stf, bool follow));
void dump_fil_t PROTO((fil_t *fil, bool details, bool follow));
void dump_symtab_t PROTO((symtab_t *st, bool follow));
#endif
