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

extern struct butst *bmtop();	/* the top displayed menu */

extern struct butst *curb;	/* the current button */
extern struct wrectst mwr;	/* main editing area */
extern int wn;		/* file descriptor of current window */

struct rectst {
	struct butst *re_first;
	struct butst *re_last;
	short re_left;
	short re_top;
	short re_width;
	short re_height;
};

static struct butst *bmndiv PROTO((int left, int top, int width, int height));
static int bmrect PROTO((struct butst *bm, struct rectst *r, int left, int top,
		  int width, int height));
static int getrect PROTO((struct rectst *rp));

/*  Find the largest bmenu rectangle enclosed within the rectangle described
 *  by the arguments. If there is no included rectangle -1 will be returned
 *  if there is one  will be returned.
 */
static 
int
bmrect(bm,r,left,top,width,height)
struct butst *bm;
struct rectst *r;
int left, top, width, height;
{
	register struct butst *b1, *b2;
	int right, bottom;

	if (bm == NULL)
		return(-1);

	right = left + width;
	bottom = top + height;
	if (bm->but_flags & BM_HOR) {
		if ((bm->but_top>bottom) || (bm->but_top+bm->but_height<top))
			return(-1);
		if ((top<bm->but_top) && (bottom>bm->but_top+bm->but_height)) {
			/*  Try and find a horizontal sequence.
			 */
			for (b1 = bm; b1 != NULL; b1 = b1->but_b)
				if (b1->but_left >= left)
					break;
			for (b2 = b1; b2 != NULL; b2 = b2->but_b)
				if (b2->but_left + b2->but_width > right)
					break;
			if (b1 != b2) {
				r->re_first = b1;
				r->re_last = b2;
				r->re_top = b1->but_top;
				r->re_height = b1->but_height;
				r->re_left = b1->but_left;
				r->re_width = 0;
				for (; b1 != b2; b1 = b1->but_b)
					r->re_width += b1->but_width;
				return(0);
			}
		}
	} else {
		if ((bm->but_left>right) ||(bm->but_left + bm->but_width<left))
			return(-1);
		if ((left<bm->but_left) && (right>bm->but_left+bm->but_width)){
			/*  Try and find a vertical sequence.
			 */
			for (b1 = bm; b1 != NULL; b1 = b1->but_b)
				if (b1->but_top >= top)
					break;
			for (b2 = b1; b2 != NULL; b2 = b2->but_b)
				if (b2->but_top + b2->but_height > bottom)
					break;
			if (b1 != b2) {
				r->re_first = b1;
				r->re_last = b2;
				r->re_left = b1->but_left;
				r->re_width = b1->but_width;
				r->re_top = b1->but_top;
				r->re_height = 0;
				for (; b1 != b2; b1 = b1->but_b)
					r->re_height += b1->but_height;
				return(0);
			}
		}
	}
	/*  There is a partial overlap at this level. Try and
	 *  find the best rectangle from  a submenu.
	 */
	for (b1 = bm; b1 != NULL; b1 = b1->but_b) {
		if (ISOPEN(b1))
			if (bmrect(b1->but_u,r,left,top,width,height) == 0)
				return(0);
	}
	return(-1);
}

/*  xor a grey pattern into the rectangle
 */
static
void
greyrect(left,top,width,height)
{
	wn_shade_area(wn,left,top,width,height,WN_GREY2,R_XOR);
}

/*  draw a box and remove the previous one. If x2 and y2 are both zero no box is drawn
 */
static
void
drawbox(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
	static lastx = 0, lasty = 0;

	if ((x2 == lastx) && (y2 == lasty))
		return;

	Mpushsu(wn);
	if ((lastx != 0) || (lasty != 0)) {
		wn_invert_line(wn, x1, y1, x1, lasty);
		wn_invert_line(wn, lastx, y1, lastx, lasty);
		wn_invert_line(wn, x1, y1, lastx, y1);
		wn_invert_line(wn, x1, lasty, lastx, lasty);
	}
	if ((x2 != 0) && (y2 != 0)) {
		wn_invert_line(wn, x1, y1, x1, y2);
		wn_invert_line(wn, x2, y1, x2, y2);
		wn_invert_line(wn, x1, y1, x2, y1);
		wn_invert_line(wn, x1, y2, x2, y2);
	}
	lastx = x2;
	lasty = y2;
	Mpopsu(wn);
}


