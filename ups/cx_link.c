/* cx_link.c - write ocx files and link them together */

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


/* @(#)cx_link.c	1.4 16 Sep 1994 (UKC) */
char ups_cx_link_c_rcsid[] = "$Id$";

#define UKC_WANT_COMMON_UNIX_EXTENSIONS 1

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>		/* for MAXPATHLEN */

#ifdef __STDC__
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>

#include "ups.h"
#include "ci.h"
#include "xc_opcodes.h"
#include "xc_machine.h"
#include "xc_load.h"
#include "cx_link.h"
#include "cx_symio.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#endif

char *getwd PROTO((char *buf));		/* BUG: shouldn't be here */

/*  Element in the list of external names used by the linker.
 */
typedef struct extname_s {
	const char *en_name;
	bool en_isfunc;			/* !function => variable */
	struct fileinfo_s *en_file;	/* File where var defined */
	taddr_t en_addr;		/* Address within file */
	const char *en_refpath;		/* File where first referenced */
	int en_index;			/* Index into in-file array */
	struct extname_s *en_next;
} extname_t;

/*  Per-file information.
 */
typedef struct fileinfo_s {
	const char *fx_path;		/* Path of .ocx file */
	off_t fx_offset;		/* Offset into .ocx file (libs) */
	size_t fx_text_size;
	const char *fx_srcpath;		/* Path of .c file */
	extname_t **fx_funcrefs;	/* Array of refs to external funcs */
	size_t fx_nfuncrefs;
	extname_t **fx_varrefs;		/* Array of refs to external vars */
	size_t fx_nvarrefs;
	extname_t **fx_vardefs;		/* Array of definitions of ext vars */
	size_t fx_nvardefs;
	long fx_index;
	char *fx_extdata;
	size_t fx_extdata_size;
	size_t fx_extbss_size;
	ext_reloc_t *fx_extdata_relocs;
	reloc_pos_t *fx_extdata_funcrelocs;
	symfunc_t *fx_symfuncs;
	size_t fx_nsymfuncs;
	const char **fx_symvars;
	size_t fx_nsymvars;
	struct fileinfo_s *fx_next;
} fileinfo_t;

/*  Overall link information.
 */
typedef struct olinkinfo_s {
	alloc_pool_t *ol_apool;
	const char *ol_dir;
	fileinfo_t *ol_files;
	extname_t *ol_extnames;
	extname_t *ol_main_sym;
	extname_t *ol_exit_sym;
	extname_t *ol_setjmp_sym;
	extname_t *ol_longjmp_sym;
	bool ol_failed;
} olinkinfo_t;

static const char Intpath[] = "/home/cur/mtr/bin/xc";

static extname_t *lookup_extname PROTO((olinkinfo_t *ol, const char *path,
						const char *name, bool isfunc));
static extname_t *push_extname PROTO((olinkinfo_t *ol, const char *refpath,
				      const char *name, bool isfunc));
static voidptr z_alloc PROTO((alloc_pool_t *ap, size_t nbytes));
static void resolve_names PROTO((olinkinfo_t *ol, linkname_t *ln,
				 fileinfo_t *fx, extname_t **vardefs));
static int write_linknames PROTO((FILE *fp, ebuf_t *str_eb,
				  linkname_t *ln, size_t *p_count));
static int read_linknames PROTO((alloc_pool_t *ap, FILE *fp,
				 const char *path, const char *strings,
				 size_t count, linkname_t **p_lnlist));
static int write_refs PROTO((FILE *fp, extname_t **p_en, size_t count));
static int write_static_relocs PROTO((FILE *fp, reloc_pos_t *rplist, size_t *p_count));
static int write_ext_relocs PROTO((FILE *fp, ext_reloc_t *erlist, size_t *p_count));
static ext_reloc_t *copy_extdata_relocs PROTO((alloc_pool_t *ap, ext_reloc_t *er));
static int read_ext_relocs PROTO((alloc_pool_t *ap, FILE *fp,
				      const char *path, size_t count,
				      ext_reloc_t **p_list));
static int read_cx PROTO((alloc_pool_t *ap, FILE *fp, const char *path,
				cx_header_t *cx, bool read_text_etc,
				linklist_t **p_ll));
static int read_static_relocs PROTO((alloc_pool_t *ap, FILE *fp,
				     const char *path, size_t count,
				     reloc_pos_t **p_list));
static int get_header PROTO((FILE *fp, const char *path,
					cx_header_t *cx, off_t *p_size));
static void link_file PROTO((link_id_t link_id, linkinfo_t *li,
			     cx_header_t *cx, const char *path, off_t offset));
static int read_library PROTO((alloc_pool_t *ap, FILE *fp, const char *path,
			       cx_header_t *cx, bool read_text_etc,
			       linklist_t **p_ll));
static int link_library PROTO((olinkinfo_t *ol, linklist_t *linklist,
					      const char *path, bool verbose));
static reloc_pos_t *copy_static_relocs PROTO((alloc_pool_t *ap,
					      reloc_pos_t *rp));
static extname_t *push_builtin PROTO((olinkinfo_t *ol, fileinfo_t *fx,
							const char *name));
static bool satisfies_ref PROTO((olinkinfo_t *ol, linkinfo_t *li,
							const char *path));
static extname_t *name_to_extname PROTO((olinkinfo_t *ol, const char *name));
static symfunc_t *copy_symfuncs PROTO((alloc_pool_t *ap,
						symfunc_t *oldsf, int count));
static const char **copy_symvars PROTO((alloc_pool_t *ap,
						const char **oldsv, int count));

