/* ao_core.c - code for reading and writing core files */

/*  Copyright 1993 Mark Russell, University of Kent at Canterbury.
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


/* @(#)ao_core.c	1.5 24 May 1995 (UKC) */
char ups_ao_core_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#if defined(AO_TARGET)

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "data.h"
#include "ao_core.h"
#include "ao_elfcore.h"
#include "ao_aout.h"

/*  Information about a core file.
 */
struct Coredesc {
	int fd;
	Core_segment *segments;
	int nsegments;
	const char *cmdline;
	char *regs;
};

static bool check_is_corefile PROTO((const char *corename, bool user_gave_core,
				     struct stat *st));
static int core_matches_text PROTO((const char *corename, const char *textname,
				    const char *cmdname, time_t core_mtime,
				    int user_gave_core));
static bool find_core_segment PROTO((Coredesc *co,
				     taddr_t addrbase, taddr_t addrlim,
				     Core_segment **p_cs));

#ifdef AO_ELF
#define ps_get_core_info	elf_get_core_info
#else
#define ps_get_core_info	aout_get_core_info
#endif

static bool
check_is_corefile(corename, user_gave_core, st)
const char *corename;
bool user_gave_core;
struct stat *st;
{
	if (stat(corename, st) < 0) {
		if (user_gave_core)
			failmesg("Can't stat", "core file", corename);
	}
	else if (!S_ISREG(st->st_mode)) {
		if (user_gave_core)
			errf("%s is not a regular file", corename);
	}
	else {
		return TRUE;
	}

	return FALSE;
}

static int
core_matches_text(corename, textname, cmdname, core_mtime, user_gave_core)
const char *corename, *textname, *cmdname;
time_t core_mtime;
int user_gave_core;
{
	struct stat tstat;
	
	if (stat(textname, &tstat) < 0) {
		if (user_gave_core)
			failmesg("Can't stat", "", textname);
		return FALSE;
	}
	
	if (cmdname != NULL) {
		const char *basetext, *basecmd;
		size_t basetext_len, cmdname_len, len;
		
		if ((basetext = strrchr(textname, '/')) == NULL)
			basetext = textname;
		else
			++basetext;
		basetext_len = strlen(basetext);
		
		if ((basecmd = strrchr(cmdname, '/')) == NULL)
			basecmd = cmdname;
		else
			++basecmd;
		cmdname_len = strlen(basecmd);
		
		len = (basetext_len < cmdname_len) ? basetext_len : cmdname_len;
	
		if (strncmp(basetext, basecmd, len) != 0) {
			if (user_gave_core) {
				errf("Warning: %s dumped from %s not %s",
				     corename, cmdname, textname);
			}
			else {
				return FALSE;
			}
		}
	}
	
	if (core_mtime < tstat.st_mtime) {
		if (user_gave_core) {
			errf("Warning: core file %s is older than %s",
			     corename, textname);
		}
		else {
			return FALSE;
		}
	}
	
	return TRUE;
}

/*  Check that corename is a core dump and was dumped from textname.
 *
 *  If user_gave_core is FALSE, we are silent about most things.
 *  Otherwise we complain about errors, but we accept (with a warning)
 *  out of date core files and core files which don't seem to have been
 *  dumped from textname.
 *
 *  The data_addr argument is the address in memory of the start of
 *  the text segment.  It is only used on systems where we can't get
 *  this information from the core file.
 *
 *  On the Sun (where we can attach to a running process, we treat a numeric
 *  corename as a process id, and check that we can send a signal to that
 *  process.
 */
bool
open_corefile(ap, corename, textname, user_gave_core, data_addr,
	      p_core, p_lastsig)
