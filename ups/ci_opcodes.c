/* ci_opcodes.c - routines to write and disassemble interpreter code */

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


/* @(#)ci_opcodes.c	1.22 16 Sep 1994 (UKC) */
char ups_ci_opcodes_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <mtrprog/so.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "ci_parse.h"
#include "ci_util.h"
#include "xc_opcodes.h"
#include "xc_machine.h"
#include "cx_link.h"
#include "ci_compile.h"
#include "ci_opcodes.h"

typedef enum { OS_ZERO, OS_BYTE, OS_WORD, OS_LONG, OS_OTHER } operand_size_t;

typedef struct {
	const char *oi_name;
	int oi_stack_effect;
	operand_size_t oi_os;
} opinfo_t;

static void show_generic PROTO((FILE *fp, linkinfo_t *li,
				       operand_size_t os, textword_t *pc));
int disassemble_text PROTO((FILE *fp, linkinfo_t *li, textword_t *text,
			       size_t entry_point, size_t textsize));
static int show_reloc PROTO((FILE *fp, linkinfo_t *li, textword_t *pc,
								long val));
static int dump_linkinfo PROTO((FILE *fp, linkinfo_t *li));
static void dump_linknames PROTO((FILE *fp, const char *what,
					linkname_t *list, int *p_count));
static linkname_t *index_to_linkname PROTO((linkinfo_t *li, int var_index));
static void show_call PROTO((FILE *fp, linkinfo_t *li, int func_index,
					int nargs, int typecode, int optype));
static void dump_ext_relocs PROTO((FILE *fp, const char *what,
							ext_reloc_t *er));
static void dump_data PROTO((FILE *fp, const char *what,
			     const unsigned char *data, size_t count));
static void dump_static_relocs PROTO((FILE *fp, const char *what,
							reloc_pos_t *rplist));
static void show_opname PROTO((const char *opname, FILE *fp));
static void show_special PROTO((FILE *fp, linkinfo_t *li,
				       textword_t *text, textword_t *pc,
				       opcode_t opcode));
static textword_t *get_pc_after_switch_on_chain PROTO((textword_t *pc,
						       int switchval));
static int get_pc_after_call PROTO((machine_t *ma, codefile_t *fcf,
				    int func_index, int operand_size,
				    codefile_t **p_cf, textword_t **p_pc));
static lnum_entry_t *addr_to_le PROTO((symfunc_t *sf, taddr_t addr));
static int print_source_info PROTO(( FILE *fp, linkinfo_t *li, taddr_t addr,
						symfunc_t **p_sf, so_t **p_so));

static opinfo_t Optab[NUM_OPCODES] = {
#define WANT_OPINFO
#include "xc_opcodes.h"
};

void
ci_code_byte(tx, byte)
text_t *tx;
long byte;
{
	if ((textword_t)byte != byte)
		ci_panic("byte out of range in cb");
	
	if (tx->tx_pc >= tx->tx_pclim) {
		do {
			tx->tx_pclim *= 2;
		} while (tx->tx_pc >= tx->tx_pclim);
		tx->tx_text = (textword_t *)e_realloc((char *)tx->tx_text,
				  (size_t)(tx->tx_pclim * sizeof(textword_t)));
	}
	tx->tx_text[tx->tx_pc++] = byte;
}

void
ci_code_word(tx, word)
text_t *tx;
long word;
{
	ci_code_byte(tx, word & 0xff);
	ci_code_byte(tx, (word >> 8) & 0xff);
}

void
ci_code_long(tx, l)
text_t *tx;
long l;
{
	ci_code_word(tx, l & 0xffff);
	ci_code_word(tx, (l >> 16) & 0xffff);
}