link_id_t
ci_make_link_id()
{
	alloc_pool_t *ap;
	olinkinfo_t *ol;
	fileinfo_t *crt0_fx;
	char dir[MAXPATHLEN + 1];

	if (getcwd(dir, MAXPATHLEN) == NULL) {
		failmesg("Can't get current directory", "", "");
		return NULL;
	}

	ap = alloc_create_pool();
	ol = (olinkinfo_t *)alloc(ap, sizeof(olinkinfo_t));

	crt0_fx = (fileinfo_t *)alloc(ap, sizeof(fileinfo_t));
	crt0_fx->fx_path = "crt0.o";
	crt0_fx->fx_index = -1;

	ol->ol_apool = ap;
	ol->ol_dir = alloc_strdup(ap, dir);
	ol->ol_files = NULL;
	ol->ol_failed = FALSE;
	ol->ol_extnames = NULL;
	ol->ol_main_sym = push_extname(ol, crt0_fx->fx_path, "main", TRUE);
	ol->ol_exit_sym = push_extname(ol, crt0_fx->fx_path, "exit", TRUE);
	ol->ol_setjmp_sym = push_builtin(ol, crt0_fx, "__cx_setjmp");
	ol->ol_longjmp_sym = push_builtin(ol, crt0_fx, "__cx_longjmp");

	return (link_id_t)ol;
}

static extname_t *
push_builtin(ol, fx, name)
olinkinfo_t *ol;
fileinfo_t *fx;
const char *name;
{
	extname_t *en;

	en = push_extname(ol, (const char *)NULL, name, TRUE);
	en->en_file = fx;
	en->en_addr = 0;

	return en;
}

int
ci_write_asm_file(linklist, path)
linklist_t *linklist;
const char *path;
{
	linklist_t *ll;
	bool many, bad;
	FILE *fp;

	if ((fp = fopen(path, "w")) == NULL) {
		failmesg("Can't create", "", path);
		return -1;
	}

	many = linklist->ll_next != NULL;

	for (ll = linklist; ll != NULL; ll = ll->ll_next) {
		if (many)
			fprintf(fp, "Offset %ld\n\n", ll->ll_offset);

		if (ci_disassemble_linkinfo(ll->ll_linkinfo, fp, path) != 0) {
			fclose(fp);
			return -1;
		}
	}

	bad = fflush(fp) == EOF;

	if (fclose(fp) == EOF || bad) {
		failmesg("Error writing to", "", path);
		return -1;
	}

	return 0;
}

