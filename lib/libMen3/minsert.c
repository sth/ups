/* minsert.c - menu creation from static data */

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


/* @(#)minsert.c	1.10 26/4/92 (UKC) */
char Men3_minsert_c_rcsid[] = "$Id$";

#include <stdio.h>
#include <string.h>

#include <local/wn.h>
#include "menu3.h"
#include "menu_priv.h"

static void rplink MPROTO((MENU *mp, MENU *parent));
static MENU *mrvfind MPROTO((MENU *m_main, int rv));
static void mdump MPROTO((MENU *m, int indent));

void
Mget_menu_colors(name, p_fg, p_bg, fg_specified, bg_specified)
const char* name;
long *p_fg, *p_bg;
int *fg_specified, *bg_specified;
{
  static int color_checks_done = -1;
  color_t color;
  static int want_menu_colors = 0, menu_fg, menu_bg;
  int wn = WN_STDWIN;
  
  if (color_checks_done == -1)
  {
    menu_fg = WN_FG(wn);
    menu_bg = WN_BG(wn);
    want_menu_colors = wn_use_menu_colors();
    if (want_menu_colors)
    {
      const char *colorname;
	    
      wn_use_alloc_color(1); /* set */
      colorname = wn_get_default("MenuForeground");
      if (colorname != NULL)
      {
	wn_parse_color(wn, colorname, &color);
	if (wn_alloc_color(wn, &color, colorname))
	  menu_fg = color.co_pixel;
	else
	  menu_fg = WN_FG(wn);
      }

      colorname = wn_get_default("MenuBackground");
      if (colorname != NULL)
      {
	wn_parse_color(wn, colorname, &color);
	if (wn_alloc_color(wn, &color, colorname))
	  menu_bg = color.co_pixel;
	else
	  menu_bg = WN_BG(wn);
      }
    }
    color_checks_done = 0;	/* done */
  }

  *fg_specified = *bg_specified = 0;
  if (!want_menu_colors || !name)
  {
    *p_fg = menu_fg;
    *p_bg = menu_bg;
  }
  else
  {
    const char *colorname;
    char long_name[256];	/* RGA allow for long lib names (e.g. purified libs) */

    color.co_pixel = menu_fg;
    if (name[0] == '*' && name[1] == 0)
      name = "Asterisk";
    sprintf(long_name, "%sForeground", name);
    colorname = wn_get_default(long_name);
    if (colorname != NULL)
    {
      *fg_specified = 1;
      wn_parse_color(wn, colorname, &color);
      wn_use_alloc_color(1); /* set */
      wn_alloc_color(wn, &color, colorname);
    }
    if (!*p_fg)
      *p_fg = wn_use_alloc_color(0) ? color.co_pixel : menu_fg;

    color.co_pixel = menu_bg;
    sprintf(long_name, "%sBackground", name);
    colorname = wn_get_default(long_name);
    if (colorname != NULL)
    {
      *bg_specified = 1;
      wn_parse_color(wn, colorname, &color);
      wn_use_alloc_color(1); /* set */
      wn_alloc_color(wn, &color, colorname);
    }
    if (!*p_bg)
      *p_bg = wn_use_alloc_color(0) ? color.co_pixel : menu_bg;
  }
}

MENU*
Mpropagate_colors(m_main,fg,bg)
MENU *m_main;
long fg, bg;
{
	MENU *ms;
	int fg_specified = 0, bg_specified = 0;

	if (m_main == NULL)
		return(NULL);
	if (m_main->me_cap != NULL)
	   Mget_menu_colors
	     ((char *)m_main->me_cap, &m_main->me_fg_colour, &m_main->me_bg_colour,
	      &fg_specified, &bg_specified);
	else
	{
	  m_main->me_fg_colour = fg;
	  m_main->me_bg_colour = bg;
	}
	if ((ms = Mpropagate_colors
	     (m_main->me_topleft,
	      m_main->me_fg_colour,m_main->me_bg_colour)) != NULL)
		return(ms);
	if ((ms = Mpropagate_colors
	     (m_main->me_botrite,
	      m_main->me_fg_colour,m_main->me_bg_colour)) != NULL)
		return(ms);
	if (!fg_specified)
	  m_main->me_fg_colour = fg;
	if (!bg_specified)
	  m_main->me_bg_colour = bg;
	return(NULL);
}

/*  Insert the menu in the _menu_ array.  A menu identifier is returned which
 *  is used to identify the menu in all subsequent function calls.
 *  -1 is returned if unsuccessful.
 */
int
Minsert(menu)
MENU * menu;
{
	register int i;
	int fg_specified, bg_specified;

	for (i = 0; i < MAXMEN; i++)
		if (_menu_[i].om_root == NULL)
			break;
	if (i == MAXMEN) {
		menerr = MTABFL;
		return(-1);
	}

	Mget_menu_colors(menu->me_cap,
			&menu->me_fg_colour, &menu->me_bg_colour,
			 &fg_specified, &bg_specified);
	Mpropagate_colors(menu, menu->me_fg_colour, menu->me_bg_colour);
	rplink(menu,(MENU *)NULL);

	_menu_[i].om_root = menu;
	_menu_[i].om_last = NULL;
	_menu_[i].om_md = i;
	_menu_[i].om_font[0] =
	_menu_[i].om_font[1] =
	_menu_[i].om_font[2] = 
	_menu_[i].om_font[3] = Mstdfont();
	_menu_[i].om_fback[0] = MH_GREY;
	_menu_[i].om_fback[1] = MH_BLACK;
	_menu_[i].om_fback[2] = MH_GREY;
	_menu_[i].om_wantbox = 1;
	momen(_menu_[i].om_root,&_menu_[i]);
	menerr = MENOK;
	return(i);
}

