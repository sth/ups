/* ao_dwfutil.c - DWARF misc functions */

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

char ups_ao_dwfutil_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#if WANT_DWARF

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <mtrprog/hash.h>
#include <limits.h>
#include <stdlib.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "ci.h"
#include "st.h"
#include "data.h"
#include "ao_dwarf.h"
#include "ao_syms.h"
#include "ao_symscan.h"
#include "ao_dwfutil.h"
#include "state.h"

/*
 * Normalise a path - where possible eliminate any "..".
 * Overwrites the original string.
 * e.g. normalise_path(strf("%s/%s", dir, name))
 */
char *
normalise_path(path)
char *path;
{
    char **components, **optr, **result, **nptr;
    char *copy;
    int count = 0;
    bool full_path = FALSE;

    if ((path == NULL) || (*path == '\0'))
	return path;
    if (*path == '/')
	full_path = TRUE;

    components = ssplit(path, "/");
    optr = components;
    while (*optr != NULL) {
	count++;
	optr++;
    }
    result = (char **)e_malloc((count+1) * sizeof(char *));
    nptr = result;
    *nptr = NULL;
    optr = components;
    while (*optr != NULL) {
	if ((strcmp (*optr, "..") == 0) && (nptr > result)) {
	    nptr--;
	    optr++;
	} else if (strcmp (*optr, "..") == 0) {
	    /* Above starting directory - can't normalise this. */
	    free(components);
	    free(result);
	    return path;
	} else if (strcmp (*optr, ".") == 0) {
	    optr++;
	} else {
	    *(nptr++) = *(optr++);
	}
	*nptr = NULL;
    }

    copy = strdup(path);
    *copy = '\0';
    nptr = result;
    while (*nptr != NULL) {
	strcat(copy, "/");
	strcat(copy, *nptr);
	nptr++;
    }
    if (full_path)
	strcpy(path, copy);
    else
	strcpy(path, copy+1);

    free (components);
    free (result);
    free (copy);
    return path;
}

/*
 * Does the user want the data from this file loaded ?
 */
bool
load_this_stf(stf)
stf_t *stf;
{
    char *lib;
    bool load = TRUE;

    if (*(stf->stf_name) != '/') {
	lib = normalise_path(strf("%s/%s", stf->stf_objpath_hint, stf->stf_name));
	if (lib)
	    load = user_wants_library_loaded(lib);
	free(lib);
    }
    else
	load = user_wants_library_loaded((char *)stf->stf_name);
    return load;
}

/*
 * Get the source file language.
 *
 * 'cu_die' is the compilation unit DIE
 * See .../lib/libdwarf/dwarf.h for list of languages.
 * dwarfTODO: environment variable UPS_LANGUAGE ?
 */
language_t
dwf_get_src_language(dbg, cu_die)
Dwarf_Debug dbg;
Dwarf_Die cu_die;
{
    int rv;
    Dwarf_Error err;
    Dwarf_Unsigned sl;

    if ((rv = dwarf_srclang(cu_die, &sl, &err)) != DW_DLV_OK)
	dwf_fatal_error("dwarf_srclan", rv, cu_die, err);
    switch (sl)
    {
    case DW_LANG_C:		return LANG_C;
    case DW_LANG_C89:		return LANG_C;
    case DW_LANG_C99:		return LANG_C;
    case DW_LANG_C_plus_plus:	return LANG_CC;
    case DW_LANG_Fortran77:	return LANG_F77;
    case DW_LANG_Fortran90:	return LANG_F90;
    case DW_LANG_Fortran95:	return LANG_F90;
    case DW_LANG_Pascal83:	return LANG_UNKNOWN;
    case DW_LANG_Java:		return LANG_UNKNOWN;
    }
    return LANG_UNKNOWN;
}

/*
 * Get the compiler type.
 *
 * 'cu_die' is the compilation unit DIE
 * dwarfTODO: environment variable UPS_LANGUAGE ?
 */
