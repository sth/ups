/* serverhost.c - getpwent() style interface to /etc/serverhosts */

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


/* @(#)serverhost.c	1.7 16 Sep 1994 (UKC) */
char serverhost_c_rcsid[] = "$Id$";

#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <string.h>
#include <local/ukcprog.h>

#include "serverhost.h"
#include "utils.h"

static const char Sh_filename[] = "/etc/serverhosts";
static FILE *Sfp;
static int Lnum;
static serverhost_status_t Sh_status = SH_CLOSED;

#define MAX_HOSTS	10

serverhost_status_t
getserverhoststatus()
{
	return Sh_status;
}

int
setserverhostent()
{
	if (Sfp == NULL) {
		if ((Sfp = fopen(Sh_filename, "r")) == NULL) {
			failmesg("Can't open", "", Sh_filename);
			return -1;
		}
	}
	else {
		if (fseek(Sfp, 0L, L_SET) == EOF) {
			failmesg("Can't fseek in", "", Sh_filename);
			return -1;
		}
		clearerr(Sfp);
	}
	Lnum = 0;
	Sh_status = SH_OPEN;
	return 0;
}

void
endserverhostent()
{
	(void) fclose(Sfp);
	Sfp = NULL;
	Sh_status = SH_CLOSED;
}

struct serverhostent *
getserverhostent()
{
	static const char *hostnames[MAX_HOSTS + 1];
	static char buf[256];
	static char *cptr;
	static struct serverhostent shbuf;
	int nhosts;

	do {
		if (fgets(buf, sizeof(buf), Sfp) == NULL) {
			Sh_status = ferror(Sfp) ? SH_ERROR : SH_EOF;
			if (ferror(Sfp))
				failmesg("Read error in", "", Sh_filename);
			return NULL;
		}
		++Lnum;

		if ((cptr = strchr(buf, '\n')) == NULL) {
			errf("Line %d too long in %s", Lnum, Sh_filename);
			return NULL;
		}
		*cptr = '\0';

		for (cptr = buf; *cptr == ' ' || *cptr == '\t'; ++cptr)
			;
	} while (*cptr == '#' || *cptr == '\0');
	
	shbuf.sh_servname = cptr;
	if ((cptr = strchr(cptr, ':')) == NULL) {
		errf("Missing ':' on line %d of %s", Lnum, Sh_filename);
		return NULL;
	}
	*cptr++ = '\0';

	nhosts = 0;
	for (;;) {
		while (*cptr == ' ' || *cptr == '\t')
			++cptr;
		if (*cptr == '\0' || *cptr == '#')
			break;
		if (nhosts == MAX_HOSTS) {
			errf("Too many hosts on line %d of %s (max %d)",
						Lnum, Sh_filename, MAX_HOSTS);
			return NULL;
		}
		hostnames[nhosts++] = cptr;
		while (*cptr != ' ' && *cptr != '\t' && *cptr != '\0')
			++cptr;
		if (*cptr == '\0')
			break;
		*cptr++ = '\0';
	}
	hostnames[nhosts] = NULL;
	shbuf.sh_hosts = hostnames;

	return &shbuf;
}

struct serverhostent *
getserverhostbyservname(servname)
const char *servname;
{
	struct serverhostent *sh;

	if (setserverhostent() != 0)
		return NULL;
	while ((sh = getserverhostent()) != NULL)
		if (strcmp(sh->sh_servname, servname) == 0)
			return sh;
	return NULL;
}
