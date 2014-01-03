/* ao_elfreadbase.c - functions for reading ELF and ar files  */

/*  Copyright 2004 Tom Hughes <thh@cyberscience.com>
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

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#ifdef AO_ELF

#include <stdlib.h>
#if HAVE_ELF_H
#include <elf.h>
#define FREEBSD_ELF 1
#include <link.h>
#undef FREEBSD_ELF
#elif HAVE_LINUX_ELF_H
#include <linux/elf.h>
#endif

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <mtrprog/hash.h>
#include <mtrprog/io.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "st.h"
#include "ao_syms.h"
#include "ao_elfpriv.h"
#include "ao_elfread.h"
#include "ao_elflib.h"
#include "elfstab.h"

struct elf_r_debug {
	int r_version;
	Elf_Addr r_map;
	Elf_Addr r_brk;
	int r_state;
	Elf_Addr r_ldbase;
};

struct elf_link_map {
	Elf_Addr l_addr;
	Elf_Addr l_name;
	Elf_Addr l_ld;
	Elf_Addr l_next, l_prev;
};

static int
ELF(address_size)(void)
{
	return ELF_ADDRESS_SIZE;
}

static bool
ELF(read_header)(const char *path, int fd, off_t offset, Elf_Ehdr **p_eh)
{
        *p_eh = e_malloc(sizeof(Elf_Ehdr));

        return read_chunk(path, "", fd, "ELF header", offset, *p_eh, sizeof(Elf_Ehdr));
}

static bool
ELF(validate_header)(const char *path, Elf_Ehdr *eh)
{
	if (eh->e_type == ET_EXEC) {
		if (eh->e_phnum == 0) {
			errf("No program header table in ELF file %s", path);
			return FALSE;
		}
		
		if (eh->e_phentsize != sizeof(Elf_Phdr)) {
			errf("Wrong program header entry size in %s "
			     "(expected %ld, found %d)",
			     path, (long)sizeof(Elf_Phdr), eh->e_phentsize);
			return FALSE;
		}
	}

	if (eh->e_shstrndx >= eh->e_shnum) {
		errf("ELF strings section number %d not in range 0..%d in %s",
		     eh->e_shstrndx, eh->e_shnum - 1, path);
		return FALSE;
	}

	return TRUE;
}

static int
ELF(header_type)(Elf_Ehdr *eh)
{
	return eh->e_type;
}

static taddr_t
ELF(header_entry)(Elf_Ehdr *eh)
{
	return eh->e_entry;
}

static off_t
ELF(header_phoff)(Elf_Ehdr *eh)
{
	return eh->e_phoff;
}

static size_t
ELF(header_phnum)(Elf_Ehdr *eh)
{
	return eh->e_phnum;
}

static size_t
ELF(header_phsize)(Elf_Ehdr *eh)
{
	return eh->e_phnum * eh->e_phentsize;
}

static off_t
ELF(header_shoff)(Elf_Ehdr *eh)
{
	return eh->e_shoff;
}

static size_t
ELF(header_shnum)(Elf_Ehdr *eh)
{
	return eh->e_shnum;
}

static size_t
ELF(header_shsize)(Elf_Ehdr *eh)
{
	return eh->e_shnum * eh->e_shentsize;
}

static Elf_Shdr *
ELF(header_shstr)(Elfinfo *el, Elf_Ehdr *eh)
{
	return el->sections + eh->e_shstrndx;
}

static Elf_Phdr *
ELF(lookup_phdr)(Elf_Phdr *phtab, size_t index)
{
	return phtab + index;
}

static int
ELF(phdr_type)(Elf_Phdr *ph)
{
	return ph->p_type;
}

static unsigned int
ELF(phdr_flags)(Elf_Phdr *ph)
{
	return ph->p_flags;
}

static off_t
ELF(phdr_offset)(Elf_Phdr *ph)
{
	return ph->p_offset;
}

static taddr_t
ELF(phdr_vaddr)(Elf_Phdr *ph)
{
	return ph->p_vaddr;
}

static size_t
ELF(phdr_filesz)(Elf_Phdr *ph)
{
	return ph->p_filesz;
}

static size_t
ELF(phdr_memsz)(Elf_Phdr *ph)
{
	return ph->p_memsz;
}

static bool
ELF(lookup_section)(Elfinfo *el, const char *secname, Elf_Shdr **p_sh)
{
	int i;

	for (i = 0; i < el->num_sections; ++i) {
		if (strcmp(&el->sec_strings[el->sections[i].sh_name],
			   secname) == 0) {
			*p_sh = &el->sections[i];
			return TRUE;
		}
	}

	return FALSE;
}

static bool
ELF(find_section)(Elfinfo *el, const char *secname, const char *what,
		  Elf_Shdr **p_sh)
{
	if (!ELF(lookup_section)(el, secname, p_sh)) {
		errf("Can't find %s in ELF %s %s (no `%s' section)",
		     what, el->what, el->path, secname);
		return FALSE;
	}

	return TRUE;
}

static bool
ELF(read_section)(Elfinfo *el, const char *what, Elf_Shdr *sh, void *buf)
{
	return read_chunk(el->path, el->what, el->fd, what,
			  (off_t)sh->sh_offset, buf, sh->sh_size);
}

static taddr_t
ELF(section_addr)(Elf_Shdr *sh)
{
	return sh->sh_addr;
}

static off_t
ELF(section_offset)(Elf_Shdr *sh)
{
	return sh->sh_offset;
}

static size_t
ELF(section_size)(Elf_Shdr *sh)
{
	return sh->sh_size;
}

static Elf_Shdr *
ELF(section_link)(Elfinfo *el, Elf_Shdr *sh)
{
	return el->sections + sh->sh_link;
}

static size_t
ELF(section_entries)(Elf_Shdr *sh)
{
	return sh->sh_size / sh->sh_entsize;
}

static void *
ELF(lookup_note)(Elfinfo *el, const char *notename, int notetype, size_t *p_notesize)
{
	void *note = NULL;
	Elf_Shdr *sh;

	for (sh = el->sections; note == NULL && sh < el->sections + el->num_sections; sh++) {
		if (sh->sh_type == SHT_NOTE) {
			unsigned char *data = e_malloc(sh->sh_size);

			if (ELF(read_section)(el, el->sec_strings + sh->sh_name, sh, data)) {
				int offset = 0;

				while (note == NULL && offset < sh->sh_size) {
					Elf_Nhdr *nh = (Elf_Nhdr *)(data + offset);
					int nameoffset = sizeof(Elf_Nhdr);
					char *name = (char *)(data + nameoffset);
					int descoffset = ELF_ALIGN(nameoffset + nh->n_namesz);
					unsigned char *desc = (unsigned char *)(data + descoffset);
					int nextoffset = ELF_ALIGN(descoffset + nh->n_descsz);

					if (strcmp(name, notename) == 0 && nh->n_type == notetype) {
						note = e_malloc(nh->n_descsz);
						memcpy(note, desc, nh->n_descsz);
						*p_notesize = nh->n_descsz;
					}
					
					offset = nextoffset;
				}
			}

			free(data);
		}
	}

	return note;
}

static Elf_Sym *
ELF(lookup_symbol)(Elf_Sym *syms, size_t index)
{
	return syms + index;
}

static const char *
ELF(symbol_name)(Elf_Sym *sym, const char *strings, size_t strings_nbytes)
{
	return sym->st_name > strings_nbytes ? NULL : strings + sym->st_name;
}

static taddr_t
ELF(symbol_value)(Elf_Sym *sym)
{
	return sym->st_value;
}

static int
ELF(symbol_type)(Elf_Sym *sym)
{
	return ELF_ST_TYPE(sym->st_info);
}

static int
ELF(symbol_bind)(Elf_Sym *sym)
{
	return ELF_ST_BIND(sym->st_info);
}

static Elf_Shdr *
ELF(symbol_section)(Elfinfo *el, Elf_Sym *sym)
{
	return sym->st_shndx == SHN_UNDEF ? NULL : el->sections + sym->st_shndx;
}

static Elf_Dyn *
ELF(lookup_dynamic)(Elf_Dyn *dyntab, size_t index)
{
	return dyntab + index;
}

static long
ELF(dynamic_tag)(Elf_Dyn *dyn)
{
	return dyn->d_tag;
}

static unsigned long
ELF(dynamic_val)(Elf_Dyn *dyn)
{
	return dyn->d_un.d_val;
}

static taddr_t
ELF(dynamic_ptr)(Elf_Dyn *dyn)
{
	return dyn->d_un.d_ptr;
}

static taddr_t
ELF(dynamic_ptr_address)(Elf_Shdr *dynsh, Elf_Dyn *dyntab, Elf_Dyn *dyn)
{
	return dynsh->sh_addr + ((char *)&dyn->d_un.d_ptr - (char *)dyntab);
}

static size_t
ELF(relocation_size)(int rel_type)
{
	if (rel_type == DT_REL)
		return sizeof(Elf_Rel);
	else if (rel_type == DT_RELA)
		return sizeof(Elf_Rela);
	else
		return 0;
}

static bool
ELF(resolve_relocation)(target_t *xp, taddr_t st_base_address,
			taddr_t symtab_vaddr, taddr_t strtab_vaddr,
			int rel_type, taddr_t off, char *name)
{
	Elf_Sym sym;
   
	if (rel_type == DT_REL) {
		Elf_Rel rel;

		xp_read_data(xp, st_base_address + off, (char *)&rel, sizeof(rel));
		off = ELF_R_SYM(rel.r_info) * sizeof(sym) + symtab_vaddr;
        }
        else if (rel_type == DT_RELA) {
		Elf_Rela rela;

		xp_read_data(xp, st_base_address + off, (char *)&rela, sizeof(rela));
		off = ELF_R_SYM(rela.r_info) * sizeof(sym) + symtab_vaddr;
        }
        else {
		return FALSE;
        }

	xp_read_data(xp, st_base_address + off, (char *)&sym, sizeof(sym));

	off = sym.st_name + strtab_vaddr;
	xp_read_data(xp, st_base_address + off, name, 256);

        return TRUE;
}

static struct elf_r_debug *
ELF(read_r_debug)(taddr_t addr, Elfread read_callback, void *read_handle)
{
	Elf_Addr dbaddr;
	struct elf_r_debug *debug;

	debug = e_malloc(sizeof(struct elf_r_debug));
   
	if (read_callback(read_handle, addr, &dbaddr, sizeof(dbaddr)) < 0 ||
	    read_callback(read_handle, dbaddr, debug, sizeof(struct elf_r_debug)) < 0) {
		free(debug);
		return NULL;
	}

	return debug;
}

static taddr_t
ELF(r_debug_map)(struct elf_r_debug *debug)
{
	return debug->r_map;
}

static taddr_t
ELF(r_debug_brk)(struct elf_r_debug *debug)
{
	return debug->r_brk;
}

static struct elf_link_map *
ELF(read_link_map)(taddr_t addr, Elfread read_callback, void *read_handle)
{
	struct elf_link_map *lmap;

	lmap = e_malloc(sizeof(struct elf_link_map));
   
	if (read_callback(read_handle, addr, lmap, sizeof(struct elf_link_map)) < 0) {
		free(lmap);
		return NULL;
	}

	return lmap;
}

static taddr_t
ELF(link_map_addr)(struct elf_link_map *lmap)
{
	return lmap->l_addr;
}

static taddr_t
ELF(link_map_name)(struct elf_link_map *lmap)
{
	return lmap->l_name;
}

static taddr_t
ELF(link_map_next)(struct elf_link_map *lmap)
{
	return lmap->l_next;
}

static taddr_t
ELF(link_map_prev)(struct elf_link_map *lmap)
{
	return lmap->l_prev;
}

const Elfops ELF(ops) = {
	ELF(address_size),
	ELF(read_header),
	ELF(validate_header),
	ELF(header_type),
	ELF(header_entry),
	ELF(header_phoff),
	ELF(header_phnum),
	ELF(header_phsize),
	ELF(header_shoff),
	ELF(header_shnum),
	ELF(header_shsize),
	ELF(header_shstr),
        ELF(lookup_phdr),
        ELF(phdr_type),
        ELF(phdr_flags),
        ELF(phdr_offset),
        ELF(phdr_vaddr),
        ELF(phdr_filesz),
        ELF(phdr_memsz),
	ELF(lookup_section),
	ELF(find_section),
        ELF(read_section),
        ELF(section_addr),
        ELF(section_offset),
        ELF(section_size),
        ELF(section_link),
        ELF(section_entries),
	ELF(lookup_note),
        ELF(lookup_symbol),
        ELF(symbol_name),
        ELF(symbol_value),
        ELF(symbol_type),
        ELF(symbol_bind),
        ELF(symbol_section),
        ELF(lookup_dynamic),
        ELF(dynamic_tag),
        ELF(dynamic_val),
        ELF(dynamic_ptr),
        ELF(dynamic_ptr_address),
	ELF(relocation_size),
	ELF(resolve_relocation),
	ELF(read_r_debug),
	ELF(r_debug_map),
	ELF(r_debug_brk),
	ELF(read_link_map),
	ELF(link_map_addr),
	ELF(link_map_name),
	ELF(link_map_next),
	ELF(link_map_prev)
};

#endif /* AO_ELF */