void
ci_code_opcode(tx, opcode)
text_t *tx;
opcode_t opcode;
{
	int tmpspace;

	if (tx->tx_flags & CI_CP_CHECKSP) {
		switch (opcode) {

		/*  Avoid infinite recursion.
		 */
		case OC_CHECK_SP_B:
		case OC_CHECK_SP_W:
		case OC_CHECK_SP_L:
			break;
		
		/*  We put a trap at location zero, and we don't want the
		 *  trap preceded by a checksp.  In general, we want
		 *  a trap at a given location to really result in a trap
		 *  opcode at that location.
		 */
		case OC_TRAP:
			break;
		
		/*  We use nops as a filler for the text segment to get
		 *  it to an aligned size - we don't want extra opcodes
		 *  as well.
		 */
		case OC_NOP:
			break;

		/*  We don't want a check here, because the frame pointer
		 *  is not yet set up - the link instruction is just about
		 *  to to that.
		 */
		case OC_LINK_B:
		case OC_LINK_W:
		case OC_LINK_L:
			break;
		
		default:
			ci_code_generic_opcode(tx, OC_CHECK_SP_B, tx->tx_sp);
		}
	}

	switch (opcode) {
	case OC_CALL_B:
	case OC_CALL_W:
	case OC_CALL_L:
		tmpspace = sizeof(stackword_t);
		break;
	default:
		tmpspace = 0;
		break;
	}

	tx->tx_sp += Optab[(int)opcode].oi_stack_effect;

	if (tx->tx_sp + tmpspace > tx->tx_max_sp)
		tx->tx_max_sp = tx->tx_sp + tmpspace;

	ci_code_byte(tx, (int)opcode);
}

static void
show_generic(fp, li, os, pc)
FILE *fp;
linkinfo_t *li;
operand_size_t os;
textword_t *pc;
{
	long val;
	int typec;

	switch (os) {
	case OS_BYTE:
		val = *pc;
		typec = 'b';
		break;
	case OS_WORD:
		val = GETWORD(pc);
		typec = 'w';
		break;
	case OS_LONG:
		val = GETLONG(pc);
		typec = 'l';
		break;
	default:
		ci_panic("opcode botch in sg");
		val = 0;	/* to satisfy gcc */
		typec = 0;	/* to satisfy gcc */
	}
	
	if (typec != 'l' || show_reloc(fp, li, pc, val) != 0)
		fprintf(fp, "%ld.%c", val, typec);
}

static int
show_reloc(fp, li, pc, val)
FILE *fp;
linkinfo_t *li;
textword_t *pc;
long val;
{
	taddr_t addr;
	reloc_pos_t *rp;
	ext_reloc_t *er;
	bool found_one;

	if (li == NULL)
		return -1;

	addr = pc - li->li_text;
	found_one = FALSE;

	for (rp = li->li_static_rps; rp != NULL; rp = rp->rp_next) {
		if (rp->rp_location == addr) {
			if (found_one)
				panic("dup reloc in sr");
			found_one = TRUE;

			fprintf(fp, "data+%ld", val);
		}
	}

	for (er = li->li_ext_relocs; er != NULL; er = er->er_next) {
		if (er->er_location == addr) {
			linkname_t *ln;

			if (found_one)
				panic("dup reloc in sr");
			found_one = TRUE;

			ln = index_to_linkname(li, er->er_index);

			if (val == 0) {
				fprintf(fp, "&%s [%ld]",
					ln->ln_name, ln->ln_addr);
			}
			else {
				fprintf(fp, "&%s+%ld [%ld]",
						ln->ln_name, val,
						ln->ln_addr + val);
			}
		}
	}

	return found_one ? 0 : -1;
}

static linkname_t *
index_to_linkname(li, var_index)
linkinfo_t *li;
int var_index;
{
	linkname_t *ln;
	int count;

	count = 0;

	for (ln = li->li_undef_vars; ln != NULL; ln = ln->ln_next)
		if (count++ == var_index)
			return ln;

	for (ln = li->li_vars; ln != NULL; ln = ln->ln_next)
		if (count++ == var_index)
			return ln;
	
	panic("index botch in il");
	return NULL;	/* to satisfy gcc */
}

int
ci_disassemble_machine(parse_id, ma, fp)
parse_id_t parse_id;
machine_t *ma;
FILE *fp;
{
	panic("ci_disassemble_machine NYI");
	return -1;	/* to satisfy gcc */
}

