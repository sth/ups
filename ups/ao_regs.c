/* ao_regs.c - common register related routines */

/*  Copyright 2003 Tom Hughes <thh@cyberscience.com>
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


char ups_ao_regs_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#if ARCH_386

#if HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#include <local/ukcprog.h>

#include "ao_regs.h"

#if defined(ARCH_LINUX386_64)

static const int regmap32[] = {
	RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI, RIP, EFLAGS
};

static const int regmap64[] = {
	RAX, RDX, RCX, RBX, RSI, RDI, RBP, RSP,
	R8, R9, R10, R11, R12, R13, R14, R15
};

#elif defined(ARCH_LINUX386) || defined(ARCH_SOLARIS386)

static const int regmap32[] = {
	EAX, ECX, EDX, EBX, UESP, EBP, ESI, EDI, EIP, EFL
};

static const int *regmap64 = NULL;

#elif defined(ARCH_FREEBSD386) && defined(z__FreeBSD__) && __FreeBSD__ > 3

static const int regmap32[] = {
	/*  9     8     7     6     5    4      3     2 */
	  tECX, tEDX, tEBX, tISP, tEBP, tESI, tEDI, tDS,
	/*  12    14  13   16   1    0 */
	  tERR, tCS, tEIP, tESP, tES, tFS
};

static const int *regmap64 = NULL;

#elif defined(ARCH_FREEBSD386)

static const int regmap32[] = {
	/*  9     8     7     6     5    4      3     2 */
	  tEAX, tECX, tEDX, tEBX, tESP, tEBP, tESI, tEDI,
	/*  12    14      13   16   1    0 */
	  tEIP, tEFLAGS, tCS, tSS, tDS, tES
};

static const int *regmap64 = NULL;

#endif

static const int *regmap = regmap32;

void
x86_gcc_register_init(int addrsize)
{
	if (addrsize == 32) {
		regmap = regmap32;
	}
	else if (addrsize == 64) {
		regmap = regmap64;
	}
        
	return;
}

/*
 *  This maps 'regno' (the register number GCC puts in the symbol table)
 *  to the actual register.
 *  Originally copied from 'do_register_translation()' in ao_pt_uarea.c.
 *  FIX: where does this come from ?  Must be some way to get it out
 *  of the header files.
 */
int
x86_gcc_register(regno)
int regno;
{
	return regmap[regno];
}

#endif /* ARCH_386 */
