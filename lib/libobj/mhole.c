/* mhole.c - mouse hole implementation over wn */

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


/* @(#)mhole.c	1.13 8/11/92 (UKC) */
char shared_mhole_c_rcsid[] = "$Id$";


#include <local/wn.h>
#include <string.h>

#include <local/ukcprog.h>

#include "mhole.h"

extern int errno;

static window_t Mh_wn = -1;		/* file descriptor of the current window */

static short Mh_x, Mh_y;	/* mouse hole top-left corner */

static short Mh_width, Mh_height;	/* width and height of mousehole */

/*  Number of buttons - will be overwritten with 4 on a Perq with
 *  the high resolution 4 button tablet
 */
static int Num_buttons = 3;

/*  Structure describing the position of a mouse hole caption.
 *  The position is that of the top-centre of the caption.
 */
struct mhcst {
	short mh_capnum;
	short mh_x;
	short mh_y;
} Bpos[4] = { {0}, {1}, {2}, {3} };

/*  We do not record absolute positions for mousehole captions, as
 *  the size of the mousehole depends on the font size. Instead we
 *  record value with this structure.
 *  The actual value is calculated at runtime as
 *
 *	(ex_mul * Sysfont_{width,height}) / 4 + ex_add
 */
struct expst {
	short ex_mul;
	short ex_add;
};

static void draw_mousehole_boxes PROTO((void));
static void put_mhcap PROTO((const char **caps, int cnum, int x, int y));
static int calcpos PROTO((struct expst exp, int scale));

/*  Given a struct expst exp and a scale, return a size (see comment
 *  on struct expst above.
 */
static int
calcpos(exp, scale)
struct expst exp;
int scale;
{
	return (exp.ex_mul * scale) / 4 + exp.ex_add;
}

/*  Structure for recording a caption position - the x and y coords,
 *  stored as expst structures (above).
 */
struct pbposst {
	struct expst pb_x;
	struct expst pb_y;
};

/*  caption positions for physical buttons for the 3 button puck
 *  and the sun mouse
 */
static struct pbposst pbpos[4] = {
	 { { 1, -8 },	{ 2, -9 } },		/* Left button */
	 { { 2,  0 },	{ 0,  4 } },		/* Middle button */
	 { { 3,  8 },	{ 2, -9 } },		/* Right button */
	 { { 0, -1 }, 	{ 0, -1 } },		/* dummy */
};

/*  Determine the mouse hole position and draw the box. Should be called after
 *  a window size change. Box is drawn at x,y.
 */
void
draw_mousehole(wn, x, y, width, height)
int wn;
int x, y, width, height;
{
	int pb;

	for (pb = 0; pb < Num_buttons; pb++) {
		Bpos[pb].mh_x = calcpos(pbpos[pb].pb_x, width);
		Bpos[pb].mh_y = calcpos(pbpos[pb].pb_y, height);
	}

	Mh_width = width;
	Mh_height = height;
	Mh_wn = wn;
	Mh_x = x;
	Mh_y = y;
	put_mhcap((const char **)NULL, 0, 0, 0);
	wn_updating_off(Mh_wn);
	draw_mousehole_boxes();
	wn_updating_on(Mh_wn);
}

/*  Various #defines for the sizes of the button rectangles and
 *  other uninteresting stuff
 */
#define B_TOTAL_WIDTH 36
#define B_TOTAL_DEPTH 40

#define N_BOXES 3
#define B_WIDTH (B_TOTAL_WIDTH / N_BOXES)

#define B_DEPTH B_TOTAL_DEPTH

#define B_HBORDER 3
#define B_VBORDER 6

#define D_RADIUS (B_TOTAL_WIDTH / 4)

/*  Draw the mousehole with the given captions.
 */
static void
draw_mousehole_boxes()
{
	int l;
	int bx, by, ystart;

	if (Num_buttons == 3) {
		bx = Mh_x + (Mh_width - B_TOTAL_WIDTH) / 2 + B_HBORDER;
		ystart = calcpos(pbpos[1].pb_y, Mh_height) +
					wn_get_sysfont()->ft_height + 6;
		by = Mh_y + ystart + ((Mh_height - ystart) - B_DEPTH) / 2;
		for (l = 0; l < 3; l++) {
			wn_box_round(Mh_wn, bx, by,
				B_WIDTH - B_HBORDER*2,
				B_DEPTH - B_VBORDER*2, WN_FG(Mh_wn));
			bx += B_WIDTH;
		}
	}
}

/*  Display a mousehole caption s at x, y, relative to the mousehole.
 *  Clip to the mousehole size, and centre about x
 */
static void
put_mhcap(caps, cnum, x, y)
const char **caps;
int cnum, x, y;
{
	struct boxst {
		short box_x;
		short box_y;
		short box_w;
		short box_h;
	};
	static struct boxst boxtab[4];
	register struct boxst *b;
	char buf[30];
	int w, h;
	
	if (caps == NULL) {
		for (b = boxtab; b < boxtab + 4; b++)
			b->box_w = 0;
		return;
	}
	b = boxtab + cnum;
	if (b->box_w != 0)
		wn_set_area(Mh_wn, b->box_x, b->box_y, b->box_w, b->box_h, WN_BG(Mh_wn));

	if (caps[cnum] == NULL) {
		b->box_w = 0;
		return;
	}

	(void) strncpy(buf, caps[cnum], sizeof(buf));

	w = ((x >= Mh_width/2) ? Mh_width - x : x) * 2;
	h = wn_get_sysfont()->ft_height;

	buf[sizeof(buf) - 1] = '\0';
	buf[wn_strpos(buf, w, (font_t *)NULL, FALSE)] = '\0';
	w = wn_strwidth(buf, (font_t *)NULL);

	b->box_x = Mh_x + x - w / 2;
	b->box_y = Mh_y + y;
	b->box_w = w;
	b->box_h = h;
	wn_tputs(Mh_wn, buf, b->box_x, b->box_y);
}

void
mhdraw(caps)
const char **caps;
{
	int pb;
	if ( Mh_wn >= 0)
	{

		wn_updating_off(Mh_wn);
		for (pb = 0; pb < Num_buttons; pb++)
			put_mhcap(caps, Bpos[pb].mh_capnum, Bpos[pb].mh_x, Bpos[pb].mh_y);
		wn_updating_on(Mh_wn);
	}
}
