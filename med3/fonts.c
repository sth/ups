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
#
#include <local/wn.h>
#include <local/menu3.h>
#include "layout.h"
#include "bmenu.h"
#include "cursors.h"
#include "font.h"
#include "med3.h"
#ifndef lint
static char sccsid[] = "%W% (UKC) %G%";
#endif  /* lint */

extern int wn;			/* The window number of the output window */
extern font_t *boldfont, *plainfont;

#define NFONTS	4	/* maximum number of fonts */
#define STFNAME "standard_font"
#define SYFNAME "default_font"
#define NOPTS	2	/* the number of feedback options */
#define NSCALE	2	/* the number of submenu scale options */
#define NCREL	2	/* the number of cursor relative options */
#define NLCLS	2	/* the number of line colours */
#define NLAYER	4	/* the number of return value layers */
#define FBSIZE 12	/* the size of a button */

struct fbpst {
	short fb_x;
	short fb_y;
};

static font_t *font[NFONTS];
static char *fname[NFONTS];
static int curfont;	/* the number of the current font */
static struct fbpst fbpos[NFONTS];

static struct fbpst optpos[NOPTS];
static int curopt;	/* the current option */

static struct fbpst sclpos[NSCALE];
static int curscl;	/* current scaling option */

static struct fbpst crelpos[NCREL];
static int curcrel;	/* current 'cursor relative' option */

static struct fbpst layrpos[NLAYER];

static struct fbpst lcpos[NLCLS];
static int curlc;	/* current line colour */


static void showbs PROTO((struct fbpst *rbp, int n, int current));
static int getstyle PROTO((void));

/*  Return the default font for a new button
 */
int
def_font()
{
	return(0);
}

font_t *
perqfont(n)
int n;
{
	if (n < 0)
		return(NULL);
	else
		return(font[n]);
}


/*  Initialize the fonts and other buttons. Should be called just once.
 */
void
init_fonts()
{
	int i;

	for (i = 0; i < NFONTS; i++) {
		if (i < 2) {
			font[i] = boldfont;
			fname[i] = malloc(sizeof(STFNAME));
			strcpy(fname[i],STFNAME);
		} else {
			font[i] = wn_get_sysfont();
			fname[i] = malloc(sizeof(SYFNAME));
			strcpy(fname[i],SYFNAME);
		}
		reg_fontname(&fname[i], 0, 0, 0, i);
		fbpos[i].fb_x = FALEFT + 10;
		fbpos[i].fb_y = FATOP + 4 + (i + 1) * 16;
	}
	curfont = -1;
	curopt = -1;
	curscl = -1;
	curlc = -1;
	curcrel = -1;
}

/*  Set up the font names and option buttons. Should be called after a
 *  window size change.
 */
