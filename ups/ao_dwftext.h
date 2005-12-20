/* ao_dwftext.h - routine definitions for ao_dwftext.c */

/*  Copyright (C) 2002 Ian Edwards
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

/* $Id$ */

#if WANT_DWARF

int dw_read_text PROTO((symtab_t *st, taddr_t addr, char *buf, size_t nbytes));
int dw_get_min_bpt_addr PROTO((func_t *f, taddr_t *p_addr));
void dw_close_symtab_data PROTO((symtab_t *st));

bool dw_cblocks_match PROTO((char *data, char *newdata, bool same_func));
void dw_set_cblock_data PROTO((char **p_data, char *data, bool append));
void dw_free_cblock_data PROTO((char *data));
bool dw_cblock_has_var PROTO((char *data, const char *name));
var_t * dw_get_cblock_vars PROTO((symtab_t *st, char *data, taddr_t addr));

func_t * dw_ensure_fu PROTO((func_t *f));
fil_t * dw_get_fi PROTO((symtab_t *st));
lno_t * dw_get_fu_lnos PROTO((func_t *f));
block_t * dw_get_fu_blocks PROTO((func_t *f));
var_t * dw_get_fi_vars PROTO((fil_t *fil));
macro_t * dw_get_fi_macros PROTO((fil_t *fil));

bool dw_unwind PROTO((target_t *xp, symtab_t *st, taddr_t *fp,
                      taddr_t *sp, taddr_t *pc, taddr_t *cfa));

#endif /* WANT_DWARF */
