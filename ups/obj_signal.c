/* obj_signal.c - target signal handling routines */

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


/* @(#)obj_signal.c	1.27 25 May 1995 (UKC) */
char ups_obj_signal_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <local/wn.h>
#include <local/obj/obj.h>
#include <local/ukcprog.h>
#include "objtypes.h"

#include "ups.h"
#include "ui.h"
#include "obj_signal.h"
#include "menudata.h"
#include "tdr.h"
#include "symtab.h"
#include "target.h"
#include "state.h"

typedef struct sigst {
	int si_number;
	const char *si_name;
	int si_attrs;
} sig_info_t;

static void add_signal_object PROTO((objid_t par, sig_info_t *si));
static char *sig_attr_string PROTO((sig_info_t *si));
static int sigcmp PROTO((objid_t obj1, objid_t obj2));
static int get_sig_attrs PROTO((int sig));

#define SGH_CONT		(1<<0)	/* continue automatically after sig */
#define SGH_REDRAW		(1<<1)	/* refresh display before continuing */
#define SGH_ACCEPT		(1<<2)	/* target accepts signal */
#define SGH_DFL_STOPS		(1<<3)	/* signal stops target by default */
#define SGH_DFL_IGNORE		(1<<4)	/* signal is ignored by default */

static sig_info_t Sigtab[] = {
	{ SIGINT,	"SIGINT",	0				      },
	{ SIGHUP,	"SIGHUP",	SGH_CONT | SGH_ACCEPT		      },
	{ SIGQUIT,	"SIGQUIT",	0				      },
	{ SIGILL,	"SIGILL",	0				      },
	{ SIGTRAP,	"SIGTRAP",	0				      },
#if defined(SIGIOT) && (!defined(SIGABRT) || SIGIOT != SIGABRT)
	{ SIGIOT,	"SIGIOT",	0				      },
#endif
#if defined (SIGABRT)
	{ SIGABRT,	"SIGABRT",	0				      },
#endif
#if defined (SIGEMT)
	{ SIGEMT,	"SIGEMT",	SGH_CONT | SGH_ACCEPT		      },
#endif
	{ SIGFPE,	"SIGFPE",	SGH_CONT | SGH_ACCEPT		      },
	{ SIGKILL,	"SIGKILL",	SGH_CONT | SGH_ACCEPT		      },
	{ SIGBUS,	"SIGBUS",	0				      },
	{ SIGSEGV,	"SIGSEGV",	0				      },
#if defined (SIGSYS)
	{ SIGSYS,	"SIGSYS",	0				      },
#endif
	{ SIGPIPE,	"SIGPIPE",	0				      },
	{ SIGALRM,	"SIGALRM",	SGH_CONT | SGH_ACCEPT		      },
	{ SIGTERM,	"SIGTERM",	0				      },
	{ SIGUSR1,	"SIGUSR1",	0				      },
	{ SIGUSR2,	"SIGUSR2",	0				      },
	{ SIGCHLD,	"SIGCHLD",	SGH_DFL_IGNORE | SGH_CONT | SGH_ACCEPT},
#if defined (SIGPWR)
	{ SIGPWR,	"SIGPWR",	SGH_DFL_IGNORE | SGH_CONT | SGH_ACCEPT},
#endif
#if defined (SIGWINCH)
	{ SIGWINCH,	"SIGWINCH",	SGH_DFL_IGNORE | SGH_CONT | SGH_ACCEPT},
#endif
	{ SIGURG,	"SIGURG",	SGH_DFL_IGNORE | SGH_CONT | SGH_ACCEPT},
#if defined(SIGPOLL) && defined(SIGIO) && (SIGPOLL == SIGIO)
	/* SIGPOLL same as SIGIO, prefer SIGPOLL */
	{ SIGPOLL,	"SIGPOLL",	SGH_CONT | SGH_ACCEPT		      },
#else
#if defined(SIGPOLL)
	{ SIGPOLL,	"SIGPOLL",	SGH_CONT | SGH_ACCEPT		      },
#endif
#if defined(SIGIO)
	{ SIGIO,	"SIGIO",	SGH_DFL_IGNORE | SGH_CONT | SGH_ACCEPT},
#endif
#endif
	{ SIGSTOP,	"SIGSTOP",	SGH_DFL_STOPS			      },
	{ SIGTSTP,	"SIGTSTP",	SGH_DFL_STOPS | SGH_CONT | SGH_ACCEPT },
	{ SIGCONT,	"SIGCONT",	SGH_DFL_IGNORE | SGH_CONT | SGH_ACCEPT},
	{ SIGTTIN,	"SIGTTIN",	SGH_DFL_STOPS			      },
	{ SIGTTOU,	"SIGTTOU",	SGH_DFL_STOPS			      },
#ifdef SIGVTALRM
	{ SIGVTALRM,	"SIGVTALRM",	SGH_CONT | SGH_ACCEPT		      },
#endif
	{ SIGPROF,	"SIGPROF",	0				      },
#ifdef SIGXCPU
	{ SIGXCPU,	"SIGXCPU",	0				      },
#endif
#ifdef SIGXFSZ
	{ SIGXFSZ,	"SIGXFSZ",	0				      },
#endif
#ifdef SIGWAITING
	{ SIGWAITING,	"SIGWAITING",	0				      },
#endif
#ifdef SIGLWP
	{ SIGLWP,	"SIGLWP",	0				      },
#endif
#ifdef SIGFREEZE
	{ SIGFREEZE,	"SIGFREEZE",	0				      },
#endif
#ifdef SIGTHAW
	{ SIGTHAW,	"SIGTHAW",	0				      },
#endif
#if defined (SIGUNUSED)
	{ SIGUNUSED,	"SIGUNUSED",	0				      },
#endif
#if defined (SIGCANCEL)
	{ SIGCANCEL,	"SIGCANCEL",	0				      },
#endif
#if defined (SIGLOST)
	{ SIGLOST,	"SIGLOST",	0				      },
#endif
#if defined(__SIGRTMIN) && !defined(_SIGRTMIN)
#define _SIGRTMIN __SIGRTMIN
#endif
#if defined(__SIGRTMAX) && !defined(_SIGRTMAX)
#define _SIGRTMAX __SIGRTMAX
#endif
#if defined (_SIGRTMIN) && defined (_SIGRTMAX)
#if (_SIGRTMIN)  <= (_SIGRTMAX)
	{ _SIGRTMIN,	"SIGRT0",	0				      },
#endif
#if (_SIGRTMIN)+1  <= (_SIGRTMAX)
	{ _SIGRTMIN+1,	"SIGRT1",	0				      },
#endif
#if (_SIGRTMIN)+2  <= (_SIGRTMAX)
	{ _SIGRTMIN+2,	"SIGRT2",	0				      },
#endif
#if (_SIGRTMIN)+3  <= (_SIGRTMAX)
	{ _SIGRTMIN+3,	"SIGRT3",	0				      },
#endif
#if (_SIGRTMIN)+4  <= (_SIGRTMAX)
	{ _SIGRTMIN+4,	"SIGRT4",	0				      },
#endif
#if (_SIGRTMIN)+5  <= (_SIGRTMAX)
	{ _SIGRTMIN+5,	"SIGRT5",	0				      },
#endif
#if (_SIGRTMIN)+6  <= (_SIGRTMAX)
	{ _SIGRTMIN+6,	"SIGRT6",	0				      },
#endif
#if (_SIGRTMIN)+7  <= (_SIGRTMAX)
	{ _SIGRTMIN+7,	"SIGRT7",	0				      },
#endif
#if (_SIGRTMIN)+8  <= (_SIGRTMAX)
	{ _SIGRTMIN+8,	"SIGRT8",	0				      },
#endif
#if (_SIGRTMIN)+9  <= (_SIGRTMAX)
	{ _SIGRTMIN+9,	"SIGRT9",	0				      },
#endif
#if (_SIGRTMIN)+10  <= (_SIGRTMAX)
	{ _SIGRTMIN+10,	"SIGRT10",	0				      },
#endif
#endif
};

