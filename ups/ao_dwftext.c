/* ao_dwftext.c - DWARF implementations of sym_ops_t functions */

/*  Copyright 2002 Ian Edwards
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

char ups_ao_dwftext_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#if WANT_DWARF

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <mtrprog/hash.h>
#include <local/wn.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "st.h"
#include "ao_dwarf.h"
#include "ao_syms.h"
#include "ao_symscan.h"
#include "ao_dwfsyms.h"
#include "ao_dwftext.h"
#include "ao_dwfutil.h"
#include "ao_symcb.h"
#include "ao_text.h"
#include "cursors.h"
#include "ui.h"
#if WANT_DEBUG
#include "st_debug.h"
#endif


void display_message PROTO((const char *mesg));

int
dw_read_text(st, addr, buf, nbytes)
symtab_t *st;
taddr_t addr;
char *buf;
size_t nbytes;
{
    /* Original probably OK to use */
    return -1;
}

int
dw_get_min_bpt_addr(f, p_addr)
func_t *f;
taddr_t *p_addr;
{
    /* Original probably OK to use */
    return -1;
}

void
dw_close_symtab_data(st)
symtab_t *st;
{
    Dwarf_Error dw_err;
    dwarf_finish(AO_STDATA(st)->st_dw_dbg, &dw_err);
    dwarf_finish(AO_STDATA(st)->st_dw_debug_dbg, &dw_err);
    ao_close_symtab_data(st);
}

bool
dw_cblocks_match(data, newdata, same_func)
char *data, *newdata;
bool same_func;
{
    /* Original probably OK to use */
    return ao_cblocks_match(data, newdata, same_func);
}

void
dw_set_cblock_data(p_data, data, append)
char **p_data, *data;
bool append;
{
    /* Original probably OK to use */
    ao_set_cblock_data(p_data, data, append);
}

void
dw_free_cblock_data(data)
char *data;
{
    /* Original probably OK to use */
    ao_free_cblock_data(data);
}

bool
dw_cblock_has_var(data, name)
char *data;
const char *name;
{
    /* Original probably OK to use */
    return ao_cblock_has_var(data, name);
}

var_t *
dw_get_cblock_vars(st, data, addr) 
symtab_t *st;
char *data;
taddr_t addr;
{
    panic("dw_get_cblock_vars NYI");

    return NULL; /* Shut compiler up */
}

func_t *
dw_ensure_fu(f)
func_t *f;
{
    symtab_t *st = f->fu_symtab;
    ao_stdata_t *ast = AO_STDATA(st);
    func_t *flist;

    dwf_scan_symtab(st, NULL, NULL, &flist, NULL, ast->st_dw_debug_dbg);

    return f;
}

fil_t *
dw_get_fi(st)
symtab_t *st;
{
    ao_stdata_t *ast = AO_STDATA(st);
    func_t *flist;

    dwf_scan_symtab(st, NULL, NULL, &flist, NULL, ast->st_dw_debug_dbg);

    return st->st_sfiles;
}

/*
 * Attach information about its lines of source code to a function.
 */
lno_t *
dw_get_fu_lnos(f)
func_t *f;
{
    fsyminfo_t *fs;
    symtab_t *st;
    stf_t *stf;
    ao_stdata_t *ast;

    /*
     * Already done ?
     */
    if (f->fu_flags & FU_DONE_LNOS)
	return f->fu__lnos;

    st = f->fu_symtab;
    fs = AO_FSYMDATA(f);
    ast = AO_STDATA(st);

    /*
     * 'stf' may be one for an included file, we want the CU file
     */
    stf = AO_FIDATA(f->fu_fil);
    if (stf->stf_parent != NULL)
	stf = stf->stf_parent;

    /*
     * As we are given the information for a compilation unit (CU) we do the
     * work for all the functions in the CU rather than trying to do just the
     * single one we are interested in.
     */
    dwf_do_cu_lines(st, stf);

#if WANT_DEBUG
dump_header(f->fu_demangled_name);
dump_fil_t(f->fu_fil, FALSE, FALSE);
dump_lno_t(f->fu__lnos, TRUE);
dump_trailer();
#endif
    if (f->fu_flags & FU_DONE_LNOS)
	return f->fu__lnos;
    panic("botched dw_get_fu_lnos");
    return (lno_t *)NULL;
}

/*
 * Attach information about its lexical blocks to a function.
 * This includes information about the variables defined.
 * Based on ao_get_fu_blocks() in ao_symload.c
 */