void
set_fonts()
{
	int x1, y1, x2, y2;
	int i, x, y, width, wwidth, wheight, fdiff;
	char s[4];
	extern font_t *boldfont;

	wn_get_window_size(wn,&wwidth,&wheight);
	fdiff = FONTHEIGHT(boldfont) - FBSIZE - 1;
	width = wwidth - RVWIDTH - FALEFT - 170;
	x = FALEFT + 10;
	y = FATOP + 3;

	wtext(x, y, "Font:", boldfont,(struct wrectst *)NULL);
	showbs(fbpos,NFONTS,curfont);
	for (i = 0; i < NFONTS; i++) {
		y += 16;
		sr_box(fname[i],x + 20, y, width - x - 20,FONTHEIGHT(plainfont),1);
	}

	x = width;
	y = FATOP + 6;
	wtext(x, y, "Submenu discipline:", boldfont, (struct wrectst *)NULL);
	y += LDEPTH + 3;
	for (i = 0; i < NOPTS; i++) {
		optpos[i].fb_x = x  + i * 90;
		optpos[i].fb_y = y + fdiff;
	}
	x += 20;
	wtext(x, y, "Press", boldfont, (struct wrectst *)NULL);
	x += 90;
	wtext(x, y, "Drag", boldfont, (struct wrectst *)NULL);
	showbs(optpos,NOPTS,curopt);

	x = width;
	y += LDEPTH;
	for (i = 0; i < NSCALE; i++) {
		sclpos[i].fb_x = x  + i * 90;
		sclpos[i].fb_y = y + fdiff;
	}
	x += 20;
	wtext(x, y, "Resize", boldfont, (struct wrectst *)NULL);
	x += 90;
	wtext(x, y, "Fixed", boldfont, (struct wrectst *)NULL);
	showbs(sclpos,NSCALE,curscl);

	x = width;
	y += LDEPTH;
	for (i = 0; i < NCREL; i++) {
		crelpos[i].fb_x = x  + i * 90;
		crelpos[i].fb_y = y + fdiff;
	}
	x += 20;
	wtext(x, y, "Static", boldfont, (struct wrectst *)NULL);
	x += 90;
	wtext(x, y, "C/Rel", boldfont, (struct wrectst *)NULL);
	showbs(crelpos,NCREL,curcrel);

	x = width;
	y += 22;
	wtext(x, y, "Line colour",boldfont, (struct wrectst *)NULL);
	y += LDEPTH + 3;
	for (i = 0; i < NLCLS; i++) {
		lcpos[i].fb_x = x  + i * 90;
		lcpos[i].fb_y = y + fdiff;
	}
	x += 20;
	wtext(x, y, "F/G", boldfont, (struct wrectst *)NULL);
	x += 90;
	wtext(x, y, "B/G", boldfont, (struct wrectst *)NULL);
	showbs(lcpos,NLCLS,curlc);

	x = wwidth - RVWIDTH;
	y = MATOP - LDEPTH - 5;
	wtext(x, y, "Layer:", boldfont, (struct wrectst *)NULL);
	x += 80;
	for (i = 0; i < NLAYER; i++) {
		sprintf(s,"%1d",i);
		wtext(x - 13, y, s, boldfont, (struct wrectst *)NULL);
		layrpos[i].fb_x = x;
		layrpos[i].fb_y = y + fdiff;
		x += 40;
	}
	showbs(layrpos,NLAYER,curlayer());

	/* Draw a line along the top.
	 */
	x1 = 0;
	x2 = wwidth;
	y1 = y2 = FATOP - 1;
	wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
	/*  We delimit the main editing area here for want of somewhere
	 *  better. Draw a thick line across the whole window and one down
	 *  the left hand side.
	 */
	y1 = y2 = MATOP - 1;
	wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
	y1 = y2 = MATOP;
	wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
	y2 = wheight;
	x1 = x2 = MALEFT - 1;
	wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
	x1 = x2 = MALEFT;
	wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
}

/*  Redraw the rv layer radio buttons.
 */
void
showlrb()
{
	showbs(layrpos,NLAYER,curlayer());
}

/*  Read in font i. -1 is returned if successful, otherwise
 *  0 is returned.
 */
int
newfont(i)
int i;
{
	font_t *f;
	int oldcnum;

	oldcnum = setcp(CP_WAIT);
	if (strlen(fname[i]) == 0) {
		free(fname[i]);
		if (i < 2) {
			fname[i] = malloc(sizeof(STFNAME) + 1);
			strcpy(fname[i],STFNAME);
			f = boldfont;
		} else {
			fname[i] = malloc(sizeof(SYFNAME) + 1);
			strcpy(fname[i],SYFNAME);
			f = wn_get_sysfont();
		}
		srredraw(fname[i]);
	} else if (strcmp(fname[i],STFNAME) == 0)
		f = boldfont;
	else if (strcmp(fname[i],SYFNAME) == 0)
		f = wn_get_sysfont();
	else if ((f = getfont(fname[i])) == NULL) {
		mprintf("### can't open font '%s' ###",fname[i]);
		setcp(oldcnum);
		return(-1);
	}
	if (font[i] != boldfont)
		free((char *)font[i]);
	font[i] = f;
	bmshow(bmtop());
	setcp(oldcnum);
	return(0);
}

