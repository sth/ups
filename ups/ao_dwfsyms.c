/* ao_dwfsyms.c - DWARF implementations of ao_symscan functions */

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

char ups_ao_dwfsyms_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#if WANT_DWARF

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <mtrprog/hash.h>
#include <limits.h>
#include <stdlib.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "ci_util.h"
#include "st.h"
#include "ao_dwarf.h"
#include "ao_syms.h"
#include "ao_symscan.h"
#include "ao_dwfname.h"
#include "ao_dwfsyms.h"
#include "ao_dwftype.h"
#include "ao_dwfutil.h"
#include "ui.h"
#if WANT_DEBUG
#include "st_debug.h"
#endif


static lno_t *
penultimate_lno PROTO((lno_t *lno));
static snlist_t *
dwf_add_globalvar_to_symtab PROTO((symtab_t *st, stf_t *stf, const char *name));
static func_t *
dwf_tag_subprogram PROTO((Dwarf_Debug dbg, Dwarf_Die die, int dw_level,
			  symtab_t *st, stf_t *stf, func_t **p_flist));
static void
dwf_load_globals_from_cu PROTO((symtab_t *st, stf_t *stf, func_t **p_flist,
				Dwarf_Die cu_die, Dwarf_Off cu_hdr_offset,
				Dwarf_Signed global_count,
				Dwarf_Global *globals));
static bool
dwf_load_from_die PROTO((symtab_t *st, stf_t *stf, func_t **p_flist, func_t *f,
			 block_t **p_blocklist, block_t *parent_bl,
			 dtype_t *parent_dt, Dwarf_Die parent_die,
			 dwload_t dw_what, int dw_level, int recursed));

/*
 * Find last but one 'lno' in the list.
 */
static lno_t *
penultimate_lno(lno)
lno_t *lno;
{
    if ((lno == NULL) || (lno->ln_next == NULL))
	return (lno_t *)NULL;
    while (lno->ln_next->ln_next != NULL)
	lno = lno->ln_next;
    return lno;
}

/*
 * Process DWARF line number information for the CU of this 'stf'.
 *
 * As we are given the information for a compilation unit (CU) we do the
 * work for all the functions in the CU rather than trying to do just the
 * single one we are interested in.
 */
void
dwf_do_cu_lines(st, stf)
symtab_t *st;
stf_t *stf;
{
    int rv;
    int i;
    Dwarf_Debug dbg;
    Dwarf_Error err;
    Dwarf_Die cu_die;
    Dwarf_Line *lines;
    Dwarf_Signed count;
    Dwarf_Addr addr = 0, prev_addr = ~0;
    Dwarf_Unsigned ln_num;
    Dwarf_Signed ln_col;
    char *name;
    funclist_t *fl;
    func_t *f;
    lno_t *lno = NULL;


    dbg = stf->stf_dw_dbg;
    cu_die = dwf_stf_cu_die(stf);

    /*
     * Work through the source line info.  Start at end so we can push the
     * 'lno' at the head of the list (saves having to reverse it later).
     * This assumes the information is is ascending address order.
     *
     * dwarf_lineoff() gives column statement starts at or -1 if not set.
     * (GCC does not set it)
     *
     * Use DW_TAG_lexical_block DIEs (elsewhere) rather than dwarf_lineblock()
     * to get basic block info. (GCC does not set it)
     */
    if ((rv = dwarf_srclines(cu_die, &lines, &count, &err)) != DW_DLV_OK)
	dwf_fatal_error("dwarf_srclines", rv, cu_die, err);
    for (i = count-1; i >= 0; i--) {

	if ((rv = dwarf_lineaddr(lines[i], &addr, &err)) != DW_DLV_OK)
	    dwf_fatal_error("dwarf_lineaddr", rv, cu_die, err);
	if ((rv = dwarf_lineno(lines[i], &ln_num, &err)) != DW_DLV_OK)
	    dwf_fatal_error("dwarf_lineno", rv, cu_die, err);
	if ((rv = dwarf_lineoff(lines[i], &ln_col, &err)) != DW_DLV_OK)
	    dwf_fatal_error("dwarf_lineoff", rv, cu_die, err);
	if ((rv = dwarf_linesrc(lines[i], &name, &err)) != DW_DLV_OK)
	    dwf_fatal_error("dwarf_linesrc", rv, cu_die, err);

	if (addr > prev_addr)
	    errf("dwf_do_cu_lines: Address went up - 0x%lx to 0x%lx",
		 (long)addr, (long)prev_addr);
	prev_addr = addr;

	if ((f = dwf_lookup_func_by_addr(stf, addr)) == NULL) {
	    errf("\bAddress 0x%lx not found in %s", (long)addr, name);
	    dwarf_dealloc(dbg, name, DW_DLA_STRING);
	    dwarf_dealloc(dbg, lines[i], DW_DLA_LINE);
	    lines[i] = (Dwarf_Line)0;
	    continue;
	}
	if (f->fu_flags & FU_DONE_LNOS)
	    panic("dwf_do_cu_lines: Already done lnos");

	dwarf_dealloc(dbg, name, DW_DLA_STRING);
	dwarf_dealloc(dbg, lines[i], DW_DLA_LINE);
	lines[i] = (Dwarf_Line)0;

	lno = (lno_t *)alloc(st->st_apool, sizeof(lno_t));
	lno->ln_fil = f->fu_fil;
	lno->ln_col = ln_col;
	lno->ln_num = ln_num;
	lno->ln_addr = (taddr_t)addr + stf->stf_addr;
	lno->ln_next = f->fu__lnos;
	f->fu__lnos = lno;
	if (f->fu_max_lnum < ln_num)
	    f->fu_max_lnum = ln_num;
    }
    dwarf_dealloc(dbg, lines, DW_DLA_LIST);

    /*
     * Now fix things : we get entries for the opening '{' and closing '}'
     * which we don't really want.
     */
    for (i = 0; i < stf->stf_mapsize; i++) {
	fl = stf->stf_fmap[i]->hf_stf->stf_fil->fi_funclist;
	while (fl != NULL) {

	    f = fl->fl_func;
	    if (! (f->fu_flags & FU_DONE_LNOS)) {
		/*
		 * The first 'lno' may be the function preamble.
		 * Unless it was the only one, start at the next.
		 */
		lno = f->fu__lnos;
		if (lno && lno->ln_next && (lno->ln_addr <= f->fu_addr))
		    f->fu__lnos = lno->ln_next;

		/*
		 * dwarfTODO: better test for dropping last 'lno'; this
		 * is too conservative.
		 */
		lno = penultimate_lno(f->fu__lnos);
		if (lno && (lno != f->fu__lnos)
			&& (lno->ln_next->ln_addr >= AO_FSYMDATA(f)->fs_high_pc))
		    lno->ln_next = NULL;

		f->fu_flags |= FU_DONE_LNOS;
	    }
	    fl = fl->fl_next;
	}
    }
}

