/* ao_elfcore.h - header file for ao_elfcore.c */

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


/* @(#)ao_elfcore.h	1.1 24/5/95 (UKC) */

#if HAVE_SYS_PROCFS_H
#include <sys/procfs.h>
#endif

#if HAVE_PRGREG_T
/* Solaris 2.5.1 etc */

#define AO_ELF_CORE_REGS	1
typedef prgreg_t		elf_core_greg_t;
typedef prgregset_t		elf_core_gregset_t;
typedef prfpregset_t		elf_core_fpregset_t;

#elif HAVE_GREG_T || HAVE_GREGSET_T
/* FreeBSD, Linux */

#define AO_ELF_CORE_REGS	1
#if HAVE_GREG_T
typedef greg_t			elf_core_greg_t;
#else
typedef unsigned int		elf_core_greg_t;	/* Not in FreeBSD. */
#endif
typedef gregset_t		elf_core_gregset_t;
typedef fpregset_t		elf_core_fpregset_t;

#else

#define AO_ELF_CORE_REGS	0
/* Dummies to allow compilation. */
typedef int			elf_core_greg_t;
typedef int			elf_core_gregset_t;
typedef int			elf_core_fpregset_t;

#endif

#if AO_ELF_CORE_REGS
typedef struct {
	elf_core_gregset_t *regtab;
	elf_core_fpregset_t *p_fpregs;
} Elf_core_regs;
#endif

bool elf_get_core_info PROTO((alloc_pool_t *ap, const char *corepath, int fd,
			      bool want_messages, int *p_signo,
			      char **p_cmdname, const char **p_cmdline,
			      Core_segment **p_segments, int *p_nsegments,
			      char **p_regs));

int elf_get_core_reg PROTO((Coredesc *co, int regno, taddr_t *p_val));

