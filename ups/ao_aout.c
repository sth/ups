/* ao_aout.h - low level handling of a.out files */

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


/* @(#)ao_aout.c	1.2 24 May 1995 (UKC) */
char ups_ao_aout_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#ifndef AO_ELF

#include <a.out.h>
#include <sys/param.h>
#ifdef ARCH_VAX
#include <sys/dir.h>	/* vax needs this for user.h */
#endif
#include <signal.h>
#include <errno.h>

#include <sys/types.h>
#include <unistd.h>
#include <a.out.h>
#include <stab.h>
#if !defined(OS_BSD44) && !defined(OS_LINUX) /* RGA linux merge */
#include <sys/vmparam.h>
#endif

#ifndef OS_SUNOS
#include <sys/user.h>
#endif

#ifdef ARCH_MIPS
#include <machine/reg.h>
#endif

#ifdef OS_SUNOS

#ifdef OS_SUNOS_4
#define FPU
#define c_fpstatus	c_fpu.f_fpstatus
#define c_fparegs	c_fpu.f_fparegs
#endif /* OS_SUNOS_4 */

#include <sys/core.h>

#define DATADDR(c)	N_DATADDR(c.c_aouthdr)
#endif /* OS_SUNOS */

#ifndef L_SET
#define L_SET 0
#endif

#ifdef ARCH_MIPS
#include "mips_frame.h"	/* mreg.h needs this for MIPS_SP_REGNO */
#endif

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <mtrprog/io.h>

#include "ups.h"
#include "symtab.h"
#include "st.h"
#include "target.h"
#include "ao_execinfo.h"
#include "ao_syms.h"
#include "ao_text.h"
#include "ao_core.h"
#include "ao_aout.h"
#include "ao_shlib.h"
#include "mreg.h"

#ifdef ARCH_VAX
#define N_TXTADDR(x)	0
#endif

#ifdef ARCH_CLIPPER
#ifndef N_TXTADDR
#define N_TXTADDR(x)	0x8000
#endif
#endif

#ifdef USERCORE
struct core {
	union c_header {
		struct user cu_u;
		char cu_dummy[ctob(UPAGES)];
	} c_un;
};
#endif

#ifdef ARCH_BSDI386
static int get_uword PROTO((int arg, int offset));
#endif
#ifdef OS_SUNOS_4
static bool get_rwindow PROTO((int fd, const char *corepath, struct core *c, 
                               Core_segment *ss, bool want_messages, 
                               sunregs_t *sr));
#endif


#ifdef USERCORE
#undef DATADDR

#ifndef DATADDR
#define DATADDR(u)	)
#endif

#define CORE_NAMELEN	MAXNAMLEN
#endif /* USERCORE */

bool
aout_get_exec_info(textpath, fd, shlib_load_addr, ei)
const char *textpath;
int fd;
taddr_t shlib_load_addr;
Execinfo *ei;
{
	struct exec hdr;
	taddr_t txtaddr;
	
	if (read(fd, (char *)&hdr, sizeof(hdr)) != sizeof(hdr)) {
		failmesg("Read error in", "", textpath);
		return FALSE;
	}

	/*  TXTADDR is sometimes caddr_t, thus the taddr_t cast.
	 */
	txtaddr = (taddr_t)N_TXTADDR(hdr);
	
	ei->text_mem_addr = txtaddr + shlib_load_addr;
	ei->text_size = hdr.a_text;
	ei->text_addr_delta = shlib_load_addr;
	
	ei->addr_to_fpos_offset = ei->text_mem_addr - N_TXTOFF(hdr);
	
	ei->file_syms_offset = N_SYMOFF(hdr);
	ei->nsyms = hdr.a_syms / SYMSIZE;
	ei->file_symstrings_offset = N_STROFF(hdr);

#if defined(OS_SUNOS)
	ei->dynamic = hdr.a_dynamic;
#elif defined (OS_BSD)
	ei->dynamic = N_GETFLAG(hdr) & EX_DYNAMIC;
#else
	ei->dynamic = FALSE;
#endif

	ei->entry_addr = hdr.a_entry;

	return TRUE;
}

