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

#define MAXCUT	500	/* maximum number of cut buttons saved */

struct butst *curb = NULL;	/* the current button */
static struct butst *cutbuttons[MAXCUT];	/* the last cut button */
static int ncut = 0;		/* number of buttons saved */
extern int wd;			/* file descriptor of the current window */

extern struct butst *bmtop();	/* the top displayed menu */

static int bmpush PROTO((struct butst *bm));
static void movervs PROTO((struct butst *bm, int tomain));
static struct butst * bmpop PROTO((void));
static void freervs PROTO((struct butst *bm));

/*  Set the current button to b.
 */
void
setcurb(b)
struct butst *b;
{
	curb = b;
	if (curb != NULL) {
		setstyle(curb->but_style);
		setrv(curb->but_rv);
	} else {
		setstyle(-1);
		setrv(-1);
	}
}

/*  Set the return value of the current button
 */
void
bmsetrv(rv)
int rv;
{
	if (curb == NULL)
		return;	/* should never happen */
	curb->but_rv = rv;
}

/*  Set the font number and style in the current button
 */
void
setbutstyle(n)
{
	if (curb == NULL)
		return;
	curb->but_style = n;
}

/*  Return the font number of the button
 */
int
bmfont(bm)
struct butst *bm;
{
	return(bm->but_style & SM_FONT);
}

static
int
bmpush(bm)
struct butst *bm;
{
	if (ncut == MAXCUT)
		return(-1);
	cutbuttons[ncut++] = bm;
	movervs(bm,0);
	rvsetup(0);
	return(0);
}

static struct butst *
bmpop()
{
	struct butst *b;

	if (ncut == 0)
		return(NULL);
	b = cutbuttons[--ncut];
	movervs(b,1);
	rvsetup(0);
	return(b);
}

/*  Move all the menus return values between main and cut.
 */
static
void
movervs(bm,tomain)
struct butst *bm;
int tomain;
{
	register struct butst *b;

	if (bm == NULL)
		return;

	for (b = bm; b != NULL; b = b->but_b) {
		if ((b->but_flags & BM_CLEAR) == 0)
			rvmove(b->but_rv,tomain);
		movervs(b->but_u,tomain);
	}
}

/*  Free all the rv's in the menu.
 */
static
void
freervs(bm)
struct butst *bm;
{
	register struct butst *b;

	if (bm == NULL)
		return;

	for (b = bm; b != NULL; b = b->but_b) {
		if ((b->but_flags & BM_CLEAR) == 0)
			rvfree(b->but_rv);
		freervs(b->but_u);
	}
}

/*  Remove the current button from the menu and save it so that it
 *  can be pasted back in.
 */
