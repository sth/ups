/* ao_elflib.h - header file for ao_elflib.c */

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


/* @(#)ao_elflib.h	1.1 24/5/95 (UKC) */

#ifndef ELFINFO_TYPEDEFED
typedef struct Elfinfo Elfinfo;
#define ELFINFO_TYPEDEFED
#endif

#ifndef SOLIB_TYPEDEFED
typedef struct Solib Solib;
typedef struct Solib_addr Solib_addr;
#define SOLIB_TYPEDEFED
#endif

#ifdef TARGET_H_INCLUDED
bool elf_next_symtab PROTO((target_t *xp, symtab_t *st, bool load_new,
			    symtab_t **p_next_st));

void elf_zap_solib_addrs PROTO((target_t *xp));

void dump_elf_libs PROTO((target_t *xp));

void elf_rescan_dynamic_solibs PROTO((target_t *xp, bool check_only));

void load_from_solibs PROTO((target_t *xp, char *name));

void elf_make_main_symtab PROTO((target_t *xp, bool target_updated, int pid));

func_t *elf_decode_plt PROTO((target_t *xp, taddr_t pc));
#endif

#ifdef AO_TARGET_H_INCLUDED
bool elf_note_shlib_addr PROTO((alloc_pool_t *ap, iproc_t *ip, dev_t dev, ino_t ino,
				off_t size, long mtv_sec,  long mtv_nsec,
				off_t offset, taddr_t vaddr, size_t pagesize));

bool elf_get_core_shlib_info PROTO((alloc_pool_t *ap, iproc_t *ip));
#endif

struct func_s;
bool elf_get_dynamic_solibs PROTO((alloc_pool_t *target_ap, const char *path,
				   Solib **p_solib, bool check_only, bool save_state));

void elf_update_dynamic_solibs PROTO((alloc_pool_t *target_ap, const char *path,
				   Solib **p_solib, bool load_libs));

bool scan_main_elf_symtab PROTO((alloc_pool_t *target_ap, const char *path,
				 int fd, long modtime, Solib **p_solib,
				 Solib_addr **p_solib_addrs,
				 int *p_addrsize, taddr_t *p_entryaddr,
				 struct func_s **p_mainfunc,
				 struct func_s **p_initfunc,
				 bool target_updated, int pid));

bool add_solib_entry PROTO((alloc_pool_t *ap, symtab_t *st, func_t *flist,
                            Elfinfo *el, Solib **p_solibs));

void reset_main_debug_address PROTO((void));
char *get_unloaded_lib PROTO((taddr_t addr));
