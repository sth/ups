/* ci_compile.c - routines to compile a parse tree into interpreter code */

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


/* @(#)ci_compile.c	1.23 09 Sep 1994 (UKC) */
char ups_ci_compile_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

#include <local/ukcprog.h>
#include <mtrprog/alloc.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "ci_parse.h"
#include "ci_types.h"
#include "xc_opcodes.h"
#include "xc_machine.h"
#include "cx_link.h"
#include "ci_compile.h"
#include "ci_compile_expr.h"
#include "ci_util.h"

labeldesc_t make_label PROTO((text_t *tx));
static branch_label_t *labeldesc_to_label PROTO((text_t *tx,
						 labeldesc_t labeldesc));
static void add_label_reference PROTO((text_t *tx, labeldesc_t labeldesc,
							    taddr_t location));
void set_label_addr PROTO((text_t *tx, labeldesc_t labeldesc, taddr_t addr));
static void set_var_addrs PROTO((text_t *tx, var_t *varlist,
				 taddr_t *p_stack_addr, bool doing_params));
static taddr_t assign_local_addrs PROTO((text_t *tx,
					 block_t *blocks, taddr_t base_addr));
static void set_local_addrs PROTO((text_t *tx, func_t *f));
static void adjust_local_addrs PROTO((block_t *par, long delta));
static void compile_func PROTO((text_t *tx, func_t *f));
static void compile_statement PROTO((text_t *tx, statement_t *st));

static void compile_labeled_statement PROTO((text_t *tx, labeled_stm_t *ls));
static void compile_case_labeled_statement PROTO((text_t *tx,
						  case_labeled_stm_t *cs));
static void compile_expression_statement PROTO((text_t *tx, expr_t *expr));
static void compile_compound_statement PROTO((text_t *tx, compound_stm_t *co));
static void compile_if_statement PROTO((text_t *tx, if_stm_t *is));
static void compile_switch_statement PROTO((text_t *tx, switch_stm_t *ss));
static void compile_while_or_do_statement PROTO((text_t *tx,
						 while_stm_t *ws,
						 bool is_while_statement));
static void compile_for_statement PROTO((text_t *tx, for_stm_t *fs));
static void compile_goto_statement PROTO((text_t *tx, goto_label_t *gl));
static void compile_continue_or_break_statement PROTO((text_t *tx,
					             label_type_t label_type));
static void compile_return_statement PROTO((text_t *tx, expr_t *expr));
static bool need_final_return PROTO((statement_t *body_st));
static void push_label PROTO((text_t *tx, label_type_t label_type));
static void pop_label PROTO((text_t *tx, label_type_t label_type));
static void set_parameter_addrs PROTO((func_t *f));
static void add_lno_entry PROTO((text_t *tx, taddr_t addr, lexinfo_t *lx));
static void report_compile_error PROTO((char *arg, errtype_t errtype,
					lexinfo_t *lx, const char *mesg));

static labelref_t *new_labelref PROTO((void));
static void free_labelref_list PROTO((labelref_t *labelref));
ALLOC_NEW_FREELIST(static,labelref_t,labelref,lr_next)

static label_list_t *new_labellist PROTO((void));
static void free_labellist PROTO((label_list_t *labellist));
ALLOC_NEW_FREE(static,label_list_t,labellist,ll_next)
static text_t *compile PROTO((parse_res_t *pr,
			      ci_report_error_func_t report_error,
			      ci_checkarg_proc_t checkarg_proc,
			      ci_regno_to_addr_proc_t regno_to_addr_proc,
			      unsigned long flags));
static void set_var_extflags PROTO((text_t *tx, initialiser_t *in));
static void add_stringspace PROTO((initialiser_t *in, taddr_t *p_addr));
static void make_relocs PROTO((text_t *tx, linkinfo_t *li,
			       char *data, char *extdata,
			       taddr_t str_addr, taddr_t extstr_addr));

static void make_extvars PROTO((alloc_pool_t *ap, linkinfo_t *li, vlist_t *vlist, size_t extbss_size));
static void make_funcinfo PROTO((alloc_pool_t *ap, linkinfo_t *li, int nfuncs, func_t *funclist));
static var_t *lookup_var_def PROTO((var_t *vlist, const char *name));
static void add_to_vlist PROTO((text_t *tx, var_t *v));
static void finish_vlist PROTO((text_t *tx));
static void write_text_long PROTO((textword_t *text, unsigned long val));
static void make_symfuncs PROTO((linkinfo_t *li, func_t *funcs, int nfuncs));
static void make_symvars PROTO((linkinfo_t *li, var_t *vars));
static int lecmp PROTO((const void *cle1, const void *cle2));
static libfunc_t *reverse_libfunc_list PROTO((libfunc_t *lf));
static linkname_t *reverse_linkname_list PROTO((linkname_t *ln));

labeldesc_t
make_label(tx)
text_t *tx;
{
	labeldesc_t labeldesc;
	branch_label_t *lb;

	for (labeldesc = 0; labeldesc < tx->tx_labellim; ++labeldesc) {
		if (tx->tx_labels[labeldesc] == NULL)
			break;
	}
	if (labeldesc == tx->tx_labellim) {
		int i;

		tx->tx_labellim += 50;
		tx->tx_labels = (branch_label_t **)e_realloc((char *)tx->tx_labels,
						tx->tx_labellim * sizeof(branch_label_t *));
		for (i = labeldesc; i < tx->tx_labellim; ++i)
			tx->tx_labels[i] = NULL;
	}

	lb = (branch_label_t *)e_malloc(sizeof(branch_label_t));
	lb->lb_addr = 0;
	lb->lb_refs = NULL;
	tx->tx_labels[labeldesc] = lb;
	return labeldesc;
}

static branch_label_t *
labeldesc_to_label(tx, labeldesc)
text_t *tx;
labeldesc_t labeldesc;
{
	if (labeldesc > MAX_LABELDESC || labeldesc >= tx->tx_labellim ||
						tx->tx_labels[labeldesc] == NULL)
		ci_panic("labeldesc botch in ar");
	return tx->tx_labels[labeldesc];
}

static void
add_label_reference(tx, labeldesc, location)
text_t *tx;
labeldesc_t labeldesc;
taddr_t location;
{
	branch_label_t *lb;
	labelref_t *lr;
	
	lb = labeldesc_to_label(tx, labeldesc);
	lr = new_labelref();
	lr->lr_location = location;
	lr->lr_next = lb->lb_refs;
	lb->lb_refs = lr;
}

void
set_label_addr(tx, labeldesc, addr)
text_t *tx;
labeldesc_t labeldesc;
taddr_t addr;
{
	branch_label_t *lb;

	lb = labeldesc_to_label(tx, labeldesc);
	if (lb->lb_addr != 0)
		ci_panic("multiple calls of sla");
	lb->lb_addr = addr;
}

void
ci_resolve_and_free_label(tx, labeldesc)
text_t *tx;
labeldesc_t labeldesc;
{
	branch_label_t *lb;
	labelref_t *lr;
	taddr_t savepc;

	lb = labeldesc_to_label(tx, labeldesc);
	if (lb->lb_addr == 0)
		lb->lb_addr = tx->tx_pc;
	
	savepc = tx->tx_pc;
	for (lr = lb->lb_refs; lr != NULL; lr = lr->lr_next) {
		long offset;

		tx->tx_pc = lr->lr_location;
		offset = lb->lb_addr - lr->lr_location;
		if (offset != (short)offset)
			ci_panic("jump out of range");
		ci_code_word(tx, (long)(unsigned short)offset);
	}
	tx->tx_pc = savepc;
	
	free_labelref_list(lb->lb_refs);
	free((char *)lb);
	tx->tx_labels[labeldesc] = NULL;
}