const char Sig_format[] = "signal %[-]12cs %3cs   %[-]46cs\n";

#define FN_SIG_NAME		0
#define FN_SIG_NUMBER		1
#define FN_SIG_ACTION		2
#define FN_SIG_LAST		3

#define SIG_TO_OBJ(sigtab_index)		((objid_t)(Sigtab + (sigtab_index)))
#define OBJ_TO_SIG(obj)		((int)(((sig_info_t *)(obj)) - Sigtab))

#define SIGTAB_SIZE	(sizeof Sigtab / sizeof Sigtab[0])

const char *
sig_getobjname(obj)
objid_t obj;
{
	static char name[30];
	char *s;

	strncpy(name, get_field_value(obj, FN_SIG_NAME), sizeof(name) - 1);
	name[sizeof(name) - 1] = '\0';

	for (s = name; isalnum(*s); ++s)
		if (isupper(*s))
			*s = tolower(*s);
	*s = '\0';

	return name;
}

static int
get_sig_attrs(sigtab_index)
int sigtab_index;
{
	int i;

	for (i = 0; i < SIGTAB_SIZE; ++i)
		if (Sigtab[i].si_number == sigtab_index)
			return Sigtab[i].si_attrs;

	/*  All the flags mean non-default things, so just return zero for a
	 *  signal that is not in our table.
	 */
	return 0;
}