/*  Open the a.out file textpath and check that it's reasonable.
 *
 *  If the file is OK, return a file descriptor referring to textpath
 *  open for reading and positioned at the start of the file.
 *
 *  On some machines we set *p_text_addr and *p_data_addr to the
 *  addresses of the start of the text and data.  We do this on
 *  machines where this information can't be got from the core file.
 *
 *  Otherwise give an error message and return -1.
 */
bool
aout_check_exec_header(fd, textpath, p_data_addr)
int fd;
const char *textpath;
taddr_t *p_data_addr;
{
#ifdef ST_COFF
	struct filehdr hdr;
#else
	struct exec hdr;
#endif

	if (!read_chunk(textpath, "", fd, "a.out header",
			(off_t)0, (char *)&hdr, sizeof(hdr)))
		return FALSE;

#ifdef ST_COFF
	if (hdr.f_magic != ST_COFF_MAGIC)
		errf("%s is not an executable file for this machine", textpath);
	else if (hdr.f_nsyms == 0)
		errf("%s has been stripped", textpath);
#else
	if (N_BADMAG(hdr))
		errf("%s is not an executable file for this machine", textpath);
	else if (hdr.a_syms == 0)
		errf("%s has been stripped", textpath);
	else if (hdr.a_syms % SYMSIZE != 0)
		errf("Bad symbol table size %ld (not a multiple of %ld)",
						hdr.a_syms, SYMSIZE);
#endif /* !ST_COFF */
	else if (lseek(fd, L_SET, (long)0) == -1)
		failmesg("Can't lseek in", "", textpath);
	else {
#ifdef ARCH_CLIPPER
		*p_data_addr = (taddr_t)N_DATADDR(hdr);
#else
		*p_data_addr = 0;
#endif
		return TRUE;
	}
	
	close(fd);
	return FALSE;
}

/*  Get the main symbol table for textpath, via fd which should
 *  be a file descriptor referring to textpath.  The file pointer
 *  is assumed to be at the start of the file.
 * 
 *  If textpath is a shared library, we try to guess which
 *  shared libraries it will use and preload these into the
 *  shared library cache.  This is to allow the user to set
 *  breakpoints in shared library functions before the target
 *  is started, and to avoid a long pause when the target is
 *  started for the first time.
 */
bool
aout_scan_main_symtab(textpath, fd, p_symtabs, p_symtab_cache, p_entryaddr, p_mainfunc)
const char *textpath;
int fd;
symtab_t **p_symtabs, **p_symtab_cache;
taddr_t *p_entryaddr;
func_t **p_mainfunc;
{
	symtab_t *st;
	
	if (!aout_scan_symtab(textpath, fd, (taddr_t)0, &st, p_entryaddr, p_mainfunc))
		return -1;

	*p_symtab_cache = NULL;
#if defined(OS_SUNOS_4) || defined(OS_BSD44)
	if (AO_STDATA(st)->st_dynamic) {
		if (make_symtab_cache(textpath, p_symtab_cache) != 0)
			return -1;
	}
#endif
	*p_symtabs = st;

	return TRUE;
}

/*  Do a prescan of the symbol table of textpath.
 */
bool
aout_scan_symtab(textpath, fd, shlib_load_addr, p_symtab, p_entryaddr, p_mainfunc)
const char *textpath;
int fd;
taddr_t shlib_load_addr;
symtab_t **p_symtab;
taddr_t *p_entryaddr;
func_t **p_mainfunc;
{
	symtab_t *st;
	bool ok;
	Execinfo eibuf;
	const char *mainfunc_name, **p_mainfunc_name;
	func_t *flist;
	symtab_type_t st_is = ST_STABS;

#ifdef ST_COFF
	ok = coff_get_exec_info(textpath, fd, &eibuf);
#else
#ifdef ST_TE
	ok = te_get_exec_info(textpath, fd, &eibuf);
	symtab_type_t st_is = ST_ThirdEye;
#else
	ok = aout_get_exec_info(textpath, fd, shlib_load_addr, &eibuf);
#endif /* !ST_TE */
#endif /* !ST_COFF */

	if (!ok)
		return FALSE;
	
	*p_entryaddr = eibuf.entry_addr;

	mainfunc_name = NULL;
	p_mainfunc_name = (p_mainfunc != NULL) ? &mainfunc_name : NULL;

	if (!scan_ao_symtab(textpath, fd, &eibuf, NULL, shlib_load_addr,
			   st_is, &st, &flist, p_mainfunc_name)) {
		return FALSE;
	}
	
	do_ao_postscan_stuff(st, flist, &eibuf, p_mainfunc, mainfunc_name, NULL, NULL);

	*p_symtab = st;
	return TRUE;
}