labeldesc_t
ci_code_jump_to_label(tx, opcode, labeldesc)
text_t *tx;
opcode_t opcode;
labeldesc_t labeldesc;
{
	if (labeldesc == NO_LABELDESC)
		labeldesc = make_label(tx);

	ci_code_opcode(tx, opcode);
	add_label_reference(tx, labeldesc, tx->tx_pc);
	ci_code_word(tx, (long)labeldesc);

	return labeldesc;
}

static void
push_label(tx, label_type)
text_t *tx;
label_type_t label_type;
{
	label_list_t *ll;

	ll = new_labellist();
	ll->ll_labeldesc = make_label(tx);
	ll->ll_next = tx->tx_label_lists[(int)label_type];
	tx->tx_label_lists[(int)label_type] = ll;
}

static void
pop_label(tx, label_type)
text_t *tx;
label_type_t label_type;
{
	label_list_t *ll;

	ll = tx->tx_label_lists[(int)label_type];
	tx->tx_label_lists[(int)label_type] = ll->ll_next;
	ci_resolve_and_free_label(tx, ll->ll_labeldesc);
	free_labellist(ll);
}

static var_t *
lookup_var_def(vlist, name)
var_t *vlist;
const char *name;
{
	var_t *v;

	for (v = vlist; v != NULL; v = v->va_next)
		if (v->va_class == CL_EXT && strcmp(v->va_name, name) == 0)
			return v;

	return NULL;
}

void
ci_add_func_reloc(tx, loctype, location)
text_t *tx;
loctype_t loctype;
taddr_t location;
{
	funcreloc_t *fr;

	fr = (funcreloc_t *)alloc(tx->tx_apool, sizeof(funcreloc_t));
	fr->fr_loctype = loctype;
	fr->fr_location = location;
	fr->fr_next = tx->tx_func_relocs;
	tx->tx_func_relocs = fr;
}

void
ci_add_var_reloc(tx, loctype, location, v, offset)
text_t *tx;
loctype_t loctype;
taddr_t location;
var_t *v;
long offset;
{
	reloc_t *rl;

	if (v->va_class == CL_DECL) {
		var_t *def_v;

		def_v = lookup_var_def(tx->tx_varlist, v->va_name);

		if (def_v != NULL) {
			v = def_v;
		}
		else {
			v->va_class = CL_EXTREF;
			v->va_flags |= VA_EXT_RELOC;
			add_to_vlist(tx, v);
		}
	}

	rl = (reloc_t *)alloc(tx->tx_apool, sizeof(reloc_t));
	rl->rl_loctype = loctype;
	rl->rl_location = location;
	rl->rl_var = v;
	rl->rl_offset = offset;
	rl->rl_next = tx->tx_var_relocs;
	tx->tx_var_relocs = rl;
}

void
ci_add_string_reloc(tx, loctype, location, sc, offset)
text_t *tx;
loctype_t loctype;
taddr_t location;
string_const_t *sc;
long offset;
{
	reloc_t *rl;

	rl = (reloc_t *)alloc(tx->tx_apool, sizeof(reloc_t));
	rl->rl_loctype = loctype;
	rl->rl_location = location;
	rl->rl_sc = sc;
	rl->rl_offset = offset;
	rl->rl_next = tx->tx_string_relocs;
	tx->tx_string_relocs = rl;
}

static void
set_var_addrs(tx, varlist, p_stack_addr, doing_params)
text_t *tx;
var_t *varlist;
taddr_t *p_stack_addr;
bool doing_params;
{
	var_t *v;

	for (v = varlist; v != NULL; v = v->va_next) {
		long tsize;

		if (doing_params != (v->va_class == CL_ARG))
			continue;
		
		if (!IS_LOCAL_CLASS(v->va_class)) {
			add_to_vlist(tx, v);
			continue;
		}

		tsize = ci_typesize(v->va_lexinfo, v->va_type);
		v->va_addr = ci_align_addr_for_type(*p_stack_addr, v->va_type);
		*p_stack_addr = v->va_addr + tsize;

		if (doing_params && tsize < sizeof(stackword_t) &&
						  IS_LOCAL_CLASS(v->va_class)) {
			static int endian = 0;
			static bool big_endian;

			if (endian == 0) {
				endian = 42;
				big_endian = *(char *)&endian != 42;
			}

			if (big_endian)
				v->va_addr += sizeof(stackword_t) - tsize; 

			*p_stack_addr += sizeof(stackword_t) - tsize;
		}
	}
}
			
func_t *
ci_addr_to_func(funclist, addr)
func_t *funclist;
taddr_t addr;
{
	func_t *f;

	for (f = funclist; f != NULL; f = f->fu_next)
		if (f->fu_addr == addr)
			return f;
	return NULL;
}

int
ci_funcname_to_index(tx, lx, name)
text_t *tx;
lexinfo_t *lx;
const char *name;
{
	func_t *f;
	libfunc_t *lf;
	int func_index;

	func_index = 1;
	for (f = tx->tx_funclist; f != NULL; f = f->fu_next) {
		if (strcmp(f->fu_name, name) == 0)
			return func_index;
		++func_index;
	}
	
	for (lf = tx->tx_undef_funcs; lf != NULL; lf = lf->lf_next)
		if (strcmp(lf->lf_name, name) == 0)
			return lf->lf_index;
	
	lf = (libfunc_t *)alloc(tx->tx_apool, sizeof(libfunc_t));
	lf->lf_name = name;
	lf->lf_addr = 0;
	lf->lf_index = -tx->tx_num_undef_funcs++;
	lf->lf_lexinfo = *lx;
	lf->lf_next = tx->tx_undef_funcs;
	tx->tx_undef_funcs = lf;
		
	return lf->lf_index;
}

static void
report_compile_error(arg, errtype, lx, mesg)
char *arg;
errtype_t errtype;
lexinfo_t *lx;
const char *mesg;
{
	text_t *tx;

	tx = (text_t *)arg;

	if (tx->tx_report_error_func != NULL &&
	                                (*tx->tx_report_error_func)(lx, mesg)) {
		tx->tx_report_error_func = NULL;
	}

	if (errtype == ET_ERROR)
		tx->tx_had_error = TRUE;
}

static void
set_var_extflags(tx, in)
text_t *tx;
initialiser_t *in;
{
	var_t *v;

	switch (in->in_inittype) {
	case IT_VARADDR:
		v = in->in_addrvar;

		if ((v->va_flags & VA_EXT_RELOC) == 0) {
			v->va_flags |= VA_TODO;

			if ((v->va_flags & VA_ON_VLIST) == 0) {
				if (v->va_class != CL_DECL)
					panic("class botch in sve");
				v->va_class = CL_EXTREF;
				v->va_flags |= VA_EXT_RELOC;
				add_to_vlist(tx, in->in_addrvar);
			}
		}
		break;
	case IT_LIST:
		for (in = in->in_list; in != NULL; in = in->in_next)
			set_var_extflags(tx, in);
		break;
	default:
		break;
	}
}

static void
add_stringspace(in, p_addr)
initialiser_t *in;
taddr_t *p_addr;
{
	switch (in->in_inittype) {
	case IT_STRINGADDR:
		*p_addr += in->in_stringval->sc_size;
		break;
	case IT_LIST:
		for (in = in->in_list; in != NULL; in = in->in_next)
			add_stringspace(in, p_addr);
		break;
	default:
		break;
	}
}

