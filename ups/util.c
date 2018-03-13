/* util.c - low level utility routines */

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


/* @(#)util.c	1.5 04 Jun 1995 (UKC) */
char ups_util_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <stdlib.h>
#include <sys/stat.h>
#include <ctype.h>
#include <fcntl.h>

#include <local/ukcprog.h>
#include <local/arg.h>

#include "util.h"

bool
parse_line(line, p_words, p_nwords)
const char *line;
char ***p_words;
int *p_nwords;
{
	ebuf_t *eb;
	int quotec, nbytes, nwords, i;
	const char *iptr;
	char *obuf, *optr, *word;
	char **words, **argv;
	bool word_exists;

	/*  No escape sequence makes a line longer, so the length of
	 *  the line is an upper bound on the length of the processes line.
	 */
	optr = word = obuf = e_malloc(strlen(line) + 1);
	eb = ebuf_create(TRUE);

	iptr = line;
	quotec = '\0';
	word_exists = FALSE;

	while (isspace(*line))
		++line;
		
	for (iptr = line; *iptr != '\0'; ++iptr) {
		if (*iptr == '\\') {
			++iptr;

			if (*iptr == '\0') {
				errf("Backslash at end of line");
				return FALSE;
			}
			else {
				static const char escapes[] = "t\tn\nb\br\r";
				const char *pos;
				int ch;
				
				pos = strchr(escapes, *iptr);

				if (pos != NULL && (pos - escapes) % 2 == 0)
					ch = pos[1];
				else
					ch = *iptr;

				*optr++ = ch;
			}
		}
		else if (*iptr == quotec) {
			quotec = '\0';
		}
		else if ((*iptr == '\'' || *iptr == '"') && quotec == '\0') {
			quotec = *iptr;
			word_exists = TRUE;
		}
		else if (isspace(*iptr) && quotec == '\0') {
			while (isspace(iptr[1]))
				++iptr;
			
			ebuf_add(eb, &word, sizeof(word));

			*optr++ = '\0';
			word = optr;
			word_exists = FALSE;
		}
		else {
			*optr++ = *iptr;
		}
	}

	if (quotec != '\0') {
		errf("Missing closing %s quote",
		     (quotec == '"') ? "double" : "single");
		return FALSE;
	}

	if (optr > word || word_exists) {
		ebuf_add(eb, &word, sizeof(word));
		*optr++ = '\0';
	}
	
	words = (char **)ebuf_get(eb, &nbytes);
	nwords = nbytes / sizeof(char *);

	/*  Make a copy of the vector and argument strings in a single
	 *  buffer.  We need to allow space for the final NULL in argv.
	 *
	 *  The caller frees this when they have finished with it.
	 */
	argv = (char **)e_malloc((size_t)(nwords + 1) * sizeof(char *) +
				 (optr - obuf));
	optr = (char *)&argv[nwords + 1];

	for (i = 0; i < nwords; ++i) {
		argv[i] = strcpy(optr, words[i]);
		optr += strlen(optr) + 1;
	}
	argv[i] = NULL;

	ebuf_free(eb);
	free(obuf);

	*p_words = argv;
	*p_nwords = nwords;
	return TRUE;
}

void
put_quoted_string(fp, str)
FILE *fp;
const char *str;
{
	const char *s;

	fputc('"', fp);
	
	for (s = str; *s != '\0'; ++s) {
		if (*s == '\\' || *s == '"')
			fputc('\\', fp);
		fputc(*s, fp);
	}

	fputc('"', fp);
}

void
dump_text(text, fp)
const char *text;
FILE *fp;
{
	bool at_line_start;
	
	at_line_start = TRUE;
	
	for (; *text != '\0'; ++text) {
		if (at_line_start)
			putc('\t', fp);
		
		putc(*text, fp);
		
		at_line_start = *text == '\n';
	}

	if (!at_line_start)
		putc('\n', fp);
}

bool
same_string(s1, s2)
const char *s1, *s2;
{
	return s1 == s2 || (s1 != NULL && s2 != NULL && strcmp(s1, s2) == 0);
}

bool
get_debug_output_path(cmdline, defpath, p_cmdline, p_path, p_overwrite)
const char *cmdline, *defpath;
const char **p_cmdline, **p_path;
bool *p_overwrite;
{
	const char *new_cmdline, *path;
	int template = 0;
	bool overwrite;

	/* Look for redirection of output with syntax
	**
	**   $debug:asmsrc func > file
	**
	** but do not get confused by template parameters in function names, 
	** e.g.   $debug:asmsrc  funct<type>::foo 
	*/

        for (path = cmdline; *path && (*path != '>' || template) ; path++)
	{
		if (*path == '<') template++;
		if (*path == '>') template--;
	}
	if ( *path == 0)
	{
		path = defpath;
		new_cmdline = cmdline;
		overwrite = TRUE;
	}
	else {
		static char *last;
		const char *pos;

		for (pos = path; pos > cmdline && pos[-1] == ' '; --pos)
			;

		overwrite = path[1] == '!';
		if (overwrite)
			++path;

		for (++path; isspace(*path); ++path)
			;
		if (*path == '\0') {
			errf("Redirection to zero length output file");
			return FALSE;
		}

		if (last != NULL)
			free(last);
		last = strf("%.*s", pos - cmdline, cmdline);
		
		new_cmdline = last;
	}

	*p_cmdline = new_cmdline;
	*p_path = path;
	*p_overwrite = overwrite;

	return TRUE;
}

FILE *
fopen_with_twiddle(path, mode)
const char *path;
const char *mode;
{
  char *new_path;
  FILE *fp;

  new_path = arg_expand_twiddle(path, (int)'~');
  fp = fopen(new_path, mode);
  free(new_path);
  return fp;
}

int
open_with_twiddle(path, mode)
const char *path;
int mode;
{
  char *new_path;
  int fd;

  new_path = arg_expand_twiddle(path, (int)'~');
  fd = open(new_path, mode);
  free(new_path);
  return fd;
}