alloc_pool_t *ap;
const char *corename, *textname;
int user_gave_core;
taddr_t data_addr;
Coredesc **p_core;
int *p_lastsig;
{
	Coredesc *co;
	Core_segment *segments;
	const char *warn;
	char *cmdname;
	const char *cmdline;
	int fd, signo, nsegments;
	char *regs;
	struct stat stbuf;
	
	*p_core = NULL;
	*p_lastsig = 0;

	if (is_number(corename)) {
#if defined(OS_SUNOS) || defined(OS_LINUX) || defined(OS_BSD44)
		if (kill(atoi(corename), 0) == 0)
			return TRUE;
		if (errno == EPERM)
			errno = EACCES;
		failmesg("Can't send signal to", "process", corename);
		return !user_gave_core;
#else
		errf("Can't attach to processes on this machine");
		return !user_gave_core;
#endif /* !(defined(OS_SUNOS) || defined(OS_LINUX)) */
	}

	if (!check_is_corefile(corename, user_gave_core, &stbuf))
		return !user_gave_core;
	
	if ((fd = open(corename, O_RDONLY)) == -1) {
		warn = user_gave_core ? "" : "warning: ";
		errf("%scan't open core file %s: %s",
		     warn, corename, get_errno_str());
		return !user_gave_core;
	}

	if (!ps_get_core_info(ap, corename, fd, user_gave_core,
			      &signo, &cmdname, &cmdline,
			      &segments, &nsegments, &regs)) {
		close(fd);
		return !user_gave_core;
	}

	if (!core_matches_text(corename, textname, cmdname, stbuf.st_mtime,
			       user_gave_core)) {
		close(fd);	/* RGA */
		free(cmdname);
		return !user_gave_core;
	}

	free(cmdname);
	
	co = (Coredesc *)alloc(ap, sizeof(Coredesc));
	co->fd = fd;
	co->segments = segments;
	co->nsegments = nsegments;
	co->regs = regs;
	co->cmdline = cmdline;

#ifndef AO_ELF
	co->cmdline = get_command_line_from_stack(ap, co);
#endif
	
	*p_core = co;
	*p_lastsig = signo;
	return TRUE;
}

char *
core_getregs(co)
Coredesc *co;
{
	return co->regs;
}

const char *
core_get_cmdline(co)
Coredesc *co;
{
	return co->cmdline;
}

static bool
find_core_segment(co, addrbase, addrlim, p_cs)
Coredesc *co;
taddr_t addrbase, addrlim;
Core_segment **p_cs;
{
	Core_segment *cs, *lim;
	
	lim = &co->segments[co->nsegments];
	
	for (cs = co->segments; cs < lim; ++cs) {
		if (addrbase >= cs->base && addrlim <= cs->lim) {
		       *p_cs = cs;
		       return TRUE;
	       }
	}

	return FALSE;
}
			
bool
core_dread(co, addr, buf, nbytes)
Coredesc *co;
taddr_t addr;
voidptr buf;
size_t nbytes;
{
	off_t offset;
	Core_segment *cs;
	size_t rrv;

	if (!find_core_segment(co, addr, addr + nbytes, &cs))
		return FALSE;
	
	offset = cs->file_offset + (addr - cs->base);
			
	if (lseek(co->fd, offset, SEEK_SET) == -1)
		return FALSE;

	rrv = read(co->fd, buf, nbytes);
	return (size_t)rrv == nbytes;
}

bool
core_read(co, offset, buf, nbytes)
Coredesc *co;
off_t offset;
voidptr buf;
size_t nbytes;
{
	if (lseek(co->fd, offset, SEEK_SET) == -1)
		return FALSE;
	return read(co->fd, buf, nbytes) == nbytes;
}
	
bool
core_readstr(co, addr, buf, buflen)
Coredesc *co;
taddr_t addr;
char *buf;
size_t buflen;
{
	Core_segment *cs;
	off_t offset;
	size_t maxlen, nbytes;

	if (!find_core_segment(co, addr, addr + 1, &cs))
		return FALSE;

	offset = cs->file_offset + (addr - cs->base);
	
	if (lseek(co->fd, offset, SEEK_SET) == -1)
		return FALSE;

	maxlen = cs->lim - addr;
	nbytes = (buflen < maxlen) ? buflen : maxlen;
	
	return (read(co->fd, buf, nbytes) == nbytes &&
		memchr(buf, '\0', nbytes) != NULL);
}

#endif /* AO_TARGET */
