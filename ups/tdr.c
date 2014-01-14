/* tdr.c - code to record and replay text based test driver files */

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


/* @(#)tdr.c	1.19 04 Jun 1995 (UKC) */
char ups_tdr_c_rcsid[] = "$Id$";

#define UKC_WANT_COMMON_UNIX_EXTENSIONS 1/* for popen, pclose and lstat */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <errno.h>

#ifdef __STDC__
#include <unistd.h>
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <local/ukcprog.h>
#include <mtrprog/utils.h>

#include <local/wn.h>
#include <local/obj/obj.h>
#include <local/menu3.h>
#include <local/edit/edit.h>
#include <mtrprog/ifdefs.h>
#include <mtrprog/so.h>

#include "srcwin.h"
#include "ui.h"
#include "cursors.h"
#include "reg.h"
#include "ups.h"
#include "symtab.h"
#include "st.h"
#include "exec.h"
#include "objtypes.h"
#include "obj_misc.h"
#include "obj_target.h"
#include "obj_bpt.h"
#include "obj_signal.h"
#include "obj_env.h"
#include "obj_stack.h"
#include "obj_util.h"
#include "va.h"
#include "menudata.h"
#include "expr.h"
#include "state.h"
#include "tdr.h"
#include "util.h"
#include "srcpath.h"
#include "sccsdata.h"

typedef struct Replay_mode Replay_mode;

struct Replay_mode {
	int lnum;
	const char *mode;
	Replay_mode *next;
};

/*  Avoid messing up `%' in vi with literal braces.
 */
#define OPEN_BRACE	'{'
#define CLOSE_BRACE	'}'

#define errf	DONT_CALL_ERRF__USE_SYSF_INSTEAD

/*  Version number for the actions file.  We complain (but continue anyway)
 *  if we are fed a file with a version number higher than we expect.
 *  RGA updated to version 3.
 */
#define CURRENT_VERSION		3

static FILE *Record_fp = NULL;
static const char *Record_file_name;

static FILE *Replay_fp = NULL;
static FILE *Replay_copy_fp = NULL;
static const char *Replay_file_name;
static const char *Replay_copy_file_name;
int Replay_lnum;
static bool Check_message_failed;
static bool load_flag = FALSE;
static bool noload_flag = FALSE;
static bool Had_check_failure;

static bool Interactive;
static bool Want_select_recording = TRUE;
static bool Have_window = TRUE;

static bool Runcmds = TRUE;
static bool Want_stop_on_check_failure = TRUE;
static bool Want_cmd_echo = FALSE;
static bool Want_wait_for_user = FALSE;
static bool Want_wait_on_check = FALSE;
static bool Want_verbose_test = FALSE;
static bool Want_displayed_source_check = TRUE;

static Replay_mode *Replay_modes;

static const char *Replay_idstr = NULL;

static objid_t Obj_to_make_visible;
static objid_t Default_obj;

static fil_t *Current_displayed_fil = NULL;

static void (*Error_func)PROTO((const char *s));

#define NEL(a)	(sizeof(a) / sizeof*(a))

typedef void (*mfunc_t)PROTO((objid_t objid, int cmd, char *arg));

typedef int (*cmdfunc_t)PROTO((const char *cmd, int argc, char **args));

typedef struct {
	int am_rv;
	const char *am_name;
} actmap_t;

typedef struct {
	const char *mc_name;
	mfunc_t mc_func;
	bool mc_apply_to_objs;
	actmap_t *mc_actmap;
	int mc_actmap_size;
} mcmds_t;

static void do_permanent_menu PROTO((objid_t unused_objid, int cmd,
				     char *unused_arg));
static void do_target_menu PROTO((objid_t unused_objid, int cmd,
				  char *unused_arg));
static void do_source_window_menu PROTO((objid_t unused_objid, int cmd,
					 char *unused_arg));
static void do_output_window_menu PROTO((objid_t unused_objid, int cmd,
					 char *unused_arg));

static cmdfunc_t cmd_to_func PROTO((const char *cmd, bool *p_alwaysexec));
static cmdfunc_t preload_cmd_to_func PROTO((const char *cmd,
					    bool *p_alwaysexec));
static int process_command PROTO((char *line, bool preload_only));

static mcmds_t *mname_to_menu PROTO((const char *name));
static void list_mnames PROTO(( FILE *fp));

static const char *rv_to_name PROTO((mcmds_t *mc, int rv));
static int name_to_rv PROTO((mcmds_t *mc, const char *name, int *p_rv));
static void list_rvs PROTO((FILE *fp, mcmds_t *mc));

static void show_error_on_tty PROTO((const char *s));
static void wait_for_user PROTO((const char *line, bool *p_quit));

static void show_obj_path PROTO((const char *op, objid_t obj,
							int endc, FILE *fp));
static int checkline PROTO((const char *line, const char *endword));
static int record_obj_dump PROTO((objid_t obj));
static int r_flush PROTO((void));
static int r_getline PROTO((bool lose_leading_whitespace, char **p_line));

static int cf_refresh PROTO((const char *cmd, int argc, char **unused_args));
static int cf_menu PROTO((const char *cmd, int argc, char **args));
static int cf_replaymode PROTO((const char *cmd, int argc, char **unused_args));
static int cf_select PROTO((const char *cmd, int argc, char **args));
static int cf_remove PROTO((const char *cmd, int argc, char **args));
static int cf_show_var PROTO((const char *cmd, int argc, char **args));
static int cf_to_lnum PROTO((const char *cmd, int argc, char **args));
static int cf_debug PROTO((const char *cmd, int argc, char **args));
static int cf_show PROTO((const char *cmd, int argc, char **args));
static int cf_edit PROTO((const char *cmd, int argc, char **args));
static int cf_bptcode PROTO((const char *cmd, int argc, char **args));
static int cf_help PROTO((const char *cmd, int argc, char **unused_args));
static int cf_version PROTO((const char *cmd, int argc, char **unused_args));
static int cf_check PROTO((const char *cmd, int argc, char **args));
static int cf_cond PROTO((const char *cmd, int argc, char **args));
static int cf_echo PROTO((const char *cmd, int unused_argc, char **args));
static int cf_exit PROTO((const char *cmd, int argc, char **args));
static int cf_shellcmd PROTO((const char *cmd, int argc, char **args));
static int cf_message PROTO((const char *cmd, int argc, char **args));
static int cf_output PROTO((const char *cmd, int argc, char **args));
static int cf_checksrc PROTO((const char *cmd, int argc, char **args));
static int cf_load PROTO((const char *cmd, int argc, char **args));
static int cf_noload PROTO((const char *cmd, int argc, char **args));
static int cf_use PROTO((const char *cmd, int argc, char **args));
static int cf_bpt PROTO((const char *cmd, int argc, char **args));

static bool lines_match PROTO((const char *fline, const char *uline));
static bool mf_spaces PROTO((const char *unused_fline, const char **p_uline));
static bool mf_addr PROTO((const char *unused_fline, const char **p_uline));
static bool mf_choice PROTO((const char *fline, const char **p_uline));
static bool mf_value PROTO((const char *unused_fline, const char **p_uline));
static bool mf_junkaddr PROTO((const char *unused_fline, const char **p_uline));
static bool mf_junkstring PROTO((const char *unused_fline, const char **p_uline));
static bool mf_junkfuncptr PROTO((const char *unused_fline, const char **p_uline));

static int check_cond PROTO((const char *cond, bool *p_cond_holds));
static const char *not PROTO((const char *cond));
static int handle_cond PROTO((const char *cmd, const char *cond));
static int skip_to PROTO((const char *word));
static void sysf PROTO((const char *fmt, ...)) FORMF_ARGS(1, 2);
static int check_message PROTO((const char *what, const char *mesg));
static int cmp_lines PROTO((const char *what,
			     const char *expected, const char *got));
static int check_shell_line PROTO((const char *line));
static bool mf_badaddr PROTO((const char **p_uline));
static const char *get_idstr PROTO((void));
static bool str_matches_pat PROTO((const char *str, const char *pat));
static void sysf_ofunc PROTO((const char *mesg));
static bool get_output_text PROTO((char **p_text));
static int dump_obj PROTO((char *arg, int level, char *line));

static struct {
	const char *name;
	cmdfunc_t func;
	bool alwaysexec;
} Cmdtab[] = {
	{ "menu",	cf_menu,	FALSE	},
	{ "select",	cf_select,	FALSE	},
	{ "addselect",	cf_select,	FALSE	},
	{ "deselect",	cf_select,	FALSE	},
	{ "remove",	cf_remove,	FALSE	},
	{ "replaymode",	cf_replaymode,	FALSE	},
	{ "checksrc",	cf_checksrc,	FALSE	},
	{ "refresh",	cf_refresh,	FALSE	},
	{ "execto",	cf_to_lnum,	FALSE	},
	{ "showvar",	cf_show_var,	FALSE	},
	{ "show",	cf_show,	FALSE	},
	{ "breakpoint",	cf_to_lnum,	FALSE	},
	{ "debug",	cf_debug,	FALSE	},
	{ "help",	cf_help,	FALSE	},
	{ "echo",	cf_echo,	FALSE	},
	{ "warn",	cf_echo,	FALSE	},
	{ "exit",	cf_exit,	FALSE	},
	{ "message",	cf_message,	FALSE	},
	{ "output",	cf_output,	FALSE	},
	{ "shellcmd",	cf_shellcmd,	FALSE	},

	{ "shell",	cf_shellcmd,	TRUE	},
	{ "edit",	cf_edit,	TRUE	},
	{ "bptcode",	cf_bptcode,	TRUE	},
	{ "version",	cf_version,	TRUE	},
	{ "check",	cf_check,	TRUE	},

	{ "if",		cf_cond,	TRUE	},
	{ "else",	cf_cond,	TRUE	},
	{ "endif",	cf_cond,	TRUE	},

	{ "break",	cf_bpt,		TRUE,	},
};