/*
 * DWARF version of push_symname (see ao_symscan.c)
 */
static snlist_t *
dwf_add_globalvar_to_symtab(st, stf, name)
symtab_t *st;
stf_t *stf;
const char *name;
{
    snlist_t *sn;

    /*
     * Make a snlist_t
     */
    sn = (snlist_t *)alloc(st->st_apool, sizeof(snlist_t));
    sn->sn_symtab_name = alloc_strdup(st->st_apool, name);
    sn->sn_name = alloc_strdup(st->st_apool, name);
    sn->sn_symno = 0;

    /*
     * Link the snlist_t into the stf, and return it.
     */
    sn->sn_next = stf->stf_snlist;
    stf->stf_snlist = sn;
    return sn;
}

/*
 * DWARF version of add_function_to_symtab (see ao_symscan.c)
 */
func_t *
dwf_add_function_to_symtab(st, stf, p_flist, name, is_static, die, spec_die)
symtab_t *st;
stf_t *stf;
func_t **p_flist;
const char *name;	/* Name, may be mangled. */
bool is_static;
Dwarf_Die die;		/* DIE holding routine instance details. */
Dwarf_Die spec_die;	/* DIE holding routine specification. */
{
    Dwarf_Debug dbg;
    Dwarf_Die addr_die, decl_die;
    fsyminfo_t *fs;
    stf_t *hstf;
    fil_t *fil = NULL;
    func_t *f;
    taddr_t addr;
    off_t type_offset;
    dtype_t *dt;

    dbg = stf->stf_dw_dbg;

    /*
     * Determine which DIE has the addresses and which the file number.
     */
    if (dwf_has_attribute(dbg, die, DW_AT_low_pc))
	addr_die = die;
    else
	addr_die = spec_die;
    if ((decl_die = dwf_find_decl_die(dbg, die)) == (Dwarf_Die)0)
	decl_die = die;

    /*
     * Find the file where the routine is defined so it can
     * be added to its list of routines.  Artificial routines
     * will not have a file.
     */
    if ((hstf = dwf_lookup_file_by_die(stf, decl_die)) != NULL)
	fil = hstf->stf_fil;

    fs = make_fsyminfo(st->st_apool, 0);
    fs->fs_symlim = 0;
    fs->fs_initial_lno_fil = fil;
    /* These used to process line number data later. */
    fs->fs_low_pc = dwf_get_address(dbg, addr_die, DW_AT_low_pc);
    fs->fs_high_pc = dwf_get_address(dbg, addr_die, DW_AT_high_pc);
    fs->fs_die_offset = dwf_offset_of_die(dbg, addr_die);

    addr = fs->fs_low_pc;

    f = ci_make_func(st->st_apool, name, addr, st, fil, *p_flist);
    f->fu_symdata = (char *)fs;
    f->fu_max_lnum = 0;
    f->fu_lexinfo = dwf_make_lexinfo(dbg, decl_die, st->st_apool, stf);
    if (is_static)
	f->fu_flags |= FU_STATIC;

    /*
     * If can find the type then use it, else save its offset
     * for dwf_fixup_types() to sort out.
     */
    if (dwf_has_attribute(dbg, spec_die, DW_AT_type)) {
        type_offset = dwf_get_cu_ref(dbg, spec_die, DW_AT_type);
        if ((f->fu_type = dwf_find_type(stf, type_offset)) == NULL) {
            dt = dwf_make_dtype(dbg, die, st->st_apool, stf, DT_IS_VAR, NULL, NULL);
            dt->dt_base_offset = type_offset;
            dt->dt_p_type = &f->fu_type;
        }
    }
    
    /*
     * Add to the list of functions defined in the file.
     */
    if (fil == NULL)
	f->fu_flags |= FU_NOSYM | FU_DONE_BLOCKS | FU_DONE_LNOS;
    else
	add_to_fil_funclist(st->st_apool, hstf->stf_fil, f);

    *p_flist = f;
    return f;
}