void
bmcut()
{
	struct butst *bc, *b;

	if (curb == NULL) {
		mprintf("There is no button selected");
		bell(0);
		return;
	}

	bc = curb;
	deregister(bc->but_caption);
	bmdereg(bc->but_u);
	if (bc->but_flags & BM_NDIV) {
		bspop();
		if (bc->but_a == NULL) {
			bmroot((struct butst *)NULL);
			blankma();
		} else {
			bc->but_a->but_u = NULL;
			bmredraw();
		}
		bc->but_b = NULL;
		bmpush(bc);
		return;
	}
	if (bc->but_flags & BM_FIRST) {
		if (bc->but_b == NULL) {
			if (bc->but_a == NULL) {
				bmroot((struct butst *)NULL);
				blankma();
			} else {
				bc->but_a->but_u = NULL;
				bmreset(bc->but_u,0);
			}
			bmpush(bc);
			return;
		}
		b = bc->but_b;
		bc->but_b = NULL;	
		/*  enlarge the next button to fill the gap
		 */
		if (bc->but_a == NULL) {
			bmroot(b);
			b->but_a = NULL;
		} else {
			bc->but_a->but_u = b;
			b->but_a = bc->but_a;
		}
		b->but_flags |= BM_FIRST;
		if (bc->but_flags & BM_HOR) {
			movebm(b->but_u,bc->but_left - b->but_left, 0);
			b->but_left = bc->but_left;
			scalebmw(b->but_u,bc->but_width + b->but_width,
								b->but_width);
			b->but_width += bc->but_width;
		} else if (bc->but_flags & BM_VER) {
			movebm(b->but_u, 0, bc->but_top - b->but_top);
			b->but_top = bc->but_top;
			scalebmh(b->but_u,bc->but_height + b->but_height,
							b->but_height);
			b->but_height += bc->but_height;
		}
	} else {
		/*  enlarge the previous button
		 */
		b = bc->but_a;
		b->but_b = bc->but_b;
		bc->but_b = NULL;
		if (b->but_b != NULL)
			b->but_b->but_a = b;
		if (bc->but_flags & BM_HOR) {
			scalebmw(b->but_u,bc->but_width + b->but_width,
								b->but_width);
			b->but_width += bc->but_width;
		} else if (bc->but_flags & BM_VER) {
			scalebmh(b->but_u,bc->but_height + b->but_height,
							b->but_height);
			b->but_height += bc->but_height;
		}
	}
	bmpush(bc);
	if ((b->but_flags & BM_FIRST) && (b->but_b == NULL)) {
		/*  We have a single button sequence so we have to
		 *  get rid of it.
		 */
		bc = b->but_a;
		bc->but_style = b->but_style;
		bc->but_rv = b->but_rv;
		if (b->but_flags & BM_CLEAR)
			bc->but_flags |= BM_CLEAR;
		else {
			deregister(b->but_caption);
			bc->but_caption = b->but_caption;
			reg_caption(&bc->but_caption,bc->but_left,bc->but_top,
					bc->but_width,bc->but_height,bc);
			bc->but_flags &= ~BM_CLEAR;
		}
		bc->but_u = b->but_u;
		if (bc->but_u != NULL)
			bc->but_u->but_a = bc;
		bc->but_flags &= ~BM_OPEN;
		bc->but_flags |= b->but_flags & BM_OPEN;
		free((char *)b);
		b = bc;
	}
	bmshow(b);
}
			
/*  Paste the last cut button into the currently selected button.
 */
void
bmpaste()
{
	struct butst *bc, *b;

	if (curb == NULL) {
		mprintf("There is no button currently selected");
		bell(0);
		return;
	}
	if ((b = bmpop()) == NULL) {
		mprintf("There is no cut button");
		bell(0);
		return;
	}
	bc = curb;
	freervs(bc->but_u);
	rvfree(bc->but_rv);
	rvsetup(0);
	bc->but_u = b->but_u;
	if (bc->but_u != NULL)
		bc->but_u->but_a = bc;
	bc->but_rv = b->but_rv;
	bc->but_style = b->but_style;
	movebm(bc->but_u,bc->but_left - b->but_left,bc->but_top - b->but_top);
	scalebmw(bc->but_u,bc->but_width,b->but_width);
	scalebmh(bc->but_u,bc->but_height,b->but_height);
	deregister(bc->but_caption);
	free(bc->but_caption);
	bc->but_caption = b->but_caption;
	reg_caption(&bc->but_caption,bc->but_left,bc->but_top,bc->but_width-1,
						bc->but_height - 1,bc);
	bmregall(bc->but_u);
	bmreset(bc,0);
	srsel(bc->but_left + 1,bc->but_top + 1);
}

/*  register all the captions in the bmenu
 */
void
bmregall(bm)
struct butst *bm;
{
	register struct butst *b;

	if (bm == NULL)
		return;
	for (b = bm; b != NULL; b = b->but_b) {
		if ((b->but_flags & BM_CLEAR) == 0)
			reg_caption(&b->but_caption,b->but_left,
						b->but_top,b->but_width-1,
						b->but_height - 1,b);
		bmregall(b->but_u);
	}
}

/*  deregister all the strings in the bmenu
 */
void
bmdereg(bm)
struct butst *bm;
{
	register struct butst *b;

	if (bm == NULL)
		return;
	for (b = bm; b != NULL; b = b->but_b) {
		if ((b->but_flags & BM_CLEAR) == 0)
			deregister(b->but_caption);
		bmdereg(b->but_u);
	}
}


/*  Uncover a hidden submenu by deleting the current button
 */
