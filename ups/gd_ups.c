/* gd_ups.c - target driver for gdb backend - UPS side */

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


/* @(#)gd_ups.c	1.10 04 Jun 1995 (UKC) */
char ups_gd_ups_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include "gd_defs.h"


#define GET_STD(st)	((gd_stdata_t *)(st)->st_data)
#define GET_FID(fil)	((gd_fidata_t *)(fil)->fi_data)

static int ugd_preinitialise PROTO((void));
static int ugd_initialise PROTO((ebuf_t *usage_eb, char **argv));
static void ugd_show_target_driver_info PROTO((bool name_only));
static bool ugd_match PROTO((const char *textpath,
			     text_block_t *tb, size_t nbytes));
static int ugd_init_from_textfile PROTO((target_t *xp, int textfd,
					 const char *corepath,
					 bool user_gave_core,
					 const char **p_cmdline));
static int ugd_next_symtab PROTO((target_t *xp, symtab_t *st, bool load_new,
				 symtab_t **p_next_st));
static stopres_t ugd_start PROTO((target_t *xp));
static stopres_t ugd_step PROTO((target_t *xp));
static stopres_t ugd_next PROTO((target_t *xp));
static stopres_t ugd_cont PROTO((target_t *xp));
static void ugd_kill PROTO((target_t *xp));


xp_ops_t Gd_ops = {
	ugd_preinitialise, ugd_initialise, ugd_show_target_driver_info,
	ugd_match, ugd_init_from_textfile, ugd_next_symtab,
	ugd_start, ugd_step, ugd_next, ugd_cont, ugd_kill,
	NULL, NULL,
	NULL, NULL,
	gd_add_breakpoint, gd_remove_breakpoint,
	gd_enable_breakpoint, gd_disable_breakpoint,
	gd_addr_to_breakpoint, gd_get_breakpoint_addr,
	NULL, NULL, NULL, NULL, NULL, NULL,
	gd_is_attached, gd_detach, gd_get_addrsize,
	gd_get_state, gd_get_lastsig, NULL, gd_get_sigstate,
	gd_get_stack_trace, gd_get_reg_addr, gd_get_signal_tag,
	gd_read_fpval, gd_read_fpreg, gd_readreg, gd_setreg,
	NULL, NULL, NULL, NULL,
	NULL,
	gd_read_data, gd_write_data, gd_read_text_from_process,
	NULL, gd_call_func, gd_handle_signal_change, gd_do_debug_command,
	NULL
};


static int
ugd_preinitialise()
{
    ggd_preinitialise ();
}

static int
ugd_initialise(eb, argv)
ebuf_t *usage_eb;
char **argv;
{
    enum target_signal ts;
    int disable, want_output, want_cmdlog;

    disable_gdb = extract_bool_arg(usage_eb, argv, "-nogdb");
    want_output = extract_bool_arg(usage_eb, argv, "-gdboutput");
    want_cmdlog = extract_bool_arg(usage_eb, argv, "-loggdbcmds");

    for (ts = TARGET_SIGNAL_HUP; ts < TARGET_SIGNAL_UNKNOWN; ++ts) {
	int sig;

	sig = target_signal_to_host(ts);
	
	if (ts != 0) {
	    set_signal_attrs(sig,
			     !signal_program[ts],
			     signal_print[ts],
			     signal_stop[ts]);
	}
    }

    ggd_initialise (disable, want_output, want_cmdlog);

    return 0;
}

static void
ugd_show_target_driver_info(name_only)
bool name_only;
{
    if (name_only) {
	puts("\tObject file support via a modified version of gdb");
	return;
    }

    ggd_show_target_driver_info(name_only);
}

static bool
ugd_match(textpath, tb, nbytes)
const char *textpath;
text_block_t *tb;
size_t nbytes;
{
    return ggd_match();
}

static int
ugd_init_from_textfile(xp, textfd, corepath, user_gave_core, p_cmdline)
target_t *xp;
int textfd;
const char *corepath;
bool user_gave_core;
const char **p_cmdline;
{
    struct objfile *objfile;

    if (do_gdb_command(exec_file_command, xp->xp_textpath) != 0)
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

    if (do_gdb_command(symbol_file_command, xp->xp_textpath) != 0)
	    return -1;

    if (object_files == NULL || object_files->next != NULL)
	    panic("#objfiles botch in iftf");
    objfile = object_files;

    if (user_gave_core) {
	    Gdb_cmdfunc cmd;

	    cmd = is_number(corepath) ? attach_command : core_file_command;

	    if (do_gdb_command(cmd, corepath) != 0)
		    return -1;
    }

    xp->xp_symtabs = objfile_to_st(objfile);
    /* xp->xp_symtab_cache = NULL; */
    xp->xp_data = NULL;

    xp->xp_words_big_endian = TARGET_BYTE_ORDER == BIG_ENDIAN;
    xp->xp_bits_big_endian = BITS_BIG_ENDIAN;

    if (attach_flag)
	    update_shared_library_symtabs(xp);

    return 0;
}

static int 
gd_next_symtab (xp, st, load_new, p_next_st)
target_t *xp;
symtab_t *st;
bool load_new;
symtab_t **p_next_st;
{
    if (st == NULL) {
	    *p_next_st = xp->xp_symtabs;
	    return TRUE;
    }
    return FALSE;
}

static stopres_t
gd_start(xp)
target_t *xp;
{
    stopres_t stopres;
    char *cmdline;
    struct objfile *objfile;
    infargs_t iabuf;

    if (setup_shellcmd(FALSE, &cmdline) != 0)
	    return SR_DIED;

    /*  We get the path from the objfile rather than using xp_textpath
     *  because fork_inferior() wants the absolute path, and xp_textpath
     *  can be a relative pathname.
     */
    objfile = GET_STD(xp->xp_symtabs)->st_objfile;

    iabuf.ia_path = objfile->name;
    iabuf.ia_cmdline = cmdline;
    iabuf.ia_envp = get_environment();
    stopres = run_gdb_target(start_gdb_inferior, (char *)&iabuf);

    if (stopres != SR_DIED)
	    update_shared_library_symtabs(xp);

    return stopres;
}

static stopres_t
gd_step(xp)
target_t *xp;
{
    return do_next_or_step(step_command);
}

static stopres_t
gd_next(xp)
target_t *xp;
{
    return do_next_or_step(next_command);
}

static stopres_t
gd_cont(xp)
target_t *xp;
{
    return run_gdb_target(continue_command, (char *)NULL);
}

static void
gd_kill(xp)
target_t *xp;
{
    do_gdb_command(kill_command, (const char *)NULL);
}