bool
accept_signal(sig)
int sig;
{
	return (get_sig_attrs(sig) & SGH_ACCEPT) != 0;
}

bool
sig_causes_refresh(sig)
int sig;
{
	return (get_sig_attrs(sig) & SGH_REDRAW) != 0;
}

bool
sig_stops_target(sig)
int sig;
{
	return (get_sig_attrs(sig) & SGH_CONT) == 0;
}

bool
sig_is_fatal(sig)
int sig;
{
#ifdef OS_LINUX
	return 0
#ifdef SIGILL
		|| sig == SIGILL
#endif
#ifdef SIGSEGV
		|| (!(get_sig_attrs(sig) & SGH_ACCEPT) && sig == SIGSEGV)
#endif
#ifdef SIGBUS
		|| (!(get_sig_attrs(sig) & SGH_ACCEPT) && sig == SIGBUS)
#endif
			;
#else

 /* RGA allow target to continue after fatal signal if set to accept */
 /* Formerly was: */
 /*	return sig == SIGILL || sig == SIGSEGV || sig == SIGBUS;*/

	return (sig == SIGILL) ||
	  ( !(get_sig_attrs(sig) & SGH_ACCEPT) &&
	    (sig == SIGSEGV || sig == SIGBUS) );
#endif
}

const char *
signame(sig)
int sig;
{
	static char buf[50];
	int i;

	for (i = 0; i < SIGTAB_SIZE; ++i) {
		if (Sigtab[i].si_number == sig)
			return Sigtab[i].si_name;
	}

	(void) sprintf(buf, "<signal #%d>", sig);
	return buf;
}

bool
sig_kills_target_by_default(sig)
int sig;
{
	return (get_sig_attrs(sig) & (SGH_DFL_STOPS | SGH_DFL_IGNORE)) == 0;
}

const char Sghead_format[] = "Signals\n";

#define SGHEAD_OBJCODE	((objid_t)Sghead_format)

/*  Add the signal list header to the form.
 */
void
add_signals_header(par)
objid_t par;
{
	new_object(SGHEAD_OBJCODE, OT_SGHEAD, par, OBJ_CHILD);
}

static int
sigcmp(obj1, obj2)
objid_t obj1, obj2;
{
	int si1, si2;

	si1 = OBJ_TO_SIG(obj1);
	si2 = OBJ_TO_SIG(obj2);
	
	return (Sigtab[si1].si_number < Sigtab[si2].si_number) ? -1 : 1;
}

/*  Process the return from the signal header menu. Expand or collapse
 *  the signal list
 */
