/* xc_load.c - load a cx `binary' for execution */

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


/* @(#)xc_load.c	1.5 16 Sep 1994 (UKC) */
char ups_xc_load_c_rcsid[] = "$Id$";

#include <sys/types.h>
#include <fcntl.h>

#ifdef __STDC__
#include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <local/ukcprog.h>
#include <mtrprog/utils.h>

#ifndef __STDC__
char *memcpy();
#endif

#include "ci.h"
#include "xc_opcodes.h"
#include "xc_machine.h"
#include "xc_load.h"
#include "xc_builtins.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#endif

static int readfd PROTO((int fd, const char *path, char *buf, size_t nbytes));
static void cvtwords PROTO((alloc_pool_t *ap, size_t *words, size_t nwords,
			    size_t nfiles, size_t nrefs, size_t n_extrefs,
			    size_t extdata_size, size_t extbss_size,
			    size_t n_libvars,
			    size_t n_symfuncs, size_t n_symvars,
			    const char *strings,
			    codefile_t *cftab, extref_t *ertab,
			    funcmap_t *funcmap, char *ext_data_and_bss));
static int load_text PROTO((alloc_pool_t *ap, const char *cwd,
					codefile_t *cftab, codefile_t *cf));
static int check_magic PROTO((const char *path, size_t magic, size_t version));
static void do_relocs PROTO((textword_t *text, char *data, char **varaddrs,
			     textaddr_t *relocs, size_t nrelocs));
static int make_machine PROTO((alloc_pool_t *ap, size_t stack_size,
			       bool want_opcounts,
			       codefile_t *cftab, extref_t *ertab,
			       funcmap_t *funcmap, char *ext_data_and_bss,
			       x_header_t *xh, const char *cwd,
			       char **argv, char **envp,
			       machine_t **p_machine));
static void setup_entry_cf PROTO((alloc_pool_t *ap, codefile_t *cf,
				  x_header_t *xh, extref_t *ertab,
				  startup_info_t *su));
static int open_ocx_file PROTO((const char *cwd, codefile_t *cf,
				cx_header_t *cx, char **p_fullpath));

static int
readfd(fd, path, buf, nbytes)
int fd;
const char *path;
char *buf;
size_t nbytes;
{
	int nread;

	if ((nread = read(fd, buf, nbytes)) == nbytes)
		return 0;

	if (nread >= 0)
		errf("Unexpected EOF reading from %s", path);
	else
		failmesg("Error reading from", "", path);
	
	return -1;
}

static int
check_magic(path, magic, version)
const char *path;
size_t magic, version;
{
	if (magic != OCX_MAGIC) {
		errf("%s is not a cx file (magic=0x%lx, expected 0x%x)",
		     path, (long)magic, OCX_MAGIC);
		return -1;
	}

	if (version != OCX_VERSION) {
		errf("%s has bad format version %ld - expected %d",
					path, (long)version, OCX_VERSION);
		return -1;
	}
			
	return 0;
}

