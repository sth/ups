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
#include "st.h"
#include "ao_execinfo.h"
#include "ao_dwarf.h"
#include "ao_elfpriv.h"
#include "ao_elfread.h"
#include "ao_elflib.h"
#include "elfstab.h"

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
check_elf_header_type(Elf32_Ehdr *eh, const char *path, int type,
		      const char *action)
{
#if defined(ELFMAG) && defined(SELFMAG)
	if (memcmp(eh->e_ident, ELFMAG, SELFMAG) != 0) {
#elif defined(IS_ELF)
	if (!IS_ELF (*eh)) {
#endif
		errf("Can't %s %s: it is not an ELF file", action, path);
		return FALSE;
	}

	if (eh->e_type != type) {
		errf("Can't %s %s: it is %s (need %s)",
		     action, path,
		     elftype_to_what(eh->e_type), elftype_to_what(type));
		return FALSE;
	}
		       
	return TRUE;
}

int
check_elf_file_type(int fd, const char *path, int type, const char *action)
{
	Elf32_Ehdr hdr;

	return (read_chunk(path, "", fd, "ELF header",
			  (off_t)0, &hdr, sizeof(hdr)) &&
		check_elf_header_type(&hdr, path, type, action));
}

int
check_elf_exec_header(int fd, const char *path)
{
	return check_elf_file_type(fd, path, ET_EXEC, "debug");
}

static bool
lookup_section(Elfinfo *el, const char *secname, Elf32_Shdr **p_sh)
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

bool
read_elf_section(Elfinfo *el, const char *what, Elf32_Shdr *sh, void *buf)
{
	return read_chunk(el->path, el->what, el->fd, what,
			  (off_t)sh->sh_offset, buf, sh->sh_size);
}

bool
elf_find_section(Elfinfo *el, const char *secname, const char *what,
		 Elf32_Shdr **p_sh)
{
	if (!lookup_section(el, secname, p_sh)) {
		errf("Can't find %s in ELF %s %s (no `%s' section)",
		     what, el->what, el->path, secname);
		return FALSE;
	}

	return TRUE;
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
validate_elf_file(const char *path, Elf32_Ehdr *eh)
{
	if (eh->e_type == ET_EXEC) {
		if (eh->e_phnum == 0) {
			errf("No program header table in ELF file %s", path);
			return FALSE;
		}
		
		if (eh->e_phentsize != sizeof(Elf32_Phdr)) {
			errf("Wrong program header entry size in %s "
			     "(expected %ld, found %d)",
			     path, (long)sizeof(Elf32_Phdr), eh->e_phentsize);
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

static bool
get_load_addrs(const char *path, Elf32_Phdr *phtab, int phnum,
	       taddr_t *p_text_load_addr, size_t *p_text_size,
	       off_t *p_addr_to_fpos_offset, taddr_t *p_min_file_vaddr)
{
	int i;
	Elf32_Phdr *textph, *loadph;

	textph = loadph = NULL;

	for (i = 0; i < phnum; ++i) {
		Elf32_Phdr *ph;

		ph = &phtab[i];
		
		if (ph->p_type == PT_LOAD) {
			if (loadph == NULL || ph->p_vaddr < loadph->p_vaddr)
				loadph = ph;
			
			/*  We assume that the first read-only loadable segment
			 *  is the text segment.
			 */
			if ((ph->p_flags & PF_W) == 0)
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

	*p_text_load_addr = textph->p_vaddr;
	*p_text_size = textph->p_memsz;
	*p_addr_to_fpos_offset = textph->p_vaddr - textph->p_offset;

	*p_min_file_vaddr = loadph->p_vaddr - loadph->p_offset;

	return TRUE;
}

bool
prepare_to_parse_elf_file(const char *textpath, int fd,
			  Elf32_Ehdr *eh, Elfinfo **p_el, off_t ar_offset)
{
	Elfinfo *el;
	Elf32_Shdr *strsh;
	alloc_pool_t *ap;

	if (!validate_elf_file(textpath, eh))
		return FALSE;

	ap = alloc_create_pool();
	el = (Elfinfo *)alloc(ap, sizeof(Elfinfo));

	el->apool = ap;

	el->path = textpath;
	el->fd = fd;
	el->sections = alloc(ap, (size_t)eh->e_shnum * eh->e_shentsize);

	if (!read_chunk(textpath, Elfwhat, fd, "section header table",
			(off_t)eh->e_shoff + ar_offset, el->sections,
			(size_t)eh->e_shnum * eh->e_shentsize)) {
		alloc_free_pool(ap);
		return FALSE;
	}

	strsh = &el->sections[eh->e_shstrndx];

	el->num_sections = eh->e_shnum;
	el->sec_strings = allocstr(ap, strsh->sh_size);

	/*  In Japanese language no article. [...] In Japanese hotel rooms
	 *  not many articles either.  -- David Lodge.
	 */
	el->what = strchr(elftype_to_what(eh->e_type), ' ') + 1;

	if (!read_chunk(textpath, Elfwhat, fd, "section strings table",
			(off_t)strsh->sh_offset + ar_offset,
			(char *)el->sec_strings, strsh->sh_size)) {
		alloc_free_pool(ap);
		return FALSE;
	}

	*p_el = el;
	return TRUE;
}

bool
elf_get_exec_info(const char *textpath, int fd, Libdep *libdep,
		  Execinfo *ex, Elfinfo **p_el, symtab_type_t *p_st_is)
{
	Elf32_Ehdr hdr;
	Elf32_Phdr *phtab;
	Elf32_Shdr *textsh;
	Elf32_Shdr *symsh, *symstrsh, *indexsh, *indexstrsh;
	Elfinfo *el;
	const char *symtab_secname;
#if WANT_DWARF
	Elf32_Shdr *dwarfsh;
	Dwarf_Debug dw_dbg;
	Dwarf_Error dw_err;
	int rv;
#endif

	if (!read_chunk(textpath, "", fd, "ELF header",
			(off_t)0, &hdr, sizeof(Elf32_Ehdr)))
		return FALSE;

	if (!prepare_to_parse_elf_file(textpath, fd, &hdr, &el, (off_t)0))
		return FALSE;

	if (!elf_find_section(el, ".text", "text", &textsh)) {
		free_elfinfo(el);
		return FALSE;
	}

	if (lookup_section(el, ".symtab", &el->symtab_sh)) {
		symtab_secname = ".symtab";
	}
	else {
		if (!lookup_section(el, ".dynsym", &el->symtab_sh)) {
			errf("Can't debug %s: no .symtab or .dynsym section",
			     el->path);
			free_elfinfo(el);
			return FALSE;
		}

		/* RGA don't complain about stripped libraries on invocation */
		if (get_message_wn() != -1)
		  errf("Warning: no .symtab section in %s - "
		       "using .dynsym instead", el->path);
		symtab_secname = ".dynsym";
	}

	if (el->symtab_sh->sh_link >= el->num_sections) {
		errf("sh_link (%ld) for %s section of %s not in range 0..%d",
		     el->symtab_sh->sh_link, symtab_secname, el->path,
		     el->num_sections - 1);
		free_elfinfo(el);
		return FALSE;
	}

	if (!lookup_section(el, ".plt", &el->pltsh)) {
		el->pltsh = NULL;
	}

#if WANT_DWARF
	if (lookup_section(el, ".debug_info", &dwarfsh)) {
		*p_st_is = ST_DWARF;
		symsh = symstrsh = NULL;
	} else
#endif
	if (lookup_section(el, ".stab", &symsh)) {
		if (!lookup_section(el, ".stabstr", &symstrsh)) {
			errf("Found .stab section in %s but no "
			     ".stabstr strings section for it", el->path);
			free_elfinfo(el);
			return FALSE;
		}
		*p_st_is = ST_STABS;
	}
	else {
		symsh = symstrsh = NULL;
	}
		
	if (lookup_section(el, ".stab.index", &indexsh)) {
		if (!lookup_section(el, ".stab.indexstr", &indexstrsh)) {
			errf("Found .stab.index section in %s but no "
			     ".stab.indexstr strings section for it", el->path);
			indexsh = NULL;
		}
	}
	else {
		indexsh = indexstrsh = NULL;
	}

	if (!check_elf_entsize(symsh, ".stab", sizeof(struct nlist)) ||
	    !check_elf_entsize(indexsh, ".stab.index", sizeof(struct nlist)) ||
	    !check_elf_entsize(el->symtab_sh, ".symtab", sizeof(Elf32_Sym)))
		return FALSE;

	phtab = e_malloc((size_t)(hdr.e_phentsize * hdr.e_phnum));
	if (!read_chunk(textpath, Elfwhat, fd, "program header table",
			(off_t)hdr.e_phoff,
			phtab, (size_t)(hdr.e_phentsize * hdr.e_phnum)) ||
	    !get_load_addrs(textpath, phtab, hdr.e_phnum,
			    &ex->text_mem_addr, &ex->text_size,
			    &ex->addr_to_fpos_offset,
			    &el->min_file_vaddr)) {
		free(phtab);
		free_elfinfo(el);
		return FALSE;
	}
	free(phtab);

	el->libdep = libdep;
	el->indexsh = indexsh;
	el->indexstrsh = indexstrsh;

	ex->text_addr_delta = 0;
	ex->dynamic = FALSE;
	ex->entry_addr = hdr.e_entry;

#if WANT_DWARF
	if (*p_st_is == ST_DWARF) {
		ex->file_syms_offset = 0;
		ex->nsyms = 0;
		ex->file_symstrings_offset = 0;
	}
	else
#endif
	if (symsh != NULL) {
		ex->file_syms_offset = symsh->sh_offset;
		ex->nsyms = symsh->sh_size / symsh->sh_entsize;
		ex->file_symstrings_offset = symstrsh->sh_offset;
	}
	else {
		ex->file_syms_offset = 0;
		ex->nsyms = 0;
		ex->file_symstrings_offset = 0;
	}

	*p_el = el;

	return TRUE;
}

void
free_elfinfo(Elfinfo *el)
{
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
