/* xc_text.c - to open and parse C interpreter `binaries' */

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


/* @(#)xc_text.c	1.4 24 May 1995 (UKC) */
char ups_xc_text_c_rcsid[] = "$Id$";

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <stdio.h>

#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "st.h"
#include "ci.h"
#include "xc_opcodes.h"
#include "xc_machine.h"
#include "xc_symparse.h"
#include "xc_load.h"
#include "ci_opcodes.h"
#include "xc_builtins.h"
#include "xc.h"
#include "as.h"
#include "breakpoint.h"
#include "dx.h"
#include "cc.h"
#include "state.h"

typedef struct xc_tdata_s {
	symtab_t *xp_symtab;
	machine_t *xp_machine;
	int xp_lastsig;
	taddr_t xp_base_sp;
	tstate_t xp_tstate;
	stopres_t xp_stopres;
} xc_tdata_t;

typedef struct {
	int pr_bpt_offset;
} xc_preamble_t;

typedef struct {
	taddr_t xl_addr;
	int xl_lnum;
} xc_lnum_t;

typedef struct {
	codefile_t *xf_cf;
	xc_lnum_t *xf_letab;
	int xf_letab_size;
} xc_fidata_t;

typedef struct {
	xc_lnum_t *xfu_firstxl;
	xc_lnum_t *xfu_xllim;
} xc_fudata_t;

#define GET_XTD(xp)		((xc_tdata_t *)(xp)->xp_data)
#define GET_XF(fil)		((xc_fidata_t *)(fil)->fi_data)
#define GET_XFU(f)		((xc_fudata_t *)(f)->fu_symdata)
#define GET_PREAMBLE(f)		((xc_preamble_t *)(f)->fu_predata)
#define GET_ST_MACHINE(st)	((machine_t *)(st)->st_data)

static char **Libvars;

static bool xc_match PROTO((const char *textpath,
			    text_block_t *tb, size_t nbytes));
static int xc_init_from_textfile PROTO((target_t *xp, int textfd,
					const char *corepath,
					bool user_gave_core,
					const char **p_cmdline));
static bool cc_match PROTO((const char *textpath,
			    text_block_t *tb, size_t nbytes));
static int cc_init_from_textfile PROTO((target_t *xp, int textfd,
					const char *corepath,
					bool user_gave_core,
					const char **p_cmdline));
static int xc_read_text PROTO((symtab_t *st, taddr_t addr, char *buf,
								size_t nbytes));
static taddr_t xc_get_addr_lim PROTO((func_t *f));
static void xc_close_symtab_data PROTO((symtab_t *st));
static var_t *xc_get_fi_vars PROTO((fil_t *fil));
static macro_t *xc_get_fi_macros PROTO((fil_t *fil));
static bool xc_unwind PROTO((target_t *xp, symtab_t *st, taddr_t *fp,
                             taddr_t *sp, taddr_t *pc, taddr_t *cfa));
static func_t *xc_ensure_fu PROTO((func_t *f));
static fil_t *xc_get_fi PROTO((symtab_t *st));
static block_t *xc_get_fu_blocks PROTO((func_t *f));
static lno_t *xc_get_fu_lnos PROTO((func_t *f));
static const char *xc_disassemble_instruction PROTO((func_t *f, taddr_t addr,
				   		     const char *text,
						     const char **p_buf));
static bool xc_fil_may_have_matching_globals PROTO((fil_t *fil, const char *pat,
						    matchfunc_t matchf));
static bool cc_fil_may_have_matching_globals PROTO((fil_t *fil, const char *pat,
						    matchfunc_t matchf));
static jump_t *xc_get_jumps PROTO((func_t *f, taddr_t addr, const char *text,
				   size_t len, bool want_calls));
static int xc_create_child PROTO((target_t *xp, const char **argv,
				  const char **envp, long rdlist,
				  stopres_t *p_stopres));
static void xc_set_base_sp PROTO((target_t *xp, taddr_t sp));
static taddr_t xc_get_base_sp PROTO((target_t *xp));
static stopres_t xc_restart_child PROTO((target_t *xp, int sig,
					 cont_type_t ctype));
static void xc_kill PROTO((target_t *xp));
static int xc_is_attached PROTO((target_t *xp));
static int xc_get_addrsize PROTO((target_t *xp));
static tstate_t xc_get_state PROTO((target_t *xp));
static int xc_get_lastsig PROTO((target_t *xp));
static const siginfo_t *xc_get_lastsiginfo PROTO((target_t *xp));
static stopres_t xc_get_stopres PROTO((target_t *xp));
static sigstate_t xc_get_sigstate PROTO((target_t *xp, int sig));
static Stack *xc_get_stack_trace PROTO((target_t *xp));
static int xc_read_fpval PROTO((target_t *xp, taddr_t addr, int num_bytes,
							fpval_t *p_val));
static int xc_readreg PROTO((target_t *xp, int regno, taddr_t *p_val));
static int xc_setreg PROTO((target_t *xp, int regno, taddr_t val));
static taddr_t xc_get_retaddr_after_sig PROTO((target_t *xp));
static taddr_t xc_get_retaddr_after_jsr PROTO((target_t *xp, int rlink_reg));
static int xc_tswap PROTO((target_t *xp, taddr_t addr, xp_opcode_t opcode,
						xp_opcode_t *p_old_opcode));
static int xc_read_data PROTO((target_t *xp,
			       taddr_t addr, char *buf, size_t nbytes));
static int xc_write_data PROTO((target_t *xp,
				taddr_t addr, const char *buf, size_t nbytes));
static int xc_read_text_from_process PROTO((target_t *xp, taddr_t addr,
					    char *buf, size_t nbytes));
static int xc_call_func PROTO((target_t *xp, char *cma, taddr_t addr,
			       taddr_t *args, size_t *argsizes, int nargs,
			       taddr_t *p_res, typecode_t restype,
			       const char **p_mesg));
static taddr_t pack_textloc PROTO((machine_t *ma, codefile_t *cf,
				   textword_t *pc));
static void unpack_textloc PROTO((machine_t *ma, taddr_t addr,
				  codefile_t **p_cf, textword_t **p_pc));
static ci_exec_result_t single_step_machine PROTO((machine_t *ma));
static int load_symtab PROTO((fil_t *fil));
static const char *xc_get_signal_tag PROTO((target_t *xp, int sig,
					    const siginfo_t *siginfo));
