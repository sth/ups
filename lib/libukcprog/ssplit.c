/* ssplit.c - split a string into a vector of arguments */

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


/* @(#)ssplit.c	1.7 30/5/93 UKC */
char ukcprog_ssplit_rcsid[] = "$Id$";

#include <string.h>
#include <stdlib.h>

#include "ukcprog.h"

/*  Split a string into a NULL terminated vector of words.
 *  The original string is not modified.
 *
 *  Words in the given string are delimited by any character in
 *  the delimiters argument.
 *
 *  By default leading delimiters are ignored and multiple adjacent
 *  delimiters are treated as a single delimiter.  If the delimiters
 *  string starts with a NUL then each occurence of a delimiter
 *  indicates a field and zero length fields are possible.  Thus to
 *  split a password file line, use ssplit(line, "\0:").
 *
 *  The returned vector is malloc'ed storage - when it is finished with
 *  it should be passed to free() to release its storage.
 */
char **
ssplit(line, delimiters)
const char *line, *delimiters;
{
	int ncomp;
	bool want_null_fields;
	const char *cp;
	char **vecp, *buf;
	char *mem;

	want_null_fields = *delimiters == '\0';
	if (want_null_fields)
		++delimiters;

	ncomp = 1;
	if (!want_null_fields) {
		while (*line != '\0' && strchr(delimiters, *line) != NULL)
			++line;
	}
	for (cp = line; *cp != '\0'; cp++)
		if (strchr(delimiters, *cp) != NULL)
			++ncomp;

	/*  We need ncomp+1 char* sized slots for the string pointers
	 *  and terminating NULL, plus space for a copy of the string
	 *  including the terminating NUL.
	 */
	mem = e_malloc((ncomp + 1) * sizeof(char *) + strlen(line) + 1);

	vecp = (char **)mem;
	buf = mem + (ncomp + 1) * sizeof(char *);
	strcpy(buf, line);

	for (;;) {
		if (!want_null_fields) {
			while (*buf != '\0' &&
					strchr(delimiters, *buf) != NULL)
				++buf;
			if (*buf == '\0')
				break;
		}

		*vecp++ = buf;

		if (*buf == '\0')
			break;

		while (*buf != '\0' && strchr(delimiters, *buf) == NULL)
			buf++;

		if (*buf == '\0')
			break;
		*buf++ = '\0';
	}
	*vecp = NULL;

	return (char **)mem;
}
