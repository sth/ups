/* cc.c - common support routines for cx.c and cg.c  */

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


/* @(#)cc.c	1.3 16 Sep 1994 (UKC) */
char ups_cc_c_rcsid[] = "$Id$";

#define UKC_WANT_COMMON_UNIX_EXTENSIONS 1	/* for popen() and pclose() */

#include <mtrprog/ifdefs.h>

#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "cc.h"

static const char *getline PROTO((char *arg));
static parse_id_t parse PROTO((const char *name, const char *cppflags,
			       unsigned long parse_flags));

#include "xc_opcodes.h"
#include "xc_machine.h"
#include "xc_builtins.h"

#if HAVE_X_WINDOWS
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
extern int _Xdebug;
#endif


/* ---------------------------------------------------------------------------
 */

#define WANT_DECLS
#include "cx_libfuncs.h"
#undef  WANT_DECLS

static const char *Libfuncnames[] = {
#define WANT_NAMES
#include "cx_libfuncs.h"
#undef  WANT_NAMES
};
#define N_LIBFUNCS ((int)(sizeof Libfuncnames / sizeof *Libfuncnames))

static const char *Libvarnames[] = {
#define WANT_NAMES
#include "cx_libvars.h"
#undef  WANT_NAMES
};
#define N_LIBVARS ((int)(sizeof Libvarnames / sizeof *Libvarnames))

/*  This is a hack to support the -ccerrfmt flag, which means produce
 *  errors in the same style as cc so emacs can parse them.
 */
static const char *Errsep = ", ";

struct ccstate_s {
	char *cs_cppflags;
	bool cs_oldc;
	const char *cs_extra_flags;
	unsigned long cs_compile_flags;
	unsigned long cs_parse_flags;
};

static const char Common_flags[] =
"[-g] [-oldc] [-Idir] [-Dsym[=value]] [-Usym] [-ccerrfmt]";

/*  C pre-processor flags, undefine __GNUC__ to remove various extensions.
 */
static const char Std_cppflags[] = 
"-D__STDC__=1 -D__CXC__=1 -U__GNUC__ -I/usr/local/lib/ups/ansi-include ";
/*
"'-D__attribute__(x)=' "
"-D__inline__= "
"-D__extension__= "
"-D__signed__=signed "
"-I/usr/src/local/X11/ups/ups/cxinclude "
*/

ccstate_t *
cc_create_ccstate(extra_flags, compile_flags)
const char *extra_flags;
unsigned compile_flags;
{
	ccstate_t *cs;

	cs = (ccstate_t *)e_malloc(sizeof(ccstate_t));
	cs->cs_extra_flags = extra_flags;
	cs->cs_cppflags = strsave(Std_cppflags);
	cs->cs_oldc = FALSE;
	cs->cs_compile_flags = compile_flags;
	cs->cs_parse_flags = 0;

	Errsep = ", ";

	/** Dibyendu: New warning messages **/
	ci_add_message_action("Floating point suffix ignored", MA_WARNING_ONLY);
	ci_add_message_action("Integer suffix ignored", MA_WARNING_ONLY);
	ci_add_message_action("Cast ignored in initialisation", MA_WARNING_ONLY);
	ci_add_message_action("Conversion to '", MA_WARNING_ONLY);

	/** Dibyendu: These are now warnings rather than errors **/
	ci_add_message_action("call via old style function expression",
				      MA_WARNING_ONLY);
	ci_add_message_action("No prototype in scope", MA_WARNING_ONLY);
	ci_add_message_action("Implicit declaration of", MA_WARNING_ONLY);

	return cs;
}

void
cc_free_ccstate(cs)
ccstate_t *cs;
{
	free(cs->cs_cppflags);
	free(cs);
}

const char *
cc_get_usage(cs)
ccstate_t *cs;
{
	return strf("%s %s", Common_flags, cs->cs_extra_flags);
}

bool
cc_get_syminfo_flag(cs)
ccstate_t *cs;
{
	return (cs->cs_compile_flags & CI_CP_WANT_SYMINFO) != 0;
}

void
cc_get_libinfo(p_nlibfuncs, p_nlibvars)
size_t *p_nlibfuncs, *p_nlibvars;
{
	static int i = 0;
	static int j = 0;

	/* Following is a temporary feature for checking supported
	 * library functions 
	 */
	if (getenv("CX_DUMP_LIBFUNC_NAMES") != NULL) {
		fprintf(stderr, "Library functions recognised\n");
		for (; i < N_LIBFUNCS; i++) 
			fprintf(stderr, "%d: %s\n", i, Libfuncnames[i]);
		fprintf(stderr, "Library variables recognised\n");
		for (; j < N_LIBVARS; j++) 
			fprintf(stderr, "%d: %s\n", j, Libvarnames[j]);
	}
	
	*p_nlibfuncs = N_LIBFUNCS;
	*p_nlibvars = N_LIBVARS;
}

