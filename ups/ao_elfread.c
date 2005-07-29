/* ao_elfread.c - functions for reading ELF and ar files  */

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


/* @(#)ao_elfread.c	1.2 04 Jun 1995 (UKC) */
char ups_ao_elfread_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#ifdef AO_ELF

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#if HAVE_ELF_H
#include <elf.h>
#elif HAVE_LINUX_ELF_H
#include <linux/elf.h>
#endif
#include <ar.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <mtrprog/io.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "st.h"
#include "ao_execinfo.h"
#include "ao_dwarf.h"
#include "ao_elfpriv.h"
#include "ao_elfread.h"
#include "ao_elflib.h"
#include "elfstab.h"
#include "state.h"

static const char Elfwhat[] = "ELF executable";

static const char *
elftype_to_what(int type)
{
	static char *last = NULL;

	switch (type) {
		case ET_EXEC:	return "an executable";
		case ET_REL:	return "a .o file";
		case ET_DYN:	return "a shared library";
		case ET_CORE:	return "a core file";
	}

	free(last);
	last = strf("an unknown ELF file type %d", type);
	return last;
}

bool
check_elf_ident(const char *path, int fd, off_t offset, const char *action,
		const Elfops **p_ops)
{
	unsigned char ident[EI_NIDENT];

	if (!read_chunk(path, "", fd, "ELF ident", offset, ident, EI_NIDENT)) {
		return FALSE;
	}
   
#if defined(ELFMAG) && defined(SELFMAG)
	if (memcmp(ident, ELFMAG, SELFMAG) != 0) {
#elif defined(IS_ELF)
	if (!IS_ELF (ident)) {
#endif
		errf("Can't %s %s: it is not an ELF file", action, path);
		return FALSE;
	}

	if (ident[EI_CLASS] == ELFCLASS32) {
		*p_ops = &elf32_ops;
	}
	else if (ident[EI_CLASS] == ELFCLASS64) {
		*p_ops = &elf64_ops;
	}
	else {
		errf("Can't %s %s: unsupported ELF class", action, path);
		return FALSE;
	}

	return TRUE;
}

bool
check_elf_header_type(const Elfops *elops, Elf_Ehdr *eh, const char *path,
		      int type, const char *action)
{
	if (elf_header_type(elops, eh) != type) {
		errf("Can't %s %s: it is %s (need %s)",
		     action, path,
		     elftype_to_what(elf_header_type(elops, eh)),
		     elftype_to_what(type));
		return FALSE;
	}
		       
	return TRUE;
}

int
check_elf_file_type(int fd, const char *path, int type, const char *action)
{
	const Elfops *elops;
	Elf_Ehdr *hdr;
	bool ok;

	if (!check_elf_ident(path, fd, (off_t)0, action, &elops) ||
	    !elf_read_header(elops, path, fd, (off_t)0, &hdr))
		return FALSE;
	
	ok = check_elf_header_type(elops, hdr, path, type, action);

	free(hdr);

	return ok;
}

int
check_elf_exec_header(int fd, const char *path)
{
	return check_elf_file_type(fd, path, ET_EXEC, "debug");
}

bool
check_elf_entsize(Elf32_Shdr *sh, const char *secname, size_t structsize)
{
	/*  Check for sh NULL, as we don't mind a missing section.
	 */
	if (sh != NULL && sh->sh_entsize != structsize) {
		errf("Wrong entry size for %s section (expected %d, got %ld)",
		     secname, structsize, sh->sh_entsize);
		return FALSE;
	}

	return TRUE;
}

static bool
get_load_addrs(const char *path, const Elfops *elops, Elf_Phdr *phtab,
	       int phnum, taddr_t *p_text_load_addr, size_t *p_text_size,
	       off_t *p_addr_to_fpos_offset, taddr_t *p_min_file_vaddr)
{
	int i;
	Elf_Phdr *textph, *loadph;

	textph = loadph = NULL;

	for (i = 0; i < phnum; ++i) {
		Elf_Phdr *ph;

		ph = elf_lookup_phdr(elops, phtab, i);
		
		if (elf_phdr_type(elops, ph) == PT_LOAD) {
			if (loadph == NULL ||
			    elf_phdr_vaddr(elops, ph) < elf_phdr_vaddr(elops, loadph))
				loadph = ph;
			
			/*  We assume that the first read-only loadable segment
			 *  is the text segment.
			 */
			if ((elf_phdr_flags(elops, ph) & PF_W) == 0)
				textph = ph;
		}
	}

	if (loadph == NULL) {
		errf("Can't find a loadable segment in %s", path);
		return FALSE;
	}

	if (textph == NULL) {
		errf("Can't find text segment in %s "
		     "(no read-only loadable segment found)", path);
		return FALSE;
	}

	*p_text_load_addr = elf_phdr_vaddr(elops, textph);
	*p_text_size = elf_phdr_memsz(elops, textph);
	*p_addr_to_fpos_offset = elf_phdr_vaddr(elops, textph) - elf_phdr_offset(elops, textph);

	*p_min_file_vaddr = elf_phdr_vaddr(elops, loadph) - elf_phdr_offset(elops, loadph);

	return TRUE;
}

bool
prepare_to_parse_elf_file(const char *textpath, int fd, Elf_Ehdr *eh,
			  const Elfops *elops, Elfinfo **p_el, off_t ar_offset)
{
	Elfinfo *el;
	Elf_Shdr *strsh;
	alloc_pool_t *ap;

	if (!elf_validate_header(elops, textpath, eh))
		return FALSE;

	ap = alloc_create_pool();
	el = (Elfinfo *)alloc(ap, sizeof(Elfinfo));

	el->apool = ap;
	el->ops = elops;

	el->path = textpath;
	el->fd = fd;
	el->sections = alloc(ap, elf_header_shsize(elops, eh));
	el->debugel = el;

	if (!read_chunk(textpath, Elfwhat, fd, "section header table",
			elf_header_shoff(elops, eh) + ar_offset, el->sections,
			elf_header_shsize(elops, eh))) {
		alloc_free_pool(ap);
		return FALSE;
	}

	strsh = elf_header_shstr(el, eh);

	el->num_sections = elf_header_shnum(elops, eh);
	el->sec_strings = allocstr(ap, elf_section_size(el, strsh));

	/*  In Japanese language no article. [...] In Japanese hotel rooms
	 *  not many articles either.  -- David Lodge.
	 */
	el->what = strchr(elftype_to_what(elf_header_type(elops, eh)), ' ') + 1;

	if (!read_chunk(textpath, Elfwhat, fd, "section strings table",
			elf_section_offset(el, strsh) + ar_offset,
			(char *)el->sec_strings, elf_section_size(el, strsh))) {
		alloc_free_pool(ap);
		return FALSE;
	}

	*p_el = el;
	return TRUE;
}

/*
 * This routine for calculating the CRC for a separate debug file
 * is GPLed code borrowed from binutils.
 */
static unsigned long
calc_gnu_debuglink_crc32(unsigned long crc, const unsigned char *buf, int len)
{
  static const unsigned long crc32_table[256] =
    {
      0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419,
      0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4,
      0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07,
      0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
      0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
      0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
      0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4,
      0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
      0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3,
      0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac, 0x51de003a,
      0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599,
      0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
      0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190,
      0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f,
      0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e,
      0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
      0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
      0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
      0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3,
      0xfbd44c65, 0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
      0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a,
      0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5,
      0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010,
      0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
      0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17,
      0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6,
      0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615,
      0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
      0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1, 0xf00f9344,
      0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
      0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a,
      0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
      0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1,
      0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c,
      0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef,
      0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
      0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe,
      0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31,
      0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0, 0xec63f226, 0x756aa39c,
      0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
      0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b,
      0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
      0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1,
      0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
      0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278,
      0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7,
      0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66,
      0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
      0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605,
      0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8,
      0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b,
      0x2d02ef8d
    };
  const unsigned char *end;

  crc = ~crc & 0xffffffff;
  for (end = buf + len; buf < end; ++ buf)
    crc = crc32_table[(crc ^ *buf) & 0xff] ^ (crc >> 8);
  return ~crc & 0xffffffff;;
}

static bool
debug_file_exists(const char *debugpath, unsigned long crc)
{
	int fd;
	unsigned char buffer[8192];
	int count;
	unsigned long file_crc;

	if ((fd = open(debugpath, O_RDONLY)) < 0)
		return FALSE;

	file_crc = 0;
	
	while ((count = read(fd, buffer, sizeof(buffer))) > 0)
		file_crc = calc_gnu_debuglink_crc32(file_crc, buffer, count);

	close(fd);
	
	return crc == file_crc;
}

static bool
find_debug_file(const char *textpath, const char *debugname,
		unsigned long crc, char *debugpath)
{
	char *textdir;
	char *textdirptr;

	textdir = strsave(textpath);

	if ((textdirptr = strrchr(textdir, '/')) != NULL) {
		*textdirptr = '\0';
	}

	sprintf(debugpath, "%s/%s", textdir, debugname);

	if (!debug_file_exists(debugpath, crc)) {
		sprintf(debugpath, "%s/.debug/%s", textdir, debugname);

		if (!debug_file_exists(debugpath, crc)) {
			sprintf(debugpath, "/usr/lib/debug%s/%s", textdir, debugname);

			if (!debug_file_exists(debugpath, crc)) {
				free(textdir);
				return FALSE;
			}
		}
	}

	free(textdir);

	return TRUE;
}

static Elfinfo *
open_debug_file(const char *debugpath)
{
	int fd;
	const Elfops *elops;
	Elf_Ehdr *hdr;
	Elf_Phdr *phtab;
	Elfinfo *el;
	taddr_t text_mem_addr;
	size_t text_size;
	off_t addr_to_fpos_offset;

	if ((fd = open(debugpath, O_RDONLY)) < 0)
		return NULL;

	if (!check_elf_ident(debugpath, fd, (off_t)0, "debug", &elops))
		return NULL;

	if (!elf_read_header(elops, debugpath, fd, (off_t)0, &hdr))
		return NULL;

	if (!prepare_to_parse_elf_file(debugpath, fd, hdr, elops, &el, (off_t)0)) {
		free(hdr);
		return NULL;
	}

	phtab = e_malloc(elf_header_phsize(elops, hdr));
	if (!read_chunk(debugpath, Elfwhat, fd, "debug program header table",
			elf_header_phoff(elops, hdr),
			phtab, elf_header_phsize(elops, hdr)) ||
	    !get_load_addrs(debugpath, elops, phtab, elf_header_phnum(elops, hdr),
			    &text_mem_addr, &text_size,
			    &addr_to_fpos_offset,
			    &el->min_file_vaddr)) {
		free(phtab);
		free(hdr);
		free_elfinfo(el);
		return FALSE;
	}
	free(phtab);

	free(hdr);

	return el;
}

bool
elf_get_exec_info(const char *textpath, int fd, Libdep *libdep,
		  Execinfo *ex, Elfinfo **p_el, symtab_type_t *p_st_is)
{
	const Elfops *elops;
	Elf_Ehdr *hdr;
	Elf_Phdr *phtab;
	Elf_Shdr *textsh;
	Elf_Shdr *symsh, *symstrsh, *indexsh, *indexstrsh;
	Elf_Shdr *debuglinksh;
	Elfinfo *el;
	const char *symtab_secname;
#if WANT_DWARF
	Elf_Shdr *dwarfsh;
#endif

	if (!check_elf_ident(textpath, fd, (off_t)0, "debug", &elops))
		return FALSE;

	if (!elf_read_header(elops, textpath, fd, (off_t)0, &hdr))
		return FALSE;

	if (!prepare_to_parse_elf_file(textpath, fd, hdr, elops, &el, (off_t)0)) {
		free(hdr);
		return FALSE;
	}

	if (!elf_find_section(el, ".text", "text", &textsh)) {
		free(hdr);
		free_elfinfo(el);
		return FALSE;
	}

	if (elf_lookup_section(el, ".symtab", &el->symtab_sh)) {
		el->stripped = FALSE;
		symtab_secname = ".symtab";
	}
	else {
		if (!elf_lookup_section(el, ".dynsym", &el->symtab_sh)) {
			errf("Can't debug %s: no .symtab or .dynsym section",
			     el->path);
			free(hdr);
			free_elfinfo(el);
			return FALSE;
		}

		/* RGA don't complain about stripped libraries on invocation */
		if (get_message_wn() != -1)
		  errf("Warning: no .symtab section in %s - "
		       "using .dynsym instead", el->path);
		el->stripped = TRUE;
		symtab_secname = ".dynsym";
	}

	if (elf_section_link(el, el->symtab_sh) == NULL) {
		errf("sh_link for %s section of %s not in range 0..%d",
		     symtab_secname, el->path,
		     el->num_sections - 1);
		free(hdr);
		free_elfinfo(el);
		return FALSE;
	}

	if (!elf_lookup_section(el, ".plt", &el->pltsh)) {
		el->pltsh = NULL;
	}

	if (elf_lookup_section(el, ".gnu_debuglink", &debuglinksh)) {
		char *debuglink = e_malloc(elf_section_size(el, debuglinksh));

		if (read_chunk(textpath, Elfwhat, fd, "debug file name",
			       elf_section_offset(el, debuglinksh), debuglink,
			       elf_section_size(el, debuglinksh))) {
			int crc_offset;
			unsigned long crc;
			char debugpath[256];

			crc_offset = strlen(debuglink) + 1;
			crc_offset = (crc_offset + 3) & ~3;
			
			crc = *(unsigned int *)(debuglink + crc_offset);

			if (find_debug_file(textpath, debuglink, crc, debugpath)) {
			   el->debugel = open_debug_file(debugpath);
			}
		}
		
		free(debuglink);
	}

#if WANT_DWARF
	if (elf_lookup_section(el->debugel, ".debug_info", &dwarfsh)) {
		*p_st_is = ST_DWARF;
		symsh = symstrsh = NULL;
	} else
#endif
	if (elf_lookup_section(el->debugel, ".stab", &symsh)) {
		if (!elf_lookup_section(el->debugel, ".stabstr", &symstrsh)) {
			errf("Found .stab section in %s but no "
			     ".stabstr strings section for it", el->path);
			free(hdr);
			free_elfinfo(el);
			return FALSE;
		}
		*p_st_is = ST_STABS;
	}
	else {
		symsh = symstrsh = NULL;
		*p_st_is = ST_NONE;
	}
		
	if (elf_lookup_section(el->debugel, ".stab.index", &indexsh)) {
		if (!elf_lookup_section(el->debugel, ".stab.indexstr", &indexstrsh)) {
			errf("Found .stab.index section in %s but no "
			     ".stab.indexstr strings section for it", el->path);
			indexsh = NULL;
		}
	}
	else {
		indexsh = indexstrsh = NULL;
	}

#ifdef fixme
	if (!check_elf_entsize(symsh, ".stab", sizeof(struct nlist)) ||
	    !check_elf_entsize(indexsh, ".stab.index", sizeof(struct nlist)) ||
	    !check_elf_entsize(el->symtab_sh, ".symtab", sizeof(Elf32_Sym)))
		return FALSE;
#endif

	phtab = e_malloc(elf_header_phsize(elops, hdr));
	if (!read_chunk(textpath, Elfwhat, fd, "program header table",
			elf_header_phoff(elops, hdr),
			phtab, elf_header_phsize(elops, hdr)) ||
	    !get_load_addrs(textpath, elops, phtab, elf_header_phnum(elops, hdr),
			    &ex->text_mem_addr, &ex->text_size,
			    &ex->addr_to_fpos_offset,
			    &el->min_file_vaddr)) {
		free(phtab);
		free(hdr);
		free_elfinfo(el);
		return FALSE;
	}
	free(phtab);

	el->libdep = libdep;
	el->indexsh = indexsh;
	el->indexstrsh = indexstrsh;

	ex->text_addr_delta = 0;
	ex->debug_addr_delta = el->min_file_vaddr - el->debugel->min_file_vaddr;
	ex->dynamic = FALSE;
	ex->entry_addr = elf_header_entry(elops, hdr);

#if WANT_DWARF
	if (*p_st_is == ST_DWARF) {
		ex->file_syms_offset = 0;
		ex->nsyms = 0;
		ex->file_symstrings_offset = 0;
	}
	else
#endif
	if (*p_st_is == ST_STABS) {
		ex->file_syms_offset = elf_section_offset(el->debugel, symsh);
		ex->nsyms = elf_section_entries(el->debugel, symsh);
		ex->file_symstrings_offset = elf_section_offset(el->debugel, symstrsh);
	}
	else {
		ex->file_syms_offset = 0;
		ex->nsyms = 0;
		ex->file_symstrings_offset = 0;
	}

	free(hdr);

	*p_el = el;

	return TRUE;
}

void
free_elfinfo(Elfinfo *el)
{
	if (el->debugel != el) {
		close(el->debugel->fd);
		free_elfinfo(el->debugel);
	}
   
	alloc_free_pool(el->apool);
}

void
ar_get_path_and_member(const char *objpath, char **p_path, char **p_member)
{
	size_t len;
	const char *lbrac;

	len = strlen(objpath);
	if (len == 0 || objpath[len - 1] != ')' ||
	    (lbrac = strrchr(objpath, '(')) == NULL) {
		*p_path = strsave(objpath);
		*p_member = NULL;
	}
	else {
		size_t pathlen, memberlen;
		
		pathlen = lbrac - objpath;
		*p_path = e_malloc(pathlen + 1);
		memcpy(*p_path, objpath, pathlen);
		(*p_path)[pathlen] = '\0';

		memberlen = &objpath[len - 1] - (lbrac + 1);
		*p_member = e_malloc(memberlen + 1);
		memcpy(*p_member, lbrac + 1, memberlen);
		(*p_member)[memberlen] = '\0';
	}
}

static bool
parse_ar_field(const char *path, const char *what, const char *fieldname,
	       const char *field, size_t len, size_t *p_res)
{
	size_t res;
	int i;

	res = 0;

	for (i = 0; i < len; ++i) {
		int ch;

		ch = field[i];

		if (ch == ' ')
			break;
					
		if (!isdigit(ch)) {
			errf("Illegal %s `%.*s' in %s %s",
			     fieldname, (int)len, field, what, path);
			return FALSE;
		}

		res = res * 10 + (ch - '0');
	}

	if (i == 0) {
		errf("Illegal zero length %s in %s %s", fieldname, what, path);
		return FALSE;
	}

	*p_res = res;
	return TRUE;
}

static bool
get_member_name(const char *path, const char *what, struct ar_hdr *ar,
		const char *strings, size_t strings_size,
		const char **p_name, size_t *p_namelen)
{
	size_t str_offset, namelen_lim, len;
	const char *name;

	if (ar->ar_name[0] != '/') {
		/*  Ordinary entry, with the name the entry itself.
		 */
		name = ar->ar_name;
		namelen_lim = sizeof(ar->ar_name);
	}
	else if (ar->ar_name[1] == ' ') {
		/*  Symbol table.
		 */
		name = NULL;
		namelen_lim = 0;
	}
	else {
			
		if (!parse_ar_field(path, what, "strings offset",
				    ar->ar_name + 1,
				    sizeof(ar->ar_name) - 1, &str_offset)) {
			return FALSE;
		}

		if (strings == NULL) {
			errf("Missing strings table in %s %s", what, path);
			return FALSE;
		}
			
		if (str_offset >= strings_size) {
			errf("Strings offset %ld not in range 0..%ld in %s %s",
			     (long)str_offset, (long)strings_size - 1,
			     what, path);
			return FALSE;
		}

		name = &strings[str_offset];
		namelen_lim = strings_size - str_offset;
	}

	for (len = 0; len < namelen_lim; ++len) {
		if (name[len] == '/')
			break;
	}

	if (namelen_lim != 0 && len == namelen_lim) {
		errf("Unterminated name `%.*s' in %s %s",
		     (int)len, ar->ar_name, what, path);
		return FALSE;
	}

	*p_name = name;
	*p_namelen = len;
	return TRUE;
}

bool
ar_get_member_offset(const char *path, int fd, const char *member,
		     off_t *p_offset)
{
	static const char what[] = "ar archive";
	char magicbuf[SARMAG];
	off_t offset;
	char *strings;
	size_t strings_size, member_len;
	struct stat stbuf;

	offset = 0;

	if (member == NULL) {
		*p_offset = 0;
		return TRUE;
	}

	if (!read_chunk(path, "", fd, "archive magic string",
			(off_t)0, magicbuf, (off_t)SARMAG)) {
		return FALSE;
	}
	offset += SARMAG;

	if (memcmp(magicbuf, ARMAG, SARMAG) != 0) {
		errf("%s is not an ar(1) archive file", path);
		return FALSE;
	}

	if (fstat(fd, &stbuf) != 0) {
		failmesg("Can't fstat", what, path);
		return FALSE;
	}

	strings = NULL;
	strings_size = 0;
	member_len = strlen(member);

	for (;;) {
		size_t size;
		struct ar_hdr hdr;
		
		if (offset == stbuf.st_size) {
			errf("Can't find `%s' in %s %s", member, what, path);
			goto bad;
		}
		
		if (!read_chunk(path, what, fd, "archive member header",
				offset, &hdr, sizeof(hdr)))
			goto bad;

		if (memcmp(hdr.ar_fmag, ARFMAG, sizeof(hdr.ar_fmag)) != 0) {
			errf("Bad archive header at offset %ld in %s %s",
			     offset, what, path);
			goto bad;
		}

		offset += sizeof(hdr);

		if (!parse_ar_field(path, what, "entry size",
				    hdr.ar_size, sizeof(hdr.ar_size), &size))
			goto bad;

		if (hdr.ar_name[0] == '/' && hdr.ar_name[1] == '/') {
			strings_size = size;
			strings = e_malloc(strings_size);
			
			if (!read_chunk(path, what, fd, "archive strings table",
					offset, strings, strings_size))
				goto bad;
		}
		else {
			size_t len;
			const char *name;
			
			if (!get_member_name(path, what, &hdr, strings,
					     strings_size, &name, &len))
				goto bad;
			
			if (len == member_len && memcmp(name, member, len) == 0)
				break;
		}
		
		offset += size;

		/*  Sizes are rounded up to an even byte count.
		 */
		if (offset % 2 != 0)
			++offset;
	}

	*p_offset = offset;
	return TRUE;

bad:
	free(strings);
	return FALSE;
}

#endif /* AO_ELF */
