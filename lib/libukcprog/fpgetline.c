/* fpgetline.c - read an arbitrary length line from a stdio stream */

/*  Copyright 1992  Mark Russell, University of Kent at Canterbury.
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


/* @(#)fpgetline.c	1.7 9/10/93 UKC */
char ukcprog_fpgetline_rcsid[] = "$Id$";

#ifndef __STDC__
#include <sys/types.h>		/* for size_t */
#endif

#include <stdio.h>
#include <stdlib.h>

#include "ukcprog.h"

#define SMALLBUF_SIZE	((size_t)200)

char *
fpgetline(fp)
FILE *fp;
{
	static char *buf;
	static size_t bufsize = 0;
	int ch, pos;

	if (bufsize == 0) {
		bufsize = 80;
		buf = e_malloc(bufsize + 1);
	}

	for (pos = 0; (ch = getc(fp)) != EOF && ch != '\n'; buf[pos++] = ch) {
		if (pos == bufsize) {
			bufsize *= 2;
			buf = e_realloc(buf, bufsize + 1);
		}
	}
	buf[pos] = '\0';

	/*  If we have a huge buffer from the last call and now have a
	 *  short line, try to dump the excess.
	 */
	if (pos <= SMALLBUF_SIZE && bufsize > 5000) {
		char *smallbuf;
		
		if ((smallbuf = realloc(buf, SMALLBUF_SIZE + 1)) != NULL) {
			buf = smallbuf;
			bufsize = SMALLBUF_SIZE;
		}
	}

	return (pos == 0 && ch == EOF) ? NULL : buf;
}
