/*  Copyright 1993 John Bovey, University of Kent at Canterbury.
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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <local/wn.h>
#include <local/menu3.h>
#include "layout.h"
#include "font.h"
#include "bmenu.h"
#include "med3.h"
#ifndef lint
static char sccsid[] = "%W% (UKC) %G%";
#endif  /* lint */

extern int wn;			/* the window number */
extern font_t *plainfont;

/*  Raster pattern for a 3 button puck.
 */
static unsigned short b3pat[] = {
	0xFF1F,0xE3FC,0xAB15,0x62AC,
	0xD51A,0xA354,0xAB15,0x62AC,
	0xD51A,0xA354,0xAB15,0x62AC,
	0xD51A,0xA354,0xAB15,0x62AC,
	0xD51A,0xA354,0xAB15,0x62AC,
	0xD51A,0xA354,0xAB15,0x62AC,
	0xD51A,0xA354,0xAB15,0x62AC,
	0xD51A,0xA354,0xAB15,0x62AC,
	0xD51A,0xA354,0xAB15,0x62AC,
	0xD51A,0xA354,0xAB15,0x62AC,
	0xD51A,0xA354,0xAB15,0x62AC,
	0xD51A,0xA354,0xAB15,0x62AC,
	0xD51A,0xA354,0xAB15,0x62AC,
	0xD51A,0xA354,0xAB15,0x62AC,
	0xFF1F,0xE3FC,
};

static bitmap_t buttonmap = wn_static_bm(30,29,1,0,0,BM_BIT0_RIGHT,b3pat);

static short mh_top, mh_left;	/* mouse hole top-left corner */

/*  Structure describing the position of a mouse hole caption.
 *  The position is that of the top-centre of the caption.
 */
struct mhcst {
	short mh_x;
	short mh_y;
};

/*  caption positions for physical buttons for the 3 button puck
 */
static struct mhcst bpos[] = {
	{ MHWIDTH/4 - 8, MHHEIGHT/2 - 9},	/* Left button */
	{ MHWIDTH/2, 4 },			/* Middle button */
	{ 3*MHWIDTH/4 + 8, MHHEIGHT/2 - 9 },	/* Right button */
};

/*  Determine the mouse hole position and draw the box. Should be called after
 *  a window size change.
 */
void
mhset()
{
	int x1, y1, x2, y2;
	int width, height;

	/* determine the mousehole position
	 */
	wn_get_window_size(wn,&width,&height);
	mh_left = width - MHWIDTH;
	mh_top = -1;

	/* Draw the box
	 */
	x1 = mh_left;
	y1 = mh_top;
	x2 = x1;
	y2 = y1 + MHHEIGHT;
	wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
	x1 += MHWIDTH;
	x2 += MHWIDTH;
	wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
	x1 = mh_left;
	y1 = mh_top;
	x2 = x1 + MHWIDTH;
	y2 = y1;
	wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
	y1 += MHHEIGHT;
	y2 += MHHEIGHT;
	wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));

	/*  Draw the buttons
	 */
	wn_rop_from_mem(&buttonmap,0,0,30,29,wn,mh_left + MHWIDTH/2 - 15,
				mh_top + MHHEIGHT/2 - 10,R_RPL);
}

void
mhcaps(capa,capb,capc)
char *capa, *capb, *capc;
{
	static char *caps[] = {NULL, NULL, NULL, NULL};
	register sw, pb, x, y;
	register char *cp = 0;

	if (caps == NULL) {
		return;
	}
	Mpushsu(wn);
	for (pb = 0; pb < 3; pb++) {
		switch (pb) {
			case 0 : cp = capa; break;
			case 1 : cp = capb; break;
			case 2 : cp = capc; break;
		}
		if (cp == caps[pb])
			continue;
		if (caps[pb] != NULL) {
			/* delete the previous caption
			 */
			sw = wn_strwidth(caps[pb],plainfont);
			x = mh_left + bpos[pb].mh_x - sw/2;
			y = mh_top + bpos[pb].mh_y;
			wn_set_area(wn,x,y,sw,FONTHEIGHT(plainfont),WN_BG(wn));
		}
		caps[pb] = cp;
		if (cp != NULL) {
			sw = wn_strwidth(caps[pb],plainfont);
			x = mh_left + bpos[pb].mh_x - sw/2;
			y = mh_top + bpos[pb].mh_y;
			wn_text(wn,plainfont,caps[pb],x,y,WN_FG(wn),WN_BG(wn),WN_USE_TOP);
		}
	}
	Mpopsu(wn);
}
