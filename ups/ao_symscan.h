/* ao_symscan.h - public header file for ao_symscan.c */

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


/* @(#)ao_symscan.h	1.2 20 Jun 1995 (UKC) */

language_t srctype PROTO((const char *name));

void parse_fname PROTO((alloc_pool_t *ap, const char *name,
			int modsepc, bool ext, 
                        const char **p_modname, const char **p_fname));

fsyminfo_t *make_fsyminfo PROTO((alloc_pool_t *ap, int symno));

stf_t *make_stf PROTO((alloc_pool_t *ap, const char *name, symtab_t *st,
		       int symno, language_t language, taddr_t addr));

fil_t *ao_make_fil PROTO((stf_t *stf, block_t *parblock, const char *path_hint, 
                          fil_t *next));

void scan_symtab PROTO((symtab_t *st, const char *path, stf_t *stf,
			func_t **p_flist, const char **p_mainfunc_name));

void add_function_to_symtab PROTO((symtab_t *st, func_t **p_flist, 
                                   const char *namestr,
				   fil_t *fil, fil_t *solfil,
				   bool is_static, bool is_textsym,
				   int symno, taddr_t addr));

snlist_t *push_symname PROTO((Symrec *sr, alloc_pool_t *ap,  stf_t *stf,
			      const char *sym_name, int symno));

bool symtab_name_to_sn PROTO((snlist_t *snlist, const char *name, 
                              const char *prefix, const char *fname,
			      bool use_allnames, snlist_t **p_sn));

bool symtab_sym_name_to_sn PROTO((snlist_t *snlist, const char *name, 
                              snlist_t **p_sn, bool justone));

const char *parse_name PROTO((Symrec *sr, const char **p_s,
			      alloc_pool_t *ap, int func,
			      Compiler_type compiler, bool is_fortran));

bool parse_number PROTO((stf_t *stf, Symrec *sr, const char **p_s, int *p_val));
bool parse_typenum PROTO((stf_t *stf, Symrec *sr, bool assume_paren,
			  const char **p_s,
			  int *p_fnum, int *p_tnum));

bool char_to_utypecode PROTO((int c, typecode_t *p_typecode));

void change_base_address PROTO((symtab_t *st, taddr_t new_addr));

bool find_sol_fil PROTO((fil_t *sfiles, const char *path_hint, const char *name,
			 fil_t **p_fil));

char bump_str PROTO((Symrec *sr, const char **p_s));

void wrapup_stf PROTO((stf_t *stf, hf_t **orig_fmap, int mapsize));
void add_to_fil_funclist PROTO((alloc_pool_t *ap, fil_t *fil, func_t *f));
void reverse_stf_funclist PROTO((stf_t *stf));

