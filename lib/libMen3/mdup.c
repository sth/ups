/* mdup.c - Mdup code */

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


/* @(#)mdup.c	1.10 17 Apr 1994 (UKC) */
char Men3_mdup_c_rcsid[] = "$Id$";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <local/wn.h>
#include "menu3.h"
#include "menu_priv.h"

static MENU *mdup MPROTO((const MENU *menu));

/*  Duplicate the menu described by menu descriptor md and return the
 *  descriptor for the new menu.
 */
int
Mdup(md)
int md;
{
	register int dupmd, i;
	int fg_specified, bg_specified;

	if ((md < 0) || (md >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
	if (_menu_[md].om_root == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
	for (dupmd = 0; dupmd < MAXMEN; dupmd++)
		if (_menu_[dupmd].om_root == NULL)
			break;
	if (dupmd == MAXMEN) {
		menerr = MTABFL;
		return(-1);
	}
	_menu_[dupmd].om_root = mdup(_menu_[md].om_root);
	Mget_menu_colors(_menu_[dupmd].om_root->me_cap,
			 &(_menu_[dupmd].om_root->me_fg_colour),
			 &(_menu_[dupmd].om_root->me_bg_colour),
			 &fg_specified, &bg_specified);
	Mpropagate_colors(_menu_[dupmd].om_root,
			 _menu_[dupmd].om_root->me_fg_colour,
			 _menu_[dupmd].om_root->me_bg_colour);
	_menu_[dupmd].om_root->me_parent = NULL;
	_menu_[dupmd].om_last = NULL;
	_menu_[dupmd].om_rect = NULLRECT;
	_menu_[dupmd].om_md = dupmd;
	for (i = 0; i < 3; i++)
		_menu_[dupmd].om_fback[i] = _menu_[md].om_fback[i];
	for (i = 0; i < 4; i++)
		_menu_[dupmd].om_font[i] = _menu_[md].om_font[i];
	_menu_[dupmd].om_wantbox = _menu_[md].om_wantbox;
	momen(_menu_[dupmd].om_root,&_menu_[dupmd]);
	menerr = MENOK;
	return(dupmd);
}

/*  recursive menu duplicating function
 */
static MENU *
mdup(menu)
const MENU *menu;
{
	MENU *dupmenu;
	char *newcap;

	dupmenu = (MENU *)malloc(sizeof(MENU));
	*dupmenu = *menu;
	if (menu->me_cap != NULL) {
		newcap = malloc(strlen(menu->me_cap) + 1);
		strcpy(newcap, menu->me_cap);
		dupmenu->me_cap = newcap;
	}
	dupmenu->me_flags |= (ME_FREC | ME_FREN);
	if (menu->me_topleft != NULL) {
		dupmenu->me_topleft = mdup(menu->me_topleft);
		dupmenu->me_topleft->me_parent = dupmenu;
	}
	if (menu->me_botrite != NULL) {
		dupmenu->me_botrite = mdup(menu->me_botrite);
		dupmenu->me_botrite->me_parent = dupmenu;
	}
	return(dupmenu);
}
