/* errf.c -- formatted error messages */

/*  Copyright 1992  Godfrey Paul, University of Kent at Canterbury.
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


/* @(#)errf.c	1.15 30/5/93 UKC */
char ukcprog_errf_rcsid[] = "$Id$";

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ukcprog.h"

static const char *munge_progname PROTO((const char *));
static void default_ofunc PROTO((const char *str));

static errf_ofunc_t User_errf_ofunc = default_ofunc;
static errf_ofunc_t User_errf_ofunc1 = NULL;
static const char *Output_prefix = "";
static char *Progname = NULL;

static void
default_ofunc(str)
const char *str;
{
	fprintf(stderr, "%s%s\n", Output_prefix, str);
	fflush(stderr);
}

errf_ofunc_t
errf_set_ofunc(func)
errf_ofunc_t func;
{
	errf_ofunc_t old;

	if (func == NULL)
		func = default_ofunc;

	old = User_errf_ofunc;
	User_errf_ofunc = func;

	return old;
}

errf_ofunc_t
errf_set_ofunc1(func)
errf_ofunc_t func;
{
	errf_ofunc_t old;

	old = User_errf_ofunc1;
	User_errf_ofunc1 = func;

	return old;
}


/*  errf_set_prefix()
 *  Sets the string that errf() prefixes to messages.
 *  Returns the old one.
 */
const char *
errf_set_prefix(prefix)
const char *prefix;
{
	const char *old;

	old = Output_prefix;
	Output_prefix = prefix;

	return old;
}


/*  errf_get_prefix()
 *  Return the current prefix.
 */
const char *
errf_get_prefix()
{
	return Output_prefix;
}


#if defined(unix) || defined(__unix__) || defined(__NetBSD__)
static const char *
munge_progname(progname)
const char *progname;
{
	const char *name;
	
	if ((name = strrchr(progname, '/')) == NULL)
		name = progname;

	else
		++name;

	if (Progname != NULL)
		free(Progname);

	Progname = strsave(name);

	return "%s: ";
}
#endif /* unix */


#ifdef VMS
static const char *
munge_progname(progname)
const char *progname;
{
	const char *name, *dot;
	char  *s;
	int i;

	if ((name = strrchr(progname, ']')) == NULL)
		name = progname;
	else
		++name;

	if ((dot = strchr(name, '.')) == NULL)
		dot = name + strlen(name);

	s = strf("%.*s", dot - name, name);

	for (i = 0; i < dot - name; ++i)
		s[i] = toupper(s[i]);

	if (Progname != NULL)
		free(Progname);

	Progname = s;

	return "%%%s, ";
}
#endif /* VMS */


#ifdef MSDOS
static const char *
munge_progname(progname)
const char *progname;
{
	const char *name;
	
	if ((name = strrchr(progname, '\\')) == NULL)
		name = progname;

	else
		++name;

	if (Progname != NULL)
		free(Progname);

	Progname = strsave(name);

	return "%s: ";
}
#endif /* MSDOS */


/*  errf_set_progname()
 *  Convenience routine to set the errf prefix to include the progname.
 */
void
errf_set_progname(name)
const char *name;
{
	const char *fmt;

	fmt = munge_progname(name);
	errf_set_prefix(strf(fmt, Progname));
}


/*  errf_get_progname()
 */
const char *
errf_get_progname()
{
	return Progname;
}


#ifdef __STDC__
void
errf(const char *fmt, ...)
{

#else /* !__STDC__ */
void
errf(va_alist)
va_dcl
{
	char *fmt;
#endif /* !__STDC__ */
	va_list args;
	char buffer[100];
	char *s;

#ifdef __STDC__
	va_start(args, fmt);
#else
	va_start(args);
	fmt = va_arg(args, char *);
#endif

	s = formf(buffer, sizeof(buffer), fmt, args);

	va_end(args);

	(*User_errf_ofunc)(s);
	if (User_errf_ofunc1 != NULL)
	{
	  strcat(s, "\n");
	  (*User_errf_ofunc1)((*s == '\b') ? s + 1 : s);
	}

	if (s != buffer)	/* must have been obtained from malloc */
		free(s);
}


/*  errf_usage()
 *  Output a usage string prefixed by the program name and exit(1).
 */
void
errf_usage(usage)
const char *usage;
{
	if (User_errf_ofunc != default_ofunc)
		(*User_errf_ofunc)(strf("Usage: %s %s", Progname, usage));
	else
		fprintf(stderr, "Usage: %s %s\n", Progname, usage);
	exit(1);
}