static xc_tdata_t *make_xtd PROTO((alloc_pool_t *ap,
				   machine_t *ma, symtab_t *st));
static taddr_t xc_funcptr_to_addr PROTO((symtab_t *st, taddr_t addr));
static taddr_t xc_addr_to_funcptr PROTO((symtab_t *st, taddr_t addr));
static symtab_t *make_xc_symtab PROTO((alloc_pool_t *ap, target_t *xp,
				       machine_t *ma, bool is_cc,
				       fil_t *sfiles, func_t *funcs));
static void make_sfiles_and_funcs PROTO((alloc_pool_t *ap, machine_t *ma,
					 fil_t **p_sfiles, func_t **p_funcs));
static ci_nametype_t getaddr PROTO((const char *name, taddr_t *p_addr));
static void adjust_static_addrs PROTO((var_t *v, char *delta));
static void xc_show_target_driver_info PROTO((bool name_only));
static void cc_show_target_driver_info PROTO((bool name_only));
static int xc_initialise PROTO((ebuf_t *usage_eb, char **argv));
static int cc_initialise PROTO((ebuf_t *usage_eb, char **argv));
static int xc_get_min_bpt_addr PROTO((func_t *f, taddr_t *p_addr));
static bool xc_next_symtab PROTO((target_t *xp, symtab_t *st, bool load_new,
				  symtab_t **p_next_st));

xp_ops_t Xc_ops = {
	NULL, xc_initialise, xc_show_target_driver_info,
	xc_match, xc_init_from_textfile, xc_next_symtab,
	dx_start, dx_step, dx_next, dx_cont, xc_kill,
	xc_create_child, xc_restart_child,
	xc_set_base_sp, xc_get_base_sp,
	dx_add_breakpoint, dx_remove_breakpoint,
	dx_enable_breakpoint, dx_disable_breakpoint,
	dx_addr_to_breakpoint, dx_get_breakpoint_addr,
	dx_add_watchpoint, dx_remove_watchpoint,
	dx_enable_watchpoint, dx_disable_watchpoint,
	NULL, NULL,
	xc_is_attached, NULL, xc_get_addrsize,
	xc_get_state, xc_get_lastsig, xc_get_lastsiginfo,
	xc_get_stopres, xc_get_sigstate,
	xc_get_stack_trace, NULL, xc_get_signal_tag,
	xc_read_fpval, NULL, xc_readreg, xc_setreg, NULL, NULL,
	xc_get_retaddr_after_jsr, xc_get_retaddr_after_sig,
	xc_tswap,
	xc_read_data, xc_write_data, xc_read_text_from_process,
	NULL, xc_call_func, NULL, NULL, NULL, NULL, NULL
};

static bool Disable_xc;

xp_ops_t Cc_ops = {
	NULL, cc_initialise, cc_show_target_driver_info,
	cc_match, cc_init_from_textfile, xc_next_symtab,
	dx_start, dx_step, dx_next, dx_cont, xc_kill,
	xc_create_child, xc_restart_child,
	xc_set_base_sp, xc_get_base_sp,
	dx_add_breakpoint, dx_remove_breakpoint,
	dx_enable_breakpoint, dx_disable_breakpoint,
	dx_addr_to_breakpoint, dx_get_breakpoint_addr,
	dx_add_watchpoint, dx_remove_watchpoint,
	dx_enable_watchpoint, dx_disable_watchpoint,
	NULL, NULL,
	xc_is_attached, NULL, xc_get_addrsize,
	xc_get_state, xc_get_lastsig, xc_get_lastsiginfo,
	xc_get_stopres, xc_get_sigstate,
	xc_get_stack_trace, NULL, xc_get_signal_tag,
	xc_read_fpval, NULL, xc_readreg, xc_setreg, NULL, NULL,
	xc_get_retaddr_after_jsr, xc_get_retaddr_after_sig,
	xc_tswap,
	xc_read_data, xc_write_data, xc_read_text_from_process,
	NULL, xc_call_func, NULL, NULL, NULL, NULL, NULL
};

static bool Disable_cc;

static int
xc_initialise(usage_eb, argv)
ebuf_t *usage_eb;
char **argv;
{
	Disable_xc = extract_bool_arg(usage_eb, argv, "-noxc");
	return 0;
}

static void
xc_show_target_driver_info(name_only)
bool name_only;
{
	if (Disable_xc)
		return;
	if (name_only)
		puts("\tC interpreter output files");
}

static int
cc_initialise(usage_eb, argv)
ebuf_t *usage_eb;
char **argv;
{
	Disable_cc = extract_bool_arg(usage_eb, argv, "-nocc");
	return 0;
}

static void
cc_show_target_driver_info(name_only)
bool name_only;
{
	if (Disable_cc)
		return;
	
	if (name_only)
		puts("\tC source files");
}


static bool
xc_match(textpath, tb, nbytes)
const char *textpath;
text_block_t *tb;
size_t nbytes;
{
	return !Disable_xc &&
		nbytes >= sizeof(x_header_t) &&
		((x_header_t *)tb)->xh_magic == OCX_MAGIC;
}

static bool
cc_match(textpath, tb, nbytes)
const char *textpath;
text_block_t *tb;
size_t nbytes;
{
	const char *s, *lim;

	if (Disable_cc)
		return FALSE;
	
	if (strlen(textpath) < (size_t)3 ||
	    strcmp(textpath + strlen(textpath) - 2, ".c") != 0)
		return FALSE;

	lim = &tb->buf[nbytes];
	for (s = tb->buf; s < lim; ++s) {
		if (!isprint(*s) && !isspace(*s))
			return FALSE;
	}

	return TRUE;
}

/*  BUG: this is a copy of the getaddr() in cg.c - we should have a single
 *       copy somewhere.
 */
static ci_nametype_t
getaddr(name, p_addr)
const char *name;
taddr_t *p_addr;
{
	ci_nametype_t nametype;

	if ((nametype = cc_getaddr(name, p_addr)) == CI_DATA)
		*p_addr = (taddr_t)Libvars[*p_addr];

	return nametype;
}

