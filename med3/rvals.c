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

#include <stdio.h>
#include <local/wn.h>
#include <local/menu3.h>
#include "layout.h"
#include "bmenu.h"
#include "med3.h"
#ifndef lint
static char sccsid[] = "%W% (UKC) %G%";
#endif  /* lint */

#define WIDTH	16	/*  width if the return value grid */
#define HEIGHT	4	/*  height if the return value grid */
#define CELLW	15	/*  width of each cell in pixels */
#define CELLH	18	/*  height each cell in pixels */
#define BOXW	(WIDTH * CELLW)
#define BOXH	(HEIGHT * CELLH)
#define RVMAX	(WIDTH * HEIGHT)
#define NLAYER	4

#define RV_CHR(rv)	((rv) & 0177)
#define RV_LAYER(rv)	(((rv) >> 8) & 3)
#define RV_FULL(l,c)	(((l) << 8) | (c))

static unsigned char rvmain[NLAYER][RVMAX]; /* return values in use in the main menu*/
static unsigned char rvcut[NLAYER][RVMAX];  /* return values in use in cut menus */
static int curcell = -1;	/* current return value cell*/
static int curlayr = 0;		/* currently displayed rv layer */
static short left,top;		/* the position of the rv box */
static short rvcell[256];	/* indexes of return values in rv array */

/*  All the possible character return values
 */
static unsigned char rv[] = {
	'a','b','c','d','e','f','g','h',
	'i','j','k','l','m','n','o','p',
	'q','r','s','t','u','v','w','x',
	'y','z','A','B','C','D','E','F',
	'G','H','I','J','K','L','M','N',
	'O','P','Q','R','S','T','U','V',
	'W','X','Y','Z','0','1','2','3',
	'4','5','6','7','8','9','?','!'
};

extern int wn;		/* descriptor of the current window */
extern font_t *plainfont;

static void wchar PROTO((int c, int x, int y));
static void rvinuse PROTO((int cell, int cut, int main));
static void rvcurrent PROTO((int cell));
static void rvclear PROTO((int cell));

/*  Initialise the return value data structures. Should be called just
 *  once.
 */
void
rvinit()
{
	register i, j;

	for (i = 0; i < RVMAX; i++) {
		for (j = 0; j < NLAYER; j++) {
			rvmain[j][i] = 0;
			rvcut[j][i] = 0;
		}
		rvcell[rv[i]] = i;
	}
}

/*  Redraw the return value area. Should be called after a window size
 *  change The argument is the window width, if zero the last width is used.
 */
void
rvsetup(width)
int width;
{
	int x1, y1, x2, y2;
	register int x, y, i, j, cell;
	extern font_t *boldfont;
	static int lastwidth;

	Mpushsu(wn);
	if (width <= 0)
		width = lastwidth;
	else
		lastwidth = width;
	left = width - RVWIDTH;
	top = RVTOP + 4;
	wtext(left,top - 1, "Return Value:",boldfont, NULL);
	top += LDEPTH;

	rblank(left,top,BOXW,BOXH);
	/* draw the grid
	 */
	y1 = top;
	y2 = top + BOXH;
	x = left;
	for (i = 0; i <= WIDTH; i++) {
		x1 = x2 = x;
		wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
		x += CELLW;
	}
	x1 = left;
	x2 = left + BOXW;
	y = top;
	for (i = 0; i <= HEIGHT; i++) {
		y1 = y2 =  y;
		wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
		y += CELLH;
	}

	cell = 0;
	y = top + 2;
	for (i = 0; i < HEIGHT; i++) {
		x = left + 4;
		for (j = 0; j < WIDTH; j++) {
			wchar(rv[cell],x,y);
			if (cell == curcell)
				rvcurrent(cell);
			else
				rvinuse(cell,rvmain[curlayr][cell],
							rvcut[curlayr][cell]);
			x += CELLW;
			cell++;
		}
		y += CELLH;
	}
	Mpopsu(wn);
}

/*  Change the currently displayed return value layer and change the current
 *  button's return value if there is one.
 */
void
newlayer(layer)
int layer;
{
	if (layer == curlayr)
		return;

	if (curcell >= 0) {
		bmsetrv(RV_FULL(layer,rv[curcell]));
		rvmain[curlayr][curcell]--;
		rvmain[layer][curcell]++;
	}
	curlayr = layer;
	rvsetup(0);
}

/*  Return the current layer
 */
int
curlayer()
{
	return(curlayr);
}

/*  Set the current return value cell.
 */
void
setrv(rv)
int rv;
{
	int cell;

	if (rv < 0) {
		/* No button selected. Clear cell but leave layer as is.
		 */
		if (curcell >= 0) {
			rvclear(curcell);
			rvinuse(curcell,rvmain[curlayr][curcell],
						rvcut[curlayr][curcell]);
		}
		curcell = -1;
		return;
	}

	if (curlayr != RV_LAYER(rv)) {
		/*  The layer has changed so we need to redraw the
		 *  whole rv matrix.
		 */
		curlayr = RV_LAYER(rv);
		curcell = rvcell[RV_CHR(rv)];
		rvsetup(0);
		showlrb();
		return;
	}
		
	Mpushsu(wn);
	cell = rvcell[RV_CHR(rv)];
	if ((cell != curcell) && (curcell >= 0)) {
		rvclear(curcell);
		rvinuse(curcell,(int)rvmain[curcell],(int)rvcut[curcell]);
	}
	curcell = cell;
	rvclear(cell);
	rvcurrent(cell);
	Mpopsu(wn);
}

