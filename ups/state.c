/* state.c - set/get ups state variables */

/*  Copyright 1991 Mark Russell, University of Kent at Canterbury.
 *  All Rights Reserved.
 *
 *  This file is part of UPS.
 *
 *  UPS is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  UPS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with UPS; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* @(#)state.c	1.13 09 Apr 1995 (UKC) */
char ups_state_c_rcsid[] = "$Id$";

#include <local/wn.h>
#include <local/menu3.h>

#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "srcwin.h"
#include "reg.h"
#include "target.h"
#include "cc.h"
#include "state.h"

static struct {
	int st_message_wn;
	int st_display_area_wn;
	target_menu_info_t st_target_menu_info;
	int st_start_target_menu;
	int st_restart_target_menu;
	Region *st_current_srcwin_region;
	int st_current_srcwin_menu;
	Outwin *st_current_outwin;
	Region *st_display_area_region;
	Region *st_typing_line_region;
	Region *st_extended_typing_line_region;
	Region *st_dynamic_menu_region;
	target_t *st_current_target;
	Outwin *st_display_area_overlay;
	ccstate_t *st_cs;
} State;

void
set_ccstate(cs)
ccstate_t *cs;
{
	State.st_cs = cs;
}

ccstate_t *
get_ccstate()
{
	return State.st_cs;
}

void
set_target_menu_info(tm)
target_menu_info_t *tm;
{
	State.st_target_menu_info = *tm;
}

target_menu_info_t *
get_target_menu_info()
{
	return &State.st_target_menu_info;
}

void
set_start_target_menu(md)
int md;
{
	State.st_start_target_menu = md;
}

int
get_start_target_menu()
{
	return State.st_start_target_menu;
}

void
set_restart_target_menu(md)
int md;
{
	State.st_restart_target_menu = md;
}

int
get_restart_target_menu()
{
	return State.st_restart_target_menu;
}

void
set_message_wn(wn)
int wn;
{
	State.st_message_wn = wn;
}

int
get_message_wn()
{
	return State.st_message_wn;
}

void
set_current_srcwin_region(region)
Region *region;
{
	State.st_current_srcwin_region = region;
}

Region *
get_current_srcwin_region()
{
	return State.st_current_srcwin_region;
}

void
set_current_srcwin_menu(menu)
int menu;
{
	State.st_current_srcwin_menu = menu;
}

int
get_current_srcwin_menu()
{
	return State.st_current_srcwin_menu;
}

void
set_current_outwin(ow)
Outwin *ow;
{
	State.st_current_outwin = ow;
}

Outwin *
get_current_outwin()
{
	return State.st_current_outwin;
}

void
set_display_area_overlay(dw)
Outwin *dw;
{
	State.st_display_area_overlay = dw;
}

Outwin *
get_display_area_overlay()
{
	return State.st_display_area_overlay;
}

void
set_display_area_region(region)
Region *region;
{
	State.st_display_area_region = region;
}

Region *
get_display_area_region()
{
	return State.st_display_area_region;
}

void
set_typing_line_region(region)
Region *region;
{
	State.st_typing_line_region = region;
}

Region *
get_typing_line_region()
{
	return State.st_typing_line_region;
}

/*  These should set and get the region for the typing line
**  extended by the history and enter buttons
*/
void
set_extended_typing_line_region(region)
Region *region;
{
	State.st_extended_typing_line_region = region;
}

Region *
get_extended_typing_line_region()
{
	return State.st_extended_typing_line_region
	       ? State.st_extended_typing_line_region
	       : State.st_typing_line_region;
}

Srcwin *
get_current_srcwin()
{
	if (State.st_current_srcwin_region == NULL)
		return NULL;	/* -nowindow */
	
	return (Srcwin *)re_get_data(State.st_current_srcwin_region);
}

void
set_dynamic_menu_region(region)
Region *region;
{
	State.st_dynamic_menu_region = region;
}

Region *
get_dynamic_menu_region()
{
	return State.st_dynamic_menu_region;
}

void
set_current_target(xp)
target_t *xp;
{
	State.st_current_target = xp;
}

target_t *
get_current_target()
{
	return State.st_current_target;
}