Compiler_type
dwf_get_compiler_type(dbg, cu_die)
Dwarf_Debug dbg;
Dwarf_Die cu_die;
{
    Compiler_type ct = CT_UNKNOWN;
    char *producer;

    if ((producer = dwf_get_string(dbg, NULL, cu_die, DW_AT_producer)) != NULL) {
	/*
	 * dwarfTODO: find out other values of 'producer'
	 * dwarfTODO: if used C++ on a C file may want to change language
	 */
	if (strncmp (producer, "GNU C ", 6) == 0)
	    ct = CT_GNU_CC;
	else if (strncmp (producer, "GNU C++ ", 8) == 0)
	    ct = CT_GNU_CC;
	else if (strncmp (producer, "GNU F77 ", 8) == 0)
	    ct = CT_GNU_CC;
	
	free (producer);
    }

    return ct;
}

/*
 * Find the DIE specifying an object.
 */
Dwarf_Die
dwf_find_spec_die(dbg, die)
Dwarf_Debug dbg;
Dwarf_Die die;
{
    Dwarf_Die next_die;

    /*
     * Is the specification elsewhere ?
     */
    if (dwf_has_attribute(dbg, die, DW_AT_abstract_origin)) {
	next_die = dwf_die_at_attribute(dbg, die, DW_AT_abstract_origin);
	return dwf_find_spec_die(dbg, next_die);
    } else if (dwf_has_attribute(dbg, die, DW_AT_specification)) {
	next_die = dwf_die_at_attribute(dbg, die, DW_AT_specification);
	return dwf_find_spec_die(dbg, next_die);
    }

    /*
     * This is the specification DIE.
     */
    return die;
}

/*
 * Find the DIE giving the file where an object is declared.
 */
Dwarf_Die
dwf_find_decl_die(dbg, die)
Dwarf_Debug dbg;
Dwarf_Die die;
{
    Dwarf_Die next_die;

    if (dwf_has_attribute(dbg, die, DW_AT_decl_file))
	return die;

    /*
     * Is the specification elsewhere ?
     */
    if (dwf_has_attribute(dbg, die, DW_AT_abstract_origin)) {
	next_die = dwf_die_at_attribute(dbg, die, DW_AT_abstract_origin);
	return dwf_find_decl_die(dbg, next_die);
    } else if (dwf_has_attribute(dbg, die, DW_AT_specification)) {
	next_die = dwf_die_at_attribute(dbg, die, DW_AT_specification);
	return dwf_find_decl_die(dbg, next_die);
    }
    return (Dwarf_Die)0;
}

/*
 * Get the compilation unit DIE for the specified 'stf'.
 */
Dwarf_Die
dwf_stf_cu_die(stf)
stf_t *stf;
{
    return dwf_die_at_offset(stf->stf_dw_dbg, stf->stf_cu_die_offset);
}

/*
 * Look in the source/headers list for file number 'fnum'
 */
stf_t *
dwf_lookup_file_by_fnum(stf, fnum)
stf_t *stf;
int fnum;
{
    int i;

    for (i = 0; i < stf->stf_mapsize; i++) {
	if (stf->stf_fmap[i]->hf_id == fnum)
	    return stf->stf_fmap[i]->hf_stf;
    }
    return (stf_t *)NULL;
}

/*
 * Find a 'func_t' from an address.
 */
func_t *
dwf_lookup_func_by_addr(stf, addr)
stf_t *stf;
taddr_t addr;
{
    int i;
    func_t *f;
    stf_t *hstf;
    fsyminfo_t *fs;
    funclist_t *fl;

    /*
     * First check the functions in the main file.
     */
    fl = stf->stf_fil->fi_funclist;
    while (fl != NULL) {
	fs = AO_FSYMDATA(fl->fl_func);
	if ((addr >= fs->fs_low_pc) && (addr <= fs->fs_high_pc))
	    return fl->fl_func;
	fl = fl->fl_next;
    }

    /*
     * Then check in the included files.
     */
    for (i = 0; i < stf->stf_mapsize; i++) {
	hstf = stf->stf_fmap[i]->hf_stf;
	if (stf == hstf)
	    continue;
	if ((f = dwf_lookup_func_by_addr(hstf, addr)) != NULL)
	    return f;
    }
    return (func_t *)NULL;
}