/*  Do what ci_code_long() does.
 */
static void
write_text_long(text, val)
textword_t *text;
unsigned long val;
{
	text[0] = val & 0xff;
	text[1] = (val >> 8) & 0xff;
	text[2] = (val >> 16) & 0xff;
	text[3] = (val >> 24) & 0xff;
}

static void
make_relocs(tx, li, data, extdata, str_addr, extstr_addr)
text_t *tx;
linkinfo_t *li;
char *data, *extdata;
taddr_t str_addr, extstr_addr;
{
	reloc_t *rl;
	funcreloc_t *fr;
	reloc_pos_t *static_rps, *static_funcrps, *extdata_funcrelocs;
	ext_reloc_t *ext_relocs, *extdata_relocs;

	static_rps = static_funcrps = extdata_funcrelocs = NULL;
	ext_relocs = extdata_relocs = NULL;

	for (rl = tx->tx_var_relocs; rl != NULL; rl = rl->rl_next) {
		var_t *v;
		bool is_ext;
		int location;
		unsigned long val;

		v = rl->rl_var;

		is_ext = (v->va_flags & VA_EXT_RELOC) != 0;

		val = rl->rl_offset;		

		if (!is_ext)
			val += v->va_addr;

		location = rl->rl_location;

		switch (rl->rl_loctype) {
		case RT_TEXTLOC:
			write_text_long(li->li_text + location, val);
			break;
		case RT_DATALOC:
			*(long *)(data + location) = val;
			location += li->li_text_size;
			break;
		case RT_EXTDATALOC:
			if (!is_ext)
				panic("extreloc botch in mr");
			*(long *)(extdata + location) = val;
			break;
		default:
			panic("bad loctype in mr");
			break;
		}
		
		if (is_ext) {
			ext_reloc_t *er;

			er = (ext_reloc_t *)alloc(tx->tx_apool,
							sizeof(ext_reloc_t));
			er->er_location = location;
			er->er_index = v->va_addr;

			if (rl->rl_loctype == RT_EXTDATALOC) {
				er->er_next = extdata_relocs;
				extdata_relocs = er;
			}
			else {
				er->er_next = ext_relocs;
				ext_relocs = er;
			}
		}
		else {
			reloc_pos_t *rp;

			rp = (reloc_pos_t *)alloc(tx->tx_apool,
							sizeof(reloc_pos_t));
			rp->rp_location = location; 
			rp->rp_next = static_rps;
			static_rps = rp;
		}
	}

	for (rl = tx->tx_string_relocs; rl != NULL; rl = rl->rl_next) {
		string_const_t *sc;
		int location;
		unsigned long val;

		sc = rl->rl_sc;

		if (rl->rl_loctype == RT_EXTDATALOC) {
			val = extstr_addr + rl->rl_offset;
			memcpy(extdata + extstr_addr, sc->sc_val, sc->sc_size);
			extstr_addr += sc->sc_size;
		}
		else {
			val = str_addr + rl->rl_offset;
			memcpy(data + str_addr, sc->sc_val, sc->sc_size);
			str_addr += sc->sc_size;
		}

		location = rl->rl_location;
			
		switch (rl->rl_loctype) {
		case RT_TEXTLOC:
			write_text_long(li->li_text + location, val);
			break;
		case RT_DATALOC:
			*(long *)(data + location) = val;
			location += li->li_text_size;
			break;
		case RT_EXTDATALOC:
			*(long *)(extdata + location) = val;
			break;
		default:
			panic("bad str loctype in mr");
		}

		if (rl->rl_loctype == RT_EXTDATALOC) {
			ext_reloc_t *er;

			er = (ext_reloc_t *)alloc(tx->tx_apool,
							sizeof(ext_reloc_t));
			er->er_location = location;
			er->er_index = -1;
			er->er_next = extdata_relocs;
			extdata_relocs = er;
		}
		else {
			reloc_pos_t *rp;

			rp = (reloc_pos_t *)alloc(tx->tx_apool,
							sizeof(reloc_pos_t));
			rp->rp_location = location; 
			rp->rp_next = static_rps;
			static_rps = rp;
		}
	}	
	for (fr = tx->tx_func_relocs; fr != NULL; fr = fr->fr_next) {
		reloc_pos_t *rp;

		rp = (reloc_pos_t *)alloc(tx->tx_apool, sizeof(reloc_pos_t));
		rp->rp_location = fr->fr_location; 

		if (fr->fr_loctype == RT_EXTDATALOC) {
			rp->rp_next = extdata_funcrelocs;
			extdata_funcrelocs = rp;
		}
		else {
			if (fr->fr_loctype == RT_DATALOC)
				rp->rp_location += li->li_text_size;

			rp->rp_next = static_funcrps;
			static_funcrps = rp;
		}
	}	

	li->li_extdata_funcrelocs = extdata_funcrelocs;
	li->li_extdata_relocs = extdata_relocs;
	li->li_ext_relocs = ext_relocs;
	li->li_static_rps = static_rps;
	li->li_static_funcrps = static_funcrps;
}

static void
add_to_vlist(tx, v)
text_t *tx;
var_t *v;
{
	vlist_t *vl;

	if (v->va_class == CL_DECL || v->va_type->ty_code == DT_FUNC_RETURNING)
		return;

	vl = (vlist_t *)alloc(tx->tx_apool, sizeof(vlist_t));
	vl->vl_var = v;
	vl->vl_next = tx->tx_vlist;
	tx->tx_vlist = vl;

	if (v->va_flags & VA_ON_VLIST)
		panic("vlist botch");
	v->va_flags |= VA_ON_VLIST;
}

static void
finish_vlist(tx)
text_t *tx;
{
	var_t *v;
	vlist_t *vl;

	for (v = tx->tx_varlist; v != NULL; v = v->va_next)
		add_to_vlist(tx, v);

	for (vl = tx->tx_vlist; vl != NULL; vl = vl->vl_next) {
		switch (vl->vl_var->va_class) {
		case CL_EXTREF:
		case CL_EXT:
		case CL_STAT:
		case CL_LSTAT:
			break;
		default:
			panic("class botch in mv");
		}
	}
}

static linkname_t *
reverse_linkname_list(ln)
linkname_t *ln;
{
	linkname_t *list, *next;

	list = NULL;
	for (; ln != NULL; ln = next) {
		next = ln->ln_next;
		ln->ln_next = list;
		list = ln;
	}

	return list;
}

static libfunc_t *
reverse_libfunc_list(lf)
libfunc_t *lf;
{
	libfunc_t *list, *next;

	list = NULL;
	for (; lf != NULL; lf = next) {
		next = lf->lf_next;
		lf->lf_next = list;
		list = lf;
	}

	return list;
}

static int
lecmp(cle1, cle2)
constvoidptr cle1, cle2;
{
	lnum_entry_t *le1, *le2;

	le1 = (lnum_entry_t *)cle1;
	le2 = (lnum_entry_t *)cle2;

	if (le1->le_lnum == le2->le_lnum)
		return 0;

	return (le1->le_lnum < le2->le_lnum) ? -1 : 1;
}

static void
make_symvars(li, vars)
linkinfo_t *li;
var_t *vars;
{
	int nvars;
	var_t *v;
	const char **names, **np;

	nvars = 0;
	for (v = vars; v != NULL; v = v->va_next) {
		if (v->va_class != CL_EXTREF &&
		    v->va_type->ty_code != DT_FUNC_RETURNING)
			++nvars;
	}

	names = (const char **)alloc(li->li_apool,
						nvars * sizeof(const char *));

	np = names;
	for (v = vars; v != NULL; v = v->va_next) {
		if (v->va_class != CL_EXTREF &&
		    v->va_type->ty_code != DT_FUNC_RETURNING)
			*np++ = v->va_name;
	}

	li->li_symvars = names;
	li->li_nsymvars = nvars;
}

