/* ao_text.c - low level code for opening and reading text files */

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


/* @(#)ao_text.c	1.6 20 Jun 1995 (UKC) */
char ups_ao_text_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#ifdef AO_TARGET

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef ARCH_VAX
#include <sys/types.h>
#include <machine/frame.h>
#endif
#if HAVE_SYS_REG_H
#include <sys/reg.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <mtrprog/hash.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "st.h"
#include "as.h"
#include "ao_dwarf.h"
#include "ao_syms.h"
#include "ao_asm.h"
#include "ao_symread.h"
#include "ao_execinfo.h"
#include "ao_text.h"
#include "ao_symload.h"
#include "ao_symscan.h"
#include "ao_symcb.h"
#include "ao_shlib.h"
#include "ao_elfread.h"
#include "ao_elfsym.h"
#include "ao_dwftext.h"
#include "ao_dwfsyms.h"
#include "ao_aout.h"
#include "trun.h"

#ifdef OS_LINUX
#include <sys/ptrace.h>
#endif


static int ao_get_min_bpt_addr PROTO((func_t *f, taddr_t *p_addr));
static void ao_close_symtab_data PROTO((symtab_t *st));
static const char *ao_disassemble_instruction PROTO((func_t *f, taddr_t addr,
						     const char *text,
						     const char **p_buf));
static bool get_startup_code PROTO((func_t *f, ao_preamble_t *pr));
static jump_t *ao_get_jumps PROTO((func_t *f, taddr_t addr, const char *text,
				   size_t len, bool want_calls));
static taddr_t ao_passthru_addr PROTO((symtab_t *st, taddr_t addr));

static int ao_read_text PROTO((symtab_t *st,
			       taddr_t addr, char *buf, size_t nbytes));
static const char *wherestr PROTO((off_t pos, taddr_t addr, const char *path));
static int read_text_from_symtab PROTO((symtab_t *st, taddr_t addr, char *buf, 
                                        size_t nbytes));

bool
open_textfile(fd, path, p_data_addr)
int fd;
const char *path;
taddr_t *p_data_addr;
{
#ifdef AO_ELF
	*p_data_addr = 0;
	return check_elf_exec_header(fd, path);
#else
	return aout_check_exec_header(fd, path, p_data_addr);
#endif
}

static taddr_t
ao_passthru_addr(st, addr)
symtab_t *st;
taddr_t addr;
{
	return addr;
}

void
do_ao_postscan_stuff(st, flist, ei, p_mainfunc, mainfunc_name,
		     p_initfunc, initfunc_name)
symtab_t *st;
func_t *flist;
Execinfo *ei;
func_t **p_mainfunc;
const char *mainfunc_name;
func_t **p_initfunc;
const char *initfunc_name;
{
#ifndef ST_TE
	func_t *f;
	int flist_len;

	flist_len = 0;
	for (f = flist; f != NULL; f = f->fu_next)
		++flist_len;
	
	make_funcinfo(st->st_apool, flist, flist_len,
		      ei->text_mem_addr,
		      ei->text_mem_addr + ei->text_size,
		      ei->text_addr_delta,
		      &st->st_funclist, &st->st_functab);
	
	finish_common_blocks(st);
#endif
	
	if (p_mainfunc != NULL && mainfunc_name != NULL) {
		for (f = st->st_funclist; f != NULL; f = f->fu_next) {
			if (strcmp(f->fu_demangled_name, mainfunc_name) == 0) {
				*p_mainfunc = f;
				break;
			}
		}
	}

	if (p_initfunc != NULL && initfunc_name != NULL) {
		for (f = st->st_funclist; f != NULL; f = f->fu_next) {
			if (strcmp(f->fu_demangled_name, initfunc_name) == 0) {
				*p_initfunc = f;
				break;
			}
		}
	}
}

bool
scan_ao_symtab(textpath, fd, ei, dw_dbg, base_address,
	       st_is, p_symtab, p_flist, p_mainfunc_name)