void
bmuncover()
{
	register struct butst *b, *cb;

	if (curb == NULL) {
		mprintf("There is no button selected");
		bell(0);
		return;
	}
	if (curb->but_u == NULL) {
		mprintf("The current button has no submenu");
		bell(0);
		return;
	}
	if (curb->but_u->but_flags & BM_NDIV) {
		mprintf("Not a covering button");
		bell(0);
		return;
	}
	cb = curb;
	rvfree(cb->but_rv);
	deregister(cb->but_caption);
	free(cb->but_caption);
	b = cb->but_u;
	if (((cb->but_flags & BM_HOR) && (b->but_flags & BM_VER)) ||
	    ((cb->but_flags & BM_VER) && (b->but_flags & BM_HOR)) ||
					  (cb->but_flags & BM_NDIV)) {
		cb->but_flags |= BM_CLEAR;
		bmshow(cb);
		return;
	}
	/* We have to delete the button and replace it with its
	 * submenu.
	 */
	b->but_a = cb->but_a;
	if (b->but_a != NULL) {
		if (cb->but_flags & BM_FIRST) {
			b->but_a->but_u = b;
			b->but_flags |= BM_FIRST;
		} else {
			b->but_a->but_b = b;
			b->but_flags &= ~BM_FIRST;
		}
	} else
		bmroot(b);
	while (b->but_b != NULL)
		b = b->but_b;
	b->but_b = cb->but_b;
	if (b->but_b)
		b->but_b->but_a = b;
	b = cb->but_u;
	free((char *)cb);
	bmshow(b);
	cb = NULL;
}

/*  Add a hidden subfield under the current button the same size as the one
 *  in the argument. 0 is returned if successful. -1 is returned
 *  if the insertion is impossible.
 */
int
addhidden()
{
	struct butst *b;
	char *nullcap();

	if (curb == NULL) {
		mprintf("### there is no button selected ###");
		return(-1);
	}
	if (curb->but_u != NULL) {
		mprintf("### the selected button already has a submenu ###");
		return(-1);
	}
	b = (struct butst *)malloc(sizeof(struct butst));
	b->but_left = curb->but_left;
	b->but_top = curb->but_top;
	b->but_width = curb->but_width;
	b->but_height = curb->but_height;
	b->but_flags = BM_FIRST;
	b->but_a = curb;
	b->but_b = NULL;
	b->but_u = NULL;
	b->but_rv = rvnext();
	b->but_style = 0;
	b->but_caption = nullcap();
	curb->but_flags |= BM_OPEN;
	curb->but_u = b;
	cap_off(curb->but_caption,1);
	reg_caption(&b->but_caption,b->but_left,b->but_top,
				b->but_width - 1, b->but_height - 1,b);
	return(0);
}

/*  Close all the menu's open buttons and turn off invisible captions.
 *  Should be called with conceal equal to 0.
 */
void
bmreset(bm,conceal)
struct butst *bm;
int conceal;
{
	register struct butst *b;

	for (b = bm; b != NULL; b = b->but_b) {
		b->but_flags &= ~BM_OPEN;
		if (!ISOPEN(b)) {
			if (conceal)
				cap_off(b->but_caption,0);
			else
				cap_on(b->but_caption,0);
		}
		if (b->but_u != NULL)
			bmreset(b->but_u,!ISOPEN(b) || conceal);
	}
}

/*  Create a new bmenu structure of one button with the given width and
 *  height in the top left hand corner of the clip area.
 */
struct butst *
newbmen(wrp,width,height)
int width, height;
struct wrectst *wrp;
{
	struct butst *b;

	b = (struct butst *)malloc(sizeof(struct butst));
	b->but_left = wrp->wr_x;
	b->but_top = wrp->wr_y;
	b->but_width = width - wrp->wr_x + 1;
	b->but_height = height - wrp->wr_y + 1;
	b->but_flags = BM_FIRST | BM_NDIV | BM_FREE;
	b->but_rv = rvnext();
	b->but_style = 0;
	b->but_a = NULL;
	b->but_b = NULL;
	b->but_u = NULL;
	b->but_caption = nullcap();
	reg_caption(&b->but_caption,b->but_left,b->but_top,
				b->but_width - 1, b->but_height - 1,b);
	srsel(b->but_left + 1, b->but_top + 1);
	bmroot(b);
	return 0;
}

/*  Return a zero length malloc'ed caption
 */
char *
nullcap()
{
	char *s;

	s = malloc(1);
	*s = 0;
	return(s);
}
