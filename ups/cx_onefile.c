/* cx_onefile.c - compile from a single source module to a machine */

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


/* @(#)cx_onefile.c	1.2 16 Sep 1994 (UKC) */
char ups_cx_onefile_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "ci_util.h"
#include "xc_opcodes.h"
#include "xc_machine.h"
#include "cx_link.h"

static void do_linkinfo_relocs PROTO((textword_t *text, char *data,
				      linkinfo_t *li,
				      taddr_t *varaddrs));
static codefile_t *make_codefile_from_linkinfo PROTO((alloc_pool_t *ap,
						      linkinfo_t *li,
						      ci_getaddrproc_t
						                  get_libaddr));
static void report_link_error PROTO((char *arg, errtype_t errtype,
					lexinfo_t *lx, const char *mesg));
static void adjust_location PROTO((textword_t *text, char *data,
				   unsigned long offset, unsigned long delta));
static int make_varaddrs PROTO((alloc_pool_t *ap, linkinfo_t *li,
				ci_getaddrproc_t get_libaddr,
				taddr_t **p_varaddrs));
static int make_funcrefs PROTO((alloc_pool_t *ap, linkinfo_t *li,
				ci_getaddrproc_t get_libaddr,
				extref_t ***p_funcrefs));
static reg_reloc_t *copy_reg_relocs PROTO((alloc_pool_t *ap, reg_reloc_t *rr));

static void
adjust_location(text, data, offset, delta)
textword_t *text;
char *data;
unsigned long offset, delta;
{
	char *addr;

	addr = (char *)text + offset;

	if (addr < data)
		ci_write_text_long((textword_t *)addr, GETLONG(addr) + delta);
	else
		*(unsigned long *)addr = *(unsigned long *)addr + delta;
}

static void
do_linkinfo_relocs(text, data, li, varaddrs)
textword_t *text;
char *data;
linkinfo_t *li;
taddr_t *varaddrs;
{
	reloc_pos_t *rp;
	ext_reloc_t *er;

	for (rp = li->li_static_rps; rp != NULL; rp = rp->rp_next) {
		adjust_location(text, data,
				rp->rp_location, (unsigned long)data);
	}

	for (er = li->li_ext_relocs; er != NULL; er = er->er_next) {
		adjust_location(text, data,
				er->er_location, varaddrs[er->er_index]);
	}

	/*  There is no need to do static function relocations because they
	 *  consist of OR'ing in the index of the code file, and with a
	 *  codefile the index is zero.
	 */

	if (li->li_extdata_size != 0 || li->li_extbss_size != 0)
		panic("unexpected external data");

	if (li->li_extdata_funcrelocs != NULL)
		panic("unexpected extdata funcrelocs");
	
	if (li->li_extdata_relocs != NULL)
		panic("unexpected extdata relocs");
}

static int
make_varaddrs(ap, li, get_libaddr, p_varaddrs)
alloc_pool_t *ap;
linkinfo_t *li;
ci_getaddrproc_t get_libaddr;
taddr_t **p_varaddrs;
{
	linkname_t *ln;
	int res, n_undef_vars, var_index;
	taddr_t *varaddrs;

	n_undef_vars = 0;
	for (ln = li->li_undef_vars; ln != NULL; ln = ln->ln_next)
		++n_undef_vars;

	varaddrs = (taddr_t *)alloc(ap, n_undef_vars * sizeof(taddr_t));
	
	res = 0;
	var_index = 0;
	
	for (ln = li->li_undef_vars; ln != NULL; ln = ln->ln_next) {
		taddr_t addr;
		
		switch ((*get_libaddr)(ln->ln_name, &addr)) {
		case CI_DIRECT_LIBFUNC:
		case CI_INDIRECT_LIBFUNC:
			diagf(ET_ERROR, (lexinfo_t *)NULL,
			      "%s is a function, but referred to as data",
			      ln->ln_name);
			res = -1;
			break;
				
		case CI_DATA:
			varaddrs[var_index] = addr;
			break;
			
		case CI_UNDEFINED:
			diagf(ET_ERROR, (lexinfo_t *)NULL,
			      "Variable %s undefined", ln->ln_name);
			res = -1;
			break;
			
		default:
			panic("get_libaddr botch");
		}

		++var_index;
	}

	*p_varaddrs = varaddrs;
	return res;
}