const char *textpath;
int fd;
Execinfo *ei;
Dwarf_Debug dw_dbg;
taddr_t base_address;
symtab_type_t st_is;
symtab_t **p_symtab;
func_t **p_flist;
const char **p_mainfunc_name;
{
	static sym_ops_t stabs_ops = {
		ao_read_text,
		ao_get_min_bpt_addr,
		get_addr_lim_via_functab,

		ao_passthru_addr,
		ao_passthru_addr,

		ao_fil_may_have_matching_globals,
		ao_close_symtab_data,

		ao_cblocks_match,
		ao_set_cblock_data,
		ao_free_cblock_data,
		ao_cblock_has_var,
		ao_get_cblock_vars,

		ao_ensure_fu,
		ao_get_fi,
		ao_get_fu_lnos,
		ao_get_fu_blocks,
		ao_get_fi_vars,
		ao_get_fi_macros,

		ao_disassemble_instruction,
		ao_get_jumps,
	};
#if WANT_DWARF
	static sym_ops_t dwarf_ops = {
		ao_read_text,			/* original OK */
		ao_get_min_bpt_addr,		/* original OK */
		get_addr_lim_via_functab,

		ao_passthru_addr,
		ao_passthru_addr,

		ao_fil_may_have_matching_globals,
		ao_close_symtab_data,		/* original OK */

		dw_cblocks_match,
		dw_set_cblock_data,
		dw_free_cblock_data,
		dw_cblock_has_var,
		dw_get_cblock_vars,

		dw_ensure_fu,
		dw_get_fi,
		dw_get_fu_lnos,
		dw_get_fu_blocks,
		dw_get_fi_vars,
		dw_get_fi_macros,

		ao_disassemble_instruction,
		ao_get_jumps,
	};
#endif
	alloc_pool_t *ap;
	ao_stdata_t *ast;
	symtab_t *st;
	func_t *flist;

	ap = alloc_create_pool();

	ast = (ao_stdata_t *)alloc(ap, sizeof(ao_stdata_t));
	ast->st_base_address = base_address;
	ast->st_addr_to_fpos_offset = ei->addr_to_fpos_offset;
	ast->st_textfd = fd;
	ast->st_dynamic = ei->dynamic;
#ifndef AO_ELF
	ast->st_next = NULL;
#endif

#if WANT_DWARF
	ast->st_type_names = NULL;
	ast->st_dw_dbg = 0;
	ast->st_dw_scanned = FALSE;
	if (st_is == ST_DWARF) {
	    ast->st_dw_dbg = dw_dbg;
	    st = make_symtab(ap, textpath, (fil_t *)NULL, (func_t *)NULL,
			     &dwarf_ops, (char *)ast);
	}
	else
#endif
	    st = make_symtab(ap, textpath, (fil_t *)NULL, (func_t *)NULL,
			     &stabs_ops, (char *)ast);

#ifdef ST_TE
	if (skim_te_symtab(st, fd, file_syms_offset, ei->addr_to_fpos_offset,
			   ei->base_address,
			   shlib_load_addr + ei->mem_text_offset + ei->text_size,
			   get_rootblock()) != 0) {
		alloc_free_pool(ap);
		return FALSE;
	}
	flist = NULL;
#else
#if WANT_DWARF
	if (st_is == ST_DWARF) {
		ast->st_text_symio = NULL;
		flist = NULL;
	} else
#endif
		if (ei->nsyms == 0) {
		ast->st_text_symio = NULL;
		flist = NULL;
	} else {
		ast->st_text_symio = make_symio(ap, textpath, dup(fd),
						ei->file_syms_offset,
						ei->nsyms,
						ei->file_symstrings_offset);
		scan_symtab(st, textpath, (stf_t *)NULL, &flist,
			    p_mainfunc_name);
	}
#endif

	*p_symtab = st;
	*p_flist = flist;
	return TRUE;
}

