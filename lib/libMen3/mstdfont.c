/* mstdfont.c - Mstdfont code and data */

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


/* @(#)mstdfont.c	1.6 5/5/91 (UKC) */
char Men3_mstdfont_c_rcsid[] = "$Id$";

#include <local/wn.h>
#include "menu3.h"

#ifndef NULL
#define NULL 0
#endif

#define MXSTDFONT	"-*-courier-bold-r-*-*-*-140-*-*-*-*-*-*"
#define MSUNSTDFONT	"/usr/lib/fonts/fixedwidthfonts/screen.b.12"

static font_t *Stdfont = NULL;

font_t *
Mstdfont()
{
	if (Stdfont == NULL) {
		switch (wn_get_wm_type()) {
		case WN_X11:
			Stdfont = wn_open_font(MXSTDFONT);
			break;
		case WN_SUNVIEW:
			Stdfont = wn_open_font(MSUNSTDFONT);
			break;
		default:
			/* Just leave stdfont as NULL */
			break;
		}
	}

	if (Stdfont == NULL)
		Stdfont = wn_get_sysfont();

	return(Stdfont);
}

void
Msetstdfont(font)
font_t *font;
{
	Stdfont = font;
}