/*
 *  Get a register value from a core file.
 *  SunOS has the same structure (sunregs_t) in co->regs as when running
 *  an executable.
 *  The rest have to go through the u-area so have an array of ureg_t in
 *  co->regs.  NOTE - used by systems that would not go through the u-area
 *  to get at the registers when running an executable.
 */
int
aout_get_core_reg(co, regno, p_val)
Coredesc *co;
int regno;
taddr_t *p_val;
{
#if defined(OS_SUNOS)
	*p_val = get_sun_regval((sunregs_t *)core_getregs(co), 0, regno);
	return 0;
#elif defined(USERCORE)
	ureg_t *uregs;
	taddr_t addr;
#ifdef ARCH_MIPS
	int mreg;

	switch (regno) {
	case UPSREG_PC:
		mreg = EF_EPC;
		break;
	case UPSREG_SP:
		mreg = EF_SP;
		break;
	case UPSREG_FP:
	case UPSREG_AP:
		mreg = EF_SP;	/* for now */
		break;
	default:
		if (regno < 0 || regno >= 32)
			panic("bad reg in core_getreg");
		mreg = regno + 3;	/* look at <machine/reg.h> */
		break;
	}
	fpos = (off_t)&CORE_REG(mreg, 0);
#else /* ARCH_MIPS */
	uregs = (ureg_t *)core_getregs(co);
	addr = uregs[reg_to_uarea_index(regno)].ur_uaddr;
#endif /* !ARCH_MIPS */
	if (!core_read(co, (off_t)addr, (char *)p_val, 4))
		return -1;
	return 0;
#else
	panic("core_getreg NYI");
	return 0;	/* to satisfy gcc */
#endif /* !USERCORE*/
}

#ifdef OS_SUNOS
static bool
get_rwindow(fd, corepath, c, ss, user_gave_core, sr)
int fd;
const char *corepath;
struct core *c;
Core_segment *ss;
bool user_gave_core;
sunregs_t *sr;
{
#ifdef ARCH_SUN4
	off_t offset;
	int nread;
	
	sr->sr_fpu = c->c_fpu;			/* ditto */
	sr->sr_need_fpu = FALSE;

	offset = ss->file_offset + ((taddr_t)c->c_regs.r_sp - ss->base);
	
	if (lseek(fd, offset, SEEK_SET) == -1) {
		errf("%sCan't seek to find core registers in core file %s: %s",
		     user_gave_core ? "" : "Warning: ",
		     corepath, get_errno_str());
		return FALSE;
	}

	nread = read(fd, (char *)&sr->sr_rwindow, sizeof(sr->sr_rwindow));
	if (nread != sizeof(sr->sr_rwindow)) {
		if (nread == -1) {
			errf("%sError reading registers from core file %s: %s",
			     user_gave_core ? "" : "Warning: ", corepath,
			     get_errno_str());
		}
		else {
			errf("%sEOF reading registers from core file %s",
			     user_gave_core ? "" : "Warning: ", corepath);
		}
		return FALSE;
	}
#endif /* ARCH_SUN4 */
	
	sr->sr_regs = c->c_regs;		/* structure assignment */
	return TRUE;
}
#endif /* OS_SUNOS */

bool
aout_get_core_info(ap, corepath, fd, want_messages,
		   p_signo, p_cmdname, p_cmdline,
		   p_segments, p_nsegments, p_regs)