static const char *
wherestr(offset, addr, path)
off_t offset;
taddr_t addr;
const char *path;
{
	static char *last = NULL;

	if (last != NULL)
		free(last);

	last = strf("offset %ld (virtual address %p) in %s",
		    (long)offset, (char *)addr, path);
	return last;
}

bool
text_dread(xp, addr, buf, nbytes)
target_t *xp;
taddr_t addr;
char *buf;
size_t nbytes;
{
	symtab_t *st;
	
	st = NULL;
	
	while (xp_next_symtab(xp, st, TRUE, &st)) {
		if (read_text_from_symtab(st, addr, buf, nbytes))
			return TRUE;
	}

	return FALSE;
}

/*  Like ao_read_text, but don't give error messages.  This is because
 *  we want to try all the symbol tables in turn.
 *
 *  TODO: check address ranges.  As things stand, we could wrongly
 *  return success for an address that is out of the range
 *  of addresses covered by this symbol table, but within
 *  the file data outside the address range.
 */
static int
read_text_from_symtab(st, addr, buf, nbytes)
symtab_t *st;
taddr_t addr;
char *buf;
size_t nbytes;
{
	ao_stdata_t *ast;
	off_t offset;
	long nread;
	int fd;

	ast = AO_STDATA(st);
/* RGA use fd of symio instead of symtab, because getsym(), findsym(), etc
   call get_free_symio_slot() which reuses fd numbers, so symtab_t fd 
   can be invalid */

#ifdef ST_TE
	fd = ast->st_textfd;
#else
	if (ast->st_text_symio)
	  fd = get_symio_fd(ast->st_text_symio);
	else
	  fd = ast->st_textfd;
#endif
	offset = addr - ast->st_addr_to_fpos_offset;
	
	return (offset >= 0 &&
		lseek(fd, offset, 0) == offset &&
		(nread = read(fd, buf, nbytes)) == nbytes);
}

/*  Read n bytes of text starting at virtual address addr from the
 *  a.out file.
 */
static int
ao_read_text(st, addr, buf, nbytes)
symtab_t *st;
taddr_t addr;
char *buf;
size_t nbytes;
{
	ao_stdata_t *ast;
	off_t offset;
	long nread;
	int fd;

	ast = AO_STDATA(st);

/* RGA use fd of symio instead of symtab, because getsym(), findsym(), etc
   call get_free_symio_slot() which reuses fd numbers, so symtab_t fd 
   can be invalid */

#ifdef ST_TE
	fd = ast->st_textfd;
#else
	if (ast->st_text_symio)
	  fd = get_symio_fd(ast->st_text_symio);
	else
	  fd = ast->st_textfd;
#endif

	offset = addr - ast->st_addr_to_fpos_offset;
	
	/*  TODO: check address ranges.  As things stand, we could wrongly
	 *        return success for an address that is out of the range
	 *        of addresses covered by this symbol table, but within
	 *        the file data outside the address range.
	 */
	if (offset < 0)
		return -1;
	
	if (lseek(fd, offset, 0) != offset) {
		errf("Can't lseek to %s: %s",
		     wherestr(offset, addr, st->st_path), get_errno_str());
		return -1;
	}
	
	if ((nread = read(fd, buf, nbytes)) < 0) {
		errf("Error reading %ld bytes at %s: %s",
		     (long)nbytes, wherestr(offset, addr, st->st_path),
		     get_errno_str());
		return -1;
	}

	if (nread < nbytes) {
		errf("Unexpected EOF at %s: expected %ld byte%s, got %ld",
		     wherestr(offset, addr, st->st_path),
		     (long)nbytes, (nbytes == 1) ? "" : "s", nread);
		return -1;
	}

	return 0;
}

bool
get_preamble(f, p_pr)
func_t *f;
ao_preamble_t **p_pr;
{
#ifndef ARCH_MIPS
	if (f->fu_predata == NULL) {
		ao_preamble_t prbuf;

		if (!get_startup_code(f, &prbuf))
			return FALSE;
		
		f->fu_predata = alloc(f->fu_symtab->st_apool, sizeof(prbuf));
		memcpy(f->fu_predata, (char *)&prbuf, sizeof(prbuf));
	}
#endif

	if (p_pr != NULL)
		*p_pr = AO_PREDATA(f);
	
	return TRUE;
}

