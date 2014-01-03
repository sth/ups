/* ao_elfread64.c - functions for reading ELF and ar files  */

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


char ups_ao_elfread64_c_rcsid[] = "$Id$";

#define ELF_ADDRESS_SIZE 64

#define Elf_Addr Elf64_Addr
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr
#define Elf_Phdr Elf64_Phdr
#define Elf_Nhdr Elf64_Nhdr
#define Elf_Sym Elf64_Sym
#define Elf_Dyn Elf64_Dyn
#define Elf_Rel Elf64_Rel
#define Elf_Rela Elf64_Rela

#define ELF_ST_TYPE(val) ELF64_ST_TYPE(val)
#define ELF_ST_BIND(val) ELF64_ST_BIND(val)
#define ELF_R_SYM(val) ELF64_R_SYM(val)
#define ELF_R_TYPE(val) ELF64_R_TYPE(val)

#define ELF_ALIGN(offset) (((offset) + 3) & ~3)

#define ELF(x) elf64_##x

#include "ao_elfreadbase.c"