int
ci_disassemble_linkinfo(li, fp, path)
linkinfo_t *li;
FILE *fp;
const char *path;
{
	int res;

	res = dump_linkinfo(fp, li);

	if (res == 0) {
		res = disassemble_text(fp, li, li->li_text, li->li_text_size,
				       (size_t)-1);
	}
	
	return res;
}

static int
dump_linkinfo(fp, li)
FILE *fp;
linkinfo_t *li;
{
	libfunc_t *lf;
	int i, vcount, fcount;

	fprintf(fp, "path:%s bss=%ld extbss=%ld srcpath=%s\n",
		li->li_path,
		(long)li->li_bss_size, (long)li->li_extbss_size,
		li->li_srcpath);
		    
	fprintf(fp, "extdata:%ld text:%ld data:%ld\n\n",
		(long)li->li_extdata_size, (long)li->li_text_size,
		(long)li->li_data_size);
	
	fprintf(fp, "%12s %5s %5s %s\n", "Undef funcs:",
						"index", "addr", "name");
	for (lf = li->li_undef_funcs; lf != NULL; lf = lf->lf_next) {
		fprintf(fp, "%12s %5d %5ld %s\n", " ",
				lf->lf_index, lf->lf_addr, lf->lf_name);
	}
	fputc('\n', fp);

	vcount = fcount = 0;
	dump_linknames(fp, "Undef vars:", li->li_undef_vars, &vcount);
	dump_linknames(fp, "Funcs:", li->li_funcs, &fcount);
	dump_linknames(fp, "Vars:", li->li_vars, &vcount);

	dump_static_relocs(fp, "Static", li->li_static_rps);
	dump_static_relocs(fp, "Static function ", li->li_static_funcrps);

	dump_ext_relocs(fp, "Ext", li->li_ext_relocs);
	dump_ext_relocs(fp, "Extdata", li->li_extdata_relocs);
	dump_static_relocs(fp, "Extdata function", li->li_extdata_funcrelocs);

	fputs("Funcaddrs:\n", fp);
	for (i = 0; i < li->li_nfuncaddrs; ++i)
		fprintf(fp, "%4d: addr=%ld\n", i, li->li_funcaddrs[i]);
	fputc('\n', fp);

	fputs("Symfuncs\n", fp);
	for (i = 0; i < li->li_nsymfuncs; ++i) {
		symfunc_t *sf;
		lnum_entry_t *le, *lim;

		sf = &li->li_symfuncs[i];

		fprintf(fp, "%4d: %6ld %s\n", i, sf->sf_addr, sf->sf_name);

		lim = sf->sf_letab + sf->sf_letab_size;
		fprintf(fp, "\t%4s  %5s %6s\n", "", "lnum", "addr");
		for (le = sf->sf_letab; le < lim; ++le) {
			fprintf(fp, "\t%4d: %5d %6ld\n", le - sf->sf_letab,
						  le->le_lnum, le->le_addr);
		}
	}
	fputc('\n', fp);

	fputs("Symvars\n", fp);
	for (i = 0; i < li->li_nsymvars; ++i)
		fprintf(fp, "%4d: %s\n", i, li->li_symvars[i]);
	fputc('\n', fp);

	dump_data(fp, "Static", (const unsigned char *)li->li_data,
							li->li_data_size);
	dump_data(fp, "External", (const unsigned char *)li->li_extdata,
							li->li_extdata_size);
	
	return ferror(fp) ? -1 : 0;
}

static void
dump_data(fp, what, data, count)
FILE *fp;
const char *what;
const unsigned char *data;
size_t count;
{
#define NPERLINE	8
	int lnum, nlines;
	const unsigned char *cp, *lim;

	nlines = (count + NPERLINE - 1) / NPERLINE;

	lnum = 0;
	cp = data;
	lim = data + count;

	fprintf(fp, "%s data:\n", what);
	for (lnum = 0; lnum < nlines; ++lnum) {
		int i;
		unsigned char buf[NPERLINE];

		fprintf(fp, "  %06x   ", cp - data);
		
		for (i = 0; i < NPERLINE; ++i) {
			if (cp < lim) {
				buf[i] = isprint(*cp) ? *cp : '.';
				fprintf(fp, "%02x ", *cp++);
			}
			else {
				fputs("   ", fp);
				buf[i] = ' ';
			}

			if (i == 3)
				fputc(' ', fp);
		}
		
		fprintf(fp, "  %.*s\n", NPERLINE, buf);
	}

	fputc('\n', fp);
}

