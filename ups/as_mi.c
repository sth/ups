/* as_mi.c - ups machine independent assembler interface */

/*  Copyright 1991 Mark Russell, University of Kent at Canterbury.
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


/* @(#)as_mi.c	1.25 04 Jun 1995 (UKC) */
char ups_as_mi_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <local/edit/edit.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "st.h"
#include "trun.h"
#include "as.h"
#include "debug.h"
#include "state.h"
#include "srcbuf.h"
#include "util.h"

#ifdef KNOW_ASM
static void disassemble PROTO((FILE *fp, func_t *f, taddr_t addr,
			       const char *text, size_t len));
static void disassemble_text_range PROTO((FILE *fp, func_t *f,
					       taddr_t firstaddr, taddr_t lastaddr));
static int disassemble_func PROTO((func_t *f, taddr_t addrlim,
						char *cfp, char *unused_arg2));
static int dump_func_as_assembler PROTO((func_t *f, const char *filename,
					 bool overwrite, bool want_source));
static void dump_srcline PROTO((FILE *fp, Srcbuf *sb, int lnum));

static void
disassemble(fp, f, addr, text, len)
FILE *fp;
func_t *f;
taddr_t addr;
const char *text;
size_t len;
{
	bool want_offset_addrs, flush_each_line;
	const char *name, *next, *lim, *asmline;

	want_offset_addrs = (Debug_flags & DBFLAG_ASM_OFFSET_ADDRS) != 0;
	flush_each_line = (Debug_flags & DBFLAG_ASM_LINEBUF) != 0;

	name = f->fu_name;
	if (want_offset_addrs && *name == '[' && (Debug_flags & DBFLAG_DBXASM)) {
		static char namebuf[50];

		name = strcpy(namebuf, name + 1);
		namebuf[strlen(name) - 1] = '\0';
	}

	lim = text + len;
	while (text < lim) {
		next = st_disassemble_instruction(f, addr, text, &asmline);
		if (want_offset_addrs) {
			int offset;

			offset = addr - f->fu_addr;
			if (offset == 0)
				fprintf(fp, "%s:\t%s\n", name, asmline);
			else if (offset < 10)
				fprintf(fp, "%s+%d:\t%s\n", name, offset, asmline);
			else
				fprintf(fp, "%s+0x%x:\t%s\n", name, offset, asmline);
		}
		else
			fprintf(fp, "\t%08lx  %s\n", addr, asmline);
		if (flush_each_line)
			fflush(fp);
		addr += next - text;
		text = next;
	}
}

static void
disassemble_text_range(fp, f, firstaddr, lastaddr)
FILE *fp;
func_t *f;
taddr_t firstaddr, lastaddr;
{
	char *buf;
	size_t nbytes;

	nbytes = lastaddr + 1 - firstaddr;

	if (nbytes <= 0) {
		fputs("\t<No text>\n", fp);
		return;
	}

	buf = e_malloc(nbytes);
	if (st_read_text(f->fu_symtab, firstaddr, buf, nbytes) == 0)
		disassemble(fp, f, firstaddr, buf, nbytes);
	else
		errf("Tread failed in dtr");
	free(buf);
}

/* ARGSUSED */
static int
disassemble_func(f, addrlim, cfp, unused_arg2)
func_t *f;
taddr_t addrlim;
char *cfp;
char *unused_arg2;
{
	FILE *fp;
	taddr_t base, lim;

	fp = (FILE *)cfp;
	base = f->fu_addr;
#ifdef ARCH_VAX
	base += 2;		/* VAXen have reg save mask at func addr */
#endif
	lim = addrlim - 1;
	if ((Debug_flags & DBFLAG_DBXASM) == 0)
		fprintf(fp, "\n%s: 0x%lx - 0x%lx\n", f->fu_name, base, lim);
	disassemble_text_range(fp, f, base, lim);
	return 0;
}

