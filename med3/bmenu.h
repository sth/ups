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

/* %W% (UKC) %G% */

/* @(#)bmenu.h	1.2  7/30/86 */
static char bmsccsid[] = "$Id$";

struct butst {
	short but_left;
	short but_top;
	short but_width;
	short but_height;
	short but_flags;
	short but_style;
	short but_rv;		/* return value */
	struct butst *but_a;	/* previous (left or higher) button */
	struct butst *but_b;	/* next (right or below) button */
	struct butst *but_u;	/* menu underneath this one */
	char *but_caption;	/* the displayed caption */
};

/*  Some button flags controling local use
 */
#define BM_HOR		0x1	/* part of a horizontal sequence */
#define BM_VER		0x2	/* part of a vertical sequence */
#define BM_CLEAR	0x4	/* button is transparent */
#define BM_OPEN	0x8	/* the button has been opened */
#define BM_FIRST	0x10	/* This is the first button of a submenu */
#define BM_NDIV	0x20	/* Incapable of being subdivided */
#define BM_FREE	0x40	/* freely positioned */
#define BM_BREAK	0x80	/* used when converting to a lib menu */

/*  Style flags
 */
#define SM_FONT		0x3	/* mask for the font number */
#define SM_POPUP	0x4	/* The menu will operate in popup style */
#define SM_BGLINE	0x8	/* lines in background colour */
#define SM_NOSCALE	0x10	/* Don't resize submenu */
#define SM_CREL		0x20	/* Cursor relative submenu */

#define ISOPEN(b)	((b)->but_flags & (BM_CLEAR | BM_OPEN))

#define LSPEED		(0.4)	/* speed of moving division lines */

/* Submenu styles
 */
#define NORMAL	0
#define POPUP	1

/* The structure defining an area of the window
 */
struct wrectst {
	int wr_x;
	int wr_y;
	int wr_width;
	int wr_height;
};