static int
cc_init_from_textfile(xp, textfd, corepath, user_gave_core, p_cmdline)
target_t *xp;
int textfd;
const char *corepath;
bool user_gave_core;
const char **p_cmdline;
{
	parse_id_t parse_id;
	linkinfo_t *li;
	machine_t *ma;
	codefile_t *cf;
	char *delta;
	fil_t *fil;
	xc_fidata_t *xf;
	func_t *f, *funcs;
	symtab_t *st;
	size_t n_libfuncs, n_libvars;

	if (user_gave_core) {
		errf("C interpreter core files are not supported");
		return -1;
	}

	if ((li = cc_parse_and_compile(get_ccstate(), xp->xp_textpath,
				       &parse_id, NULL)) == NULL)
		return -1;

	cc_get_libinfo(&n_libfuncs, &n_libvars);
	Libvars = cx_get_libvars(n_libvars);
	
	ma = ci_make_machine_from_linkinfo(li, "main", 10240, getaddr,
					   (ci_get_regaddr_proc_t)NULL,
					   (char *)NULL, cc_report_error);
	if (ma == NULL)
		return -1;
	
	ma->ma_libfuncs = cx_get_libfuncs(n_libfuncs);

	cf = ma->ma_cftab;
	delta = (char *)cf->cf_text_and_data + cf->cf_text_size;
	
	funcs = ci_get_parse_id_funcs(parse_id);
	for (f = funcs; f != NULL; f = f->fu_next) {
		f->fu_flags |= FU_DONE_LNOS;
		iterate_over_vars_of_block(f->fu__blocks,
					   adjust_static_addrs, delta);
	}

	xf = (xc_fidata_t *)alloc(ma->ma_apool, sizeof(xc_fidata_t));
	xf->xf_cf = cf;
	xf->xf_letab = NULL;
	xf->xf_letab_size = 0;
	
	fil = funcs->fu_fil;
	fil->fi_flags |= FI_DONE_VARS;
	fil->fi_data = (char *)xf;
		
	iterate_over_vars_of_block(fil->fi_block, adjust_static_addrs, delta);

	st = make_xc_symtab(ma->ma_apool, xp, ma, TRUE, fil, funcs);
	xp->xp_data = (char *)make_xtd(xp->xp_apool, ma, st);
	
	return 0;
}

static void
adjust_static_addrs(v, delta)
var_t *v;
char *delta;
{
	if (v->va_class == CL_STAT || v->va_class == CL_LSTAT)
		v->va_addr += (taddr_t)delta;
}

static int
xc_init_from_textfile(xp, textfd, corepath, user_gave_core, p_cmdline)
target_t *xp;
int textfd;
const char *corepath;
bool user_gave_core;
const char **p_cmdline;
{
	machine_t *ma;
	fil_t *sfiles;
	func_t *funcs;
	symtab_t *st;
	alloc_pool_t *ap;

	if (user_gave_core) {
		errf("C interpreter core files are not supported");
		return -1;
	}

	if (ci_load(xp->xp_textpath, 10240, FALSE,
				(char **)NULL, (char **)NULL, &ma) != 0)
		return -1;

	ap = alloc_create_pool();
	make_sfiles_and_funcs(ap, ma, &sfiles, &funcs);
	
	st = make_xc_symtab(ap, xp, ma, FALSE, sfiles, funcs);
	xp->xp_data = (char *)make_xtd(xp->xp_apool, ma, st);
	
	return 0;
}

static bool
xc_next_symtab(xp, st, load_new, p_next_symtab)
target_t *xp;
symtab_t *st;
bool load_new;
symtab_t **p_next_symtab;
{
	if (st != NULL)
		return FALSE;
	
	*p_next_symtab = GET_XTD(xp)->xp_symtab;
	return TRUE;
}

static xc_tdata_t *
make_xtd(ap, ma, st)
alloc_pool_t *ap;
machine_t *ma;
symtab_t *st;
{
	xc_tdata_t *xtd;
	
	xtd = (xc_tdata_t *)alloc(ap, sizeof(xc_tdata_t));
	xtd->xp_machine = ma;
	xtd->xp_lastsig = 0;
	xtd->xp_tstate = TS_NOTR;
	xtd->xp_symtab = st;

	return xtd;
}

static int 
xc_create_child(xp, argv, envp, rdlist, p_stopres)
target_t *xp;
const char **argv, **envp;
long rdlist;
stopres_t *p_stopres;
{
	xc_tdata_t *xtd;
	machine_t *ma;
	taddr_t main_addr, main_min_bpt_addr;

	xtd = GET_XTD(xp);
	ma = xtd->xp_machine;

	ci_initialise_machine(ma, TRUE, FALSE, (char **)argv, (char **)envp);

	if (get_startup_stop_addrs(xp, &main_addr, &main_min_bpt_addr) != 0)
		panic("can't get startup stop addrs");
	*p_stopres = xp_execto(xp, main_min_bpt_addr);

	return 0;
}

static void
xc_set_base_sp(xp, sp)
target_t *xp;
taddr_t sp;
{
	GET_XTD(xp)->xp_base_sp = sp;
}

static taddr_t
xc_get_base_sp(xp)
target_t *xp;
{
	return GET_XTD(xp)->xp_base_sp;
}

static stopres_t
xc_restart_child(xp, sig, ctype)
target_t *xp;
int sig;
cont_type_t ctype;
{
	ci_exec_result_t res;
	machine_t *ma;
	taddr_t pcaddr;
	xc_tdata_t *xtd;
	breakpoint_t *bp;

	xtd = GET_XTD(xp);
	ma = xtd->xp_machine;

	if (sig != 0)
		panic("cont botch in xc_restart_child");

	/*  Step over bpt if there is one at ma->ma_pc.
	 */
	pcaddr = pack_textloc(ma, ma->ma_codefile, ma->ma_pc);

	if (xtd->xp_tstate == TS_STOPPED &&
	    (bp = get_breakpoint_at_addr(xp, pcaddr)) != NULL) {
		if (uninstall_breakpoint(bp) != 0)
			panic("uib failed");

		if (single_step_machine(ma) != CI_ER_TRAP)
			panic("ssm botch");

		if (install_breakpoint(bp, xp, FALSE) != 0)
			panic("ib failed");

		if (ctype == CT_STEP)
			return SR_SSTEP;
	}
	
	switch (ctype) {
	case CT_CONT:
		res = ci_execute_machine(ma,
			(textaddr_t)0, (textaddr_t)0,
			(textaddr_t)0, (textaddr_t)0,
			(ci_readproc_t)NULL, (ci_writeproc_t)NULL,
		        (ci_indirect_call_proc_t)NULL);
		break;
	case CT_STEP:
		res = single_step_machine(ma);
		break;
	default:
		panic("bad ctype in xc_restart_child");
		res = (ci_exec_result_t)0;	/* to satisfy gcc */
	}
	
	switch (res) {
	case CI_ER_TRAP:
		xtd->xp_lastsig = 0;
		xtd->xp_tstate = TS_STOPPED;
		xtd->xp_stopres = (ctype == CT_STEP) ? SR_SSTEP: SR_BPT;
		break;
	case CI_ER_EXITED:
		ci_reset_machine(ma);
		xtd->xp_lastsig = -1;
		xtd->xp_tstate = TS_NOTR;
		xtd->xp_stopres = SR_DIED;
		break;
	default:
		xtd->xp_lastsig = -((int)res + 1);
		xtd->xp_tstate = TS_HALTED;
		xtd->xp_stopres = SR_SIG;
		break;
	}

	return xtd->xp_stopres;
}

