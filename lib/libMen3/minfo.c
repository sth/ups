/* minfo.c - functions giving information about menu geometry */

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


/* @(#)minfo.c	1.6 4/7/91 (UKC) */
char Men3_minfo_c_rcsid[] = "$Id$";

#include <stdio.h>

#include <local/wn.h>
#include "menu3.h"
#include "menu_priv.h"

/*  Return the left edge of the menu.
 */
int
Mleft(md)
int md;
{
	if ((md < 0) || (md >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
	if (_menu_[md].om_root == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
	return(_menu_[md].om_root->me_xstart + 1);
}

/*  Return the right edge of the menu.
 */
int
Mright(md)
int md;
{
	if ((md < 0) || (md >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
	if (_menu_[md].om_root == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
	return(_menu_[md].om_root->me_xend);
}
/*  Return the top edge of the menu.
 */
int
Mtop(md)
int md;
{
	if ((md < 0) || (md >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
	if (_menu_[md].om_root == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
	return(_menu_[md].om_root->me_ystart + 1);
}

/*  Return the bottom edge of the menu.
 */
int
Mbottom(md)
int md;
{
	if ((md < 0) || (md >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
	if (_menu_[md].om_root == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
	return(_menu_[md].om_root->me_yend);
}

/*  Return the width of the menu.
 */
int
Mwidth(md)
int md;
{
	if ((md < 0) || (md >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
	if (_menu_[md].om_root == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
	return(_menu_[md].om_root->me_xend - _menu_[md].om_root->me_xstart - 1);
}

/*  Return the height of the menu.
 */
int
Mheight(md)
int md;
{
	if ((md < 0) || (md >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
	if (_menu_[md].om_root == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
	return(_menu_[md].om_root->me_yend - _menu_[md].om_root->me_ystart - 1);
}