static void
dump_static_relocs(fp, what, rplist)
FILE *fp;
const char *what;
reloc_pos_t *rplist;
{
	reloc_pos_t *rp;
	int i;

	fprintf(fp, "%s relocs:\n", what);

	for (i = 0, rp = rplist; rp != NULL; rp = rp->rp_next, ++i)
		fprintf(fp, "%4d: loc=%ld\n", i, rp->rp_location);

	fputc('\n', fp);
}

static void
dump_ext_relocs(fp, what, er)
FILE *fp;
const char *what;
ext_reloc_t *er;
{
	int i;

	fprintf(fp, "%s relocs:\n", what);

	for (i = 0; er != NULL; ++i, er = er->er_next) {
		fprintf(fp, "%4d: %5ld [%d]\n",
					i, er->er_location, er->er_index);
	}

	fputc('\n', fp);
}

static void
dump_linknames(fp, what, list, p_count)
FILE *fp;
const char *what;
linkname_t *list;
int *p_count;
{
	linkname_t *ln;

	fprintf(fp, "%10s %5s %5s %s\n", what, "index", "addr", "name");
	
	for (ln = list; ln != NULL; ln = ln->ln_next) {
		fprintf(fp, "%10s %5d %5ld %s\n", " ",
					(*p_count)++, ln->ln_addr, ln->ln_name);
	}
	fputc('\n', fp);
}

static lnum_entry_t *
addr_to_le(sf, addr)
symfunc_t *sf;
taddr_t addr;
{
	lnum_entry_t *le, *lim;

	le = sf->sf_letab;
	lim = le + sf->sf_letab_size;

	for (; le < lim; ++le) {
		if (le->le_addr == addr)
			return le;
	}

	return NULL;
}

static int
print_source_info(fp, li, addr, p_sf, p_so)
FILE *fp;
linkinfo_t *li;
taddr_t addr;
symfunc_t **p_sf;
so_t **p_so;
{
	symfunc_t *sf;
	lnum_entry_t *le;

	sf = *p_sf;

	le = (sf != NULL) ? addr_to_le(sf, addr) : NULL;
		
	if (le == NULL) {
		int i;

		for (i = 0; le == NULL; ++i) {
			if (i == li->li_nsymfuncs)
				return 0;
			sf = &li->li_symfuncs[i];
			le = addr_to_le(sf, addr);
		}

		*p_sf = sf;

		if (*p_so == NULL) {
			*p_so = so_open_file(li->li_srcpath, 
						   (so_line_callback_t)NULL);
	   		if (*p_so == NULL) {
				failmesg("Can't open", "source file",
					 li->li_srcpath);
				return -1;
			}
		}

		fprintf(fp, "\n\n%s:\n", sf->sf_name);
	}

	if (le->le_lnum < so_get_nlines(*p_so))
		fprintf(fp, "\n#%s\n", so_getline(*p_so, le->le_lnum - 1));
	else
		fprintf(fp, "\n# [%s,%d]\n", li->li_srcpath, le->le_lnum - 1);

	return 0;
}

static void
show_opname(opname, fp)
const char *opname;
FILE *fp;
{
	const char *s, *lim;

	s = opname + sizeof("OC_") - 1;
	lim = s + strlen(s);

	/*  Don't print a _[BWL] suffix.
	 */
	if (lim - s > 2 && lim[-2] == '_' && strchr("BWL", lim[-1]) != NULL)
		lim -= 2;

	for (; s < lim; ++s)
		fputc(tolower(*s), fp);
}

