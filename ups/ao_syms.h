/* ao_syms.h - a.out file symbol table definitions */

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


/* @(#)ao_syms.h	1.4 20 Jun 1995 (UKC) */

/*  The DS3100 has it's own symbol table format, designed
 *  by Third Eye Software.  Set a cpp flag to indicate this.
 */
#if defined(ARCH_MIPS_MIPS) || defined(ARCH_ULTRIX_MIPS)
#define ST_TE
#define ST_COFF
#endif

#ifdef ST_TE
typedef SYMR nlist_t;
#else
typedef struct nlist nlist_t;
#endif
#define SYMSIZE		sizeof(nlist_t)

/*  We fake N_DATA, N_TEXT and N_EXT in getsym and findsym, so define
 *  them here.
 */
#ifndef AO_ELF
#define n_offset	n_un.n_strx
#endif
#define n_dbx_type	n_type

typedef struct fsyminfo {
	int fs_symno;		/* Start of syms in file for this func */

	fil_t *fs_initial_lno_fil;	/* fil that source lines start in */
	unsigned fs_initial_sline_offset; /* initial lnum offset (N_XLINE) */
	int fs_symlim;			/* ditto */
	long fs_cblist;			/* used only in st_cb.c */
#if WANT_THIRD_EYE
	/* Fields for extracting line number information.  */
	int fs_lno_base;
	int fs_first_lnum;
	int fs_last_lnum;

	frame_t fs_frbuf;
#endif
#if WANT_DWARF
	off_t fs_die_offset;		/* Offset of this function's DIE */
	taddr_t fs_low_pc;		/* Function code start addr. */
	taddr_t fs_high_pc;		/* Function code end addr. */
	vaddr_t *fs_frame_base;		/* Frame base location */
#endif
} fsyminfo_t;

#define AO_FSYMDATA(f)		((fsyminfo_t *)(f)->fu_symdata)

#ifdef ST_TE
typedef struct extsymst {
	const char *es_name;
	taddr_t es_addr;
	struct stfst *es_stf;
	int es_symno;
} extsym_t;
#else
typedef struct Ftype Ftype;

typedef struct snlistst {
	int sn_symno;
	const char *sn_symtab_name;
	const char *sn_name;
#ifdef ultrix
	taddr_t sn_addr;
#endif
	struct snlistst *sn_next;
} snlist_t;
#endif

typedef struct symio_s symio_t;		/* Defined in ao_symread.c */

#ifndef ELFINFO_TYPEDEFED
typedef struct Elfinfo Elfinfo;
#define ELFINFO_TYPEDEFED
#endif

#ifndef SOLIB_TYPEDEFED
typedef struct Solib Solib;
typedef struct Solib_addr Solib_addr;
#define SOLIB_TYPEDEFED
#endif

#if WANT_DWARF
/*
 * Save having to include all DWARF headers when including headers only
 * for routine prototypes.
 */
#ifndef DWARF_DEBUG_TYPEDEFED
typedef struct Dwarf_Debug_s *Dwarf_Debug;
typedef struct Dwarf_Die_s *Dwarf_Die;
#define DWARF_DEBUG_TYPEDEFED
#endif
#endif


/*  Non-generic symbol table information for a.out files.  The st_data
 *  field of the generic symtab_t structure points at this structure for
 *  a.out files.
 *
 *  Sun's compiler leaves most debugging symbols in the .o files rather
 *  than copying them all to the a.out file, so we need multiple symio
 *  structures.  On other systems text_symio will be the only symio structure.
 */
typedef struct ao_stdata_t {
	int st_dynamic;			/* TRUE if dynamically linked */
	long st_base_address;		/* Addr shlib mapped rel to sym addrs */
	off_t st_addr_to_fpos_offset;	/* Core addr - textfile offset */
	int st_textfd;			/* Fd open for reading on textfile */
	symio_t *st_text_symio;		/* Symio for the a.out file itself */

#if WANT_THIRD_EYE
	extsym_t *st_exttab;		/* External symbols */
	int st_exttab_size;		/* # external symbols */
	char *st_lnotab;		/* The whole line number table */
	long st_lnotab_size;		/* Size of the table */
	strcache_id_t st_aux_strcache;	/* Aux symbols (TIRs etc) */
	struct stfst **st_stftab;	/* For rndx mapping */
	int st_stftab_size;		/* # entries in st_stftab */
#endif

#if WANT_DWARF
	hashtab_t *st_type_names;	/* Type name -> DIE. */
	Dwarf_Debug st_dw_dbg;		/* Handle for libdwarf calls */
	bool st_dw_scanned;		/* Dwarf symbols scanned? */
	taddr_t st_dw_base_address;	/* Addr shlib mapped rel to Dwarf sym addrs */
#endif

#if WANT_ELF
	Solib *st_solib;
#else
	symtab_t *st_next;
#endif
} ao_stdata_t;

typedef struct {
	symio_t *symio;
	int symno;
} Symrec;

#define AO_STDATA(st)	((ao_stdata_t *)(st)->st_data)