int
ci_load(path, stack_size, want_opcounts, argv, envp, p_machine)
const char *path;
size_t stack_size;
bool want_opcounts;
char **argv, **envp;
machine_t **p_machine;
{
	int fd;
	size_t nwords;
	alloc_pool_t *ap;
	x_header_t xhbuf;
	codefile_t *cftab;
	extref_t *ertab;
	size_t *words;
	char *strings, *ext_data_and_bss;
	funcmap_t *funcmap;

	if ((fd = open(path, O_RDONLY)) == -1) {
		failmesg("Can't open", "file", path);
		return -1;
	}

	if (readfd(fd, path, (char *)&xhbuf, sizeof(xhbuf)) != 0 ||
	    check_magic(path, xhbuf.xh_magic, xhbuf.xh_version) != 0) {
		close(fd);
		return -1;
	}

	ap = alloc_create_pool();

	if (xhbuf.xh_extdata_size % sizeof(size_t) != 0)
		panic("eds botch in load");

	nwords = xhbuf.xh_nfiles * 11 +
		 xhbuf.xh_nrefs +
		 xhbuf.xh_n_extrefs * 2 +
		 xhbuf.xh_extdata_size / sizeof(size_t) +
		 xhbuf.xh_n_extdata_relocs * 2 +
		 xhbuf.xh_n_extdata_funcrelocs +
		 xhbuf.xh_nsymfuncs * 3 + xhbuf.xh_nsymvars;

	strings = allocstr(ap, xhbuf.xh_strings_size);
	words = (size_t *)e_malloc(nwords * sizeof(size_t));
	
	if (readfd(fd, path, (char *)words, nwords * sizeof(size_t)) != 0 ||
	    readfd(fd, path, strings, xhbuf.xh_strings_size) != 0) {
		alloc_free_pool(ap);
		free((char *)words);
		close(fd);
		return -1;
	}

	close(fd);

	/*  We allocate an extra codefile slot for the entry codefile (built
	 *  by setup_entry_cf()).  pack_textloc() in xc_text.c requires
	 *  that all codefile_t pointers are indexes into the ma_cftab array.
	 */
	cftab = (codefile_t *)alloc(ap,
				    sizeof(codefile_t) * (xhbuf.xh_nfiles + 1));
	ertab = (extref_t *)alloc(ap,
				  sizeof(extref_t) * xhbuf.xh_n_extrefs);
	funcmap = (funcmap_t *)alloc(ap,
				     sizeof(funcmap_t) * xhbuf.xh_nsymfuncs);
	ext_data_and_bss = alloc(ap, xhbuf.xh_extdata_size +
				     xhbuf.xh_extbss_size);

	cvtwords(ap, words, nwords, xhbuf.xh_nfiles, xhbuf.xh_nrefs,
				xhbuf.xh_n_extrefs, xhbuf.xh_extdata_size,
				xhbuf.xh_extbss_size, xhbuf.xh_n_libvars,
				xhbuf.xh_nsymfuncs, xhbuf.xh_nsymvars, strings,
				cftab, ertab, funcmap, ext_data_and_bss);
	
	memset(words, 0x99, nwords * sizeof(size_t));	/* for safety */
	free((char *)words);

	return make_machine(ap, stack_size, want_opcounts,
			    cftab, ertab, funcmap, ext_data_and_bss,
			    &xhbuf, &strings[xhbuf.xh_cwd],
			    argv, envp, p_machine);
}

static void
cvtwords(ap, words, nwords, nfiles, nrefs, n_extrefs,
		extdata_size, extbss_size, n_libvars, n_symfuncs, n_symvars,
		strings, cftab, ertab, funcmap, ext_data_and_bss)
