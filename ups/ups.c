/* ups.c - initialisation and entry point for ups */

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


/* @(#)ups.c	1.53 04 Jun 1995 (UKC) */
char ups_ups_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <termios.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <local/ukcprog.h>

#ifdef __STDC__
#include <unistd.h>
#endif

#include <local/wn.h>
#include <local/menu3.h>
#include <local/arg.h>		/* for arg_version */
#include <local/obj/obj.h>	/* for obj_version */
#include <mtrprog/utils.h>	/* for mtrprog_version */
#include <local/edit/edit.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "reg.h"
#include "tdr.h"
#include "exec.h"
#include "target.h"
#include "ao_dwfname.h"
#include "st.h"
#include "ui.h"
#include "obj_stack.h"
#include "obj_buildf.h"
#include "obj_bpt.h"
#include "obj_env.h"
#include "obj_target.h"
#include "cc.h"
#include "state.h"
#include "debug.h"
#include "menudata.h"
#include "cursors.h"
#include "config.h"
#include "va.h"
#include "srcpath.h"
#include "srcwin.h"
#include "exec.h"
#include "sccsdata.h"


#define DEFAULT_TABWIDTH 8

static void catch_fatal_sig PROTO((int signo));
static void add_multiple_paths PROTO((const char *ro_paths, const char *defpath));
static int is_directory PROTO((const char *name));
static void get_default_paths PROTO((const char *textname, const char **p_dsrc, const char **p_dcore));
static void usage PROTO((const char *mesg));
static int setup_display PROTO((void));
static int ups PROTO((target_t *xp, const char* textname, const char *args, bool replay,
		      bool want_window,
		      bool want_load_config, bool want_save_config,
		      bool want_save_sigs,
		      bool want_auto_start, bool use_full_path, const char *state_dir,
		      bool attach));
int main PROTO((int argc, char **argv));
static const char *checkarg PROTO((char **argv));
static void handle_panic PROTO((const char *s));
static void null_ofunc PROTO((const char *s));
static const char *make_editfont_name PROTO((const char *srcfont_name));
static bool statedir_exists PROTO((const char *state_dir));
static void set_special_edit_chars PROTO((Edit_keymap *keymap));
static void add_keymap_entry PROTO((Edit_keymap *keymap, unsigned modifiers,
				    int ch, Edit_function func));

unsigned long Debug_flags = 0;

static bool Done_panic = FALSE;

static struct termios save_stty0;
static struct termios save_stty1;

static Edit_history_item history_items[] =
{
   /*  "name"      address              default_size */
   { "TypingLine", &typing_line_history, 20},
   { "BreakPoint", &breakpoint_history,  20},
   { "BptFunct",   &bpt_history,         20},
   { "Environment",&env_history,         10},
   { "ComandArgs", &cmd_history,         10},
   { "SubScriptEdit",  &subscript_history, 5},
   { "ValueEdit",  &value_history,       20},
   { "ExprEdit",   &expr_history,        20},
   { "Prompt",     &prompt_history,      10},
   { NULL, NULL, 0 }
};

/*  This spider on a web icon is supposed to suggest something that
**  catches bugs.
*/
#define ups_icon_width 48
#define ups_icon_height 48
static unsigned char ups_icon_bits[] = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x03,0xf0,0x07,0x70,0x00,
  0x80,0x07,0xfc,0x1f,0xd8,0x00,0xc0,0x0c,0xfe,0x3f,0x8c,0x01,
  0x60,0x18,0xff,0x7f,0x06,0x03,0x30,0xb8,0xff,0xff,0x03,0x06,
  0x18,0xf0,0xff,0xff,0x01,0x0e,0xc8,0xe0,0xff,0xff,0x01,0x08,
  0xc4,0xe1,0xff,0xff,0x83,0x01,0x60,0xe3,0xff,0xff,0xc3,0x03,
  0x60,0xf2,0xff,0xff,0x67,0x02,0x30,0xf6,0xff,0xff,0x37,0x06,
  0x30,0xfc,0xff,0xff,0x1f,0x04,0x18,0xf8,0xff,0xff,0x0f,0x0c,
  0x18,0xf8,0xff,0xff,0x07,0x08,0x0c,0xf8,0xff,0xff,0x87,0x18,
  0xc4,0xf8,0xff,0xff,0xc7,0x11,0xc0,0xf9,0xff,0xff,0xe7,0x13,
  0x60,0xfb,0xff,0xff,0xff,0x13,0x60,0xfe,0xff,0xff,0x1f,0x1e,
  0x30,0xfc,0xff,0xff,0x0f,0x66,0x30,0xf8,0xff,0xff,0x0f,0x8c,
  0x18,0xf0,0xff,0xff,0x0f,0x0c,0x18,0xe0,0xff,0xff,0xff,0x0c,
  0x04,0xe3,0xff,0xff,0x7b,0x0f,0x00,0xcf,0xff,0xff,0xdd,0x38,
  0x80,0xfd,0xff,0xff,0xcf,0xc0,0x80,0xf1,0xff,0xff,0x8b,0x01,
  0xc0,0x20,0xff,0xff,0xff,0x01,0xe0,0x10,0xfe,0x7f,0x08,0x07,
  0x60,0x08,0xfe,0x1f,0x08,0x1a,0x60,0x04,0xf2,0x07,0x08,0x66,
  0x30,0x04,0xf9,0x0f,0x08,0x84,0x10,0x82,0xfc,0x1f,0x7f,0x04,
  0x00,0x41,0xfc,0xff,0x88,0x03,0x00,0x41,0xfc,0x1f,0x08,0x0c,
  0x00,0x21,0xfc,0x1f,0x08,0x10,0x80,0x20,0xfc,0x1f,0x08,0x20,
  0x80,0x10,0xfc,0x2f,0x7f,0x40,0x40,0x10,0xf2,0xe7,0x88,0x81,
  0xc0,0x09,0xe2,0x63,0x08,0x02,0x40,0x1e,0x41,0x50,0x08,0x04,
  0x40,0xe8,0x21,0x88,0x3e,0x08,0x20,0x08,0x2f,0x04,0x49,0x10,
  0x20,0x84,0xf0,0x84,0x89,0x10,0x20,0x84,0x10,0x47,0x0a,0x21,
};
static bitmap_t ups_icon =
	wn_make_static_X11bm(ups_icon_width, ups_icon_height, ups_icon_bits);