/*
 * Handle DW_TAG_subprogram
 */
static func_t *
dwf_tag_subprogram(dbg, die, dw_level, st, stf, p_flist)
Dwarf_Debug dbg;
Dwarf_Die die;
int dw_level;
symtab_t *st;
stf_t *stf;
func_t **p_flist;
{
    func_t *f;
    char *name = NULL, *mangled = NULL;
    bool is_static;
    Dwarf_Die spec_die;
    alloc_pool_t *ap;

    /*
     * If there is an address it is the definition,
     * otherwise just a declaration.
     */
    if (! dwf_has_attribute(dbg, die, DW_AT_low_pc))
	return (func_t *)NULL;

    ap = st->st_apool;

    /*
     * dwarfTODO: do we need to save any info about inline functions ?
     */
    if (dwf_has_attribute(dbg, die, DW_AT_abstract_origin)) {
    }

    /*
     * Find the DIE with the routine specification.
     */
    spec_die = dwf_find_spec_die(dbg, die);
    dwf_get_name(dbg, ap, spec_die, &name, &mangled);
    is_static = !dwf_get_opt_flag(dbg, spec_die, DW_AT_external);

    /*
     * If there is a mangled name then use it.
     * ci_make_func() will demangle it.
     */
    f = dwf_add_function_to_symtab(st, stf, p_flist, (mangled ? mangled : name),
				   is_static, die, spec_die);
    return f;
}

/*
 * Load the globals that are in the CU into UPS structures
 *
 * dwarfTODO: not currently used - UPS currently wants all functions,
 * not just globals, loaded on startup.
 */
static void
dwf_load_globals_from_cu(st, stf, p_flist, cu_die, cu_hdr_offset, global_count, globals)
symtab_t *st;
stf_t *stf;
func_t **p_flist;
Dwarf_Die cu_die;
Dwarf_Off cu_hdr_offset;
Dwarf_Signed global_count;
Dwarf_Global *globals;
{
    int rv, i;
    Dwarf_Error err;
    Dwarf_Debug dbg;
    Dwarf_Off offset, die_offset;
    Dwarf_Die die;
    Dwarf_Half tag;
    snlist_t *sn;
    alloc_pool_t *ap;
    char *global_name;

    /*
     * Initalise
     */
    ap = st->st_apool;
    dbg = stf->stf_dw_dbg;
    die = (Dwarf_Die)0;

    /*
     * Look for globals that are in this CU.
     * TODO : this is done by trawling the globals list looking for those with
     * the same CU offset.  Probably not very efficient.
     */
    for (i = 0; i < global_count; i++) {

	/* Already done ? */
	if (globals[i] == (Dwarf_Global)0)
	    continue;

	/* Is global in the CU ? */
	if ((rv = dwarf_global_cu_offset(globals[i], &offset, &err)) != DW_DLV_OK)
	    dwf_fatal_error("dwarf_global_cu_offset", rv, die, err);
	if (offset != cu_hdr_offset)
	    continue;

	/* Get the DIE and the name. */
	if ((rv = dwarf_global_die_offset(globals[i], &die_offset, &err)) != DW_DLV_OK)
	    dwf_fatal_error("dwarf_global_dieoffset", rv, die, err);
	if ((rv = dwarf_globname(globals[i], &global_name, &err)) != DW_DLV_OK)
	    dwf_fatal_error("dwarf_globname", rv, die, err);

	/* What sort of global : function, variable, ... ? */
	die = dwf_die_at_offset(dbg, die_offset);
	tag = dwf_get_tag(dbg, die);

	switch (tag) {
	case DW_TAG_subprogram:
	    dwf_tag_subprogram(dbg, die, 1, st, stf, p_flist);
	    break;

	case DW_TAG_variable:
	    sn = dwf_add_globalvar_to_symtab(st, stf, strdup(global_name));
	    break;

	default:
	    errf("DWARF global '%s' has unknown tag %d\n", global_name, tag);
	    break;
	}

	/* Done with this global. */
	dwarf_dealloc(dbg, global_name, DW_DLA_STRING);
	dwarf_dealloc(dbg, globals[i], DW_DLA_GLOBAL);
	globals[i] = (Dwarf_Global)0;

    } /* for each global */
}

/*
 * Load debugging information from the specified DIE.
 *
 * The 'dw_what' bit pattern controls what gets processed and whether
 * we make recursive calls to process child DIEs or not.
 */
static bool
dwf_load_from_die(st, stf, p_flist, f, p_blocklist, parent_bl,
		  parent_dt, parent_die, dw_what, dw_level, recursed)