static void
make_symfuncs(li, funcs, nfuncs)
linkinfo_t *li;
func_t *funcs;
int nfuncs;
{
	symfunc_t *sf, *symfuncs;
	func_t *f;
	block_t **blocks;
	ebuf_t *eb;

	symfuncs = (symfunc_t *)alloc(li->li_apool, nfuncs * sizeof(symfunc_t));
	blocks = (block_t **)alloc(li->li_apool, nfuncs * sizeof(block_t));
	eb = ebuf_create(TRUE);

	for (sf = symfuncs, f = funcs; f != NULL; ++sf, f = f->fu_next) {
		lno_t *ln;
		const char *obuf;
		char *buf;
		int int_nbytes;
		size_t nbytes, count;

		sf->sf_name = f->fu_name;
		sf->sf_addr = f->fu_addr;
		blocks[sf - symfuncs] = f->fu__blocks;

		for (ln = f->fu__lnos; ln != NULL; ln = ln->ln_next) {
			lnum_entry_t lebuf;

			lebuf.le_addr = ln->ln_addr;
			lebuf.le_lnum = ln->ln_num;
			ebuf_add(eb, (char *)&lebuf, sizeof(lebuf));
		}

		obuf = ebuf_get(eb, &int_nbytes);
		nbytes = int_nbytes;	/* ebuf_get should use size_t */
		buf = memcpy(alloc(li->li_apool, nbytes), obuf, nbytes);
		count = nbytes / sizeof(lnum_entry_t);

		qsort(buf, count, sizeof(lnum_entry_t), lecmp);

		sf->sf_letab = (lnum_entry_t *)buf;
		sf->sf_letab_size = count;

		ebuf_reset(eb);
	}

	li->li_symfuncs = symfuncs;
	li->li_funcblocks = blocks;
	li->li_nsymfuncs = nfuncs;

	ebuf_free(eb);
}

static void
make_extvars(ap, li, vlist, extbss_size)
alloc_pool_t *ap;
linkinfo_t *li;
vlist_t *vlist;
size_t extbss_size;
{
	linkname_t *var_refs, *var_defs;
	int nrefs, ndefs;
	vlist_t *vl;

	nrefs = ndefs = 0;
	var_refs = var_defs = NULL;

	for (vl = vlist; vl != NULL; vl = vl->vl_next) {
		var_t *v;
		linkname_t *ln;
		class_t class;

		v = vl->vl_var;

		if ((v->va_flags & VA_EXT_RELOC) == 0) {
			if ((v->va_flags & VA_HAS_INITIALISER) == 0)
				v->va_addr += li->li_data_size;
			continue;
		}
		
		class = v->va_class;

		if ((v->va_flags & VA_TODO) != 0)
			panic("vlist botch in me");

		ln = (linkname_t *)alloc(ap, sizeof(linkname_t));

		if (class == CL_EXTREF && v->va_addr == 0) {
			ln->ln_name = v->va_name;
			ln->ln_addr = 0;
			ln->ln_next = var_refs;
			var_refs = ln;
			v->va_addr = nrefs++;
		}
		else if (class == CL_EXT || class == CL_STAT) {
			if ((v->va_flags & VA_HAS_INITIALISER) == 0) {
				v->va_addr -= extbss_size;
				if ((long)v->va_addr >= 0)
					panic("bssaddr botch");
			}
				
			if (class == CL_EXT) {
				ln->ln_name = v->va_name;
			}
			else {
				static int unique_id = 0;
				char *name;

				name = strf("[%s:%d]", v->va_name, unique_id++);
				ln->ln_name = alloc_strdup(ap, name);
				free(name);
			}
				
			ln->ln_addr = v->va_addr;
			ln->ln_next = var_defs;
			var_defs = ln;
			v->va_addr = ndefs++;
			v->va_flags |= VA_TODO;
		}
		else {
			panic("var class botch in me");
		}
	}

	for (vl = vlist; vl != NULL; vl = vl->vl_next) {
		if ((vl->vl_var->va_flags & VA_TODO) != 0) {
			vl->vl_var->va_addr += nrefs;
			vl->vl_var->va_flags &= ~VA_TODO;
		}
	}

	li->li_undef_vars = var_refs;
	li->li_vars = var_defs;
}

static void
make_funcinfo(ap, li, nfuncs, funclist)
alloc_pool_t *ap;
linkinfo_t *li;
int nfuncs;
func_t *funclist;
{
	func_t *f;
	int func_index;

	li->li_funcaddrs = (taddr_t *)alloc(ap, nfuncs * sizeof(taddr_t));
	li->li_nfuncaddrs = nfuncs;

	li->li_funcs = NULL;
	func_index = 1;
	for (f = funclist; f != NULL; f = f->fu_next) {
		linkname_t *ln;

		li->li_funcaddrs[func_index++] = f->fu_addr;

		if ((f->fu_flags & FU_STATIC) != 0)
			continue;

		ln = (linkname_t *)alloc(ap, sizeof(linkname_t));
		ln->ln_name = f->fu_name;
		ln->ln_addr = f->fu_addr;
		ln->ln_next = li->li_funcs;
		li->li_funcs = ln;
	}

	if (func_index != li->li_nfuncaddrs)
		panic("func_index botch in cfl");
}

linkinfo_t *
ci_compile(srcpath, parse_id, report_error, checkarg_proc,
	   regno_to_addr_proc, flags)