static int
dump_func_as_assembler(f, filename, overwrite, want_source)
func_t *f;
const char *filename;
bool overwrite, want_source;
{
	char *buf;
	taddr_t addr, nextaddr;
	size_t buflen, linelen;
	lno_t *l;
	FILE *fp;
	target_t *xp;
	symtab_t *symtab;

	if (!fopen_new_file("output file", filename, FALSE, &fp))
		return -1;
	
	xp = get_current_target();
	symtab = (f != NULL) ? f->fu_symtab : xp_get_mainfunc(xp)->fu_symtab;
	
	if (f == NULL) {
		iterate_over_functions(symtab, disassemble_func,
						(char *)fp, (char *)symtab);
	}
	else if (FU_LNOS(f) == NULL || !want_source) {
		disassemble_func(f, st_get_addr_lim(f), (char *)fp,
							(char *)symtab);
	}
	else {
		/*  We don't check for errors here - we just don't print
		 *  source lines later if fi_srcbuf ends up as NULL.
		 */
		open_source_file(f->fu_fil, FALSE, FALSE);
		
		buflen = 128;
		buf = e_malloc(buflen);
		
		for (l = FU_LNOS(f); l != NULL; l = l->ln_next) {
			addr = l->ln_addr;
			
			if (l->ln_next != NULL)
				nextaddr = l->ln_next->ln_addr;
			else
				nextaddr = st_get_addr_lim(f);
			
			if (nextaddr == addr)
				continue;
			
			linelen = nextaddr - addr;
			
			if (linelen > buflen) {
				buflen = linelen;
				buf = e_realloc(buf, buflen);
			}
			
			if (st_read_text(symtab, addr, buf, linelen) != 0) {
				errf("tread failed in dfaa");
				free(buf);
				break;
			}
			
			fprintf(fp, "\n%d: ", l->ln_num);
			if (f->fu_fil->fi_srcbuf != NULL) {
				dump_srcline(fp, f->fu_fil->fi_srcbuf,
					     l->ln_num - 1);
			}
			
			putc('\n', fp);
			
			disassemble(fp, f, addr, buf, linelen);
		}
		free(buf);
	}

	return fclose_new_file("output file", filename, TRUE, fp);
}

static void
dump_srcline(fp, sb, lnum)
FILE *fp;
Srcbuf *sb;
int lnum;
{
	size_t point, start, lim;
	const char *iptr, *ilim;
	Edit_buffer *buffer;
	
	srcbuf_file_lnum_to_point_range(sb, lnum, &start, &lim);

	buffer = srcbuf_get_buffer(sb);
	iptr = ilim = NULL;
	
	for (point = start; point < lim; ++point, ++iptr) {
		if (iptr == ilim &&
		    !edit_get_bytes(buffer, point, &iptr, &ilim))
			break;

		if (*iptr != '\n')
			putc(*iptr, fp);
	}
}
#endif /* KNOW_ASM */

void
dump_as_assembler(funcname, want_source)
const char *funcname;
int want_source;
{
	func_t *f, *f1;
	const char *opath;
	bool overwrite;
	char *what;

	if (!get_debug_output_path(funcname, "asm.s",
				   &funcname, &opath, &overwrite)) {
		return;
	}

	if (*funcname == '\0') {
		f = NULL;
		what = strsave("Entire text");
	}
	else {
		if (find_func_by_name(funcname, &f, &f1, TRUE) != 0)
			return;

		if (f->fu_fil == NULL)
			what = strsave(f->fu_name);
		else
			what = strf("%s:%s", f->fu_fil->fi_name, f->fu_name);
	}

#ifdef KNOW_ASM
	if (dump_func_as_assembler(f, opath, overwrite, want_source) == 0)
		errf("\b%s dumped as assembler to %s", what, opath);
#else
	errf("Can't disassemble %s - don't know assembler for this machine",
								    what);
#endif /* !KNOW_ASM */

	free(what);
}

const char *
hex(n)
int n;
{
	static char buf[1 + 2 + 8 + 1];	/* - 0x ffffffff NUL */
#if ARCH_SUN3
	char *cptr;

	cptr = buf;
	if (n < 0) {
		n = -n;
		*cptr++ = '-';
	}
	(void) sprintf(cptr, (n < 10) ? "%d" : "0x%x", n);
#else
	sprintf(buf, "0x%x", n);
#endif
	return buf;
}

const char *
addr_to_func_and_offset(addr, allow_offset)
taddr_t addr;
int allow_offset;
{
	static char buf[50], namebuf[30];
	func_t *f;

	if ((f = addr_to_func(addr)) != NULL) {
		const char *name;

		name = f->fu_name;
		if (*name == '[' && (Debug_flags & DBFLAG_DBXASM) != 0) {
			name = strcpy(namebuf, name + 1);
			namebuf[strlen(name) - 1] = '\0';
		}

		if (f->fu_addr == addr)
			(void) strcpy(buf, name);
		else if (allow_offset && (Debug_flags & DBFLAG_DBXASM) != 0)
			(void) sprintf(buf, "%s+%s", name,
						hex((int)addr - (int)f->fu_addr));
		else
			(void) strcpy(buf, hex((int)addr));
	}
	else
		(void) strcpy(buf, hex((int)addr));
	return buf;
}