alloc_pool_t *ap;
size_t *words;
size_t nwords, nfiles, nrefs, n_extrefs;
size_t extdata_size, extbss_size, n_libvars, n_symfuncs, n_symvars;
const char *strings;
codefile_t *cftab;
extref_t *ertab;
funcmap_t *funcmap;
char *ext_data_and_bss;
{
	typedef struct {
		int fi_nfuncrefs;
		int fi_nvarrefs;
		size_t *fi_extdata_relocs;
		int fi_n_extdata_relocs;
		size_t *fi_extdata_funcrelocs;
		int fi_n_extdata_funcrelocs;
		char *fi_extdata;
		size_t fi_extdata_size;
		char *fi_extbss;
		size_t fi_extbss_size;
		int *fi_var_indices;
	} fileinfo_t;
	fileinfo_t *fi, *filim, *fitab;
	codefile_t *cf, *cflim;
	extref_t *er, *erlim;
	const char **globalnames, **varnames;
	char **varaddrs, **libvars;
	char *extdata, *extbss;
	alloc_pool_t *tmp_ap;
	size_t *w;
	int i;

	tmp_ap = alloc_create_pool();
	libvars = cx_get_libvars(n_libvars);
	fitab = (fileinfo_t *)alloc(tmp_ap, nfiles * sizeof(fileinfo_t));
	filim = fitab + nfiles;

	varnames = (const char **)alloc(ap, n_symvars * sizeof(const char *));
	globalnames = varnames;

	cflim = cftab + nfiles;
	erlim = ertab + n_extrefs;

	w = words;
	for (cf = cftab, fi = fitab; cf < cflim; ++cf, ++fi) {
		cf->cf_path = strings + *w++;
		cf->cf_srcpath = strings + *w++;
		cf->cf_offset = *w++;
		cf->cf_loaded = FALSE;
		cf->cf_text_and_data = NULL;
		cf->cf_text_size = *w++;
		cf->cf_data_size = 0;	/* set on loading */
		cf->cf_bss_size = 0;	/* set on loading */
		cf->cf_nglobalnames = *w++;
		cf->cf_globalnames = globalnames;
		globalnames += cf->cf_nglobalnames;

		fi->fi_nfuncrefs = *w++;
		fi->fi_nvarrefs = *w++;
		fi->fi_extdata_size = *w++;
		fi->fi_extbss_size = *w++;
		fi->fi_n_extdata_relocs = *w++;
		fi->fi_n_extdata_funcrelocs = *w++;
	}

	for (cf = cftab, fi = fitab; cf < cflim; ++cf, ++fi) {
		extref_t **funcrefs;
		
		funcrefs = (extref_t **)alloc(ap,
				         fi->fi_nfuncrefs * sizeof(extref_t *));
		cf->cf_varaddrs = (char **)alloc(ap,
					  fi->fi_nvarrefs * sizeof(char *));
		fi->fi_var_indices = (int *)alloc(tmp_ap,
					  fi->fi_nvarrefs * sizeof(int));

		cf->cf_funcrefs = funcrefs + fi->fi_nfuncrefs;
		for (i = 0; i < fi->fi_nfuncrefs; ++i)
			cf->cf_funcrefs[-(i + 1)] = &ertab[*w++];
	
		for (i = 0; i < fi->fi_nvarrefs; ++i)
			fi->fi_var_indices[i] = *w++;	
	}

	extdata = memcpy(ext_data_and_bss, (char *)w, extdata_size);
	extbss = extdata + extdata_size + extbss_size;
	memset(extbss - extbss_size, '\0', extbss_size);

	for (fi = fitab; fi < filim; ++fi) {
		fi->fi_extdata = extdata;
		extdata += fi->fi_extdata_size;

		fi->fi_extbss = extbss;
		extbss -= fi->fi_extbss_size;
	}

	if (extbss != extdata)
		panic("extbss botch");
	w = (size_t *)((char *)w + extdata_size);

	for (fi = fitab; fi < filim; ++fi) {
		fi->fi_extdata_relocs = w;
		w += fi->fi_n_extdata_relocs * 2;
	}

	for (fi = fitab; fi < filim; ++fi) {
		fi->fi_extdata_funcrelocs = w;
		w += fi->fi_n_extdata_funcrelocs;
	}

	varaddrs = (char **)e_malloc(sizeof(char *) * n_extrefs);

	for (er = ertab; er < erlim; ++er) {
		long offset;
		int file_index;
		char *addr;

		file_index = *w++;
		er->er_file = (file_index == -1) ? NULL : &cftab[file_index];
		er->er_indirect = FALSE;
		er->er_addr = offset = *w++;

		if (er->er_file == NULL)
			addr = libvars[offset];
		else if (offset < 0)
			addr = fitab[file_index].fi_extbss + offset;
		else
			addr = fitab[file_index].fi_extdata + offset;
		varaddrs[er - ertab] = addr;
	}

	if (n_symfuncs != 0) {
		funcmap_t *fm, *fmlim;

		fmlim = funcmap + n_symfuncs;
		for (fm = funcmap; fm < fmlim; ++fm) {
			fm->fm_name = strings + *w++;
			fm->fm_addr = *w++;
			fm->fm_file_index = *w++;
		}
	}

	if (n_symvars != 0) {
		const char **sv, **svlim;

		svlim = varnames + n_symvars;
		for (sv = varnames; sv < svlim; ++sv)
			*sv = strings + *w++;
	}

	if (w - words != nwords)
		panic("nwords botch in cw");

	for (cf = cftab, fi = fitab; cf < cflim; ++cf, ++fi) {
		for (i = 0; i < fi->fi_nvarrefs; ++i)
			cf->cf_varaddrs[i] = varaddrs[fi->fi_var_indices[i]];	
	}

	free(varaddrs);

	for (cf = cftab, fi = fitab; cf < cflim; ++cf, ++fi) {
		size_t *rp;

		rp = fi->fi_extdata_relocs;
		for (i = 0; i < fi->fi_n_extdata_relocs; ++i) {
			unsigned long *loc;
			char *offset;
			int var_index;

			loc = (unsigned long *)(fi->fi_extdata + *rp++);

			if ((var_index = *rp++) == -1)
				offset = fi->fi_extdata;
			else
				offset = cf->cf_varaddrs[var_index];

			*loc += (unsigned long)offset;
		}
	}

	for (cf = cftab, fi = fitab; cf < cflim; ++cf, ++fi) {
		size_t *rp;

		rp = fi->fi_extdata_funcrelocs;
		for (i = 0; i < fi->fi_n_extdata_funcrelocs; ++i) {
			unsigned long *loc;

			loc = (unsigned long *)(fi->fi_extdata + *rp++);
			*loc |= (cf - cftab) << 16;
		}
	}

	alloc_free_pool(tmp_ap);
}

