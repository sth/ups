/* ao_elfread.h - header file for ao_elfread.c */

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


/* @(#)ao_elfread.h	1.1 24/5/95 (UKC) */

#ifndef ELFINFO_TYPEDEFED
typedef struct Elfinfo Elfinfo;
#define ELFINFO_TYPEDEFED
#endif

#ifndef LIBDEP_TYPEDEFED
typedef struct Libdep Libdep;
#define LIBDEP_TYPEDEFED
#endif

#ifdef AO_EXECINFO_H_INCLUDED
bool elf_get_exec_info PROTO((const char *textpath, int fd, Libdep *libdep,
			      Execinfo *ex, Elfinfo **p_el, symtab_type_t *p_st_is));
#endif

void free_elfinfo PROTO((Elfinfo *el));

int check_elf_exec_header PROTO((int fd, const char *path));

#ifdef AO_ELFPRIV_H_INCLUDED
void ar_get_path_and_member PROTO((const char *objpath,
				   char **p_path, char **p_member));

bool ar_get_member_offset PROTO((const char *path, int fd, const char *member,
				 off_t *p_offset));

bool elf_find_section PROTO((Elfinfo *el, const char *secname,
			     const char *what, Elf32_Shdr **p_sh));
bool check_elf_file_type PROTO((int fd, const char *path, int type,
				const char *action));
bool check_elf_header_type PROTO((Elf32_Ehdr *eh, const char *path, int type,
				  const char *action));
bool prepare_to_parse_elf_file PROTO((const char *path, int fd, Elf32_Ehdr *eh,
				      Elfinfo **p_el, off_t ar_offset));

bool check_elf_entsize PROTO((Elf32_Shdr *sh, const char *secname,
			      size_t structsize));

bool read_elf_section PROTO((Elfinfo *el, const char *what, Elf32_Shdr *sh,
			     void *buf));
#endif
