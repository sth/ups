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
#include "bmenu.h"
#include "cursors.h"
#include "med3.h"

#ifndef lint
static char sccsid[] = "%W% (UKC) %G%";
#endif  /* lint */

extern int wn;				/* descriptor of the current window */
extern struct wrectst mwr;		/* main area */
static struct butst *bmenu = NULL;	/* the menu itself */
static int shortl = 0;		/* show menus with lines shortened */
static struct bmstk *stack = NULL;	/* stack of open menus */

/*  Used for stacking open menus
 */
struct bmstk {
	struct butst *bs_bm;
	struct bmstk *bs_next;
};

/*  routines for manipulating the stack of open menus
 */

static void bspush PROTO((struct butst *bm));
static void showstack PROTO((struct bmstk *bs));
static struct butst * rfindb PROTO((struct butst *bm, int x, int y));
static struct butst * rfindedge PROTO((struct butst *bm, int x, int y, int w));

static 
void 
bspush(bm)
struct butst *bm;
{
	struct bmstk *bs;

	bs = (struct bmstk *)malloc(sizeof(struct bmstk));
	bs->bs_bm = bm;
	bs->bs_next = stack;
	stack = bs;
}

void
bspop()
{
	struct butst *bm;
	struct bmstk *bs;

	if (stack == NULL)
		return;
	bs = stack;
	stack = bs->bs_next;
	bm = bs->bs_bm;
	bm->but_a->but_flags &= ~BM_OPEN;
	free((char *)bs);
	return;
}

void
bsclear()
{
	while (stack != NULL)
		bspop();
}

struct butst *
bmtop()
{
	if (stack == NULL)
		return(bmenu);
	return(stack->bs_bm);
}

void
bmroot(bm)
struct butst *bm;
{
	bmenu = bm;
}

/*  redraw the whole menu
 */
void
bmredraw()
{
	Mpushsu(wn);
	blankma();
	showstack(stack);
	Mpopsu(wn);
}

static
void
showstack(bs)
struct bmstk *bs;
{
	if (bs == NULL)
		bmshow(bmenu);
	else {
		showstack(bs->bs_next);
		bmshow(bs->bs_bm);
	}
}

/*  Return the visible button, if any, containing the coordinates.
 *  If the coordinates are not in a button NULL is returned.
 */
struct butst *
findbut(x,y)
int x, y;
{
	struct butst *rfindb();
	int popped = 0;
	struct butst *b;

	while ((b = rfindb(bmtop(),x,y)) == NULL) {
		if (bmtop() == bmenu)
			return(NULL);
		bspop();
		popped = 1;
	}
	return(popped ? NULL : b);
}

/*  Recursive button search in bmenu bm.
 */
static struct butst *
rfindb(bm,x,y)
struct butst *bm;
int x, y;
{
	register struct butst *b;

	for (b = bm; b != NULL; b = b->but_b)
		if ((x >= b->but_left) &&
		    (x <= b->but_left + b->but_width) &&
		    (y >= b->but_top) &&
		    (y <= b->but_top + b->but_height)) {
			if (ISOPEN(b))
				return(rfindb(b->but_u,x,y));
			else
				return(b);
		}
	return(NULL);
}

/*  Find the button whose edge is within w of the coordinates. For divisible
 *  this will be the earliest edge. For other buttons it can be eny edge.
 */
struct butst *
findedge(x,y,w)
int x, y, w;
{
	struct butst *rfindedge();

	return(rfindedge(bmtop(),x,y,w));
}

/*  Recursively search the bmenu trying to find the largest whose edge is
 *  within a distance of w from the coordinates.
 */
static struct butst *
rfindedge(bm,x,y,w)
struct butst *bm;
int x, y, w;
{
	register struct butst *b;
	int left, right, top, bottom;

	left = bm->but_left;
	top = bm->but_top;
	right = left + bm->but_width;
	bottom = top + bm->but_height;
	if (bm->but_flags & BM_NDIV) {
		if (((x <= left + w) || (y <= top + w)) && (bm->but_a == NULL))
			return(NULL);
		if ((x > left + w) && (x < right - w) &&
		    (y > top + w) && (y < bottom - w))
			if (ISOPEN(bm))
				return(rfindedge(bm->but_u,x,y,w));
			else
				return(NULL);
		if ((x >= left - w) && (x <= right + w) &&
		    (y >= top - w) && (y <= bottom + w))
			return(bm);
		return(NULL);
	}
	if (bm->but_flags & BM_HOR) {
		if ((y < top + w) || (y > bottom - w) || (x < left - w))
			return(NULL);
		for (b = bm; b != NULL; b = b->but_b) {
			if (x <= left + w)
				return(b);
			left += b->but_width;
			if (x < left - w)
				if (ISOPEN(b))
					return(rfindedge(b->but_u,x,y,w));
				else
					return(NULL);
		}
	}
	if (bm->but_flags & BM_VER) {
		if ((x < left + w) || (x > right - w) || (y < top - w))
			return(NULL);
		for (b = bm; b != NULL; b = b->but_b) {
			if (y <= top + w)
				return(b);
			top += b->but_height;
			if (y < top - w)
				if (ISOPEN(b))
					return(rfindedge(b->but_u,x,y,w));
				else
					return(NULL);
		}
	}
	return(NULL);
}