static void
ao_close_symtab_data(st)
symtab_t *st;
{
	close(AO_STDATA(st)->st_textfd);	/* RGA */
	close_symio(AO_STDATA(st)->st_text_symio);
}

static int
ao_get_min_bpt_addr(f, p_addr)
func_t *f;
taddr_t *p_addr;
{
	ao_preamble_t *pr;
	
	if (!get_preamble(f, &pr))
		return -1;
	
	*p_addr = f->fu_addr + pr->pr_bpt_offset;
	return 0;
}

static const char *
ao_disassemble_instruction(f, addr, text, p_buf)
func_t *f;
taddr_t addr;
const char *text, **p_buf;
{
#ifdef KNOW_ASM
	return disassemble_one_instruction(addr, text, p_buf);
#else
	return 0;
#endif
}

static jump_t *
ao_get_jumps(f, addr, text, len, want_calls)
func_t *f;
taddr_t addr;
const char *text;
size_t len;
bool want_calls;
{
#ifdef KNOW_ASM
	return get_asm_jumps(addr, text, len, want_calls);
#else
	return 0;
#endif
}

#ifdef ARCH_386
/*  Some 80386 opcodes that get_startup_code() must recognise at the start
 *  of functions.
 *
 *  The 80386 appears to read its opcodes as a byte stream (like a VAX)
 *  but we want to treat the first four opcodes of a function as a single
 *  integer.  Hence the SW_MOVL_ESP_EBP and SW_SUBL_IMM_ISP definitions.
 */
#define PUSHL_EBP	0x55
#define PUSHQ_RBP	0x55
#define JMP_FAR		0xe9	/* + 4 byte operand */
#define JMP_SHORT	0xeb

#define SUBL_IMM_ESP		0xec81	/* four byte operand */
#define SUBL_IMM_ESP_SHORT	0xec83	/* one byte operand */

/*  No, I don't have the faintest idea why opcodes should be different
 *  across operating systems using the same chip ...
 */
#ifdef OS_SUNOS
#define MOVL_ESP_EBP	0xec8b
#define MOVQ_RSP_RBP	0xec8b48
#endif

#ifdef OS_LINUX
#define MOVL_ESP_EBP	0xe589
#define MOVQ_RSP_RBP	0xe58948
#endif

#ifdef OS_BSD44
#define MOVL_ESP_EBP	0xe589
#define MOVQ_RSP_RBP	0xe58948
#endif

/*  80386 version of get_startup_code().
 */