static void
handle_panic(s)
const char *s;
{
	Done_panic = TRUE;
}

/*  This routine is called when we get a signal that causes a core dump.
 *  We want to avoid overwriting an existing core file in the current
 *  directory because that may be from the target and thus precious to
 *  the user.
 *
 *  We restrict ourselves to system calls and simple string routines here
 *  because we are in an inconsistent state (we have just had a SEGV or
 *  the like!).
 */
static void
catch_fatal_sig(signo)
int signo;
{
	struct stat stbuf;
	static const char upscore[] = "ups-core";
#ifdef SIG_UNBLOCK
	sigset_t mask;
#endif
#define ERR(m)	write(2, (m), sizeof(m) - 1)

	if (!Done_panic) {
		const char *sigstr;

		ERR("Fatal error: ");

		/*  Convert the signal number to text ourself as sprintf,
		 *  strf etc are too complex to call from here.
		 */

		switch (signo) {
#ifdef SIGSEGV
		case SIGSEGV:	sigstr = "segmentation fault";		break;
#endif
#ifdef SIGBUS
		case SIGBUS:	sigstr = "bus error";			break;
#endif
#ifdef SIGILL
		case SIGILL:	sigstr = "illegal instruction";		break;
#endif
#ifdef SIGFPE
		case SIGFPE:	sigstr = "floating point exception";	break;
#endif
#ifdef SIGQUIT
		case SIGQUIT:	sigstr = "SIGQUIT";			break;
#endif
		default:	sigstr = NULL;				break;
		}

		if (sigstr != NULL) {
			write(2, sigstr, strlen(sigstr));
		}
		else {
			char nbuf[20];
			char *np;
			int n;

			np = nbuf + sizeof(nbuf);
			*--np = '\0';

			n = (signo < 0) ? -signo : signo;
			for (; n != 0 && np > nbuf; n /= 10)
				*--np = n % 10 + '0';
			
			if (signo < 0)
				*--np = '-';

			ERR("Got signal ");
			write(2, np, sizeof(nbuf) - (np - nbuf));
		}

		ERR(".\n");
	}
	
	if (lstat("core", &stbuf) != 0 && errno == ENOENT) {
		ERR("Dumping core");
	}
	else if (chdir(upscore) == 0 ||
		 	(mkdir(upscore, 0755) == 0 && chdir(upscore) == 0)) {
		ERR("Dumping core in ups-core/core");
	}
	else if (chdir("/tmp") == 0) {
		ERR("Dumping core in /tmp/core");
	}
	else {
		ERR("Exiting without dumping core.\n");
		_exit(1);
	}
	ERR(" ... ");

#ifdef SIG_UNBLOCK
	sigemptyset(&mask);
	sigaddset(&mask, signo);
	sigprocmask(SIG_UNBLOCK, &mask, (sigset_t *)NULL);
#else
	sigsetmask(0);
#endif
	signal(signo, SIG_DFL);
	kill(getpid(), signo);

	ERR("Unexpectedly survived signal - exiting without dumping core.\n");
	_exit(1);
}

/*  Treat paths as a colon seperated list of paths, and add each to
 *  the source path list.
 *
 *  If paths starts with a colon, the default path is added first.
 */
static void
add_multiple_paths(ro_paths, defpath)
const char *ro_paths, *defpath;
{
	char *last, *paths, *pathcopy;

	pathcopy = paths = strsave(ro_paths);

	if (*paths == ':')
		srcpath_add_path(defpath);
	
	last = paths;
	for (;;) {
		while (*paths == ':' && *paths != '\0')
			paths++;
		
		if (*paths == '\0')
			break;
		
		last = paths;
		while (*paths != ':' && *paths != '\0')
			paths++;
		
		if (*paths == ':')
			*paths++ = '\0';
		
		srcpath_add_path(last);
	}
	
	free(pathcopy);
}

static int
is_directory(name)
const char *name;
{
	struct stat stbuf;

	return stat(name, &stbuf) == 0 && S_ISDIR(stbuf.st_mode);
}

const char *
hold_default_source_path(path, set)
const char *path;
bool set;
{
  static const char *source_path = NULL;

  if (set)
    source_path = path;
  return source_path;
}

static void
get_default_paths(textname, p_dsrc, p_dcore)
const char *textname;
const char **p_dsrc, **p_dcore;
{
	const char *cptr;

	if ((cptr = strrchr(textname, '/')) == NULL) {
		*p_dsrc = ".";
#ifdef OS_BSD44
		*p_dcore = strf("core.%s", textname);
#else
		*p_dcore = "core";
#endif
	}
	else if (cptr == textname) {
		*p_dsrc = "/";
		*p_dcore = "/core";
	}
	else {
		*p_dsrc = strf("%.*s", cptr - textname, textname);
#ifdef OS_BSD44
		*p_dcore = strf("%s/core.%s", *p_dsrc, cptr + 1);
#else
		*p_dcore = strf("%s/core", *p_dsrc);
#endif
	}
}

