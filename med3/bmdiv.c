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

extern struct butst *curb;	/* the current button */
extern int wn;			/* descriptor of current window */

extern struct butst *bmtop();

/*  Subdivide the current menu button. 0 is returned if successful. 1 is
 *  returned if aborted. The wr argument is the editing area within which the
 *  subdivided menu lies.
 */
int
bmdivide(wr)
struct wrectst *wr;
{
	int x1, x2, y1, y2;
	int x, y, bx, by, ax, ay;
	int xstrt=0, ystrt=0, lastx, lasty;
	int vertical, lastbutton, btns;
	int height, width;
	struct butst *bc, *b, *b1, *b2;
	char *nullcap();
	struct butst *findbut();

	mhcaps("","select","abort");
	lastx = -1;
	lasty = -1;
	for (lastbutton = 0;;lastbutton = btns) {
		if ((btns = getpuck(&x,&y)) & B_RIGHT)
			return(-1);
		showcoords(x - wr->wr_x,y - wr->wr_y);
		if ((curb != NULL) &&
		    (x >= curb->but_left) &&
		    (x <= curb->but_left + curb->but_width) &&
		    (y >= curb->but_top) &&
		    (y <= curb->but_top + curb->but_height)) {
		        mhcaps("divide","select","abort");
			bx = x - curb->but_left;
			by = y - curb->but_top;
			ax = abs(bx - curb->but_width / 2);
			ay = abs(by - curb->but_height / 2);
			if (ax * curb->but_height < ay * curb->but_width) {
				setcp(CP_DIVHOR);
				vertical = 0;
			} else {
				setcp(CP_DIVVER);
				vertical = 1;
			}
			if ((btns & B_LEFT) && !(lastbutton & B_LEFT))
				break;
		} else {
			setcp(CP_DIVIDE);
			if (y >= wr->wr_y)
				mhcaps("submenu","select","abort");
			else if (inmmain(x,y))
				mhcaps("do","select","abort");
			else if (inrvals(x,y) || inrbutton(x,y))
				mhcaps("set","select","abort");
			else
				mhcaps("abort","select","abort");
		}
		if ((btns & B_LEFT) && !(lastbutton & B_LEFT)) {
			if (y < wr->wr_y){
				ungetpuck(btns,x,y);
				return(-1);
			}
			if ((b = findbut(x,y)) != NULL) {
				if (b->but_u != NULL)
					bmopen(b);
				else {
					mprintf("no submenu");
					bell(0);
				}
			} else {
				bmreset(bmtop(),0);
				bmredraw();
			}
		}
	}
	/* Button A has been pressed in a selected menu button. We can
	 * try and insert a subdivision.
	 */
	mhcaps("drag","select","");
	if (vertical) {
		y1 = curb->but_top;
		y2 = curb->but_top + curb->but_height - 1;
		x1 = x2 = x;
		wn_invert_line(wn,x1,y1,x2,y2);
		lastx = x;
		xstrt = x;
	} else {
		x1 = curb->but_left;
		x2 = curb->but_left + curb->but_width - 1;
		y1 = y2 = y;
		wn_invert_line(wn,x1,y1,x2,y2);
		ystrt = y;
		lasty = y;
	}
	bc = curb;
	for (;btns & B_LEFT; btns = getpuck(&x,&y)) {
		if (vertical) {
			x = (1 - LSPEED) * xstrt + LSPEED * x + 0.5;
			if (x < bc->but_left + 3)
				x = bc->but_left + 3;
			if (x > bc->but_left + bc->but_width - 4)
				x = bc->but_left + bc->but_width - 4;
			if (x != lastx) {
				x1 = x2 = lastx;
				wn_invert_line(wn,x1,y1,x2,y2);
				x1 = x2 = x;
				wn_invert_line(wn,x1,y1,x2,y2);
				lastx = x;
			}
		} else {
			y = (1 - LSPEED) * ystrt + LSPEED * y + 0.5;
			if (y < bc->but_top + 3)
				y = bc->but_top + 3;
			if (y > bc->but_top + bc->but_height - 4)
				y = bc->but_top + bc->but_height - 4;
			if (y != lasty) {
				y1 = y2 = lasty;
				wn_invert_line(wn,x1,y1,x2,y2);
				y1 = y2 = y;
				wn_invert_line(wn,x1,y1,x2,y2);
				lasty = y;
			}
		}
		showcoords(x - wr->wr_x,y - wr->wr_y);
	}
	/*  Button A released again, divide the current button.
	 */
	height = bc->but_height; /* save for scaling etc. */
	width = bc->but_width;
	if (vertical && !(bc->but_flags & (BM_VER | BM_NDIV))) {
		/*  We can divide vertically.
		 */
		b = (struct butst *)malloc(sizeof(struct butst));
		/* insert b after bc
		 */
		b->but_top = bc->but_top;
		b->but_height = bc->but_height;
		b->but_left = lastx + 1;
		b->but_width = bc->but_width - lastx + bc->but_left - 1;
		bc->but_width -= b->but_width;
		bc->but_flags |= BM_HOR;
		b->but_flags = BM_HOR;
		b->but_b = bc->but_b;
		if (b->but_b != NULL)
			b->but_b->but_a = b;
		b->but_a = bc;
		bc->but_b = b;
		if (bx < width / 2) {
			b->but_rv = bc->but_rv;
			b->but_style = bc->but_style;
			bc->but_rv = rvnext();
			bc->but_style = 0;
			deregister(bc->but_caption);
			b->but_caption = bc->but_caption;
			b->but_u = bc->but_u;
			if (b->but_u != NULL)
				b->but_u->but_a = b;
			movebm(b->but_u,bc->but_width,0);
			scalebmw(b->but_u,b->but_width,width);
			bc->but_u = NULL;
			bc->but_caption = nullcap();
			reg_caption(&bc->but_caption,bc->but_left,bc->but_top,
				bc->but_width - 1,bc->but_height - 1,bc);
			reg_caption(&b->but_caption,b->but_left,b->but_top,
				b->but_width - 1, b->but_height - 1,b);
			srsel(b->but_left + 1, b->but_top + 1);
		} else {
			b->but_rv = rvnext();
			b->but_style = 0;
			scalebmw(bc->but_u,bc->but_width,width);
			b->but_u = NULL;
			b->but_caption = nullcap();
			reg_caption(&b->but_caption,b->but_left,b->but_top,
				b->but_width - 1, b->but_height - 1,b);
		}
	} else if (!vertical && !(bc->but_flags & (BM_HOR | BM_NDIV))) {
		/*  We can divide horizontally
		 */
		b = (struct butst *)malloc(sizeof(struct butst));
		/* insert b after bc
		 */
		b->but_left = bc->but_left;
		b->but_width = bc->but_width;
		b->but_top = lasty + 1;
		b->but_height = bc->but_height - lasty + bc->but_top - 1;
		bc->but_height -= b->but_height;
		bc->but_flags |= BM_VER;
		b->but_flags = BM_VER;
		b->but_b = bc->but_b;
		if (b->but_b != NULL)
			b->but_b->but_a = b;
		b->but_a = bc;
		bc->but_b = b;
		if (by < height / 2) {
			b->but_rv = bc->but_rv;
			b->but_style = bc->but_style;
			bc->but_rv = rvnext();
			bc->but_style = 0;
			deregister(bc->but_caption);
			b->but_caption = bc->but_caption;
			b->but_u = bc->but_u;
			if (b->but_u != NULL)
				b->but_u->but_a = b;
			movebm(b->but_u,0,bc->but_height);
			scalebmh(b->but_u,b->but_height,height);
			bc->but_u = NULL;
			bc->but_caption = nullcap();
			reg_caption(&bc->but_caption,bc->but_left,bc->but_top,
				bc->but_width - 1,bc->but_height - 1,bc);
			reg_caption(&b->but_caption,b->but_left,b->but_top,
				b->but_width - 1, b->but_height - 1,b);
			srsel(b->but_left + 1, b->but_top + 1);
		} else {
			b->but_rv = rvnext();
			b->but_style = 0;
			scalebmh(bc->but_u,bc->but_height,height);
			b->but_u = NULL;
			b->but_caption = nullcap();
			reg_caption(&b->but_caption,b->but_left,b->but_top,
				b->but_width - 1, b->but_height - 1,b);
		}
	} else if (vertical) {
		/* We need to go underneath
		 */
		b1 = (struct butst *)malloc(sizeof(struct butst));
		b2 = (struct butst *)malloc(sizeof(struct butst));
		b1->but_top = bc->but_top;
		b2->but_top = bc->but_top;
		b1->but_height = bc->but_height;
		b2->but_height = bc->but_height;
		b1->but_left = bc->but_left;
		b2->but_left = lastx + 1;
		b1->but_width = lastx + 1 - b1->but_left;
		b2->but_width = bc->but_width - b1->but_width;
		b1->but_flags = BM_HOR | BM_FIRST;
		b2->but_flags = BM_HOR;
		b1->but_a = bc;
		b1->but_b = b2;
		b2->but_a = b1;
		b2->but_b = NULL;
		if (bx < bc->but_width / 2) {
			b1->but_rv = rvnext();
			b1->but_style = 0;
			b2->but_rv = bc->but_rv;
			b2->but_style = bc->but_style;
			b1->but_caption = nullcap();
			b1->but_u = NULL;
			b2->but_u = bc->but_u;
			if (b2->but_u != NULL)
				b2->but_u->but_a = b2;
			movebm(b2->but_u,b1->but_width,0);
			scalebmw(b2->but_u,b2->but_width,width);
			reg_caption(&b1->but_caption,b1->but_left,b1->but_top,
				b1->but_width - 1, b1->but_height - 1,b1);
			deregister(bc->but_caption);
			b2->but_caption = bc->but_caption;
			reg_caption(&b2->but_caption,b2->but_left,b2->but_top,
				b2->but_width - 1, b2->but_height - 1,b2);
			srsel(b2->but_left + 1, b2->but_top + 1);
		} else {
			b2->but_rv = rvnext();
			b2->but_style = 0;
			b1->but_rv = bc->but_rv;
			b1->but_style = bc->but_style;
			b2->but_caption = nullcap();
			b2->but_u = NULL;
			b1->but_u = bc->but_u;
			if (b1->but_u != NULL)
				b1->but_u->but_a = b1;
			scalebmw(b1->but_u,b1->but_width,width);
			reg_caption(&b2->but_caption,b2->but_left,b2->but_top,
				b2->but_width - 1, b2->but_height - 1,b2);
			deregister(bc->but_caption);
			b1->but_caption = bc->but_caption;
			reg_caption(&b1->but_caption,b1->but_left,b1->but_top,
				b1->but_width - 1, b1->but_height - 1,b1);
			srsel(b1->but_left + 1, b1->but_top + 1);
		}
		bc->but_rv = 0;
		bc->but_style = 0;
		bc->but_u = b1;
		bc->but_caption = NULL;
		bc->but_flags |= BM_CLEAR;
	} else {
		/* We need to go underneath
		 */
		b1 = (struct butst *)malloc(sizeof(struct butst));
		b2 = (struct butst *)malloc(sizeof(struct butst));
		b1->but_left = bc->but_left;
		b2->but_left = bc->but_left;
		b1->but_width = bc->but_width;
		b2->but_width = bc->but_width;
		b1->but_top = bc->but_top;
		b2->but_top = lasty + 1;
		b1->but_height = lasty + 1 - b1->but_top;
		b2->but_height = bc->but_height - b1->but_height;
		b1->but_flags = BM_VER | BM_FIRST;
		b2->but_flags = BM_VER;
		b1->but_a = bc;
		b1->but_b = b2;
		b2->but_a = b1;
		b2->but_b = NULL;
		if (by < bc->but_height / 2) {
			b1->but_rv = rvnext();
			b1->but_style = 0;
			b2->but_rv = bc->but_rv;
			b2->but_style = bc->but_style;
			b1->but_caption = nullcap();
			b1->but_u = NULL;
			b2->but_u = bc->but_u;
			if (b2->but_u != NULL)
				b2->but_u->but_a = b2;
			movebm(b2->but_u,0,b1->but_height);
			scalebmh(b2->but_u,b2->but_height,height);
			reg_caption(&b1->but_caption,b1->but_left,b1->but_top,
				b1->but_width - 1, b1->but_height - 1,b1);
			deregister(bc->but_caption);
			b2->but_caption = bc->but_caption;
			reg_caption(&b2->but_caption,b2->but_left,b2->but_top,
				b2->but_width - 1, b2->but_height - 1,b2);
			srsel(b2->but_left + 1, b2->but_top + 1);
		} else {
			b2->but_rv = rvnext();
			b2->but_style = 0;
			b1->but_rv = bc->but_rv;
			b1->but_style = bc->but_style;
			b2->but_caption = nullcap();
			b2->but_u = NULL;
			b1->but_u = bc->but_u;
			if (b1->but_u != NULL)
				b1->but_u->but_a = b1;
			scalebmh(b1->but_u,b1->but_height,height);
			reg_caption(&b2->but_caption,b2->but_left,b2->but_top,
				b2->but_width - 1, b2->but_height - 1,b2);
			deregister(bc->but_caption);
			b1->but_caption = bc->but_caption;
			reg_caption(&b1->but_caption,b1->but_left,b1->but_top,
				b1->but_width - 1, b1->but_height - 1,b1);
			srsel(b1->but_left + 1, b1->but_top + 1);
		}
		bc->but_rv = 0;
		bc->but_style = 0;
		bc->but_u = b1;
		bc->but_caption = NULL;
		bc->but_flags |= BM_CLEAR;
	}
	return(0);	
}

