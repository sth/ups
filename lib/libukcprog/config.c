/* config.c - routines for parsing configuration files */

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


/* @(#)config.c	1.4 30/5/93 UKC */
char ukcprog_config_rcsid[] = "$Id$";

#include <stdio.h>	/* for NULL */
#include <ctype.h>
#include <string.h>

#include "ukcprog.h"

/*  Trim anything following a `#' and leading and trailing whitespace
 *  from a line.  We do this in place and return a pointer to the
 *  trimmed line.
 */
char *
config_trim_line(line)
char *line;
{
	char *hash;
	int len;
	
	while (isspace(*line))
		++line;
	if ((hash = strchr(line, '#')) != NULL)
		*hash = '\0';

	len = strlen(line);
	while (--len >= 0) {
		if (!isspace(line[len]))
			break;
	}
	line[len + 1] = '\0';

	return line;
}