static void
usage(mesg)
const char *mesg;
{
	if (mesg != NULL)
		fprintf(stderr, "%s\n", mesg);

	errf_usage("obj [corefile|pid] [srcdir[:srcdir]...] [-a args]\n\
Say `ups -fullusage' for a complete list of options.");
}

static const char *
checkarg(argv)
char **argv;
{
	if (*argv == NULL) {
		usage(strf("The %s option requires an argument", argv[-1]));
		exit(1);
	}

	return *argv;
}

void 
save_tty()
{
	(void)tcgetattr(0, &save_stty0);
	(void)tcgetattr(1, &save_stty1);

	return;
}

void 
reset_tty()
{
	(void)tcsetattr(0, TCSANOW, &save_stty0);
	(void)tcsetattr(1, TCSANOW, &save_stty1);

	return;
}

int
main(argc, argv)
int argc;
char **argv;
{
	char res_buff[50];
	static const int siglist[] = {
#ifdef SIGQUIT
		SIGQUIT,
#endif
#ifdef SIGILL
		SIGILL,
#endif
#ifdef SIGIOT
		SIGIOT,
#endif
#ifdef SIGEMT
		SIGEMT,
#endif
#ifdef SIGFPE
		SIGFPE,
#endif
#ifdef SIGSEGV
		SIGSEGV,
#endif
#ifdef SIGSYS
		SIGSYS,
#endif
#ifdef SIGBUS
		SIGBUS,
#endif
	};
	static const char *(*vfuncs[])PROTO((void)) = {
		arg_version, 
		obj_version,
		edit_version,
		Mversion,
		mtrprog_version,
		ukcprog_version,
		wn_version,
		NULL
	};
	static const char default_state_dir[] = "ups-state";
	static char marker[] = "MARKER";
	static bool started = FALSE;
	const char *corename, *textname;
	const char *default_source_path, *default_corename, *state_dir;
	ebuf_t *usage_eb;
	const char *args;
	char *save_arg;
	ccstate_t *cs;
	int i;
	bool got_source_path, replay, want_window, user_gave_core, done_cc_arg;
	bool seen_startup_message_flag, want_startup_message;
	bool want_load_config, want_save_config;
	bool want_auto_start, use_full_path, want_save_sigs;
	target_t *xp;

	/*  Object if main is called recursively.  This can happen on a
	 *  VAX with a call via a NULL function pointer.
	 */
	if (started)
		panic("main called recursively");
	started = TRUE;

	arg_save_open_fds();
	
	save_tty();

	preinitialise_target_drivers();

	usage_eb = ebuf_create(TRUE);
	if (initialise_target_drivers(usage_eb, argv) != 0)
		exit(1);
	for (argc = 0; argv[argc] != NULL; ++argc)
		;

	errf_set_progname(*argv);

	if (argc == 2 && strcmp(argv[1], "-v") == 0) {
		puts(ups__sccsdata[0]);
		exit(0);
	}
	
	if (argc == 2 && strcmp(argv[1], "-V") == 0) {
		puts(ups__sccsdata[0]);
		fputc('\n', stdout);
		
		puts("Library versions:");
		for (i = 0; vfuncs[i] != NULL; ++i)
			printf("\t%s\n", (*vfuncs[i])());
		fputc('\n', stdout);

		puts("Target drivers: ");
		show_target_driver_info(TRUE);
		
		exit(0);
	}

	if (argc == 2 && strcmp(argv[1], "-fullusage") == 0) {
		const char *driver_flags;

		ebuf_add(usage_eb, "", 1);
		driver_flags = ebuf_get(usage_eb, (int *)NULL);
		
		printf("\
Usage: ups -V\n\
   or: ups -fullusage\n\
   or: ups obj [-coredir dir] [corefile|pid] [srcdir[:srcdir]...] [-a args]\n\
	       [-noconfig] [-noloadconfig] [-nosaveconfig]  [-nosavesigs]\n\
	       [-statedir dir] [-nodemangle] [-[no]install]\n\
	       [-[no]split[:num]] [-[no]splitoutwin[:num]]\n\
	       [-[no]mousehole] [-[no]logging] [-[no]findtypes]\n \
	       [-[no]raise_on_break] [-[no]lower_on_run] [-[no]iconify_on_run]\n \
	       [-replay path] [-record path]\n\
               [-nowindow] [-replayidstr str]\n\
	       [-replaymode [lnum:]{%s}]\n\
               [-go] [-nostartupmesg] [-use_srcpath_only] [-fullpath]\n\
               [-dbflags flags]\n\
               %s\n",
		       td_get_replay_mode_list(), driver_flags);

		exit(0);
	}

	ebuf_free(usage_eb);

	set_message_wn(-1);
	errf_set_ofunc(display_message);
	set_window_error_handler();

	for (i = 0; i < sizeof(siglist) / sizeof(siglist[0]); i++)
		signal(siglist[i], catch_fatal_sig);
	install_panic_handler(handle_panic);
	ci_catch_arithmetic_signals(catch_fatal_sig);
	
	/*  We have to set this here because we want to act on the
	 *  -nowindow flag on the first pass through the arguments.
	 *  This is because we must not call wn_munge_args() if we
	 *  have the -nowindow flag (it opens the display connection).
	 *  This means we end up checking the -nowindow flag twice, but it
	 *  doesn't break anything.
	 */
	want_window = TRUE;

	/*  Hide anything following a "-a" argument from wn_munge_args().
	 */
	save_arg = 0; /* to satisfy gcc */
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-a") == 0 && argv[i + 1] != NULL) {
			save_arg = argv[i + 1];
			argv[i + 1] = marker;
			break;
		}
		else if (strcmp(argv[i], "-nowindow") == 0) {
			want_window = FALSE;
		}
	}

	if (want_window) {
		wn_set_classname("Ups");
		wn_munge_args(argc, (const char **)argv);
	}

	++argv;
	for (i = 0; argv[i] != NULL; i++)
		if (argv[i] == marker)
			argv[i] = save_arg;

	
	cs = cc_create_ccstate("", (unsigned)(CI_CP_ONEFILE |
					      CI_CP_WANT_SYMINFO));
	done_cc_arg = FALSE;
	for (; *argv != NULL && **argv == '-'; ++argv) {
		if (!cc_handle_arg(cs, &argv))
			usage("Flags must come after the object file name");
		done_cc_arg = TRUE;
	}

	set_ccstate(cs);

	if (*argv == NULL)
		usage((const char *)NULL);

	textname = *argv++;

	if (done_cc_arg && (strlen(textname) < (size_t)3 ||
			    strcmp(textname + strlen(textname) - 2, ".c") != 0))
		usage("Can only give cc type flags with a .c file");
	    
	
	get_default_paths(textname, &default_source_path, &default_corename);
	hold_default_source_path(default_source_path, TRUE); /* store */
	got_source_path = FALSE;