int
disassemble_text(fp, li, text, textsize, entry_point)
FILE *fp;
linkinfo_t *li;
textword_t *text;
size_t textsize, entry_point;
{
	textword_t *pc, *pclim;
	symfunc_t *sf;
	so_t *so;
	bool source_ok;
#define JUMPDEST(pc, text)		((pc + (short)GETWORD(pc)) - text)

	pc = text; 
	sf = NULL;
	so = NULL;
	source_ok = TRUE;
	pclim = text + textsize;

	while (pc < pclim && !ferror(fp)) {
		taddr_t addr;
		opcode_t opcode;
		opinfo_t *oi;

		addr = (taddr_t)(pc - text);

		if (addr == entry_point)
			fputs("\n\n<entry_point>\n", fp);

		if (source_ok && print_source_info(fp, li, addr, &sf, &so) != 0)
			source_ok = FALSE;

		opcode = (opcode_t)*pc++;
		oi = &Optab[(int)opcode];

		fprintf(fp, "%8ld: ", addr);
		show_opname(oi->oi_name, fp);

		switch (oi->oi_os) {
		case OS_ZERO:
			break;
		case OS_BYTE:
		case OS_WORD:
		case OS_LONG:
			fputc(' ', fp);
			show_generic(fp, li, oi->oi_os, pc);
			break;
		case OS_OTHER:
			fputc(' ', fp);
			show_special(fp, li, text, pc, opcode);
			break;
		default:
			panic("bad os in dt");
		}

		fputc('\n', fp);

		pc += ci_operand_size(opcode, pc);
	}

	if (ferror(fp) || fflush(fp) == EOF)
		return -1;
	
	return 0;
}

static textword_t *
get_pc_after_switch_on_chain(pc, switchval)
textword_t *pc;
int switchval;
{
	int i, ncase;
	opcode_t opcode;

	opcode = (opcode_t)pc[-1];

	ncase = GETWORD(pc);
	pc += 2;
	switchval -= GETLONG(pc);
	pc += 4;

	for (i = 0; i < ncase; ++i) {
		int val;

		switch (opcode) {
		case OC_SWITCH_ON_CHAIN_B:
			val = *pc++;
			break;
		case OC_SWITCH_ON_CHAIN_W:
			val = GETWORD(pc);
			pc += 2;
			break;
		case OC_SWITCH_ON_CHAIN_L:
			val = GETLONG(pc);
			pc += 4;
			break;
		default:
			ci_panic("bad opcode");
			val = 0;	/* to satisfy gcc */
		}
		if (val == switchval) {
			/* Dibyendu : this was missing */
			pc += (short)GETWORD(pc);
			break;
		}
		pc += 2;
	}
	if (i == ncase)
		pc += (short)GETWORD(pc);

	return pc;
}

int
ci_get_next_pc_and_cf(ma, p_cf, p_pc)
machine_t *ma;
codefile_t **p_cf;
textword_t **p_pc;
{
	codefile_t *cf, *fcf;
	textword_t *pc;
	opcode_t opcode;
	unsigned addr;
	int res, ncase, switchval;

	cf = ma->ma_codefile;
	pc = ma->ma_pc;
	opcode = (opcode_t)*pc++;
	res = 0;

	switch (opcode) {
	case OC_SWITCH_ON_TABLE:
		ncase = GETWORD(pc);
		pc += 2;
		switchval = *ma->ma_sp - GETLONG(pc);
		if ((unsigned)switchval >= ncase)
			switchval = ncase;
		pc += 4 + switchval * 2;
		pc += (short)GETWORD(pc);
		break;

	case OC_SWITCH_ON_CHAIN_B:
	case OC_SWITCH_ON_CHAIN_W:
	case OC_SWITCH_ON_CHAIN_L:
		pc = get_pc_after_switch_on_chain(pc, (int)(ma->ma_sp[0]));
		break;

	case OC_PUSH_FLOAT_CONST:
		pc += FLOAT_NBYTES;
		break;
	case OC_PUSH_DOUBLE_CONST:
		pc += DOUBLE_NBYTES;
		break;

	case OC_CALL_B:
		res = get_pc_after_call(ma, cf, *pc, sizeof(char),
								&cf, &pc);
		break;
	case OC_CALL_W:
		res = get_pc_after_call(ma, cf, (int)GETWORD(pc), sizeof(short),
								&cf, &pc);
		break;
	case OC_CALL_L:
		res = get_pc_after_call(ma, cf, (int)GETLONG(pc), sizeof(long),
								&cf, &pc);
		break;
	case OC_CALL_INDIRECT:
		addr = *ma->ma_sp;
		fcf = &ma->ma_cftab[addr >> 16];

		if (!fcf->cf_loaded && XLOAD(ma, fcf) != 0) {
			res = -1;
			break;
		}

		res = get_pc_after_call(ma, fcf, (short)addr, 0, &cf, &pc);
		break;

	case OC_RET_FLOAT:
	case OC_RET_DOUBLE:
	case OC_RET_WORD:
	case OC_RET:
		pc = ((textword_t **)ma->ma_fp)[1];
		cf = ((codefile_t **)ma->ma_fp)[2];
		break;

	case OC_SETJMP:
		pc = (textword_t *)ma->ma_sp[0];
		cf = (codefile_t *)ma->ma_sp[1];
		break;
	case OC_LONGJMP:
		cf = ((machine_state_t *)ma->ma_sp[2])->ms_cf;
		pc = ((machine_state_t *)ma->ma_sp[2])->ms_pc;
		break;

	case OC_JUMP:
		pc += (short)GETWORD(pc);
		break;
	case OC_JUMP_IF_ZERO:
		pc += (*ma->ma_sp == 0) ? (short)GETWORD(pc) : 2;
		break;
	case OC_JUMP_IF_NON_ZERO:
		pc += (*ma->ma_sp != 0) ? (short)GETWORD(pc) : 2;
		break;

	default:
		pc += ci_operand_size(opcode, pc);
		break;
	}

	*p_cf = cf;
	*p_pc = pc;

	return res;
}