#ifdef ARCH_386
/*  Max number of registers that can be saved on function entry.
 *  We are conservative here - the real number is probably 3 or 4.
 */
#define N_PUSHABLE_REGS	8
#endif

typedef struct ao_preamble_s {
	unsigned pr_bpt_offset;	/* offset from func addr of first bpt loc */
	unsigned pr_rsave_mask;	/* register save mask */
	int pr_rsave_offset;	/* offset from fp of start of reg save area */
#ifdef ARCH_CLIPPER
	int pr_frame_size;	/* frame size for functions with no fp */
#endif
#if defined(ARCH_MIPS) || defined(ARCH_CLIPPER) || defined(ARCH_SUN3)
	unsigned pr_fpreg_rsave_mask;	/* floating point reg rsave mask */
	int pr_fpreg_rsave_offset;	/* floating point reg save offset */
#endif
#ifdef ARCH_386
	char pr_regtab[N_PUSHABLE_REGS];/* which order regs saved */
#endif
} ao_preamble_t;

#define AO_PREDATA(f)	((ao_preamble_t *)(f)->fu_predata)

#ifdef ST_TE
typedef struct aggrlistst {
	type_t *al_type;
	int al_symno;
	struct aggrlistst *al_next;
} aggrlist_t;
#endif

#if WANT_ELF
enum { AR_DATA, AR_BSS, AR_RODATA, AR_NTYPES };

typedef struct Dataspace {
	taddr_t base;
	taddr_t lim;
} Addr_range;
#endif

/*
 * Used to keep track of type etc. information gathered from each
 * compilation unit (CU).
 * Currently only used for DWARF.
 */
typedef enum {
    DT_IS_TYPE,
    DT_IS_TYPEDEF,
    DT_IS_VAR,
    DT_IS_RANGE,
    DT_IS_BITFIELD,
} dt_is_t;
typedef struct dtype_s {
	off_t dt_offset;	/* CU relative offset of DIE for this type. */
	off_t dt_base_offset;	/* CU relative offset of DIE for base type. */
	dt_is_t dt_is;
	type_t **dt_p_type;	/* Type pointer to be set. */
	type_t *dt_type;	/* Type (DT_IS_TYPE/DT_IS_TYPEDEF) */
	struct dtype_s *dt_next;
} dtype_t;

/*
 * Type information saved on initial skim through compilation units.
 * Currently only C++ classes are recorded.
 * Currently only used for DWARF.
 */
typedef struct typename_s {
	char *tn_name;		/* Name. */
	off_t tn_offset;	/* CU relative offset of DIE for this type. */
	struct stf_s *tn_stf;
	struct typename_s *tn_next;	/* Next with the same name. */
} typename_t;

/*
 * Symbol table information dependent on symbol table types being supported.
 *
 * Use #if/#ifdef ONLY on defines in <ifdefs.h> or things will break.
 */
typedef struct stf_s {
	const char *stf_name;
	language_t stf_language;
	Compiler_type stf_compiler_type;
	symtab_t *stf_symtab;
	fil_t *stf_fil;
	int stf_symno;
	int stf_symlim;
	taddr_t stf_addr;
	unsigned stf_flags;
#if WANT_ELF
	const char *stf_objpath_hint;
	const char *stf_objname;
	time_t stf_obj_mtime;
	const char *stf_global_prefix;
	symio_t *stf_symio;
	Addr_range stf_range[AR_NTYPES];
#endif
#ifdef ARCH_CLIPPER
	addrlist_t *stf_addrlist;
#endif
#if WANT_THIRD_EYE
	aggrlist_t *stf_aggrlist;
	long *stf_rfdtab;
	int stf_rfdtab_size;
	long stf_aux_base;
	long stf_strings_base;
	int stf_lno_base;
	int stf_lno_lim;
#else
	snlist_t *stf_snlist;
	Ftype *stf_ftypes;
	int stf_mapsize;
	struct hfst **stf_fmap;
	int stf_fnum;
#endif
#if WANT_DWARF
	Dwarf_Debug stf_dw_dbg;		/* Handle for libdwarf calls */
	off_t stf_cu_hdr_offset;	/* Offset of the CU header */
	off_t stf_cu_die_offset;	/* Offset of the CU DIE */
	dtype_t *stf_dtypes;		/* List of types. */
	dtype_t *stf_last_dt;		/* End of stf_dtypes list. */
	struct stf_s *stf_parent;	/* Parent (for stf_fmap entries) */
#endif
} stf_t;

#define AO_FIDATA(fil)	((stf_t *)fil->fi_data)

/*  Flag bits in stf_flags.
 */
#define STF_LNOS_PRECEDE_FUNCS	0x01	/* for gcc */
#define STF_HIDE		0x02	/* don't show this under source files */
#define STF_NEED_SYMSCAN	0x04    /* need to get syms from .o file */
#define STF_NEED_PATCH		0x08    /* some types have internal type codes */
#define STF_FUNCS_REPEATED	0x10	/* two N_FUN syms for each function */

typedef struct hfst {
	stf_t *hf_stf;
	int hf_id;
	struct hfst *hf_next;
} hf_t;

