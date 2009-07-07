/* ao_text.h - public header file for ao_text.c */

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


/* @(#)ao_text.h	1.2 24 May 1995 (UKC) */

bool open_textfile PROTO((int fd, const char *textpath, taddr_t *p_data_addr));

bool text_dread PROTO((target_t *xp, taddr_t addr, char *buf, size_t nbytes));

#ifdef SYMTAB_H_INCLUDED
bool get_preamble PROTO((func_t *f, ao_preamble_t **p_pr));
void ao_close_symtab_data PROTO((symtab_t *st));

#ifdef AO_EXECINFO_H_INCLUDED
bool scan_ao_symtab PROTO((const char *textpath, int fd, Execinfo *ei, 
                           Dwarf_Debug dw_dbg, Dwarf_Debug dw_debug_dbg,
                           taddr_t base_address, symtab_type_t st_is,
                           symtab_t **p_symtab, func_t **p_flist,
                           const char **p_mainfunc_name));

void do_ao_postscan_stuff PROTO((symtab_t *st, func_t *flist, Execinfo *ei, 
				 func_t **p_mainfunc, 
				 const char *mainfunc_name, 
				 func_t **p_initfunc, 
				 const char *initfunc_name));
#endif /* EXECINFO_H_INCLUDED */

#endif /* SYMTAB_H_INCLUDED */