static int
make_funcrefs(ap, li, get_libaddr, p_funcrefs)
alloc_pool_t *ap;
linkinfo_t *li;
ci_getaddrproc_t get_libaddr;
extref_t ***p_funcrefs;
{
	libfunc_t *lf;
	extref_t *ertab, **funcrefs;
	int res, n_undef_funcs, func_index;

	n_undef_funcs = 0;
	for (lf = li->li_undef_funcs; lf != NULL; lf = lf->lf_next)
		++n_undef_funcs;
    
	ertab = (extref_t *)alloc(ap, n_undef_funcs * sizeof(extref_t));
	funcrefs = (extref_t **)alloc(ap, n_undef_funcs * sizeof(extref_t *)) +
		   n_undef_funcs;
	
	res = 0;
	func_index = 0;
	
	for (lf = li->li_undef_funcs; lf != NULL; lf = lf->lf_next) {
		taddr_t addr;
		ci_nametype_t nametype;

		switch (nametype = (*get_libaddr)(lf->lf_name, &addr)) {
		case CI_DIRECT_LIBFUNC:
		case CI_INDIRECT_LIBFUNC:
			ertab[func_index].er_file = NULL;
			ertab[func_index].er_indirect =
				               nametype == CI_INDIRECT_LIBFUNC;
			ertab[func_index].er_addr = addr;
			funcrefs[-(func_index + 1)] = &ertab[func_index];
			break;
				
		case CI_DATA:
			diagf(ET_ERROR, (lexinfo_t *)NULL,
			      "%s is data, but referred to as a function",
			      lf->lf_name);
			res = -1;
			break;
			
		case CI_UNDEFINED:
			diagf(ET_ERROR, &lf->lf_lexinfo,
			      "Function %s undefined", lf->lf_name);
			res = -1;
			break;
			
		default:
			panic("get_libaddr botch");
		}

		++func_index;
	}

	*p_funcrefs = funcrefs;
	return res;
}

static codefile_t *
make_codefile_from_linkinfo(ap, li, get_libaddr)
alloc_pool_t *ap;
linkinfo_t *li;
ci_getaddrproc_t get_libaddr;
{
	codefile_t *cf;
	extref_t **funcrefs;
	taddr_t *varaddrs;
	textword_t *text_and_data;
	
	if (make_funcrefs(ap, li, get_libaddr, &funcrefs) != 0 ||
	    make_varaddrs(ap, li, get_libaddr, &varaddrs) != 0)
		return NULL;
		
	cf = (codefile_t *)alloc(ap, sizeof(codefile_t));

	text_and_data = (textword_t *)alloc(ap, li->li_text_size +
					        li->li_data_size +
					        li->li_bss_size);
	memcpy(text_and_data, li->li_text, li->li_text_size);
	memcpy(text_and_data + li->li_text_size, li->li_data, li->li_data_size);
	memset(text_and_data + li->li_text_size + li->li_data_size, '\0',
	                                                      li->li_bss_size);
	
	cf->cf_funcrefs = funcrefs;
	cf->cf_varaddrs = NULL;
	cf->cf_path = NULL;
	cf->cf_offset = 0;
	cf->cf_loaded = TRUE;
	cf->cf_text_and_data = text_and_data;
	cf->cf_data_copy = memcpy(alloc(ap, li->li_data_size),
				  li->li_data, li->li_data_size);
	cf->cf_text_size = li->li_text_size;
	cf->cf_data_size = li->li_data_size;
	cf->cf_bss_size = li->li_bss_size;
	cf->cf_funcaddrs = li->li_funcaddrs;
	cf->cf_num_funcs = li->li_nfuncaddrs;

	cf->cf_srcpath = NULL;
	cf->cf_globalnames = NULL;
	cf->cf_nglobalnames = 0;

	do_linkinfo_relocs(cf->cf_text_and_data,
			   (char *)cf->cf_text_and_data + cf->cf_text_size,
			   li, varaddrs);

	return cf;
}