const char *srcpath;
parse_id_t parse_id;
ci_report_error_func_t report_error;
ci_checkarg_proc_t checkarg_proc;
ci_regno_to_addr_proc_t regno_to_addr_proc;
unsigned flags;
{
	taddr_t data_addr, extdata_addr, bss_addr, extbss_addr;
	taddr_t str_addr, extstr_addr, aligned_pc;
	char *data, *extdata;
	bool done_one, one_file;
	size_t tsize, max_stack_nwords;
	text_t *tx;
	parse_res_t *pr;
	reloc_t *rl;
	taddr_t *p_addr;
	linkinfo_t *li;
	initlist_t *il;
	var_t *v;
	vlist_t *vl;

	pr = (parse_res_t *)parse_id;

	tx = compile(pr, report_error, checkarg_proc,
		     regno_to_addr_proc, flags);
	
	if (tx == NULL)
		return NULL;
	
	aligned_pc = ci_align_addr_for_type(tx->tx_pc, (type_t *)NULL);
	while (tx->tx_pc < aligned_pc)
		ci_code_opcode(tx, OC_NOP);

	finish_vlist(tx);

	one_file = (tx->tx_flags & CI_CP_ONEFILE) != 0;

	for (vl = tx->tx_vlist; vl != NULL; vl = vl->vl_next) {
		v = vl->vl_var;

		/*  If we are linking a single .c file then we don't need to
		 *  worry about external data - we treat it all as internal.
		 */
		if (one_file && v->va_class == CL_EXT)
			v->va_class = CL_STAT;
		
		if ((v->va_class == CL_EXT || v->va_class == CL_EXTREF) &&
			             v->va_type->ty_code != DT_FUNC_RETURNING)
			v->va_flags |= VA_TODO;
	}

	/*  Any variable whose address is an initialiser for an external
	 *  variable must also get external relocation, because it can
	 *  be referenced via the initialised variable before the text
	 *  file is loaded.  This effect can propagate through a series
	 *  of static variables (consider a statically initialised
	 *  linked list with all but the head element static).
	 */
	do {
		done_one = FALSE;
		for (il = tx->tx_initlist; il != NULL; il = il->il_next) {
			if ((il->il_var->va_flags & VA_TODO) != 0) {
				set_var_extflags(tx, (initialiser_t *)
						         il->il_initialiser_id);
				il->il_var->va_flags |= VA_EXT_RELOC;
				il->il_var->va_flags &= ~VA_TODO;
				done_one = TRUE;
			}
		}
	} while (done_one);

	data_addr = bss_addr = extdata_addr = extbss_addr = 0;

	for (vl = tx->tx_vlist; vl != NULL; vl = vl->vl_next) {
		v = vl->vl_var;

		if ((v->va_flags & VA_TODO) != 0) {
			v->va_flags |= VA_EXT_RELOC;
			v->va_flags &= ~VA_TODO;
		}

		/*  We don't set addresses on extern variables.
		 */
		if (v->va_class == CL_EXTREF)
			continue;
		 
		if (v->va_flags & VA_HAS_INITIALISER) {
			p_addr = (v->va_flags & VA_EXT_RELOC) ? &extdata_addr
							      : &data_addr;
		}
		else {
			p_addr = (v->va_flags & VA_EXT_RELOC) ? &extbss_addr
							      : &bss_addr;
		}

		tsize = ci_typesize(v->va_lexinfo, v->va_type);
		v->va_addr = ci_align_addr_for_type(*p_addr, v->va_type);
		*p_addr = v->va_addr + tsize;
	}

	extstr_addr = extdata_addr;
	str_addr = data_addr;

	/*  Add space for string initialisers.
	 */
	for (il = tx->tx_initlist; il != NULL; il = il->il_next) {
		p_addr = (il->il_var->va_flags & VA_EXT_RELOC) ? &extdata_addr
							       : &data_addr;
		add_stringspace((initialiser_t *)il->il_initialiser_id, p_addr);
	}

	/*  Space for strings referenced from text.
	 */
	for (rl = tx->tx_string_relocs; rl != NULL; rl = rl->rl_next)
		data_addr += rl->rl_sc->sc_size;

	data_addr = ci_align_addr_for_type(data_addr, (type_t *)NULL);
	extdata_addr = ci_align_addr_for_type(extdata_addr, (type_t *)NULL);

	data = alloc(tx->tx_apool, (size_t)data_addr);
	memset(data, '\0', (size_t)data_addr);

	extdata = alloc(tx->tx_apool, (size_t)extdata_addr);
	memset(extdata, '\0', (size_t)extdata_addr);

	ci_do_static_initialisations(tx, tx->tx_initlist, data, extdata);

	ci_set_diag_handler((diag_handler_func_t)NULL, (char *)NULL);

	if (tx->tx_had_error) {
		alloc_free_pool(tx->tx_apool);
		return NULL;
	}

	/*  We have tx_max_sp set to the maximum stack space used by
	 *  any one function.  If there is only a single function, then
	 *  that is all the space we need, apart from a little slop for
	 *  any trap frame and parameters.  If there is more than one
	 *  function we don't know how much space might be needed - we
	 *  use the value zero to indicate this.
	 *
	 *  The one function case will be the normal one for ups breakpoint
	 *  code and display expressions.
	 *
	 *  TODO: work out maximum stack size when there are multiple
	 *        functions - possible if there is no recursion.
	 *
	 *  Note that tx_num_funcs is one higher than the number of
	 *  because it starts from one (to avoid NULL valid function ptrs.
	 */
	if (tx->tx_num_funcs <= 2) {
		if ((tx->tx_max_sp % sizeof(stackword_t)) != 0)
			panic("misaligned stack in ci_compile");
		max_stack_nwords = tx->tx_max_sp / sizeof(stackword_t) + 20;
	}
	else {
		max_stack_nwords = 0;
	}

	li = (linkinfo_t *)alloc(tx->tx_apool, sizeof(linkinfo_t));
	li->li_apool = tx->tx_apool;
	li->li_undef_vars = NULL;
	li->li_undef_funcs = tx->tx_undef_funcs;
	li->li_text = tx->tx_text;
	li->li_text_size = tx->tx_pc;
	li->li_data = data;
	li->li_data_size = data_addr;
	li->li_max_stack_nwords = max_stack_nwords;
	li->li_extdata = extdata;
	li->li_extdata_size = extdata_addr;
	li->li_bss_size = bss_addr;
	li->li_extbss_size = extbss_addr;
	li->li_reg_relocs = tx->tx_reg_relocs;
	
	make_extvars(tx->tx_apool, li, tx->tx_vlist, (size_t)extbss_addr);
	make_relocs(tx, li, data, extdata, str_addr, extstr_addr);
	make_funcinfo(tx->tx_apool, li, tx->tx_num_funcs, tx->tx_funclist);

	if ((tx->tx_flags & CI_CP_WANT_SYMINFO) != 0) {
		make_symfuncs(li, tx->tx_funclist, tx->tx_num_funcs - 1);
		make_symvars(li, tx->tx_varlist);
		li->li_filblock = pr->pr_block;
	}
	else {
		li->li_funcblocks = NULL;
		li->li_filblock = NULL;
		li->li_symfuncs = NULL;
		li->li_nsymfuncs = 0;
		li->li_symvars = NULL;
		li->li_nsymvars = 0;
	}

	li->li_srcpath = srcpath;
	li->li_undef_funcs = reverse_libfunc_list(li->li_undef_funcs);
	li->li_undef_vars = reverse_linkname_list(li->li_undef_vars);
	li->li_vars = reverse_linkname_list(li->li_vars);

#if 1
	if (tx->tx_labels != NULL)
		free(tx->tx_labels);
#endif

	return li;
}