/*	want_startup_message = TRUE;*/
/* RGA SLB only */
	want_startup_message = FALSE;
	want_load_config = TRUE;
	state_dir = default_state_dir;
	want_save_config = TRUE;
	want_save_sigs = TRUE;
	seen_startup_message_flag = FALSE;
	args = NULL;
	corename = NULL;
	replay = FALSE;
	want_auto_start = FALSE;
	use_full_path = FALSE;

	for (; *argv != NULL; argv++) {
		if (strcmp(*argv, "-use_srcpath_only") == 0) {
			srcpath_ignore_path_hints();
		}
		else if (strcmp(*argv, "-dbflags") == 0) {
			Debug_flags = strtol(checkarg(++argv), (char **)NULL,0);
		}
		else if (strcmp(*argv, "-nooutwin") == 0) {
			wn_set_default("WantOutwin", "no");
		}
		else if (strcmp(*argv, "-outwin") == 0) {
			wn_set_default("WantOutwin", "yes");
		}
		else if (strcmp(*argv, "-nowindow") == 0) {
			want_window = FALSE;
			want_startup_message = FALSE;
		}
		else if (strcmp(*argv, "-noconfig") == 0) {
			want_load_config = want_save_config = FALSE;
		}
		else if (strcmp(*argv, "-noloadconfig") == 0) {
			want_load_config = FALSE;
		}
		else if (strcmp(*argv, "-nosaveconfig") == 0) {
			want_save_config = FALSE;
		}
		else if (strcmp(*argv, "-nosplit") == 0) {
			wn_set_default("WantSplitWindows", "no");
		}
		else if (strcmp(*argv, "-split") == 0) {
			wn_set_default("WantSplitWindows", "yes");
		}
		else if (strncmp(*argv, "-split:", 7) == 0) {
			strcpy(res_buff, "yes:");
			strncat(res_buff, *argv+7,10);
			wn_set_default("WantSplitWindows", res_buff);
		}
		else if (strcmp(*argv, "-nosplitoutwin") == 0) {
			wn_set_default("WantSplitOutputWindow", "no");
		}
		else if (strcmp(*argv, "-splitoutwin") == 0) {
			wn_set_default("WantSplitOutputWindow", "yes");
		}
		else if (strncmp(*argv, "-splitoutwin:", 13) == 0) {
			strcpy(res_buff, "yes:");
			strncat(res_buff, *argv+13,10);
			wn_set_default("WantSplitOutputWindow", res_buff);
		}
		else if (strcmp(*argv, "-nomousehole") == 0) {
			wn_set_default("WantMousehole", "no");
		}
		else if (strcmp(*argv, "-mousehole") == 0) {
			wn_set_default("WantMousehole", "yes");
		}
		else if (strcmp(*argv, "-nologging") == 0) {
			wn_set_default("WantMessageLogging", "no");
		}
		else if (strcmp(*argv, "-logging") == 0) {
			wn_set_default("WantMessageLogging", "yes");
		}
		else if (strcmp(*argv, "-noraise_on_break") == 0) {
			wn_set_default("WantRaiseOnBreak", "no");
			wn_set_default("WantLowerOnRun", "no");
			wn_set_default("WantIconifyOnRun", "no");
		}
		else if (strcmp(*argv, "-raise_on_break") == 0) {
			wn_set_default("WantRaiseOnBreak", "yes");
			wn_set_default("WantLowerOnRun", "no");
			wn_set_default("WantIconifyOnRun", "no");
		}
		else if (strcmp(*argv, "-lower_on_run") == 0) {
			wn_set_default("WantLowerOnRun", "yes");
			wn_set_default("WantIconifyOnRun", "no");
		}
		else if (strcmp(*argv, "-nolower_on_run") == 0) {
			wn_set_default("WantLowerOnRun", "no");
			wn_set_default("WantIconifyOnRun", "no");
		}
		else if (strcmp(*argv, "-iconify_on_run") == 0) {
			wn_set_default("WantIconifyOnRun", "yes");
		}
		else if (strcmp(*argv, "-noiconify_on_run") == 0) {
			wn_set_default("WantIconifyOnRun", "no");
		}
		else if (strcmp(*argv, "-nosavesigs") == 0) {
			want_save_sigs = FALSE;
		}
		else if (strcmp(*argv, "-go") == 0) {
			want_auto_start = TRUE;
		}
		else if (strcmp(*argv, "-fullpath") == 0) {
			use_full_path = TRUE;
		}
		else if (strcmp(*argv, "-nostartupmesg") == 0) {
			want_startup_message = FALSE;
			seen_startup_message_flag = TRUE;
		}
		else if (strcmp(*argv, "-startupmesg") == 0) {
			want_startup_message = TRUE;
			seen_startup_message_flag = TRUE;
		}
		else if (strcmp(*argv, "-replay") == 0) {
			if (td_replay_from(checkarg(++argv)) != 0)
				exit(1);
			replay = TRUE;
		}
		else if (strcmp(*argv, "-replaymode") == 0) {
			if (td_set_replay_mode(checkarg(++argv)) != 0)
				exit(1);
		}
		else if (strcmp(*argv, "-replayidstr") == 0) {
			td_set_replay_idstr(checkarg(++argv));
		}
		else if (strcmp(*argv, "-record") == 0) {
			if (td_record_to(checkarg(++argv)) != 0)
				exit(1);
		}
		else if (strcmp(*argv, "-coredir") == 0) {
			default_corename = strf("%s/core", checkarg(++argv));
		}
		else if (strcmp(*argv, "-nodemangle") == 0) {
			demangling_enabled(0, 1); /* reset */
		}
		else if (strcmp(*argv, "-nofindtypes") == 0) {
			set_find_types_by_name(0);
		}
		else if (strcmp(*argv, "-findtypes") == 0) {
			set_find_types_by_name(1);
		}
		else if (strcmp(*argv, "-statedir") == 0) {
			state_dir = checkarg(++argv);
		}
		else if (strcmp(*argv, "-a") == 0) {
			if (args != NULL)
				usage("Multiple -a options given");
			args = checkarg(++argv);
		}
		else if (**argv == '-') {
			usage(strf("Unknown flag `%s'", *argv));
		}
		else if (strchr(*argv, ':') != NULL || is_directory(*argv)) {
			if (got_source_path)
				usage("Multiple source paths given");
			
			add_multiple_paths(*argv, default_source_path);
			got_source_path = TRUE;
		}
		else {
			if (corename != NULL) {
				usage(strf(
				  "Multiple core files specified (%s and %s)",
							     corename, *argv));
			}
			corename = *argv;
		}
	}

	if ((Debug_flags & DBFLAG_EDITCHECKS) != 0)
		srcwin_set_want_checks_flag();

	/*  Before we do any potentially time consuming stuff, check
	 *  that we are going to be able to fire up a window.
	 */
	if (want_window) {
		if (setup_display() != 0)
			exit(1);
		
		if (!seen_startup_message_flag) {
			const char *wantmsgstr;

			wantmsgstr = wn_get_default("WantStartupMessage");
			want_startup_message = wantmsgstr != NULL &&
					       strcmp(wantmsgstr, "no") != 0;
		}
	}
	else {
		if (!replay) {
			errf("You must give the -replay flag with -nowindow");
			exit(1);
		}
		td_set_no_window_flag();
	}

	if (want_startup_message) {
		puts(ups__sccsdata[0]);
		fputs("Copyright 1994-2002 Mark Russell, ", stdout);
		puts("University of Kent at Canterbury.");

		show_target_driver_info(FALSE);
		fputc('\n', stdout);
	}

	/*  Only try to save state if the upsinfo directory exists or
	 *  the user has explicitly named a state directory.
	 */
	if ((want_save_config || want_save_sigs) &&
	    state_dir == default_state_dir && !statedir_exists(state_dir)) {
		want_save_config = want_save_sigs = FALSE;
	}
	
	if (!got_source_path)
		add_multiple_paths(":", default_source_path);

	user_gave_core = corename != NULL;
	if (corename == NULL)
		corename = default_corename;

	if (td_init_from())
	{
		bool eof;
		int res;

		res = td_load_loop(&eof);

		if (res != 0)
			fprintf(stderr, "Error in init file\n");
		td_restore_replay_fp();
	}

	/* Initialize the edit histories */
	edit_initialize_history(state_dir, "editHistory",".upsEditHistory", history_items);

	if (make_target(textname, corename, user_gave_core, &xp,
			&args, FALSE, FALSE) == -1)
		exit(1);
	set_current_target(xp);

	if (ups(xp,textname, args, replay, want_window, want_load_config,
		want_save_config, want_save_sigs, want_auto_start,
		use_full_path, state_dir, is_number(corename)) != 0)
		exit(1);

	edit_terminate_history();
	reset_tty();
	exit(0);
	return 0;	/* to satisfy gcc */
}