static void
report_link_error(arg, errtype, lx, mesg)
char *arg;
errtype_t errtype;
lexinfo_t *lx;
const char *mesg;
{
	ci_report_error_func_t *p_errfunc;

	p_errfunc = (ci_report_error_func_t *)arg;
	
	if (*p_errfunc != NULL && (*p_errfunc)(lx, mesg))
		*p_errfunc = NULL;
}


machine_t *
ci_make_machine_from_linkinfo(li, entry_name, stack_size, getaddr,
			      get_regaddr_proc, get_regaddr_proc_arg, errfunc)
linkinfo_t *li;
const char *entry_name;
unsigned stack_size;
ci_getaddrproc_t getaddr;
ci_get_regaddr_proc_t get_regaddr_proc;
char *get_regaddr_proc_arg;
ci_report_error_func_t errfunc;
{
	ci_report_error_func_t *p_errfunc;
	codefile_t *cf;
	size_t stack_nwords;
	size_t entry_point;
	alloc_pool_t *ap;
	machine_t *ma;

	ap = alloc_create_pool();
	
	p_errfunc = &errfunc;
	ci_set_diag_handler(report_link_error, (char *)p_errfunc);

	/*  ci_compile() will never start a function at address zero
	 *  because of the initial trap instruction.
	 */
	entry_point = 0;

	if (entry_name == NULL) {
		if (li->li_nfuncaddrs < 2) {
			diagf(ET_ERROR, (lexinfo_t *)NULL,
			      "No functions defined");
		}
		else {
			entry_point = li->li_funcaddrs[1];
		}
	}
	else {
		linkname_t *ln;
		
		for (ln = li->li_funcs; ln != NULL; ln = ln->ln_next) {
			if (strcmp(ln->ln_name, entry_name) == 0) {
				entry_point = ln->ln_addr;
				break;
			}
		}

		if (ln == NULL) {
			diagf(ET_ERROR, (lexinfo_t *)NULL,
			      "Function `%s' undefined", entry_name);
		}
	}

	if (entry_point == 0)
		cf = NULL;
	else
		cf = make_codefile_from_linkinfo(ap, li, getaddr);
	
	ci_set_diag_handler((diag_handler_func_t)NULL, (char *)NULL);
	
	if (cf == NULL) {
		alloc_free_pool(ap);
		return NULL;
	}


	if (stack_size == 0) {
		if ((stack_nwords = li->li_max_stack_nwords) == 0)
			stack_nwords = (16 * 1024) / sizeof(stackword_t);
	}
	else {
		stack_nwords = stack_size / sizeof(stackword_t);
	}

	ma = ci_create_machine(ap, stack_nwords, cf, 1, cf, entry_point);
	
	ma->ma_reg_relocs = copy_reg_relocs(ma->ma_apool, li->li_reg_relocs);
	ma->ma_get_regaddr_proc = get_regaddr_proc;
	ma->ma_get_regaddr_proc_arg = get_regaddr_proc_arg;

	if (entry_name != NULL && strcmp(entry_name, "main") == 0) {
		ma->ma_envp_pos = (textword_t *)--ma->ma_initial_sp;
		ma->ma_argv_pos = (textword_t *)--ma->ma_initial_sp;
		ma->ma_argc_pos = (textword_t *)--ma->ma_initial_sp;
		ci_push_trap_frame(ma);
	}

	return ma;
}

static reg_reloc_t *
copy_reg_relocs(ap, rr)
alloc_pool_t *ap;
reg_reloc_t *rr;
{
	reg_reloc_t *newlist;

	newlist = NULL;
	
	for (; rr != NULL; rr = rr->rr_next) {
		reg_reloc_t *new_rr;
		
		new_rr = (reg_reloc_t *)alloc(ap, sizeof(reg_reloc_t));
		*new_rr = *rr;
		new_rr->rr_next = newlist;
		newlist = new_rr;
	}

	return newlist;
}
	       