static bool
get_startup_code(f, pr)
func_t *f;
ao_preamble_t *pr;
{
	/*  Worst case is
	 *
	 *	pushl %ebp		1 byte
	 *	movl %esp, %ebp		2 bytes
	 *	subl xxx, %esp		5 bytes
	 *	8 * pushl <reg>		8 bytes
	 *
	 * or, in 64 bit mode:
	 * 
	 *	pushq %rbp		1 byte
	 *	movq %rsp, %rbp		3 bytes
	 *	subq xxx, %rsp		6 bytes
	 *	8 * pushq <reg>		16 bytes
	 *
	 *  I don't think we'd ever see 8 register pushes, but
	 *  who cares, PTRACE_READ is reasonably fast.
	 */
	unsigned char textbuf[1 + 3 + 6 + 16], *text;
	taddr_t addr;
	int count;

	st_read_text(f->fu_symtab, f->fu_addr,
					     (char *)textbuf, sizeof textbuf);
	text = textbuf;

	addr = 0;
	pr->pr_bpt_offset = 0;

	/*  The 80386 C compiler without -O is not very clever - it
	 *  jumps to the end of the function and back because at the
	 *  end of the function it knows how much stack space to grab
	 *  and which registers to save.  Thus we check for a jump
	 *  and follow it.
	 */
	if (*text == JMP_SHORT) {
		taddr_t target = f->fu_addr + 2 + text[1];
		if (addr_to_func(target) == f) {
			addr = target;
		pr->pr_bpt_offset = 2;
	}
	}
	else if (*text == JMP_FAR) {
		taddr_t target = f->fu_addr + 5 + *(unsigned *)&text[1];
		if (addr_to_func(target) == f) {
			addr = target;
		pr->pr_bpt_offset = 5;
	}
	}

	if (addr != 0) {
		st_read_text(f->fu_symtab, addr,
					(char *)textbuf, sizeof(textbuf));
		text = textbuf;
	}

	if ((*(unsigned *)text & 0xffffff) == (PUSHL_EBP | (MOVL_ESP_EBP << 8))) {
		text += 3;
	}
	else if ((*(unsigned *)text & 0xffffffff) == (PUSHQ_RBP | (MOVQ_RSP_RBP << 8))) {
		text += 4;
	}
	else
		f->fu_flags |= FU_NO_FP;

	if (*(unsigned short *)text == SUBL_IMM_ESP) {
		pr->pr_rsave_offset = -*(int *)&text[2];
		text += 6;
	}
	else if (*(unsigned short *)text == SUBL_IMM_ESP_SHORT) {
		pr->pr_rsave_offset = -*(unsigned char *)&text[2];
		text += 3;
	}
	else
		pr->pr_rsave_offset = 0;
	
	count = 0;
	pr->pr_rsave_mask = 0;
	while ((text[0] & 0x7) == 0x50 ||
	       (text[0] == 0x41 && (text[1] & 0x7) == 0x50)) {
#ifdef OS_SUNOS
		static int regs[] = { EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI };
#endif
#if defined(ARCH_LINUX386_64)
		static int regs[] = { RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI, R8, R9, R10, R11, R12, R13, R14, R15 };
#elif defined(ARCH_LINUX386)
		static int regs[] = { EAX, ECX, EDX, EBX, UESP, EBP, ESI, EDI };
#endif
		int reg = 0;

		if (text >= textbuf + sizeof(textbuf))
			panic("pushl botch in gsf");

		if (text[0] == 0x41) {
			reg += 8;
			text++;
		}

#if defined(OS_SUNOS) || defined(OS_LINUX)
		reg += regs[text[0] & 7];
#endif
#ifdef OS_BSD44
		reg += text[0] & 7;
#endif

		pr->pr_rsave_mask |= 1 << reg;
		pr->pr_regtab[count++] = reg;

		text++;
	}
		
	if (pr->pr_bpt_offset == 0)
		pr->pr_bpt_offset = text - textbuf;

	return TRUE;
}
#endif /* ARCH_386 */

#ifdef ARCH_CLIPPER
/*  Some Clipper opcodes needed by get_startup_code().
 */
#define PUSHW_R14_R15		(0x1400 | (15 << 4) | 14)
#define PUSHW_RX_R15		(0x1400 | (15 << 4))
#define MOVW_R15_R14		(0x8400 | (15 << 4) | 14)
#define NOOP_0			0x0000
#define SUBI_R15_LONG_FMT	(0xa330 | 15)
#define SUBI_R15_SHORT_FMT	(0xa3b0 | 15)
#define SUBQ_X_R15		(0xa200 | 15)
#define SAVEWX			0xb400
#define SAVEWX_WORD1		0x0000
#define SAVEDX			0xb420
#define SAVEDX_WORD1		0x0000

/*  Clipper version of get_startup_code.
 */