static struct {
	const char *name;
	cmdfunc_t func;
	bool alwaysexec;
} Preload_cmdtab[] = {
	{ "load",	cf_load,	FALSE	},
	{ "noload",	cf_noload,	FALSE	},
	{ "use",	cf_use,		FALSE	},
};

static actmap_t Am_var[] = {
	{ MR_VAR_STRING,		"string"			},
	{ MR_VAR_SIGNED_DECIMAL,	"signed_decimal"		},
	{ MR_VAR_UNSIGNED_DECIMAL,	"unsigned_decimal"		},
	{ MR_VAR_SIGNED_OCTAL,		"signed_octal"			},
	{ MR_VAR_UNSIGNED_OCTAL,	"unsigned_octal"		},
	{ MR_VAR_SIGNED_HEX,		"signed_hex"			},
	{ MR_VAR_UNSIGNED_HEX,		"unsigned_hex"			},
	{ MR_VAR_UNSIGNED_BINARY,	"unsigned_binary"		},
	{ MR_VAR_ASCII_BYTE,		"ascii_byte"			},
	{ MR_VAR_DEREF,			"deref"				},
	{ MR_VAR_ADDRESS,		"address"			},
	{ MR_VAR_EXPAND_COMPLETELY,	"expand_completely"		},
	{ MR_VAR_EXPAND_LIKE_BEFORE,	"expand_like_before"		},
	{ MR_VAR_DUP,			"dup"				},
	{ MR_VAR_DELETE,		"delete"			},
	{ MR_VAR_COLLAPSE,		"collapse"			},
	{ MR_VAR_COLLAPSE_COMPLETELY,	"collapse_completely"		},
	{ MR_VAR_WANT_TYPEDEFS,		"want_typedefs"			},
	{ MR_VAR_NO_TYPEDEFS,		"no_typedefs"			},
	{ MR_VAR_SHOW_HIERARCHY,	"show_hierarchy"		},
	{ MR_VAR_SHOW_STATICS,		"show_statics"			},
	{ MR_VAR_DUMP_TO_OUTWIN,	"dump_to_outwin"		},
	{ MR_VAR_SHOW_SIZE,		"show_size"			},
};

static actmap_t Am_srcwin[] = {
	{ MR_SRCWIN_BACK,		"back"				},
	{ MR_SRCWIN_SEARCH_FORWARDS,	"search_forwards"		},
	{ MR_SRCWIN_SEARCH_BACKWARDS,	"search_backwards"		},
	{ MR_SRCWIN_UP_STACK,		"up_stack"			},
	{ MR_SRCWIN_DOWN_STACK,		"down_stack"			},
};

static actmap_t Am_outwin[] = {
	{ MR_OUTWIN_LOAD,		"load"				},
	{ MR_OUTWIN_SAVE,		"save"				},
	{ MR_OUTWIN_CLEAR,		"clear"				},
	{ MR_OUTWIN_SEARCH_FORWARDS,	"search_forwards"		},
	{ MR_OUTWIN_SEARCH_BACKWARDS,	"search_backwards"		},
	{ MR_OUTWIN_CLOSE,		"close"				},
};

static actmap_t Am_bphead[] = {
	{ MR_ADD_BREAKPOINT,		"add_breakpoint"		},
	{ MR_REMOVE_ALL_BREAKPOINTS,	"remove_all_breakpoints"	},
	{ MR_RESTORE_BREAKPOINTS,	"restore_breakpoints"		},
	{ MR_LOAD_BREAKPOINTS_EXC,	"load_breakpoints_inc"		},
	{ MR_LOAD_BREAKPOINTS_INC,	"load_breakpoints_exc"		},
	{ MR_ENABLE_ALL_BREAKPOINTS,	"enable_all_breakpoints"	},
	{ MR_DISABLE_ALL_BREAKPOINTS,	"disable_all_breakpoints"	},
};

static actmap_t Am_bpt[] = {
	{ MR_BPTMEN_REMOVE,		"remove"			},
	{ MR_BPTMEN_SOURCE,		"source"			},
	{ MR_BPTMEN_SAVE,		"save"				},
	{ MR_BPTMEN_ACTIVE,		"active"			},
	{ MR_BPTMEN_INACTIVE,		"inactive"			},
	{ MR_BPTMEN_EXECUTE,		"execute"			},
};

static actmap_t Am_globals[] = {
	{ MR_SHOW_UNTYPED_VARS,		"show_untyped_vars"		},
	{ MR_HIDE_UNTYPED_VARS,		"hide_untyped_vars"		},
};

static actmap_t Am_cbhead[] = {
	{ MR_SHOW_COMMON_BLOCKS,	"show_common_blocks"		},
	{ MR_HIDE_ALL_COMMON_BLOCKS,	"hide_all_common_blocks"	},
};

static actmap_t Am_mhead[] = {
	{ MR_SHOW_MODULES,		"show_modules"			},
	{ MR_HIDE_ALL_MODULES,		"hide_all_modules"		},
};

static actmap_t Am_cblock[] = {
	{ MR_EXPAND_COMMON_BLOCK_COMPLETELY, "expand_common_block_completely" },
	{ MR_EXPAND_COMMON_BLOCK_LIKE_BEFORE,"expand_common_block_like_before"},
	{ MR_COLLAPSE_COMMON_BLOCK,	"collapse_common_block"		},
	{ MR_HIDE_COMMON_BLOCK,		"hide_common_block"		},
};

static actmap_t Am_module[] = {
	{ MR_EXPAND_MODULE,		"expand_module"			},
	{ MR_COLLAPSE_MODULE,		"collapse_module"		},
	{ MR_HIDE_MODULE,		"hide_module"			}, 
};

static actmap_t Am_module_func[] = {
	{ MR_MODULE_FUNC_DISPLAY_SOURCE,"display_source"		},
	{ MR_MODULE_FUNC_HIDE,		"hide"				},
};

static actmap_t Am_srchead[] = {
	{ MR_SHOW_SOURCE_FILES,		"show_source_files"		},
	{ MR_HIDE_SOURCE_FILES,		"hide_source_files"		},
	{ MR_HIDE_SOURCE_FILES_COMPLETELY,
					"hide_source_files_completely"	},
	{ MR_ADD_SOURCE_PATH,		"add_source_path"		},
};

static actmap_t Am_source[] = {
	{ MR_EXPAND_COMPLETELY,		"expand_completely"		},
	{ MR_EXPAND_LIKE_BEFORE,	"expand_like_before"		},
	{ MR_COLLAPSE,			"collapse"			},
	{ MR_DISPLAY_SOURCE,		"display_source"		},
	{ MR_ADD_EXPRESSION,		"add_expression"		},
};

static actmap_t Am_block[] = {
	{ MR_BLOCK_EXPAND,		"block_expand"			},
	{ MR_BLOCK_COLLAPSE,		"block_collapse"		},
	{ MR_BLOCK_DELETE,		"block_delete"			},
	{ MR_BLOCK_DISPLAY_SOURCE,	"block_display_source"		},
	{ MR_BLOCK_ADD_EXPRESSION,	"block_add_expression"		},
};

static actmap_t Am_sighead[] = {
	{ MR_SHOW_ALL_SIGNALS,		"show_all_signals"		},
	{ MR_HIDE_ALL_SIGNALS,		"hide_all_signals"		},
};

static actmap_t Am_signal[] = {
	{ MR_SIG_TOGGLE_STOP_CONT,	"toggle_stop_cont"		},
	{ MR_SIG_TOGGLE_ACCEPT_IGNORE,	"toggle_accept_ignore"		},
	{ MR_SIG_TOGGLE_REDRAW,		"toggle_redraw"			},
	{ MR_HIDE_SIGNAL,		"hide_signal"			},
};

static actmap_t Am_pmenu[] = {
	{ MR_QUIT_UPS,			"quit_ups"			},
	{ MR_SNAPSHOT_SELECTED,		"snapshot_selected"		},
	{ MR_SNAPSHOT_ALL,		"snapshot_all"			},
	{ MR_DONT_QUIT,			"dont_quit"			},
	{ MR_DISWIN_SEARCH_FORWARDS,	"search_forwards"		},
	{ MR_DISWIN_SEARCH_BACKWARDS,	"search_backwards"		},
	{ MR_TOGGLE_LOGGING,		"toggle_logging"		},
	{ MR_LOGGING_ON,		"logging_on"			},
	{ MR_LOGGING_OFF,		"logging_off"			},
	{ MR_NO_RAISE_ON_BREAK,		"raise_on_break_off"		},
	{ MR_RAISE_ON_BREAK,		"raise_on_break_on"		},
	{ MR_LOWER_ON_RUN,		"lower_on_run"			},
	{ MR_ICONIFY_ON_RUN,		"iconify_on_run"		},
};

static actmap_t Am_target[] = {
	{ MR_LOAD_ONE_LIBRARY,		"load_library"			},
	{ MR_SAVE_STATE,		"save_state"			},
	{ MR_LOAD_STATE,		"load_state"			},
	{ MR_DROP_STATE,		"drop_state"			},
	{ MR_RESCAN_INIT_FILE,		"rescan_init_file"		},
};