#define TOP	1
#define LEFT	2
#define	RIGHT	3
#define BOTTOM	4

/*  Attempt to pick up and move a button edge.
 */
int
bmmove()
{
	int x, y, w;
	int xstrt=0, ystrt=0, lastx, lasty;
	int mn=0, mx=0, d, lastbutton, btns;
	int x1=0, y1=0, x2=0, y2=0;
	struct butst *b, *lastb;
	int side=0;

	setcp(CP_MOVE);
	lastb = NULL;
	w = 3;
	for (lastbutton = 0;;lastbutton = btns) {
		if ((btns = getpuck(&x,&y)) & B_RIGHT)
			return(-1);
		if (y >= mwr.wr_y)
			mhcaps("pick line","select","abort");
		else if (inmmain(x,y))
			mhcaps("do","select","abort");
		else if (inrvals(x,y) || inrbutton(x,y))
			mhcaps("set","select","abort");
		else
			mhcaps("abort","select","abort");
		if (btns & B_LEFT) {
			if (y < mwr.wr_y) {
				ungetpuck(btns,x,y);
				return(-1);
			}
			if (lastb != NULL) {
				b = lastb;
				break;
			}
			if (!(lastbutton & B_LEFT)) {
				if ((b = findbut(x,y)) != NULL) {
					if (b->but_u != NULL)
						bmopen(b);
					else
						bell(0);
				} else {
					bmreset(bmtop(),0);
					bmredraw();
				}
			}
			continue;
		}
		b = findedge(x,y,w);
		if (b == lastb)
			continue;
		if (lastb != NULL) {
			wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
		}
		if (b != NULL) {
			if (b->but_flags & BM_HOR) {
				x1 = x2 = b->but_left - 1;
				y1 = b->but_top + 3;
				y2 = b->but_top + b->but_height - 4;
				showcoords(x1 - mwr.wr_x,0);
				side = LEFT;
			}
			if (b->but_flags & BM_VER) {
				y1 = y2 = b->but_top - 1;
				x1 = b->but_left + 3;
				x2 = b->but_left + b->but_width - 4;
				showcoords(0,y1 - mwr.wr_y);
				side = TOP;
			}
			if (b->but_flags & BM_NDIV) {
				if (x <= b->but_left + w) {
					x1 = x2 = b->but_left-1;
					y1 = b->but_top + 3;
					y2 = b->but_top+b->but_height-3;
					showcoords(x1 - mwr.wr_x,0);
					side = LEFT;
				} else if (y <= b->but_top + w) {
					y1 = y2 = b->but_top-1;
					x1 = b->but_left + 3;
					x2 = b->but_left+b->but_width-3;
					showcoords(0,y1 - mwr.wr_y);
					side = TOP;
				} else if (x >= b->but_left+b->but_width-w) {
					x1 = x2 = b->but_left +
								b->but_width-1;
					y1 = b->but_top + 3;
					y2 = b->but_top+b->but_height-3;
					showcoords(x1 - mwr.wr_x,0);
					side = RIGHT;
				} else {
					y1 = y2 = b->but_top +
							b->but_height-1;
					x1 = b->but_left + 3;
					x2 = b->but_left+b->but_width-3;
					showcoords(0,y1 - mwr.wr_y);
					side = BOTTOM;
				}
			}
			wn_draw_line(wn,x1,y1,x2,y2,WN_BG(wn));
		} else
			showcoords(0,0);
		lastb = b;
	}
	/*  We have selected a line to move. First find out the direction
	 *  and how far we can move it.
	 */
	if (b->but_flags & BM_HOR) {
		mn = b->but_left - b->but_a->but_width + 3;
		mx = b->but_left + b->but_width - 3;
		xstrt = b->but_left - 1;
	} else if (b->but_flags & BM_VER) {
		mn = b->but_top - b->but_a->but_height + 3;
		mx = b->but_top + b->but_height - 3;
		ystrt = b->but_top - 1;
	} else {
		/* must be indivisible
		 */
		switch (side) {
		    case TOP :
			mn = mwr.wr_y;
			mx = b->but_top + b->but_height - 3;
			ystrt = b->but_top - 1;
			break;
		    case LEFT :
			mn = mwr.wr_x;
			mx = b->but_left + b->but_width - 3;
			xstrt = b->but_left - 1;
			break;
		    case RIGHT :
			mn = b->but_left + 3;
			mx = 10000;
			xstrt = b->but_left + b->but_width - 1;
			break;
		    case BOTTOM :
			mn = b->but_top + 3;
			mx = 10000;
			ystrt = b->but_top + b->but_height - 1;
			break;
		}
	}
	lastx  = xstrt;
	lasty  = ystrt;
	wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
	for (;btns & B_LEFT; btns = getpuck(&x,&y)) {
		if ((side == LEFT) || (side == RIGHT)) {
			x = (1 - LSPEED) * xstrt + LSPEED * x + 0.5;
			if (x < mn)
				x = mn;
			if (x > mx)
				x = mx;
			if (x != lastx) {
				x1 = x2 = lastx;
				wn_invert_line(wn,x1,y1,x2,y2);
				x1 = x2 = x;
				wn_invert_line(wn,x1,y1,x2,y2);
				lastx = x;
			}
			showcoords(x - mwr.wr_x, 0);
		} else {
			y = (1 - LSPEED) * ystrt + LSPEED * y + 0.5;
			if (y < mn)
				y = mn;
			if (y > mx)
				y = mx;
			if (y != lasty) {
				y1 = y2 = lasty;
				wn_invert_line(wn,x1,y1,x2,y2);
				y1 = y2 = y;
				wn_invert_line(wn,x1,y1,x2,y2);
				lasty = y;
			}
			showcoords(0, y - mwr.wr_y);
		}
	}
	/*  We have the new position. Set the new button sizes.
	 */
	if (b->but_flags & BM_HOR) {
		d = lastx + 1 - b->but_left;
		scalebmw(b->but_u,b->but_width - d,b->but_width);
		b->but_width -= d;
		movebm(b->but_u,d,0);
		b->but_left += d;
		b = b->but_a;
		scalebmw(b->but_u,b->but_width + d,b->but_width);
		b->but_width += d;
	} else if (b->but_flags & BM_VER) {
		d = lasty + 1 - b->but_top;
		scalebmh(b->but_u,b->but_height - d,b->but_height);
		b->but_height -= d;
		movebm(b->but_u,0,d);
		b->but_top += d;
		b = b->but_a;
		scalebmh(b->but_u,b->but_height + d,b->but_height);
		b->but_height += d;
	} else {
		/* must be indivisible
		 */
		switch (side) {
		    case TOP :
			d = lasty + 1 - b->but_top;
			scalebmh(b->but_u,b->but_height - d,b->but_height);
			b->but_height -= d;
			movebm(b->but_u,0,d);
			b->but_top += d;
			break;
		    case LEFT :
			d = lastx + 1 - b->but_left;
			scalebmw(b->but_u,b->but_width - d,b->but_width);
			b->but_width -= d;
			movebm(b->but_u,d,0);
			b->but_left += d;
			break;
		    case RIGHT :
			d = lastx + 1 - b->but_left - b->but_width;
			scalebmw(b->but_u,b->but_width + d,b->but_width);
			b->but_width += d;
			break;
		    case BOTTOM :
			d = lasty + 1 - b->but_top - b->but_height;
			scalebmh(b->but_u,b->but_height + d,b->but_height);
			b->but_height += d;
			break;
		}
	}
	return(0);
}