static int
get_pc_after_call(ma, fcf, func_index, operand_size, p_cf, p_pc)
machine_t *ma;
codefile_t *fcf;
int func_index, operand_size;
codefile_t **p_cf;
textword_t **p_pc;
{
	textword_t *pc;

	pc = *p_pc;

	/* Dibyendu : following looks incorrect - see xc_machine.c */
	/* if (func_index >= 0) */
	{
		extref_t *er;

		if (func_index > 0) {
			*p_cf = fcf;
			*p_pc = fcf->cf_text_and_data + fcf->cf_funcaddrs[func_index];
			return 0;
		}

		/* Dibyendu : following looks incorrect - see xc_machine.c */
		/* er = fcf->cf_funcrefs[-func_index]; */
		er = fcf->cf_funcrefs[func_index];

		if (er->er_file != NULL) {
			*p_cf = er->er_file;

			if (!(*p_cf)->cf_loaded && XLOAD(ma, *p_cf) != 0)
				return -1;

			*p_pc = (*p_cf)->cf_text_and_data + er->er_addr;
			return 0;
		}
	}

	*p_pc += operand_size + 1 + WANT_TYPE_PUSHED;
	return 0;
}

int
ci_operand_size(opcode, text)
opcode_t opcode;
textword_t *text;
{
	if ((int)opcode < 0 || (int)opcode >= NUM_OPCODES)
		panic("bad opcode in gos");

	switch (Optab[(int)opcode].oi_os) {
	case OS_ZERO:
		return 0;
	case OS_BYTE:
		return sizeof(char);
	case OS_WORD:
		return sizeof(short);
	case OS_LONG:
		return sizeof(long);
	case OS_OTHER:
		switch (opcode) {
		case OC_CALL_B:
			return sizeof(char) + 1 + WANT_TYPE_PUSHED;
		case OC_CALL_W:
			return sizeof(short) + 1 + WANT_TYPE_PUSHED;
		case OC_CALL_L:
			return sizeof(long) + 1 + WANT_TYPE_PUSHED;
#if WANT_TYPE_PUSHED
		case OC_CALL_INDIRECT:
			return sizeof(char) + WANT_TYPE_PUSHED;
#endif
#if WANT_OLD_LINK
		case OC_LINK_B:
			return sizeof(char) + 1;
		case OC_LINK_W:
			return sizeof(short) + 1;
		case OC_LINK_L:
			return sizeof(long) + 1;
#else
		/* Fix LINK ARG (CHAR -> LONG) : Dibyendu */
		case OC_LINK_B:
			return sizeof(char) + 4;
		case OC_LINK_W:
			return sizeof(short) + 4;
		case OC_LINK_L:
			return sizeof(long) + 4;
#endif
		case OC_PUSH_FLOAT_CONST:
			return FLOAT_NBYTES;
		case OC_PUSH_DOUBLE_CONST:
			return DOUBLE_NBYTES;
		case OC_INSERT_SIGNED_BITFIELD:
		case OC_EXTRACT_SIGNED_BITFIELD:
		case OC_INSERT_UNSIGNED_BITFIELD:
		case OC_EXTRACT_UNSIGNED_BITFIELD:
			return 2;	/* offset and count */
		case OC_JUMP:
		case OC_JUMP_IF_ZERO:
		case OC_JUMP_IF_NON_ZERO:
			return sizeof(short);
		case OC_SWITCH_ON_TABLE:
			return 2 + 4 + GETWORD(text) * 2 + 2;
			break;
		case OC_SWITCH_ON_CHAIN_B:
			return 2 + 4 + GETWORD(text) * (1 + 2) + 2;
		case OC_SWITCH_ON_CHAIN_W:
			return 2 + 4 + GETWORD(text) * (2 + 2) + 2;
		case OC_SWITCH_ON_CHAIN_L:
			return 2 + 4 + GETWORD(text) * (4 + 2) + 2;
			break;

		default:
			panic("gos called for OS_OTHER opcode");
			return -1;	/* to satisfy gcc */
		}
		break;
	default:
		panic("bad os in dt");
		return -1;	/* to satisfy gcc */
	}
}