int
ci_link(link_id, want_syminfo, path, get_libaddr, n_libfuncs, n_libvars)
link_id_t link_id;
bool want_syminfo;
const char *path;
ci_getaddrproc_t get_libaddr;
size_t n_libfuncs, n_libvars;
{
	olinkinfo_t *ol;
	extname_t *en;
	x_header_t xhbuf;
	fileinfo_t *fx;
	fileinfo_t libfxbuf;
	mode_t mask;
	size_t n_extnames, nfiles, nrefs;
	size_t n_extdata_relocs, n_extdata_funcrelocs;
	size_t extdata_size, extbss_size, strings_size;
	ebuf_t *str_eb;
	FILE *fp;

	ol = (olinkinfo_t *)link_id;
	libfxbuf.fx_index = -1;

	n_extnames = 0;
	for (en = ol->ol_extnames; en != NULL; en = en->en_next) {
		taddr_t addr;

		en->en_index = n_extnames++;

		if (en->en_file != NULL)
			continue;
		
		switch ((*get_libaddr)(en->en_name, &addr)) {
		case CI_DATA:
			if (!en->en_isfunc) {
				en->en_addr = addr;
				en->en_file = &libfxbuf;
				continue;
			}
			errf("%s is data, but referred to as a function in %s",
						en->en_name, en->en_refpath);
			break;

		case CI_DIRECT_LIBFUNC:
			if (en->en_isfunc) {
				en->en_addr = addr;
				en->en_file = &libfxbuf;
				continue;
			}
			errf("%s is a function, but referred to as data in %s",
						en->en_name, en->en_refpath);
			break;
			
		case CI_UNDEFINED:
			errf("Undefined %s %s (first referenced in %s)",
					en->en_isfunc ? "function" : "variable",
					en->en_name, en->en_refpath);
			break;

		case CI_INDIRECT_LIBFUNC:
		default:
			panic("extname botch in ci_link");
		}

		ol->ol_failed = TRUE;
	}

	if (ol->ol_failed)
		return -1;

	if ((fp = fopen(path, "w")) == NULL) {
		failmesg("Can't open", "", path);
		return -1;
	}

	if (fseek(fp, sizeof(x_header_t), SEEK_SET) != 0) {
		failmesg("Can't seek in", "", path);
		fclose(fp);
		return -1;
	}

	str_eb = ebuf_create(TRUE);

	strnf(xhbuf.xh_intpath, sizeof(xhbuf.xh_intpath), "#! %s\n", Intpath);
	xhbuf.xh_magic = OCX_MAGIC;
	xhbuf.xh_version = OCX_VERSION;
	xhbuf.xh_cwd = note_string(str_eb, ol->ol_dir);
	xhbuf.xh_n_libfuncs = n_libfuncs;
	xhbuf.xh_n_libvars = n_libvars;
	xhbuf.xh_main_sym = ol->ol_main_sym->en_index;
	xhbuf.xh_exit_sym = ol->ol_exit_sym->en_index;
	xhbuf.xh_setjmp_sym = ol->ol_setjmp_sym->en_index;
	xhbuf.xh_longjmp_sym = ol->ol_longjmp_sym->en_index;

	/*  Write out an array of file entries.
	 */
	nfiles = 0;
	for (fx = ol->ol_files; fx != NULL; fx = fx->fx_next) {
		ext_reloc_t *er;
		reloc_pos_t *rp;

		fx->fx_index = nfiles++;

		n_extdata_relocs = 0;
		for (er = fx->fx_extdata_relocs; er != NULL; er = er->er_next)
			++n_extdata_relocs;

		n_extdata_funcrelocs = 0;
		for (rp = fx->fx_extdata_funcrelocs; rp != NULL;
							       rp = rp->rp_next)
			++n_extdata_funcrelocs;

		if (save_string(str_eb, fx->fx_path, fp) != 0 ||
		    save_string(str_eb, fx->fx_srcpath, fp) != 0 ||
		    fp_write_val(fp, (size_t)fx->fx_offset) != 0 ||
		    fp_write_val(fp, fx->fx_text_size) != 0 ||
		    fp_write_val(fp, fx->fx_nsymvars) != 0 ||
		    fp_write_val(fp, fx->fx_nfuncrefs) != 0 ||
		    fp_write_val(fp, fx->fx_nvarrefs + fx->fx_nvardefs) != 0 ||
		    fp_write_val(fp, fx->fx_extdata_size) != 0 ||
		    fp_write_val(fp, fx->fx_extbss_size) != 0 ||
		    fp_write_val(fp, n_extdata_relocs) != 0 ||
		    fp_write_val(fp, n_extdata_funcrelocs) != 0)
			goto bad;

	}
	xhbuf.xh_nfiles = nfiles;

	/*  Now write out all the function and variable references
	 *  and definitions in a single array.
	 */
	nrefs = 0;
	for (fx = ol->ol_files; fx != NULL; fx = fx->fx_next) {
		if (write_refs(fp, fx->fx_funcrefs, fx->fx_nfuncrefs) != 0 ||
		    write_refs(fp, fx->fx_varrefs, fx->fx_nvarrefs) != 0 ||
		    write_refs(fp, fx->fx_vardefs, fx->fx_nvardefs) != 0)
			goto bad;

		nrefs += fx->fx_nfuncrefs + fx->fx_nvarrefs + fx->fx_nvardefs;
	}
	xhbuf.xh_nrefs = nrefs;

	/*  Write out all the external data in a single lump.
	 */
	extdata_size = extbss_size = 0;
	for (fx = ol->ol_files; fx != NULL; fx = fx->fx_next) {
		if (fp_write(fp, fx->fx_extdata, fx->fx_extdata_size) != 0)
			goto bad;
		extdata_size += fx->fx_extdata_size;
		extbss_size += fx->fx_extbss_size;
	}
	xhbuf.xh_extdata_size = extdata_size;
	xhbuf.xh_extbss_size = extbss_size;

	/*  Write out all the relocs in external data.
	 */
	n_extdata_relocs = 0;
	for (fx = ol->ol_files; fx != NULL; fx = fx->fx_next) {
		size_t count;

		if (write_ext_relocs(fp, fx->fx_extdata_relocs, &count) != 0)
			goto bad;
		n_extdata_relocs += count;
	}
	xhbuf.xh_n_extdata_relocs = n_extdata_relocs;

	n_extdata_funcrelocs = 0;
	for (fx = ol->ol_files; fx != NULL; fx = fx->fx_next) {
		size_t count;

		if (write_static_relocs(fp, fx->fx_extdata_funcrelocs,
								&count) != 0)
			goto bad;
		n_extdata_funcrelocs += count;
	}
	xhbuf.xh_n_extdata_funcrelocs = n_extdata_funcrelocs;

	/*  Now write out all the external name entries.
	 */
	for (en = ol->ol_extnames; en != NULL; en = en->en_next) {
		if (fp_write_val(fp, (size_t)en->en_file->fx_index) != 0 ||
		    fp_write_val(fp, (size_t)en->en_addr) != 0) {
			goto bad;
		}
	}
	xhbuf.xh_n_extrefs = n_extnames;

	if (want_syminfo) {
		int old_strings_size, new_strings_size;
		size_t nsymfuncs, nsymvars;

		ebuf_get(str_eb, &old_strings_size);

		nsymfuncs = 0;
		for (fx = ol->ol_files; fx != NULL; fx = fx->fx_next) {
			symfunc_t *sf, *lim;

			if ((sf = fx->fx_symfuncs) == NULL)
				continue;

			lim = sf + fx->fx_nsymfuncs;
			for (; sf < lim; ++sf) {
				if (save_string(str_eb, sf->sf_name, fp) != 0 ||
				    fp_write_val(fp, (size_t)sf->sf_addr) != 0 ||
				    fp_write_val(fp, (size_t)fx->fx_index) != 0)
			    		goto bad;
			}

			nsymfuncs += fx->fx_nsymfuncs;
		}

		nsymvars = 0;
		for (fx = ol->ol_files; fx != NULL; fx = fx->fx_next) {
			const char **sv, **lim;

			if ((sv = fx->fx_symvars) == NULL)
				continue;

			lim = sv + fx->fx_nsymvars;
			for (; sv < lim; ++sv) {
				if (save_string(str_eb, *sv, fp) != 0)
					goto bad;
			}

			nsymvars += fx->fx_nsymvars;
		}

		ebuf_get(str_eb, &new_strings_size);

		xhbuf.xh_nsymfuncs = nsymfuncs;
		xhbuf.xh_nsymvars = nsymvars;
		xhbuf.xh_symstrings_size = new_strings_size - old_strings_size;
	}
	else {
		xhbuf.xh_nsymfuncs = 0;
		xhbuf.xh_nsymvars = 0;
		xhbuf.xh_symstrings_size = 0;
	}

	if (write_strings(str_eb, fp, &strings_size) != 0)
		goto bad;
	xhbuf.xh_strings_size = strings_size;

	if (fseek(fp, 0, SEEK_SET) != 0) {
		failmesg("Can't seek to beginning of", "", path);
		fclose(fp);
		ebuf_free(str_eb);
		return -1;
	}

	if (fp_write(fp, (char *)&xhbuf, sizeof(xhbuf)) != 0)
		goto bad;
		
	if (ferror(fp) || fflush(fp) == EOF)
		goto bad;
	
	umask(mask = umask((mode_t)0));
	if (fchmod(fileno(fp), 0777 & ~mask) != 0)
		failmesg("Warning: can't fchmod", "", path);

	fclose(fp);
	ebuf_free(str_eb);

	return 0;
	
bad:
	failmesg("Error writing to", "", path);
	fclose(fp);
	ebuf_free(str_eb);
	return -1;
}

static int
write_refs(fp, p_en, count)
FILE *fp;
extname_t **p_en;
size_t count;
{
	size_t i;

	for (i = 0; i < count; ++i)
		if (fp_write_val(fp, (size_t)p_en[i]->en_index) != 0)
			return -1;
	
	return 0;
}