/*
 * Look in the source/headers list for file called 'name'
 */
stf_t *
dwf_lookup_file_by_name(stf, name)
stf_t *stf;
char *name;
{
    int i;
    stf_t *hstf;

    for (i = 0; i < stf->stf_mapsize; i++) {
	hstf = stf->stf_fmap[i]->hf_stf;
	if (strcmp(hstf->stf_name, name) == 0)
	    return hstf;
    }
    return (stf_t *)NULL;
}

/*
 * Given a DIE get the 'stf_t' the object was declared in.
 */
stf_t *
dwf_lookup_file_by_die(stf, die)
stf_t *stf;
Dwarf_Die die;
{
    int fnum;
    Dwarf_Debug dbg;

    dbg = stf->stf_dw_dbg;
    if (! dwf_has_attribute(dbg, die, DW_AT_decl_file))
	return (stf_t *)NULL;

    fnum = dwf_get_number(dbg, die, DW_AT_decl_file);
    return dwf_lookup_file_by_fnum(stf, fnum);
}

/*
 * Get the list of source files in this CU and save in the 'stf'.
 */
void
dwf_get_cu_files(dbg, cu_die, ap, stf)
Dwarf_Debug dbg;
Dwarf_Die cu_die;
alloc_pool_t *ap;
stf_t *stf;
{
    int rv;
    Dwarf_Error err;
    Dwarf_Signed count;
    int i;
    stf_t *hf_stf;
    char **srcfiles, *name;
    const char *comp_dir;
    int comp_dir_len = 0;
    hf_t **list, *file;

    /*
     * Get the list of source and header files in this CU.
     * Usually the first will be the source file and the rest are the
     * headers.
     */
    if ((rv = dwarf_srcfiles(cu_die, &srcfiles, &count, &err)) == DW_DLV_NO_ENTRY)
	return;
    if (rv != DW_DLV_OK)
	dwf_fatal_error("dwarf_srcfiles", rv, cu_die, err);
    comp_dir = stf->stf_objpath_hint;
    if (comp_dir)
	comp_dir_len = strlen(comp_dir);
    list = (hf_t **)alloc(ap, count * sizeof(hf_t *));
    for (i = 0; i < count; i++) {
	/*
	 * srcfiles[i] can be either a full path or a relative path,
	 * and does not always match DW_AT_name from the CU header
	 */
	if (comp_dir && (strncmp (comp_dir, srcfiles[i], comp_dir_len) == 0))
	    name = alloc_strdup(ap, srcfiles[i]+comp_dir_len+1);
	else
	    name = alloc_strdup(ap, srcfiles[i]);
	if (strcmp (name, stf->stf_fil->fi_name) == 0) {
	    /*
	     * Is the main source file so just point to it.
	     */
	    hf_stf = stf;
	} else {
	    /*
	     * An included file.  Create an 'stf_t' for the file and copy stuff
	     * from the main 'stf' into it.
	     */
	    hf_stf = make_stf(ap, name, stf->stf_symtab, 0, stf->stf_language, 0);
	    hf_stf->stf_symlim = -1;
	    hf_stf->stf_dw_dbg = dbg;
	    hf_stf->stf_cu_die_offset = stf->stf_cu_die_offset;
	    hf_stf->stf_cu_hdr_offset = stf->stf_cu_hdr_offset;
	    hf_stf->stf_compiler_type = stf->stf_compiler_type;
	    hf_stf->stf_parent = stf;
	    hf_stf->stf_fil = ao_make_fil(hf_stf, (block_t *)NULL, comp_dir, (fil_t *)NULL);
	}
	hf_stf->stf_fnum = i + 1;	/* numbers start at 1 */

	file = (hf_t *)alloc(ap, sizeof(hf_t));
	file->hf_id = i + 1;		/* numbers start at 1 */
	file->hf_stf = hf_stf;
	file->hf_next = NULL;
	list[i] = file;

	dwarf_dealloc(dbg, srcfiles[i], DW_DLA_STRING);
    }
    dwarf_dealloc(dbg, srcfiles, DW_DLA_LIST);

    stf->stf_fmap = list;
    stf->stf_mapsize = count;
}

