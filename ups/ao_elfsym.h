/* ao_elfsym.h - header file for ao_elfsym.c */

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


/* @(#)ao_elfsym.h	1.2 04 Jun 1995 (UKC) */

void elf_handle_optsym PROTO((alloc_pool_t *ap, const char *opts,
			      language_t* language,
			      const char **p_global_prefix,
			      Compiler_type *p_compiler_type,
			      bool *p_has_debug_syms));

const char *elf_name_from_symtab_name PROTO((const char *prefix,
					     const char *symtab_name));

bool elf_scan_then_setup_symio PROTO((fil_t *fil, symio_t **p_symio));

#ifdef AO_ELFPRIV_H_INCLUDED
bool scan_elf_symtab PROTO((alloc_pool_t *target_ap,
			    const char *textpath, int fd,
			    Libdep *libdep,
			    taddr_t *p_entryaddr,
			    struct func_s **p_mainfunc,
			    struct func_s **p_initfunc,
			    Solib **p_solibs,
			    bool target_updated));
#endif