static bool
get_startup_code(f, pr)
func_t *f;
ao_preamble_t *pr;
{
	unsigned short textbuf[20];	/* an overestimate */
	unsigned short *text;

	/*  The following stuff is *very* compiler dependent and liable
	 *  to change with compiler releases.
	 */

	/*  Get the first few instructions
	 *
	 *  We read from the text file as the process may not have started yet.
	 */
	st_read_text(f->fu_symtab,
				f->fu_addr, (char *)textbuf, sizeof(textbuf));

	text = textbuf;
	
	pr->pr_frame_size = 0;

	/*  Functions compiled with -g have the instructions
	 *
	 *	pushw r14, r15		(r14 is the fp, r15 the sp)
	 *	noop 0
	 *	movw r15, r14
	 *
	 *  at the start to build the frame.
	 */
	if (*text == PUSHW_R14_R15 && text[1] == NOOP_0 && text[2] == MOVW_R15_R14)
		text += 3;
	else {
		/*  No frame pointer.
		 */
		f->fu_flags |= FU_NO_FP;
	}

	if (*text == SUBI_R15_LONG_FMT) {
		int operand;

		operand = text[1] | (text[2] << 16);
		if (operand < 0)
			panic("negative (long fmt) frame size");
		pr->pr_frame_size += operand;
		text += 3;
	}

	if (*text == SUBI_R15_SHORT_FMT) {
		if ((short)text[1] < 0)
			panic("negative frame size");
		pr->pr_frame_size += text[1];
		text += 2;
	}
	else if ((*text & 0xff0f) == SUBQ_X_R15)
		pr->pr_frame_size += (*text++ >> 4) & 0xf;

	pr->pr_rsave_offset = -pr->pr_frame_size;
	pr->pr_rsave_mask = 0;

	/*  The compiler sometimes emits a some pushw rx, r15 instructions
	 *  rather than a single register savewn, so check for these.
	 */
	while ((*text & 0xfff0) == PUSHW_RX_R15) {
		pr->pr_rsave_mask |= (1 << (*text++ & 0xf));
		pr->pr_frame_size += 4;
		pr->pr_rsave_offset -= 4;
	}

	if ((*text & 0xfff0) == SAVEWX && text[1] == SAVEWX_WORD1) {
		int i, basereg, regbytes;

		basereg = *text & 0xf;
		text += 2;
		for (i = basereg; i < 15; ++i)
			pr->pr_rsave_mask |= (1 << i);

		regbytes = (15 - basereg) * 4;

		pr->pr_frame_size += regbytes;

		/*  Registers are saved below the frame pointer, so the
		 *  offset is negative.
		 */
		pr->pr_rsave_offset -= regbytes;
	}

	pr->pr_fpreg_rsave_mask = 0;
	pr->pr_fpreg_rsave_offset = 0;

	if ((*text & 0xfff8) == SAVEDX && text[1] == SAVEDX_WORD1) {
		int i, basereg, regbytes;

		basereg = *text & 7;
		text += 2;
		for (i = basereg; i < 8; ++i)
			pr->pr_fpreg_rsave_mask |= 1 << i;
		regbytes = (8 - basereg) * 8;

		pr->pr_frame_size += regbytes;
		pr->pr_fpreg_rsave_offset = pr->pr_rsave_offset - regbytes;
	}

	pr->pr_bpt_offset = (text - textbuf) * sizeof(textbuf[0]);

	if (pr->pr_bpt_offset > sizeof(textbuf))
		panic("startup botch in gsc");
}
#endif

#ifdef ARCH_SUN4
#define SAVE_MASK	(((unsigned)3 << 30) | (077 << 19))
#define SAVE		(((unsigned)2 << 30) | (074 << 19))

/*  SPARC version of get_startup_code().
 */
static bool
get_startup_code(f, pr)
func_t *f;
ao_preamble_t *pr;
{
#define NWORDS	3
	unsigned words[NWORDS];
	int i;

	if (st_read_text(f->fu_symtab, f->fu_addr,
					(char *)words, sizeof(words)) != 0)
		return FALSE;

	for (i = 0; i < NWORDS; ++i)
		if ((words[i] & SAVE_MASK) == SAVE)
			break;
	if (i == NWORDS) {
		f->fu_flags |= FU_NO_FP;
		pr->pr_bpt_offset = 0;
	}
	else {
		pr->pr_bpt_offset = (i + 1) * 4;
	}

	pr->pr_rsave_mask = 0;
	pr->pr_rsave_offset = 0;

	return TRUE;
}
#endif /* ARCH_CLIPPER || ARCH_SUN4 */

