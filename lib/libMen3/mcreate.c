/* mcreate.c - menu creation from static data */

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


/* @(#)mcreate.c	1.10 13/5/92 (UKC) */
char Men3_mcreate_c_rcsid[] = "$Id$";

#include <stdio.h>
#include <stdlib.h>

#include <local/wn.h>
#include "menu3.h"
#include "menu_priv.h"

static int result;

/* to do - make the line depth dependent on the fonts of this menu */
#define LDEPTH (16 + 4) /* the depth of one menu slot */

static void mcheck MPROTO((MENU * m));
static MENU *mcreate MPROTO((const char **caps, const int *rvs, const int *fonts,
				int ncaps, int ystart, int xend, int style,
				MENU *parent));

int
Mmake(name,captions,rvalues,fonts,style,width)
const char *name;		/* for X color resource for whole pop-up */
const char **captions;
const int *rvalues, *fonts;
int style, width;
{
	register int i, j;
	MENU *m;
	int fg_specified, bg_specified;

	for (i = 0; i < MAXMEN; i++)
		if (_menu_[i].om_root == NULL)
			break;
	if (i == MAXMEN) {
		menerr = MTABFL;
		return(-1);
	}
	for (j = 0; captions[j] != NULL; j++)
		;
	if (j == 0) {
		menerr = MBADARG;
		return(-1);
	}

	result = 1;
	m = mcreate(captions,rvalues,fonts,j,0,width,style,(MENU *)NULL);
	m->me_flags |= ME_FREE;
	if (style & MM_DRAG)
		m->me_flags |= ME_POPUP;

	Mget_menu_colors(name,
			&m->me_fg_colour, &m->me_bg_colour,
			 &fg_specified, &bg_specified);
	Mpropagate_colors(m, m->me_fg_colour, m->me_bg_colour);
	_menu_[i].om_root = m;
	_menu_[i].om_md = i;
	_menu_[i].om_last = NULL;
	_menu_[i].om_fback[0] = MH_GREY;
	_menu_[i].om_fback[1] = MH_BLACK;
	_menu_[i].om_fback[2] = MH_GREY;
	_menu_[i].om_font[0] =
	_menu_[i].om_font[1] =
	_menu_[i].om_font[2] =
	_menu_[i].om_font[3] = Mstdfont();
	_menu_[i].om_wantbox = 1;
	momen(_menu_[i].om_root,&_menu_[i]);
	menerr = MENOK;
	return(i);
}

/*  insert the omen pointers
 */
void
momen(menu,omen)
MENU *menu;
struct omenst *omen;
{
	if (menu == NULL)
		return;
	menu->me_omen = omen;
	momen(menu->me_topleft,omen);
	momen(menu->me_botrite,omen);
}

static MENU *
mcreate(caps,rvs,fonts,ncaps,ystart,xend,style,parent)
const char **caps;
const int *rvs, *fonts;
int ncaps,ystart,xend,style;
MENU *parent;
{
	MENU * mp;

	mp = (MENU *)malloc(sizeof(MENU));
	mp->me_xstart = 0;
	mp->me_xend = xend;
	mp->me_ystart = ystart;
	mp->me_yend = ystart + ncaps * LDEPTH;
	mp->me_xcurs = 0;
	mp->me_ycurs = 0;
	mp->me_flags = ME_FREN;
	if (style & MM_LEFT)
		mp->me_flags |= ME_LEFT;
	mp->me_colour = 0;
	mp->me_fg_colour = 0;
	mp->me_bg_colour = 0;
	mp->me_pos = ystart + (ncaps / 2) * LDEPTH;
	mp->me_parent = parent;
	mp->me_toggle = NULL;
	if (ncaps > 1) {
		mp->me_cap = NULL;
		mp->me_flags |= ME_VER;
		if (style & MM_NOLINE)
			mp->me_flags |= ME_BGLINE;
		mp->me_topleft = mcreate(caps,rvs,fonts,ncaps/2,ystart,
					xend,style,mp);
		if (rvs != NULL)
			rvs += ncaps/2;
		if (fonts != NULL)
			fonts += ncaps/2;
		mp->me_botrite = mcreate(caps + ncaps/2,rvs,fonts,
				ncaps - ncaps/2,mp->me_pos,xend,style,mp);
	} else {
		mp->me_topleft = NULL;
		mp->me_botrite = NULL;
		mp->me_cap = caps[0];
		mp->me_rv = (rvs != NULL) ? rvs[0] : result++;
		if (fonts != NULL)
			mp->me_flags |= (ME_FONT & (fonts[0] - 1));
	}

	return(mp);
}

void
Mcheck(md)
int md;
{
	mcheck(_menu_[md].om_root);
}

static void
mcheck(m)
MENU * m;
{
	if (m == NULL)
		return;
	printf("xstart=%d, xend=%d, ystart=%d, yend=%d, pos=%d, flags=%x\n",
	m->me_xstart,m->me_xend,m->me_ystart,m->me_yend,m->me_pos,m->me_flags);
	mcheck(m->me_topleft);
	mcheck(m->me_botrite);
}