/*
 * Make a 'lexinfo_t' to describe the location of a declaration etc.
 * If not location information return NULL.
 */
lexinfo_t *
dwf_make_lexinfo(dbg, die, ap, stf)
Dwarf_Debug dbg;
Dwarf_Die die;
alloc_pool_t *ap;
stf_t *stf;
{
    lexinfo_t *lx;
    int cnum = -1;
    int lnum = 0;
    int fnum = 0;
    stf_t *hstf;
    const char *name = NULL;

    /*
     * Ignore artificial (compiler created) stuff.
     * Will not have the attributes anyway.
     */
    if (dwf_get_opt_flag(dbg, die, DW_AT_artificial))
	return (lexinfo_t *)NULL;

    /*
     * Get the declaration file and line.
     */
    if (dwf_has_attribute(dbg, die, DW_AT_decl_file))
	fnum = dwf_get_number(dbg, die, DW_AT_decl_file);
    if (dwf_has_attribute(dbg, die, DW_AT_decl_line))
	lnum = dwf_get_number(dbg, die, DW_AT_decl_line);
    if (dwf_has_attribute(dbg, die, DW_AT_decl_column))
	cnum = dwf_get_number(dbg, die, DW_AT_decl_column);

    if ((fnum == 0) && (lnum == 0))
	return (lexinfo_t *)NULL;

    /*
     * Convert file number to name.
     * Only works if dwf_get_cu_files() has been called.
     */
    if ((hstf = dwf_lookup_file_by_fnum(stf, fnum)) != NULL)
	name = hstf->stf_name;
    else if (fnum == 1)
	name = stf->stf_name;

    lx = (lexinfo_t *)alloc(ap, sizeof(lexinfo_t));
    lx->lx_filename = name;
    lx->lx_lnum = lnum;
    lx->lx_cnum = cnum;
    return lx;
}

/*
 * Convert a DWARF 'location description' into a 'vaddr_t'.
 *
 * dwarfTODO: only handles the bare minimum required to get UPS working
 */