void
do_sgh(obj, command, arg)
objid_t obj;
int command;
char *arg;
{
	int i;

	switch(command) {
	case MR_SHOW_ALL_SIGNALS:
		for (i = 0; i < SIGTAB_SIZE; ++i) {
			if (!obj_child_exists(obj, SIG_TO_OBJ(i)))
				add_signal_object(obj, &Sigtab[i]);
		}
		sort_children(obj, sigcmp);
		break;
	case MR_HIDE_ALL_SIGNALS:
		remove_object(obj, OBJ_DESCENDENTS);
		break;
	default:
		panic("bad cmd in dsh");
	}
}

/*  Create an object describing an individual signal.
 */
static void
add_signal_object(par, si)
objid_t par;
sig_info_t *si;
{
	fval_t fields[FN_SIG_LAST + 1];
	objid_t obj;
	
	fields[FN_SIG_NAME] = (fval_t)si->si_name;
	fields[FN_SIG_ACTION] = (fval_t)strsave(sig_attr_string(si));
	fields[FN_SIG_NUMBER] = (fval_t)strf("%d", si->si_number);
	fields[FN_SIG_LAST] = (fval_t) NULL;

	obj = SIG_TO_OBJ(si - Sigtab);
	new_object(obj, OT_SIG, par, OBJ_CHILD);
	set_all_fields(obj, fields, (fval_t)NULL);
}

void
sig_getsize(obj, unused_par, sz)
objid_t obj, unused_par;
struct szst *sz;
{
	font_t *font;
	int actlen;

	font = wn_get_sysfont();

	actlen = strlen((char *)get_field_value(obj, FN_SIG_ACTION));

	sz->sz_depth = font->ft_height;
	sz->sz_width = font->ft_width * (26 + actlen);
}

void
free_sig(obj)
objid_t obj;
{
	free((char *)get_field_value(obj, FN_SIG_ACTION));
	free((char *)get_field_value(obj, FN_SIG_NUMBER));
}

/*  Return a string describing the signal's attributes.
 */
static char *
sig_attr_string(si)
sig_info_t *si;
{
	static char buf[100];
	int attrs;

	attrs = si->si_attrs;
	
	if ((attrs & SGH_CONT) == 0) {
		strcpy(buf, "Stop -");
		if (attrs & SGH_ACCEPT)
			strcat(buf, " accept signal on continue.");
		else
			strcat(buf, " ignore signal on continue.");
		return buf;
	}
	
	if ((attrs & SGH_REDRAW) != 0) {
		strcpy(buf, "Refresh display and continue,");
		if (attrs & SGH_ACCEPT)
			strcat(buf, " accepting signal.");
		else
			strcat(buf, " ignoring signal.");
		return buf;
	}
	
	if ((attrs & SGH_ACCEPT) != 0)
		strcpy(buf, "Accept and continue.");
	else
		strcpy(buf, "Ignore.");
	
	return buf;
}

char *
sig_format_obj(code)
objid_t code;
{
	sig_info_t *si;

	si = &Sigtab[OBJ_TO_SIG(code)];

	return strf("%-10s %s", si->si_name, sig_attr_string(si));
}

/*  Process the return from a signal menu. Switching attributes on and off.
 */
void
do_sig(obj, command, arg)
objid_t obj;
int command;
char *arg;
{
	target_t *xp;
	sig_info_t *si;
	
	si = &Sigtab[OBJ_TO_SIG(obj)];
	
	switch(command) {
	case MR_SIG_TOGGLE_STOP_CONT:
		si->si_attrs ^= SGH_CONT;
		break;
	case MR_SIG_TOGGLE_ACCEPT_IGNORE:
		si->si_attrs ^= SGH_ACCEPT;
		break;
	case MR_SIG_TOGGLE_REDRAW:
		si->si_attrs ^= SGH_REDRAW;
		break;
	case MR_HIDE_SIGNAL:
		remove_object(obj, OBJ_SELF);
		return;
	default:
		panic("bad cmd in ds");
	}

	change_field(obj, FN_SIG_ACTION, sig_attr_string(si));

	xp = get_current_target();
	if (xp->xp_ops->xo_handle_signal_change != NULL) {
		xp_handle_signal_change(xp,
					si->si_number,
					(si->si_attrs & SGH_ACCEPT) == 0,
					(si->si_attrs & SGH_REDRAW) != 0,
					(si->si_attrs & SGH_CONT) == 0);
	}
}