static ci_exec_result_t
single_step_machine(ma)
machine_t *ma;
{
	codefile_t *ncf;
	textword_t *npc;
	textword_t saved_opcode;
	ci_exec_result_t res;

	ci_get_next_pc_and_cf(ma, &ncf, &npc);

	saved_opcode = GETOPCODE(npc);
	PUTOPCODE(npc, OC_TRAP);

	res = ci_execute_machine(ma,
			(textaddr_t)0, (textaddr_t)0,
			(textaddr_t)0, (textaddr_t)0,
			(ci_readproc_t)NULL, (ci_writeproc_t)NULL,
		        (ci_indirect_call_proc_t)NULL);

	PUTOPCODE(npc, saved_opcode);

	return res;
}

static void
xc_kill(xp)
target_t *xp;
{
	xc_tdata_t *xtd;

	xtd = GET_XTD(xp);

	if (uninstall_all_breakpoints(xp) != 0)
		panic("can't uninstall bpts in xc_kill");

	ci_reset_machine(xtd->xp_machine);
	xtd->xp_lastsig = 0;
	xtd->xp_tstate = TS_NOTR;
	xtd->xp_stopres = SR_DIED;
}

static int
xc_is_attached(xp)
target_t *xp;
{
	return FALSE;
}

static int
xc_get_addrsize(xp)
target_t *xp;
{
	return 32;
}

static tstate_t
xc_get_state(xp)
target_t *xp;
{
	return GET_XTD(xp)->xp_tstate;
}

static int
xc_get_lastsig(xp)
target_t *xp;
{
	return GET_XTD(xp)->xp_lastsig;
}

static const siginfo_t *
xc_get_lastsiginfo(xp)
target_t *xp;
{
	return NULL;
}

static stopres_t
xc_get_stopres(xp)
target_t *xp;
{
	return GET_XTD(xp)->xp_stopres;
}

static sigstate_t
xc_get_sigstate(xp, sig)
target_t *xp;
int sig;
{
	errf("\bxc_get_sigstate NYI");
	return SGH_DEFAULT;	/* to satisfy gcc */
}

static Stack *
xc_get_stack_trace(xp)
target_t *xp;
{
	typedef struct {
		stackword_t *fr_savfp;
		textword_t *fr_savpc;
		codefile_t *fr_savcf;
	} frame_t;
	machine_t *ma;
	frame_t *fr;
	textword_t *pc;
	codefile_t *cf;
	stackword_t *fp;
	Stack *stk, *last;
	func_t *f;
	
	ma = GET_XTD(xp)->xp_machine;

	pc = ma->ma_pc;
	fp = (stackword_t *)ma->ma_fp;
	cf = ma->ma_codefile;

	for (stk = last = NULL; fp != NULL; last = stk) {
		taddr_t pcaddr;
		fil_t *fil;
		int lnum;

		pcaddr = pack_textloc(ma, cf, pc);
		
		if ((f = addr_to_func(pcaddr)) == NULL)
			panic("bad func in gst");

		addr_to_fil_and_lnum(f, pcaddr, &fil, &lnum, FALSE);
		
		stk = make_stk(f, pcaddr, fil, lnum, last);
		stk->stk_fp = (taddr_t)fp;
		stk->stk_ap = (taddr_t)fp;
		stk->stk_sp = (taddr_t)fp;

		fr = (frame_t *)fp;
		fp = fr->fr_savfp;
		pc = fr->fr_savpc;
		cf = fr->fr_savcf;
	}

	if (fp != NULL && stk != NULL)
		stk->stk_bad = TRUE;

	return stk;
}

static const char *
xc_get_signal_tag(xp, sig, siginfo)
target_t *xp;
int sig;
const siginfo_t *siginfo;
{
	ci_exec_result_t res;

	if (sig > 0)
		return signame(sig, siginfo);

	res = (ci_exec_result_t)(-sig - 1);

	return ci_exec_result_to_string(res);
}


static int
xc_read_fpval(xp, addr, num_bytes, p_val)
target_t *xp;
taddr_t addr;
int num_bytes;
fpval_t *p_val;
{
	errf("\bxc_read_fpval NYI");
	return -1;
}

static taddr_t
xc_funcptr_to_addr(st, addr)
symtab_t *st;
taddr_t addr;
{
	machine_t *ma;
	codefile_t *cf;
	unsigned cf_index, pc_offset;
	int func_index;

	if (addr == 0)
		return 0;
	
	ma = GET_ST_MACHINE(st);

	cf_index = addr >> 16;
	func_index = (short)addr;		/* Want sign extension */
	
	if (cf_index >= ma->ma_cftab_size)
		return addr;
	cf = ma->ma_cftab + cf_index;

	if (!cf->cf_loaded && XLOAD(ma, cf) != 0)
		return addr;

	if (func_index >= cf->cf_num_funcs)
		return addr;

	if (func_index > 0) {
		pc_offset = cf->cf_funcaddrs[func_index];
	}
	else {
		extref_t *er;
		
		er = cf->cf_funcrefs[func_index];

		if (er->er_file == NULL)
			return addr;

		cf_index = er->er_file - ma->ma_cftab;
		pc_offset = er->er_addr;
	}
	
	
	return (cf_index << 16) | pc_offset;
}

