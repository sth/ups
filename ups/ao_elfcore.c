/* ao_elfcore.c - extract information from an ELF core file */

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


/* @(#)ao_elfcore.c	1.1 24/5/95 (UKC) */
char ups_ao_elfcore_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"
#ifdef AO_ELF

#include <sys/types.h>
#include <elf.h>

#if OS_SUNOS_5
/*
 *  Solaris 2.5 does not have _STRUCTURED_PROC
 *  Solaris 2.6 does     have _STRUCTURED_PROC, but we cannot use it
 *  Solaris 2.7 does     have _STRUCTURED_PROC
 */
#define	_STRUCTURED_PROC	1
#include <sys/procfs.h>

#ifndef _FILE_OFFSET_BITS
#undef _STRUCTURED_PROC
#endif

#ifndef NT_PSTATUS
#undef _STRUCTURED_PROC
#endif

#endif /* OS_SUNOS_5 */

#include <stdlib.h>
#include <string.h>

#include <local/ukcprog.h>
#include <mtrprog/io.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "ao_core.h"
#include "ao_dwarf.h"
#include "ao_elfpriv.h"
#include "ao_elfread.h"
#include "ao_elfcore.h"

#if (defined ARCH_LINUX386)
#if HAVE_SYS_REG_H
#include <sys/reg.h>
#endif
#define GREGSET_STRUCT	0
#else
#define GREGSET_STRUCT	1
#endif


#ifdef _STRUCTURED_PROC
static void
get_pstatus_info(alloc_pool_t *ap, pstatus_t *pr,
		int *p_signo, elf_core_gregset_t **p_regs)
{
	*p_signo = pr->pr_lwp.pr_cursig;

	*p_regs = alloc(ap, sizeof(elf_core_gregset_t));
	memcpy(*p_regs, pr->pr_lwp.pr_reg, sizeof(elf_core_gregset_t));
}
#else
static void
get_prstatus_info(alloc_pool_t *ap, prstatus_t *pr,
		  int *p_signo, elf_core_gregset_t **p_regs)
{
	*p_signo = pr->pr_cursig;

	*p_regs = alloc(ap, sizeof(elf_core_gregset_t));
#if GREGSET_STRUCT
	memcpy(*p_regs, &(pr->pr_reg), sizeof(elf_core_gregset_t));
#else
	memcpy(*p_regs, pr->pr_reg, sizeof(elf_core_gregset_t));
#endif
}
#endif /* not _STRUCTURED_PROC */


#ifdef _STRUCTURED_PROC
static void
get_psinfo_info(alloc_pool_t *ap, psinfo_t *ps,
		char **p_cmdname, const char **p_cmdline)
{
	char *cmdline, *args;
	
	cmdline = alloc(ap, sizeof(ps->pr_psargs) + 1);
	memcpy(cmdline, ps->pr_psargs, sizeof(ps->pr_psargs));
	cmdline[sizeof(ps->pr_psargs)] = '\0';

	args = strchr(cmdline, ' ');

	if (args != NULL) {
		*args++ = '\0';
		*p_cmdname = strsave(cmdline);
		*p_cmdline = args;
	}
	else {
		*p_cmdname = e_malloc(sizeof(ps->pr_fname) + 1);
		memcpy(*p_cmdname, ps->pr_fname, sizeof(ps->pr_fname));
		(*p_cmdname)[sizeof(ps->pr_fname)] = '\0';

		*p_cmdline = cmdline;
	}
}
#else
static void
get_prpsinfo_info(alloc_pool_t *ap, prpsinfo_t *ps,
		  char **p_cmdname, const char **p_cmdline)
{
	char *cmdline, *args;
	
	cmdline = alloc(ap, sizeof(ps->pr_psargs) + 1);
	memcpy(cmdline, ps->pr_psargs, sizeof(ps->pr_psargs));
	cmdline[sizeof(ps->pr_psargs)] = '\0';

	args = strchr(cmdline, ' ');

	if (args != NULL) {
		*args++ = '\0';
		*p_cmdname = strsave(cmdline);
		*p_cmdline = args;
	}
	else {
		*p_cmdname = e_malloc(sizeof(ps->pr_fname) + 1);
		memcpy(*p_cmdname, ps->pr_fname, sizeof(ps->pr_fname));
		(*p_cmdname)[sizeof(ps->pr_fname)] = '\0';

		*p_cmdline = cmdline;
	}
}
#endif /* not _STRUCTURED_PROC */