static bool
statedir_exists(state_dir)
const char *state_dir;
{
	static const char what[] = "saved state directory";
	static const char consequence[] = "no state will be loaded or saved";
	struct stat stbuf;
	
	if (stat(state_dir, &stbuf) != 0) {
		if (errno != ENOENT) {
			errf("Can't stat %s `%s' (%s),\nso %s",
			     what, state_dir, get_errno_str(), consequence);
		}
		return FALSE;
	}

	if (!S_ISDIR(stbuf.st_mode)) {
		errf("%c%s `%s' should be a directory but is a %s,\nso %s",
		     toupper(*what), what + 1, state_dir,
		     filetype_to_string((int)stbuf.st_mode), consequence);
		return FALSE;
	}

	return TRUE;
}

static int
setup_display()
{
	const char *errmesg;
	const char *tabwidth_str, *menufont_name, *srcfont_name;
	font_t *sysfont, *menufont, *srcfont, *editfont;
	int tabwidth;
	const char *editfont_name;
	
	if ((errmesg = wn_open_display()) != NULL) {
		errf("%s", errmesg);
		return -1;
	}

	if ((tabwidth_str = wn_get_default("TabWidth")) == NULL) {
		tabwidth = DEFAULT_TABWIDTH;
	}
	else {
		tabwidth = atoi(tabwidth_str);
		if (tabwidth < 1) {
			errf("Illegal TabWidth resource value `%s' ignored",
			     tabwidth_str);
			tabwidth = DEFAULT_TABWIDTH;
		}
	}
	srcwin_set_tabwidth(tabwidth);

	sysfont = wn_get_sysfont();
		
	if ((menufont_name = wn_get_default("MenuFont")) == NULL) {
		menufont = sysfont;
	}
	else if ((menufont = wn_open_font(menufont_name)) == NULL) {
		errf("Can't open font `%s' %s - using the system font",
		     menufont_name, "specified by the MenuFont resource");
		menufont = sysfont;
	}
	
	Msetstdfont(menufont);

	if ((srcfont_name = wn_get_default("SrcFont")) == NULL) {
		srcfont = sysfont;
	}
	else if ((srcfont = wn_open_font(srcfont_name)) == NULL) {
		errf("Can't open srcfont %s - using default", srcfont_name);
		srcfont = sysfont;
		srcfont_name = NULL;
	}
	
	if (wn_get_wm_type() == WN_X11)
		editfont_name = make_editfont_name(srcfont_name);
	else
		editfont_name = "screen.b.14";

	editfont = NULL;
	
	if (editfont_name != NULL &&
	    (editfont = wn_open_font(editfont_name)) == NULL) {
		static const char fallback[] =
				"-*-fixed-bold-r-normal--15-*-*-*-*-*-*-*";
		
		errf("Can't open editfont %s - trying %s", editfont_name,
		     fallback);
		editfont_name = fallback;
	}

	if (editfont == NULL) {
		if ((editfont = wn_open_font(editfont_name)) == NULL) {
			errf("Can't open %s %s - using system font",
			     "default edit font", editfont_name);
			editfont = sysfont;
		}
	}
	
	srcwin_set_fonts(srcfont, editfont, (font_t *)NULL);
	display_overlay_set_font(sysfont);

	set_special_edit_chars(edit_get_default_keymap());

	wn_set_cur_posn_func(get_cur_posn);
	wn_set_overlay_insert_string_func(display_area_overlay_insert_string);
	return 0;
}