int
ci_read_symtab_bytes(ap, cwd, cf, p_symtab_buf, p_nbytes)
alloc_pool_t *ap;
const char *cwd;
codefile_t *cf;
char **p_symtab_buf;
size_t *p_nbytes;
{
	char *path;
	cx_header_t cxbuf;
	int fd;
	char *buf;
	off_t nbytes;

	if ((fd = open_ocx_file(cwd, cf, &cxbuf, &path)) == -1)
		return -1;

	buf = NULL;

	nbytes = cxbuf.cx_text_size + cxbuf.cx_data_size +
	         (cxbuf.cx_nfuncaddrs + cxbuf.cx_n_static_relocs +
		 cxbuf.cx_n_static_funcrelocs +
		 cxbuf.cx_n_ext_relocs * 2) * sizeof(size_t);

	if (lseek(fd, nbytes, SEEK_CUR) == -1) {
		errf("Can't seek forwards %ld bytes in text file %s: %s",
		     nbytes, path, get_errno_str());
		goto bad;
	}

	buf = e_malloc(cxbuf.cx_symtab_size);

	if (readfd(fd, path, buf, cxbuf.cx_symtab_size) != 0)
		goto bad;

	if (close(fd) != 0) {
		failmesg("Error closing", "", path);
		goto bad;
	}

	if (path != NULL)
		free(path);

	/*  The symbol table stuff needs cf_data to be set the right value
	 *  so it can correctly set va_addr for internal non-auto variables.
	 */
	if (cf->cf_text_and_data == NULL) {
		cf->cf_text_and_data = (textword_t *)alloc(ap,
							   cxbuf.cx_text_size +
							   cxbuf.cx_data_size +
							   cxbuf.cx_bss_size);
	}

	*p_symtab_buf = buf;
	*p_nbytes = cxbuf.cx_symtab_size;
	return 0;

bad:
	if (path != NULL)
		free(path);
	if (buf != NULL)
		free(buf);
	close(fd);
	return -1;
}


static int
open_ocx_file(cwd, cf, cx, p_fullpath)
const char *cwd;
codefile_t *cf;
cx_header_t *cx;
char **p_fullpath;
{
	const char *path;
	char *fullpath;
	int fd;
	off_t offset;

	path = cf->cf_path;
	
	if (*path != '/')
		path = fullpath = strf("%s/%s", cwd, path);
	else
		fullpath = NULL;

	if ((fd = open(path, O_RDONLY)) == -1) {
		errf("Can't open %s to load text: %s", path, get_errno_str());
		goto bad;
	}

	if (cf->cf_offset != 0 && lseek(fd, cf->cf_offset, SEEK_SET) == -1) {
		errf("Can't seek to offset %ld in %s: %s",
		     cf->cf_offset, path, get_errno_str());
		goto bad;
	}

	if (readfd(fd, path, (char *)cx, sizeof(*cx)) != 0 ||
	    check_magic(path, cx->cx_magic, cx->cx_version) != 0) {
		goto bad;
	}

	if (cf->cf_text_size != cx->cx_text_size) {
		errf("Text size mismatch in %s - expected %ld, found %ld",
		     path, (long)cf->cf_text_size, (long)cx->cx_text_size);
		goto bad;
	}
	
	offset = cf->cf_offset + sizeof(cx_header_t) +
		 (cx->cx_nfuncrefs + cx->cx_nvarrefs +
	          cx->cx_nfuncs * 2 + cx->cx_nvars * 2 +
		  cx->cx_n_extdata_relocs * 2 +
		  cx->cx_n_extdata_funcrelocs) * sizeof(size_t) +
		 cx->cx_strings_size + cx->cx_extdata_size;

	if (lseek(fd, offset, SEEK_SET) == -1) {
		errf("Can't seek to %ld in text file %s: %s",
		     offset, path, get_errno_str());
		goto bad;
	}

	*p_fullpath = fullpath;
	return fd;

bad:
	if (fd != -1)
		close(fd);
	if (fullpath != NULL)
		free(fullpath);
	return -1;
}

