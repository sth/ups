/* cx_link.h - header file for cx_link.c */

/*  Copyright 1992 Mark Russell, University of Kent at Canterbury.
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


/* @(#)cx_link.h	1.3 16 Apr 1994 (UKC) */

typedef struct libfuncst {
	const char *lf_name;
	taddr_t lf_addr;
	int lf_index;
	lexinfo_t lf_lexinfo;
	struct libfuncst *lf_next;
} libfunc_t;

typedef struct {
	taddr_t le_addr;
	int le_lnum;
} lnum_entry_t;

typedef struct linkname_s {
	const char *ln_name;
	taddr_t ln_addr;
	struct linkname_s *ln_next;
} linkname_t;

typedef struct ext_reloc_s {
	int er_index;
	taddr_t er_location;
	struct ext_reloc_s *er_next;
} ext_reloc_t;

typedef struct reloc_pos_s {
	taddr_t rp_location;
	struct reloc_pos_s *rp_next;
} reloc_pos_t;

typedef struct symfunc_s {
	const char *sf_name;
	taddr_t sf_addr;
	lnum_entry_t *sf_letab;
	int sf_letab_size;
} symfunc_t;

/*  This structure contains all the information about a .ocx file that
 *  is needed to link it into an `executable' cx file.  The information
 *  is constructed by ci_compile() and either used directly or written
 *  out to a .ocx file and read in again later.
 *
 *  See doc/linking.ms for a description of how linking and relocation
 *  is done.
 */
struct linkinfo_s {
	alloc_pool_t *li_apool;

	const char *li_path;

	size_t li_bss_size;
	size_t li_extbss_size;
	const char *li_srcpath;

	libfunc_t *li_undef_funcs;	/* funcs referenced but not defined */
	linkname_t *li_undef_vars;	/* vars referenced but not defined */
	linkname_t *li_funcs;		/* defined funcs with ext linkage */
	linkname_t *li_vars;		/* defined vars with ext linkage */
	char *li_extdata;		/* external visible initialised data */
	size_t li_extdata_size;
	ext_reloc_t *li_extdata_relocs;
	reloc_pos_t *li_extdata_funcrelocs;

	reloc_pos_t *li_static_rps;
	reloc_pos_t *li_static_funcrps;
	ext_reloc_t *li_ext_relocs;
	struct reg_reloc_s *li_reg_relocs;
	taddr_t *li_funcaddrs;
	int li_nfuncaddrs;
	size_t li_max_stack_nwords;
	unsigned char *li_text;
	size_t li_text_size;
	char *li_data;
	size_t li_data_size;

	/*  Optional symbol table information.  Only maintained with the -g
	 *  flag to the compiler.
	 */
	symfunc_t *li_symfuncs;	/* funcname->addr map.  Includes static funcs */
	struct block_s **li_funcblocks;		/* nsymfuncs func blocks */
	struct block_s *li_filblock;
	int li_nsymfuncs;
	const char **li_symvars;
	int li_nsymvars;
};

typedef struct { int dummy; } *link_id_t;

link_id_t ci_make_link_id PROTO((void));

int ci_link PROTO((link_id_t link_id, bool want_syminfo, const char *path,
		   ci_getaddrproc_t get_libaddr,
		   size_t n_libfuncs, size_t n_libvars));



#ifdef EOF
int ci_disassemble_linkinfo PROTO((linkinfo_t *li, FILE *fp, const char *path));
#endif

#ifdef OCX_MAGIC
typedef struct linklist_s {
	off_t ll_offset;
	linkinfo_t *ll_linkinfo;
	cx_header_t *ll_cx;
	struct linklist_s *ll_next;
} linklist_t;

int ci_write_asm_file PROTO((linklist_t *ll, const char *path));

linklist_t *ci_read_cx_file PROTO((const char *path, bool read_text_etc));

int ci_write_cx_file PROTO((linkinfo_t *li, cx_header_t *cx,
			    bool want_syminfo, const char *path));

linklist_t *ci_make_linklist PROTO((linkinfo_t *li, cx_header_t *cx));

void ci_add_linklist PROTO((link_id_t link_id, linklist_t *linklist,
					const char *path, bool verbose));

void ci_free_linklist PROTO((linklist_t *ll));
#endif /* OCX_MAGIC */