symtab_t *st;
stf_t *stf;
func_t **p_flist;	/* DWL_xxx_FUNCS */
func_t *f;		/* DWL_LEXICAL_BLOCKS */
block_t **p_blocklist;	/* DWL_LEXICAL_BLOCKS */
block_t *parent_bl;	/* DWL_LEXICAL_BLOCKS */
dtype_t *parent_dt;
Dwarf_Die parent_die;
dwload_t dw_what;
int dw_level;		/* DWARF level, 1 = top */
int recursed;		/* Recursion level, 0 = top. */
{
    int i;
    Dwarf_Debug dbg;
    long rel_offset;
    Dwarf_Die die, spec_die;
    Dwarf_Half tag;
    snlist_t *sn;
    block_t *bl_head = NULL;
    aggr_or_enum_def_t *ae;
    alloc_pool_t *ap;
    dtype_t *dt;
    type_t *type;
    bool is_static;
    char *name;
    int count = 0;
    dwload_t dw_what_next;
    bool ok;
    bool descend;

    /*
     * Initalise
     */
    ap = st->st_apool;
    dbg = stf->stf_dw_dbg;

    /*
     * Work through the child DIEs
     */
    die = dwf_child_die(dbg, parent_die);
    while (die != (Dwarf_Die)0) {

	/* dwarfTODO: Allow user to stop this. */
	count++;
	if ((count % 100) == 0) {
	    if (!stop_pressed(0, 0) && user_wants_stop(TRUE))
		stop_pressed(1, 0);	/* set */
	}

	/* Offset in CU is very useful when debugging. */
	rel_offset = (long)dwf_cu_offset_of_die(dbg, die);

	/*
	 * What sort of item : function, variable, ... ?
	 */
	ok = FALSE;
	descend = FALSE;
        dw_what_next = dw_what; /* in most cases */
	dt = NULL;
	tag = dwf_get_tag(dbg, die);
	switch (tag) {

	case DW_TAG_compile_unit:
	    break;

	case DW_TAG_subprogram:
	    if ((dw_level == 1) && (dw_what & DWL_TOP_FUNCS)) {
		/*
		 * Top level function
		 */
		f = dwf_tag_subprogram(dbg, die, dw_level, st, stf, p_flist);

	    } else if ((dw_level > 1) && (dw_what & DWL_NESTED_FUNCS)) {
		/*
		 * dwarfTODO: nested functions ??
		 */
		descend = TRUE;
	    }
	    break;

	case DW_TAG_formal_parameter:
	    if (dw_what & DWL_ANY_VARS) {
		/*
		 * Routine parameter
		 */
		dwf_make_variable(dbg, die, ap, stf, &(parent_bl->bl_vars),
				  dw_level, CL_ARG);
	    }
	    break;

	case DW_TAG_lexical_block:
	    if (dw_what & DWL_LEXICAL_BLOCKS) {
		block_t *bl;
		/*
		 * Start of a lexical block.
		 * Make a new block and trawl the DIEs in it.
		 */
		bl = dwf_make_block(dbg, die, ap, stf, parent_bl, f);
		bl->bl_next = bl_head;
		bl_head = bl;

		parent_bl = bl_head;
		descend = TRUE;
	    }
	    break;

	case DW_TAG_entry_point:
	    break;

	case DW_TAG_variable:
	    if ((dw_level == 1) && (dw_what & DWL_SKIM_VARS)) {
		/*
		 * If there is an address it is the definition,
		 * otherwise just a declaration.
		 */
		if (dwf_has_attribute(dbg, die, DW_AT_location)) {
		    /*
		     * Add globals and static with global scope in current CU.
		     */
		    spec_die = dwf_find_spec_die(dbg, die);
		    is_static = !dwf_get_opt_flag(dbg, spec_die, DW_AT_external);
		    name = dwf_get_string(dbg, ap, spec_die, DW_AT_name);
		    sn = dwf_add_globalvar_to_symtab(st, stf, name);
		}
	    } else if ((dw_level == 1) && (dw_what & DWL_TOP_VARS)) {
		/*
		 * dwarfTODO:
		 */
		dwf_make_variable(dbg, die, ap, stf, &(parent_bl->bl_vars),
				  dw_level, CL_AUTO);
	    } else if ((dw_level > 1) && (dw_what & DWL_LOCAL_VARS)) {
		/*
		 * dwarfTODO:
		 */
		dwf_make_variable(dbg, die, ap, stf, &(parent_bl->bl_vars),
				  dw_level, CL_AUTO);
	    }
	    break;

	case DW_TAG_base_type:
	    if (dw_what & DWL_BASE_TYPES) {
		typecode_t typecode;
		int nbytes;

		/*
		 * Basic type - these can be more than just the
		 * standard 'C' basic types, e.g. "fpos_t".
		 * Extract the infomation and make a type.
		 */
		name = dwf_get_string(dbg, ap, die, DW_AT_name);
		nbytes = dwf_get_number(dbg, die, DW_AT_byte_size);
		typecode = dwf_get_typecode(dbg, die);

		type = ci_make_type(ap, typecode);
		type->ty_size = nbytes;
		type->ty_name = name;
		type->ty_lexinfo = dwf_make_lexinfo(dbg, die, ap, stf);

		if (type->ty_code == TY_BOOLEAN)
		    ci_set_sizeof_bool(type->ty_size);

		/*
		 * Save it.
		 */
		dt = dwf_make_dtype(dbg, die, ap, stf, DT_IS_TYPE, NULL, type);
	    }
	    break;

	case DW_TAG_typedef:
	    if (dw_what & DWL_ANY_TYPES) {
		/*
		 * typedef - save it.
		 */
		dt = dwf_make_typedef(dbg, die, ap, stf, parent_bl);
	    }
	    break;

	case DW_TAG_array_type:
	    if (dw_what & DWL_ANY_TYPES) {
		/*
		 * Array of ...
		 * Need to descend to get array dimensions.
		 */
		dt = dwf_make_type(dbg, die, ap, stf, DT_ARRAY_OF);

		dw_what_next = DWL_SUBRANGES;
		descend = TRUE;
	    }
	    break;

	case DW_TAG_subrange_type:
	    if (dw_what & DWL_SUBRANGES) {
		dim_t *dim;
		/*
		 * Array dimensions.
		 */
		dim = dwf_make_subrange(dbg, die, ap, stf);

		if (parent_dt->dt_type->ty_dim) {
		    type = ci_make_type(ap, DT_ARRAY_OF);

		    type->ty_base = parent_dt->dt_type;
		    type->ty_dim = type->ty_base->ty_dim;
		    type->ty_base->ty_dim = dim;

		    parent_dt->dt_type = type;
		}
		else {
		    parent_dt->dt_type->ty_dim = dim;
		}
	    }
	    break;

	case DW_TAG_pointer_type:
	    if (dw_what & DWL_ANY_TYPES) {
		/*
		 * Pointer to ...
		 */
		dt = dwf_make_type(dbg, die, ap, stf, DT_PTR_TO);
	    }
	    break;

	case DW_TAG_reference_type:
	    if (dw_what & DWL_ANY_TYPES) {
		/*
		 * Reference to ...
		 * dwarfTODO: implement DT_REF_TO ?
		 */
		dt = dwf_make_type(dbg, die, ap, stf, DT_PTR_TO);
	    }
	    break;

	case DW_TAG_file_type:
	    break;

	case DW_TAG_set_type:
	    break;

	case DW_TAG_string_type:
	    break;

	case DW_TAG_subroutine_type:
	    if (dw_what & DWL_GLOBAL_TYPES) {
		/*
		 * Function returning ...
		 */
		dt = dwf_make_type(dbg, die, ap, stf, DT_FUNC_RETURNING);

		/*
		 * dwarfTODO: we could get the information for 'funcret' ...
		 */
		dt->dt_type->ty_funcret = ci_make_funcret(ap, FDT_IDLIST, 0, FALSE);
	    }
	    break;

	case DW_TAG_thrown_type:
	    break;

	case DW_TAG_const_type:
	case DW_TAG_volatile_type:
	case DW_TAG_packed_type:
	    if (dw_what & DWL_GLOBAL_TYPES) {
		/*
		 * Make a dummy type, dwf_fixup_types() fixes things.
		 */
		dt = dwf_make_type(dbg, die, ap, stf, TY_NOTYPE);
		if (tag == DW_TAG_volatile_type)
		    dt->dt_type->ty_qualifiers |= QU_VOLATILE;
		else if (tag == DW_TAG_packed_type)
		    dt->dt_type->ty_qualifiers |= QU_PACKED;
		else
		    dt->dt_type->ty_qualifiers |= QU_CONST;
	    }
	    break;

	case DW_TAG_member:
	    if (dw_what & DWL_STRUCT_MEMBERS) {
		var_t *v;
		class_t hint;
		/*
		 * Member of structure or union
		 */
		ae = parent_dt->dt_type->ty_aggr_or_enum;
		if (parent_dt->dt_type->ty_code == TY_UNION)
		    hint = CL_MOU;
		else
		    hint = CL_MOS;
		v = dwf_make_variable(dbg, die, ap, stf, &(ae->ae_aggr_members),
				      dw_level, hint);
		if (dwf_has_attribute(dbg, die, DW_AT_artificial)) {
		    /*
		     * GCC virtual function table ?
		     */
		    if (v->va_name && (strncmp (v->va_name, "_vptr$", 6) == 0))
			v->va_flags = VA_VTBLPTR;
		}
	    }
	    break;

	case DW_TAG_inheritance:
	    if (dw_what & DWL_CLASS_MEMBERS) {
		var_t *v;
		/*
		 * A base class the current class inherits from.
		 * Base class name set in dwf_finish_class().
		 */
		ae = parent_dt->dt_type->ty_aggr_or_enum;
		v = dwf_make_variable(dbg, die, ap, stf, &(ae->ae_aggr_members),
				      dw_level, CL_MOS);
		v->va_flags = VA_BASECLASS;
	    }
	    break;

	case DW_TAG_enumerator:
	    if (dw_what & DWL_ENUM_MEMBERS) {
		enum_member_t *em;
		/*
		 * Enumeration member
		 */
		ae = parent_dt->dt_type->ty_aggr_or_enum;
		em = dwf_make_enumerator(dbg, die, ap, stf, ae);
	    }
	    break;

	case DW_TAG_class_type:
	    /*
	     * GCC seems to use DW_TAG_structure_type :-(
	     */
	    /* DROPTHROUGH */

	case DW_TAG_structure_type:
	    if (dw_what & DWL_SKIM_TYPES) {
		/*
		 * If it looks like a definition, save it.
		 */
		if (! dwf_has_attribute(dbg, die, DW_AT_declaration))
		    dwf_save_typename(dbg, die, st, stf);

	    } else if (dw_what & DWL_ANY_TYPES) {
		/*
		 * Structure type (class/struct)
		 * Is it a definition or just a declaration ?
		 */
		if (dwf_has_attribute(dbg, die, DW_AT_declaration)) {
		    /*
		     * Just a declaration - check to see if we have seen a
		     * definition elsewhere.
		     */
		    if ((type = dwf_find_alt_type_defn(dbg, die, stf)) == NULL) {
			/*
			 * No definition - make an 'undefined struct'
			 */
			dt = dwf_make_struct_type(dbg, die, ap, stf, TY_U_STRUCT, parent_bl);
		    } else {
			/*
			 * Have a definition - use it.
			 */
			dt = dwf_make_dtype(dbg, die, ap, stf, DT_IS_TYPE, NULL, type);
		    }

		    /*
		     * Determine which child DIEs we need to process.
		     * For C++ the "struct" may really be a class,
		     * and contain type definitions, which can get referenced
		     * from a higher level, so we have to descend :-(
		     *
		     * It seems that gcc 3.2.2 also embeds types inside
		     * structures in C mode, so we always load the types.
		     */
		    dw_what_next = DWL_ANY_TYPES;
		    descend = TRUE;

		} else {
		    /*
		     * A definition - dwf_finish_aggregate() will change
		     * the typecode if it turns out to have no members.
		     */

		    /*
		     * Determine which child DIEs we need to process.
		     * For C++ the "struct" may really be a "class",
		     * and contain type definitions.
		     *
		     * It seems that gcc 3.2.2 also embeds types inside
		     * structures in C mode, so we always load the types.
		     */
		    dw_what_next = DWL_STRUCT_MEMBERS | DWL_ANY_TYPES;
		    if (stf->stf_language == LANG_CC)
			dw_what_next |= DWL_CLASS_MEMBERS;
		    descend = TRUE;

		    dt = dwf_make_struct_type(dbg, die, ap, stf, TY_STRUCT, parent_bl);
		}
	    }
	    break;

	case DW_TAG_union_type:
	    if (dw_what & DWL_ANY_TYPES) {
		/*
		 * Union type.
		 * Determine which child DIEs we need to process.
		 */
		dw_what_next = DWL_STRUCT_MEMBERS;
		descend = TRUE;

		dt = dwf_make_ae_type(dbg, die, ap, stf, TY_UNION, parent_bl);
	    }
	    break;

	case DW_TAG_enumeration_type:
	    if (dw_what & DWL_ANY_TYPES) {
		/*
		 * Enumerated type.
		 * Determine which child DIEs we need to process.
		 */
		dw_what_next = DWL_ENUM_MEMBERS;
		descend = TRUE;

		dt = dwf_make_ae_type(dbg, die, ap, stf, TY_ENUM, parent_bl);
	    }
	    break;

	case DW_TAG_ptr_to_member_type:
	    break;

	case DW_TAG_restrict_type:	/* DWARF 3 */
	case DW_TAG_interface_type:	/* DWARF 3 */
	case DW_TAG_unspecified_type:	/* DWARF 3 */
	case DW_TAG_mutable_type:	/* DWARF 3 */
	    break;

	} /* switch (tag) */

	/*
	 * Process child DIEs ?
	 */
	if (descend)
	    dwf_load_from_die(st, stf, p_flist, f,
			      &(parent_bl->bl_blocks), parent_bl,
			      dt, die, dw_what_next, dw_level+1, recursed+1);

	/* Next DIE at this level. */
	die = dwf_next_die(dbg, die);

    } /* while another DIE */

    /*
     * If doing class/structure/union/enum members the type should
     * now be complete.
     */
    if (dw_what & DWL_AE_MEMBERS) {
	dwf_finish_aggregate(parent_dt);
    }

    /*
     * If doing types then fix forward references now,
     * Also deal with any dummy types that were created
     * for qualified types and typedefs.
     */
    if (dw_what & DWL_ANY_TYPES) {
	i = dwf_fixup_types(stf->stf_dtypes, recursed);
	if ((i > 0) && (recursed == 0))
	    errf("\b%d incomplete DWARF types", i);
    }

    /*
     * If doing class members ...
     */
    if (dw_what & DWL_CLASS_MEMBERS) {
	dwf_finish_class(parent_dt);
    }

    /*
     * Save the block list ?
     */
    if (dw_what & DWL_LEXICAL_BLOCKS) {
	if (p_blocklist)
	    *p_blocklist = bl_head;
    }

    return TRUE;
}