/*  Use a rubber box to let the user define a rectangle within the bmenu.
 *  0 is returned if a rectangle is defined.
 * -1 is returned if aborted.
 *  1 is returned if no box is defined. In this case rp->re_left and
 *    rp->re_top will be the puck coordinates on button release.
 */
static
int
getrect(rp)
struct rectst *rp;
{
	int lastx, lasty, left, top, width, height;
	struct rectst rec, lastrec;
	int rv, lastrv;
	int x, y, btns;

	setcp(CP_RECT);
	mhcaps("corner","select","abort");
	do {
		btns = getpuck(&x,&y);
		if (btns & B_RIGHT)
			return(-1);
	} while ((btns & B_LEFT) == 0);
	setcp(CP_PULL);
	mhcaps("drag","select","");

	lastx = x;
	lasty = y;

	lastrv = -1;
	do {
		btns = getpuck(&x,&y);
		drawbox(lastx, lasty, x, y);
		if (btns & B_RIGHT) {
			/* disable the rubber box
			 */
			drawbox(lastx, lasty, 0, 0);
			return(-1);
		}
		if (lastx < x) {
			width = x - lastx;
			left = lastx;
		} else {
			width = lastx - x;
			left = x;
		}
	
		if (lasty < y) {
			height = y - lasty;
			top = lasty;
		} else {
			height = lasty - y;
			top = y;
		}
		rv = bmrect(bmtop(),&rec,left,top,width,height);
		if ((rv < 0) && (lastrv < 0))
			continue;
		if ((rv == 0) && (lastrv == 0))
			if ((rec.re_first == lastrec.re_first) &&
			    (rec.re_last == lastrec.re_last))
				continue;
		Mpushsu(wn);
		if (lastrv == 0)
			greyrect(lastrec.re_left,lastrec.re_top,
				lastrec.re_width-1,lastrec.re_height-1);
		if (rv == 0) {
			greyrect(rec.re_left,rec.re_top,rec.re_width-1,
							rec.re_height-1);
			lastrec = rec;
		}
		Mpopsu(wn);
		lastrv = rv;
	} while (btns & B_LEFT);

	/* disable the rubber box
	 */
	drawbox(lastx, lasty, 0, 0);
	if (rv < 0) {
		rp->re_left = x;
		rp->re_top = y;
		return(1);
	}
	*rp = rec;
	return(0);
}

/*  Attempt to select a rectangle and convert it into a free submenu
 */
void
addsubrect()
{
	struct rectst r;
	register struct butst *b, *bc;
	int rv;
	struct butst *bmndiv();

	if (curb == NULL) {
		mprintf("There is no button selected");
		return;
	}
	if (curb->but_u != NULL) {
		mprintf("The selected button already has a submenu");
		return;
	}
	bc = curb;

	while ((rv = getrect(&r)) == 1)
		;
	if (rv < 0) {
		bmreset(bmtop(),0);
		return;
	}
	/*  A valid rectangle has been selected
	 *  so convert it to a button.
	 */
	b = bmndiv(r.re_left,r.re_top,r.re_width,r.re_height);
	bc->but_u = b;
	b->but_a = bc;
	bmopen(bc);
}

/* Use a rubber box to create a free submenu.
 */