vaddr_t *
dwf_get_location(Dwarf_Debug dbg, alloc_pool_t *ap, Dwarf_Die die, Dwarf_Half id, vaddr_t *frame_base)
{
    int i;
    Dwarf_Locdesc **loclist;
    Dwarf_Signed count = 0;
    Dwarf_Small op;
    vaddr_t *head = NULL, *vaddr;
    unsigned char fp_regop;
    unsigned char sp_regop;
    unsigned char fp_reg;
    unsigned char sp_reg;

    if ((loclist = dwf_get_loclist(dbg, die, id, &count)) == NULL)
	return (vaddr_t *)NULL;

    switch (xp_get_addrsize(get_current_target()))
    {
    case 32:
	fp_regop = DW_OP_breg5;
	sp_regop = DW_OP_breg4;
	fp_reg = 5;
	sp_reg = 4;
	break;
    case 64:
	fp_regop = DW_OP_breg6;
	sp_regop = DW_OP_breg7;
	fp_reg = 6;
	sp_reg = 7;
	break;
    default:
	panic("dwf_get_location : unsupported address size");
    }

    for (i = 0; i < count; i++) {

	vaddr = (vaddr_t *)alloc(ap, sizeof(vaddr_t));
	if (head == NULL) {
	    head = vaddr;
	} else {
	    errf("dwf_get_location : location list too long");
	    head = NULL;
	    break;
        }

	if (loclist[i]->ld_cents != 1) {
	    errf("dwf_get_location : location expression too complicated");
	    head = NULL;
	    break;
	}

	op = loclist[i]->ld_s->lr_atom;
	if (op == DW_OP_addr) {
	    /*
	     * Address
	     */
	    vaddr->v_op = OP_ADDR;
	    vaddr->v_addr = loclist[i]->ld_s->lr_number;
	} else if ((op >= DW_OP_reg0) && (op <= DW_OP_reg31)) {
	    /*
	     * Register 0..31
	     */
	    vaddr->v_op = OP_REGISTER;
	    vaddr->v_register = op - DW_OP_reg0;
	} else if (op == DW_OP_regx) {
	    /*
	     * Register
	     */
	    vaddr->v_op = OP_REGISTER;
	    vaddr->v_register = loclist[i]->ld_s->lr_number;
	} else if (op == DW_OP_plus_uconst) {
	    /*
	     * Unsigned offset.
	     */
	    vaddr->v_op = OP_U_OFFSET;
	    vaddr->v_u_offset = loclist[i]->ld_s->lr_number;
	} else if (op == DW_OP_fbreg) {
	    /*
	     * Relative to frame base address.
	     */
	    if (frame_base == BAD_FRAME_BASE) {
		errf("dwf_get_location : frame base not known");
		head = NULL;
		i = count;
	    } else if (frame_base && frame_base->v_op == OP_REGISTER) {
		if (frame_base->v_register == fp_reg) {
		    vaddr->v_op = OP_FP_RELATIVE;
		    vaddr->v_offset = (Dwarf_Signed)loclist[i]->ld_s->lr_number;
		} else if (frame_base->v_register == sp_reg) {
		    vaddr->v_op = OP_SP_RELATIVE;
		    vaddr->v_offset = (Dwarf_Signed)loclist[i]->ld_s->lr_number;
		} else {
		    panic("dwf_get_location : frame base register not fp or sp");
		}
	    } else {
		vaddr->v_op = OP_CFA_RELATIVE;
		vaddr->v_offset = (Dwarf_Signed)loclist[i]->ld_s->lr_number;
	    }
	} else if (op == fp_regop) {
	    /*
	     * Relative to frame pointer.
	     */
	    vaddr->v_op = OP_FP_RELATIVE;
	    vaddr->v_offset = (Dwarf_Signed)loclist[i]->ld_s->lr_number;
	} else if (op == sp_regop) {
	    /*
	     * Relative to stack pointer.
	     */
	    vaddr->v_op = OP_SP_RELATIVE;
	    vaddr->v_offset = (Dwarf_Signed)loclist[i]->ld_s->lr_number;
	} else if (op == DW_OP_call_frame_cfa) {
	    /*
	     * Frame base address.
	     */
	    if (frame_base == BAD_FRAME_BASE) {
		errf("dwf_get_location : frame base not known");
		head = NULL;
		i = count;
	    } else if (frame_base && frame_base->v_op == OP_REGISTER) {
		if (frame_base->v_register == fp_reg) {
		    vaddr->v_op = OP_FP_RELATIVE;
		    vaddr->v_offset = 0;
		} else if (frame_base->v_register == sp_reg) {
		    vaddr->v_op = OP_SP_RELATIVE;
		    vaddr->v_offset = 0;
		} else {
		    panic("dwf_get_location : frame base register not fp or sp");
		}
	    } else {
		vaddr->v_op = OP_CFA_RELATIVE;
		vaddr->v_offset = 0;
	    }
	} else {
	    panic("dwf_get_location : unsupported opcode in location expression");
	}

	dwarf_dealloc(dbg, loclist[i]->ld_s, DW_DLA_LOC_BLOCK);
	dwarf_dealloc(dbg, loclist[i], DW_DLA_LOCDESC);
    }
    dwarf_dealloc(dbg, loclist, DW_DLA_LIST);

    return head;
}

#if defined(ARCH_386)

/*
 * Unwind the value of a specified register.
 */