/*
 * Load global type information for the CU of this 'stf'.
 */
void
dwf_do_cu_types(st, stf)
symtab_t *st;
stf_t *stf;
{
    Dwarf_Die cu_die;
    int dw_level;
    dwload_t dw_what;

    /*
     * Already done or being done ?
     */
    if (stf->stf_fil->fi_flags & FI_DONE_TYPES)
	return;
    if (stf->stf_fil->fi_flags & FI_DOING_TYPES)
	return;
    stf->stf_fil->fi_flags |= FI_DOING_TYPES;

    /*
     * Initalise
     */
    dw_level = 1;	/* Follow 'dwarfdump' comvention. */
    dw_what = DWL_BASE_TYPES | DWL_GLOBAL_TYPES;
    cu_die = dwf_stf_cu_die(stf);

    /*
     * Trawl the DIEs
     */
    dwf_load_from_die(st, stf, (func_t **)NULL, (func_t *)NULL,
		      (block_t **)NULL, stf->stf_fil->fi_block,
		      (dtype_t *)NULL, cu_die, dw_what, dw_level, 0);

    stf->stf_fil->fi_flags &= ~FI_DOING_TYPES;
    stf->stf_fil->fi_flags |= FI_DONE_TYPES;
#if WANT_DEBUG
dump_header(stf->stf_fil->fi_name);
dump_dtype_t(stf->stf_dtypes, 0, TRUE);
dump_typedef_t(stf->stf_fil->fi_block->bl_typedefs, 0, TRUE);
dump_trailer();
#endif
}

