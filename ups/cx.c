/* cx.c - standalone driver for the ups C interpreter */

/*  Copyright 1991 Mark Russell, University of Kent at Canterbury.
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


/* @(#)cx.c	1.16 12/22/93 (UKC) */
char ups_cx_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "xc_load.h"
#include "cx_link.h"
#include "cc.h"

int main PROTO((int argc, char **argv));
static char *get_opath PROTO((const char *srcpath, bool want_ccpaths,
					bool want_asm, bool *p_is_source));

static const char Extra_usage[] = "\
[-asm] [-S] [-c] [-o objpath] [-ccpaths] [-verboselink] file [file ...]";

int
main(argc, argv)
int argc;
char **argv;
{
	const char *linkfile;
	bool want_asm, want_link, want_asmsrc, want_ccpaths, many;
	bool verbose_link, want_syminfo;
	link_id_t link_id;
	ccstate_t *cs;

	errf_set_progname(*argv++);

	want_asmsrc = FALSE;	/* to satify gcc */

	cs = cc_create_ccstate(Extra_usage, (unsigned)0);
	
	want_asm = FALSE;			/* -asm, -S */
	want_link = TRUE;			/* -c */
	linkfile = "cx.out";			/* -o linkfile */
	want_ccpaths = FALSE;			/* -ccpaths */
	verbose_link = FALSE;			/* -verboselink */

	for (; *argv != NULL && **argv == '-'; ++argv) {
		const char *flag;

		if (cc_handle_arg(cs, &argv))
			continue;
		
		flag = *argv + 1;

		if (strcmp(flag, "ccpaths") == 0) {
			want_ccpaths = TRUE;
		}
		else if (strcmp(flag, "o") == 0) {
			linkfile = cc_checkarg(cs, ++argv);
		}
		else if (strcmp(flag, "S") == 0) {
			want_asm = TRUE;
			want_asmsrc = TRUE;
			want_link = FALSE;
		}
		else if (strcmp(flag, "asm") == 0) {
			want_asm = TRUE;
			want_asmsrc = FALSE;
			want_link = FALSE;
		}
		else if (strcmp(flag, "c") == 0) {
			want_link = FALSE;
		}
		else if (strcmp(flag, "verboselink") == 0) {
			verbose_link = TRUE;
		}
		else {
			errf("Unknown flag -%s", flag);
			errf_usage(cc_get_usage(cs));
		}
	}

	want_syminfo = cc_get_syminfo_flag(cs);

	if (want_link) {
		if ((link_id = ci_make_link_id()) == NULL)
			exit(1);
	}
	else {
		link_id = NULL;
	}

	many = argv[0] != NULL && argv[1] != NULL;

	for (; *argv != NULL; ++argv) {
		const char *srcpath;
		char *opath;
		bool is_source;
		linklist_t *ll;
		parse_id_t parse_id;

		srcpath = *argv;

		opath = get_opath(srcpath, want_asm, want_ccpaths, &is_source);

		if (many && (is_source || want_asm))
			printf("%s -> %s\n", srcpath, opath);

		if (is_source) {
			cx_header_t cxbuf;
			linkinfo_t *li;

			li = cc_parse_and_compile(cs, srcpath, &parse_id, NULL);
			
			if (li == NULL || ci_write_cx_file(li, &cxbuf,
						    want_syminfo, opath) != 0) {
				ll = NULL;
			}
			else {
				li->li_path = opath;
				ll = ci_make_linklist(li, &cxbuf);
			}
		}
		else {
			ll = ci_read_cx_file(srcpath, want_asm);
			parse_id = NULL;
		}

		if (ll == NULL)
			exit(1);
			
		if (want_asm) {
			if (ci_write_asm_file(ll, opath) != 0)
				exit(1);
		}
		else if (link_id != NULL) {
			ci_add_linklist(link_id, ll, opath, verbose_link);
		}
		
		ci_free_linklist(ll);
		free(opath);

		if (parse_id != NULL)
			ci_free_parse_id(parse_id);
	}

	if (want_link) {
		size_t n_libfuncs, n_libvars;
		
		if (many)
			printf("Linking -> %s\n", linkfile);

		cc_get_libinfo(&n_libfuncs, &n_libvars);
		
		if (ci_link(link_id, want_syminfo, linkfile,
				      cc_getaddr, n_libfuncs, n_libvars) != 0) {
			exit(1);
		}
	}
	
	exit(0);
	return 0;	/* to satisfy gcc */
}

static char *
get_opath(srcpath, want_asm, want_ccpaths, p_is_source)
const char *srcpath;
bool want_asm, want_ccpaths;
bool *p_is_source;
{
	const char *srcname, *objpath, *suf;
	int len;

	if ((srcname = strrchr(srcpath, '/')) != NULL)
		++srcname;
	else
		srcname = srcpath;

	objpath = srcpath;

	if (want_ccpaths)
		srcpath = srcname;
	
	if ((suf = strrchr(srcname, '.')) != NULL) {
		len = suf - srcpath;
		++suf;
	}
	else {
		len = strlen(srcpath);
	}

	*p_is_source = suf != NULL && strcmp(suf, "c") == 0;

	if (want_asm)
		return strf("%.*s.s", len, srcpath);

	if (!*p_is_source)
		return strsave(objpath);
	
	return strf("%.*s.%s", len, srcpath, want_ccpaths ? "o" : "ocx");
}

/* (IDE) Dummy so 'cx' will build. */
void
demangle_name_2(name, len, alloc_id, ptr, func, fil)
     char *name;
     int len;
     alloc_pool_t *alloc_id;
     char **ptr;
     int func;
     fil_t *fil;
{
  *ptr = strdup (name);
}