void
addsubbox()
{
	register struct butst *b, *bc;
	int x, y, btns;
	int left, top, width, height, lastx, lasty;
	struct butst *bmndiv();

	if (curb == NULL) {
		mprintf("There is no button selected");
		return;
	}
	if (curb->but_u != NULL) {
		mprintf("The selected button already has a submenu");
		return;
	}
	bc = curb;
	setcp(CP_BOX);
	mhcaps("corner","select","abort");
	do {
		btns = getpuck(&x, &y);
		showcoords(x - mwr.wr_x, y - mwr.wr_y);
		if (btns & B_RIGHT)
			return;
	} while (!(btns & B_LEFT));
	setcp(CP_PULL);

	lastx = x;
	lasty = y;
	do {
		btns = getpuck(&x,&y);
		drawbox(lastx, lasty, x, y);
		showcoords(x - mwr.wr_x, y - mwr.wr_y);
		if (btns & B_RIGHT) {
			/* disable the rubber box
			 */
			drawbox(lastx, lasty, 0, 0);
		}
	} while (btns & B_LEFT);
	/* disable the rubber box
	 */
	drawbox(lastx, lasty, 0, 0);

	if (lastx < x) {
		width = x - lastx;
		left = lastx;
	} else {
		width = lastx - x;
		left = x;
	}
	if (left < mwr.wr_x)
		left = mwr.wr_x;
	if (width < 10)
		width = 10;

	if (lasty < y) {
		height = y - lasty;
		top = lasty;
	} else {
		height = lasty - y;
		top = y;
	}
	if (top < mwr.wr_y)
		top = mwr.wr_y;
	if (height < 10)
		height = 10;

	b = bmndiv(left,top,width,height);
	b->but_flags |= BM_FREE;
	bc->but_u = b;
	b->but_a = bc;
	bmopen(bc);
}

/*  Create an indivisible menu button of the requested size and position
 */
static struct butst *
bmndiv(left,top,width,height)
{
	register struct butst *b;
	char *nullcap();

	b = (struct butst *)malloc(sizeof(struct butst));
	b->but_left = left;
	b->but_top = top;
	b->but_width = width;
	b->but_height = height;
	b->but_u = NULL;
	b->but_b = NULL;
	b->but_a = NULL;
	b->but_caption = nullcap();
	b->but_rv = rvnext();
	b->but_style = 0;
	b->but_flags = BM_NDIV | BM_FIRST;
	reg_caption(&b->but_caption,left,top,width,height,b);
	return(b);
}

/*  Attempt to select a rectangle and hide it behind an opaque button.
 */
int
rectobut()
{
	struct rectst r;
	register struct butst *b;
	int rv;
	char *nullcap();

	while ((rv = getrect(&r)) == 1)
		;
	if (rv < 0) {
		bmreset(bmtop(),0);
		return(-1);
	}
	/*  A valid rectangle has been selected
	 *  so convert it to a button.
	 */
	if (r.re_first->but_b == r.re_last) {
		/*  Just one button selected
		 */
		b = r.re_first;
		if ((b->but_flags & BM_CLEAR) == 0) {
			mprintf("Only one button selected");
			bell(0);
			bmreset(b,0);
			return(0);
		}
		b->but_flags &= ~BM_CLEAR;
	} else {
		/*  We need to create a new button
		 */
		b = (struct butst *)malloc(sizeof(struct butst));
		b->but_width = r.re_width;
		b->but_height = r.re_height;
		b->but_left = r.re_left;
		b->but_top = r.re_top;
		b->but_b = r.re_last;
		b->but_flags = 0;
		b->but_style = 0;
		if (b->but_b != NULL) {
			b->but_b->but_a->but_b = NULL;
			b->but_b->but_a = b;
			b->but_flags |= b->but_b->but_flags&(BM_HOR|BM_VER);
		}
		b->but_u = r.re_first;
		b->but_a = b->but_u->but_a;
		b->but_u->but_a = b;
		if (b->but_u->but_flags & BM_FIRST) {
			if (b->but_a != NULL)
				b->but_a->but_u = b;
			else
				bmroot(b);
			b->but_flags |= BM_FIRST;
		} else {
			b->but_a->but_b = b;
			b->but_flags |= b->but_a->but_flags&(BM_HOR|BM_VER);
		}
		b->but_u->but_flags |= BM_FIRST;
	}
	b->but_caption = nullcap();
	b->but_rv = rvnext();
	reg_caption(&b->but_caption,b->but_left,b->but_top,b->but_width-1,
							b->but_height-1,b);
	srsel(b->but_left + 1, b->but_top + 1);
	bmreset(b,0);
	return(0);
}

/*  Get a rectangle from the user and attempt to equispace the 
 *  enclosed buttons.
 */