/*  Move the menu be distance (dx, dy)
 */
void
movebm(bm,dx,dy)
struct butst *bm;
int dx, dy;
{
	register struct butst *b;
	extern struct wrectst mwr;

	if (bm == NULL)
		return;
	if (dx < mwr.wr_x - bm->but_left)
		dx = mwr.wr_x - bm->but_left;
	if (dy < mwr.wr_y - bm->but_top)
		dy = mwr.wr_y - bm->but_top;
	for (b = bm; b != NULL; b = b->but_b) {
		b->but_left += dx;
		b->but_top += dy;
		movebm(b->but_u,dx,dy);
	}
}

/*  Scale the menu height by nh / oh.
 */
void
scalebmh(bm,nh,oh)
struct butst *bm;
int nh, oh;
{
	register struct butst *b;
	int height, top, newtop, h;

	if (bm == NULL)
		return;
	if (bm->but_flags & BM_FREE)
		return;

	if (bm->but_flags & BM_VER){
		newtop = bm->but_top;
		top = newtop;
		height = 0;
		for (b = bm; b != NULL; b = b->but_b) {
			movebm(b->but_u, 0, newtop - b->but_top);
			b->but_top = newtop;
			height += b->but_height;
			h = height * nh / oh - newtop + top;
			scalebmh(b->but_u, h, b->but_height);
			b->but_height = h;
			newtop += b->but_height;
		}
	} else {
		for (b = bm; b != NULL; b = b->but_b) {
			b->but_height = b->but_height * nh / oh;
			scalebmh(b->but_u,nh,oh);
		}
	}
}

/*  Scale the menu width by nw / ow.
 */
void
scalebmw(bm,nw,ow)
struct butst *bm;
int nw, ow;
{
	register struct butst *b;
	int newleft, left, width, w;

	if (bm == NULL)
		return;
	if (bm->but_flags & BM_FREE)
		return;

	if (bm->but_flags & BM_HOR) {
		newleft = bm->but_left;
		left = newleft;
		width = 0;
		for (b = bm; b != NULL; b = b->but_b) {
			movebm(b->but_u,newleft - b->but_left, 0);
			b->but_left = newleft;
			width += b->but_width;
			w = width * nw / ow - newleft + left;
			scalebmw(b->but_u, w, b->but_width);
			b->but_width = w;
			newleft += w;
		}
	} else {
		for (b = bm; b != NULL; b = b->but_b) {
			b->but_width = b->but_width * nw / ow;
			scalebmw(b->but_u,nw,ow);
		}
	}
}

