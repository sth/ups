/* gd_gdb.c - target driver for gdb backend - GDB side */

/*  Copyright 1994 Mark Russell, University of Kent at Canterbury.
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


/* @(#)gd_gdb.c	1.10 04 Jun 1995 (UKC) */
char ups_gd_gdb_c_rcsid[] = "$Id$";

/* GDB... */
#include <defs.h>
#include <setjmp.h>
#include <top.h>
#include <target.h>
#include <inferior.h>
#include <call-cmds.h>
#include <getopt.h>
#include <sys/types.h>
#include <gdb_stat.h>
#include <ctype.h>
#include <gdb_string.h>

/* These are only reqd. for 'objfiles'. */
#include <symfile.h>
#include <objfiles.h>
/* ..GDB */

/* UPS... */
#ifdef HAVE_LIBM
#undef HAVE_LIBM
#endif
#undef PROTO
#undef CAT3

#include <mtrprog/ifdefs.h>

#include <local/ukcprog.h>

#include "gd_defs.h"
/* ...UPS */

void fputs_maybe_filtered PARAMS((const char *s, FILE *fp));
void error PARAMS((const char *cmd, ...));

extern struct breakpoint *breakpoint_chain;
extern jmp_buf error_return, quit_return;
extern unsigned char *signal_stop, *signal_print, *signal_program;


static int Sent_sigint_to_child;

static int Want_gdb_cmdlog;
static int Want_gdb_output;
static int Disable_gdb;

#define CATCH_ERRORS(action) \
 { \
	 if (setjmp(error_return) != 0) { \
		 action; \
	 } \
	 memcpy(quit_return, error_return, sizeof(jmp_buf)); \
 }

/*
 * Called from UPS
 */
void
gd_print_gdb_version (fp)
FILE *fp;
{
    print_gdb_version (gdb_stdout);
}

void
gd_error_to_top_level ()
{
	return_to_top_level(RETURN_ERROR);
}

const char *
gd_gdbcmd_to_name(cfunc)
Gdb_cmdfunc cfunc;
{
	if (cfunc == next_command)		return "next";
	if (cfunc == step_command)		return "step";
	if (cfunc == continue_command)		return "continue";
	if (cfunc == finish_command)		return "next";
	if (cfunc == kill_command)		return "kill";
	if (cfunc == detach_command)		return "detach";
	if (cfunc == enable_command)		return "enable";
	if (cfunc == disable_command)		return "disable";
	if (cfunc == exec_file_command)		return "exec-file";
	if (cfunc == symbol_file_command)	return "symbol-file";
	if (cfunc == core_file_command)		return "core-file";
	if (cfunc == attach_command)		return "attach";
	if (cfunc == break_command)		return "break";
	if (cfunc == delete_command)		return "delete";

	return "UNKNOWN-COMMAND";
}

int
gd_do_command(cfunc, arg)
Gdb_cmdfunc cfunc;
const char *arg;
{
	if (Want_gdb_cmdlog && cfunc != gd_start_gdb_inferior)
		logcmd(stdout, gd_gdbcmd_to_name(cfunc), arg);
	
	CATCH_ERRORS(return -1);

	(*cfunc)((char *)arg, FALSE);

	do_all_gdb_cleanups();
	
	/*  If we get here, the command succeeded.
	 */
	return 0;
}

/*
 *  This is called from GDB 'gdb_init()' through init_ui_hook.
 */
void
ups_init_ui ()
{
    /*
     * Redirect output into string buffers.
     */
    gdb_stdout = gdb_file_init_astring (0);
    gdb_stderr = gdb_file_init_astring (0);
}


/*
 *  All GDB output on 'gdb_stdout' and 'gdb_stderr' will end up here.
 *  This may come in one character at a time.
 */
void
ups_fputs (linebuffer, stream)
     const char *linebuffer;
     GDB_FILE *stream;
{
    if (stream->ts_streamtype == astring)
    {
/*
	gdb_file_adjust_strbuf (strlen (linebuffer), stream);
        strcat (stream->ts_strbuf, linebuffer);
*/
	fputs (linebuffer, stdout);
    }
    else
	fputs (linebuffer, stream->ts_filestream);
}


