/* msize.c - code for changing menu geometry */

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


/* @(#)msize.c	1.8 26/4/92 (UKC) */
char Men3_msize_c_rcsid[] = "$Id$";

#include <stdio.h>

#include <local/wn.h>
#include "menu3.h"
#include "menu_priv.h"

static int mscale MPROTO((MENU * menu, int width, int height));

/*  The menu identified by descriptor md is scaled so that its width and
 *  and height are as given by the arguments. If either 'width' or 'height'
 *  are zero then that dimension is left unchanged.
 *  -1 is returned if unsuccessful, 0 if ok.
 */
int
Msize(md,width,height)
int md, width, height;
{
	if ((md < 0) || (md >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
	if (_menu_[md].om_root == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
	if ((width < 0) || (height < 0)) {
		menerr = MBADARG;
		return(-1);
	}

	return(mscale(_menu_[md].om_root,width,height));
}

/*  Scale the menu so that it has the new height and width. If either the
 *  width or height arguments are zero then the corresponding parameter
 *  is left unchanged in the menu.
 */
static int
mscale(menu,width,height)
MENU * menu;
int width, height;
{
	static float xratio, yratio;
	static short xstart, ystart;
	int x, y;

	if (menu == NULL)
		return(0);
	if (menu->me_parent == NULL) {
		xratio = 1.0;
		yratio = 1.0;
		if (width)
		  xratio = (float)(width+1)/(menu->me_xend-menu->me_xstart);
		if (height)
		  yratio = (float)(height+1)/(menu->me_yend-menu->me_ystart);
		xstart = menu->me_xstart;
		ystart = menu->me_ystart;
	} else if ((menu->me_flags & ME_FREE) && 
				(menu->me_parent->me_flags & ME_NOSCALE)) {
		/*  Don't scale free submenu.
		 */
		x = xstart + (int)(xratio*(menu->me_xstart-xstart)+0.5);
		y = ystart + (int)(yratio*(menu->me_ystart-ystart)+0.5);
		mshift(menu,x + 1,y + 1,1);
		return(0);
	}
 
	menu->me_xstart = xstart + (int)(xratio*(menu->me_xstart-xstart)+0.5);
	menu->me_ystart = ystart + (int)(yratio*(menu->me_ystart-ystart)+0.5);
	menu->me_xend=xstart+(int)(xratio*(menu->me_xend-xstart)+0.5);
	menu->me_yend=ystart+(int)(yratio*(menu->me_yend-ystart)+0.5);
	menu->me_xcurs = menu->me_xcurs * xratio + 0.5;
	menu->me_ycurs = menu->me_ycurs * yratio + 0.5;
	if (menu->me_flags & ME_VER)
		menu->me_pos = ystart+(int)(yratio*(menu->me_pos-ystart)+0.5);
	if (menu->me_flags & ME_HOR)
		menu->me_pos = xstart+(int)(xratio*(menu->me_pos-xstart)+0.5);
	mscale(menu->me_topleft,0,0);
	mscale(menu->me_botrite,0,0);
	return(0);
}


/*  Move the menu identified by menu descriptor md so that its top left hand
 *  corner is at position x,y.
 */
int
Mplace(md,x,y)
int md, x, y;
{

	if ((md < 0) || (md >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
	if (_menu_[md].om_root == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
	return(mshift(_menu_[md].om_root,x,y,1));
}

/*  Shift the menu to location x,y.
 */
int
mshift(menu,x,y,root)
MENU * menu;
int x,y,root;
{
	static short dx,dy;

	if (menu == NULL)
		return(0);
	if (root) {
		dx = x - menu->me_xstart - 1;
		dy = y - menu->me_ystart - 1;
	}
	menu->me_xstart += dx;
	menu->me_xend += dx;
	menu->me_ystart += dy;
	menu->me_yend += dy;
	if (menu->me_flags & ME_VER)
		menu->me_pos += dy;
	if (menu->me_flags & ME_HOR)
		menu->me_pos += dx;
	mshift(menu->me_topleft,x,y,0);
	mshift(menu->me_botrite,x,y,0);
	return(0);
}

/*  Check that the menu as currently positioned will fit inside
**  the main window. Reposition the menu and, if may_warp_cursor is
**  set, warp the cursor if necessary make the menu fit and maintain
**  the initially selected item.
**
**  If the cursor is warped, *pcx and *pcy will contain the new
**  position.
**
**  Currently assumes all items are arranged vertically or all are
**  horizontal.
*/
int
Mposition_popup(md, wn, pcx, pcy, may_warp_cursor)
int md;
int wn;
int *pcx, *pcy;   /* point to current cursor position in window */
int may_warp_cursor;
{
	MENU* menu;
	if ((md < 0) || (md >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}

	menu = _menu_[md].om_root;
	if (menu == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
	return mposition_popup(menu, wn, pcx, pcy, may_warp_cursor);
}

int
mposition_popup(menu, wn, pcx, pcy, may_warp_cursor)
MENU* menu;
int wn;
int *pcx, *pcy;   /* point to current cursor position in window */
int may_warp_cursor;
{
#define BORDER_LO  10 /* min distance of menu from top or left */
#define BORDER_HI   5 /* min distance of menu from bottom or right */
#define BORDER_MENU 5 /* min distance of cursor from menu boundary, if originally inside */
	int base_width, base_height;
	int xstart, ystart, xend, yend, delta;
	int xmin, xmax, ymin, ymax;
	int cursor_inside;

	int cx = *pcx, cy = *pcy;
	int base_wn = wn_get_root_window(wn);
	int do_move = 0;

	/* Get the dimensions of the base window */
	wn_get_window_size(base_wn, &base_width, &base_height);


	xstart = menu->me_xstart;
	ystart = menu->me_ystart;
	xend = menu->me_xend;
	yend = menu->me_yend;

	/* decide if the cursor is inside the menu */
	cursor_inside = (xstart <= cx ) && (cx <= xend)
		      && (ystart <= cy ) && (cy <= yend);

	/* Get the permitted region */
	wn_trans_coords(base_wn, BORDER_LO, BORDER_LO, wn, &xmin, &ymin);
	wn_trans_coords(base_wn, base_width-BORDER_HI, base_height-BORDER_HI, wn, &xmax, &ymax);

	/* If the menu started close to the top/left borders, allow 
	** it to stay there */
	if ( xstart <  xmin)
	    xmin -= BORDER_LO;
	if ( ystart <  ymin)
	    ymin -= BORDER_LO;


	/* Decide if the menu will fit on the screen */

	/* Look at right edge */
	delta = xmax - xend;
	if ( delta < 0 )
	{
	    do_move = 1;
	    xstart += delta;
	    xend += delta;
	    if ( cursor_inside )
	    {
		if ( menu->me_flags&ME_HOR)
	          cx += delta;
		else if (cx >= xend)
		    cx = xend-BORDER_MENU;
	    }
	}

	/* Look at bottom edge */
	delta = ymax-yend;
	if ( delta < 0 )
	{
	    do_move = 1;
	    ystart += delta;
	    yend += delta;
	    if ( cursor_inside )
	    {
		if ( menu->me_flags&ME_VER)
	          cy += delta;
		else if (cy >= yend)
		    cy = yend -BORDER_MENU;
	    }
	}

	/* If we moved the menu up or to the left, make sure it
	** hasn't gone too far
	*/
	/* Look at left edge */
	delta = xmin-xstart;
	if ( delta > 0 )
	{
	    do_move = 1;
	    xstart += delta;
	    xend += delta;
	    if ( cursor_inside )
	    {
		if ( menu->me_flags&ME_HOR)
		    cx += delta;
		else if (cx <= xstart)
		    cx = xstart+BORDER_MENU;
	    }
	}

	/* Look at top edge */
	delta = ymin - ystart;
	if ( delta > 0)
	{
	    do_move = 1;
	    ystart += delta;
	    yend += delta;
	    if ( cursor_inside )
	    {
		if ( menu->me_flags&ME_VER)
		    cy += delta;
		else if (cy <= ystart)
		    cy = ystart+BORDER_MENU;
	    }
	}

	if ( do_move)
	{
	    mshift(menu,xstart+1,ystart+1,1);
	    if ( may_warp_cursor && (*pcx != cx || *pcy != cy))
	    {
		wn_warp_mouse(wn, cx, cy);
		*pcx = cx;
		*pcy = cy;
	    }
	}
	return 0;
#undef BORDER_LO
#undef BORDER_HI
#undef BORDER_MENU
}

