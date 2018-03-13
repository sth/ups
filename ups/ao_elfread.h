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

#define elf_address_size(el) \
	(el)->ops->address_size()

#define elf_read_header(elops, path, fd, offset, p_eh) \
	(elops)->read_header(path, fd, offset, p_eh)
#define elf_validate_header(elops, path, eh) \
	(elops)->validate_header(path, eh)
#define elf_header_type(elops, eh) \
	(elops)->header_type(eh)
#define elf_header_entry(elops, eh) \
	(elops)->header_entry(eh)
#define elf_header_phoff(elops, eh) \
	(elops)->header_phoff(eh)
#define elf_header_phnum(elops, eh) \
	(elops)->header_phnum(eh)
#define elf_header_phsize(elops, eh) \
	(elops)->header_phsize(eh)
#define elf_header_shoff(elops, eh) \
	(elops)->header_shoff(eh)
#define elf_header_shnum(elops, eh) \
	(elops)->header_shnum(eh)
#define elf_header_shsize(elops, eh) \
	(elops)->header_shsize(eh)
#define elf_header_shstr(el, eh) \
	(el)->ops->header_shstr(el, eh)

#define elf_lookup_phdr(elops, phtab, index) \
	(elops)->lookup_phdr(phtab, index)
#define elf_phdr_type(elops, ph) \
	(elops)->phdr_type(ph)
#define elf_phdr_flags(elops, ph) \
	(elops)->phdr_flags(ph)
#define elf_phdr_offset(elops, ph) \
	(elops)->phdr_offset(ph)
#define elf_phdr_vaddr(elops, ph) \
	(elops)->phdr_vaddr(ph)
#define elf_phdr_filesz(elops, ph) \
	(elops)->phdr_filesz(ph)
#define elf_phdr_memsz(elops, ph) \
	(elops)->phdr_memsz(ph)

#define elf_lookup_section(el, secname, p_sh) \
	(el)->ops->lookup_section(el, secname, p_sh)
#define elf_find_section(el, secname, what, p_sh) \
	(el)->ops->find_section(el, secname, what, p_sh)
#define elf_read_section(el, what, sh, buf) \
	(el)->ops->read_section(el, what, sh, buf)
#define elf_section_addr(el, sh) \
	(el)->ops->section_addr(sh)
#define elf_section_offset(el, sh) \
	(el)->ops->section_offset(sh)
#define elf_section_size(el, sh) \
	(el)->ops->section_size(sh)
#define elf_section_link(el, sh) \
	(el)->ops->section_link(el, sh)
#define elf_section_entries(el, sh) \
	(el)->ops->section_entries(sh)

#define elf_lookup_note(el, notename, notetype, p_notesize) \
	(el)->ops->lookup_note(el, notename, notetype, p_notesize)

#define elf_lookup_symbol(el, syms, index) \
	(el)->ops->lookup_symbol(syms, index)
#define elf_symbol_name(el, sym, strings, strings_nbytes) \
	(el)->ops->symbol_name(sym, strings, strings_nbytes)
#define elf_symbol_value(el, sym) \
	(el)->ops->symbol_value(sym)
#define elf_symbol_type(el, sym) \
	(el)->ops->symbol_type(sym)
#define elf_symbol_bind(el, sym) \
	(el)->ops->symbol_bind(sym)
#define elf_symbol_section(el, sym) \
	(el)->ops->symbol_section(el, sym)

#define elf_lookup_dynamic(el, dyntab, index) \
	(el)->ops->lookup_dynamic(dyntab, index)
#define elf_dynamic_tag(el, dyn) \
	(el)->ops->dynamic_tag(dyn)
#define elf_dynamic_val(el, dyn) \
	(el)->ops->dynamic_val(dyn)
#define elf_dynamic_ptr(el, dyn) \
	(el)->ops->dynamic_ptr(dyn)
#define elf_dynamic_ptr_address(el, dynsh, dyntab, dyn) \
	(el)->ops->dynamic_ptr_address(dynsh, dyntab, dyn)

#define elf_relocation_size(elops, rel) \
	(elops)->relocation_size(rel)
#define elf_resolve_relocation(elops, xp, ast, symtab, strtab, rel, off, name) \
	(elops)->resolve_relocation(xp, ast, symtab, strtab, rel, off, name)

#define elf_read_r_debug(elops, addr, callback, handle) \
	(elops)->read_r_debug(addr, (Elfread)callback, handle)
#define elf_r_debug_map(elops, debug) \
	(elops)->r_debug_map(debug)
#define elf_r_debug_brk(elops, debug) \
	(elops)->r_debug_brk(debug)

#define elf_read_link_map(elops, addr, callback, handle) \
	(elops)->read_link_map(addr, (Elfread)callback, handle)
#define elf_link_map_addr(elops, lmap) \
	(elops)->link_map_addr(lmap)
#define elf_link_map_name(elops, lmap) \
	(elops)->link_map_name(lmap)
#define elf_link_map_next(elops, lmap) \
	(elops)->link_map_next(lmap)
#define elf_link_map_prev(elops, lmap) \
	(elops)->link_map_prev(lmap)

bool check_elf_ident PROTO((const char *path, int fd, off_t offset,
                            const char *action, const Elfops **p_ops));
bool check_elf_file_type PROTO((int fd, const char *path, int type,
				const char *action));
bool check_elf_header_type PROTO((const Elfops *ops, Elf_Ehdr *eh,
                                  const char *path, int type,
				  const char *action));
bool prepare_to_parse_elf_file PROTO((const char *path, int fd, Elf_Ehdr *eh,
				      const Elfops *elops, Elfinfo **p_el,
                                      off_t ar_offset));

bool check_elf_entsize PROTO((Elf32_Shdr *sh, const char *secname,
			      size_t structsize));

extern const Elfops elf32_ops;
extern const Elfops elf64_ops;

#endif