/*  Open the button and display the revealed submenu.
 */
void
bmopen(bm)
struct butst *bm;
{
	struct butst *b;

	if (bm->but_u == NULL)
		return;
	b = bm->but_u;
	if (b->but_flags & BM_NDIV) {
		srsel(-1,-1);
		cap_nsel();
		bspush(b);
	} else {
		cap_off(bm->but_caption,0);
		bm->but_flags |= BM_OPEN;
	}
	bmshow(b);
}

void
bmshowshort()
{
	shortl = 3;
}

void
bmshownormal()
{
	shortl = 0;
}

/*  display the bmenu in the window. No initial blanking is done.
 */
void
bmshow(bm)
struct butst *bm;
{
	register struct butst *b;
	int x1, y1, x2, y2;

	if (bm == NULL)
		return;
	if ((bm->but_flags & BM_NDIV) && (bm->but_a != NULL))
		cap_nsel();

	if (bm->but_flags & BM_HOR) {
		y1 = bm->but_top + shortl;
		y2 = bm->but_top + bm->but_height - 1 - shortl;
		for (b = bm; b != NULL; b = b->but_b) {
			if (b->but_b != NULL) {
				x1 = b->but_left + b->but_width - 1;
				x2 = x1;
				wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
			}
			if (ISOPEN(b))
				bmshow(b->but_u);
			else {
				sr_box(b->but_caption,b->but_left,b->but_top, 
					b->but_width-1,b->but_height - 1,0);
				cap_on(b->but_caption,1);
			}
		}
	} else if (bm->but_flags & BM_VER) {
		x1 = bm->but_left + shortl;
		x2 = bm->but_left + bm->but_width - 1 - shortl;
		for (b = bm; b != NULL; b = b->but_b) {
			if (b->but_b != NULL) {
				y1 = b->but_top + b->but_height - 1;
				y2 = y1;
				wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
			}
			if (ISOPEN(b))
				bmshow(b->but_u);
			else {
				sr_box(b->but_caption,b->but_left,b->but_top,
					b->but_width-1,b->but_height - 1,0);
				cap_on(b->but_caption,1);
			}
		}
	} else {
		y1 = bm->but_top + shortl;
		y2 = bm->but_top + bm->but_height - 1 - shortl;
		x1 = bm->but_left + bm->but_width - 1;
		x2 = x1;
		wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
		if ((bm->but_flags & BM_NDIV) && (bm->but_a != NULL)) {
			x1 = bm->but_left - 1;
			x2 = x1;
			wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
		}
		x1 = bm->but_left + shortl;
		x2 = bm->but_left + bm->but_width - 1 - shortl;
		y1 = bm->but_top + bm->but_height - 1;
		y2 = y1;
		wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
		if ((bm->but_flags & BM_NDIV) && (bm->but_a != NULL)) {
			y1 = bm->but_top - 1;
			y2 = y1;
			wn_draw_line(wn,x1,y1,x2,y2,WN_FG(wn));
		}
		if (ISOPEN(bm))
			bmshow(bm->but_u);
		else {
			sr_box(bm->but_caption,bm->but_left,bm->but_top,
				bm->but_width - 1, bm->but_height - 1,0);
			cap_on(bm->but_caption,1);
		}
	}
}