static taddr_t
xc_addr_to_funcptr(st, addr)
symtab_t *st;
taddr_t addr;
{
	machine_t *ma;
	codefile_t *cf;
	unsigned cf_index, pc_offset;
	int func_index;

	if (addr == 0)
		return addr;

	ma = GET_ST_MACHINE(st);

	cf_index = addr >> 16;
	pc_offset = (unsigned short)addr;

	if (cf_index >= ma->ma_cftab_size)
		return addr;
	cf = ma->ma_cftab + cf_index;

	if (!cf->cf_loaded && XLOAD(ma, cf) != 0)
		return addr;

	for (func_index = 1; func_index < cf->cf_num_funcs; ++func_index) {
		if (cf->cf_funcaddrs[func_index] == pc_offset)
			return (cf_index << 16) | func_index;
	}

	return addr;
}

static taddr_t
pack_textloc(ma, cf, pc)
machine_t *ma;
codefile_t *cf;
textword_t *pc;
{
	unsigned cf_index, pc_offset;

	cf_index = cf - ma->ma_cftab;
	pc_offset = pc - cf->cf_text_and_data;

	if (((cf_index | pc_offset) & ~(unsigned)0xffff) != 0)
		panic("pc botch in ptl");

	return (cf_index << 16) | pc_offset;
}

static void
unpack_textloc(ma, addr, p_cf, p_pc)
machine_t *ma;
taddr_t addr;
codefile_t **p_cf;
textword_t **p_pc;
{
	unsigned cf_index, pc_offset;

	cf_index = (addr >> 16);
	pc_offset = addr & 0xffff;

	if (cf_index > ma->ma_cftab_size)
		panic("cf botch in utl");
	*p_cf = ma->ma_cftab + cf_index;

	if (pc_offset > (*p_cf)->cf_text_size)
		panic("pc botch in utl");
	*p_pc = (*p_cf)->cf_text_and_data + pc_offset;
}

static int
xc_readreg(xp, regno, p_val)
target_t *xp;
int regno;
taddr_t *p_val;
{
	machine_t *ma;

	ma = GET_XTD(xp)->xp_machine;
	
	switch (regno) {
	case UPSREG_PC:
		*p_val = pack_textloc(ma, ma->ma_codefile, ma->ma_pc);
		break;
	case UPSREG_SP:
		*p_val = (taddr_t)ma->ma_sp;
		break;
	case UPSREG_FP:
	case UPSREG_AP:
		*p_val = (taddr_t)ma->ma_fp;
		break;
	default:
		panic("bad regno in rr");
	}

	return 0;
}

static int
xc_setreg(xp, regno, val)
target_t *xp;
int regno;
taddr_t val;
{
	machine_t *ma;

	ma = GET_XTD(xp)->xp_machine;
	
	switch (regno) {
	case UPSREG_PC:
		unpack_textloc(ma, val, &ma->ma_codefile, &ma->ma_pc);
		break;
	case UPSREG_SP:
		ma->ma_sp = (stackword_t *)val;
		break;
	case UPSREG_FP:
	case UPSREG_AP:
		ma->ma_fp = (stackword_t *)val;
		break;
	default:
		panic("bad regno in rr");
	}

	return 0;
}

static taddr_t
xc_get_retaddr_after_sig(xp)
target_t *xp;
{
	panic("gras NYI");
	return 0;	/* to satisfy gcc */
}

static taddr_t
xc_get_retaddr_after_jsr(xp, rlink_reg)
target_t *xp;
int rlink_reg;
{
	panic("graj NYI");
	return 0;	/* to satisfy gcc */
}

static int
xc_tswap(xp, addr, xp_opcode, p_old_opcode)
target_t *xp;
taddr_t addr;
xp_opcode_t xp_opcode, *p_old_opcode;
{
	machine_t *ma;
	codefile_t *cf;
	textword_t *pc;
	opcode_t opcode, old_opcode;

	ma = GET_XTD(xp)->xp_machine;
	unpack_textloc(ma, addr, &cf, &pc);

	/*  BUG: pack_textloc() and unpack_textloc() should use pc offsets,
	 *       not actual text pointers.  This kludge would not then be
	 *       necessary.
	 */
	if (!cf->cf_loaded) {
		if (XLOAD(ma, cf) != 0)
			return -1;
		unpack_textloc(ma, addr, &cf, &pc);
	}

	opcode = (p_old_opcode != NULL) ? OC_TRAP : (opcode_t)xp_opcode;

	old_opcode = GETOPCODE(pc);
	PUTOPCODE(pc, opcode);

	if ((opcode == OC_TRAP) == (old_opcode == OC_TRAP)) {
		if (opcode == OC_TRAP)
			/* Dibyendu : don't know why this happens - ignore */
			errf("duplicate breakpoint in xc_tswap");
		else
			panic("vanished breakpoint in xc_tswap");
	}

	if (p_old_opcode != NULL)
		*p_old_opcode = (xp_opcode_t)old_opcode;

	return 0;
}

static int
xc_read_data(xp, addr, buf, nbytes)
target_t *xp;
taddr_t addr;
char *buf;
size_t nbytes;
{
	if (!ci_is_ci_addr(GET_XTD(xp)->xp_machine, addr, nbytes))
		return -1;

	memcpy(buf, (char *)addr, nbytes);
	return 0;
}

static int
xc_write_data(xp, addr, buf, nbytes)
target_t *xp;
taddr_t addr;
const char *buf;
size_t nbytes;
{
	if (!ci_is_ci_addr(GET_XTD(xp)->xp_machine, addr, nbytes))
		return -1;

	memcpy((char *)addr, buf, nbytes);
	return 0;
}

static int
xc_read_text_from_process(xp, addr, buf, nbytes)
target_t *xp;
taddr_t addr;
char *buf;
size_t nbytes;
{
	codefile_t *cf;
	textword_t *text;

	unpack_textloc(GET_XTD(xp)->xp_machine, addr, &cf, &text);
	memcpy(buf, (char *)text, nbytes);
	return 0;
}

