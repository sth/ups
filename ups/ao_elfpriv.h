/* ao_elfpriv.h - stuff private to the ao_elf*.c routines */

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


/* @(#)ao_elfpriv.h	1.2 04 Jun 1995 (UKC) */

#define AO_ELFPRIV_H_INCLUDED

#ifndef ELFINFO_TYPEDEFED
typedef struct Elfinfo Elfinfo;
#define ELFINFO_TYPEDEFED
#endif

#ifndef LIBDEP_TYPEDEFED
typedef struct Libdep Libdep;
#define LIBDEP_TYPEDEFED
#endif

#ifndef Elf_Ehdr
typedef struct Elf_Ehdr Elf_Ehdr;
#endif

#ifndef Elf_Shdr
typedef struct Elf_Shdr Elf_Shdr;
#endif

#ifndef Elf_Phdr
typedef struct Elf_Phdr Elf_Phdr;
#endif

#ifndef Elf_Nhdr
typedef struct Elf_Nhdr Elf_Nhdr;
#endif

#ifndef Elf_Sym
typedef struct Elf_Sym Elf_Sym;
#endif

#ifndef Elf_Dyn
typedef struct Elf_Dyn Elf_Dyn;
#endif

struct Elfinfo {
	alloc_pool_t *apool;
	const struct Elfops *ops;
	
	const char *path;
	int fd;
	const char *what;

	taddr_t min_file_vaddr;
	Libdep *libdep;

	Elf_Shdr *sections;
	int num_sections;
	const char *sec_strings;

	Elf_Shdr *symtab_sh;
	bool stripped;
	
	Elf_Shdr *indexsh;
	Elf_Shdr *indexstrsh;

	Elf_Shdr *pltsh;

	struct Elfinfo *debugel;
};

typedef int (*Elfread)PROTO((void *handle, taddr_t addr, void *buf, size_t nbytes));

typedef struct Elfops {
	int (*address_size)PROTO((void));

	bool (*read_header)PROTO((const char *path, int fd, off_t offset,
				  Elf_Ehdr **p_eh));
	bool (*validate_header)PROTO((const char *path, Elf_Ehdr *eh));
	int (*header_type)PROTO((Elf_Ehdr *eh));
	taddr_t (*header_entry)PROTO((Elf_Ehdr *eh));
	off_t (*header_phoff)PROTO((Elf_Ehdr *eh));
	size_t (*header_phnum)PROTO((Elf_Ehdr *eh));
	size_t (*header_phsize)PROTO((Elf_Ehdr *eh));
	off_t (*header_shoff)PROTO((Elf_Ehdr *eh));
	size_t (*header_shnum)PROTO((Elf_Ehdr *eh));
	size_t (*header_shsize)PROTO((Elf_Ehdr *eh));
	Elf_Shdr *(*header_shstr)PROTO((Elfinfo *, Elf_Ehdr *eh));

	Elf_Phdr *(*lookup_phdr)PROTO((Elf_Phdr *phtab, size_t index));
	int (*phdr_type)PROTO((Elf_Phdr *ph));
	unsigned int (*phdr_flags)PROTO((Elf_Phdr *ph));
	off_t (*phdr_offset)PROTO((Elf_Phdr *ph));
	taddr_t (*phdr_vaddr)PROTO((Elf_Phdr *ph));
	size_t (*phdr_filesz)PROTO((Elf_Phdr *ph));
	size_t (*phdr_memsz)PROTO((Elf_Phdr *ph));

	bool (*lookup_section)PROTO((Elfinfo *el, const char *secname,
				     Elf_Shdr **p_sh));
	bool (*find_section)PROTO((Elfinfo *el, const char *secname,
				   const char *what, Elf_Shdr **p_sh));
	bool (*read_section)PROTO((Elfinfo *el, const char *what,
				   Elf_Shdr *sh, void *buf));
	taddr_t (*section_addr)PROTO((Elf_Shdr *sh));
	off_t (*section_offset)PROTO((Elf_Shdr *sh));
	size_t (*section_size)PROTO((Elf_Shdr *sh));
	Elf_Shdr *(*section_link)PROTO((Elfinfo *el, Elf_Shdr *sh));
	size_t (*section_entries)PROTO((Elf_Shdr *sh));

	void *(*lookup_note)PROTO((Elfinfo*el, const char *notename,
				   int notetype, size_t *notesize));

	Elf_Sym *(*lookup_symbol)PROTO((Elf_Sym *syms, size_t index));
	const char *(*symbol_name)PROTO((Elf_Sym *sym, const char *strings,
					 size_t strings_nbytes));
	taddr_t (*symbol_value)PROTO((Elf_Sym *sym));
	int (*symbol_type)PROTO((Elf_Sym *sym));
	int (*symbol_bind)PROTO((Elf_Sym *sym));
	Elf_Shdr *(*symbol_section)PROTO((Elfinfo *el, Elf_Sym *sym));

	Elf_Dyn *(*lookup_dynamic)PROTO((Elf_Dyn *dyntab, size_t index));
	long (*dynamic_tag)PROTO((Elf_Dyn *dyn));
	unsigned long (*dynamic_val)PROTO((Elf_Dyn *dyn));
	taddr_t (*dynamic_ptr)PROTO((Elf_Dyn *dyn));
	taddr_t (*dynamic_ptr_address)PROTO((Elf_Shdr *dynsh, Elf_Dyn *dyntab,
					     Elf_Dyn *dyn));

	size_t (*relocation_size)PROTO((int rel_type));
	bool (*resolve_relocation)PROTO((target_t *xp, taddr_t st_base_address,
					 taddr_t symtab_vaddr,
					 taddr_t strtab_vaddr,
					 int rel_type, taddr_t off, char *name));

	struct elf_r_debug *(*read_r_debug)PROTO((taddr_t addr,
						  Elfread read_callback,
						  void *read_handle));
	taddr_t (*r_debug_map)PROTO((struct elf_r_debug *debug));
	taddr_t (*r_debug_brk)PROTO((struct elf_r_debug *debug));

	struct elf_link_map *(*read_link_map)PROTO((taddr_t addr,
						    Elfread read_callback,
						    void *read_handle));
	taddr_t (*link_map_addr)PROTO((struct elf_link_map *lmap));
	taddr_t (*link_map_name)PROTO((struct elf_link_map *lmap));
	taddr_t (*link_map_next)PROTO((struct elf_link_map *lmap));
	taddr_t (*link_map_prev)PROTO((struct elf_link_map *lmap));
} Elfops;