/*
 * Load global variable information for the CU of this 'stf'.
 */
void
dwf_do_cu_vars(st, stf)
symtab_t *st;
stf_t *stf;
{
    Dwarf_Die cu_die;
    int dw_level;
    dwload_t dw_what;

    /*
     * Done ?
     */
    if (stf->stf_fil->fi_flags & FI_DONE_VARS)
	return;

    /*
     * Initalise
     */
    dw_level = 1;	/* Follow 'dwarfdump' comvention. */
    dw_what = DWL_TOP_VARS;
    cu_die = dwf_stf_cu_die(stf);

    /*
     * Trawl the DIEs
     */
    dwf_load_from_die(st, stf, (func_t **)NULL, (func_t *)NULL,
		      (block_t **)NULL, stf->stf_fil->fi_block,
		      (dtype_t *)NULL, cu_die, dw_what, dw_level, 0);

    stf->stf_fil->fi_flags |= FI_DONE_VARS;
}

/*
 * Load lexical block information for a function.
 * Also loads local types and variables for the function.
 */
void
dwf_do_fu_blocks(st, stf, f, fs)
symtab_t *st;
stf_t *stf;
func_t *f;
fsyminfo_t *fs;
{
    Dwarf_Debug dbg;
    Dwarf_Die die;
    int dw_level;
    dwload_t dw_what;
    block_t *bl_head;

    /*
     * Initalise
     */
    dbg = stf->stf_dw_dbg;
    dw_level = 2;	/* Assume function at DWARF level 2. */
    dw_what = DWL_LEXICAL_BLOCKS | DWL_LOCAL_TYPES | DWL_LOCAL_VARS;

    /*
     * Get the DIE for this function
     */
    die = dwf_die_at_offset(dbg, fs->fs_die_offset);

    /*
     * Make the block for the function.
     * (dwf_load_from_die() processes the children of the DIE)
     */
    bl_head = dwf_make_block(dbg, die, st->st_apool, stf, f->fu_fil->fi_block, f);

    /*
     * Add file-level types here so "add expr" can reference them.
     */
    if (f->fu_fil->fi_block) {
	bl_head->bl_aggr_or_enum_defs = f->fu_fil->fi_block->bl_aggr_or_enum_defs;
	bl_head->bl_typedefs = f->fu_fil->fi_block->bl_typedefs;
    }

    /*
     * Trawl the DIEs
     */
    dwf_load_from_die(st, stf, (func_t **)NULL, f,
		      &(bl_head->bl_blocks), bl_head,
		      (dtype_t *)NULL, die, dw_what, dw_level, 0);

    /*
     * Finish up.
     */
    f->fu__blocks = bl_head;
    f->fu_flags |= FU_DONE_BLOCKS;
}