static int
load_text(ap, cwd, cftab, cf)
alloc_pool_t *ap;
const char *cwd;
codefile_t *cftab;
codefile_t *cf;
{
	cx_header_t cxbuf;
	int fd, i;
	char *data, *text, *path;
	size_t fbytes, nbytes;
	textaddr_t *static_relocs, *static_funcrelocs, *ext_relocs;

	if (cf->cf_loaded)
		panic("dup load in lt");
	
	if ((fd = open_ocx_file(cwd, cf, &cxbuf, &path)) == -1)
		return -1;

	nbytes = cxbuf.cx_text_size + cxbuf.cx_data_size;

	if (cf->cf_text_and_data == NULL) {
		cf->cf_text_and_data = (textword_t *)alloc(ap,
						  nbytes + cxbuf.cx_bss_size);
	}
	text = (char *)cf->cf_text_and_data;

	memset(text + nbytes, '\0', cxbuf.cx_bss_size);

	fbytes = (cxbuf.cx_nfuncaddrs + cxbuf.cx_n_static_relocs +
		  cxbuf.cx_n_static_funcrelocs +
		  cxbuf.cx_n_ext_relocs * 2) * sizeof(size_t);
	static_relocs = (textaddr_t *)alloc(ap, fbytes);

	if (readfd(fd, path, text, nbytes) != 0 ||
	    readfd(fd, path, (char *)static_relocs, fbytes) != 0) {
		if (path != NULL)
			free(path);
		close(fd);
		return -1;
	}
	if (close(fd) != 0) {
		failmesg("Error closing", "", path);
		if (path != NULL)
			free(path);
		return -1;
	}

	free(path);

	data = text + cxbuf.cx_text_size;
	cf->cf_data_size = cxbuf.cx_data_size;
	cf->cf_bss_size = cxbuf.cx_bss_size;

	static_funcrelocs = &static_relocs[cxbuf.cx_n_static_relocs];
	ext_relocs = &static_funcrelocs[cxbuf.cx_n_static_funcrelocs];
	cf->cf_funcaddrs = &ext_relocs[cxbuf.cx_n_ext_relocs * 2];
	cf->cf_num_funcs = cxbuf.cx_nfuncaddrs;


	do_relocs(cf->cf_text_and_data, data, (char **)NULL,
		  		   static_relocs, cxbuf.cx_n_static_relocs);

	do_relocs(cf->cf_text_and_data, data, cf->cf_varaddrs,
		  		   ext_relocs, cxbuf.cx_n_ext_relocs);
	
	for (i = 0; i < cxbuf.cx_n_static_funcrelocs; ++i) {
		char *loc;
		unsigned long val;

		loc = (char *)cf->cf_text_and_data + *static_funcrelocs++;

		if (loc < data) {
			val = GETLONG(loc);
			val |= (cf - cftab) << 16;
			ci_write_text_long((textword_t *)loc, val);
		}
		else {
			val = *(unsigned long *)loc;
			val |= (cf - cftab) << 16;
			*(unsigned long *)loc = val;
		}
	}

	cf->cf_data_copy = alloc(ap, cxbuf.cx_data_size);
	memcpy(cf->cf_data_copy, data, cxbuf.cx_data_size);

	cf->cf_loaded = TRUE;

	return 0;
}