/*  Install keymap entries for character and line erase.
 *  If the user started ups from a terminal, use whatever keys they
 *  are using.  Otherwise just use common defaults.
 */
static void
set_special_edit_chars(keymap)
Edit_keymap *keymap;
{
	struct termios term;
	
	if (tcgetattr(0, &term) != 0) {
		if (errno != ENOTTY && errno != ENXIO && errno != EINVAL)
			errf("Warning: Can't get %s for stdin: %s",
			     "terminal attributes", get_errno_str());

		/*  Lots of people use ^H for delete and ^U for line erase.
		 *  We don't currently use them, so add the mappings.
		 *
		 *  We should probably dump this when we implement some way
		 *  for the user to add their own key mappings.
		 */
		add_keymap_entry(keymap, EDIT_CONTROL, 'h',
				 edit_f_delete_backward_char);
		add_keymap_entry(keymap, EDIT_CONTROL, 'u',
				 edit_f_backward_kill_line);
	}
	else {
		add_keymap_entry(keymap, 0, term.c_cc[VERASE],
				 edit_f_delete_backward_char);
		add_keymap_entry(keymap, 0, term.c_cc[VKILL],
				 edit_f_backward_kill_line);
	}
}

static void
add_keymap_entry(keymap, modifiers, ch, func)
Edit_keymap *keymap;
unsigned modifiers;
int ch;
Edit_function func;
{
	if (ch < 0x20) {
		ch = tolower(ch | '@');
		modifiers |= EDIT_CONTROL;
	}
	else if (isupper(ch)) {
		ch = tolower(ch);
		modifiers |= EDIT_SHIFT;
	}
	
	edit_add_keymap_entry(keymap, modifiers, ch, func);

	if ((modifiers & EDIT_CONTROL) != 0)
		edit_add_keymap_entry(keymap, modifiers | EDIT_SHIFT, ch, func);
	
	if (func == edit_f_delete_backward_char) {
		edit_add_keymap_entry(keymap, modifiers | EDIT_ALT, ch,
				      edit_f_backward_kill_word);

		if ((modifiers & EDIT_CONTROL) != 0) {
			edit_add_keymap_entry(keymap,
					      modifiers | EDIT_ALT | EDIT_SHIFT,
					      ch, edit_f_backward_kill_word);
		}
	}
}