font_t *
getfont(name)
char *name;
{
	return(wn_open_font(name));
}

/*  draw the n buttons in the array and highlight the current one.
 */
static
void
showbs(rbp,n,current)
struct fbpst *rbp;
int n, current;
{
	int i;

	Mpushsu(wn);
	for (i = 0; i < n; i++) {
		wn_set_area(wn,rbp[i].fb_x,rbp[i].fb_y,
				FBSIZE,FBSIZE,WN_FG(wn));
		wn_set_area(wn,rbp[i].fb_x + 1,rbp[i].fb_y + 1,
				FBSIZE - 2,FBSIZE - 2,WN_BG(wn));
		if (i == current) {
		wn_set_area(wn,rbp[i].fb_x + 3,rbp[i].fb_y + 3,
				FBSIZE - 6,FBSIZE - 6,WN_FG(wn));

		}
	}
	Mpopsu(wn);
}

/*  Return 1 if the coordinates are within a selectable radio button.
 *  Otherwise return 0. This is only for the benefit of the mouse hole
 *  captions.
 */
int
inrbutton(x,y)
register int x, y;
{
	register i;

	if (curfont < 0)
		return(0);
	for (i = 0; i < NFONTS; i++) {
		if ((x >= fbpos[i].fb_x) &&
		    (x <= fbpos[i].fb_x + FBSIZE) &&
		    (y >= fbpos[i].fb_y) &&
		    (y <= fbpos[i].fb_y + FBSIZE))
			return(1);
	}
	for (i = 0; i < NLAYER; i++) {
		if ((x >= layrpos[i].fb_x) &&
		    (x <= layrpos[i].fb_x + FBSIZE) &&
		    (y >= layrpos[i].fb_y) &&
		    (y <= layrpos[i].fb_y + FBSIZE))
			return(1);
	}
	for (i = 0; i < NOPTS; i++) {
		if ((x >= optpos[i].fb_x) &&
		    (x <= optpos[i].fb_x + FBSIZE) &&
		    (y >= optpos[i].fb_y) &&
		    (y <= optpos[i].fb_y + FBSIZE))
			return(1);;
	}
	for (i = 0; i < NSCALE; i++) {
		if ((x >= sclpos[i].fb_x) &&
		    (x <= sclpos[i].fb_x + FBSIZE) &&
		    (y >= sclpos[i].fb_y) &&
		    (y <= sclpos[i].fb_y + FBSIZE))
			return(1);;
	}
	for (i = 0; i < NCREL; i++) {
		if ((x >= crelpos[i].fb_x) &&
		    (x <= crelpos[i].fb_x + FBSIZE) &&
		    (y >= crelpos[i].fb_y) &&
		    (y <= crelpos[i].fb_y + FBSIZE))
			return(1);;
	}
	for (i = 0; i < NLCLS; i++) {
		if ((x >= lcpos[i].fb_x) &&
		    (x <= lcpos[i].fb_x + FBSIZE) &&
		    (y >= lcpos[i].fb_y) &&
		    (y <= lcpos[i].fb_y + FBSIZE))
			return(1);
	}
	return(0);
}

/*  If the coordinates are within a button make the appropriate selection.
 */