static voidptr
z_alloc(ap, nbytes)
alloc_pool_t *ap;
size_t nbytes;
{
	if (nbytes == 0)
		return NULL;
	
	return alloc(ap, nbytes);
}

static extname_t *
name_to_extname(ol, name)
olinkinfo_t *ol;
const char *name;
{
	extname_t *en;

	for (en = ol->ol_extnames; en != NULL; en = en->en_next)
		if (strcmp(en->en_name, name) == 0)
			return en;
	
	return NULL;
}

static extname_t *
lookup_extname(ol, srcpath, name, isfunc)
olinkinfo_t *ol;
const char *srcpath, *name;
bool isfunc;
{
	extname_t *en;

	if ((en = name_to_extname(ol, name)) == NULL)
		return NULL;
	
	if (en->en_isfunc != isfunc) {
		const char *otherpath;

		if (en->en_file != NULL)
			otherpath = en->en_file->fx_path;
		else
			otherpath = en->en_refpath;

		if (isfunc) {
			errf("`%s' is a variable in %s and a function in %s",
						    name, otherpath, srcpath);
		}
		else {
			errf("`%s' is a function in %s and a variable in %s",
						    name, otherpath, srcpath);
		}
		ol->ol_failed = TRUE;
	}

	return en;
}

static extname_t *
push_extname(ol, refpath, name, isfunc)
olinkinfo_t *ol;
const char *refpath, *name;
bool isfunc;
{
	extname_t *en;

	en = (extname_t *)alloc(ol->ol_apool, sizeof(extname_t));
	en->en_name = alloc_strdup(ol->ol_apool, name);
	en->en_isfunc = isfunc;
	en->en_file = NULL;
	en->en_refpath = refpath;
	en->en_next = ol->ol_extnames;
	ol->ol_extnames = en;

	return en;
}

static bool
satisfies_ref(ol, li, path)
olinkinfo_t *ol;
linkinfo_t *li;
const char *path;
{
	extname_t *en;
	linkname_t *ln;

	en = NULL;	/* to satisfy gcc */

	for (ln = li->li_funcs; ln != NULL; ln = ln->ln_next) {
		en = name_to_extname(ol, ln->ln_name);
		if (en != NULL && en->en_file == NULL)
			break;
	}

	if (ln == NULL) {
		for (ln = li->li_vars; ln != NULL; ln = ln->ln_next) {
			en = name_to_extname(ol, ln->ln_name);
			if (en != NULL && en->en_file == NULL)
				break;
		}
	}

	if (ln == NULL)
		return FALSE;
	
	if (path != NULL) {
		printf("Including %s from %s for symbol `%s' (%s %s)\n",
					li->li_srcpath, path,ln->ln_name,
					"first referenced in", en->en_refpath);
	}

	return TRUE;
}

static int
link_library(ol, linklist, path, verbose)
olinkinfo_t *ol;
linklist_t *linklist;
const char *path;
bool verbose;
{
	linklist_t *wanted, *unused;
	bool done_one;
	const char *srpath;

	wanted = NULL;
	unused = linklist;

	srpath = verbose ? path : NULL;

	do {
		linklist_t *ll, *next;

		done_one = FALSE;

		ll = unused;
		unused = NULL;

		for (; ll != NULL; ll = next) {
			next = ll->ll_next;

			if (satisfies_ref(ol, ll->ll_linkinfo, srpath)) {
				link_file((link_id_t)ol, ll->ll_linkinfo,
					  ll->ll_cx, path, ll->ll_offset);
				done_one = TRUE;

				ll->ll_next = wanted;
				wanted = ll;
			}
			else {
				ll->ll_next = unused;
				unused = ll;
			}
		}
	} while (done_one);

	if (verbose) {
		linklist_t *ll;

		for (ll = unused; ll != NULL; ll = ll->ll_next)
			printf("Not including %s from %s - no wanted symbols\n",
					ll->ll_linkinfo->li_srcpath, path);
	}

	return 0;
}

void
ci_add_linklist(link_id, linklist, path, verbose)
link_id_t link_id;
linklist_t *linklist;
const char *path;
bool verbose;
{
	if (linklist->ll_next != NULL) {
		link_library((olinkinfo_t *)link_id, linklist, path, verbose);
	}
	else {
		link_file(link_id, linklist->ll_linkinfo, linklist->ll_cx,
						    path, linklist->ll_offset);
	}
}

/*  Update the global link information with the names defined and used
 *  in this file.
 *
 *  We have to be careful to make copies of string names here - the
 *  global information must survive the freeing of the per-file strings.
 */