void
set_signal_attrs(sig, ignore, redraw, stop)
int sig;
bool ignore, redraw, stop;
{
	int i, attrs;

	attrs = (ignore ? 0 : SGH_ACCEPT) |
		(redraw ? SGH_REDRAW : 0) |
		(stop ? 0 : SGH_CONT);

	for (i = 0; i < SIGTAB_SIZE; ++i) {
		if (Sigtab[i].si_number == sig)
			Sigtab[i].si_attrs = attrs;
	}
}

int
handle_signal_command(cmd, args, nargs, is_hint)
const char *cmd;
char **args;
int nargs;
bool is_hint;
{
	int i, sigidx;
	char *signame, *sigstate;

	if (nargs < 2 || nargs > 4) {
		errf("Usage: %s signame [accept|ignore] [stop|continue] [redraw]", cmd);
		return -1;
	}

	signame = args[0];
	sigidx = -1;
	for (i = 0; i < SIGTAB_SIZE; i++) {
		if (!strcmp(signame, Sigtab[i].si_name)) {
			sigidx = i;
			break;
		}
	}
	if (sigidx < 0) {
		errf("bad signal name: %s", signame);
		return -1;
	}
	i = 1;
	for (; i < nargs; i++) {
	    sigstate = args[i];
	    if (!strcmp(sigstate, "accept")) {
		    Sigtab[sigidx].si_attrs |= SGH_ACCEPT;
	    }
	    else if (!strcmp(sigstate, "ignore")) {
		    Sigtab[sigidx].si_attrs &= (~SGH_ACCEPT);
	    }
	    else if (!strcmp(sigstate, "stop")) {
		    Sigtab[sigidx].si_attrs &= (~SGH_CONT);
	    }
	    else if (!strcmp(sigstate, "continue")) {
		    Sigtab[sigidx].si_attrs |= SGH_CONT;
	    }
	    else if (!strcmp(sigstate, "redraw")) {
		    Sigtab[sigidx].si_attrs |= SGH_REDRAW;
	    }
	    else if (!strcmp(sigstate, "noredraw")) {
		    Sigtab[sigidx].si_attrs &= (~SGH_REDRAW);
	    }
	    else {
		    errf("bad signal attribute: %s", sigstate);
		    return -1;
	    }
	}
	return 0;
}

int
save_signal_state_to_file(fp)
FILE *fp;
{
	int i;

	/*
	   It would be better if this routine only saved the signals which
	   changed from the defaults. However, at least this method ensures
	   consistent behaviour from the config file, even if the defaults
	   in ups change.  It's verbose, though.
	*/
	for (i = 0; i < SIGTAB_SIZE; i++) {
		(void)fprintf(fp, "signal %s", Sigtab[i].si_name);
		if (Sigtab[i].si_attrs & SGH_ACCEPT)
			(void)fputs(" accept", fp);
		else
			(void)fputs(" ignore", fp);
		if (Sigtab[i].si_attrs & SGH_CONT)
			(void)fputs(" continue", fp);
		else
			(void)fputs(" stop", fp);
		if (Sigtab[i].si_attrs & SGH_REDRAW)
			(void)fputs(" redraw", fp);
		else
			(void)fputs(" noredraw", fp);
		(void)fputc('\n', fp);
	}
	return 0;
}


void
sig_getcolor(obj, wn, p_fg, p_bg)
objid_t obj;
int wn;
long *p_fg, *p_bg;
{
  static color_t color= { -1, 0, 0, 0 };

  if (color.co_pixel == -1)
  {
    const char *colorname;
    
    color.co_pixel = WN_FG(wn);
    if (wn_use_extended_text_colors() == TRUE)
    {
      colorname = wn_get_default("SignalTextColor");
      if (colorname != NULL)
      {
	wn_parse_color(wn, colorname, &color);
	if (!wn_alloc_color(wn, &color,colorname))
	  color.co_pixel = WN_FG(wn);
      }
    }
  }
  *p_fg = color.co_pixel;
  *p_bg = WN_BG(wn);
}