static void
do_relocs(text, data, varaddrs, relocs, nrelocs)
textword_t *text;
char *data;
char **varaddrs;
textaddr_t *relocs;
size_t nrelocs;
{
	size_t i;
	bool is_ext;
	textaddr_t *rp;

	is_ext = varaddrs != NULL;
	rp = relocs;

	for (i = 0; i < nrelocs; ++i) {
		char *loc;
		unsigned long val;

		loc = (char *)text + *rp++;

		if (loc < data) {
			val = GETLONG(loc);
			val += (unsigned long)(is_ext ? varaddrs[*rp++] : data);
			ci_write_text_long((textword_t *)loc, val);
		}
		else {
			val = *(unsigned long *)loc;
			val += (unsigned long)(is_ext ? varaddrs[*rp++] : data);
			*(unsigned long *)loc = val;
		}
	}
}

static int
make_machine(ap, stack_size, want_opcounts,
				cftab, ertab, funcmap, ext_data_and_bss,
				xh, cwd, argv, envp, p_machine)
alloc_pool_t *ap;
size_t stack_size;
bool want_opcounts;
codefile_t *cftab;
extref_t *ertab;
funcmap_t *funcmap;
char *ext_data_and_bss;
x_header_t *xh;
const char *cwd;
char **argv, **envp;
machine_t **p_machine;
{
	machine_t *ma;
	startup_info_t subuf;
	codefile_t *entry_cf;

	entry_cf = &cftab[xh->xh_nfiles];

	setup_entry_cf(ap, entry_cf, xh, ertab, &subuf);

	ma = ci_create_machine(ap, stack_size / sizeof(stackword_t),
			       cftab, xh->xh_nfiles + 1,
			       &cftab[xh->xh_nfiles], subuf.su_start_addr);
	
	ma->ma_data = ext_data_and_bss;
	ma->ma_data_size = xh->xh_extdata_size;
	ma->ma_bss_size = xh->xh_extbss_size;

	ma->ma_data_copy = alloc(ap, ma->ma_data_size);
	memcpy(ma->ma_data_copy, ma->ma_data, ma->ma_data_size);

	ma->ma_libfuncs = cx_get_libfuncs(xh->xh_n_libfuncs);

	ma->ma_loadfunc = load_text;
	ma->ma_cwd = cwd;

	ma->ma_funcmap = funcmap;
	ma->ma_funcmap_size = xh->xh_nsymfuncs;

	ma->ma_argc_pos = entry_cf->cf_text_and_data + subuf.su_argc_pos;
	ma->ma_argv_pos = entry_cf->cf_text_and_data + subuf.su_argv_pos;
	ma->ma_envp_pos = entry_cf->cf_text_and_data + subuf.su_envp_pos;

	ci_initialise_machine(ma, FALSE, want_opcounts, argv, envp);

	*p_machine = ma;
	return 0;
}

static void
setup_entry_cf(ap, cf, xh, ertab, su)
alloc_pool_t *ap;
codefile_t *cf;
x_header_t *xh;
extref_t *ertab;
startup_info_t *su;
{
#define MAIN_INDEX	(-1)
#define EXIT_INDEX	(-2)
	extref_t **funcrefs;
	
	funcrefs = (extref_t **)alloc(ap, 2 * sizeof(extref_t)) + 2;

	funcrefs[MAIN_INDEX] = &ertab[xh->xh_main_sym];
	funcrefs[EXIT_INDEX] = &ertab[xh->xh_exit_sym];

	ci_make_crt0(ap, MAIN_INDEX, EXIT_INDEX, su);

	ertab[xh->xh_setjmp_sym].er_file = cf;
	ertab[xh->xh_setjmp_sym].er_indirect = FALSE;
	ertab[xh->xh_setjmp_sym].er_addr = su->su_setjmp_addr;

	ertab[xh->xh_longjmp_sym].er_file = cf;
	ertab[xh->xh_longjmp_sym].er_indirect = FALSE;
	ertab[xh->xh_longjmp_sym].er_addr = su->su_longjmp_addr;

	cf->cf_funcrefs = funcrefs;
	cf->cf_varaddrs = NULL;
	cf->cf_path = NULL;
	cf->cf_srcpath = "crt0.s";
	cf->cf_loaded = TRUE;
	cf->cf_text_and_data = su->su_text;
	cf->cf_text_size = su->su_text_size;
	cf->cf_data_size = 0;
	cf->cf_bss_size = 0;
	cf->cf_funcaddrs = NULL;
	cf->cf_num_funcs = 0;
	cf->cf_globalnames = NULL;
	cf->cf_nglobalnames = 0;
}