/*
 * Called from UPS
 */
int
ggd_preinitialise()
{
    /*
     * This must happen before ups first calls malloc.
     */
    init_malloc((PTR)NULL);

    /*
     * gdb hooks - see gdb/top.c.
     */
    init_ui_hook	= ups_init_ui;
    fputs_unfiltered_hook = ups_fputs;
    command_loop_hook	= ups_gdb_command_loop;

    return 0;
}

/*
 * Called from UPS
 */
int
ggd_initialise(disable, want_output, want_cmdlog)
int disable;
int want_output;
int want_cmdlog;
{
    static char dirbuf[1024];
    extern int caution;
    extern int current_display_number;

    Want_gdb_cmdlog = want_cmdlog;
    Want_gdb_output = want_output;
    Disable_gdb = disable;

    caution = 0;	/* Don't want any queries */

    current_display_number = -1;

    getcwd(dirbuf, sizeof(dirbuf));
    current_directory = dirbuf;
    initialize_all_files();

    return 0;
}

void
ggd_show_target_driver_info(name_only)
bool name_only;
{
    bool save_output_flag;

    if (Disable_gdb)
	    return;

    save_output_flag = Want_gdb_output;
    Want_gdb_output = TRUE;

    puts("\
This version of ups uses a modified version of the GNU debugger gdb:");
    print_gdb_version(stdout);
    fputc('\n', stdout);

    Want_gdb_output = save_output_flag;
}

bool
ggd_match()
{
    return !Disable_gdb;
}

/* ARGSUSED */
static void
command_loop_marker (foo)
int foo;
{
}

int
do_gdb_command(cfunc, arg)
Gdb_cmdfunc cfunc;
const char *arg;
{
    struct cleanup *old_chain;

    if (Want_gdb_cmdlog && cfunc != start_gdb_inferior)
	logcmd(stdout, gdbcmd_to_name(cfunc), arg);
	
    CATCH_ERRORS(return -1);

    quit_flag = 0;
    old_chain = make_cleanup ((make_cleanup_func) command_loop_marker, 0);

    (*cfunc)((char *)arg, FALSE);

    /* Do any commands attached to breakpoint we stopped at.  */
    bpstat_do_actions (&stop_bpstat);
    do_cleanups (old_chain);

    /* If we get here, the command succeeded.  */
    return 0;
}












void
gd_handle_signal_change(xp, sig, ignore, redraw, stop)
target_t *xp;
int sig;
bool ignore, redraw, stop;
{
	enum target_signal target_signal;

	target_signal = target_signal_from_host(sig);

	if (target_signal == TARGET_SIGNAL_UNKNOWN)
		return;

	signal_program[target_signal] = !ignore;
	signal_print[target_signal] = redraw;
	signal_stop[target_signal] = stop;
}


int
gd_init_from_textfile(xp, textfd, corepath, user_gave_core, p_cmdline)
target_t *xp;
int textfd;
const char *corepath;
bool user_gave_core;
const char **p_cmdline;
{
	struct objfile *objfile;

	if (gd_do_command(exec_file_command, get_get_xp_textpath(xp)) != 0)
		return -1;
	
	/*  If the user is attempting to attach to a process do a quick check
	 *  that we have permission before the possibly lengthy work of
	 *  loading the symbol table.
	 */
	if (user_gave_core && is_number(corepath)) {
		int pid;

		pid = atoi(corepath);
		
		if (kill(pid, 0) != 0) {
			errf("Can't attach to process %d (%m)", pid);
			return -1;
		}
	}

	if (gd_do_command(symbol_file_command, get_get_xp_textpath(xp)) != 0)
		return -1;

	if (object_files == NULL || object_files->next != NULL)
		panic("#objfiles botch in iftf");
	objfile = object_files;

	if (user_gave_core) {
		Gdb_cmdfunc cmd;

		cmd = is_number(corepath) ? attach_command : core_file_command;

		if (gd_do_command(cmd, corepath) != 0)
			return -1;
	}

	gd_setup_xp (xp, objfile_to_st(objfile));

	if (attach_flag)
		update_shared_library_symtabs(xp);

	return 0;
}