static text_t *
compile(pr, report_error, checkarg_proc, regno_to_addr_proc, flags)
parse_res_t *pr;
ci_report_error_func_t report_error;
ci_checkarg_proc_t checkarg_proc;
ci_regno_to_addr_proc_t regno_to_addr_proc;
unsigned long flags;
{
	extern bool ci_Catching_panics;
	extern jmp_buf ci_Catch_panic_env;
	text_t *tx;
	func_t *f;
	taddr_t pclim, max_sp; 
	alloc_pool_t *ap;

	pclim = 200;

	ap = alloc_create_pool();
	tx = (text_t *)alloc(ap, sizeof(text_t));
	tx->tx_apool = ap;

	if ((flags & CI_CP_DONT_PANIC) != 0) {
		if (ci_Catching_panics)
			panic("catch_panics botch");

		ci_Catching_panics = TRUE;
		if (setjmp(ci_Catch_panic_env) != 0) {
			ci_set_diag_handler((diag_handler_func_t)NULL,
								(char *)NULL);
			alloc_free_pool(tx->tx_apool);
			ci_Catching_panics = FALSE;
			return NULL;
		}
	}

	tx->tx_text = (textword_t *)alloc(tx->tx_apool, (size_t)pclim * sizeof(textword_t));
	tx->tx_pclim = pclim;
	tx->tx_pc = 0; 
	tx->tx_labels = NULL;
	tx->tx_var_relocs = NULL;
	tx->tx_string_relocs = NULL;
	tx->tx_func_relocs = NULL;
	tx->tx_labellim = 0;
	tx->tx_lnos = NULL;
	tx->tx_last_lno = NULL;
	tx->tx_curfil = NULL;
	tx->tx_initlist = pr->pr_block->bl_initlist;
	tx->tx_label_lists[(int)LT_BREAK] = NULL;
	tx->tx_label_lists[(int)LT_CONTINUE] = NULL;
	tx->tx_label_lists[(int)LT_GOTO] = NULL;
	tx->tx_funclist = pr->pr_funcs;
	tx->tx_varlist = pr->pr_block->bl_vars;
	tx->tx_vlist = NULL;

	tx->tx_undef_funcs = NULL;
	tx->tx_num_undef_funcs = 1;	/* Use 1 to avoid zero func ptrs */

	tx->tx_report_error_func = report_error;
	tx->tx_checkarg_proc = checkarg_proc;
	tx->tx_regno_to_addr_proc = regno_to_addr_proc;
	tx->tx_had_error = FALSE;
	tx->tx_flags = flags;
	tx->tx_have_proc_vars = pr->pr_block->bl_parent != NULL;
	tx->tx_want_reg_relocs = (flags & CI_CP_WANT_REG_RELOCS) != 0;
	tx->tx_reg_relocs = NULL;

	/*  We initialise sp and max_sp to avoid complaints from purify
	 *  about uninitialised variables.
	 */
	tx->tx_sp = 0;
	tx->tx_max_sp = 0;

	/*  Assign addresses to all the functions.  We start from one
	 *  to avoid having NULL valid function pointers.
	 */
	tx->tx_num_funcs = 1;
	for (f = pr->pr_funcs; f != NULL; f = f->fu_next)
		++tx->tx_num_funcs;
	
	ci_set_diag_handler(report_compile_error, (char *)tx);


	/*  Stick an OC_TRAP opcode at the start of the text.  This is used
	 *  as a return pc target when we want to get control back after a
	 *  function returns.  We only use this when compiling single .c
	 *  files, but it is handy for debugging if we always get the same
	 *  object code for a given source file.
	 *
	 *  This also guarantees that no function starts with text address
	 *  zero, which is relied upon by ci_make_machine_from_linkinfo().
	 */
	ci_code_opcode(tx, OC_TRAP);
	
	max_sp = 0;
	for (f = pr->pr_funcs; f != NULL; f = f->fu_next) {
		f->fu_addr = tx->tx_pc;
		compile_func(tx, f);
		if (need_final_return(f->fu_statement)) {
			opcode_t opcode;

			opcode = OC_RET;
			ci_code_opcode(tx, opcode);
		}
		if (tx->tx_max_sp > max_sp)
			max_sp = tx->tx_max_sp;
	}

	/*  BUG: the +4 in the expression is to work around an off-by-one
	 *       error in the stack size calculation for the single function
	 *       case.  TODO: find out what's wrong and fix it.
	 */
	tx->tx_sp = max_sp + sizeof(stackword_t);

	ci_Catching_panics = FALSE;

	return tx;
}

void
ci_free_linkinfo(li)
linkinfo_t *li;
{
	/* free(li->li_text); */
	alloc_free_pool(li->li_apool);
}

void
ci_free_machine(ma)
machine_t *ma;
{
	/* free(ma->ma_text); */
	free(ma->ma_fdtab);
	alloc_free_pool(ma->ma_apool);
}

ci_opcode_t
ci_install_trap_instruction(ma, location)
machine_t *ma;
long location;
{
	int res;

	res = ma->ma_codefile->cf_text_and_data[location];
	ma->ma_codefile->cf_text_and_data[location] = (int)OC_TRAP;
	return res;
}

void
ci_uninstall_trap_instruction(ma, location, opcode)
machine_t *ma;
long location;
ci_opcode_t opcode;
{
	if (opcode >= NUM_OPCODES)
		ci_panic("opcode botch in cuti");

	if (ma->ma_codefile->cf_text_and_data[location] != (int)OC_TRAP)
		ci_panic("missing bpt in cuti");

	ma->ma_codefile->cf_text_and_data[location] = (int)opcode;
}

static taddr_t
assign_local_addrs(tx, blocks, base_addr)
text_t *tx;
block_t *blocks;
taddr_t base_addr;
{
	block_t *bl;
	taddr_t max_addr;

	max_addr = base_addr;
	for (bl = blocks; bl != NULL; bl = bl->bl_next) {
		taddr_t addr;

		addr = base_addr;
		set_var_addrs(tx, bl->bl_vars, &addr, FALSE);
		addr = assign_local_addrs(tx, bl->bl_blocks, addr);
		if (addr > max_addr)
			max_addr = addr;
	}
	return max_addr;
}

/*  Set the addresses of the parameters of function f.
 *  These are the variables hanging off the outermost block.
 *  As the stack grows downwards, the addresses are positive offsets
 *  from the frame pointer, starting at three stack words offset
 *  (these slots are used for the saved fp, pc and cf).
 *
 *  See the diagram below for a picture of the stack layout.
 */
static void
set_parameter_addrs(f)
func_t *f;
{
	taddr_t stack_addr;
	typecode_t typecode;

	stack_addr = 3 * sizeof(stackword_t);
	typecode = f->fu_type->ty_base->ty_code;
	if (typecode == TY_UNION || typecode == TY_STRUCT)
		stack_addr += sizeof(stackword_t);
	
	set_var_addrs((text_t *)NULL, f->fu__blocks->bl_vars,
							&stack_addr, TRUE);
}

static void
set_local_addrs(tx, f)
text_t *tx;
func_t *f;
{
	taddr_t stack_addr;

	/*  Allocate space for local variables.
	 */
	stack_addr = assign_local_addrs(tx, f->fu__blocks, 0);

	if (stack_addr % sizeof(stackword_t) != 0)
		stack_addr += sizeof(stackword_t) -
					stack_addr % sizeof(stackword_t);
	tx->tx_sp = stack_addr;

	/*  We offset the local addresses so they point above the
	 *  frame pointer.  As the stack grows downwards, these are
	 *  negative offsets starting at -4(fp).  We shift the
	 *  addresses down by the number of slots used by the locals,
	 *  so the last local ends up at -4(fp).
	 *
	 *  Thus:
	 *			  .	     .
	 *			  .	     .
	 *			  |----------|
	 *		   -8(fp) | local n-1|
	 *			  |----------|
	 *		   -4(fp) | local n  |
	 *			  |----------|
	 *	fp ----->   0(fp) | saved fp |
	 *			  |----------|
	 *		    4(fp) | saved pc |
	 *			  |----------|
	 *		    8(fp) | saved cf |
	 *			  |----------|
	 *		   12(fp) |  arg 1   |
	 *			  |----------|
	 *		   16(fp) |  arg 2   |
	 *			  |----------|
	 *			  .	     .
	 *			  .	     .
	 */
	adjust_local_addrs(f->fu__blocks, -(long)stack_addr);
}

/*  Add delta to the addresses of all local variables in block block,
 *  and recursively do all the sub-blocks.
 */
static void
adjust_local_addrs(par, delta)
block_t *par;
long delta;
{
	var_t *v;
	block_t *bl;
	
	for (bl = par->bl_blocks; bl != NULL; bl = bl->bl_next)
		adjust_local_addrs(bl, delta);

	for (v = par->bl_vars; v != NULL; v = v->va_next)
		if (v->va_class == CL_AUTO)
			v->va_addr += delta;
}

