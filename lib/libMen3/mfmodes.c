/* mfmodes.c - Mfmodes code */

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


/* @(#)mfmodes.c	1.6 4/7/91 (UKC) */
char Men3_mfmodes_c_rcsid[] = "$Id$";

#include <stdio.h>

#include <local/wn.h>
#include "menu3.h"
#include "menu_priv.h"

/*  Set the feedback modes used. They should be some combination of the
 *  MH_ modes defined in menu.h.
 *  reveal - mode used when revealing a hidden submenu
 *  toact  - mode used on selecting a button that will cause some action.
 *  acting - mode used to show what command is being executed.
 */
int
Mfmodes(md,reveal,toact,acting)
int md, reveal, toact, acting;
{
	if ((md < 0) || (md >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
	if (_menu_[md].om_root == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}

	if (reveal >= 0)
		_menu_[md].om_fback[0] = reveal;
	if (toact >= 0)
		_menu_[md].om_fback[1] = toact;
	if (acting >= 0)
		_menu_[md].om_fback[2] = acting;
	return(0);
}