#ifdef ARCH_VAX
/*  VAX version of get_startup_code().
 */
static bool
get_startup_code(f, pr)
func_t *f;
ao_preamble_t *pr;
{
	unsigned short rsavemask;

	st_read_text(f->fu_symtab,
			f->fu_addr, (char *)&rsavemask, sizeof(rsavemask));
	pr->pr_rsave_mask = rsavemask;
	pr->pr_rsave_offset = sizeof(struct frame);
	pr->pr_bpt_offset = (f->fu_language == LANG_FORTRAN) ? 0 : 2;
}
#endif /* ARCH_VAX */

#ifdef ARCH_SUN3
/*  A few 68xxx opcodes that get_startup_code() must recognise at the start
 *  of functions.
 */
#define LINK_A6		0x4e56
#define ADDL_IMM_A7	0xdffc
#define MOVEML		0x48d7
#define PEA_ABSW	0x4878
#define TRAP_0		0x4e40
#define FMOVEM_A6	0xf236

/*  Sun 3 version of get_startup_code().
 */
static bool
get_startup_code(f, pr)
func_t *f;
ao_preamble_t *pr;
{
	unsigned short textbuf[12];	/* need at most 12 shorts */
	unsigned short *text;

	/*  The following stuff is *very* compiler dependent and liable
	 *  to change with compiler releases.
	 */

	/*  Get the first few instructions
	 *
	 *  We read from the text file as the process may not have started yet.
	 */
	st_read_text(f->fu_symtab,
				f->fu_addr, (char *)textbuf, sizeof(textbuf));

	text = textbuf;

	/*  System call stubs are a special case - they save no registers
	 *  and breakpoints should be placed at the function address.
	 */
	if (*text == PEA_ABSW) {
		pr->pr_rsave_mask = 0;
		pr->pr_bpt_offset = 0;
		f->fu_flags |= FU_NO_FP;
		return;
	}

	/*  Get the link command and offset.
	 *  The cast to short is there as we want sign extension in this
	 *  case, as the displacement operand of link is a 16 bit signed
	 *  number.
	 */
	if (*text == LINK_A6) {
		pr->pr_rsave_offset = (short)text[1];
		text += 2;
	}
	else
		f->fu_flags |= FU_NO_FP;

	/*  Get the add which moves the stack pointer over the locals
	 *  and registers.
	 *
	 *  This add is not present for optimised code - the addition is
	 *  done in the link instruction.
	 */
	if (*text == ADDL_IMM_A7) {
		if (pr->pr_rsave_offset != 0) {
			/*  Non zero link offset *and* addition - surely not.
			 */
			panic("bad startup code in get_startup_code");
		}
		pr->pr_rsave_offset = (text[1] << 16) | text[2];
		text += 3;
	}

	/*  Get the register save command.
	 *  Not always present for optimised code.
	 */
	if (*text == MOVEML) {
		pr->pr_rsave_mask = text[1];
		text += 2;
	}
	else {
		/*  No registers saved in this function.
		 */
		pr->pr_rsave_mask = 0;
	}

	/*  Check for a floating point register save instruction.
	 */
	if (*text == FMOVEM_A6) {
		pr->pr_fpreg_rsave_mask = text[1] & 0xff;
		pr->pr_fpreg_rsave_offset = (text[3] << 16) | text[4];
		text += 5;
	}
	else {
		/*  No floating point registers saved in this function.
		 */
		pr->pr_fpreg_rsave_mask = 0;
	}

	pr->pr_bpt_offset = (text - textbuf) * sizeof(textbuf[0]);
}
#endif /* ARCH_SUN3 */

#endif /* AO_TARGET */