alloc_pool_t *ap;
const char *corepath;
int fd;
bool want_messages;
int *p_signo;
char **p_cmdname;
const char **p_cmdline;
Core_segment **p_segments;
int *p_nsegments;
char **p_regs;
{
	Core_segment *segments, dataseg, stackseg;
	char *cmdname;
	off_t data_offset;
	size_t data_size, stack_size;
	int signo;
	char *regs;
	struct core core;
#ifdef USERCORE
	struct user *u;
#endif
	
#if defined(OS_SUNOS_4)
	if (read(fd, (char *)&core, sizeof(core)) != sizeof(core)) {
		if (want_messages)
			failmesg("Can't read", "core file", corepath);
		return -1;
	}
	
	if (core.c_magic != CORE_MAGIC) {
		if (want_messages)
			errf("%s is not a core file", corepath);
		return -1;
	}

	regs = e_malloc(sizeof(sunregs_t));
	cmdname = core.c_cmdname;
	
	data_offset = core.c_len;
	data_size = core.c_dsize;
	stack_size = core.c_ssize;
	signo = core.c_signo;
#elif defined(USERCORE)
	if (read(fd, (char *)&core, sizeof(core)) != sizeof(core)) {
		if (want_messages)
			errf("Can't read", "core file", corepath);
		return -1;
	}
	u = &(core.c_un.cu_u);
	data_size = ctob(u->u_dsize);
	stack_size = ctob(u->u_ssize);
	
#if defined(OS_BSD44)
	regs = e_malloc(sizeof(ureg_t) * N_UREGS);
	set_uarea_reg_offsets((ureg_t *)regs, get_uword, (int)&core);
#elif defined(OS_LINUX)
	regs = e_malloc(sizeof(pt_regs));
	memcpy (regs, u->regs, sizeof(pt_regs));
#else
	regs = e_malloc(sizeof(ureg_t) * N_UREGS);
	set_uarea_reg_offsets((ureg_t *)regs);
#endif

#if defined(OS_BSD44)
	cmdname = u->u_kproc.kp_proc.p_comm;
	signo = u->u_sig;
	data_offset = sizeof(struct core);
#else
	cmdname = u->u_comm;
	signo = 0;
	data_offset = sizeof(struct user);
#endif

#endif /* USERCORE */
	
#if defined(ARCH_CLIPPER)
	dataseg.base = data_addr;
#elif defined(OS_SUNOS_4)
	dataseg.base = (taddr_t)N_DATADDR(core.c_aouthdr);
#elif defined(ARCH_MIPS)
	dataseg.base = USRDATA;
#elif defined(ARCH_386) && defined(OS_BSD44)
	dataseg.base = (taddr_t)u->u_kproc.kp_eproc.e_vm.vm_daddr;
#elif defined(USERCORE)
	dataseg.base = ctob((u)->u_tsize);
#endif
	dataseg.lim = dataseg.base + data_size;
	dataseg.file_offset = data_offset;

	stackseg.base = (taddr_t)(USRSTACK - stack_size);
	stackseg.lim = (taddr_t)USRSTACK;
	stackseg.file_offset = data_offset + data_size;
	
#ifdef OS_SUNOS_4
	if (!get_rwindow(fd, corepath, &core, &stackseg, want_messages,
			 (sunregs_t *)regs))
		return FALSE;
#endif

	segments = (Core_segment *)alloc(ap, 2 * sizeof(Core_segment));
	segments[0] = dataseg;
	segments[1] = stackseg;
	
	*p_signo = signo;
	*p_cmdname = strsave(cmdname);
	*p_cmdline = NULL;
	*p_segments = segments;
	*p_nsegments = 2;
	*p_regs = regs;
	
	return TRUE;
}
	

#ifdef ARCH_BSDI386
static int
get_uword(arg, offset)
int arg, offset;
{
	struct core *cp;

	cp = (struct core *)arg;

	if (offset < 0 || offset >= sizeof(cp->c_un.cu_u))
		panic("get_uword offset botch");
	
	return *(int *)&cp->c_un.cu_dummy[offset];
}
#endif