static void
rplink(mp,parent)
MENU *mp, *parent;
{
	if (mp == NULL)
		return;
	rplink(mp->me_topleft,mp);
	rplink(mp->me_botrite,mp);
	mp->me_parent = parent;
	mp->me_flags &= ~(ME_FREC | ME_FREN);
}

/*  Link submd in as a popup submenu of mainmd under the button with
 *  return value rv. If the covering button has a popup menu 
 *  already then its position and attributes will be transfered to the
 *  new submenu. If the covering button does not have a submenu then
 *  the position of the submenu is determined by the triple xoff, yoff
 *  and mlstyle. xoff and yoff give the offset of the popup menu relative
 *  to the pressed button and mlstyle describes what is offset relative to
 *  what. If mlstyle is an |'rd combination of the following
 *
 *	ME_MLBUTR	use the right side of the button
 *	ME_MLBUTB	use the bottom of the button
 *	ME_MLPOPR	use the right side of the pop-up
 *	ME_MLPOPB	use the bottom of the pop-up
 *	ME_MLCREL	make the pop-up cursor relative
 */
int
Mlink(mainmd,submd,rv,xoff,yoff,mlstyle)
int mainmd, submd, rv,xoff,yoff,mlstyle;
{
	int x, y;
	MENU *mb, *ms;

	if ((mainmd < 0) || (mainmd >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
	if (_menu_[mainmd].om_root == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
	if ((submd < 0) || (submd >= MAXMEN)) {
		menerr = MBADMD;
		return(-1);
	}
	if ((ms = _menu_[submd].om_root) == NULL) {
		menerr = MNOTMEN;
		return(-1);
	}
	if ((mb = mrvfind(_menu_[mainmd].om_root,rv)) == NULL) {
		menerr = MBADARG;
		return(-1);
	}
	if ((mb->me_flags & ME_ISUB) && (mb->me_topleft != NULL)) {
		/*  The button already has a submenu so use its
		 *  position for the new submenu
		 */
		ms->me_xcurs = mb->me_topleft->me_xcurs;
		ms->me_ycurs = mb->me_topleft->me_ycurs;
		mshift(ms,mb->me_topleft->me_xstart,mb->me_topleft->me_ystart,1);
	} else if (mlstyle & ME_MLCREL) {
		ms->me_xcurs = xoff;
		ms->me_ycurs = yoff;
		mb->me_flags |= ME_CREL;
	} else {
    		x = xoff + (mlstyle & ME_MLBUTR ? mb->me_xend : mb->me_xstart);
		if (mlstyle & ME_MLPOPR)
			x -= ms->me_xend - ms->me_xstart;
    		y = yoff + (mlstyle & ME_MLBUTB ? mb->me_yend : mb->me_ystart);
		if (mlstyle & ME_MLPOPB)
			y -= ms->me_yend - ms->me_ystart;
		mshift(ms,x,y,1);
	}
	mb->me_flags |= ME_ISUB|ME_POPUP;
	ms->me_flags |= ME_FREE;
	mb->me_topleft = ms;
	ms->me_parent = mb;
	momen(_menu_[mainmd].om_root,&_menu_[mainmd]);
	return(0);
}

/*  Find the menu button with the given return value.
 */
static MENU *
mrvfind(m_main,rv)
MENU *m_main;
int rv;
{
	MENU *ms;

	if (m_main == NULL)
		return(NULL);
	if (m_main->me_rv == rv && m_main->me_cap != NULL)
		return(m_main);
	if ((ms = mrvfind(m_main->me_topleft,rv)) != NULL)
		return(ms);
	if ((ms = mrvfind(m_main->me_botrite,rv)) != NULL)
		return(ms);
	return(NULL);
}

/*  Dump the menu on the standard output. Used for debugging only.
 */
void
Mdump(md)
int md;
{
	mdump(_menu_[md].om_root,0);
}

static void
mdump(m,indent)
MENU *m;
int indent;
{
	int i;

	for (i = 0; i < indent; i++)
		putchar(' ');
	if (m == NULL) {
		printf("NULL\n");
		return;
	}
	printf("%-20s %x %x(%c) (%d,%d) (%d,%d) (%d,%d)\n",
		m->me_cap,
		m->me_flags,
		m->me_rv,m->me_rv & 0177,
		m->me_xstart,m->me_ystart,
		m->me_xend,m->me_yend,
		m->me_xcurs,m->me_ycurs);
	mdump(m->me_topleft,indent + 5);
	mdump(m->me_botrite,indent + 5);
}