static void
link_file(link_id, li, cx, path, offset)
link_id_t link_id;
linkinfo_t *li;
cx_header_t *cx;
const char *path;
off_t offset;
{
	olinkinfo_t *ol;
	alloc_pool_t *ap;
	extname_t *en, **p_en, **funcrefs, **varrefs, **vardefs;
	char *extdata;
	fileinfo_t *fx;
	libfunc_t *lf;
	linkname_t *ln;

	ol = (olinkinfo_t *)link_id;
	ap = ol->ol_apool;

	funcrefs = (extname_t **)z_alloc(ap, cx->cx_nfuncrefs *
							  sizeof(extname_t));
	varrefs = (extname_t **)z_alloc(ap, cx->cx_nvarrefs *
							  sizeof(extname_t));
	vardefs = (extname_t **)z_alloc(ap, cx->cx_nvars *
							  sizeof(extname_t));
	

	path = alloc_strdup(ap, path);
	extdata = alloc(ap, li->li_extdata_size);
	memcpy(extdata, li->li_extdata, li->li_extdata_size);

	fx = (fileinfo_t *)alloc(ap, sizeof(fileinfo_t));
	fx->fx_srcpath = alloc_strdup(ap, li->li_srcpath);
	fx->fx_path = path;
	fx->fx_offset = offset;
	fx->fx_text_size = cx->cx_text_size;
	fx->fx_funcrefs = funcrefs;
	fx->fx_nfuncrefs = cx->cx_nfuncrefs;
	fx->fx_varrefs = varrefs;
	fx->fx_nvarrefs = cx->cx_nvarrefs;
	fx->fx_vardefs = vardefs;
	fx->fx_nvardefs = cx->cx_nvars;
	fx->fx_extdata = extdata;
	fx->fx_extdata_size = cx->cx_extdata_size;
	fx->fx_extbss_size = cx->cx_extbss_size;
	fx->fx_extdata_relocs = copy_extdata_relocs(ap, li->li_extdata_relocs);
	fx->fx_extdata_funcrelocs = copy_static_relocs(ap,
					             li->li_extdata_funcrelocs);
	fx->fx_symfuncs = copy_symfuncs(ap, li->li_symfuncs, li->li_nsymfuncs);
	fx->fx_nsymfuncs = li->li_nsymfuncs;
	fx->fx_symvars = copy_symvars(ap, li->li_symvars, li->li_nsymvars);
	fx->fx_nsymvars = li->li_nsymvars;

	fx->fx_next = ol->ol_files;
	ol->ol_files = fx;

	p_en = funcrefs;
	for (lf = li->li_undef_funcs; lf != NULL; lf = lf->lf_next) {
		if ((en = lookup_extname(ol, path, lf->lf_name, TRUE)) == NULL)
			en = push_extname(ol, fx->fx_srcpath,lf->lf_name,TRUE);
		*p_en++ = en;
	}

	p_en = varrefs;
	for (ln = li->li_undef_vars; ln != NULL; ln = ln->ln_next) {
		if ((en = lookup_extname(ol, path, ln->ln_name, FALSE)) == NULL)
			en = push_extname(ol, fx->fx_srcpath,ln->ln_name,FALSE);
		*p_en++ = en;
	}

	resolve_names(ol, li->li_vars, fx, vardefs);
	resolve_names(ol, li->li_funcs, fx, (extname_t **)NULL);
}

static int
read_static_relocs(ap, fp, path, count, p_list)
alloc_pool_t *ap;
FILE *fp;
const char *path;
size_t count;
reloc_pos_t **p_list;
{
	int i;
	reloc_pos_t *list;

	list = NULL;

	for (i = 0; i < count; i++) {
		size_t location;
		reloc_pos_t *rp;

		if (readfp(fp, path, (char *)&location, sizeof(size_t)) != 0)
			return -1;

		rp = (reloc_pos_t *)alloc(ap, sizeof(ext_reloc_t));
		rp->rp_location = location;
		rp->rp_next = list;
		list = rp;
	}

	*p_list = list;
	return 0;
}

static int
read_ext_relocs(ap, fp, path, count, p_list)
alloc_pool_t *ap;
FILE *fp;
const char *path;
size_t count;
ext_reloc_t **p_list;
{
	int i;
	ext_reloc_t *list;

	list = NULL;

	for (i = 0; i < count; i++) {
		size_t index, location;
		ext_reloc_t *er;

		if (readfp(fp, path, (char *)&location, sizeof(size_t)) != 0 ||
		    readfp(fp, path, (char *)&index, sizeof(size_t)) != 0)
			return -1;

		er = (ext_reloc_t *)alloc(ap, sizeof(ext_reloc_t));
		er->er_location = location;
		er->er_index = index;
		er->er_next = list;
		list = er;
	}

	*p_list = list;
	return 0;
}

static const char **
copy_symvars(ap, oldsv, count)
alloc_pool_t *ap;
const char **oldsv;
int count;
{
	const char **newsv;
	int i;

	newsv = (const char **)alloc(ap, count * sizeof(const char *));

	for (i = 0; i < count; ++i)
		newsv[i] = alloc_strdup(ap, oldsv[i]);

	return newsv;
}

static symfunc_t *
copy_symfuncs(ap, oldsf, count)
alloc_pool_t *ap;
symfunc_t *oldsf;
int count;
{
	symfunc_t *newsf;
	int i;

	newsf = (symfunc_t *)alloc(ap, count * sizeof(symfunc_t));

	for (i = 0; i < count; ++i) {
		size_t nbytes;
		char *buf;

		newsf[i].sf_name = alloc_strdup(ap, oldsf[i].sf_name);
		newsf[i].sf_addr = oldsf[i].sf_addr;
		newsf[i].sf_letab_size = oldsf[i].sf_letab_size;

		nbytes = oldsf[i].sf_letab_size * sizeof(lnum_entry_t);	
		buf = alloc(ap, nbytes);
		memcpy(buf, (char *)oldsf[i].sf_letab, nbytes);

		newsf[i].sf_letab = (lnum_entry_t *)buf;
	}

	return newsf;
}

static reloc_pos_t *
copy_static_relocs(ap, rp)
alloc_pool_t *ap;
reloc_pos_t *rp;
{
	reloc_pos_t *newlist, *new_rp;

	newlist = NULL;

	for (; rp != NULL; rp = rp->rp_next) {
		new_rp = (reloc_pos_t *)alloc(ap, sizeof(reloc_pos_t));
		new_rp->rp_location = rp->rp_location;
		new_rp->rp_next = newlist;
		newlist = new_rp;
	}

	return newlist;
}

static ext_reloc_t *
copy_extdata_relocs(ap, er)
alloc_pool_t *ap;
ext_reloc_t *er;
{
	ext_reloc_t *newlist, *new_er;

	newlist = NULL;

	for (; er != NULL; er = er->er_next) {
		new_er = (ext_reloc_t *)alloc(ap, sizeof(ext_reloc_t));
		new_er->er_index = er->er_index;
		new_er->er_location = er->er_location;
		new_er->er_next = newlist;
		newlist = new_er;
	}

	return newlist;
}