/*
 * Load the symbols that are in the CU into UPS structures.
 * This just loads "interesting" stuff such as functions and
 * global variables.
 */
void
dwf_load_syms_from_cu(st, stf, p_flist, cu_die)
symtab_t *st;
stf_t *stf;
func_t **p_flist;
Dwarf_Die cu_die;
{
    int dw_level;
    dwload_t dw_what;

    /*
     * Initalise
     */
    dw_level = 1;	/* Follow 'dwarfdump' comvention. */

    /*
     * Gather functions, global and file-scope variable names,
     * and, for C++, class names.
     */
    dw_what = DWL_TOP_FUNCS | DWL_SKIM_VARS;
    if ((stf->stf_language == LANG_CC) && find_types_by_name())
	dw_what |= DWL_SKIM_TYPES;

    dwf_load_from_die(st, stf, p_flist, (func_t *)NULL,
		      (block_t **)NULL, (block_t *)NULL,
		      (dtype_t *)NULL, cu_die, dw_what, dw_level, 0);
}

/*
 * Load macro information for the CU of this 'stf'.
 */
void
dwf_do_cu_macros(st, stf)
symtab_t *st;
stf_t *stf;
{
    alloc_pool_t *ap;
    fil_t *fil;
    Dwarf_Debug dbg;
    Dwarf_Die cu_die;

    /*
     * Done ?
     */
    if (stf->stf_fil->fi_flags & FI_DONE_MACROS)
	return;

    /*
     * Initalise
     */
    ap = st->st_apool;
    fil = stf->stf_fil;
    dbg = stf->stf_dw_dbg;
    cu_die = dwf_stf_cu_die(stf);

    if (dwf_has_attribute(dbg, cu_die, DW_AT_macro_info)) {
	Dwarf_Off offset = dwf_get_number(dbg, cu_die, DW_AT_macro_info);
	Dwarf_Signed count;
	Dwarf_Macro_Details *macinfo;
	Dwarf_Error err;
	int rv;
	int i;

	if ((rv = dwarf_get_macro_details(dbg, offset, 0, &count, &macinfo, &err)) != DW_DLV_OK)
	    dwf_fatal_error("dwarf_get_macro_details", rv, cu_die, err);

	for (i = 0; i < count; i++) {
	    int depth = 0;
	    int baselnum;
	    char *name;
	    char *value;
	    int lnum;
	    int namelen;

	    switch (macinfo[i].dmd_type) {
	    case DW_MACINFO_define:
		value = dwarf_find_macro_value_start(macinfo[i].dmd_macro);
		namelen = value - macinfo[i].dmd_macro - 1;
		name = allocstr(ap, namelen + 1);
		memcpy(name, macinfo[i].dmd_macro, namelen);
		name[namelen] = 0;
		value = alloc_strdup(ap, value);
		lnum = (depth > 0) ? baselnum : macinfo[i].dmd_lineno;
		fil->fi_macros = ci_define_macro(ap, fil->fi_macros,
						 lnum, name, value);
		break;
	    case DW_MACINFO_undef:
		lnum = (depth > 0) ? baselnum : macinfo[i].dmd_lineno;
		ci_undef_macro(ap, fil->fi_macros, lnum, macinfo[i].dmd_macro);
		break;
	    case DW_MACINFO_start_file:
		if (depth == 0)
		    baselnum = macinfo[i].dmd_lineno;
		depth++;
		break;
	    case DW_MACINFO_end_file:
		depth--;
		break;
	    }
	}

	dwarf_dealloc(dbg, macinfo, DW_DLA_STRING);
    }

    stf->stf_fil->fi_flags |= FI_DONE_MACROS;
}