bool
cc_handle_arg(cs, p_argv)
ccstate_t *cs;
char ***p_argv;
{
	char **argv;
	const char *flag;
	bool oldc;

	argv = *p_argv;
	flag = *argv + 1;
	oldc = cs->cs_oldc;

	if (*flag == 'D' || *flag == 'I' || *flag == 'U') {
		char *new;

		new = strf("%s -%s", cs->cs_cppflags, flag);
		free(cs->cs_cppflags);
		cs->cs_cppflags = new;
	}
	else if (strcmp(flag, "ccerrfmt") == 0) {
		Errsep = ":";
	}
	else if (strcmp(flag, "ignore") == 0) {
		ci_add_message_action(cc_checkarg(cs, ++argv), MA_IGNORE);
	}
	else if (strcmp(flag, "ignoreall") == 0) {
		ci_add_message_action((char *)NULL, MA_IGNORE);
	}
	else if (strcmp(flag, "warning") == 0) {
		ci_add_message_action(cc_checkarg(cs, ++argv), MA_WARNING_ONLY);
	}
	else if (strcmp(flag, "warningall") == 0) {
		ci_add_message_action((char *)NULL, MA_WARNING_ONLY);
	}
	else if (strcmp(flag, "oldc") == 0) {
		oldc = TRUE;
	}
	else if (strcmp(flag, "g") == 0) {
		cs->cs_compile_flags |= CI_CP_WANT_SYMINFO;
	}
	else if (strcmp(flag, "checksp") == 0) {
		cs->cs_compile_flags |= CI_CP_CHECKSP;
	}
	else if (strcmp(flag, "dont_panic") == 0) {
		cs->cs_compile_flags |= CI_CP_DONT_PANIC;
		cs->cs_parse_flags |= CI_DONT_PANIC;
	}
	else {
		return FALSE;
	}

	if (oldc && !cs->cs_oldc) {
		char *new;

		ci_add_message_action("call via old style function expression",
				      MA_IGNORE);
		ci_add_message_action("No prototype in scope", MA_IGNORE);
		ci_add_message_action("Implicit declaration of", MA_IGNORE);

		cs->cs_parse_flags |= CI_MAKE_EXTERNS_TOPLEVEL;

		new = strf("%s -traditional", cs->cs_cppflags);
		free(cs->cs_cppflags);
		cs->cs_cppflags = new;
		
		cs->cs_oldc = TRUE;
	}
	
	*p_argv = argv;
	return TRUE;
}

linkinfo_t *
cc_parse_and_compile(cs, srcpath, p_parse_id, checkarg_proc)
ccstate_t *cs;
const char *srcpath;
parse_id_t *p_parse_id;
ci_checkarg_proc_t checkarg_proc;
{
	*p_parse_id = parse(srcpath, cs->cs_cppflags, cs->cs_parse_flags);
	
	if (*p_parse_id == NULL) {
		errf("Parse of %s failed", srcpath);
		return NULL;
	}
			
	return ci_compile(srcpath, *p_parse_id, cc_report_error,
			  checkarg_proc,
			  (ci_regno_to_addr_proc_t)NULL,
			  cs->cs_compile_flags);
}

const char *
cc_checkarg(cs, argv)
ccstate_t *cs;
char **argv;
{
	if (*argv == NULL) {
		errf("The %s flag needs an argument", argv[-1]);
		errf_usage(cc_get_usage(cs));
	}

	return *argv;
}

bool
cc_report_error(lx, mesg)
lexinfo_t *lx;
const char *mesg;
{
	if (lx != NULL)
		fprintf(stderr, "%s%s%d: %s\n",
			lx->lx_filename, Errsep, lx->lx_lnum, mesg);
	else
		fprintf(stderr, "\t%s\n", mesg);
	
	fflush(stderr);
	return FALSE;
}

static const char *
getline(arg)
char *arg;
{
	return fpgetline((FILE *)arg);
}

ci_nametype_t
cc_getaddr(name, p_addr)
const char *name;
taddr_t *p_addr;
{
	int i;

	for (i = 0; i < N_LIBFUNCS; ++i) {
		if (strcmp(Libfuncnames[i], name) == 0) {
			*p_addr = i;
			return CI_DIRECT_LIBFUNC;
		}
	}

	for (i = 0; i < N_LIBVARS; ++i) {
		if (strcmp(Libvarnames[i], name) == 0) {
			*p_addr = i;
			return CI_DATA;
		}
	}

	return CI_UNDEFINED;
}

static parse_id_t
parse(name, cppflags, parse_flags)
const char *name;
const char *cppflags;
unsigned long parse_flags;
{
	FILE *fp;
	int len;
	parse_id_t parse_id = NULL;

	len = strlen(name);
	
	if ((fp = fopen(name, "r")) == NULL) {
		failmesg("Can't open", "source file", name);
		parse_id = NULL;
	}
	else if (len > 2 && strcmp(name + len - 2, ".c") == 0) {
		char *cmd;

		fclose(fp);
#if defined(C_PREPROCESSOR)
		cmd = strf("%s %s %s", C_PREPROCESSOR, cppflags, name);
#else
		cmd = strf("gcc -E %s %s", cppflags, name);
#endif
		errno = 0;
		if ((fp = popen(cmd, "r")) == NULL) {
			errf("Can't run \"%s\": %s", cmd, get_errno_str());
			exit(1);
		}
		else {
			parse_id = ci_parse_file(name,
						 (block_t *)NULL,
						 parse_flags, cc_report_error,
						 (ci_resolve_name_func_t)NULL,
						 getline, (char *)fp);
			pclose(fp);
		}
		free(cmd);
	}
	else {
		parse_id = ci_parse_file(name, (block_t *)NULL,
					 parse_flags, cc_report_error,
					 (ci_resolve_name_func_t)NULL,
					 getline, (char *)fp);
		fclose(fp);
	}

	return parse_id;
}