static int
xc_call_func(xp, cma, addr, args, argsizes, nargs, p_res, restype, p_mesg)
target_t *xp;
char *cma;
taddr_t addr;
taddr_t *args;
size_t *argsizes;
int nargs;
taddr_t *p_res;
typecode_t restype;
const char **p_mesg;
{
	machine_t *ma;
	stackword_t *sp;
	codefile_t *cf, *fcf;
	textword_t *pc, *fpc;
	int i;

	ma = GET_XTD(xp)->xp_machine;
	
	sp = ma->ma_sp;
	pc = ma->ma_pc;
	cf = ma->ma_codefile;

	for (i = nargs - 1; i >= 0; --i)
		*--ma->ma_sp = args[i];

	unpack_textloc(ma, addr, &fcf, &fpc);

	/*  BUG: pack_textloc() and unpack_textloc() should use pc offsets,
	 *       not actual text pointers.  This kludge would not then be
	 *       necessary.
	 */
	if (!fcf->cf_loaded) {
		if (XLOAD(ma, fcf) != 0)
			return -1;
		unpack_textloc(ma, addr, &fcf, &fpc);
	}

	*--ma->ma_sp = (stackword_t)cf;
	*--ma->ma_sp = (stackword_t)cf->cf_text_and_data;

	ma->ma_pc = fpc;
	ma->ma_codefile = fcf;

	ci_execute_machine(ma, (textaddr_t)NULL, (textaddr_t)NULL,
			   (textaddr_t)NULL, (textaddr_t)NULL,
			   (ci_readproc_t)NULL, (ci_writeproc_t)NULL,
			   (ci_indirect_call_proc_t)NULL);

	*p_mesg = NULL;
	*p_res = ma->ma_retval.d_word;
	
	ma->ma_pc = pc;
	ma->ma_codefile = cf;
	ma->ma_sp = sp;
	
	return 0;
}

static symtab_t *
make_xc_symtab(ap, xp, ma, is_cc, sfiles, funcs)
alloc_pool_t *ap;
target_t *xp;
machine_t *ma;
bool is_cc;
fil_t *sfiles;
func_t *funcs;
{
	static sym_ops_t xc_ops = {
		xc_read_text,
		xc_get_min_bpt_addr,
		xc_get_addr_lim,

		xc_funcptr_to_addr,
		xc_addr_to_funcptr,
		
		xc_fil_may_have_matching_globals,
		xc_close_symtab_data,

		NULL,
		NULL,
		NULL,
		NULL,
		NULL,

		xc_ensure_fu,
		xc_get_fi,
		xc_get_fu_lnos,
		xc_get_fu_blocks,
		xc_get_fi_vars,
		xc_get_fi_macros,

		xc_unwind,

		xc_disassemble_instruction,
		xc_get_jumps,
	};
	static sym_ops_t cc_ops;
	symtab_t *st;
	taddr_t max_addr;
	codefile_t *cf;
	func_t *funclist;
	functab_t *functab;

	if (cc_ops.so_read_text == NULL) {
		cc_ops = xc_ops;
		cc_ops.so_fil_may_have_matching_globals =
			                   cc_fil_may_have_matching_globals;
	}
	
	cf = &ma->ma_cftab[ma->ma_cftab_size - 1];
	max_addr = pack_textloc(ma, cf,
				cf->cf_text_and_data + cf->cf_text_size);

	make_funcinfo(ap, funcs, (int)ma->ma_funcmap_size, 0, max_addr, 0,
		      &funclist, &functab);

	st = make_symtab(ap, xp->xp_textpath, sfiles, funclist,
			 is_cc ? &cc_ops : &xc_ops, (char *)ma);
	st->st_functab = functab;
	
	return st;
}

static void
make_sfiles_and_funcs(ap, ma, p_sfiles, p_funcs)
alloc_pool_t *ap;
machine_t *ma;
fil_t **p_sfiles;
func_t **p_funcs;
{
	int i;
	fil_t *nextfil, **filtab;
	func_t *nextf;
	
	filtab = (fil_t **)e_malloc(ma->ma_cftab_size * sizeof(fil_t *));

	nextfil = NULL;
	for (i = ma->ma_cftab_size - 1; i >= 0; --i) {
		xc_fidata_t *xf;

		xf = (xc_fidata_t *)alloc(ap, sizeof(xc_fidata_t));
		xf->xf_cf = &ma->ma_cftab[i];

		filtab[i] = ci_make_fil(ap, xf->xf_cf->cf_srcpath, (char *)xf,
					(block_t *)NULL, nextfil);
		/* Dibyendu : added to fool ao_compiler() into returning
		 * CT_UNKNOWN when running UPS over a CX executable.
		 * This causes demangle_name_2() to do nothing 
		 * (see ao_symscan.c).
		 */
		filtab[i]->fi_flags |= FI_FOUND_COMPILER;
		nextfil = filtab[i];
	}

	nextf = NULL;
	for (i = ma->ma_funcmap_size - 1; i >= 0; --i) {
		fil_t *fil;
		func_t *f;
		funcmap_t *fm;
		codefile_t *cf;
		funclist_t *fl;
		xc_fudata_t *xfu;
		
		fm = &ma->ma_funcmap[i];
		fil = filtab[fm->fm_file_index];
		cf = GET_XF(fil)->xf_cf;

		xfu = (xc_fudata_t *)alloc(ap, sizeof(xc_fudata_t));

		/*  BUG: the call of pack_textloc() here involves adding and
		 *  subtracting a NULL pointer from another pointer.  This is
		 *  non-portable.
		 */
		f = ci_make_func(ap, fm->fm_name,
				 pack_textloc(ma, cf,
					      cf->cf_text_and_data+fm->fm_addr),
				 (symtab_t *)NULL, fil, nextf);
		
		f->fu_symdata = (char *)xfu;

		fl = (funclist_t *)alloc(ap, sizeof(funclist_t));
		fl->fl_func = f;
		fl->fl_next = fil->fi_funclist;
		fil->fi_funclist = fl;

		nextf = f;
	}

	free((char *)filtab);
	
	*p_sfiles = nextfil;
	*p_funcs = nextf;
	
}

static int
xc_read_text(st, addr, buf, nbytes)
symtab_t *st;
taddr_t addr;
char *buf;
size_t nbytes;
{
	codefile_t *cf;
	textword_t *text;

	unpack_textloc(GET_ST_MACHINE(st), addr, &cf, &text);
	memcpy(buf, (char *)text, nbytes);
	return 0;
}