static const char *
make_editfont_name(srcfont_name)
const char *srcfont_name;
{
	char **vec;
	int ncomps;
	const char *fontname;

	if ((fontname = wn_get_default("EditFont")) != NULL)
		return fontname;
	
	if (srcfont_name == NULL)
		srcfont_name = wn_get_default("Font");

	if (srcfont_name == NULL)
		return NULL;

	vec = ssplit(srcfont_name, "\0-");

	for (ncomps = 0; vec[ncomps] != NULL; ++ncomps)
		;

	if (ncomps >= 4 && strcmp(vec[3], "medium") == 0) {
		ebuf_t *eb;
		int i;

		eb = ebuf_create(TRUE);

		for (i = 0; i < ncomps; ++i) {
			if (i != 0)
				ebuf_add(eb, "-", 1);
			ebuf_addstr(eb, (i == 3) ? "bold" : vec[i]);
		}
		ebuf_add(eb, "", 1);

		fontname = strsave(ebuf_get(eb, (int *)NULL));

		ebuf_free(eb);
	}
	else {
		fontname = NULL;
	}

	free((char *)vec);

	return fontname;
}

static void
null_ofunc(s)
const char *s;
{
}

static int
ups(xp, textname, args, replay, want_window, want_load_config,
    want_save_config, want_save_sigs, want_auto_start,
    use_full_path, state_dir, attach)
target_t *xp;
const char* textname;
const char *args;
bool replay, want_window;
bool want_load_config, want_save_config, want_save_sigs,
  want_auto_start, use_full_path;
const char *state_dir;
bool attach;
{
	Region *root_region, *root_region1;
	window_t wn, wn1, wn2;
	func_t *f, *f1;
	const char *basepath, *state_path, *user_path;
	bool file_want_auto_start;
	int res, file_access = -1;
	symtab_t *st;
	bool want_split = FALSE, want_outwin;
	int screen2 = -1;
	const char* defstr;
		
 	td_set_no_window_flag();
	if ((f = xp_get_mainfunc(xp)) == NULL) {
		errf_ofunc_t oldf;
	
		oldf = errf_set_ofunc(null_ofunc);
		res = find_func_by_name("MAIN", &f, &f1, TRUE);
		errf_set_ofunc(oldf);

		if (res != 0 || !IS_FORTRAN(f->fu_language)) {
			if (find_func_by_name("main", &f, &f1, TRUE) != 0)
				return -1;
		}

		xp_set_mainfunc(xp, f);
	}

 	td_set_window_flag();
	root_region = root_region1 = NULL;	/* to satisfy gcc */
	wn = wn1 = wn2 = -1;		/* to satisfy gcc */

	if (want_window) {
		wn_npixels_hint(1);
		if ((wn = wn_open_stdwin()) == -1) {
			errf("Can't open window");
			return -1;
		}

		/* To split the UPS windows set
			WantSplitWindows: yes
		   To split across screens e.g. screen 0 & 2, set
			WantSplitScreen: 2
		   or
			WantSplitWindows: yes:2
		*/
		defstr = wn_get_default("WantSplitWindows");
		want_split = ((defstr != NULL) && (strncmp(defstr, "yes", 3)) == 0);

		if (want_split && (defstr[3] == ':'))
			    defstr = &defstr[4];
		else if (defstr==NULL)
			    defstr = wn_get_default("WantSplitScreen");
		if ((defstr != NULL) &&
			(atoi(defstr) > 0 || *defstr == '0'))
		{
			want_split = TRUE;
			screen2 = atoi(defstr);
		}
#if WANT_MULTI_SCREEN
#else
		if (want_split && screen2 != -1)
		{
			errf("Not built for multiple screens, will put both windows on default screen");
			screen2 = -1;
		}
#endif
		if (   (want_split && (wn1=wn_open_scr_stdwin("Src",screen2)) == -1))
		{
			errf("Can't open second window");
			return -1;
		}
	}

	do_formats(want_window);

	/*  We don't want to get stopped by controlling tty related
	 *  signals sent to the process group of the child process
	 *  (which includes us).  We can't switch process groups
	 *  because we want to respond to SIGTSTP (i.e. ^Z should
	 *  stop ups).  We don't want to background ourself because
	 *  then the child process would have no controlling tty.
	 */
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);
	signal(SIGINT, SIG_IGN);

	if (want_window) {
		char title[20];
		strcpy(title, "%a ");
		textname = base_name(textname);
		strncpy(title+3,textname,sizeof(title)-4);
		title[sizeof(title)-1] = 0;
		wn_set_window_hints(wn, title,title, &ups_icon);

		/* Determine it split windows are requested */
			defstr = wn_get_default("WantOutwin");
		want_outwin = (defstr != NULL && strcmp(defstr, "yes") == 0);
		
		if (want_split)
		{
		  wn_set_window_hints(wn1, "%a source","%a source", &ups_icon);
		  root_region = divide_window_into_regions1(wn, want_outwin);
		  root_region1 = divide_window_into_regions2(wn1, want_outwin);
		}
		else
		  root_region = divide_window_into_regions(wn, want_outwin);

	}

	initialise_display_area(xp, args, use_full_path);

	if (want_window)
		update_target_menu_state(xp_get_state(xp), xp_is_attached(xp));

	td_set_obj_updating(OBJ_UPDATING_ON);

	if (xp_get_state(xp) == TS_NOTR && f->fu_fil != NULL &&
							FU_LNOS(f) != NULL)
		show_source(f->fu_fil, FU_LNOS(f)->ln_num);
	
	td_set_displayed_source((fil_t *)NULL, 0, (const char *)NULL);

	st = xp_main_symtab(xp);
	/* If single file compiled into a.out... */
	if ((strcmp(base_name(xp->xp_textpath), "a.out") == 0)
			&& (st->st_sfiles != NULL)
			&& (st->st_sfiles->fi_next == NULL))
		basepath = st->st_sfiles->fi_name;
	else
		basepath = xp->xp_textpath;

	make_config_paths(state_dir, basepath, &state_path, &user_path);

	if (td_init_from())
	{
		bool eof;

		res = td_event_loop(&eof);

		if (res != 0)
			fprintf(stderr, "Error in init file\n");
		td_restore_replay_fp();
	}

	if (want_load_config) {
		load_config(state_path, user_path, &file_want_auto_start);
	}
	else {
		file_want_auto_start = FALSE;
	}

	/* RGA add this */
	if (attach)
	  refresh_target_display(xp, SR_SIG, TRUE);

	if ((want_auto_start || file_want_auto_start) &&
	    xp_get_state(xp) != TS_CORE) {
		/*  The indicate_target_{running,stopped}() functions need
		 *  a cursor set to work properly.
		 */
		if (want_window)
			set_bm_cursor(wn, CU_DEAD);
			
		do_menu_target_command(MR_TGT_START);
	}
	
	if (replay) {
		bool eof;

		res = td_event_loop(&eof);

		if (want_window && (res != 0 || eof)) {
			fprintf(stderr, "%s in replay file - %s",
					(res == 0) ? "EOF" : "Error",
					"switching to normal events\n");
			re_event_loop();
		}
	}
	else {
		re_event_loop();
		res = 0;
	}

	xp = get_current_target(); /* RGA: attach may have changed xp */
	if (res == 0 && want_save_config) {
		if (target_process_exists(xp))
			save_var_display_state(TRUE, FALSE);
		
		res = save_state(state_dir, state_path, want_save_sigs);
	}

	/*  Make sure the target is dead or detached before exiting.
	 */
	if (target_process_exists(xp))
		kill_or_detach_from_target(xp);

	if ((file_access = access(get_temp_state_filename(), O_RDONLY)) != -1)
	  if (unlink(get_temp_state_filename()) != 0)
	    errf("Can't unlink `%s'", get_temp_state_filename());

	return res;
}