static void
resolve_names(ol, ln, fx, vardefs)
olinkinfo_t *ol;
linkname_t *ln;
fileinfo_t *fx;
extname_t **vardefs;
{
	extname_t *en, **p_en;
	bool isfunc;

	p_en = vardefs;
	isfunc = vardefs == NULL;

	for (; ln != NULL; ln = ln->ln_next) {
		en = lookup_extname(ol, fx->fx_srcpath, ln->ln_name, isfunc);

		if (en != NULL) {
			if (en->en_isfunc != isfunc && en->en_file != NULL) {
				errf("%s defined in %s and %s", ln->ln_name,
				       fx->fx_srcpath, en->en_file->fx_srcpath);
				ol->ol_failed = TRUE;
			}
		}
		else {
			en = push_extname(ol, (const char *)NULL,
							ln->ln_name, isfunc);
		}

		en->en_file = fx;
		en->en_addr = ln->ln_addr;

		if (p_en != NULL)
			*p_en++ = en;
	}
}

static int
get_header(fp, path, cx, p_size)
FILE *fp;
const char *path;
cx_header_t *cx;
off_t *p_size;
{
	size_t nwords;
	int nread;

	if ((nread = fread((char *)cx, 1, sizeof(cx_header_t), fp)) == 0) {
		if (ferror(fp) || !feof(fp)) {
			failmesg("Error reading from", "", path);
			return -1;
		}
		*p_size = 0;
		return 0;
	}

	if (nread != sizeof(cx_header_t)) {
		errf("Unexpected EOF reading from %s", path);
		return -1;
	}

	if (cx->cx_magic != OCX_MAGIC) {
		errf("%s is not .ocx file (magic=0x%x, expected 0x%x)",
					path, cx->cx_magic, OCX_MAGIC);
		return -1;
	}

	if (cx->cx_version != OCX_VERSION) {
		errf("%s has bad format version %d - expected %d",
					path, cx->cx_version, OCX_VERSION);
		return -1;
	}

	nwords = cx->cx_nfuncrefs + cx->cx_nvarrefs +
		 cx->cx_nfuncaddrs + cx->cx_n_static_relocs +
		 cx->cx_n_static_funcrelocs + cx->cx_n_extdata_funcrelocs +
	         2 * (cx->cx_nfuncs + cx->cx_nvars +
		      cx->cx_n_extdata_relocs + cx->cx_n_ext_relocs);

	*p_size = sizeof(cx_header_t) + nwords * sizeof(size_t) +
		  cx->cx_strings_size + cx->cx_extdata_size +
		  cx->cx_text_size + cx->cx_data_size + cx->cx_symtab_size;
	
	return 0;
}

linklist_t *
ci_read_cx_file(path, read_text_etc)
const char *path;
bool read_text_etc;
{
	FILE *fp;
	alloc_pool_t *ap;
	int res;
	off_t size;
	linklist_t *ll;
	cx_header_t *cx;
	struct stat stbuf;

	if ((fp = fopen(path, "r")) == NULL) {
		failmesg("Can't open", "", path);
		return NULL;
	}

	if (fstat(fileno(fp), &stbuf) != 0) {
		failmesg("Can't fstat", "", path);
		fclose(fp);
		return NULL;
	}

	ap = alloc_create_pool();
	cx = (cx_header_t *)alloc(ap, sizeof(cx_header_t));

	if ((res = get_header(fp, path, cx, &size)) != 0 || size == 0) {
		if (res == 0 && size == 0)
			errf("%s is a zero length file", path);
		fclose(fp);
		alloc_free_pool(ap);
		return NULL;
	}

	if (stbuf.st_size == size) {
		res = read_cx(ap, fp, path, cx, read_text_etc, &ll);
	}
	else if (stbuf.st_size > size) {
		res = read_library(ap, fp, path, cx, read_text_etc, &ll);
	}
	else {
		errf("File %s is only %ld bytes long (expected %ld bytes)",
						path, stbuf.st_size, size);	
		res = -1;
	}

	if (fclose(fp) == EOF && res == 0) {
		failmesg("Error closing", "", path);
		res = -1;
	}

	if (res != 0) {
		alloc_free_pool(ap);
		fclose(fp);
		return NULL;
	}

	return ll;
}

linklist_t *
ci_make_linklist(li, cx)
linkinfo_t *li;
cx_header_t *cx;
{
	linklist_t *ll;

	ll = (linklist_t *)alloc(li->li_apool, sizeof(linklist_t));

	ll->ll_offset = 0;
	ll->ll_linkinfo = li;
	ll->ll_cx = cx;
	ll->ll_next = NULL;

	return ll;
}

static int
read_library(ap, fp, path, cx, read_text_etc, p_ll)
alloc_pool_t *ap;
FILE *fp;
const char *path;
cx_header_t *cx;
bool read_text_etc;
linklist_t **p_ll;
{
	linklist_t *head, *tail;
	off_t size, offset;

	head = tail = NULL;
	offset = 0;

	do {
		linklist_t *ll;
		char *pathstr;

		if (read_cx(ap, fp, path, cx, read_text_etc, &ll) != 0)
			return -1;
		
		ll->ll_offset = offset;

		if (tail == NULL)
			head = ll;
		else
			tail->ll_next = ll;
		tail = ll;

		offset = ftell(fp);
		pathstr = strf("%s at offset %ld", path, offset);

		cx = (cx_header_t *)alloc(ap, sizeof(cx_header_t));

		if (get_header(fp, pathstr, cx, &size) != 0) {
			free(pathstr);
			return -1;
		}
		free(pathstr);

	} while (size != 0);

	/*  At this point we have an unused cx struct.  We just forget
	 *  about it.
	 */
	*p_ll = head;
	return 0;
}