static int
xc_get_min_bpt_addr(f, p_addr)
func_t *f;
taddr_t *p_addr;
{
	xc_preamble_t *pr;
	machine_t *ma;
	codefile_t *cf, *fcf;
	textword_t *text;
	opcode_t opcode;

	if ((pr = GET_PREAMBLE(f)) != NULL) {
		*p_addr = f->fu_addr + pr->pr_bpt_offset;
		return 0;
	}

	ma = GET_ST_MACHINE(f->fu_symtab);
	cf = GET_XF(f->fu_fil)->xf_cf;

	if (!cf->cf_loaded && XLOAD(ma, cf) != 0)
		return -1;

	unpack_textloc(ma, f->fu_addr, &fcf, &text);
	if (fcf != cf)
		panic("cf botch in gbo");

	opcode = (opcode_t)*text;
	switch (opcode) {
	case OC_LINK_B:
	case OC_LINK_W:
	case OC_LINK_L:
	case OC_LINK_Q:
		break;
	default:
		panic("missing link in gbo");
	}

	pr = (xc_preamble_t *)alloc(f->fu_symtab->st_apool,
							sizeof(xc_preamble_t));
	pr->pr_bpt_offset = sizeof(*text) + ci_operand_size(opcode, text + 1);

	f->fu_predata = (char *)pr;

	*p_addr = f->fu_addr + pr->pr_bpt_offset;
	return 0;
}

static taddr_t
xc_get_addr_lim(f)
func_t *f;
{
	machine_t *ma;
	codefile_t *cf;

	if (f->fu_next != NULL && f->fu_fil == f->fu_next->fu_fil)
		return f->fu_next->fu_addr;

	ma = GET_ST_MACHINE(f->fu_symtab);
	cf = GET_XF(f->fu_fil)->xf_cf;

	return pack_textloc(ma, cf, cf->cf_text_and_data + cf->cf_text_size);
}

static bool
cc_fil_may_have_matching_globals(fil, pat, matchf)
fil_t *fil;
const char *pat;
matchfunc_t matchf;
{
	return TRUE;
}

static bool
xc_fil_may_have_matching_globals(fil, pat, matchf)
fil_t *fil;
const char *pat;
matchfunc_t matchf;
{
	codefile_t *cf;
	int i;

	cf = GET_XF(fil)->xf_cf;

	for (i = 0; i < cf->cf_nglobalnames; ++i)
		if ((*matchf)(cf->cf_globalnames[i], pat))
			return TRUE;

	return FALSE;
}

static void
xc_close_symtab_data(st)
symtab_t *st;
{
	errf("\bxc_close_symtab_data NYI");
}

static var_t *
xc_get_fi_vars(fil)
fil_t *fil;
{
	if ((fil->fi_flags & FI_DONE_VARS) == 0)
		load_symtab(fil);

	return fil->fi_block->bl_vars;
}

static macro_t *
xc_get_fi_macros(fil)
fil_t *fil;
{
	fil->fi_flags |= FI_DONE_MACROS;

	return fil->fi_macros;
}

static bool
xc_unwind(xp, st, fp, sp, pc, cfa)
target_t *xp;
symtab_t *st;
taddr_t *fp;
taddr_t *sp;
taddr_t *pc;
taddr_t *cfa;
{
	return FALSE;
}

static func_t *
xc_ensure_fu(f)
func_t *f;
{
	return f;
}

static fil_t *
xc_get_fi(st)
symtab_t *st;
{
	return st->st_sfiles;
}

static block_t *
xc_get_fu_blocks(f)
func_t *f;
{
	if ((f->fu_flags & FU_DONE_BLOCKS) == 0)
		load_symtab(f->fu_fil);

	return f->fu__blocks;
}

static int
load_symtab(fil)
fil_t *fil;
{
	machine_t *ma;
	char *buf;
	const char *iptr, *ilim, *strings;
	xc_lnum_t *xl, *xltab, *xllim;
	funclist_t *fl;
	symdesc_t *sd;
	int i, addr, lnum;
	size_t nbytes;
	o_syminfo_t *os;
	xc_fidata_t *xf;
	codefile_t *cf;
	alloc_pool_t *ap;

	if ((fil->fi_flags & (FI_DONE_VARS | FI_DONE_TYPES)) != 0)
		panic("dup call of ls");

	xf = GET_XF(fil);
	cf = xf->xf_cf;
	ma = GET_ST_MACHINE(fil->fi_symtab);
	ap = fil->fi_symtab->st_apool;

	if (cf->cf_path == NULL) {
		fil->fi_block = ci_make_block(ap, (block_t *)NULL);
		fil->fi_flags |= FI_DONE_VARS | FI_DONE_TYPES;
		return 0;
	}

	if (ci_read_symtab_bytes(ma->ma_apool, ma->ma_cwd,
							cf, &buf, &nbytes) != 0)
		return -1;

	os = (o_syminfo_t *)buf;
	iptr = buf + sizeof(o_syminfo_t);

	strings = iptr +
		  os->os_nfuncs * 3 * sizeof(size_t) +
		  os->os_nvars * sizeof(size_t) +
		  os->os_symdata_nbytes +
		  os->os_letab_nbytes;

	if (strings + os->os_strings_size != buf + nbytes) {
		errf("Bad symbol table size for %s (expected %ld, got %d)",
				cf->cf_path, (long)nbytes,
				(strings + os->os_strings_size) - buf);
		free(buf);
		return -1;
	}

	xltab = (xc_lnum_t *)alloc(ap, os->os_letab_count * sizeof(xc_lnum_t));

	/* Dibyendu - not sure if this is correct - but seems to work.
	 */
	xf->xf_letab = xltab;
	xf->xf_letab_size = os->os_letab_count;

	xl = xltab;
	fl = fil->fi_funclist;
	for (i = 0; i < os->os_nfuncs; ++i) {
		size_t words[3];
		xc_fudata_t *xfu;
		func_t *f;
		codefile_t *fcf;
		textword_t *faddr;

		memcpy((char *)words, iptr, sizeof(words));
		iptr += sizeof(words);

		f = fl->fl_func;

		if (strcmp(strings + words[0], f->fu_name) != 0)
			panic("funcname mismatch in ls");

		unpack_textloc(ma, f->fu_addr, &fcf, &faddr);
		if (fcf != cf || faddr - fcf->cf_text_and_data != words[1])
			panic("func mismatch in ls");

		xfu = GET_XFU(f);
		xfu->xfu_firstxl = xl;
		xfu->xfu_xllim = xl + words[2];

		xl = xfu->xfu_xllim;
		fl = fl->fl_next;
	}

	if (xl != xltab + os->os_letab_count)
		panic("xl count botch in ls");

	/*  Now decode the packed line number information into the xltab
	 *  array.  This duplicates the algorithm in unpack_letab() in
	 *  cx_link.c.
	 */
	ilim = iptr + os->os_letab_nbytes;

	xllim = xl;

	/*  Actually these assignments are only to keep gcc happy, because
	 *  cx_link.c guarantees to write an absolute entry for the start
	 *  of each function.
	 */
	addr = lnum = 0;

	for (xl = xltab; xl < xllim; ++xl) {
		unsigned short sval;
		int addr_delta, lnum_delta;
		bool negative;

		if (iptr >= ilim)
			panic("iptr botch in ls");

		memcpy((char *)&sval, iptr, sizeof(sval));
		iptr += sizeof(sval);

		lnum_delta = sval >> 9;
		negative = (sval & (1 << 8)) != 0;
		addr_delta = sval & 0xff;

		if (negative && addr_delta == 0) {
			size_t words[2];

			memcpy((char *)words, iptr, sizeof(words));
			iptr += sizeof(words);

			lnum = words[0];
			addr = words[1];
		}
		else {
			lnum += lnum_delta;
			addr += negative ? -addr_delta : addr_delta;
		}

		xl->xl_lnum = lnum;
		xl->xl_addr = addr;
	}

	if (iptr != ilim)
		panic("ilim botch in ls");

	/*  Skip the global name table (this was copied at link time).
	 */
	iptr += os->os_nvars * sizeof(size_t);

	sd = make_symdesc(ap, buf + os->os_types_offset, strings,
			  os->os_strings_size, os->os_ntypes, cf->cf_varaddrs,
			  (taddr_t)cf->cf_text_and_data + cf->cf_text_size, iptr);

	fil->fi_block = unpack_one_block(sd);
	fil->fi_flags |= FI_DONE_VARS | FI_DONE_TYPES;
	
	for (fl = fil->fi_funclist; fl != NULL; fl = fl->fl_next) {
		fl->fl_func->fu__blocks = unpack_one_block(sd);
		fl->fl_func->fu_flags |= FU_DONE_BLOCKS;
	}
	
	free(buf);
	free_symdesc(sd);

	return 0;
}