static void
show_special(fp, li, text, pc, opcode)
FILE *fp;
linkinfo_t *li;
textword_t *text, *pc;
opcode_t opcode;
{
	int minval, ncase, i;
	int ival;
	float fval;
	double dval;

	switch(opcode) {
	case OC_SWITCH_ON_TABLE:
		ncase = GETWORD(pc);
		pc += 2;
		minval = GETLONG(pc);
		pc += 4;
		fprintf(fp, "#%d,+%d\n", ncase, minval);
		for (i = 0; i < ncase; ++i) {
			fprintf(fp, "\t\tcase %d: jump %d\n",
					i + minval, JUMPDEST(pc, text));
			pc += 2;
		}
		fprintf(fp, "\t\tdefault: jump %d", JUMPDEST(pc, text));
		break;

	case OC_SWITCH_ON_CHAIN_B:
	case OC_SWITCH_ON_CHAIN_W:
	case OC_SWITCH_ON_CHAIN_L:
		opcode = (opcode_t)pc[-1];
		ncase = GETWORD(pc);
		pc += 2;
		minval = GETLONG(pc);
		pc += 4;
		fprintf(fp, "#%d,+%d\n", ncase, minval);
		for (i = 0; i < ncase; ++i) {
			int val;

			switch (opcode) {
			case OC_SWITCH_ON_CHAIN_B:
				val = *pc++;
				break;
			case OC_SWITCH_ON_CHAIN_W:
				val = GETWORD(pc);
				pc += 2;
				break;
			case OC_SWITCH_ON_CHAIN_L:
				val = GETLONG(pc);
				pc += 4;
				break;
			default:
				panic("bad opcode");
				val = 0;	/* to satisfy gcc */
			}
			fprintf(fp, "\t\tcase %d: jump %d\n",
					val + minval, JUMPDEST(pc, text));
			pc += 2;
		}
		fprintf(fp, "\t\tdefault: jump %d", JUMPDEST(pc, text));
		break;

	case OC_PUSH_FLOAT_CONST:
		memcpy((char *)&fval, pc, FLOAT_NBYTES);
		fprintf(fp, "%f", fval);
		break;
	case OC_PUSH_DOUBLE_CONST:
		memcpy((char *)&dval, pc, DOUBLE_NBYTES);
		fprintf(fp, "%f", dval);
		break;

	/* Fix WANT_TYPE_PUSHED : Dibyendu */
#if WANT_TYPE_PUSHED
	case OC_CALL_B:
		show_call(fp, li, *pc, pc[1], pc[2], 'b');
		break;
	case OC_CALL_W:
		show_call(fp, li, (int)GETWORD(pc), pc[2], pc[3], 'w');
		break;
	case OC_CALL_L:
		show_call(fp, li, (int)GETLONG(pc), pc[4], pc[5], 'l');
		break;
#else
	case OC_CALL_B:
		show_call(fp, li, *pc, pc[1], 0, 'b');
		break;
	case OC_CALL_W:
		show_call(fp, li, (int)GETWORD(pc), pc[2], 0, 'w');
		break;
	case OC_CALL_L:
		show_call(fp, li, (int)GETLONG(pc), pc[4], 0, 'l');
		break;
#endif
#if WANT_TYPE_PUSHED
	case OC_CALL_INDIRECT:
		fprintf(fp, "#%d %s", *pc, ci_typecode_to_name(pc[1]));
		break;
#endif

#if WANT_OLD_LINK
	case OC_LINK_B:
		fprintf(fp, "%d.b,%d", *pc, pc[1]);
		break;
	case OC_LINK_W:
		fprintf(fp, "%d.w,%d", GETWORD(pc), pc[2]);
		break;
	case OC_LINK_L:
		fprintf(fp, "%d.l,%d", GETLONG(pc), pc[4]);
		break;
#else
	case OC_LINK_B:
		ival = *pc++;
		fprintf(fp, "%d.b,%d", ival, GETLONG(pc));
		break;
	case OC_LINK_W:
		ival = GETWORD(pc);
		pc += 2;
		fprintf(fp, "%d.w,%d", ival, GETLONG(pc));
		break;
	case OC_LINK_L:
		ival = GETLONG(pc);
		pc += 4;
		fprintf(fp, "%d.l,%d", ival, GETLONG(pc));
		break;
#endif

	case OC_UNRESOLVED_JUMP:
	case OC_JUMP:
	case OC_JUMP_IF_ZERO:
	case OC_JUMP_IF_NON_ZERO:
		fprintf(fp, "%d", JUMPDEST(pc, text));
		break;

	case OC_INSERT_SIGNED_BITFIELD:
	case OC_INSERT_UNSIGNED_BITFIELD:
	case OC_EXTRACT_SIGNED_BITFIELD:
	case OC_EXTRACT_UNSIGNED_BITFIELD:
		fprintf(fp, " #%d,#%d", pc[0], pc[1]);
		break;

	default:
		fprintf(fp, "<unknown opcode %d>", pc[-1]);
		break;
	}
}