block_t *
dw_get_fu_blocks(f)
func_t *f;
{
    fsyminfo_t *fs;
    symtab_t *st;
    stf_t *stf;
    errf_ofunc_t oldf;

    /*
     * Already done ?
     */
    if (f->fu_flags & FU_DONE_BLOCKS)
	return f->fu__blocks;

    oldf = errf_set_ofunc(display_message);
    errf("\bReading symbols of `%s'... ", f->fu_demangled_name);

    st = f->fu_symtab;
    fs = AO_FSYMDATA(f);
    stf = AO_FIDATA(f->fu_fil);

    /*
     * We want to use typedef names if possible for structures and
     * enums (see ci_basetype_name), so we need any type information
     * in the file.  Thus we load the file types.
     */
    dwf_do_cu_types(st, stf);

    /*
     * Get the information about lexical blocks.
     */
    dwf_do_fu_blocks(st, stf, f, fs);

    /*
     * Finish up.
     */
    /* fix_params(f); */ /* dwarfTODO: required ? */

    errf("\bReading symbols of `%s'... done", f->fu_demangled_name);
    errf_set_ofunc(oldf);
#if WANT_DEBUG
dump_header(f->fu_demangled_name);
dump_fil_t(f->fu_fil, FALSE, FALSE);
dump_block_t(f->fu__blocks, 0, TRUE);
dump_trailer();
#endif
    return f->fu__blocks;
}

/*
 * Load debugging information about file-scope variables.
 */
var_t *
dw_get_fi_vars(fil)
fil_t *fil;
{
    symtab_t *st;
    ao_stdata_t *ast;
    stf_t *stf;
/*    snlist_t *sn; */
    errf_ofunc_t oldf;
    cursor_t old_cursor;

    /*
     * Already done ?
     */
    if (fil->fi_flags & FI_DONE_VARS)
	return fil->fi_block->bl_vars;

    oldf = errf_set_ofunc(display_message);
    errf("\bReading symbols of `%s'... ", fil->fi_name_only);
    old_cursor = wn_get_window_cursor(WN_STDWIN);
    set_bm_cursor(WN_STDWIN, CU_WAIT);
    indicate_target_menu_search(1);

    st = fil->fi_symtab;
    ast = AO_STDATA(st);
    stf = AO_FIDATA(fil);

    /*
     * We want to use typedef names if possible.
     */
    dwf_do_cu_types(st, stf);

    /*
     * Work through the variable names noted 
     * dwarfTODO: why bother - just trawl the CU again.
     */
/*
    for (sn = stf->stf_snlist; sn != NULL; sn = sn->sn_next) {
    }
*/

    /*
     * Get the information about global variables.
     */
    dwf_do_cu_vars(st, stf);

    /*
     * Finish up.
     */
    fil->fi_flags |= FI_DONE_VARS;

    errf("\bReading symbols of `%s'... done", fil->fi_name_only);
    errf_set_ofunc(oldf);
    if (target_menu_search_disabled(0, 0) == FALSE) {
	bool stop = stop_pressed(0, 0);
	indicate_target_menu_search(0);
	if (stop)
	    stop_pressed(1, 0);
    }
    wn_define_cursor(WN_STDWIN, old_cursor);
#if WANT_DEBUG
dump_header(fil->fi_name_only);
dump_fil_t(fil, FALSE, FALSE);
dump_var_t(fil->fi_block->bl_vars, 0, TRUE);
dump_trailer();
#endif
    return fil->fi_block->bl_vars;
}

/*
 * Load debugging information about macros.
 */
macro_t *
dw_get_fi_macros(fil)
fil_t *fil;
{
    symtab_t *st;
    stf_t *stf;
    errf_ofunc_t oldf;
    cursor_t old_cursor;

    /*
     * Already done ?
     */
    if (fil->fi_flags & FI_DONE_MACROS)
	return fil->fi_macros;

    oldf = errf_set_ofunc(display_message);
    errf("\bReading macros of `%s'... ", fil->fi_name_only);
    old_cursor = wn_get_window_cursor(WN_STDWIN);
    set_bm_cursor(WN_STDWIN, CU_WAIT);
    indicate_target_menu_search(1);

    st = fil->fi_symtab;
    stf = AO_FIDATA(fil);

    /*
     * Get the information about macros.
     */
    dwf_do_cu_macros(st, stf);

    /*
     * Finish up.
     */
    fil->fi_flags |= FI_DONE_MACROS;

    errf("\bReading macros of `%s'... done", fil->fi_name_only);
    errf_set_ofunc(oldf);
    if (target_menu_search_disabled(0, 0) == FALSE) {
	bool stop = stop_pressed(0, 0);
	indicate_target_menu_search(0);
	if (stop)
	    stop_pressed(1, 0);
    }
    wn_define_cursor(WN_STDWIN, old_cursor);

    return fil->fi_macros;
}

/*
 * Unwind a stack frame.
 */
bool
dw_unwind(xp, st, fp, sp, pc, cfa)
target_t *xp;
symtab_t *st;
taddr_t *fp;
taddr_t *sp;
taddr_t *pc;
taddr_t *cfa;
{
    ao_stdata_t *ast = AO_STDATA(st);
    taddr_t adjusted_pc = *pc - ast->st_dw_base_address;
    taddr_t debug_adjusted_pc = *pc - ast->st_dw_debug_base_address;

    if (dwf_unwind(ast->st_dw_debug_dbg, xp, fp, sp, &debug_adjusted_pc, cfa)) {
	*pc = debug_adjusted_pc;
	return TRUE;
    } else if (dwf_unwind(ast->st_dw_dbg, xp, fp, sp, &adjusted_pc, cfa)) {
	*pc = adjusted_pc;
	return TRUE;
    }

    return FALSE;
}

#endif /* WANT_DWARF */