/*  Check the integrity of the bmenu and abort if anything is found to be
 *  wrong
 */
void
bmcheck(bm)
struct butst *bm;
{
	register struct butst *b;
	int left, top, width, height;

	if (bm == NULL)
		return;

	if ((bm->but_flags & BM_FIRST) == 0)
		abort();
	if (bm->but_a == NULL) {
		if ((bm->but_flags & BM_NDIV) == 0)
			abort();
		if (bmenu != bm)
			abort();
	} else if (bm->but_a->but_u != bm)
		abort();
	bmcheck(bm->but_u);

	if (bm->but_flags & BM_HOR) {
		if (bm->but_b == NULL)
			abort();
		left = bm->but_left + bm->but_width;;
		top = bm->but_top;
		height = bm->but_height;
		for (b = bm->but_b; b != NULL; b = b->but_b) {
			if ((b->but_height != height) ||
			    (b->but_top != top) ||
			    (b->but_left != left))
				abort();
			left += b->but_width;
			if (b->but_flags & BM_FIRST)
				abort();
			if (b->but_flags & BM_NDIV)
				abort();
			if (b->but_a->but_b != b)
				abort();
			if (b->but_flags & BM_VER)
				abort();
			if ((b->but_flags & BM_HOR) == 0)
				abort();
			bmcheck(b->but_u);
		}
	} else if (bm->but_flags & BM_VER) {
		if (bm->but_b == NULL)
			abort();
		top = bm->but_top + bm->but_height;;
		left = bm->but_left;
		width = bm->but_width;
		for (b = bm->but_b; b != NULL; b = b->but_b) {
			if ((b->but_width != width) ||
			    (b->but_top != top) ||
			    (b->but_left != left))
				abort();
			top += b->but_height;
			if (b->but_flags & BM_FIRST)
				abort();
			if (b->but_flags & BM_NDIV)
				abort();
			if (b->but_a->but_b != b)
				abort();
			if (b->but_flags & BM_HOR)
				abort();
			if ((b->but_flags & BM_VER) == 0)
				abort();
			bmcheck(b->but_u);
		}
	} else {
		if (bm->but_b != NULL)
			abort();
	}
}