static bool
get_note_info(alloc_pool_t *ap, const char *path, int fd,
	      Elf32_Phdr *note_ph,
	      int *p_signo, char **p_cmdname, const char **p_cmdline,
	      elf_core_gregset_t **p_regs, elf_core_fpregset_t **p_p_fpregs)
{
	static const char what[] = "ELF core file";
	char *buf, *cmdname;
	const char *cmdline;
	int signo;
	size_t pos;
	elf_core_fpregset_t *fpregs;
	elf_core_gregset_t *regs;

	if (note_ph == NULL) {
		errf("No PT_NOTE section in ELF core file %s", path);
		return FALSE;
	}

	buf = e_malloc(note_ph->p_filesz);
	if (!read_chunk(path, "Elf executable", fd, "note section",
			(off_t)note_ph->p_offset, buf, note_ph->p_filesz)) {
		return FALSE;
	}

	regs = NULL;
	fpregs = NULL;
	cmdname = NULL;
	cmdline = NULL;

	pos = 0;
	while (pos < note_ph->p_filesz) {
		int *ip;
		const char *name, *desc;
		int namesz, descsz, type;

		ip = (int *)&buf[pos];
		namesz = ip[0];
		descsz = ip[1];
		type = ip[2];
		name = (char *)&ip[3];
		desc = name + ((namesz + 3) / 4) * 4;

		switch (type) {
#ifdef _STRUCTURED_PROC
		case NT_PSTATUS:
		  get_pstatus_info(ap, (pstatus_t *)desc, &signo, &regs);
		  break;
		case NT_PSINFO:
		  get_psinfo_info(ap, (psinfo_t *)desc, &cmdname, &cmdline);
		  break;
#else
		case NT_PRSTATUS:
		  get_prstatus_info(ap, (prstatus_t *)desc, &signo, &regs);
		  break;
		case NT_PRPSINFO:
		  get_prpsinfo_info(ap, (prpsinfo_t *)desc, &cmdname, &cmdline);
		  break;
#endif /* not _STRUCTURED_PROC */

#ifdef NT_PRFPREG
		case NT_PRFPREG:
		  fpregs = e_malloc(sizeof(elf_core_fpregset_t));
		  memcpy(fpregs, desc, sizeof(elf_core_fpregset_t));
		  break;
#endif
#ifdef NT_FPREGSET
		case NT_FPREGSET:
		  fpregs = e_malloc(sizeof(elf_core_fpregset_t));
		  memcpy(fpregs, desc, sizeof(elf_core_fpregset_t));
		  break;
#endif

#ifdef NT_TASKSTRUCT
		case NT_TASKSTRUCT:
		  break;
#endif
#ifdef NT_GWINDOWS
		case NT_GWINDOWS:
		  break;
#endif
#ifdef NT_PRCRED
		case NT_PRCRED:
		  break;
#endif
#ifdef NT_UTSNAME
		case NT_UTSNAME:
		  break;
#endif
#ifdef NT_LWPSTATUS
		case NT_LWPSTATUS:
		  break;
#endif
#ifdef NT_LWPSINFO
		case NT_LWPSINFO:
		  break;
#endif
#ifdef NT_PRXREG
		case NT_PRXREG:
		  break;
#endif
#ifdef NT_PLATFORM
		case NT_PLATFORM:
		  break;
#endif
#ifdef NT_AUXV
		case NT_AUXV:
		  break;
#endif
		default:
		  errf("Unknown PT_NOTE type %d in ELF core file %s "
		       "- ignored", type, path);
		  break;
		}

		pos = (desc - buf) + ((descsz + 3) / 4) * 4;
	}

	free(buf);

	if (cmdname == NULL) {
		errf("No prpsinfo_t/psinfo_t item in note section of %s %s", what, path);
		return FALSE;
	}

#if AO_ELF_CORE_REGS
	if (regs == NULL) {
		errf("No prstatus_t/pstatus_t item in note section of %s %s", what, path);
		free(cmdname);
		return FALSE;
	}
#endif

	*p_cmdname = cmdname;
	if (p_regs != NULL)
	    *p_regs = regs;
	if (p_p_fpregs != NULL)
	    *p_p_fpregs = fpregs;
	*p_signo = signo;
	*p_cmdline = cmdline;
	return TRUE;
}

