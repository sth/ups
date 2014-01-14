/*  xc.c - C interpreter execution driver */

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


/* @(#)xc.c	1.3 22/12/93 (UKC) */
char ups_xc_c_rcsid[] = "$Id$";

#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "xc_load.h"

static int noread PROTO((taddr_t addr, void *buf, size_t nbytes));
static int nowrite PROTO((taddr_t addr, const void *buf, size_t nbytes));
int main PROTO((int argc, char **argv));

extern char **environ;

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

int
main(argc, argv)
int argc;
char **argv;
{
	machine_t *ma;
	ci_exec_result_t res;
	const char *path;
	size_t stack_size;
	int status;
	bool want_opcounts;

	errf_set_progname(*argv++);

	if (*argv != NULL && strncmp(*argv, "-s", 2) == 0) {
		char *ends;
		long nwords;

		stack_size = strtol(*argv + 2, &ends, 0);

		if (ends == *argv + 2) {
			errf("Stack size value missing after -s flag");
			exit(1);
		}

		if (tolower(*ends) == 'k') {
			++ends;
			stack_size *= 1024;
		}
		else if (tolower(*ends) == 'm') {
			++ends;
			stack_size *= 1024 * 1024;
		}

		if (stack_size <= 0 || *ends != '\0') {
			errf("Stack size `%s' is not a positive number",
								*argv + 2);
			exit(1);
		}

		/*  Silently round up the stack size to an even number of words.
		 */
		nwords = (stack_size + sizeof(int) - 1) / sizeof(int);
		stack_size = nwords * sizeof(int);

		++argv;
	}
	else {
		stack_size = 10240;
	}

	want_opcounts = *argv != NULL && strcmp(*argv, "-opcounts") == 0;
	if (want_opcounts)
		++argv;

	if (*argv == NULL)
		errf_usage("[-ssize] [-opcounts] cxname [args]");
	path = *argv;

	if (ci_load(path, stack_size, want_opcounts, argv, environ, &ma) != 0)
		exit(1);

	res = ci_execute_machine(ma, (taddr_t)0, (taddr_t)0, (taddr_t)0, (taddr_t)0,
						noread, nowrite,
						(ci_indirect_call_proc_t)NULL);

	if (res != CI_ER_EXITED) {
		errf("Error executing %s: %s",
					path, ci_exec_result_to_string(res));
		exit(1);
	}

	status = ci_get_exit_status(ma);
	
	if (want_opcounts)
		ci_dump_machine_stats(ma, stdout, "the standard output");

	ci_reset_machine(ma);

	exit(status);
	return 1;	/* to satisfy gcc */
}