static void
compile_func(tx, f)
text_t *tx;
func_t *f;
{
	taddr_t savepc, checkpc, savesp;

	/*  All functions should have a single outer block which holds
	 *  the parameters and outermost local variables.  Check this.
	 */
	if (f->fu__blocks == NULL ||
	    f->fu__blocks->bl_next != NULL)
		ci_panic("blocks botch in cf");

	set_parameter_addrs(f);
	set_local_addrs(tx, f);

	tx->tx_max_sp = tx->tx_sp;
	ci_code_generic_opcode(tx, OC_LINK_B, tx->tx_sp);

	savesp = tx->tx_sp;
#if WANT_OLD_LINK
	checkpc = tx->tx_pc++;
#else
	/* Fix LINK ARG (CHAR -> LONG) : Dibyendu */
	checkpc = tx->tx_pc;
	tx->tx_pc += 4;
#endif

	tx->tx_curfil = f->fu_fil;
	compile_statement(tx, f->fu_statement);
	tx->tx_curfil = NULL;

	savepc = tx->tx_pc;
	tx->tx_pc = checkpc;
#if WANT_OLD_LINK
	ci_code_byte(tx, (long)(tx->tx_max_sp - savesp));
#else
	/* Fix LINK ARG (CHAR -> LONG) : Dibyendu */
	ci_code_long(tx, (long)(tx->tx_max_sp - savesp));
#endif
	tx->tx_pc = savepc;

	f->fu__lnos = tx->tx_lnos;
	tx->tx_lnos = tx->tx_last_lno = NULL;

	while (tx->tx_label_lists[(int)LT_GOTO] != NULL)
		pop_label(tx, LT_GOTO);
}

static bool
need_final_return(body_st)
statement_t *body_st;
{
	statement_t *last, *st;

	if (body_st->st_type != STT_COMPOUND)
		ci_panic("st_type botch in nfr");
	
	last = NULL;
	for (st = body_st->st_compound->co_statements; st != NULL; st = st->st_next)
		last = st;
	
	return last == NULL || last->st_type != STT_RETURN;
}

static void
compile_statement(tx, st)
text_t *tx;
statement_t *st;
{
	if (st->st_lexinfo != NULL)
		add_lno_entry(tx, tx->tx_pc, st->st_lexinfo);

	switch (st->st_type) {
	case STT_LABELED:
		compile_labeled_statement(tx, st->st_labeled);
		break;
	case STT_CASE_LABELED:
		compile_case_labeled_statement(tx, st->st_case);
		break;
	case STT_EXPR:
		compile_expression_statement(tx, st->st_expr);
		break;
	case STT_COMPOUND:
		compile_compound_statement(tx, st->st_compound);
		break;
	case STT_IF:
		compile_if_statement(tx, st->st_if);
		break;
	case STT_SWITCH:
		compile_switch_statement(tx, st->st_switch);
		break;
	case STT_WHILE:
		compile_while_or_do_statement(tx, st->st_while, TRUE);
		break;
	case STT_DO:
		compile_while_or_do_statement(tx, st->st_while, FALSE);
		break;
	case STT_FOR:
		compile_for_statement(tx, st->st_for);
		break;
	case STT_GOTO:
		compile_goto_statement(tx, st->st_goto_label);
		break;
	case STT_CONTINUE:
		compile_continue_or_break_statement(tx, LT_CONTINUE);
		break;
	case STT_BREAK:
		compile_continue_or_break_statement(tx, LT_BREAK);
		break;
	case STT_RETURN:
		compile_return_statement(tx, st->st_expr);
		break;
	default:
		ci_panic("bad stm type in cs");
	}

}

static void
compile_case_labeled_statement(tx, cs)
text_t *tx;
case_labeled_stm_t *cs;
{
	if (cs->cs_labeldesc == NO_LABELDESC)
		ci_panic("labeldesc botch in ccls");
	set_label_addr(tx, cs->cs_labeldesc, tx->tx_pc);
	compile_statement(tx, cs->cs_stm);
}

static void
add_lno_entry(tx, addr, lx)
text_t *tx;
taddr_t addr;
lexinfo_t *lx;
{
	lno_t *lno;

	lno = (lno_t *)alloc(tx->tx_apool, sizeof(lno_t));
	lno->ln_addr = addr;
	lno->ln_num = lx->lx_lnum;
	lno->ln_next = NULL;

	if (tx->tx_lnos == NULL)
		tx->tx_lnos = lno;
	else
		tx->tx_last_lno->ln_next = lno;
	tx->tx_last_lno = lno;
}

static void
compile_expression_statement(tx, expr)
text_t *tx;
expr_t *expr;
{
	if (expr != NULL)
		ci_compile_expression(tx, expr, EC_EFFECT);
}

static void
compile_compound_statement(tx, co)
text_t *tx;
compound_stm_t *co;
{
	statement_t *st;
	initlist_t *il, *next;

	for (il = co->co_block->bl_initlist; il != NULL; il = next) {
		next = il->il_next;
		if (IS_LOCAL_CLASS(il->il_var->va_class)) {
			initialiser_t *in;

			in = (initialiser_t *)il->il_initialiser_id;
			if (in->in_inittype != IT_EXPR) {
				diagf(ET_ERROR, il->il_var->va_lexinfo,
	      "Initialisation of local aggregate %s not yet implemented",
							il->il_var->va_name);
				continue;
			}
			ci_compile_var_reference(tx, il->il_var,
						 il->il_var->va_lexinfo, EC_ADDRESS);
			ci_compile_expression(tx, in->in_expr, EC_VALUE);
			ci_code_assign(tx, il->il_var->va_type, FALSE, AT_CI_ADDR);
		}
		else {
			il->il_next = tx->tx_initlist;
			tx->tx_initlist = il;
		}
	}
	for (st = co->co_statements; st != NULL; st = st->st_next)
		compile_statement(tx, st);
}

static void
compile_if_statement(tx, is)
text_t *tx;
if_stm_t *is;
{
	labeldesc_t elselabel;

	ci_compile_expression(tx, is->is_expr, EC_VALUE);
	elselabel = ci_code_jump_to_label(tx, OC_JUMP_IF_ZERO, NO_LABELDESC);
	compile_statement(tx, is->is_ifpart);

	if (is->is_elsepart == NULL)
		ci_resolve_and_free_label(tx, elselabel);
	else {
		labeldesc_t nextlabel;

		nextlabel = ci_code_jump_to_label(tx, OC_JUMP, NO_LABELDESC);
		ci_resolve_and_free_label(tx, elselabel);
		compile_statement(tx, is->is_elsepart);
		ci_resolve_and_free_label(tx, nextlabel);
	}
}

/*  If greater than MIN_JUMP_DENSITY percent of the possible case label
 *  values in a switch statement are used, we use a jump table, otherwise
 *  we use a chain of values.
 */
#define MIN_JUMP_DENSITY	80