static int
read_cx(ap, fp, path, cx, read_text_etc, p_ll)
alloc_pool_t *ap;
FILE *fp;
const char *path;
cx_header_t *cx;
bool read_text_etc;
linklist_t **p_ll;
{
	size_t offset;
	int i;
	linkinfo_t *li;
	libfunc_t first_undef_func, *last_undef_func;
	linkname_t first_undef_var, *last_undef_var;
	char *strings;

	li = (linkinfo_t *)alloc(ap, sizeof(linkinfo_t));
	li->li_apool = ap;

	strings = allocstr(ap, cx->cx_strings_size);

	li->li_path = alloc_strdup(ap, path);
	li->li_bss_size = cx->cx_bss_size;
	li->li_extbss_size = cx->cx_extbss_size;
	li->li_srcpath = strings + cx->cx_srcpath;

	last_undef_func = &first_undef_func;
	for (i = 0; i < cx->cx_nfuncrefs; ++i) {
		libfunc_t *lf;

		if (readfp(fp, path, (char *)&offset, sizeof(offset)) != 0)
			return -1;
		lf = (libfunc_t *)alloc(ap, sizeof(libfunc_t));
		lf->lf_name = strings + offset;
		lf->lf_index = -(i + 1);
		lf->lf_addr = 0;

		last_undef_func->lf_next = lf;
		last_undef_func = lf;
	}
	last_undef_func->lf_next = NULL;
	li->li_undef_funcs = first_undef_func.lf_next;

	last_undef_var = &first_undef_var;
	for (i = 0; i < cx->cx_nvarrefs; ++i) {
		linkname_t *ln;

		if (readfp(fp, path, (char *)&offset, sizeof(offset)) != 0)
			return -1;
		ln = (linkname_t *)alloc(ap, sizeof(linkname_t));
		ln->ln_name = strings + offset;
		ln->ln_addr = 0;

		last_undef_var->ln_next = ln;
		last_undef_var = ln;
	}
	last_undef_var->ln_next = NULL;
	li->li_undef_vars = first_undef_var.ln_next;

	if (read_linknames(ap, fp, path, strings, cx->cx_nfuncs,
							&li->li_funcs) != 0)
		return -1;
	if (read_linknames(ap, fp, path, strings, cx->cx_nvars,
							&li->li_vars) != 0)
		return -1;
	
	if (readfp(fp, path, strings, cx->cx_strings_size) != 0)
		return -1;
	
	li->li_extdata = alloc(ap, cx->cx_extdata_size);
	if (readfp(fp, path, li->li_extdata, cx->cx_extdata_size) != 0)
		return -1;
	li->li_extdata_size = cx->cx_extdata_size;

	if (read_ext_relocs(ap, fp, path, cx->cx_n_extdata_relocs,
						   &li->li_extdata_relocs) != 0)
		return -1;

	if (read_static_relocs(ap, fp, path, cx->cx_n_extdata_funcrelocs,
						   &li->li_extdata_funcrelocs) != 0)
		return -1;


	li->li_text_size = cx->cx_text_size;
	li->li_data_size = cx->cx_data_size;
	li->li_nfuncaddrs = cx->cx_nfuncaddrs;

	/*  Don't need text, data or funcaddrs or relocation info
	 *  for linking.
	 */
	if (read_text_etc) {
		li->li_text = (unsigned char *)alloc(ap, li->li_text_size);
		if (readfp(fp, path, (char *)li->li_text,li->li_text_size) != 0)
			return -1;

		li->li_data = alloc(ap, li->li_data_size);
		if (readfp(fp, path, li->li_data, li->li_data_size) != 0)
			return -1;

		if (read_static_relocs(ap, fp, path, cx->cx_n_static_relocs,
						       &li->li_static_rps) != 0)
			return -1;

		if (read_static_relocs(ap, fp, path, cx->cx_n_static_funcrelocs,
						   &li->li_static_funcrps) != 0)
			return -1;

		if (read_ext_relocs(ap, fp, path, cx->cx_n_ext_relocs,
						       &li->li_ext_relocs) != 0)
			return -1;

		li->li_funcaddrs = (taddr_t *)alloc(ap, cx->cx_nfuncaddrs *
							     sizeof(taddr_t));
		if (readfp(fp, path, (char *)li->li_funcaddrs,
				     cx->cx_nfuncaddrs * sizeof(taddr_t)) != 0)
			return -1;
	}
	else {
		off_t nbytes;

		nbytes = li->li_text_size + li->li_data_size + 
			 cx->cx_n_static_relocs * sizeof(size_t) +
			 cx->cx_n_static_funcrelocs * sizeof(size_t) +
			 cx->cx_n_ext_relocs * 2 * sizeof(size_t) +
			 li->li_nfuncaddrs * sizeof(size_t);
		
		if (fseek(fp, nbytes, SEEK_CUR) == -1) {
			errf("Can't seek forwards %ld bytes in %s: %s",
			     nbytes, path, get_errno_str());
			return -1;
		}

	}

	if (cx->cx_symtab_size != 0) {
		if (read_symtab(fp, path, cx->cx_symtab_size, li) != 0)
			return -1;
	}
	else {
		li->li_symfuncs = NULL;
		li->li_nsymfuncs = 0;
		
		li->li_symvars = NULL;
		li->li_nsymvars = 0;
	}

	*p_ll = ci_make_linklist(li, cx);
	return 0;
}