/*  Attempt to get the argv array of the target from the stack so
 *  we can put the arguments in the command line.  If we succeed,
 *  put as many arguments as will fit in command_line, which is
 *  taken to be command_line_size bytes long, and return 0.
 *
 *  If we fail, return -1 (with no error message).
 *
 *  FRAGILE CODE
 *  
 *  This function assumes the following stack layout, starting from
 *  the top of the stack:
 *
 *	Zero or more words with the value zero
 *	The actual bytes of the argument and environment strings
 *	A zero word
 *	Zero or more pointers to environment strings
 *	A zero word
 *	One or more pointers to argument strings
 *
 *  We get confused if the argument bytes contain a zero word - this
 *  can happen if a command is invoked with four consecutive zero length
 *  arguments.
 *
 *  It's not fatal for this routine to fail - all that happens is that
 *  no arguments are displayed on the target line.
 *
 *  BSD 4.4 seems to have changed the format, from the top of the stack:
 *
 *	Count of the number of environment strings 	1 word
 *	Pointer 					1 word
 *	Count of the number of argument strings (argc)	1 word
 *	Pointer					 	1 word
 *	lots of zero bytes to ensure the above 4 words are at the end
 *	  of a page(?)
 *	Environment strings
 *	Argument strings (start on word boundary)
 */
const char *
get_command_line_from_stack(ap, co)
alloc_pool_t *ap;
Coredesc *co;
{
	static char dotdotdot[] = " ...";
	enum { INITIAL, IN_STRINGS, IN_ENVP, IN_ARGV, FINAL } state;
	char cmdline[2048];	/* Max cmd line length we will handle */
	char argbuf[512];
	char *lptr, *lim;
	int i, argc;
	taddr_t *argv, *envp;
	taddr_t ibuf[128];
	taddr_t *iptr, word, sp, maxsp;

	argc = 0; /* gcc doesn't understand state machines */
	sp = maxsp = (taddr_t)USRSTACK;
	iptr = ibuf;

#ifdef OS_BSD44
	if (!core_dread(co, sp - sizeof(ibuf), (char *)ibuf, sizeof(ibuf)))
		return NULL;
	iptr = ibuf + sizeof(ibuf) / sizeof(ibuf[0]);

	word = *--iptr;			/* No. of envp entries. */
	word = *--iptr;			/* Pointer to envp entries. */

	argc = word = *--iptr;		/* No. of argv entries. */
	sp = (taddr_t)word = *--iptr;	/* Pointer to argv entries. */
	sp -= 4;			/* (gets undone later) */
#else
	for (state = INITIAL; state != FINAL; ) {

		/*  Safety limit - if the stack is bad we don't
		 *  want to read the entire core.
		 */
		if (maxsp - sp > 30000)
			return NULL;

		if (iptr == ibuf) {
			if (!core_dread(co, sp - sizeof(ibuf), (char *)ibuf,
				       sizeof(ibuf)))
				return NULL;
			iptr = ibuf + sizeof(ibuf) / sizeof(ibuf[0]);
		}

		sp -= 4;
		word = *--iptr;

		switch(state) {
		case INITIAL:
			if (word != 0)
				state = IN_STRINGS;
			break;
		case IN_STRINGS:
			if (word == 0)
				state = IN_ENVP;
			break;
		case IN_ENVP:
			if (word == 0) {
				state = IN_ARGV;
				argc = 0;
			}
			break;
		case IN_ARGV:
			if (word == argc)
				state = FINAL;
			else
				++argc;
			break;
		default:
 			panic("bad state in gclafs");
		}
	}
#endif
	/*  At this point the start of the argument vector is at sp.
	 */
	if (argc == 0)
		return NULL;

	argv = (taddr_t *)e_malloc(argc * sizeof(taddr_t));
	if (!core_dread(co, sp + 4, (char *)argv, argc * sizeof(taddr_t)))
		return NULL;
	
	lptr = cmdline;
	lim = cmdline + sizeof(cmdline) - sizeof(dotdotdot);

	for (i = 0; i < argc; ++i) {
		if (!core_readstr(co, argv[i], argbuf, sizeof(argbuf)))
			return NULL;

		if (lptr + strlen(argbuf) >= lim) {
			(void) strcpy(lptr, dotdotdot);
			break;
		}

		if (i != 0)
			*lptr++ = ' ';
		strcpy(lptr, argbuf);
		lptr += strlen(argbuf);
	}

	return alloc_strdup(ap, cmdline);
}

#endif /* !AO_ELF */
