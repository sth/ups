/*
strptr.c - implementation of the elf_strptr(3) function.
Copyright (C) 1995 - 2000 Michael Riepe <michael@stud.uni-hannover.de>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <private.h>

#ifndef lint
static const char rcsid[] = "@(#) $Id$";
#endif /* lint */

char*
elf_strptr(Elf *elf, size_t section, size_t offset) {
    Elf_Data *data;
    Elf_Scn *scn;

    if (!elf) {
	return NULL;
    }
    elf_assert(elf->e_magic == ELF_MAGIC);
    if (!(scn = elf_getscn(elf, section))) {
	return NULL;
    }
    if (scn->s_index == SHN_UNDEF) {
	seterr(ERROR_NOSTRTAB);
	return NULL;
    }
    /*
     * checking the section header is more appropriate
     */
    if (elf->e_class == ELFCLASS32) {
	if (scn->s_shdr32.sh_type != SHT_STRTAB) {
	    seterr(ERROR_NOSTRTAB);
	    return NULL;
	}
    }
#if __LIBELF64
    else if (elf->e_class == ELFCLASS64) {
	if (scn->s_shdr64.sh_type != SHT_STRTAB) {
	    seterr(ERROR_NOSTRTAB);
	    return NULL;
	}
    }
#endif /* __LIBELF64 */
    else if (valid_class(elf->e_class)) {
	seterr(ERROR_UNIMPLEMENTED);
	return NULL;
    }
    else {
	seterr(ERROR_UNKNOWN_CLASS);
	return NULL;
    }
    if (offset >= 0 && offset < scn->s_size) {
	data = NULL;
	while ((data = elf_getdata(scn, data))) {
	    if (data->d_buf
	     && offset >= (size_t)data->d_off
	     && offset < (size_t)data->d_off + data->d_size) {
		return (char*)data->d_buf + (offset - data->d_off);
	    }
	}
    }
    seterr(ERROR_BADSTROFF);
    return NULL;
}