static actmap_t Am_tmenu[] = {
	{ MR_TGT_START,			"start"				},
	{ MR_TGT_CONT,			"cont"				},
	{ MR_TGT_STEP,			"step"				},
	{ MR_TGT_NEXT,			"next"				},
	{ MR_TGT_STOP,			"stop"				},
	{ MR_TGT_KILL,			"kill"				},
};

static actmap_t Am_envhead[] = {
	{ MR_RESET_ENVTAB,		"reset_envtab"			},
	{ MR_SHOW_ALL_ENV_ENTRIES,	"show_all_env_entries"		},
	{ MR_HIDE_ALL_ENV_ENTRIES,	"hide_all_env_entries"		},
	{ MR_ADD_ENV_ENTRY,		"add_env_entry"			},
};

static actmap_t Am_env[] = {
	{ MR_DELETE_ENV_ENTRY,		"delete_env_entry"		},
	{ MR_HIDE_ENV_ENTRY,		"hide_env_entry"		},
	{ MR_APPEND_ENV_ENTRY,		"append_env_entry"		},
};

#define AM(actmap)	actmap,	NEL(actmap)

static mcmds_t Mnames[] = {
	{ "target",	do_target,		TRUE,	AM(Am_target)	},
	{ "globals",	do_globals,		TRUE,	AM(Am_globals)	},
	{ "srchead",	do_srchead,		TRUE,	AM(Am_srchead)	},
	{ "source",	do_file,		TRUE,	AM(Am_source)	},
	{ "function",	do_func,		TRUE,	AM(Am_source)	},
	{ "block",	do_block,		TRUE,	AM(Am_block)	},
	{ "var",	do_var_or_expr,		TRUE,	AM(Am_var)	},
	{ "expr",	do_var_or_expr,		TRUE,	AM(Am_var)	},
	{ "bphead",	do_bps,			TRUE,	AM(Am_bphead)	},
	{ "bpt",	do_bpt,			TRUE,	AM(Am_bpt)	},
	{ "sghead",	do_sgh,			TRUE,	AM(Am_sighead)	},
	{ "signal",	do_sig,			TRUE,	AM(Am_signal)	},
	{ "mhead",	do_mhead,		TRUE,	AM(Am_mhead)	},
	{ "cbhead",	do_cbhead,		TRUE,	AM(Am_cbhead)	},
	{ "module",	do_module,		TRUE,	AM(Am_module)	},
	{ "module-func",do_module_func,		TRUE,	AM(Am_module_func) },
	{ "cblock",	do_cblock,		TRUE,	AM(Am_cblock)	},
	{ "envhead",	do_envhead,		TRUE,	AM(Am_envhead)	},
	{ "env",	do_env,			TRUE,	AM(Am_env)	},
	{ "pmenu",	do_permanent_menu,	FALSE,	AM(Am_pmenu)	},
	{ "tmenu",	do_target_menu,		FALSE,	AM(Am_tmenu)	},
	{ "outwin",	do_output_window_menu,	FALSE,	AM(Am_outwin)	},
	{ "srcwin",	do_source_window_menu,	FALSE,	AM(Am_srcwin)	},
};

static int
r_flush()
{
	if (fflush(Record_fp) == EOF || ferror(Record_fp)) {
		sysf("Error writing to %s: %s",
		     Record_file_name, get_errno_str());
		return -1;
	}

	return 0;
}

static void
do_permanent_menu(unused_objid, cmd, unused_arg)
objid_t unused_objid;
int cmd;
char *unused_arg;
{
	permanent_menu_func((char *)NULL, -1, cmd);
}

static void
do_target_menu(unused_objid, cmd, unused_arg)
objid_t unused_objid;
int cmd;
char *unused_arg;
{
	if (Have_window) {
		target_menu_index_t tmi;
		target_menu_info_t *tm;
		int md;

		switch (cmd) {
			case MR_TGT_START:  tmi = TM_START;	break;
			case MR_TGT_CONT: tmi = TM_CONT;	break;
			case MR_TGT_STEP: tmi = TM_STEP;	break;
			case MR_TGT_NEXT: tmi = TM_NEXT;	break;
			case MR_TGT_STOP: tmi = TM_STOP;	break;
			case MR_TGT_KILL: tmi = TM_KILL;	break;
			case MR_TGT_DONT_KILL: return;
			default: panic("bad cmd"); tmi = TM_KILL;
		}
		tm = get_target_menu_info();
		md = tm->tm_mdtab[(int)tmi].md;

		target_menu_func((char *)NULL, md, cmd);
	}
	else {
		do_menu_target_command(cmd);
	}
}

static void
do_source_window_menu(unused_objid, cmd, unused_arg)
objid_t unused_objid;
int cmd;
char *unused_arg;
{
	source_window_menu_func((char *)get_current_srcwin(), -1, cmd);
}

static void
do_output_window_menu(unused_objid, cmd, unused_arg)
objid_t unused_objid;
int cmd;
char *unused_arg;
{
	do_output_window_command(get_current_outwin(), cmd);
}

static const char *
rv_to_name(mc, rv)
mcmds_t *mc;
int rv;
{
	static char buf[20];
	int i;

	for (i = 0; i < mc->mc_actmap_size; ++i)
		if (rv == mc->mc_actmap[i].am_rv)
			return mc->mc_actmap[i].am_name;
	
	strnf(buf, sizeof(buf), "<%d>", rv);
	return buf;
}

static int
name_to_rv(mc, name, p_rv)
mcmds_t *mc;
const char *name;
int *p_rv;
{
	int i;
	char junkc;

	for (i = 0; i < mc->mc_actmap_size; ++i) {
		if (strcmp(name, mc->mc_actmap[i].am_name) == 0) {
			*p_rv = mc->mc_actmap[i].am_rv;
			return 0;
		}
	}
	
	if (sscanf(name, "<%d>%c", p_rv, &junkc) == 1)
		return 0;

	return -1;
}

static void
list_rvs(fp, mc)
FILE *fp;
mcmds_t *mc;
{
	int i;

	fprintf(fp, "Return value names for menu `%s':", mc->mc_name);

	for (i = 0; i < mc->mc_actmap_size; ++i)
		fprintf(fp, " %s", mc->mc_actmap[i].am_name);
	
	fputc('\n', fp);
	fflush(fp);
}

static mcmds_t *
mname_to_menu(name)
const char *name;
{
	int i;

	for (i = 0; i < NEL(Mnames); ++i)
		if (strcmp(Mnames[i].mc_name, name) == 0)
			return &Mnames[i];
	
	return NULL;
}

static void
list_mnames(fp)
FILE *fp;
{
	int i;

	fputs("Menu names:", fp);

	for (i = 0; i < NEL(Mnames); ++i)
		fprintf(fp, " %s", Mnames[i].mc_name);
	
	fputc('\n', fp);
	fflush(fp);
}

static bool
mf_spaces(unused_fline, p_uline)
const char *unused_fline, **p_uline;
{
	const char *s;

	for (s = *p_uline; isspace(*s); ++s)
		;
	
	if (s == *p_uline)
		return FALSE;

	*p_uline = s;
	return TRUE;
}

static bool
mf_value(fline, p_uline)
const char *fline, **p_uline;
{
	char *endstr;

	strtol(*p_uline, &endstr, (fline[1] == 'h') ? 16 : 10);

	if (endstr == *p_uline)
		return FALSE;
	
	*p_uline = endstr;
	return TRUE;
}

static bool
mf_junkfuncptr(unused_fline, p_uline)
const char *unused_fline, **p_uline;
{
	const char *s;
	bool need_close;

	if (mf_badaddr(p_uline))
		return TRUE;
	
	s = *p_uline;

	need_close = *s == '(';
	if (need_close)
		++s;
	
	while (isalnum(*s) || (*s != '\0' && strchr("_$+[]", *s) != NULL))
		++s;
	
	if (need_close) {
		if (*s++ != ')')
			return FALSE;
	}

	if (s[0] != '(' || s[1] != ')')
		return FALSE;

	*p_uline = s + 2;
	return TRUE;
}

static bool
mf_junkstring(fline, p_uline)
const char *fline, **p_uline;
{
	static const char nullstr[] = "*NULL";
	const char *s;

	if (mf_junkaddr(fline, p_uline))
		return TRUE;
	
	if (strncmp(*p_uline, nullstr, sizeof(nullstr) - 1) == 0) {
		*p_uline += sizeof(nullstr) - 1;
		return TRUE;
	}

	if (**p_uline != '"')
		return FALSE;
	
	for (s = *p_uline + 1; *s != '"' && *s != '\0'; ++s)
		if (*s == '\\' && s[1] == '"')
			++s;
	
	if (*s == '"')
		++s;
	
	*p_uline = s;
	return TRUE;
}

static bool
mf_junkaddr(unused_fline, p_uline)
const char *unused_fline, **p_uline;
{
	char *endstr;

	if (strncmp(*p_uline, "0x", 2) == 0) {
		strtol(*p_uline, &endstr, 16);

		if (endstr > *p_uline) {
			*p_uline = endstr;
			return TRUE;
		}
	}

	return mf_badaddr(p_uline);
}

static bool
mf_badaddr(p_uline)
const char **p_uline;
{
	static const char badaddr[] = "<bad address ";
	static const char nullstr[] = "NULL";
	char *endstr;

	if (strncmp(*p_uline, badaddr, sizeof(badaddr) - 1) == 0) {
		strtol(*p_uline + sizeof(badaddr) - 1, &endstr, 0);

		if (endstr > *p_uline) {
			if (*endstr == '>')
				++endstr;
			*p_uline = endstr;
			return TRUE;
		}
	}

	if (strncmp(*p_uline, nullstr, sizeof(nullstr) - 1) == 0) {
		*p_uline += sizeof(nullstr) - 1;
		return TRUE;
	}
	
	return FALSE;
}