/*
 * DWARF version of scan_symtab (see ao_symscan.c)
 */
bool
dwf_scan_symtab(st, path, stf, p_flist, p_mainfunc_name, dbg)
symtab_t *st;
const char *path;
stf_t *stf;
func_t **p_flist;
const char **p_mainfunc_name;
Dwarf_Debug dbg;
{
    int rv;
    Dwarf_Global *globals = NULL;
    Dwarf_Signed global_count;
    Dwarf_Error err;
    Dwarf_Die cu_die;
    Dwarf_Off cu_hdr_offset = 0;
    Dwarf_Off cu_die_offset;
    Dwarf_Unsigned cu_hdr_len, next_cu_hdr = 0;
    Dwarf_Unsigned abbrev_offset;
    Dwarf_Half version, addr_size;

    block_t *rootblock;
    ao_stdata_t *ast;
    func_t *flist = NULL;
    hf_t **fmap;
    alloc_pool_t *ap;
    int max_mapsize;

    char *cu_name;
    char *comp_dir;
    language_t lang;
    Compiler_type ct;
    bool only_globals = 0;	/* Not used. */

    /*
     * Initalise
     */
    ast = AO_STDATA(st);
    rootblock = get_rootblock();
    ap = st->st_apool;

    max_mapsize = 32;
    fmap = (hf_t **) e_malloc(max_mapsize * sizeof(hf_t *));

    /* dwarfTODO:
     * A later enhancement may be to get the debugging info in three stages:
     * 1) use dwarf_get_globals() to get entry points (gets UPS started)
     * 2) load function names when/if required
     * 3) load other info as required (as now)
     */
    if (only_globals) {
	if ((rv = dwarf_get_globals(dbg, &globals, &global_count, &err)) != DW_DLV_OK)
	    dwf_fatal_error("dwarf_get_globals", rv, NULL, err);
    }

    /*
     * Work through the compilation unit (CU) headers.
     */
    while ((rv = dwarf_next_cu_header(dbg, &cu_hdr_len, &version,
				      &abbrev_offset, &addr_size,
				      &next_cu_hdr, &err)) == DW_DLV_OK) {
	Dwarf_Half tag;

	/*
	 * Get the DIE for this CU.
	 */
	cu_die = dwf_cu_die(dbg);
	tag = dwf_get_tag(dbg, cu_die);	/* Should get DW_TAG_compile_unit */
	cu_die_offset = dwf_offset_of_die (dbg, cu_die);

	/*
	 * Get CU attributes.
	 */
	if (dwf_has_attribute(dbg, cu_die, DW_AT_name))
	    cu_name  = dwf_get_string(dbg, ap, cu_die, DW_AT_name);
	else
	    cu_name = "";
	comp_dir = dwf_get_string(dbg, ap, cu_die, DW_AT_comp_dir);
	lang     = dwf_get_src_language(dbg, cu_die);
	ct       = dwf_get_compiler_type(dbg, cu_die);

	/*
	 * Start a new 'stf'.
	 */
	stf = make_stf(ap, cu_name, st, 0, lang, 0);
	stf->stf_symlim = -1;
	stf->stf_fnum = -1;
	stf->stf_dw_dbg = dbg;
	stf->stf_cu_die_offset = (off_t)cu_die_offset;
	stf->stf_cu_hdr_offset = (off_t)cu_hdr_offset;
	stf->stf_compiler_type = ct;
	stf->stf_objpath_hint = comp_dir;
	st->st_sfiles = ao_make_fil(stf, rootblock, comp_dir, st->st_sfiles);
	if ((*cu_name != '\0') && (ast->st_type_names == NULL))
	    ast->st_type_names = hash_create_tab(ap, 1000);
	stf->stf_fil = st->st_sfiles;

	/*
	 * Compiler ?
	 */
	if (ct != CT_UNKNOWN)
	    st->st_sfiles->fi_flags |= FI_FOUND_COMPILER;

	/*
	 * Get the list of files in this CU
	 */
	dwf_get_cu_files(dbg, cu_die, ap, stf);

	if (only_globals)
	    dwf_load_globals_from_cu(st, stf, &flist, cu_die, cu_hdr_offset, global_count, globals);
	else
	    dwf_load_syms_from_cu(st, stf, &flist, cu_die);

	/* Finish the stf. */
	reverse_stf_funclist(stf);

	cu_hdr_offset = (Dwarf_Off)next_cu_hdr;

    } /* while another CU */

    if (globals != NULL)
	dwarf_dealloc(dbg, globals, DW_DLA_LIST);

    free((char *)fmap);

    *p_flist = flist;

    return TRUE;
}

#endif /* WANT_DWARF */