static lno_t *
xc_get_fu_lnos(f)
func_t *f;
{
	xc_fidata_t *xf;
	xc_fudata_t *xfu;
	xc_lnum_t *xl;
	lno_t *ln, *last;
	lno_t first;
	textword_t *text;
	machine_t *ma;

	if (f->fu_flags & FU_DONE_LNOS)
		return f->fu__lnos;
	f->fu_flags |= FU_DONE_LNOS;

	xf = GET_XF(f->fu_fil);
	xfu = GET_XFU(f);
	ma = GET_ST_MACHINE(f->fu_symtab);

	if ((f->fu_fil->fi_flags & FI_DONE_VARS) == 0)
		load_symtab(f->fu_fil);

	if (xf->xf_letab_size == 0) {
		f->fu__lnos = NULL;
		return NULL;
	}

	text = xf->xf_cf->cf_text_and_data;

	last = &first;
	for (xl = xfu->xfu_firstxl; xl < xfu->xfu_xllim; ++xl) {
		ln = (lno_t *)alloc(f->fu_symtab->st_apool, sizeof(lno_t));
		ln->ln_num = xl->xl_lnum;
		ln->ln_addr = pack_textloc(ma, xf->xf_cf, text + xl->xl_addr);
		ln->ln_fil = f->fu_fil;
		last->ln_next = ln;
		last = ln;
	}
	last->ln_next = NULL;
	f->fu__lnos = first.ln_next;

	return f->fu__lnos;
}

static const char *
xc_disassemble_instruction(f, addr, text, p_buf)
func_t *f;
taddr_t addr;
const char *text, **p_buf;
{
	panic("di NYI");
	return NULL;	/* to satisfy gcc */
}

static jump_t *
xc_get_jumps(f, addr, unused_text, len, want_calls)
func_t *f;
taddr_t addr;
const char *unused_text;
size_t len;
bool want_calls;
{
	static ebuf_t *eb = NULL;
	codefile_t *cf;
	textword_t *text, *lim;
	jump_t jbuf;
	machine_t *ma;

	eb = ebuf_start(eb, TRUE);

	ma = GET_ST_MACHINE(f->fu_symtab);
	unpack_textloc(ma, addr, &cf, &text);

	lim = text + len;

	while (text < lim) {
		opcode_t opcode;

		opcode = READOPCODE(text);

		switch (opcode) {
		case OC_JUMP:
		case OC_JUMP_IF_ZERO:
		case OC_JUMP_IF_NON_ZERO:
			jbuf.ju_type = JT_BRANCH;
			jbuf.ju_addr = pack_textloc(ma, cf, text - 1);
			jbuf.ju_dstaddr = pack_textloc(ma, cf,
						cf->cf_text_and_data + GETWORD(text));
			jbuf.ju_unconditional = opcode == OC_JUMP;
			break;

		case OC_RET:
		case OC_RET_WORD:
		case OC_RET_STRUCT:
		case OC_SWITCH_ON_TABLE:
		case OC_SWITCH_ON_CHAIN_B:
		case OC_SWITCH_ON_CHAIN_W:
		case OC_SWITCH_ON_CHAIN_L:
		case OC_SWITCH_ON_CHAIN_Q:
			jbuf.ju_type = JT_BRANCH;
			jbuf.ju_addr = pack_textloc(ma, cf, text - 1);
			jbuf.ju_dstaddr = 0;
			jbuf.ju_unconditional = FALSE;
			break;

		case OC_CALL_B:
		case OC_CALL_W:
		case OC_CALL_L:
		case OC_CALL_Q:
		case OC_CALL_INDIRECT:
			if (want_calls) {
				jbuf.ju_type = JT_CALL;
				jbuf.ju_addr = pack_textloc(ma, cf, text - 1);
				jbuf.ju_dstaddr = 0;
				jbuf.ju_unconditional = TRUE;
			}
			else {
				jbuf.ju_type = JT_END;
			}
			break;

		default:
			jbuf.ju_type = JT_END;
			break;
		}

		if (jbuf.ju_type != JT_END)
			ebuf_add(eb, (char *)&jbuf, sizeof(jump_t));

		text += ci_operand_size(opcode, text);
	}

	jbuf.ju_type = JT_END;
	jbuf.ju_addr = jbuf.ju_dstaddr = 0;
	ebuf_add(eb, (char *)&jbuf, sizeof(jbuf));

	return (jump_t *)ebuf_get(eb, (int *)NULL);
}