static bool
mf_addr(unused_fline, p_uline)
const char *unused_fline, **p_uline;
{
	char *endstr;
	long val;

	if (strncmp(*p_uline, "0x", 2) != 0)
		return FALSE;

	val = strtol(*p_uline, &endstr, 16);

	if (val == 0 || endstr == *p_uline)
		return FALSE;
	
	*p_uline = endstr;
	return TRUE;
}

static bool
mf_choice(fline, p_uline)
const char *fline, **p_uline;
{
	const char *s;

	fline += 2;
	s = fline;

	for (;;) {
		if (*s == '\0')
			panic("val botch in mfc");

		if (*s == '|' || *s == CLOSE_BRACE) {
			if (strncmp(*p_uline, fline, (size_t)(s - fline)) == 0){
				*p_uline += s - fline;
				return TRUE;
			}

			if (*s == CLOSE_BRACE)
				return FALSE;
			
			fline = s + 1;
		}

		++s;
	}
}

static bool
lines_match(fline, uline)
const char *fline, *uline;
{
	static struct {
		const char *name;
		bool (*func)PROTO((const char *ffline, const char **p_uline));
	} mf[] = {
		{ "{addr}",		mf_addr		},
		{ "{value}",		mf_value	},
		{ "{hex}",		mf_value	},
		{ "{junkaddr}",		mf_junkaddr	},
		{ "{junkfuncptr}",	mf_junkfuncptr	},
		{ "{junkstring}",	mf_junkstring	},
		{ "{spaces}",		mf_spaces	},
		{ "{?",			mf_choice	},
	};

	for (;;) {
		int i;
		const char *endf;

		if (*fline == *uline) {
			if (*fline == '\0')
				return TRUE;

			/*  Runs of spaces can be of different lengths -
			 *  this often happens when we edit the script files.
			 */
			if (*fline == ' ') {
				while (*fline == ' ')
					++fline;
				while (*uline == ' ')
					++uline;
			}
			else {
				++fline;
				++uline;
			}

			continue;
		}

		if ((endf = strchr(fline, CLOSE_BRACE)) == NULL)
			return FALSE;

		for (i = 0; i < NEL(mf); ++i) {
			if (strncmp(mf[i].name, fline, strlen(mf[i].name)) == 0)
				break;
		}
		
		if (i == NEL(mf) || !(*mf[i].func)(fline, &uline))
			return FALSE;
		
		fline = endf + 1;
	}
}

static int
checkline(upsline, endword)
const char *upsline, *endword;
{
	char *line;
	
	for (;;) {
		char *cond;
		bool cond_holds;

		if (r_getline(FALSE, &line) != 0)
			return -1;
		
		if (*line == '\t')
			break;

		if (strcmp(line, endword) == 0) {
			if (upsline == NULL)
				return 0;
			sysf("Unexpected extra ups output line `%s'", upsline);
			return Want_stop_on_check_failure ? -1 : 1;
		}

	 	if (strncmp(line, "if ", 3) != 0) {
			sysf("Unexpected extra line `%s'", line);
			return Want_stop_on_check_failure ? -1 : 1;
		}
		
		cond = line + 3;
		for (line = cond; !isspace(*line) && *line != '\0'; ++line)
			;
		if (*line != '\0')
			*line++ = '\0';
		
		if (check_cond(cond, &cond_holds) != 0)
			return -1;
		
		if (cond_holds)
			break;
	}

	if (upsline == NULL) {
		sysf("Unexpected end of ups output\n\tExpected: `%s'", line);
		return -1;
	}

	while (isspace(*line))
		++line;
	while (isspace(*upsline))
		++upsline;

	if (cmp_lines("line", line, upsline) != 0)
		return Want_stop_on_check_failure ? -1 : 1;

	return 0;
}

static int
dump_obj(arg, level, line)
char *arg;
int level;
char *line;
{
	int res;
	
	res = 0;

	if (Record_fp != NULL) {
		fprintf(Record_fp, "\t%*s%s\n", level * 4, "", line);
		if (r_flush() != 0)
			res = -1;
	}

	if (res == 0 && Replay_fp != NULL)
		res = checkline(line, "endcheck");

	/*  The format_obj callbacks all use strf() to create the line.
	 */
	free(line);
	
	return res;
}

static int
record_obj_dump(obj)
objid_t obj;
{
	int res;
	
	if (Record_fp != NULL)
		show_obj_path("check", obj, '\n', Record_fp);

	if ((res = dump_object(obj, dump_obj, (char *)NULL, OBJ_SELF)) != 0)
		return res;
	if ((res = dump_object(obj, dump_obj, (char *)NULL,
			       				OBJ_DESCENDENTS)) != 0)
		return res;

	if (Record_fp != NULL) {
		fputs("endcheck\n", Record_fp);
		r_flush();
	}

	return 0;
}

static int
skip_to(word)
const char *word;
{
	char *line;

	while (r_getline(TRUE, &line) == 0) {
		if (strcmp(line, word) == 0)
			return 0;
	}
	
	return -1;
}

static int
check_shell_line(line)
const char *line;
{
	int res;
	char *shell_line, *linecopy, *file_line;

	/*  The line to be checked came from fpgetline() and r_getline()
	 *  also calls fpgetline(), so stash the line.
	 */
	linecopy = shell_line = strsave(line);

	if (r_getline(TRUE, &file_line) != 0)
		return -1;
	
	while (isspace(*shell_line))
		++shell_line;
	
	res = cmp_lines("Shell output", file_line, shell_line);

	free(linecopy);
	return res;
}

static int
cf_shellcmd(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	char *line, *shellcmd;
	const char *usercmd;
	int res, status;
	bool expect_output;
	FILE *fp;

	if (argc != 1) {
		sysf("Usage: shellcmd cmdline");
		return -1;
	}
	
	if (!Runcmds)
		return skip_to("endshell");

	expect_output = strcmp(cmd, "shell") == 0;

	shellcmd = strf("2>&1 %s", args[0]);
	usercmd = shellcmd + 5;		/* Don't show redir in error messages */

	if ((fp = (FILE *)popen(shellcmd, "r")) == NULL) {
		sysf("Can't run `%s'", usercmd);
		free(shellcmd);
		return -1;
	}

	res = 0;
	while ((line = fpgetline(fp)) != NULL) {
		if (expect_output) {
			if (check_shell_line(line) != 0) {
				res = -1;
				break;
			}
		}
		else {
			if (res == 0) {
				sysf("Unexpected output from `%s':", usercmd);
				res = -1;
			}
			fprintf(stderr, "\t%s\n", line);
		}
	}

	if (ferror(fp)) {
		sysf("Error reading output from `%s'", usercmd);
		res = -1;
	}

	if ((status = pclose(fp)) != 0) {
		if ((status & 0x7f) != 0) {
			sysf("`%s' died of signal %d%s", usercmd,
				status & 0x7f,
				(status & 0x80) ? " (and dumped core)" : "");
		}
		else {
			sysf("`%s' exited with status %d",
						usercmd, (status >> 8) & 0xff);
		}
		res = -1;
	}

	if (expect_output && res == 0) {
		if (r_getline(TRUE, &line) != 0)
			res = -1;
		else if (strcmp(line, "endshell") != 0) {
			sysf("Expected `endshell'");
			res = -1;
		}
	}

	free(shellcmd);
	return res;
}

static int
cf_output(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	static const char endoutput[] = "endoutput";
	int res;
	char *line, *text, *s;

	if (!Runcmds)
		return skip_to(endoutput);

	if (argc != 0) {
		sysf("Usage: %s", cmd);
		return -1;
	}

	if (!get_output_text(&text))
		return -1;

	line = s = text;
	res = -1;
	
	for (;;) {
		if (*s == '\0') {
			res = 0;
			break;
		}
		
		if (*s == '\n') {
			*s = '\0';
			
			if ((res = checkline(line, endoutput)) != 0)
				break;

			line = s + 1;
		}

		++s;
	}

	if (res == 0) {
		res = checkline((const char *)NULL, endoutput);
	}
	else {
		skip_to("endoutput");

		if (res == 1) {
			Had_check_failure = TRUE;
			res = 0;
		}
	}

	free(text);

	return res;
}

static int
cf_check(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	static const char endcheck[] = "endcheck";
	objid_t obj;
	int res;

	if (!Runcmds)
		return skip_to(endcheck);

	if (argc != 1) {
		sysf("Usage: %s objpath", cmd);
		return -1;
	}

	if ((obj = path_to_obj(args[0])) == NULL)
		return -1;

	if ((res = record_obj_dump(obj)) != 0) {
		skip_to(endcheck);

		if (res == 1) {
			Had_check_failure = TRUE;
			res = 0;
		}
		
		return res;
	}

	return checkline((const char *)NULL, endcheck);
}

void
td_set_default_obj_to_selection()
{
	sel_t *sel;

	sel = get_selection();
	
	if (sel == NULL)
		Default_obj = NULL;
	else if (sel->se_next != NULL)
		sysf("More than one object selected");
	else
		Default_obj = sel->se_code;
}

