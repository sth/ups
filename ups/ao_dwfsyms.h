/* ao_dwfsyms.h - routine definitions for ao_dwfsyms.c */

/*  Copyright (C) 2002 Ian Edwards
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

/* $Id$ */

#if WANT_DWARF

/* Tell dwf_load_info_from_die what to load. */
typedef unsigned int dwload_t;
#define DWL_GLOBAL_FUNCS	0x000001	/* Global functions. */
#define DWL_STATIC_FUNCS	0x000002	/* Static functions. */
#define DWL_NESTED_FUNCS	0x000004	/* dwarfTODO: Nested functions. */
#define DWL_GLOBAL_VARS		0x000010	/* Global variables. */
#define DWL_STATIC_VARS		0x000020	/* Static variables with global scope. */
#define DWL_LOCAL_VARS		0x000040	/* Local variables. */
#define DWL_SKIM_VARS		0x000080	/* Just save names. */
#define DWL_BASE_TYPES		0x000100	/* Base types. */
#define DWL_GLOBAL_TYPES	0x000200	/* File scope types. */
#define DWL_LOCAL_TYPES		0x000400	/* Local types. */
#define DWL_SKIM_CLASSES	0x000800	/* Just save names. */
#define DWL_LEXICAL_BLOCKS	0x001000	/* Lexical blocks. */
#define DWL_SUBRANGES		0x002000	/* Subranges, e.g. array dimensions. */
#define DWL_ENUM_MEMBERS	0x010000	/* Enumerated type members. */
#define DWL_STRUCT_MEMBERS	0x020000	/* Structure/union members. */
#define DWL_CLASS_MEMBERS	0x040000	/* Class members. */

#define DWL_TOP_FUNCS	(DWL_GLOBAL_FUNCS | DWL_STATIC_FUNCS)
#define DWL_ANY_FUNCS	(DWL_GLOBAL_FUNCS | DWL_STATIC_FUNCS | DWL_NESTED_FUNCS)
#define DWL_TOP_VARS	(DWL_GLOBAL_VARS | DWL_STATIC_VARS)
#define DWL_ANY_VARS	(DWL_GLOBAL_VARS | DWL_STATIC_VARS | DWL_LOCAL_VARS)
#define DWL_TOP_TYPES	(DWL_BASE_TYPES | DWL_GLOBAL_TYPES)
#define DWL_ANY_TYPES	(DWL_BASE_TYPES | DWL_GLOBAL_TYPES | DWL_LOCAL_TYPES)
#define DWL_AE_MEMBERS	(DWL_ENUM_MEMBERS | DWL_STRUCT_MEMBERS | DWL_CLASS_MEMBERS)


void dwf_do_cu_lines PROTO((symtab_t *st, stf_t *stf));
void dwf_do_cu_types PROTO((symtab_t *st, stf_t *stf));
void dwf_do_cu_vars  PROTO((symtab_t *st, stf_t *stf));

void dwf_do_fu_blocks PROTO((symtab_t *st, stf_t *stf, func_t *f,
	fsyminfo_t *fs));

func_t * dwf_add_function_to_symtab PROTO((symtab_t *st, stf_t *stf,
	func_t **p_flist, const char *name,
	bool is_static, Dwarf_Die die, Dwarf_Die spec_die));

void dwf_load_syms_from_cu PROTO((symtab_t *st, stf_t *stf,
	func_t **p_flist, Dwarf_Die cu_die));

void dwf_do_cu_macros PROTO((symtab_t *st, stf_t *stf));

bool dwf_scan_symtab PROTO((symtab_t *st, const char *path, stf_t *stf,
	func_t **p_flist, const char **p_mainfunc_name, Dwarf_Debug dbg));

#endif /* WANT_DWARF */