static void
show_call(fp, li, func_index, nargs, typecode, optype)
FILE *fp;
linkinfo_t *li;
int func_index, nargs, typecode, optype;
{
	libfunc_t *lf;
#if WANT_TYPE_PUSHED
	char *tn = (char *)ci_typecode_to_name((typecode_t)typecode);
#else
	char *tn = "";
#endif

	if (li == NULL || (li->li_nsymfuncs == 0 && func_index > 0)) {
		fprintf(fp, "%d.%c #%d %s", func_index, optype, nargs, tn);
		return;
	}

	if (func_index > 0) {
		fprintf(fp, "%s #%d %s",
			li->li_symfuncs[func_index - 1].sf_name, nargs, tn);
    		return;
	}

	for (lf = li->li_undef_funcs; lf != NULL; lf = lf->lf_next) {
		if (lf->lf_index == func_index) {
			fprintf(fp, "%s #%d %s", lf->lf_name, nargs, tn);
			return;
		}
	}

	panic("unknown func botch");
}

void
ci_code_generic_opcode(tx, byte_opcode, arg)
text_t *tx;
opcode_t byte_opcode;
stackword_t arg;
{
	if (arg <= MAX_BYTE) {
		ci_code_opcode(tx, BYTE_FORM(byte_opcode));
		ci_code_byte(tx, (long)arg);
	}
	else if (arg < MAX_WORD) {
		ci_code_opcode(tx, SHORT_FORM(byte_opcode));
		ci_code_word(tx, (long)arg);
	}
	else {
		ci_code_opcode(tx, LONG_FORM(byte_opcode));
		ci_code_long(tx, (long)arg);
	}
}
