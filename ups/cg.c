/* cg.c - compile and execute a single .c file */

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


/* @(#)cg.c	1.1 22/12/93 (UKC) */
char ups_cg_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "xc_opcodes.h"
#include "xc_machine.h"
#include "xc_builtins.h"
#include "cc.h"

int main PROTO((int argc, char **argv));
static int noread PROTO((taddr_t addr, voidptr buf, size_t nbytes));
static int nowrite PROTO((taddr_t addr, constvoidptr buf, size_t nbytes));

static int compile_and_execute PROTO((ccstate_t *cs, const char *srcpath,
				      char **argv, char **envp,
				      const char *entry_name));
static ci_nametype_t getaddr PROTO((const char *name, taddr_t *p_addr));

extern char **environ;

static char **Libvars;

int
main(argc, argv)
int argc;
char **argv;
{
	static const char extra_usage[] = "[-e entryfunc] sourcefile [arg ...]";
	ccstate_t *cs;
	const char *entry_name;

	errf_set_progname(*argv);

	entry_name = "main";

	cs = cc_create_ccstate(extra_usage, (unsigned)CI_CP_ONEFILE);
	
	for (++argv; *argv != NULL && **argv == '-'; ++argv) {
		if (cc_handle_arg(cs, &argv))
			continue;
		
		if (strcmp(*argv, "-e") == 0) {
			entry_name = cc_checkarg(cs, ++argv);
		}
		else {
			errf("Unknown flag %s", *argv);
			errf_usage(cc_get_usage(cs));
		}
	}
	
	if (*argv == NULL)
		errf_usage(cc_get_usage(cs));

	if (compile_and_execute(cs, *argv, argv, environ, entry_name) != 0)
		return 1;

	return 0;
}

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
compile_and_execute(cs, srcpath, argv, envp, entry_name)
ccstate_t *cs;
const char *srcpath;
char **argv, **envp;
const char *entry_name;
{
	linkinfo_t *li;
	machine_t *ma;
	ci_exec_result_t res;
	size_t n_libfuncs, n_libvars;
	parse_id_t parse_id;

	if ((li = cc_parse_and_compile(cs, srcpath, &parse_id, NULL)) == NULL)
		return -1;

	cc_get_libinfo(&n_libfuncs, &n_libvars);
	Libvars = cx_get_libvars(n_libvars);
	
	ma = ci_make_machine_from_linkinfo(li, entry_name, 10240, getaddr,
					   (ci_get_regaddr_proc_t)NULL,
					   (char *)NULL, cc_report_error);
	ci_free_parse_id(parse_id);
	
	if (ma == NULL)
		return -1;

	ma->ma_libfuncs = cx_get_libfuncs(n_libfuncs);

	ci_initialise_machine(ma, FALSE, FALSE, argv, envp);
	
	res = ci_execute_machine(ma, (taddr_t)0, (taddr_t)0, noread, nowrite,
				 (ci_indirect_call_proc_t)NULL);

	if (res != CI_ER_EXITED && res != CI_ER_TRAP) {
		errf("Error executing %s: %s",
					srcpath, ci_exec_result_to_string(res));
		exit(1);
	}
  
	return 0;
}

static int
noread(addr, buf, nbytes)
taddr_t addr;
voidptr buf;
size_t nbytes;
{
	panic("unexpected readdata");
	return -1;
}

static int
nowrite(addr, buf, nbytes)
taddr_t addr;
constvoidptr buf;
size_t nbytes;
{
	panic("unexpected writedata");
	return -1;
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