int
ci_write_cx_file(li, cx, want_syminfo, path)
linkinfo_t *li;
cx_header_t *cx;
bool want_syminfo;
const char *path;
{
	size_t i, nfuncrefs, nvarrefs, count;
	libfunc_t *lf;
	linkname_t *ln;
	size_t strings_size;
	FILE *fp;
	ebuf_t *str_eb;

	if ((fp = fopen(path, "w")) == NULL) {
		failmesg("Can't create", "", path);
		return -1;
	}

	if (fseek(fp, sizeof(cx_header_t), SEEK_SET) != 0) {
		failmesg("Can't seek in", "", path);
		fclose(fp);
		return -1;
	}

	str_eb = ebuf_create(TRUE);

	cx->cx_magic = OCX_MAGIC;
	cx->cx_version = OCX_VERSION;
	cx->cx_bss_size = li->li_bss_size;
	cx->cx_extbss_size = li->li_extbss_size;
	cx->cx_srcpath = note_string(str_eb, li->li_srcpath);

	/*  Write out the stuff used by the linker.
	 */
	nfuncrefs = 0;
	for (lf = li->li_undef_funcs; lf != NULL; lf = lf->lf_next) {
		if (-lf->lf_index != nfuncrefs + 1)
			panic("func index botch in wcf");

		if (save_string(str_eb, lf->lf_name, fp) != 0)
			goto bad;

		++nfuncrefs;
	}
	cx->cx_nfuncrefs = nfuncrefs;

	nvarrefs = 0;
	for (ln = li->li_undef_vars; ln != NULL; ln = ln->ln_next) {
		if (save_string(str_eb, ln->ln_name, fp) != 0)
			goto bad;
		++nvarrefs;
	}
	cx->cx_nvarrefs = nvarrefs;

	if (write_linknames(fp, str_eb, li->li_funcs, &cx->cx_nfuncs) != 0)
		goto bad;
	if (write_linknames(fp, str_eb, li->li_vars, &cx->cx_nvars) != 0)
		goto bad;

	if (write_strings(str_eb, fp, &strings_size) != 0)
		goto bad;
	cx->cx_strings_size = strings_size;

	if (fp_write(fp, li->li_extdata, li->li_extdata_size) != 0)
		goto bad;
	cx->cx_extdata_size = li->li_extdata_size;

	if (write_ext_relocs(fp, li->li_extdata_relocs, &count) != 0)
		goto bad;
	cx->cx_n_extdata_relocs = count;
	
	if (write_static_relocs(fp, li->li_extdata_funcrelocs, &count) != 0)
		goto bad;
	cx->cx_n_extdata_funcrelocs = count;


	/*  Write out the stuff read by load_text().
	 */
	if (fp_write(fp, (char *)li->li_text, li->li_text_size) != 0)
		goto bad;
	cx->cx_text_size = li->li_text_size;

	if (fp_write(fp, li->li_data, li->li_data_size) != 0)
		goto bad;
	cx->cx_data_size = li->li_data_size;

	if (write_static_relocs(fp, li->li_static_rps, &count) != 0)
		goto bad;
	cx->cx_n_static_relocs = count;

	if (write_static_relocs(fp, li->li_static_funcrps, &count) != 0)
		goto bad;
	cx->cx_n_static_funcrelocs = count;

	if (write_ext_relocs(fp, li->li_ext_relocs, &count) != 0)
		goto bad;
	cx->cx_n_ext_relocs = count;

	for (i = 0; i < li->li_nfuncaddrs; ++i) {
		if (fp_write_val(fp, (size_t)li->li_funcaddrs[i]) != 0)
			goto bad;
	}
	cx->cx_nfuncaddrs = li->li_nfuncaddrs;

	if (want_syminfo) {
		ebuf_t *symstr_eb;
		int res;
		long symtab_size;

		symstr_eb = ebuf_create(TRUE);
		res = write_symtab(fp, path, symstr_eb, li, &symtab_size);
		cx->cx_symtab_size = symtab_size;

		ebuf_free(symstr_eb);

		if (res != 0)
			goto bad;
	}
	else {	
		cx->cx_symtab_size = 0;
	}

	if (fseek(fp, 0, SEEK_SET) != 0) {
		failmesg("Can't seek to beginning of", "", path);
		fclose(fp);
		ebuf_free(str_eb);
		return -1;
	}

	if (fp_write(fp, (char *)cx, sizeof(cx_header_t)) != 0)
		goto bad;
		
	if (ferror(fp) || fflush(fp) == EOF)
		goto bad;
	
	fclose(fp);
	ebuf_free(str_eb);

	return 0;
	
bad:
	failmesg("Error writing to", "", path);
	fclose(fp);
	ebuf_free(str_eb);
	return -1;
}

static int
write_ext_relocs(fp, erlist, p_count)
FILE *fp;
ext_reloc_t *erlist;
size_t *p_count;
{
	size_t count;
	ext_reloc_t *er;

	count = 0;
	for (er = erlist; er != NULL; er = er->er_next) {
		if (fp_write_val(fp, (size_t)er->er_location) ||
		    fp_write_val(fp, (size_t)er->er_index) != 0)
			return -1;
		++count;
	}

	*p_count = count;
	return 0;
}

static int
write_static_relocs(fp, rplist, p_count)
FILE *fp;
reloc_pos_t *rplist;
size_t *p_count;
{
	size_t count;
	reloc_pos_t *rp;

	count = 0;
	for (rp = rplist; rp != NULL; rp = rp->rp_next) {
		if (fp_write_val(fp, (size_t)rp->rp_location) != 0)
			return -1;
		++count;
	}

	*p_count = count;
	return 0;
}

static int
read_linknames(ap, fp, path, strings, count, p_lnlist)
alloc_pool_t *ap;
FILE *fp;
const char *path, *strings;
size_t count;
linkname_t **p_lnlist;
{
	int i;
	linkname_t first, *last;

	last = &first;

	for (i = 0; i < count; ++i) {
		size_t words[2];
		linkname_t *ln;
		
		if (readfp(fp, path, (char *)words, sizeof(words)) != 0)
			return -1;
		
		ln = (linkname_t *)alloc(ap, sizeof(linkname_t));
		ln->ln_name = strings + words[0];
		ln->ln_addr = words[1];

		last->ln_next = ln;
		last = ln;
	}
	last->ln_next = NULL;

	*p_lnlist = first.ln_next;
	return 0;
}

static int
write_linknames(fp, str_eb, ln, p_count)
FILE *fp;
ebuf_t *str_eb;
linkname_t *ln;
size_t *p_count;
{
	size_t count;

	count = 0;
	for (; ln != NULL; ln = ln->ln_next) {
		if (save_string(str_eb, ln->ln_name, fp) != 0)
			return -1;
		if (fp_write_val(fp, (size_t)ln->ln_addr) != 0)
			return -1;
		++count;
	}

	*p_count = count;
	return 0;
}

void
ci_free_linklist(ll)
linklist_t *ll;
{
	alloc_free_pool(ll->ll_linkinfo->li_apool);
}
