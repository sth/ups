/* arg_lparse.c - do shell type parsing of a command line into arguments */

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


/* @(#)arg_lparse.c	1.11 17 Apr 1994 (UKC) */
char arg_lparse_rcsid[] = "$Id$";

#include <stdio.h>

#include <local/ukcprog.h>

#include "arg.h"

static void skip_white PROTO((const char **p_line));
static int get_redir_arg PROTO((const char **p_line, dvec_t *p_rdv));
static int get_glob PROTO((const char **p_line, dvec_t *p_gdv));
static void add_words_and_free PROTO((ebuf_t *eb, dvec_t dv));

static void
skip_white(p_line)
const char **p_line;
{
	const char *line;

	line = *p_line;
	while(*line != '\0' && (*line == ' ' || *line == '\t' || *line == '\n'))
		line++;
	*p_line = line;
}

static int
get_redir_arg(p_line, p_rdv)
const char **p_line;
dvec_t *p_rdv;
{
	dvec_t rdv;
	const char *line, **args;
	
	skip_white(p_line);
	line = *p_line;

	if (*line == '\0') {
		errf("Missing filename for redirection");
		return -1;
	}
	
	if ((rdv = arg_glob(&line)) == 0)
	        return -1;
			
	args = get_dvec_vec(rdv);
			
	if (args[0] == NULL) {
		errf("No filenames matched pattern `%.*s'",
		     line - *p_line, *p_line);
		return -1;
	}
			
	if (args[1] != NULL) {
		errf("`%.*s': ambiguous", line - *p_line, *p_line);
		return -1;
	}

	*p_rdv = rdv;
	*p_line = line;
	return 0;
}

static int
get_glob(p_line, p_gdv)
const char **p_line;
dvec_t *p_gdv;
{
	dvec_t gdv;
	const char *line, **args;

	line = *p_line;
	
	if ((gdv = arg_glob(&line)) == 0)
		return -1;
	
	args = get_dvec_vec(gdv);
			
	if (*args == NULL) {
		errf("No files match filename pattern `%.*s'",
		     line - *p_line, *p_line);
		return -1;
	}

	*p_gdv = gdv;
	*p_line = line;
	return 0;
}

static void
add_words_and_free(eb, dv)
ebuf_t *eb;
dvec_t dv;
{
	const char **args;
	
	for (args = get_dvec_vec(dv); *args != NULL; args++) {
		ebuf_add(eb, "'", 1);
		ebuf_addstr(eb, *args);
		ebuf_add(eb, "' ", 2);
	}
	
	free_dvec(dv);
}

int
arg_make_shell_line(line, p_shell_line)
const char *line;
char **p_shell_line;
{
	ebuf_t *eb;
	int res;

	eb = ebuf_create(TRUE);
	res = -1;
	
	for (;;) {
		const char *redirstr, *postarg_str;
		
		skip_white(&line);
		
		if (*line == '\0') {
			res = 0;
			break;
		}

		if (arg_convert_redir_to_shell_syntax(&line, &redirstr,
						      &postarg_str) != 0)
			break;

		if (redirstr != NULL) {
			ebuf_addstr(eb, redirstr);
			ebuf_add(eb, " ", 1);
			
			if (postarg_str != NULL) {
				dvec_t rdv;
			
				if (get_redir_arg(&line, &rdv) != 0)
					break;

				add_words_and_free(eb, rdv);
				ebuf_addstr(eb, postarg_str);
			}
		}
		else {
			dvec_t gdv;

			if (get_glob(&line, &gdv) != 0)
				break;

			add_words_and_free(eb, gdv);
		}
			
	}

	if (res == 0) {
		ebuf_add(eb, "", 1);
		*p_shell_line = strsave(ebuf_get(eb, (int *)NULL));
	}

	ebuf_free(eb);
	return res;
}

int
arg_lparse(line, p_dv, p_rdlist)
const char *line;
dvec_t *p_dv;
long *p_rdlist;
{
	dvec_t adv;
	int res;
	long rdlist;

	rdlist = 0;
	res = -1;
	adv = make_dvec();
	
	for (;;) {
		redirtype_t redirtype;
		int *fdaddr;
		
		skip_white(&line);
		
		if (*line == '\0') {
			res = 0;
			break;
		}
		
		redirtype = arg_get_redir(&line, &fdaddr, &rdlist);
		if (redirtype == RD_ERROR)
			break;
		
		if (fdaddr != NULL) {
			dvec_t rdv;
			int fd;

			if (get_redir_arg(&line, &rdv) != 0)
				break;
			
			if ((fd = arg_open_redir_file(*get_dvec_vec(rdv),
						      redirtype)) == -1)
				break;
			
			*fdaddr = fd;
			free_dvec_and_strings(rdv);
		}
		else if (redirtype == RD_NOT_REDIR) {
			dvec_t gdv;
			const char **args;
			
			if (get_glob(&line, &gdv) != 0)
				break;
			
			for (args = get_dvec_vec(gdv); *args != NULL; args++)
				add_to_dvec(adv, *args);
			
			free_dvec(gdv);
		}
	}
	
	if (res != 0) {
		arg_tidy_redirs_in_parent(rdlist);
		free_dvec(adv);
		*p_dv = 0;
		*p_rdlist = 0;
	}
	else {
		add_to_dvec(adv, (char *)NULL);
		*p_dv = adv;
		*p_rdlist = rdlist;
	}
	
	return res;
}

