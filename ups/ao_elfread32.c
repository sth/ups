/* ao_elfread32.c - functions for reading ELF and ar files  */

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


char ups_ao_elfread32_c_rcsid[] = "$Id$";

#define ELF_ADDRESS_SIZE 32

#define Elf_Addr Elf32_Addr
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Shdr Elf32_Shdr
#define Elf_Phdr Elf32_Phdr
#define Elf_Sym Elf32_Sym
#define Elf_Dyn Elf32_Dyn
#define Elf_Rel Elf32_Rel
#define Elf_Rela Elf32_Rela

#define ELF_ST_TYPE(val) ELF32_ST_TYPE(val)
#define ELF_ST_BIND(val) ELF32_ST_BIND(val)
#define ELF_R_SYM(val) ELF32_R_SYM(val)
#define ELF_R_TYPE(val) ELF32_R_TYPE(val)

#define ELF(x) elf32_##x

#include "ao_elfreadbase.c"
