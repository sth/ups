/* wdbyte.c - Perq wdbyte emulation (should zap this) */

/*  Copyright 1991 John Bovey, University of Kent at Canterbury.
 *  All Rights Reserved.
 *
 *  This file is part of MED3.
 *
 *  MED3 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  MED3 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with UPS; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* @(#)wdbyte.c	1.10 22/12/93 (UKC) */
char Men3_wdbyte_c_rcsid[] = "$Id$";

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include <local/wn.h>
#include "menu3.h"
#include "menu_priv.h"
#include "wdbyte.h"

#ifndef NULL
#define NULL 0
#endif

/*  Emulation of the perq wdbyte(2) system call over wn.
 *  Deals correctly with variable width fonts, and the various
 *  return codes.
 *  It may differ from wdbyte() when two stop conditions arise
 *  simultaneously.
 */
/* ARGSUSED */
int
wn_wdbyte(wn,dbargp,clipp,fg_color,bg_color)
int wn;
struct DBCtl *dbargp;
Box_t *clipp;
long fg_color;
long bg_color;
{
	static char *buf;
	static unsigned buflen = 0;
	register font_t *font;
	const char *cptr, *start, *lim;
	int rv, xpos, max_x, chwidth, len;
	
	if (buflen == 0) {
		buflen = 128;
		buf = malloc((size_t)buflen + 1);
		if (buf == NULL)
			abort();
	}
	font = (dbargp->DBFont != NULL) ? dbargp->DBFont : wn_get_sysfont();
	start = dbargp->DBSrcString + dbargp->DBByteOffset;
	lim = dbargp->DBSrcString + dbargp->DBMaxByte;
	if (start >= lim)
		return(0);
	xpos = dbargp->DBX;
	max_x = dbargp->DBMaxX;
	for (cptr = start; ; cptr++) {
		if (*cptr >= 0 && *cptr < 32) {
			rv = HIT_CTL_CHAR;
			break;
		}
		chwidth = font->ft_width_tab[*cptr & 127];
		if (xpos + chwidth >= max_x) {
			rv = HIT_MAX_X;
			break;
		}
		if (cptr >= lim) {
			rv = HIT_MAXBYTE;
			break;
		}
		xpos += chwidth;
	}

	len = cptr - start;
	while (len > buflen) {
		buflen *= 2;
		free(buf);
		buf = malloc((size_t)buflen + 1);
	}
	(void) strncpy(buf, start, (size_t)len);
	buf[len] = '\0';

	if (dbargp->DBScreen == 0) {
	  wn_xtext(wn,font,buf,
		   dbargp->DBX,dbargp->DBY,
		   WN_MAXWIDTH,dbargp->DBFunc,
		   fg_color,bg_color,WN_USE_BASELINE,0);
	}
	dbargp->DBX = xpos;
	dbargp->DBByteOffset += len;
	return(rv);
}
