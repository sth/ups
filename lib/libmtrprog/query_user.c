/* query_user.c - ask the user a yes/no question */

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


/* @(#)query_user.c	1.8 16 Sep 1994 (UKC) */
char utils_query_user_rcsid[] = "$Id$";

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <local/ukcprog.h>

#include "utils.h"

int
query_user(prompt, yes, no, p_isyes)
const char *prompt, *yes, *no;
bool *p_isyes;
{
	char response[40], *nlpos, *s;
	bool yesno;

	yesno = strcmp(yes, "yes") == 0 && no != NULL && strcmp(no, "no") == 0;

	for (;;) {
		fputs(prompt, stdout);
		fflush(stdout);
		if (fgets(response, sizeof(response), stdin) == NULL) {
			if (ferror(stdin)) {
				failmesg("Read error in stdin", "", "");
			}
			else {
				puts("^D");
				errf("Unexpected EOF in stdin");
			}
			return -1;
		}

		if ((nlpos = strchr(response, '\n')) == NULL) {
			int ch;

			if (no != NULL) {
				errf(
			"Response (\"%s ...\") too long - answer `%s' or `%s'",
						       response, yes, no);
			}
			else {
				errf(
			"Response (\"%s ...\") too long - answer `%s'",
						       response, yes);
			}

			while ((ch = getchar()) != '\n' && ch != EOF)
				;
			continue;
		}
		*nlpos = '\0';

		for (s = response; *s != '\0'; ++s) {
			if (isupper(*s))
				*s = tolower(*s);
		}

		if (strcmp(response, yes) == 0 ||
					(yesno && strcmp(response, "y") == 0)) {
			if (p_isyes != NULL)
				*p_isyes = TRUE;
			return 0;
		}
		if (no != NULL && (strcmp(response, no) == 0 ||
				      (yesno && strcmp(response, "n") == 0))) {
			if (p_isyes == NULL)
				return -1;
			*p_isyes = FALSE;
			return 0;
		}
		
		if (no != NULL)
			errf("Unexpected response \"%s\" - answer `%s' or `%s'",
							response, yes, no);
		else
			errf("Unexpected response \"%s\" - answer `%s'",
							response, yes);
	}
}