/*  Attempt to select an rv cell using the coordinates and if successful
 *  try and set the return value of the current button
 */
void
selrv(x,y)
int x,y;
{
	int cell;

	if (curcell < 0)
		return;
	if ((x <= left) || (x >= left+BOXW) || (y <= top) || (y >= top+BOXH))
		return;
	cell = ((y - top) / CELLH) * WIDTH + (x - left) / CELLW;
	if (cell == curcell)
		return;
	bmsetrv(RV_FULL(curlayr,rv[cell]));
	rvclear(curcell);
	rvmain[curlayr][curcell]--;
	rvmain[curlayr][cell]++;
	rvinuse(curcell,rvmain[curlayr][curcell],rvcut[curlayr][curcell]);
	rvclear(cell);
	rvcurrent(cell);
	curcell = cell;
}

/*  Return 1 if the coordinates would select a return value. This is only
 *  for the benefit of the mouse hole captions.
 */
int
inrvals(x,y)
int x,y;
{
	if (curcell < 0)
		return(0);
	if ((x <= left) || (x >= left+BOXW) || (y <= top) || (y >= top+BOXH))
		return(0);
	return(1);
}

/*  display the character at the coordinates
 */
static
void
wchar(c,x,y)
int c;
int x, y;
{
	char s[2];

	s[0] = c;
	s[1] = 0;
	wn_text(wn,plainfont,s,x,y,WN_FG(wn),WN_BG(wn),WN_USE_TOP);
}

/*  highlight the cell to show the rv is in use.
 */
static
void
rvinuse(cell,main,cut)
int cell, cut, main;
{
	int x, y;

	if (!(main || cut))
		return;

	Mpushsu(wn);
	x = (cell % WIDTH) * CELLW + left + 1;
	y = (cell / WIDTH) * CELLH + top + 1;
	if (main)
		wn_set_area(wn,x,y,3,3,WN_FG(wn));
	if (cut)
		wn_set_area(wn,x + CELLW - 4, y, 3, 3, WN_FG(wn));
	Mpopsu(wn);
}

/*  Highlight the cell to show it is current.
 */
static
void
rvcurrent(cell)
int cell;
{
	int x, y;

	Mpushsu(wn);
	x = (cell % WIDTH) * CELLW + left + 1;
	y = (cell / WIDTH) * CELLH + top + 1;
	wn_invert_area(wn, x, y, CELLW - 1, CELLH - 1);
	Mpopsu(wn);
}

/*  Clear the cell of any highlighting.
 */
static
void
rvclear(cell)
int cell;
{
	int x, y;

	Mpushsu(wn);
	x = (cell % WIDTH) * CELLW + left + 1;
	y = (cell / WIDTH) * CELLH + top + 1;
	wn_set_area(wn,x,y,CELLW-1,CELLH-1,WN_BG(wn));
	wchar(rv[cell],x + 3,y + 1);
	Mpopsu(wn);
}


/*  Return the next free return value if there is one and update the matrix
 */
int
rvnext()
{
	register i, j;

	for (j = 0; j < NLAYER; j++)
		for (i = 0; i < RVMAX; i++)
			if ((rvmain[j][i] == 0) && (rvcut[j][i] == 0)) {
				rvmain[j][i]++;
				if (curlayr == j) {
					Mpushsu(wn);
					rvclear(i);
					rvinuse(i,rvmain[j][i],rvcut[j][i]);
					Mpopsu(wn);
				}
				return(RV_FULL(j,rv[i]));
			}
	return(0,rv[0]);
}

/*  Move a return value between main menu and cut menus
 */
void
rvmove(rv,tomain)
int rv,tomain;
{
	int cell, layer;

	cell = rvcell[RV_CHR(rv)];
	layer = RV_LAYER(rv);
	if (tomain) {
		rvmain[layer][cell]++;
		rvcut[layer][cell]--;
	} else {
		rvmain[layer][cell]--;
		rvcut[layer][cell]++;
	}
}

/*  Free the main menu return value.
 */
void
rvfree(rv)
int rv;
{
	int cell;

	cell = rvcell[RV_CHR(rv)];
	rvmain[RV_LAYER(rv)][cell]--;
}

void
rvclearmain()
{
	int i, j;

	for (j = 0; j < NLAYER; j++)
		for (i = 0; i < RVMAX; i++)
			rvmain[j][i] = 0;
}

/*  Flag the return value as taken.
 */
void
rvtaken(rv)
int rv;
{
	int cell;

	cell = rvcell[RV_CHR(rv)];
	rvmain[RV_LAYER(rv)][cell]++;
}