void
selstyle(x,y)
int x, y;
{
	int i;

	for (i = 0; i < NFONTS; i++) {
		if ((x >= fbpos[i].fb_x) &&
		    (x <= fbpos[i].fb_x + FBSIZE) &&
		    (y >= fbpos[i].fb_y) &&
		    (y <= fbpos[i].fb_y + FBSIZE))
			break;
	}
	if ((i != NFONTS) && (curfont != -1)) {
		if (curfont != i) {
			curfont = i;
			setcapfont(i);
			setbutstyle(getstyle());
			showbs(fbpos,NFONTS,curfont);
		}
		return;
	}
	for (i = 0; i < NLAYER; i++) {
		if ((x >= layrpos[i].fb_x) &&
		    (x <= layrpos[i].fb_x + FBSIZE) &&
		    (y >= layrpos[i].fb_y) &&
		    (y <= layrpos[i].fb_y + FBSIZE))
			break;
	}
	if (i != NLAYER) {
		newlayer(i);
		showbs(layrpos,NLAYER,curlayer());
		return;
	}

	for (i = 0; i < NOPTS; i++) {
		if ((x >= optpos[i].fb_x) &&
		    (x <= optpos[i].fb_x + FBSIZE) &&
		    (y >= optpos[i].fb_y) &&
		    (y <= optpos[i].fb_y + FBSIZE))
			break;
	}
	if ((i != NOPTS) && (curopt != -1)) {
		if (curopt != i) {
			curopt = i; 
			setbutstyle(getstyle());
			showbs(optpos,NOPTS,curopt);
		}
		return;
	}

	for (i = 0; i < NSCALE; i++) {
		if ((x >= sclpos[i].fb_x) &&
		    (x <= sclpos[i].fb_x + FBSIZE) &&
		    (y >= sclpos[i].fb_y) &&
		    (y <= sclpos[i].fb_y + FBSIZE))
			break;
	}
	if ((i != NSCALE) && (curscl != -1)) {
		if (curscl != i) {
			curscl = i; 
			setbutstyle(getstyle());
			showbs(sclpos,NSCALE,curscl);
		}
		return;
	}

	for (i = 0; i < NCREL; i++) {
		if ((x >= crelpos[i].fb_x) &&
		    (x <= crelpos[i].fb_x + FBSIZE) &&
		    (y >= crelpos[i].fb_y) &&
		    (y <= crelpos[i].fb_y + FBSIZE))
			break;
	}
	if ((i != NCREL) && (curcrel != -1)) {
		if (curcrel != i) {
			curcrel = i; 
			setbutstyle(getstyle());
			showbs(crelpos,NSCALE,curcrel);
		}
		return;
	}

	for (i = 0; i < NLCLS; i++) {
		if ((x >= lcpos[i].fb_x) &&
		    (x <= lcpos[i].fb_x + FBSIZE) &&
		    (y >= lcpos[i].fb_y) &&
		    (y <= lcpos[i].fb_y + FBSIZE))
			break;
	}
	if ((i != NLCLS) && (curlc != -1)) {
		if (curlc != i) {
			curlc = i; 
			setbutstyle(getstyle());
			showbs(lcpos,NLCLS,curlc);
		}
		return;
	}
}

/*  Construct as style field from the current font and options
 */
static int
getstyle()
{
	int style;

	if ((curfont < 0) || (curopt < 0) || (curlc < 0) ||
	    (curscl < 0) || (curcrel < 0))
		return(-1);	/* should never happen */
	style = curfont;
	if (curopt)
		style |= SM_POPUP;
	if (curlc)
		style |= SM_BGLINE;
	if (curscl)
		style |= SM_NOSCALE;
	if (curcrel)
		style |= SM_CREL;
	return(style);
}

/*  Set the current font and style
 */
void
setstyle(n)
int n;
{

	if (n >= 0) {
		curfont = n & SM_FONT;
		curopt = n & SM_POPUP ? 1 : 0;
		curlc = n & SM_BGLINE ? 1 : 0;
		curscl = n & SM_NOSCALE ? 1 : 0;
		curcrel = n & SM_CREL ? 1 : 0;
	} else {
		curfont = -1;
		curopt = -1;
		curlc = -1;
		curscl = -1;
		curcrel = -1;
	}
	showbs(fbpos,NFONTS,curfont);
	showbs(optpos,NOPTS,curopt);
	showbs(sclpos,NSCALE,curscl);
	showbs(lcpos,NLCLS,curlc);
	showbs(crelpos,NCREL,curcrel);
}