static bool
dwf_unwind_reg(Dwarf_Fde fde, target_t *xp, taddr_t cfa, taddr_t fp, taddr_t sp,
               taddr_t pc, int regnum, taddr_t *regval)
{
    int rv;
    Dwarf_Error err;
    Dwarf_Small value_type;
    Dwarf_Signed offset_relevant;
    Dwarf_Signed register_num;
    Dwarf_Signed offset_or_block_len;
    Dwarf_Ptr block_ptr;
    int fp_col;
    int sp_col;

    switch (xp_get_addrsize(xp))
    {
    case 32:
	fp_col = 5;
	sp_col = 4;
	break;
    case 64:
	fp_col = 6;
	sp_col = 7;
	break;
    default:
	panic("Unsupported address size");
    }

    if (regnum == DW_FRAME_CFA_COL3) {
	rv = dwarf_get_fde_info_for_cfa_reg3(fde, pc, &value_type,
					     &offset_relevant, &register_num,
					     &offset_or_block_len, &block_ptr,
					     NULL, &err);
    } else {
	rv = dwarf_get_fde_info_for_reg3(fde, regnum, pc, &value_type,
					 &offset_relevant, &register_num,
					 &offset_or_block_len, &block_ptr,
					 NULL, &err);
    }

    if (rv == DW_DLV_OK) {
	if (value_type != DW_EXPR_OFFSET)
	    panic("Unsupported value type in unwind info");

        if (register_num == DW_FRAME_CFA_COL3)
	    *regval = cfa;
	else if (register_num == fp_col)
	    *regval = fp;
	else if (register_num == sp_col)
	    *regval = sp;
	else if (register_num == DW_FRAME_SAME_VAL && regnum == fp_col)
	    *regval = fp;
	else if (register_num == DW_FRAME_SAME_VAL && regnum == sp_col)
	    *regval = cfa;
	else
	    rv = DW_DLV_ERROR;

	if (rv == DW_DLV_OK && offset_relevant)
	    *regval += offset_or_block_len;

	if (rv == DW_DLV_OK && register_num == DW_FRAME_CFA_COL3)
	    dread_addrval(xp, *regval, regval);
    }

    return rv == DW_DLV_OK;
}

/*
 * Unwind a stack frame.
 */
bool
dwf_unwind(Dwarf_Debug dbg, target_t *xp, taddr_t *fp, taddr_t *sp, taddr_t *pc, taddr_t *cfa)
{
    int rv;
    Dwarf_Error err;
    Dwarf_Cie *cie_data;
    Dwarf_Signed cie_count;
    Dwarf_Fde *fde_data;
    Dwarf_Signed fde_count;
    int fp_col;
    int sp_col;
    int ra_col;

    switch (xp_get_addrsize(xp))
    {
    case 32:
	fp_col = 5;
	sp_col = 4;
	ra_col = 8;
	break;
    case 64:
	fp_col = 6;
	sp_col = 7;
	ra_col = 16;
	break;
    default:
	panic("Unsupported address size");
    }

    if ((rv = dwarf_get_fde_list(dbg, &cie_data, &cie_count, &fde_data, &fde_count, &err)) == DW_DLV_OK ||
        (rv = dwarf_get_fde_list_eh(dbg, &cie_data, &cie_count, &fde_data, &fde_count, &err)) == DW_DLV_OK) {
	Dwarf_Fde fde;

        if ((rv = dwarf_get_fde_at_pc(fde_data, *pc, &fde, NULL, NULL, &err)) == DW_DLV_OK) {
	    taddr_t new_fp;
	    taddr_t new_sp;
	    taddr_t new_pc;

            if (dwf_unwind_reg(fde, xp, *cfa, *fp, *sp, *pc, DW_FRAME_CFA_COL3, cfa) &&
		dwf_unwind_reg(fde, xp, *cfa, *fp, *sp, *pc, ra_col, &new_pc)) {
                if (!dwf_unwind_reg(fde, xp, *cfa, *fp, *sp, *pc, fp_col, &new_fp))
		    new_fp = 0;

                if (!dwf_unwind_reg(fde, xp, *cfa, *fp, *sp, *pc, sp_col, &new_sp))
		    new_sp = *cfa;

		*fp = new_fp;
		*sp = new_sp;
		*pc = new_pc;
	    } else {
		*pc = 0;
	    }
	}

	dwarf_fde_cie_list_dealloc(dbg, cie_data, cie_count, fde_data, fde_count);
    }

    return rv == DW_DLV_OK;
}

#else /* ARCH_386 */

/*
 * Unwind a stack frame.
 */
bool
dwf_unwind(Dwarf_Debug dbg, target_t *xp, taddr_t *fp, taddr_t *sp, taddr_t *pc)
{
    return FALSE;
}
   
#endif /* ARCH_386 */

#endif /* WANT_DWARF */