bool
elf_get_core_info(alloc_pool_t *ap, const char *corepath, int fd,
		  bool user_gave_core,
		  int *p_signo, char **p_cmdname, const char **p_cmdline,
		  Core_segment **p_segments, int *p_nsegments, char **p_regs)
{
	Elf32_Ehdr hdr;
	Elf32_Phdr *phtab, *note_ph;
	Core_segment *segments, *sc, *psc;
	unsigned i;
	int nsegments, signo;
	char *cmdname;
	const char *cmdline;
#if AO_ELF_CORE_REGS
	elf_core_gregset_t *regs;
	elf_core_fpregset_t *p_fpregs;
	Elf_core_regs *cr;
#endif
	
	if (!read_chunk(corepath, "", fd, "ELF header",
			(off_t)0, &hdr, sizeof(hdr)) ||
	    !check_elf_header_type(&hdr, corepath, ET_CORE, "debug from")) {
		return FALSE;
	}

	phtab = e_malloc((size_t)(hdr.e_phentsize * hdr.e_phnum));
	
	if (!read_chunk(corepath, "Elf executable", fd, "program header table",
			(off_t)hdr.e_phoff,
			phtab, (size_t)(hdr.e_phentsize * hdr.e_phnum))) {
		free(phtab);
		return FALSE;
	}

	note_ph = NULL;
	nsegments = 0;
	
	for (i = 0; i < hdr.e_phnum; ++i) {
		switch (phtab[i].p_type) {
		case PT_NOTE:
			note_ph = &phtab[i];
			break;
			
		case PT_LOAD:
			if (phtab[i].p_filesz != 0)
				++nsegments;
			break;

		default:
			break;
		}
	}

	if (nsegments == 0) {
		errf("No PT_LOAD segments in ELF core file %s", corepath);
		free(phtab);
		return FALSE;
	}

#if AO_ELF_CORE_REGS
	if (!get_note_info(ap, corepath, fd, note_ph,
			   &signo, &cmdname, &cmdline, &regs, &p_fpregs)) {
#else
	if (!get_note_info(ap, corepath, fd, note_ph,
			   &signo, &cmdname, &cmdline, NULL, NULL)) {
#endif
		free(phtab);
		return FALSE;
	}

	segments = alloc(ap, nsegments * sizeof(Core_segment));
	sc = segments;
	psc = NULL;

	for (i = 0; i < hdr.e_phnum; ++i) {
		Elf32_Phdr *ph;

		ph = &phtab[i];

		if ((ph->p_type == PT_LOAD) && (ph->p_filesz != 0)) {
			if ((psc != NULL) && (psc->lim == ph->p_vaddr)) {
				/* Contiguous with previous segment. */
				psc->lim += ph->p_memsz;
				nsegments--;
			} else {
				sc->base = ph->p_vaddr;
				sc->lim = ph->p_vaddr + ph->p_memsz;
				sc->file_offset = ph->p_offset;
				psc = sc++;
			}
		}
	}

#if AO_ELF_CORE_REGS
	cr = (Elf_core_regs *)alloc(ap, sizeof(Elf_core_regs));
	cr->regtab = regs;
	cr->p_fpregs = p_fpregs;
#endif
	*p_signo = signo;
	*p_cmdname = cmdname;
	*p_cmdline = cmdline;
	*p_segments = segments;
	*p_nsegments = nsegments;
#if AO_ELF_CORE_REGS
	*p_regs = (char *)cr;
#else
	*p_regs = NULL;
#endif
	return TRUE;
}

/*
 *  Get a register value from a core file.
 *  Solaris uses some other routine.
 */
int
elf_get_core_reg(co, regno, p_val)
Coredesc *co;
int regno;
taddr_t *p_val;
{
#if AO_ELF_CORE_REGS
    taddr_t addr;
    Elf_core_regs *ecr;
    elf_core_gregset_t *regs;

    ecr = (Elf_core_regs *)core_getregs(co);
    regs = ecr->regtab;

    /* See /usr/include/.../elf.h and /usr/include/.../ptrace.h
       for the gory details. */

#if (defined ARCH_386)
    switch(regno) {
#if GREGSET_STRUCT
    case UPSREG_PC:
	    addr = (taddr_t)&(regs->r_eip);	/* eip */
	    break;
    case UPSREG_SP:
	    addr = (taddr_t)&(regs->r_esp);	/* esp */
	    break;
    case UPSREG_FP:
    case UPSREG_AP:
	    addr = (taddr_t)&(regs->r_ebp);	/* ebp */
	    break;
#else
    case UPSREG_PC:
	    addr = (taddr_t)&((*regs)[EIP]);	/* eip */
	    break;
    case UPSREG_SP:
	    addr = (taddr_t)&((*regs)[UESP]);	/* esp */
	    break;
    case UPSREG_FP:
    case UPSREG_AP:
	    addr = (taddr_t)&((*regs)[EBP]);	/* ebp */
	    break;
#endif

    default:
	{
	    elf_core_greg_t *iregs = (elf_core_greg_t *)regs;
	    if (regno < 0 || regno >= sizeof(elf_core_gregset_t)/sizeof(elf_core_greg_t))
		    panic("bad regno in egcr");
	    addr = (taddr_t)&(iregs[x86_gcc_register(regno)]);
	}
    } /* switch */
#else
    addr = (taddr_t)&regs[regno];	/* FIX: unlikely to be correct */
#endif

    memcpy ((char *)p_val, (char *)addr, 4);
    return 0;
#else
    errf ("Sorry, cannot get registers from ELF core files on this machine");
    *p_val = 0;
    return 0;
#endif
}

#endif /* AO_ELF */