void
td_check(path)
const char *path;
{
	FILE *save_record_fp;
	
	/*  Send output to stdout if we are not recording.
	 */
	save_record_fp = Record_fp;
	if (Record_fp == NULL)
		Record_fp = stdout;

	while (isspace(*path) && *path != '\0')
		++path;

	if (*path == '\0') {
		if (Default_obj == NULL) {
			sysf("No default object");
			return;
		}
		record_obj_dump(Default_obj);
	}
	else if (strcmp(path, "sel") == 0) {
		sel_t *sel;

		for (sel = get_selection(); sel != NULL; sel = sel->se_next)
			record_obj_dump(sel->se_code);
	}
	else if (strcmp(path, "output") == 0) {
		char *text;

		if (!get_output_text(&text))
			return;
		
		fputs("output\n", Record_fp);
		dump_text(text, Record_fp);
		fputs("endoutput\n", Record_fp);

		free(text);
	}
	else {
		objid_t obj;

		if ((obj = path_to_obj(path)) != NULL)
			record_obj_dump(obj);
	}

	Record_fp = save_record_fp;
}

static bool
get_output_text(p_text)
char **p_text;
{
	Outwin *ow;
	
	if ((ow = get_current_outwin()) == NULL) {
		sysf("No output window created yet");
		return FALSE;
	}

	*p_text = get_buffer_contents(outwin_get_buffer(ow));
	return TRUE;
}