int
want_split_output( p_screen )
int* p_screen;
{

    /* To split the UPS windows set
    	    WantSplitOutputWindow: yes
       To split across screens e.g. screen 0 & 2, set
            WantSplitOutputWindow: 2
    */
    const char *defstr = wn_get_default("WantSplitOutputWindow");
    bool want_split_outwin =  ((defstr != NULL) && (strncmp(defstr, "yes", 3)) == 0);
    int out_screen = -1;

    if ( want_split_outwin)
    {
	if (defstr[3] == ':')
	    defstr = &defstr[4];
	else
	    defstr = wn_get_default("WantSplitScreen");
	if ((defstr != NULL) &&
	    (atoi(defstr) > 0 || *defstr == '0'))
	{
	    out_screen = atoi(defstr);
	}
#if WANT_MULTI_SCREEN
#else
	if (p_screen && out_screen != -1)
	{
	    /* This can cause recursion if message logging is on.
	    ** Better to silently correct the error.
	    **
	    ** errf("Not built for multiple screens, will put both windows on default screen");
	    */
	    out_screen = -1;
	}
#endif
    }
    if ( p_screen)
	*p_screen = out_screen;
    return want_split_outwin;
}

int
get_split_out_win()
{
    int screen;
    int wn;
    if ( !want_split_output(&screen))
	 return -1;
    wn =  wn_open_scr_stdwin("Output",screen);
    wn_set_window_hints(wn, "%a output","%a output", &ups_icon);
    return wn;
}

int
rational_products_running()
{
    static int rational_products = -1;

    if (rational_products == -1)
    {
	/* see if Purify/Quantify/Purecov are used */
	func_t *f, *f1;
	int res;
	errf_ofunc_t oldf, oldf1;

	res = find_func_by_name("main", &f, &f1, TRUE);
	oldf = errf_set_ofunc(null_ofunc);
	oldf1 = errf_set_ofunc1(null_ofunc);
	res = find_func_by_name("purify_is_running", &f, &f1, TRUE);
	if (res != 0)
	    res = find_func_by_name("quantify_is_running", &f, &f1, TRUE);
	if (res != 0)
	    res = find_func_by_name("purecov_is_running", &f, &f1, TRUE);
	rational_products = res == 0;
	errf_set_ofunc(oldf);
	errf_set_ofunc1(oldf1);
    }
    return rational_products;
}

/* Called when attaching/running a different target */
void
reset_pathlist(textpath)
char *textpath;
{
  const char *default_source_path, *default_corename;

  srcpath_free_pathlist();
  get_default_paths(textpath, &default_source_path, &default_corename);
  hold_default_source_path(default_source_path, TRUE); /* store */
  add_multiple_paths(":", default_source_path);
}

/* Called when attaching/running a different target */
void
update_icon_name(textname)
char *textname;
{
  int wn;
  char title[20];

  strcpy(title, "%a ");
  textname = (char *)base_name(textname);
  strncpy(title+3,textname,sizeof(title)-4);
  title[sizeof(title)-1] = 0;
  wn = wn_get_root_window(get_message_wn());
  wn_set_window_hints(wn, title,title, &ups_icon);
}