static void
compile_switch_statement(tx, ss)
text_t *tx;
switch_stm_t *ss;
{
	case_labeled_stm_t **p_cs, **cslim;
	int ncase, minval, maxval, dist;
	labeldesc_t default_labeldesc;

	ci_compile_expression(tx, ss->ss_expr, EC_VALUE);

	ncase = ss->ss_ncase;
	cslim = ss->ss_cstab + ncase;

	/*  Make labels for all the switch statements
	 */
	for (p_cs = ss->ss_cstab; p_cs < cslim; ++p_cs)
		(*p_cs)->cs_labeldesc = make_label(tx);
	
	default_labeldesc = make_label(tx);
	if (ss->ss_default_cs != NULL)
		ss->ss_default_cs->cs_labeldesc = default_labeldesc;
	
	minval = (*ss->ss_cstab)->cs_val;
	maxval = cslim[-1]->cs_val;
	dist = maxval - minval;
	if (dist < 0)
		ci_panic("val botch in css");

	/*  Unsophisticated code generation here - if greater than
	 *  MIN_JUMP_DENSITY percent of the possible label values are
	 *  used, then use a jump table, otherwise use a chain of values.
	 */
	if (dist == 0 || (ncase * 100) / dist > MIN_JUMP_DENSITY) {
		int val;

		ci_code_opcode(tx, OC_SWITCH_ON_TABLE);
		ci_code_word(tx, dist + 1);
		ci_code_long(tx, minval);
		p_cs = ss->ss_cstab;
		for (val = 0; val <= dist; ++val) {
			labeldesc_t labeldesc;

			if (val + minval == (*p_cs)->cs_val) {
				labeldesc = (*p_cs)->cs_labeldesc;
				++p_cs;
			}
			else
				labeldesc = default_labeldesc;

			add_label_reference(tx, labeldesc, tx->tx_pc);
			tx->tx_pc += 2;
		}
		if (p_cs != cslim)
			ci_panic("case botch in css");
	}
	else {
		void (*codefunc)PROTO((text_t *ftx, long val));
		opcode_t opcode;

		if (dist <= MAX_BYTE) {
			opcode = OC_SWITCH_ON_CHAIN_B;
			codefunc = ci_code_byte;
		}
		else if (dist <= MAX_WORD) {
			opcode = OC_SWITCH_ON_CHAIN_W;
			codefunc = ci_code_word;
		}
		else {
			opcode = OC_SWITCH_ON_CHAIN_L;
			codefunc = ci_code_long;
		}
		ci_code_opcode(tx, opcode);
		ci_code_word(tx, ncase);
		ci_code_long(tx, minval);
		for (p_cs = ss->ss_cstab; p_cs < cslim; ++p_cs) {
			(*codefunc)(tx, (*p_cs)->cs_val - minval);
			add_label_reference(tx, (*p_cs)->cs_labeldesc, tx->tx_pc);
			tx->tx_pc += 2;
		}
	}
	add_label_reference(tx, default_labeldesc, tx->tx_pc);
	tx->tx_pc += 2;

	push_label(tx, LT_BREAK);
	compile_statement(tx, ss->ss_stm);
	pop_label(tx, LT_BREAK);
	ci_resolve_and_free_label(tx, default_labeldesc);

	for (p_cs = ss->ss_cstab; p_cs < cslim; ++p_cs)
		ci_resolve_and_free_label(tx, (*p_cs)->cs_labeldesc);
}

static void
compile_while_or_do_statement(tx, ws, is_while_statement)
text_t *tx;
while_stm_t *ws;
bool is_while_statement;
{
	labeldesc_t loop_label;

	push_label(tx, LT_BREAK);
	push_label(tx, LT_CONTINUE);

	if (is_while_statement)
		compile_continue_or_break_statement(tx, LT_CONTINUE);

	loop_label = make_label(tx);
	set_label_addr(tx, loop_label, tx->tx_pc);
	compile_statement(tx, ws->ws_stm);

	pop_label(tx, LT_CONTINUE);
	ci_compile_expression(tx, ws->ws_expr, EC_VALUE);
	(void) ci_code_jump_to_label(tx, OC_JUMP_IF_NON_ZERO, loop_label);

	pop_label(tx, LT_BREAK);
	ci_resolve_and_free_label(tx, loop_label);
}

static void
compile_for_statement(tx, fs)
text_t *tx;
for_stm_t *fs;
{
	labeldesc_t loop_label, test_label;

	push_label(tx, LT_BREAK);
	push_label(tx, LT_CONTINUE);

	compile_expression_statement(tx, fs->fs_init);
	test_label = ci_code_jump_to_label(tx, OC_JUMP, NO_LABELDESC);

	loop_label = make_label(tx);
	set_label_addr(tx, loop_label, tx->tx_pc);
	compile_statement(tx, fs->fs_stm);

	pop_label(tx, LT_CONTINUE);
	compile_expression_statement(tx, fs->fs_reinit);
	
	ci_resolve_and_free_label(tx, test_label);
	if (fs->fs_test != NULL) {
		ci_compile_expression(tx, fs->fs_test, EC_VALUE);
		(void) ci_code_jump_to_label(tx, OC_JUMP_IF_NON_ZERO, loop_label);
	}
	else
		(void) ci_code_jump_to_label(tx, OC_JUMP, loop_label);
	
	pop_label(tx, LT_BREAK);

	ci_resolve_and_free_label(tx, loop_label);
}

static void
compile_labeled_statement(tx, ls)
text_t *tx;
labeled_stm_t *ls;
{
	goto_label_t *gl;

	gl = ls->ls_goto_label;
	if (gl->gl_labeldesc == NO_LABELDESC) {
		push_label(tx, LT_GOTO);
		gl->gl_labeldesc = tx->tx_label_lists[(int)LT_GOTO]->ll_labeldesc;
	}
	set_label_addr(tx, gl->gl_labeldesc, tx->tx_pc);
	compile_statement(tx, ls->ls_stm);
}

static void
compile_goto_statement(tx, gl)
text_t *tx;
goto_label_t *gl;
{
	if (gl->gl_labeldesc == NO_LABELDESC) {
		push_label(tx, LT_GOTO);
		gl->gl_labeldesc = tx->tx_label_lists[(int)LT_GOTO]->ll_labeldesc;
	}
	(void) ci_code_jump_to_label(tx, OC_JUMP, gl->gl_labeldesc);
}

static void
compile_continue_or_break_statement(tx, label_type)
text_t *tx;
label_type_t label_type;
{
	label_list_t *ll;

	ll = tx->tx_label_lists[(int)label_type];
	if (ll == NULL)
		ci_panic("labellist botch in ccbs");
	(void) ci_code_jump_to_label(tx, OC_JUMP, ll->ll_labeldesc);
}

static void
compile_return_statement(tx, expr)
text_t *tx;
expr_t *expr;
{
	opcode_t opcode;
	typecode_t typecode;

	if (expr == NULL) {
		ci_code_opcode(tx, OC_RET);
		return;
	}

	typecode = expr->ex_type->ty_code;

	if (typecode == TY_STRUCT || typecode == TY_UNION) {
		if (expr->ex_exprtype == ET_FUNC_CALL) {
			ci_code_opcode(tx, OC_PUSH_STRUCTRET_ADDR);
			ci_compile_expression(tx, expr, EC_ADDRESS);
			ci_code_opcode(tx, OC_RET);
		}
		else {
			ci_compile_expression(tx, expr, EC_ADDRESS);
			ci_code_opcode(tx, OC_RET_STRUCT);
			ci_code_long(tx, ci_typesize(expr->ex_lexinfo,
								expr->ex_type));
		}
	}
	else {
		ci_compile_expression(tx, expr, EC_VALUE);

		switch (expr->ex_type->ty_code) {
		case TY_FLOAT:
			opcode = OC_RET_FLOAT;
			break;
		case TY_DOUBLE:
			opcode = OC_RET_DOUBLE;
			break;
#if WANT_LDBL
		case TY_LONGDOUBLE:
			opcode = OC_RET_LONG_DOUBLE;
			break;
#endif
#if WANT_LL
		case TY_LONGLONG:
		case TY_ULONGLONG:
			opcode = OC_RET_LONG_LONG;
			break;
#endif
		default:
			opcode = OC_RET_WORD;
			break;
		}
		ci_code_opcode(tx, opcode);
	}

}