int
equispace()
{
	struct rectst r;
	struct butst *b;
	int n, i, old, new, rv;

	while ((rv = getrect(&r)) == 1)
		;
	if (rv < 0) {
		bmreset(bmtop(),0);
		return(-1);
	}

	/* count the number of buttons enclosed.
	 */
	n = 0;
	for (b = r.re_first; b != r.re_last; b = b->but_b)
		n++;
	while ((n == 1) && (ISOPEN(r.re_first))) {
		/*  Move down and try again.
		 */
		r.re_first = r.re_first->but_u;
		r.re_last = NULL;
		n = 0;
		for (b = r.re_first; b != r.re_last; b = b->but_b)
			n++;
	}
	if (n == 1) {
		mprintf("Only one button selected");
		bmreset(b,0);
		return(0);
	}
	if (r.re_first->but_flags & BM_HOR) {
		i = 1;
		old = r.re_left;
		for (b = r.re_first; b != r.re_last; b = b->but_b) {
			movebm(b->but_u, old - b->but_left, 0);
			b->but_left = old;
			new = r.re_left + r.re_width * i++ / n;
			scalebmw(b->but_u, new - old, b->but_width);
			b->but_width = new - old;
			old = new;
		}
	} else {
		i = 1;
		old = r.re_top;
		for (b = r.re_first; b != r.re_last; b = b->but_b) {
			movebm(b->but_u, 0, old - b->but_top);
			b->but_top = old;
			new = r.re_top + r.re_height * i++ / n;
			scalebmh(b->but_u, new - old, b->but_height);
			b->but_height = new - old;
			old = new;
		}
	}
	return(0);
}

/*  Get a cursor position while echoing a rubber box. The function can be
 *  aborted by pressing button c in which case -1 is returned. If successful
 *  0 is returned.
 */
int
getbox(wr, xp, yp)
int *xp, *yp;
struct wrectst *wr;
{
	int x, y, xstrt, ystrt, lastx=0, lasty=0;
	int btns;

	btns = getpuck(&x,&y);
	do {
		while ((btns & B_LEFT) == 0) {
			if (btns & B_RIGHT)
				return(-1);
			btns = getpuck(&x,&y);
			if ((y < wr->wr_y) || (x < wr->wr_x))
				mhcaps("","select","abort");
			else
				mhcaps("locate","select","abort");
			showcoords(x - wr->wr_x,y - wr->wr_y);
		}
		if ((y < wr->wr_y) || (x < wr->wr_x)){
			/* Button pressed out of range so wait for it to be
			 * released so we can try again.
			 */
			while (btns & B_LEFT)
				btns = getpuck(&x,&y);
			continue;
		}
		mhcaps("drag","select","");
		xstrt = x;
		ystrt = y;
		lastx = x;
		lasty = y;
		wn_invert_line(wn,lastx,lasty,lastx,wr->wr_y);
		wn_invert_line(wn,lastx,lasty,wr->wr_x,lasty);
		for (;btns & B_LEFT;btns = getpuck(&x,&y)) {
			x = (1 - LSPEED) * xstrt + LSPEED * x + 0.5;
			y = (1 - LSPEED) * ystrt + LSPEED * y + 0.5;
			if (x < wr->wr_x + 5)
				x = wr->wr_x + 5;
			if (y < wr->wr_y + 5)
				y = wr->wr_y + 5;
			if ((x == lastx) && (y == lasty))
				continue;
			showcoords(x - wr->wr_x,y - wr->wr_y);
			wn_invert_line(wn,lastx,lasty,lastx,wr->wr_y);
			wn_invert_line(wn,lastx,lasty,wr->wr_x,lasty);
			wn_invert_line(wn,x,y,x,wr->wr_y);
			wn_invert_line(wn,x,y,wr->wr_x,y);
			lastx = x;
			lasty = y;
		}
		wn_invert_line(wn,lastx,lasty,lastx,wr->wr_y);
		wn_invert_line(wn,lastx,lasty,wr->wr_x,lasty);
	} while (y < wr->wr_y);
	*xp = lastx;
	*yp = lasty;
	return(0);
}