#ifdef __STDC__
static void
sysf(const char *fmt, ...)
{
#else
static void
sysf(va_alist)
va_dcl
{
	const char *fmt;
#endif
	va_list args;
	char buf[100];
	char *line;

#ifdef __STDC__
	va_start(args, fmt);
#else
	va_start(args);
	fmt = va_arg(args, const char *);
#endif
	line = formf(buf, sizeof(buf), fmt, args);
	va_end(args);

	sysf_ofunc(line);
}

static void
sysf_ofunc(mesg)
const char *mesg;
{
	if (!Interactive && Replay_file_name != NULL && Replay_lnum != 0)
		fprintf(stderr, "%s,%d: ", Replay_file_name, Replay_lnum);
	
	fputs(mesg, stderr);
	fputc('\n', stderr);
}

bool
td_set_select_recording(val)
bool val;
{
	bool oldval;

	oldval = Want_select_recording;
	Want_select_recording = val;
	return oldval;
}

void
td_record_select(obj, flags)
objid_t obj;
int flags;
{
	const char *op;

	if (Record_fp == NULL || !Want_select_recording)
		return;
	
	if ((flags & (SEL_CLEARING | SEL_CHANGING | SEL_DELETING))
							!= SEL_CHANGING)
		return;
	
	if ((flags & SEL_ON) != 0)
		op = (get_num_selected() > 1) ? "addselect" : "select";
	else
		op = "deselect";

	show_obj_path(op, obj, '\n', Record_fp);
}

void
td_record_object_removal(obj)
objid_t obj;
{
	if (Record_fp != NULL)
		show_obj_path("remove", obj, '\n', Record_fp);
}

static void
show_obj_path(op, obj, endc, fp)
const char *op;
objid_t obj;
int endc;
FILE *fp;
{
	static ebuf_t *eb = NULL;
	alloc_pool_t *ap;
	int i, ncomp;
	const char **vec;

	ap = alloc_create_pool();
	obj_to_vec(ap, obj, &vec, &ncomp);

	eb = ebuf_start(eb, TRUE);

	for (i = 0; i < ncomp; ++i) {
		if (i != 0)
			ebuf_add(eb, "|", 1);
		ebuf_addstr(eb, vec[i]);
	}
	ebuf_add(eb, "", 1);
	
	fputs(op, fp);
	fputc(' ', fp);
	put_quoted_string(fp, ebuf_get(eb, (int *)NULL));
	fputc(endc, fp);
	fflush(fp);

	alloc_free_pool(ap);
}

void
td_record_menu_command(mname, rv)
const char *mname;
int rv;
{
	const char *rv_name;
	mcmds_t *mc;

	if (Record_fp == NULL)
		return;
	
	if ((mc = mname_to_menu(mname)) == NULL)
		panic("unknown mname");

	rv_name = rv_to_name(mc, rv);

	fprintf(Record_fp, "menu %s %s\n", mname, rv_name);
	r_flush();
}

void
td_record_debug_command(cmd)
const char *cmd;
{
	if (Record_fp != NULL) {
		fprintf(Record_fp, "debug \"%s\"\n", cmd);
		r_flush();
	}
}

void
td_record_show_var(fil, lnum, name)
fil_t *fil;
int lnum;
const char *name;
{
	if (Record_fp != NULL) {
		if (fil != NULL) {
			fprintf(Record_fp, "showvar %s %d %s\n",
						fil->fi_name, lnum, name);
		}
		else {
			fprintf(Record_fp, "show %s\n", name);
		}

		r_flush();
	}
}

bool
td_set_displayed_source(fil, lnum, op)
fil_t *fil;
int lnum;
const char *op;
{
	/*  We don't want the initial source display of main to be the
	 *  first line in the file.  This is because it would then come
	 *  before the `version' line and any SCCS comment.  Thus this
	 *  flag, which is set by a call to us with fil==NULL after the
	 *  initial source display (see ups()).
	 */
	static bool no_source_stuff_yet = TRUE;

	if (fil == NULL) {
		no_source_stuff_yet = FALSE;
		return FALSE;
	}

	if (Record_fp != NULL || Replay_fp != NULL)
		Current_displayed_fil = fil;
	
	if (no_source_stuff_yet)
		return !Have_window;

	if (Record_fp != NULL) {
		fprintf(Record_fp, "%s %s %d\n", op, fil->fi_name,lnum);
		r_flush();
	}

	if (Replay_fp != NULL && Want_displayed_source_check) {
		char *line;

		line = strf("%s %d", fil->fi_name, lnum);
		if (check_message(op, line) != 0)
			Check_message_failed = TRUE;
		free(line);
	}
	
	return !Have_window;
}

int
td_get_displayed_fil(p_fil)
fil_t **p_fil;
{
	if (Have_window)
		return -1;
	
	*p_fil = Current_displayed_fil;
	return 0;
}

void
td_record_func_and_lnum_cmd(f, fil, lnum, op)
func_t *f;
fil_t *fil;
int lnum;
const char *op;
{
	if (Record_fp != NULL) {
		fprintf(Record_fp, "%s %s %s %d\n", op,
					fil->fi_name, f->fu_name, lnum);
		r_flush();
	}
}

void
td_record_field_edit(obj, fnum, value)
objid_t obj;
int fnum;
const char *value;
{
	if (Record_fp != NULL) {
		int objtype;
		fnamemap_t *fmtab, *fm;
		
		show_obj_path("edit", obj, ' ', Record_fp);

		objtype = ups_get_object_type(obj);

		if (objtype < 0 || objtype > OT_MAXTYPE)
			panic("objtype bad in ce");
		fmtab = Objtab[objtype].ot_fnamemap;

		for (fm = fmtab; fm->fnum != fnum; ++fm) {
			if (fm->name == NULL)
				panic("bad fmtab botch in rfe");
		}
		
		fputs(fm->name, Record_fp);
		fputc(' ', Record_fp);
		put_quoted_string(Record_fp, value);
		fputc('\n', Record_fp);
		
		r_flush();
	}
}

void
td_record_refresh()
{
	if (Record_fp != NULL) {
		fputs("refresh\n", Record_fp);
		r_flush();
	}
}

static int
cf_version(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	char *endstr;
	int version;

	if (argc == 0) {
		printf("Version %d - %s\n", CURRENT_VERSION, ups__sccsdata[0]);
		return 0;
	}

	version = strtol(args[0], &endstr, 10);

	if (endstr == args[0] || *endstr != '\0' || version < 0) {
		sysf("Version `%s' is not a positive integer", args[0]);
		return -1;
	}

	if (version > CURRENT_VERSION) {
		sysf("Warning: expected file version <= %d - found %d",
						CURRENT_VERSION, version);
	}

	return 0;
}

static int
cf_cond(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	if (argc != 1) {
		sysf("Usage: %s cond", cmd);
		return -1;
	}

	return handle_cond(cmd, args[0]);
}

static const char *
get_idstr()
{
	struct utsname utsname;

	if (Replay_idstr == NULL) {
		if (uname(&utsname) == -1)
			panic("uname() failed");

		Replay_idstr = strf("%s-%s-%s",
				    utsname.sysname,
				    utsname.release,
				    utsname.machine);
	}

	return Replay_idstr;
}

static bool
str_matches_pat(str, pat)
const char *str, *pat;
{
	for (;;) {
		while (*str != '\0' && tolower(*str) == tolower(*pat)) {
			++str;
			++pat;
		}

		if (*str == '-' || *str == '\0') {
			while (*pat != *str && *pat != '\0')
				++pat;
		}
		
		if (*str != *pat)
			return FALSE;

		if (*str == '\0')
			return TRUE;

		++str;
		++pat;
	}
}

static int
check_cond(cond, p_cond_holds)
const char *cond;
bool *p_cond_holds;
{
	bool invert, holds;

	invert = *cond == '!';
	if (invert)
		++cond;

	holds = (Want_verbose_test && strcmp(cond, "verbose") == 0) ||
		str_matches_pat(cond, get_idstr());
	
	*p_cond_holds = invert ? !holds : holds;
	return 0;
}

static const char *
not(cond)
const char *cond;
{
	static char *last = NULL;

	if (*cond == '!')
		return cond + 1;
	
	if (last != NULL)
		free(last);
	
	return last = strf("!%s", cond);
}

static int
handle_cond(cmd, cond)
const char *cmd, *cond;
{
	typedef struct condlist_s {
		char *cond;
		bool seen_else;
		int lnum;
		bool old_runcmds;
		struct condlist_s *outer;
	} condlist_t;
	static condlist_t *condlist = NULL;
	condlist_t *cl;
	const char *expected_cond;
	bool cond_holds, is_else;

	if (check_cond(cond, &cond_holds) != 0)
		return -1;

	if (strcmp(cmd, "if") == 0) {
		cl = (condlist_t *)e_malloc(sizeof(condlist_t));
		cl->cond = strsave(cond);
		cl->seen_else = FALSE;
		cl->lnum = Replay_lnum;
		cl->old_runcmds = Runcmds;
		cl->outer = condlist;
		condlist = cl;

		if (!cond_holds)
			Runcmds = FALSE;

		return 0;
	}

	if ((cl = condlist) == NULL) {
		sysf("`%s' command with no corresponding `if'", cmd);
		return -1;
	}

	is_else = strcmp(cmd, "endif") != 0;
	if (is_else && strcmp(cmd, "else") != 0)
		panic("bad cmd in hc");

	if (is_else && cl->seen_else) {
		sysf("Already seen `else %s' on line %d",
						not(cl->cond), cl->lnum);
		return -1;
	}
		
	expected_cond = (is_else || cl->seen_else) ? not(cl->cond) : cl->cond;

	if (strcmp(cond, expected_cond) != 0) {
		sysf("`%s %s' doesn't match line %d", cmd, cond, cl->lnum);
		return -1;
	}

	if (is_else) {
		if (cl->old_runcmds)
			Runcmds = !Runcmds;
		
		cl->seen_else = TRUE;
		cl->lnum = Replay_lnum;
	}
	else if (strcmp(cmd, "endif") == 0) {
		Runcmds = cl->old_runcmds;
		condlist = cl->outer;

		free(cl->cond);
		free((char *)cl);
	}

	return 0;
}

static int
cf_message(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	ebuf_t *eb;
	int i;

	eb = ebuf_create(TRUE);
	for (i = 0; i < argc; ++i) {
		if (i > 0)
			ebuf_add(eb, " ", 1);
		ebuf_addstr(eb, args[i]);
	}
	ebuf_add(eb, "", 1);
	
	sysf("Unexpected %s `%s'", cmd, (char *)ebuf_get(eb, (int *)NULL));
	ebuf_free(eb);
	
	return -1;
}

static int
cf_echo(cmd, unused_argc, args)
const char *cmd;
int unused_argc;
char **args;
{
	bool no_newline;

	if (strcmp(cmd, "warn") == 0)
		printf("%s,%d: ", Replay_file_name, Replay_lnum);

	no_newline = args[0] != NULL && strcmp(args[0], "-n") == 0;
	if (no_newline)
		++args;

	for (; *args != NULL; ++args) {
		fputs(*args, stdout);
		if (args[1] != NULL)
			fputc(' ', stdout);
	}

	if (!no_newline)
		fputc('\n', stdout);
	
	return 0;
}

static int
cf_exit(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	if (argc != 0) {
		sysf("Usage: %s", cmd);
		return -1;
	}

	re_set_exit_event_loop_flag();
	
	return 0;
}

static int
cf_help(cmd, argc, unused_args)
const char *cmd;
int argc;
char **unused_args;
{
	int i;

	if (argc != 0) {
		sysf("Usage: %s", cmd);
		return -1;
	}

	fputs("Commands:", stdout);

	for (i = 0; i < sizeof Cmdtab / sizeof *Cmdtab; ++i)
		printf(" %s", Cmdtab[i].name);

	fputc('\n', stdout);
	fflush(stdout);

	return 0;
}

static int
cf_load(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	if (argc != 1) {
		sysf("Usage: %s library-name", cmd);
		return -1;
	}
	if (noload_flag == TRUE)
	{
		sysf("Cannot use `load' and `noload' together");
		sysf("`load' for %s ignored", *args);
		return -1;
	}
	load_flag = TRUE;
	add_to_load_path(*args, 0, 0); /* load */
	return 0;
}

static int
cf_noload(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	if (argc != 1) {
		sysf("Usage: %s library-name", cmd);
		return -1;
	}
	if (load_flag == TRUE)
	{
		sysf("Cannot use `load' and `noload' together");
		sysf("`noload' for %s ignored", *args);
		return -1;
	}
	noload_flag = TRUE;
	add_to_load_path(*args, 1, 0); /* noload */
	return 0;
}

static int
cf_use(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	if (argc != 1) {
		sysf("Usage: %s <source-directory>", cmd);
		return -1;
	}
	srcpath_add_path(*args);
	return 0;
}

static int
cf_checksrc(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	if (argc != 1 || (strcmp(args[0], "off") != 0 &&
						strcmp(args[1], "on") != 0)) {
		sysf("Usage: %s on|off", cmd);
		return -1;
	}

	Want_displayed_source_check = strcmp(args[0], "on") == 0;

	return 0;
}

static int
cf_replaymode(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	if (argc != 1) {
		sysf("Usage: %s %s", cmd, td_get_replay_mode_list());
		return -1;
	}

	return td_set_replay_mode(args[0]);
}

void
td_set_replay_idstr(idstr)
const char *idstr;
{
	Replay_idstr = idstr;
}

int
td_set_replay_mode(cmd)
const char *cmd;
{
	const char *mode;
	bool doit;

	if ((mode = strchr(cmd, ':')) == NULL) {
		doit = TRUE;
		mode = cmd;
	}
	else {
		Replay_mode *rm;
		char *endstr;
		int lnum;

		lnum = strtol(cmd, &endstr, 10);

		if (endstr != mode || lnum <= 0) {
			sysf("Line number `%.*s' is not a positive integer",
			     mode - cmd, cmd);
			return -1;
		}

		++mode;
		
		rm = (Replay_mode *)e_malloc(sizeof(Replay_mode));
		rm->lnum = lnum;
		rm->mode = strsave(mode);
		rm->next = Replay_modes;
		Replay_modes = rm;

		doit = FALSE;
	}
	
	if (strcmp(mode, "cfcont") == 0) {
		if (doit) {
			Want_stop_on_check_failure = FALSE;
		}
	}
	else if (strcmp(mode, "cfstop") == 0) {
		if (doit) {
			Want_stop_on_check_failure = TRUE;
		}
	}
	else if (strcmp(mode, "verbose") == 0) {
		if (doit) {
			Want_verbose_test = TRUE;
		}
	}
	else if (strcmp(mode, "wait") == 0) {
		if (doit) {
			Want_wait_for_user = TRUE;
			Want_cmd_echo = TRUE;
			Want_wait_on_check = FALSE;
		}
	}
	else if (strcmp(mode, "waitcheck") == 0) {
		if (doit) {
			Want_wait_for_user = FALSE;
			Want_cmd_echo = TRUE;
			Want_wait_on_check = TRUE;
		}
	}
	else if (strcmp(mode, "echo") == 0) {
		if (doit) {
			Want_wait_for_user = FALSE;
			Want_cmd_echo = TRUE;
			Want_wait_on_check = FALSE;
		}
	}
	else if (strcmp(mode, "noecho") == 0) {
		if (doit) {
			Want_wait_for_user = FALSE;
			Want_cmd_echo = FALSE;
			Want_wait_on_check = FALSE;
		}
	}
	else {
		sysf("Unknown replay mode `%s' - expected %s",
		     mode, td_get_replay_mode_list());
		return -1;
	}

	return 0;
}

const char *
td_get_replay_mode_list()
{
	return "cfcont|cfstop|verbose|wait|waitcheck|echo|noecho";
}

static int
cf_refresh(cmd, argc, unused_args)
const char *cmd;
int argc;
char **unused_args;
{
	unsigned code;

	if (argc != 0) {
		sysf("Usage: %s", cmd);
		return -1;
	}

	code = (strcmp(cmd, "resize") == 0) ? EV_WINDOW_RESIZED
					    : EV_WINDOW_EXPOSED;

	td_record_refresh();

	if (Have_window)
		re_redraw_root(code, TRUE);

	return 0;
}

static int
cf_bpt(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
  char *fname;
  func_t *f, *f1 = NULL;

  fname = args[0];
  if (find_func_by_name(fname, &f, &f1, TRUE) == 0)
  {
    add_breakpoint_object(f, (fil_t *)NULL, -1, TRUE);
    if (f1)
      add_breakpoint_object(f1, (fil_t *)NULL, -1, TRUE);
  }
  /* RGA allows "break" processing to continue regardless of messages */
  Check_message_failed = FALSE;
  return 0;
}


static int
cf_menu(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	const char *mname, *action;
	mcmds_t *mc;
	int rv;

	if (argc != 2 && !(argc == 1 && strcmp(args[0], "?") == 0)) {
		sysf("Usage: %s menu_name action", cmd);
		return -1;
	}
	mname = args[0];
	action = args[1];

	if ((mc = mname_to_menu(mname)) == NULL) {
		if (strcmp(mname, "?") == 0) {
			list_mnames(stdout);
			return 0;
		}
		sysf("Unknown menu name `%s'%s", mname,
				Interactive ? " - say `menu ?' for list" : "");
		return -1;
	}

	if (name_to_rv(mc, action, &rv) != 0) {
		if (strcmp(action, "?") == 0) {
			list_rvs(stdout, mc);
			return 0;
		}

		if (Interactive) {
			sysf("Unknown menu action `%s' - say `menu %s ?' for list",
								action, mname);
		}
		else {
			sysf("Unknown menu action `%s'", action);
		}

		return -1;
	}

	td_record_menu_command(mname, rv);

	if (!mc->mc_apply_to_objs) {
		(*mc->mc_func)((objid_t)NULL, rv, (char *)NULL);
	}
	else {
		int cur_objtype;

		cur_objtype = get_cur_objtype();

		if (cur_objtype == OT_NO_TYPE || get_num_selected() == 0) {
			sysf("No object selected");
			return -1;
		}

		if (cur_objtype < 0 || cur_objtype > OT_MAXTYPE)
			panic("cur_objtype bad in cm");

		if (mc->mc_func != Objtab[cur_objtype].ot_mfunc) {
			sysf("Wrong menu type for currently selected objects");
			return -1;
		}

		dynamic_menu_func((char *)NULL, -1, rv);
	}

	return 0;
}

static int
cf_bptcode(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	char *line;
	objid_t obj;
	ebuf_t *eb;
	int lnum;
	bool found_endcode;

	if (!Runcmds)	
		return skip_to("endcode");

	lnum = Replay_lnum;
	
	if (argc != 1) {
		sysf("Usage: %s objpath", cmd);
		return -1;
	}

	if ((obj = path_to_obj(args[0])) == NULL)
		return -1;

	eb = ebuf_create(TRUE);
	found_endcode = FALSE;

	while (r_getline(FALSE, &line) == 0) {
		if (strcmp(line, "endcode") == 0) {
			found_endcode = TRUE;
			break;
		}

		if (*line == '\t')
			++line;

		ebuf_addstr(eb, line);
		ebuf_add(eb, "\n", 1);
	}

	
	if (found_endcode) {
		ebuf_add(eb, "", 1);
		change_bd_text(obj, ebuf_get(eb, (int *)NULL));
	}
	
	ebuf_free(eb);

	if (!found_endcode) {
		Replay_lnum = lnum;
		sysf("Unterminated bptcode (hit EOF before finding endcode)");
		return -1;
	}
	
	return 0;
}

static int
cf_edit(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	objid_t obj;
	int objtype;
	fnamemap_t *fmtab, *fm;
	char *objpath, *fname, *fvalue;
	const char *display_string;
	
	if (argc != 3) {
		sysf("Usage: %s objpath fname value", cmd);
		return -1;
	}
	
	if (!Runcmds)
		return 0;

	objpath = args[0];
	fname = args[1];
	fvalue = args[2];

	if ((obj = path_to_obj(objpath)) == NULL)
		return -1;
	
	objtype = ups_get_object_type(obj);

	if (objtype < 0 || objtype > OT_MAXTYPE)
		panic("objtype bad in ce");
	fmtab = Objtab[objtype].ot_fnamemap;

	if (fmtab == NULL) {
		sysf("Object `%s' does not have any editable fields", objpath);
		return -1;
	}

	for (fm = fmtab; fm->name != NULL; ++fm)
		if (strcmp(fm->name, fname) == 0)
			break;
	
	if (fm->name == NULL) {
		sysf("No editable field `%s' for object `%s'", fname, objpath);
		return -1;
	}

	td_record_field_edit(obj, fm->fnum, fvalue);
	
	if ((*fm->quitfunc)(obj, fvalue, &display_string))
		obj_update_size(obj);
	
	return 0;
}

static int
cf_remove(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	objid_t obj;

	if (argc != 1) {
		sysf("Usage: %s objpath", cmd);
		return -1;
	}

	if ((obj = path_to_obj(args[0])) == NULL)
		return -1;
	
	remove_object(obj, OBJ_SELF);
	
	return 0;
}

static int
cf_select(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	objid_t obj;
	bool on, clear;

	if (argc != 1) {
		sysf("Usage: %s objpath", cmd);
		return -1;
	}
	clear = strcmp(cmd, "select") == 0;
	on = clear || strcmp(cmd, "addselect") == 0;

	if (clear)
		clear_selection();

	if ((obj = path_to_obj(args[0])) == NULL)
		return -1;
	
	select_object(obj, on, OBJ_SELF);
	Obj_to_make_visible = obj;

	return 0;
}

objid_t
path_to_obj(path)
const char *path;
{
	char **vec;
	int ncomp;
	objid_t obj;

	vec = ssplit(path, "|");
	for (ncomp = 0; vec[ncomp] != NULL; ++ncomp)
		;
	
	obj = vec_to_obj((const char **)vec, ncomp);

	free((char *)vec);

	if (obj == NULL)
		sysf("Unknown object path `%s'", path);

	return obj;
}

static int
cf_debug(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	if (argc != 1) {
		sysf("Usage: %s cmdline", cmd);
		return -1;
	}

	do_debug_command(args[0]);
	return 0;
}

static int
cf_show(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	Srcwin *sw;

	if (argc != 1) {
		sysf("Usage: %s var", cmd);
		return -1;
	}

	sw = Have_window ? get_current_srcwin() : NULL;
	show_var_from_typing_line(sw, args[0]);

	td_record_show_var((fil_t *)NULL, 0, args[0]);
	return 0;
}

static int
cf_show_var(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	const char *filename, *varname;
	char *endstr;
	fil_t *fil;
	Srcwin *sw;
	int lnum;

	if (argc != 3) {
		sysf("Usage: %s filename lnum varname", cmd);
		return -1;
	}
	filename = args[0];
	lnum = strtol(args[1], &endstr, 10);
	varname = args[2];

	if (endstr == args[1] || *endstr != '\0') {
		sysf("Line number `%s' is not a number", args[1]);
		return -1;
	}

	if ((fil = name_to_fil(filename)) == NULL) {
		sysf("Unknown filename `%s'", filename);
		return -1;
	}

	sw = Have_window ? get_current_srcwin() : NULL;
	show_var(sw, fil, lnum, varname, 0, 0, FALSE);

	td_record_show_var(fil, lnum, varname);
	
	return 0;
}

static int
cf_to_lnum(cmd, argc, args)
const char *cmd;
int argc;
char **args;
{
	const char *filename, *funcname;
	char *endstr;
	fil_t *fil;
	func_t *f;
	int lnum;

	if (argc != 3) {
		sysf("Usage: %s filename funcname lnum", cmd);
		return -1;
	}
	filename = args[0];
	funcname = args[1];
	lnum = strtol(args[2], &endstr, 10);

	if (endstr == args[2] || *endstr != '\0') {
		sysf("Line number `%s' is not a number", args[2]);
		return -1;
	}

	if ((fil = name_to_fil(filename)) == NULL) {
		sysf("Unknown filename `%s'", filename);
		return -1;
	}

	if ((f = name_and_fil_to_func(funcname, fil, TRUE)) == NULL) {
		sysf("Can't find function `%s' in file %s", funcname, filename);
		return -1;
	}

	td_record_func_and_lnum_cmd(f, fil, lnum, cmd);

	if (strcmp(cmd, "execto") == 0)
		exec_to_lnum(f, fil, lnum);
	else
		Obj_to_make_visible = add_breakpoint_object(f, fil, lnum, TRUE);
	
	return 0;
}

static cmdfunc_t
cmd_to_func(cmd, p_alwaysexec)
const char *cmd;
bool *p_alwaysexec;
{
	int i;

	for (i = 0; i < sizeof Cmdtab / sizeof *Cmdtab; ++i) {
		if (strcmp(cmd, Cmdtab[i].name) == 0) {
			*p_alwaysexec = Cmdtab[i].alwaysexec;
			return Cmdtab[i].func;
		}
	}
	
	return NULL;
}

static cmdfunc_t
preload_cmd_to_func(cmd, p_alwaysexec)
const char *cmd;
bool *p_alwaysexec;
{
	int i;
	
	for (i = 0; i < sizeof Preload_cmdtab / sizeof *Preload_cmdtab; ++i) {
		if (strcmp(cmd, Preload_cmdtab[i].name) == 0) {
			*p_alwaysexec = Preload_cmdtab[i].alwaysexec;
			return Preload_cmdtab[i].func;
		}
	}
	
	return NULL;
}

static int
process_command(line, preload_only)
char *line;
bool preload_only;
{
	cmdfunc_t func;
	int argc, res;
	char *s, *cmd, **args;
	errf_ofunc_t old_ofunc;
	bool alwaysexec, ok;

	for (s = line; !isspace(*s) && *s != '\0'; ++s)
		;
	
	cmd = line;

	if (*s != '\0')
		*s++ = '\0';
	line = s;

	if (preload_only)
	{
	  func = preload_cmd_to_func(cmd, &alwaysexec);
	  if (func == NULL) 
	    return -1;
	}
	else
	{
	  func = cmd_to_func(cmd, &alwaysexec);
	  if (func == NULL)
	  {
	    /*  Don't object to unknown commands inside a false
	     *  conditional - they can be things like edit_field stuff.
	     */
	    /* check if a preload command - otherwise must be an error */
	    func = preload_cmd_to_func(cmd, &alwaysexec);
	    if (func == NULL)
	    {
	      if (!Runcmds)
		return 0;
	      
	      sysf("Unknown command `%s'%s", cmd,
		 Interactive ? " - say `help' for a list" : "");
	      return -1;
	    }
	    else
	      return 0;
	  }
	}

	if (!Runcmds && !alwaysexec)
		return 0;
	
	old_ofunc = errf_set_ofunc(sysf_ofunc);
	ok = parse_line(line, &args, &argc);
	errf_set_ofunc(old_ofunc);
	
	if (!ok)
		return -1;
	
	if (preload_only)
	  res = (*func)(cmd, argc, args);
	else
	{
	  Obj_to_make_visible = NULL;

	  td_set_obj_updating(OBJ_UPDATING_OFF);
	  res = (*func)(cmd, argc, args);
	  td_set_obj_updating(OBJ_UPDATING_ON);

	  if (Obj_to_make_visible != NULL && Have_window)
	    ensure_visible(Obj_to_make_visible);
	} 
	free((char *)args);
	
	return res;
}

int
td_restore_replay_fp()
{
  Replay_fp = Replay_copy_fp;
  free((char *)Replay_file_name);
  Replay_file_name = Replay_copy_file_name;

  if (Replay_fp)
  {
    Replay_lnum = 0;
    Check_message_failed = FALSE;
    Interactive = isatty(fileno(Replay_fp));
  }
  return 0;
}

int
td_init_from()
{
  char *filename, *home = 0;
  FILE *fp = NULL;

  home = (char *)getenv("HOME");

  if (!home)
    return(0);

  /* look for a project-specific ups initialization file first */
  filename = ".upsinit";
  if ((fp = fopen(filename, "r")) != NULL)
  {
    filename = strdup(filename);
  }
  else
  {
    filename = strf("%s/.upsinit", home);
    if ((fp = fopen(filename, "r")) == NULL)
    {
      free((char *)filename);
      return 0;
    }
  }
  Replay_copy_fp = Replay_fp;
  Replay_fp = fp;
  Replay_copy_file_name = Replay_file_name;

  Replay_file_name = filename;

  Replay_lnum = 0;
  Check_message_failed = FALSE;
  Interactive = isatty(fileno(Replay_fp));
  
  return(1);
}

int
td_replay_from(filename)
const char *filename;
{
	if (strcmp(filename, "-") == 0) {
		Replay_fp = stdin;
		Replay_file_name = "the standard input";
	}
	else {
		if ((Replay_fp = fopen(filename, "r")) == NULL) {
			sysf("Can't open driver file %s: %s",
			     filename, get_errno_str());
			return -1;
		}
		Replay_file_name = filename;
	}

	Replay_lnum = 0;
	Check_message_failed = FALSE;
	Interactive = isatty(fileno(Replay_fp));

	if (Error_func == NULL)
		Error_func = errf_set_ofunc(show_error_on_tty);

	return 0;
}

static void
wait_for_user(line, p_quit)
const char *line;
bool *p_quit;
{
	const char *cmd;

	*p_quit = FALSE;
	
	for (cmd = line; isspace(*cmd) && *cmd != '\0'; ++cmd)
		;

	if (Runcmds && Want_cmd_echo && strcmp(cmd, "noecho") != 0) {
		if (!Interactive)
			printf("%4d ", Replay_lnum);
		fputs(line, stdout);
		fflush(stdout);

		if ((Want_wait_for_user &&
		     strcmp(cmd, "nowait") != 0 &&
		     strncmp(cmd, "version ", 8) != 0) ||
		    (Want_wait_on_check &&
		     strncmp(line, "check ", 6) == 0)) {
			int ch, lastch;

			fputs(" [nceq] ", stdout);

			lastch = '\0';
			while ((ch = getchar()) != EOF && ch != '\n')
				lastch = ch;

			if (lastch == 'e') {
				*p_quit = TRUE;
				return;
			}
			
			if (lastch == 'q')
				exit(0);

			if (lastch == 'c' || ch == EOF)
				Want_cmd_echo = FALSE;
		}
		else {
			fputc('\n', stdout);
			fflush(stdout);
		}
	}
}

static void
show_error_on_tty(mesg)
const char *mesg;
{
	if (*mesg == '\b')
		++mesg;

	if (Record_fp != NULL) {
		fprintf(Record_fp, "message %s\n", mesg);
		r_flush();
	}

	if (Replay_fp != NULL && !Interactive) {
		if (check_message("message", mesg) != 0)
			Check_message_failed = TRUE;
	}
	else if (Have_window) {
		(*Error_func)(mesg);
	}
	else {
		fprintf(stderr, "%s\n", mesg);
	}
}

static int
check_message(what, mesg)
const char *what, *mesg;
{
	char *line;
	size_t len;

	if (r_getline(TRUE, &line) != 0)
		return -1;

	/*  Allow comments, blank lines and the version string to precede
	 *  the message.  This is needed because messages can be produced
	 *  at startup (e.g. by illegal commands in state files).
	 */
	while (*line == '\0' || *line == '#' ||
	       strncmp(line, "version ", 8) == 0) {
		if (r_getline(TRUE, &line) != 0)
			return -1;
	}
	
	len = strlen(what);

	if (strncmp(line, what, len) != 0 || line[len] != ' ') {
		sysf("Expected `%s %s'", what, mesg);
		return -1;
	}
	line += strlen(what) + 1;

	return cmp_lines(what, line, mesg);
}

static int
cmp_lines(what, expected, got)
const char *what, *expected, *got;
{
	if (!lines_match(expected, got)) {
		sysf("%c%s mismatch", toupper(what[0]), &what[1]);
		fprintf(stderr, "Expected: `%s'\n", expected);
		fprintf(stderr, "     Got: `%s'\n", got);
		return -1;
	}

	return 0;
}

bool
td_set_obj_updating(val)
bool val;
{
	return Have_window ? updating(val) : FALSE;
}

bool
td_replaying()
{
	return Replay_fp != NULL;
}

bool
td_have_window()
{
	return Have_window;
}

static int
r_getline(lose_leading_whitespace, p_line)
bool lose_leading_whitespace;
char **p_line;
{
	char *line;
	Replay_mode *rm;
		
	if ((line = fpgetline(Replay_fp)) == NULL) {
		if (ferror(Replay_fp))
			sysf("Read error: %s", get_errno_str());
		return -1;
	}

	if (lose_leading_whitespace) {
		while (isspace(*line))
			++line;
	}
	
	++Replay_lnum;

	for (rm = Replay_modes; rm != NULL; rm = rm->next) {
		if (Replay_lnum == rm->lnum) {
			if (td_set_replay_mode(rm->mode) != 0)
				panic("srm botch in tev");
		}
	}
		
	*p_line = line;
	return 0;
}

void
td_set_window_flag()
{
	Have_window = TRUE;
}

void
td_set_no_window_flag()
{
	Have_window = FALSE;
}

int
td_event_loop(p_eof)
bool *p_eof;
{
	bool quit;
	char *line;
	int res;

	if (Have_window)
		set_bm_cursor(WN_STDWIN, CU_MENU);
	else
		set_dynamic_menu_updating_state(DMU_OFF);

	res = 0;

	quit = FALSE;
	Had_check_failure = FALSE;
	if (Error_func == NULL)
		Error_func = errf_set_ofunc(show_error_on_tty);
	
	for (;;) {
		if (Interactive) {
			fputs("ups> ", stdout);
			fflush(stdout);
		}

		if (r_getline(FALSE, &line) != 0)
			break;

		line = config_trim_line(line);
		if (*line == '\0')
			continue;
		
		wait_for_user(line, &quit);

		if (quit)
			break;
			
		res = process_command(line, FALSE);

		/*  Quit scripts if there is an error.
		 */
		if (Check_message_failed)
			res = -1;
		if (res != 0 && !Interactive)
			break;

		if (re_get_exit_event_loop_flag())
			break;
	}

	if (ferror(Replay_fp)) {
	  if (fclose(Replay_fp) != 0)
	    sysf("Can't close replay file: %s", get_errno_str());
	  return -1;
	}

	*p_eof = quit || feof(Replay_fp);
	if (fclose(Replay_fp) != 0)
	  sysf("Can't close replay file: %s", get_errno_str());
	Replay_fp = NULL;

	if (Have_window)
		errf_set_ofunc(Error_func);

	return Had_check_failure ? -1 : res;
}

int
td_load_loop(p_eof)
bool *p_eof;
{
	char *line;
	bool quit;

	quit = FALSE;
	for (;;) {
		if (r_getline(FALSE, &line) != 0)
			break;

		line = config_trim_line(line);
		if (*line == '\0')
			continue;
		
		wait_for_user(line, &quit);

		if (quit)
			break;
			
		process_command(line, TRUE); /* preload only */

		if (Check_message_failed && !Interactive)
			break;

		if (re_get_exit_event_loop_flag())
			break;
	}

	if (ferror(Replay_fp)) {
	  if (fclose(Replay_fp) != 0)
	    sysf("Can't close replay file: %s", get_errno_str());
	  return -1;
	}

	*p_eof = feof(Replay_fp);
	if (fclose(Replay_fp) != 0)
	  sysf("Can't close replay file: %s", get_errno_str());
	Replay_fp = NULL;
	return 0;
}

void
td_record_bpt_code_edit(obj, text)
objid_t obj;
const char *text;
{
	if (Record_fp != NULL) {
		show_obj_path("bptcode", obj, '\n', Record_fp);
		dump_text(text, Record_fp);
		fputs("endcode\n", Record_fp);
		
		r_flush();
	}
}

int
td_record_to(filename)
const char *filename;
{
	if (strcmp(filename, "-") == 0) {
		Record_fp = stdout;
		Record_file_name = "the standard output";
	}
	else {
		struct stat stbuf;

		if (lstat(filename, &stbuf) == 0) {
			sysf("Record file %s already exists", filename);
			return -1;
		}
		if (errno != ENOENT) {
			sysf("Can't stat record file %s: %s",
			     filename, get_errno_str());
			return -1;
		}
		if ((Record_fp = fopen(filename, "w")) == NULL) {
			sysf("Can't create record file %s: %s",
			     filename, get_errno_str());
			return -1;
		}
		Record_file_name = filename;
	}


	if (Error_func == NULL)
		Error_func = errf_set_ofunc(show_error_on_tty);

	fprintf(Record_fp, "version %d - %s\n",
					CURRENT_VERSION, ups__sccsdata[0]);
	r_flush();

	return 0;
}

void
rescan_init_file()
{
  free_library_load_list();
  if (td_init_from())
  {
    bool eof;
    int res;
    
    res = td_load_loop(&eof);
    
    if (res != 0)
      fprintf(stderr, "Error in init file\n");
    td_restore_replay_fp();
  }
}
