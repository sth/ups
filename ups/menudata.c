/* menu_data.c - data defining menus (automatically generated - don't edit) */

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


char ups_menu_data_c_rcsid[] = "%W% %G%";

#include <local/wn.h>
#include <local/menu3.h>


/* --- Cut here for blockmen.c --- */

static MENU blockmen_MM1 = {
	ME_POPUP+ME_FREC,
	0,	/* me_pos */
	"Expand",
	'e',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	107,	/* me_xend */
	41,	/* me_yend */
	1712,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU blockmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Collapse",
	'c',	/* me_rv */
	107,	/* me_xstart */
	-1,	/* me_ystart */
	216,	/* me_xend */
	41,	/* me_yend */
	1712,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU blockmen_MM3 = {
	ME_HOR,
	107,	/* me_pos */
	NULL,	/* me_cap */
	'e',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	216,	/* me_xend */
	41,	/* me_yend */
	1712,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&blockmen_MM1,	/* me_topleft */
	&blockmen_MM2,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU blockmen_MM4 = {
	ME_FREC,
	0,	/* me_pos */
	"Delete",
	'd',	/* me_rv */
	216,	/* me_xstart */
	-1,	/* me_ystart */
	324,	/* me_xend */
	41,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU blockmen_MM5 = {
	ME_FREC,
	0,	/* me_pos */
	"Add expr",
	'x',	/* me_rv */
	324,	/* me_xstart */
	-1,	/* me_ystart */
	433,	/* me_xend */
	41,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU blockmen_MM6 = {
	ME_FREC,
	0,	/* me_pos */
	"Source",
	's',	/* me_rv */
	433,	/* me_xstart */
	-1,	/* me_ystart */
	542,	/* me_xend */
	41,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU blockmen_MM7 = {
	ME_HOR,
	433,	/* me_pos */
	NULL,	/* me_cap */
	'x',	/* me_rv */
	324,	/* me_xstart */
	-1,	/* me_ystart */
	542,	/* me_xend */
	41,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&blockmen_MM5,	/* me_topleft */
	&blockmen_MM6,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU blockmen_MM8 = {
	ME_HOR,
	324,	/* me_pos */
	NULL,	/* me_cap */
	'd',	/* me_rv */
	216,	/* me_xstart */
	-1,	/* me_ystart */
	542,	/* me_xend */
	41,	/* me_yend */
	1712,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&blockmen_MM4,	/* me_topleft */
	&blockmen_MM7,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU block_men = {
	ME_HOR+ME_FREE,
	216,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	542,	/* me_xend */
	41,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&blockmen_MM3,	/* me_topleft */
	&blockmen_MM8,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for bphmen.c --- */

static MENU bphmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Add new",
	'a',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	90,	/* me_xend */
	97,	/* me_yend */
	1832,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bphmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Remove all",
	'D',	/* me_rv */
	90,	/* me_xstart */
	-1,	/* me_ystart */
	211,	/* me_xend */
	97,	/* me_yend */
	1832,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bphmen_MM3 = {
	ME_FREC,
	0,	/* me_pos */
	"Restore",
	'r',	/* me_rv */
	211,	/* me_xstart */
	-1,	/* me_ystart */
	324,	/* me_xend */
	97,	/* me_yend */
	1832,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bphmen_MM4 = {
	ME_HOR,
	211,	/* me_pos */
	NULL,	/* me_cap */
	'D',	/* me_rv */
	90,	/* me_xstart */
	-1,	/* me_ystart */
	324,	/* me_xend */
	97,	/* me_yend */
	1832,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&bphmen_MM2,	/* me_topleft */
	&bphmen_MM3,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bphmen_MM5 = {
	ME_HOR,
	90,	/* me_pos */
	NULL,	/* me_cap */
	'a',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	324,	/* me_xend */
	97,	/* me_yend */
	1832,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&bphmen_MM1,	/* me_topleft */
	&bphmen_MM4,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bphmen_MM6 = {
	ME_FREC,
	0,	/* me_pos */
	"Load - bpts saved in statefile",
	'm',	/* me_rv */
	366,	/* me_xstart */
	49,	/* me_ystart */
	744,	/* me_xend */
	147,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bphmen_MM7 = {
	ME_FREC,
	0,	/* me_pos */
	"Load - bpts NOT saved in statefile",
	'l',	/* me_rv */
	366,	/* me_xstart */
	147,	/* me_ystart */
	744,	/* me_xend */
	250,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bphmen_MM8 = {
	ME_VER+ME_FREE,
	147,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	366,	/* me_xstart */
	49,	/* me_ystart */
	744,	/* me_xend */
	250,	/* me_yend */
	-68,	/* me_xcurs */
	-48,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&bphmen_MM6,	/* me_topleft */
	&bphmen_MM7,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bphmen_MM9 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_CREL,
	0,	/* me_pos */
	"Load file",
	'L',	/* me_rv */
	324,	/* me_xstart */
	-1,	/* me_ystart */
	434,	/* me_xend */
	97,	/* me_yend */
	1832,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&bphmen_MM8,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bphmen_MM10 = {
	ME_FREC,
	0,	/* me_pos */
	"Enable",
	'e',	/* me_rv */
	434,	/* me_xstart */
	-1,	/* me_ystart */
	517,	/* me_xend */
	97,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bphmen_MM11 = {
	ME_FREC,
	0,	/* me_pos */
	"Disable",
	'd',	/* me_rv */
	517,	/* me_xstart */
	-1,	/* me_ystart */
	609,	/* me_xend */
	97,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bphmen_MM12 = {
	ME_HOR,
	517,	/* me_pos */
	NULL,	/* me_cap */
	'e',	/* me_rv */
	434,	/* me_xstart */
	-1,	/* me_ystart */
	609,	/* me_xend */
	97,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&bphmen_MM10,	/* me_topleft */
	&bphmen_MM11,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bphmen_MM13 = {
	ME_HOR,
	434,	/* me_pos */
	NULL,	/* me_cap */
	'L',	/* me_rv */
	324,	/* me_xstart */
	-1,	/* me_ystart */
	609,	/* me_xend */
	97,	/* me_yend */
	1832,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&bphmen_MM9,	/* me_topleft */
	&bphmen_MM12,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU bph_men = {
	ME_HOR+ME_FREE,
	324,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	609,	/* me_xend */
	97,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&bphmen_MM5,	/* me_topleft */
	&bphmen_MM13,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for bptmen.c --- */

static MENU bptmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Remove",
	'd',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	82,	/* me_xend */
	40,	/* me_yend */
	1520,	/* me_xcurs */
	5,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bptmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Source",
	'e',	/* me_rv */
	82,	/* me_xstart */
	-1,	/* me_ystart */
	170,	/* me_xend */
	40,	/* me_yend */
	1520,	/* me_xcurs */
	5,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bptmen_MM3 = {
	ME_FREC,
	0,	/* me_pos */
	"Save",
	's',	/* me_rv */
	170,	/* me_xstart */
	-1,	/* me_ystart */
	248,	/* me_xend */
	40,	/* me_yend */
	1520,	/* me_xcurs */
	5,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bptmen_MM4 = {
	ME_HOR,
	170,	/* me_pos */
	NULL,	/* me_cap */
	'e',	/* me_rv */
	82,	/* me_xstart */
	-1,	/* me_ystart */
	248,	/* me_xend */
	40,	/* me_yend */
	1520,	/* me_xcurs */
	5,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&bptmen_MM2,	/* me_topleft */
	&bptmen_MM3,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bptmen_MM5 = {
	ME_HOR,
	82,	/* me_pos */
	NULL,	/* me_cap */
	'd',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	248,	/* me_xend */
	40,	/* me_yend */
	1520,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&bptmen_MM1,	/* me_topleft */
	&bptmen_MM4,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bptmen_MM6 = {
	ME_FREC,
	0,	/* me_pos */
	"Activate",
	'x',	/* me_rv */
	248,	/* me_xstart */
	-1,	/* me_ystart */
	350,	/* me_xend */
	40,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bptmen_MM7 = {
	ME_FREC,
	0,	/* me_pos */
	"Inactivate",
	'y',	/* me_rv */
	350,	/* me_xstart */
	-1,	/* me_ystart */
	467,	/* me_xend */
	40,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bptmen_MM8 = {
	ME_FREC,
	0,	/* me_pos */
	"Execute",
	'z',	/* me_rv */
	467,	/* me_xstart */
	-1,	/* me_ystart */
	557,	/* me_xend */
	40,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bptmen_MM9 = {
	ME_HOR,
	467,	/* me_pos */
	NULL,	/* me_cap */
	'y',	/* me_rv */
	350,	/* me_xstart */
	-1,	/* me_ystart */
	557,	/* me_xend */
	40,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&bptmen_MM7,	/* me_topleft */
	&bptmen_MM8,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU bptmen_MM10 = {
	ME_HOR,
	350,	/* me_pos */
	NULL,	/* me_cap */
	'x',	/* me_rv */
	248,	/* me_xstart */
	-1,	/* me_ystart */
	557,	/* me_xend */
	40,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&bptmen_MM6,	/* me_topleft */
	&bptmen_MM9,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU bpt_men = {
	ME_HOR+ME_FREE,
	248,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	557,	/* me_xend */
	40,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&bptmen_MM5,	/* me_topleft */
	&bptmen_MM10,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for cbhmen.c --- */

static MENU cbhmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Expand",
	'e',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	300,	/* me_xend */
	47,	/* me_yend */
	1352,	/* me_xcurs */
	-1,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cbhmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Collapse",
	'c',	/* me_rv */
	300,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	47,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU cbh_men = {
	ME_HOR+ME_FREE,
	300,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	47,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cbhmen_MM1,	/* me_topleft */
	&cbhmen_MM2,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for cblockmen.c --- */

static MENU cblockmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Like before",
	'e',	/* me_rv */
	175,	/* me_xstart */
	34,	/* me_ystart */
	329,	/* me_xend */
	84,	/* me_yend */
	1528,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cblockmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Completely",
	'E',	/* me_rv */
	175,	/* me_xstart */
	84,	/* me_ystart */
	329,	/* me_xend */
	134,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cblockmen_MM3 = {
	ME_VER+ME_FREE,
	84,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	175,	/* me_xstart */
	34,	/* me_ystart */
	329,	/* me_xend */
	134,	/* me_yend */
	-24,	/* me_xcurs */
	-13,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cblockmen_MM1,	/* me_topleft */
	&cblockmen_MM2,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cblockmen_MM4 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_CREL,
	0,	/* me_pos */
	"Expand",
	'e',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	199,	/* me_xend */
	47,	/* me_yend */
	1528,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cblockmen_MM3,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cblockmen_MM5 = {
	ME_FREC,
	0,	/* me_pos */
	"Collapse",
	'c',	/* me_rv */
	199,	/* me_xstart */
	-1,	/* me_ystart */
	399,	/* me_xend */
	47,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cblockmen_MM6 = {
	ME_FREC,
	0,	/* me_pos */
	"Hide",
	'h',	/* me_rv */
	399,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	47,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cblockmen_MM7 = {
	ME_HOR,
	399,	/* me_pos */
	NULL,	/* me_cap */
	'c',	/* me_rv */
	199,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	47,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cblockmen_MM5,	/* me_topleft */
	&cblockmen_MM6,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU cblock_men = {
	ME_HOR+ME_FREE,
	199,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	47,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cblockmen_MM4,	/* me_topleft */
	&cblockmen_MM7,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for cmdmen.c --- */

static MENU cmdmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Web site URL",
	'o',	/* me_rv */
	55,	/* me_xstart */
	21,	/* me_ystart */
	350,	/* me_xend */
	55,	/* me_yend */
	1064,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"cut and paste",
	'0',	/* me_rv */
	55,	/* me_xstart */
	55,	/* me_ystart */
	350,	/* me_xend */
	90,	/* me_yend */
	1064,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM3 = {
	ME_VER,
	55,	/* me_pos */
	NULL,	/* me_cap */
	'o',	/* me_rv */
	55,	/* me_xstart */
	21,	/* me_ystart */
	350,	/* me_xend */
	90,	/* me_yend */
	1064,	/* me_xcurs */
	-13,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM1,	/* me_topleft */
	&cmdmen_MM2,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM4 = {
	ME_FREC,
	0,	/* me_pos */
	"saving state",
	't',	/* me_rv */
	55,	/* me_xstart */
	90,	/* me_ystart */
	350,	/* me_xend */
	125,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM5 = {
	ME_FREC,
	0,	/* me_pos */
	"in source",
	'A',	/* me_rv */
	316,	/* me_xstart */
	125,	/* me_ystart */
	588,	/* me_xend */
	159,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM6 = {
	ME_FREC,
	0,	/* me_pos */
	"by name",
	'B',	/* me_rv */
	316,	/* me_xstart */
	159,	/* me_ystart */
	588,	/* me_xend */
	194,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM7 = {
	ME_VER,
	159,	/* me_pos */
	NULL,	/* me_cap */
	'A',	/* me_rv */
	316,	/* me_xstart */
	125,	/* me_ystart */
	588,	/* me_xend */
	194,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM5,	/* me_topleft */
	&cmdmen_MM6,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM8 = {
	ME_FREC,
	0,	/* me_pos */
	"conditional",
	'd',	/* me_rv */
	316,	/* me_xstart */
	194,	/* me_ystart */
	588,	/* me_xend */
	228,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM9 = {
	ME_FREC,
	0,	/* me_pos */
	"saving breakpoints",
	'u',	/* me_rv */
	316,	/* me_xstart */
	228,	/* me_ystart */
	588,	/* me_xend */
	263,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM10 = {
	ME_FREC,
	0,	/* me_pos */
	"C++ syntax",
	'C',	/* me_rv */
	316,	/* me_xstart */
	263,	/* me_ystart */
	588,	/* me_xend */
	298,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM11 = {
	ME_VER,
	263,	/* me_pos */
	NULL,	/* me_cap */
	'u',	/* me_rv */
	316,	/* me_xstart */
	228,	/* me_ystart */
	588,	/* me_xend */
	298,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM9,	/* me_topleft */
	&cmdmen_MM10,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM12 = {
	ME_VER,
	228,	/* me_pos */
	NULL,	/* me_cap */
	'd',	/* me_rv */
	316,	/* me_xstart */
	194,	/* me_ystart */
	588,	/* me_xend */
	298,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM8,	/* me_topleft */
	&cmdmen_MM11,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM13 = {
	ME_VER,
	194,	/* me_pos */
	NULL,	/* me_cap */
	'A',	/* me_rv */
	316,	/* me_xstart */
	125,	/* me_ystart */
	588,	/* me_xend */
	298,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM7,	/* me_topleft */
	&cmdmen_MM12,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM14 = {
	ME_FREC,
	0,	/* me_pos */
	"auto completion",
	'D',	/* me_rv */
	316,	/* me_xstart */
	298,	/* me_ystart */
	588,	/* me_xend */
	332,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM15 = {
	ME_FREC,
	0,	/* me_pos */
	"listing matches",
	'E',	/* me_rv */
	316,	/* me_xstart */
	332,	/* me_ystart */
	588,	/* me_xend */
	367,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM16 = {
	ME_VER,
	332,	/* me_pos */
	NULL,	/* me_cap */
	'D',	/* me_rv */
	316,	/* me_xstart */
	298,	/* me_ystart */
	588,	/* me_xend */
	367,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM14,	/* me_topleft */
	&cmdmen_MM15,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM17 = {
	ME_FREC+ME_NOSCALE,
	0,	/* me_pos */
	"activation",
	'F',	/* me_rv */
	316,	/* me_xstart */
	367,	/* me_ystart */
	588,	/* me_xend */
	401,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM18 = {
	ME_FREC,
	0,	/* me_pos */
	"execute",
	'G',	/* me_rv */
	316,	/* me_xstart */
	401,	/* me_ystart */
	588,	/* me_xend */
	436,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM19 = {
	ME_FREC,
	0,	/* me_pos */
	"duplicates",
	'H',	/* me_rv */
	316,	/* me_xstart */
	436,	/* me_ystart */
	588,	/* me_xend */
	471,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM20 = {
	ME_VER,
	436,	/* me_pos */
	NULL,	/* me_cap */
	'G',	/* me_rv */
	316,	/* me_xstart */
	401,	/* me_ystart */
	588,	/* me_xend */
	471,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM18,	/* me_topleft */
	&cmdmen_MM19,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM21 = {
	ME_VER,
	401,	/* me_pos */
	NULL,	/* me_cap */
	'F',	/* me_rv */
	316,	/* me_xstart */
	367,	/* me_ystart */
	588,	/* me_xend */
	471,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM17,	/* me_topleft */
	&cmdmen_MM20,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM22 = {
	ME_VER,
	367,	/* me_pos */
	NULL,	/* me_cap */
	'D',	/* me_rv */
	316,	/* me_xstart */
	298,	/* me_ystart */
	588,	/* me_xend */
	471,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM16,	/* me_topleft */
	&cmdmen_MM21,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM23 = {
	ME_VER+ME_FREE,
	298,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	316,	/* me_xstart */
	125,	/* me_ystart */
	588,	/* me_xend */
	471,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM13,	/* me_topleft */
	&cmdmen_MM22,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM24 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"breakpoints",
	'1',	/* me_rv */
	55,	/* me_xstart */
	125,	/* me_ystart */
	350,	/* me_xend */
	159,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM23,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM25 = {
	ME_VER,
	125,	/* me_pos */
	NULL,	/* me_cap */
	't',	/* me_rv */
	55,	/* me_xstart */
	90,	/* me_ystart */
	350,	/* me_xend */
	159,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM4,	/* me_topleft */
	&cmdmen_MM24,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM26 = {
	ME_VER,
	90,	/* me_pos */
	NULL,	/* me_cap */
	'o',	/* me_rv */
	55,	/* me_xstart */
	21,	/* me_ystart */
	350,	/* me_xend */
	159,	/* me_yend */
	1064,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM3,	/* me_topleft */
	&cmdmen_MM25,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM27 = {
	ME_FREC,
	0,	/* me_pos */
	"accelerators",
	'2',	/* me_rv */
	55,	/* me_xstart */
	159,	/* me_ystart */
	350,	/* me_xend */
	194,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM28 = {
	ME_FREC,
	0,	/* me_pos */
	"typing shortcuts",
	'p',	/* me_rv */
	55,	/* me_xstart */
	194,	/* me_ystart */
	350,	/* me_xend */
	229,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM29 = {
	ME_VER,
	194,	/* me_pos */
	NULL,	/* me_cap */
	'2',	/* me_rv */
	55,	/* me_xstart */
	159,	/* me_ystart */
	350,	/* me_xend */
	229,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM27,	/* me_topleft */
	&cmdmen_MM28,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM30 = {
	ME_FREC,
	0,	/* me_pos */
	"general",
	'I',	/* me_rv */
	319,	/* me_xstart */
	229,	/* me_ystart */
	589,	/* me_xend */
	263,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM31 = {
	ME_FREC,
	0,	/* me_pos */
	"step button",
	'J',	/* me_rv */
	319,	/* me_xstart */
	263,	/* me_ystart */
	589,	/* me_xend */
	298,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM32 = {
	ME_FREC,
	0,	/* me_pos */
	"casting",
	'K',	/* me_rv */
	319,	/* me_xstart */
	298,	/* me_ystart */
	589,	/* me_xend */
	333,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM33 = {
	ME_VER,
	298,	/* me_pos */
	NULL,	/* me_cap */
	'J',	/* me_rv */
	319,	/* me_xstart */
	263,	/* me_ystart */
	589,	/* me_xend */
	333,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM31,	/* me_topleft */
	&cmdmen_MM32,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM34 = {
	ME_VER,
	263,	/* me_pos */
	NULL,	/* me_cap */
	'I',	/* me_rv */
	319,	/* me_xstart */
	229,	/* me_ystart */
	589,	/* me_xend */
	333,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM30,	/* me_topleft */
	&cmdmen_MM33,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM35 = {
	ME_FREC,
	0,	/* me_pos */
	"tags",
	'L',	/* me_rv */
	319,	/* me_xstart */
	333,	/* me_ystart */
	589,	/* me_xend */
	367,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM36 = {
	ME_FREC,
	0,	/* me_pos */
	"base classes",
	'n',	/* me_rv */
	319,	/* me_xstart */
	367,	/* me_ystart */
	589,	/* me_xend */
	402,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM37 = {
	ME_FREC,
	0,	/* me_pos */
	"breakpoint code",
	'M',	/* me_rv */
	319,	/* me_xstart */
	402,	/* me_ystart */
	589,	/* me_xend */
	437,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM38 = {
	ME_VER,
	402,	/* me_pos */
	NULL,	/* me_cap */
	'n',	/* me_rv */
	319,	/* me_xstart */
	367,	/* me_ystart */
	589,	/* me_xend */
	437,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM36,	/* me_topleft */
	&cmdmen_MM37,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM39 = {
	ME_VER,
	367,	/* me_pos */
	NULL,	/* me_cap */
	'L',	/* me_rv */
	319,	/* me_xstart */
	333,	/* me_ystart */
	589,	/* me_xend */
	437,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM35,	/* me_topleft */
	&cmdmen_MM38,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM40 = {
	ME_VER+ME_FREE,
	333,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	319,	/* me_xstart */
	229,	/* me_ystart */
	589,	/* me_xend */
	437,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM34,	/* me_topleft */
	&cmdmen_MM39,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM41 = {
	ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"C++ support",
	'3',	/* me_rv */
	55,	/* me_xstart */
	229,	/* me_ystart */
	350,	/* me_xend */
	263,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM40,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM42 = {
	ME_FREC,
	0,	/* me_pos */
	"source files",
	'4',	/* me_rv */
	55,	/* me_xstart */
	263,	/* me_ystart */
	350,	/* me_xend */
	298,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM43 = {
	ME_FREC,
	0,	/* me_pos */
	"general",
	'S',	/* me_rv */
	303,	/* me_xstart */
	298,	/* me_ystart */
	536,	/* me_xend */
	332,	/* me_yend */
	0,	/* me_xcurs */
	628,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM44 = {
	ME_FREC,
	0,	/* me_pos */
	"pointers",
	'T',	/* me_rv */
	303,	/* me_xstart */
	332,	/* me_ystart */
	536,	/* me_xend */
	367,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM45 = {
	ME_VER,
	332,	/* me_pos */
	NULL,	/* me_cap */
	'S',	/* me_rv */
	303,	/* me_xstart */
	298,	/* me_ystart */
	536,	/* me_xend */
	367,	/* me_yend */
	0,	/* me_xcurs */
	285,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM43,	/* me_topleft */
	&cmdmen_MM44,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM46 = {
	ME_FREC,
	0,	/* me_pos */
	"arrays",
	'U',	/* me_rv */
	303,	/* me_xstart */
	367,	/* me_ystart */
	536,	/* me_xend */
	402,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM47 = {
	ME_FREC,
	0,	/* me_pos */
	"structures",
	'V',	/* me_rv */
	303,	/* me_xstart */
	402,	/* me_ystart */
	536,	/* me_xend */
	436,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM48 = {
	ME_VER,
	402,	/* me_pos */
	NULL,	/* me_cap */
	'U',	/* me_rv */
	303,	/* me_xstart */
	367,	/* me_ystart */
	536,	/* me_xend */
	436,	/* me_yend */
	0,	/* me_xcurs */
	663,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM46,	/* me_topleft */
	&cmdmen_MM47,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM49 = {
	ME_VER,
	367,	/* me_pos */
	NULL,	/* me_cap */
	'S',	/* me_rv */
	303,	/* me_xstart */
	298,	/* me_ystart */
	536,	/* me_xend */
	436,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM45,	/* me_topleft */
	&cmdmen_MM48,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM50 = {
	ME_FREC,
	0,	/* me_pos */
	"linked data",
	'W',	/* me_rv */
	303,	/* me_xstart */
	436,	/* me_ystart */
	536,	/* me_xend */
	471,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM51 = {
	ME_FREC,
	0,	/* me_pos */
	"changing values",
	'X',	/* me_rv */
	303,	/* me_xstart */
	471,	/* me_ystart */
	536,	/* me_xend */
	506,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM52 = {
	ME_VER,
	471,	/* me_pos */
	NULL,	/* me_cap */
	'W',	/* me_rv */
	303,	/* me_xstart */
	436,	/* me_ystart */
	536,	/* me_xend */
	506,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM50,	/* me_topleft */
	&cmdmen_MM51,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM53 = {
	ME_FREC,
	0,	/* me_pos */
	"typedefs",
	'Y',	/* me_rv */
	303,	/* me_xstart */
	506,	/* me_ystart */
	536,	/* me_xend */
	540,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM54 = {
	ME_FREC,
	0,	/* me_pos */
	"expressions",
	'Z',	/* me_rv */
	303,	/* me_xstart */
	540,	/* me_ystart */
	536,	/* me_xend */
	575,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM55 = {
	ME_FREC,
	0,	/* me_pos */
	"dump memory",
	'a',	/* me_rv */
	303,	/* me_xstart */
	575,	/* me_ystart */
	536,	/* me_xend */
	610,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM56 = {
	ME_VER,
	575,	/* me_pos */
	NULL,	/* me_cap */
	'Z',	/* me_rv */
	303,	/* me_xstart */
	540,	/* me_ystart */
	536,	/* me_xend */
	610,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM54,	/* me_topleft */
	&cmdmen_MM55,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM57 = {
	ME_VER,
	540,	/* me_pos */
	NULL,	/* me_cap */
	'Y',	/* me_rv */
	303,	/* me_xstart */
	506,	/* me_ystart */
	536,	/* me_xend */
	610,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM53,	/* me_topleft */
	&cmdmen_MM56,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM58 = {
	ME_VER,
	506,	/* me_pos */
	NULL,	/* me_cap */
	'W',	/* me_rv */
	303,	/* me_xstart */
	436,	/* me_ystart */
	536,	/* me_xend */
	610,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM52,	/* me_topleft */
	&cmdmen_MM57,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM59 = {
	ME_VER+ME_FREE,
	436,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	303,	/* me_xstart */
	298,	/* me_ystart */
	536,	/* me_xend */
	610,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM49,	/* me_topleft */
	&cmdmen_MM58,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM60 = {
	ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"examing data",
	'5',	/* me_rv */
	55,	/* me_xstart */
	298,	/* me_ystart */
	350,	/* me_xend */
	333,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM59,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM61 = {
	ME_VER,
	298,	/* me_pos */
	NULL,	/* me_cap */
	'4',	/* me_rv */
	55,	/* me_xstart */
	263,	/* me_ystart */
	350,	/* me_xend */
	333,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM42,	/* me_topleft */
	&cmdmen_MM60,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM62 = {
	ME_VER,
	263,	/* me_pos */
	NULL,	/* me_cap */
	'3',	/* me_rv */
	55,	/* me_xstart */
	229,	/* me_ystart */
	350,	/* me_xend */
	333,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM41,	/* me_topleft */
	&cmdmen_MM61,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM63 = {
	ME_VER,
	229,	/* me_pos */
	NULL,	/* me_cap */
	'2',	/* me_rv */
	55,	/* me_xstart */
	159,	/* me_ystart */
	350,	/* me_xend */
	333,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM29,	/* me_topleft */
	&cmdmen_MM62,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM64 = {
	ME_VER,
	159,	/* me_pos */
	NULL,	/* me_cap */
	'o',	/* me_rv */
	55,	/* me_xstart */
	21,	/* me_ystart */
	350,	/* me_xend */
	333,	/* me_yend */
	1064,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM26,	/* me_topleft */
	&cmdmen_MM63,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM65 = {
	ME_FREC,
	0,	/* me_pos */
	"general",
	'6',	/* me_rv */
	305,	/* me_xstart */
	333,	/* me_ystart */
	539,	/* me_xend */
	367,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM66 = {
	ME_FREC,
	0,	/* me_pos */
	"special characters",
	'e',	/* me_rv */
	305,	/* me_xstart */
	367,	/* me_ystart */
	539,	/* me_xend */
	402,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM67 = {
	ME_FREC,
	0,	/* me_pos */
	"edit history",
	'i',	/* me_rv */
	305,	/* me_xstart */
	402,	/* me_ystart */
	539,	/* me_xend */
	437,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM68 = {
	ME_VER,
	402,	/* me_pos */
	NULL,	/* me_cap */
	'e',	/* me_rv */
	305,	/* me_xstart */
	367,	/* me_ystart */
	539,	/* me_xend */
	437,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM66,	/* me_topleft */
	&cmdmen_MM67,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM69 = {
	ME_VER+ME_FREE,
	367,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	305,	/* me_xstart */
	333,	/* me_ystart */
	539,	/* me_xend */
	437,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM65,	/* me_topleft */
	&cmdmen_MM68,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM70 = {
	ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"editable fields",
	0,	/* me_rv */
	55,	/* me_xstart */
	333,	/* me_ystart */
	350,	/* me_xend */
	367,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM69,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM71 = {
	ME_FREC,
	0,	/* me_pos */
	"command arguments",
	'7',	/* me_rv */
	55,	/* me_xstart */
	367,	/* me_ystart */
	350,	/* me_xend */
	402,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM72 = {
	ME_VER,
	367,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	55,	/* me_xstart */
	333,	/* me_ystart */
	350,	/* me_xend */
	402,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM70,	/* me_topleft */
	&cmdmen_MM71,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM73 = {
	ME_FREC,
	0,	/* me_pos */
	"custom menu",
	'8',	/* me_rv */
	55,	/* me_xstart */
	402,	/* me_ystart */
	350,	/* me_xend */
	437,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM74 = {
	ME_FREC,
	0,	/* me_pos */
	"init file",
	'9',	/* me_rv */
	55,	/* me_xstart */
	437,	/* me_ystart */
	350,	/* me_xend */
	471,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM75 = {
	ME_VER,
	437,	/* me_pos */
	NULL,	/* me_cap */
	'8',	/* me_rv */
	55,	/* me_xstart */
	402,	/* me_ystart */
	350,	/* me_xend */
	471,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM73,	/* me_topleft */
	&cmdmen_MM74,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM76 = {
	ME_VER,
	402,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	55,	/* me_xstart */
	333,	/* me_ystart */
	350,	/* me_xend */
	471,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM72,	/* me_topleft */
	&cmdmen_MM75,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM77 = {
	ME_FREC,
	0,	/* me_pos */
	"attach / detach",
	'!',	/* me_rv */
	55,	/* me_xstart */
	471,	/* me_ystart */
	350,	/* me_xend */
	506,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM78 = {
	ME_FREC,
	0,	/* me_pos */
	"signals",
	'h',	/* me_rv */
	55,	/* me_xstart */
	506,	/* me_ystart */
	350,	/* me_xend */
	541,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM79 = {
	ME_VER,
	506,	/* me_pos */
	NULL,	/* me_cap */
	'!',	/* me_rv */
	55,	/* me_xstart */
	471,	/* me_ystart */
	350,	/* me_xend */
	541,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM77,	/* me_topleft */
	&cmdmen_MM78,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM80 = {
	ME_FREC,
	0,	/* me_pos */
	"About X Resources",
	0x100+'c',	/* me_rv */
	305,	/* me_xstart */
	541,	/* me_ystart */
	570,	/* me_xend */
	575,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM81 = {
	ME_FREC,
	0,	/* me_pos */
	"Fonts and Colors",
	0x100+'d',	/* me_rv */
	305,	/* me_xstart */
	575,	/* me_ystart */
	570,	/* me_xend */
	610,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM82 = {
	ME_VER,
	575,	/* me_pos */
	NULL,	/* me_cap */
	0x100+'c',	/* me_rv */
	305,	/* me_xstart */
	541,	/* me_ystart */
	570,	/* me_xend */
	610,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM80,	/* me_topleft */
	&cmdmen_MM81,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM83 = {
	ME_FREC,
	0,	/* me_pos */
	"Split Screens",
	0x100+'e',	/* me_rv */
	305,	/* me_xstart */
	610,	/* me_ystart */
	570,	/* me_xend */
	645,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM84 = {
	ME_FREC,
	0,	/* me_pos */
	"Raise/Lower",
	0x100+'f',	/* me_rv */
	305,	/* me_xstart */
	645,	/* me_ystart */
	570,	/* me_xend */
	680,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM85 = {
	ME_VER,
	645,	/* me_pos */
	NULL,	/* me_cap */
	0x100+'e',	/* me_rv */
	305,	/* me_xstart */
	610,	/* me_ystart */
	570,	/* me_xend */
	680,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM83,	/* me_topleft */
	&cmdmen_MM84,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM86 = {
	ME_VER,
	610,	/* me_pos */
	NULL,	/* me_cap */
	0x100+'c',	/* me_rv */
	305,	/* me_xstart */
	541,	/* me_ystart */
	570,	/* me_xend */
	680,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM82,	/* me_topleft */
	&cmdmen_MM85,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM87 = {
	ME_FREC,
	0,	/* me_pos */
	"Breakpoint Menu",
	0x100+'g',	/* me_rv */
	305,	/* me_xstart */
	680,	/* me_ystart */
	570,	/* me_xend */
	714,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM88 = {
	ME_FREC,
	0,	/* me_pos */
	"Scrolling",
	0x100+'h',	/* me_rv */
	305,	/* me_xstart */
	714,	/* me_ystart */
	570,	/* me_xend */
	749,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM89 = {
	ME_VER,
	714,	/* me_pos */
	NULL,	/* me_cap */
	0x100+'g',	/* me_rv */
	305,	/* me_xstart */
	680,	/* me_ystart */
	570,	/* me_xend */
	749,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM87,	/* me_topleft */
	&cmdmen_MM88,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM90 = {
	ME_FREC,
	0,	/* me_pos */
	"Resizing Windows",
	'g',	/* me_rv */
	305,	/* me_xstart */
	749,	/* me_ystart */
	570,	/* me_xend */
	784,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM91 = {
	ME_FREC,
	0,	/* me_pos */
	"No Mousehole",
	0x100+'i',	/* me_rv */
	305,	/* me_xstart */
	784,	/* me_ystart */
	570,	/* me_xend */
	819,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM92 = {
	ME_VER,
	784,	/* me_pos */
	NULL,	/* me_cap */
	'g',	/* me_rv */
	305,	/* me_xstart */
	749,	/* me_ystart */
	570,	/* me_xend */
	819,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM90,	/* me_topleft */
	&cmdmen_MM91,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM93 = {
	ME_VER,
	749,	/* me_pos */
	NULL,	/* me_cap */
	0x100+'g',	/* me_rv */
	305,	/* me_xstart */
	680,	/* me_ystart */
	570,	/* me_xend */
	819,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM89,	/* me_topleft */
	&cmdmen_MM92,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM94 = {
	ME_VER+ME_FREE,
	680,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	305,	/* me_xstart */
	541,	/* me_ystart */
	570,	/* me_xend */
	819,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM86,	/* me_topleft */
	&cmdmen_MM93,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM95 = {
	ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"preferences",
	0,	/* me_rv */
	55,	/* me_xstart */
	541,	/* me_ystart */
	350,	/* me_xend */
	575,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM94,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM96 = {
	ME_FREC,
	0,	/* me_pos */
	"ups version",
	'v',	/* me_rv */
	312,	/* me_xstart */
	575,	/* me_ystart */
	558,	/* me_xend */
	610,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM97 = {
	ME_FREC,
	0,	/* me_pos */
	"`step' action",
	's',	/* me_rv */
	312,	/* me_xstart */
	610,	/* me_ystart */
	558,	/* me_xend */
	645,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM98 = {
	ME_VER,
	610,	/* me_pos */
	NULL,	/* me_cap */
	'v',	/* me_rv */
	312,	/* me_xstart */
	575,	/* me_ystart */
	558,	/* me_xend */
	645,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM96,	/* me_topleft */
	&cmdmen_MM97,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM99 = {
	ME_FREC,
	0,	/* me_pos */
	"scrollbars",
	'O',	/* me_rv */
	312,	/* me_xstart */
	645,	/* me_ystart */
	558,	/* me_xend */
	680,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM100 = {
	ME_FREC,
	0,	/* me_pos */
	"assembler",
	'P',	/* me_rv */
	312,	/* me_xstart */
	680,	/* me_ystart */
	558,	/* me_xend */
	715,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM101 = {
	ME_FREC,
	0,	/* me_pos */
	"authors",
	'Q',	/* me_rv */
	312,	/* me_xstart */
	715,	/* me_ystart */
	558,	/* me_xend */
	751,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM102 = {
	ME_VER,
	715,	/* me_pos */
	NULL,	/* me_cap */
	'P',	/* me_rv */
	312,	/* me_xstart */
	680,	/* me_ystart */
	558,	/* me_xend */
	751,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM100,	/* me_topleft */
	&cmdmen_MM101,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM103 = {
	ME_VER,
	680,	/* me_pos */
	NULL,	/* me_cap */
	'O',	/* me_rv */
	312,	/* me_xstart */
	645,	/* me_ystart */
	558,	/* me_xend */
	751,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM99,	/* me_topleft */
	&cmdmen_MM102,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM104 = {
	ME_VER+ME_FREE,
	645,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	312,	/* me_xstart */
	575,	/* me_ystart */
	558,	/* me_xend */
	751,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM98,	/* me_topleft */
	&cmdmen_MM103,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM105 = {
	ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"miscellaneous",
	'i',	/* me_rv */
	55,	/* me_xstart */
	575,	/* me_ystart */
	350,	/* me_xend */
	610,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM104,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM106 = {
	ME_FREC,
	0,	/* me_pos */
	"append text",
	'l',	/* me_rv */
	309,	/* me_xstart */
	611,	/* me_ystart */
	582,	/* me_xend */
	645,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM107 = {
	ME_FREC,
	0,	/* me_pos */
	"always clear",
	'm',	/* me_rv */
	309,	/* me_xstart */
	645,	/* me_ystart */
	582,	/* me_xend */
	680,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM108 = {
	ME_VER+ME_FREE,
	645,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	309,	/* me_xstart */
	611,	/* me_ystart */
	582,	/* me_xend */
	680,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM106,	/* me_topleft */
	&cmdmen_MM107,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM109 = {
	ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"help window",
	'j',	/* me_rv */
	55,	/* me_xstart */
	610,	/* me_ystart */
	350,	/* me_xend */
	645,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM108,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM110 = {
	ME_VER,
	610,	/* me_pos */
	NULL,	/* me_cap */
	'i',	/* me_rv */
	55,	/* me_xstart */
	575,	/* me_ystart */
	350,	/* me_xend */
	645,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM105,	/* me_topleft */
	&cmdmen_MM109,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM111 = {
	ME_VER,
	575,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	55,	/* me_xstart */
	541,	/* me_ystart */
	350,	/* me_xend */
	645,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM95,	/* me_topleft */
	&cmdmen_MM110,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM112 = {
	ME_VER,
	541,	/* me_pos */
	NULL,	/* me_cap */
	'!',	/* me_rv */
	55,	/* me_xstart */
	471,	/* me_ystart */
	350,	/* me_xend */
	645,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM79,	/* me_topleft */
	&cmdmen_MM111,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM113 = {
	ME_VER,
	471,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	55,	/* me_xstart */
	333,	/* me_ystart */
	350,	/* me_xend */
	645,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM76,	/* me_topleft */
	&cmdmen_MM112,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM114 = {
	ME_VER+ME_FREE,
	333,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	55,	/* me_xstart */
	21,	/* me_ystart */
	350,	/* me_xend */
	645,	/* me_yend */
	-21,	/* me_xcurs */
	-11,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM64,	/* me_topleft */
	&cmdmen_MM113,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM115 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_CREL,
	0,	/* me_pos */
	"Help",
	'e',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	76,	/* me_xend */
	32,	/* me_yend */
	1064,	/* me_xcurs */
	-22,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM114,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM116 = {
	ME_FREC,
	0,	/* me_pos */
	"Backwards",
	'b',	/* me_rv */
	2,	/* me_xstart */
	0,	/* me_ystart */
	134,	/* me_xend */
	35,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM117 = {
	ME_FREC,
	0,	/* me_pos */
	"Forwards",
	'f',	/* me_rv */
	134,	/* me_xstart */
	0,	/* me_ystart */
	291,	/* me_xend */
	35,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM118 = {
	ME_HOR+ME_FREE,
	134,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	2,	/* me_xstart */
	0,	/* me_ystart */
	291,	/* me_xend */
	35,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM116,	/* me_topleft */
	&cmdmen_MM117,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM119 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"Search",
	0x100+'j',	/* me_rv */
	76,	/* me_xstart */
	-1,	/* me_ystart */
	194,	/* me_xend */
	32,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM118,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM120 = {
	ME_HOR,
	76,	/* me_pos */
	NULL,	/* me_cap */
	'e',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	194,	/* me_xend */
	32,	/* me_yend */
	1064,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM115,	/* me_topleft */
	&cmdmen_MM119,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM121 = {
	ME_FREC,
	0,	/* me_pos */
	"Snapshot Selected Object",
	'c',	/* me_rv */
	236,	/* me_xstart */
	10,	/* me_ystart */
	606,	/* me_xend */
	45,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM122 = {
	ME_FREC,
	0,	/* me_pos */
	"Snapshot All Objects",
	'w',	/* me_rv */
	236,	/* me_xstart */
	45,	/* me_ystart */
	606,	/* me_xend */
	80,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM123 = {
	ME_VER,
	45,	/* me_pos */
	NULL,	/* me_cap */
	'c',	/* me_rv */
	236,	/* me_xstart */
	10,	/* me_ystart */
	606,	/* me_xend */
	80,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM121,	/* me_topleft */
	&cmdmen_MM122,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM124 = {
	ME_FREC,
	0,	/* me_pos */
	"Raise Display",
	'R',	/* me_rv */
	236,	/* me_xstart */
	80,	/* me_ystart */
	606,	/* me_xend */
	115,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM125 = {
	ME_FREC,
	0,	/* me_pos */
	"Raise Source",
	'r',	/* me_rv */
	236,	/* me_xstart */
	115,	/* me_ystart */
	606,	/* me_xend */
	150,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM126 = {
	ME_FREC,
	0,	/* me_pos */
	"Raise Output",
	'z',	/* me_rv */
	236,	/* me_xstart */
	150,	/* me_ystart */
	606,	/* me_xend */
	185,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM127 = {
	ME_VER,
	150,	/* me_pos */
	NULL,	/* me_cap */
	'r',	/* me_rv */
	236,	/* me_xstart */
	115,	/* me_ystart */
	606,	/* me_xend */
	185,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM125,	/* me_topleft */
	&cmdmen_MM126,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM128 = {
	ME_VER,
	115,	/* me_pos */
	NULL,	/* me_cap */
	'R',	/* me_rv */
	236,	/* me_xstart */
	80,	/* me_ystart */
	606,	/* me_xend */
	185,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM124,	/* me_topleft */
	&cmdmen_MM127,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM129 = {
	ME_VER,
	80,	/* me_pos */
	NULL,	/* me_cap */
	'c',	/* me_rv */
	236,	/* me_xstart */
	10,	/* me_ystart */
	606,	/* me_xend */
	185,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM123,	/* me_topleft */
	&cmdmen_MM128,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM130 = {
	ME_FREC,
	0,	/* me_pos */
	"Message Logging",
	'x',	/* me_rv */
	236,	/* me_xstart */
	185,	/* me_ystart */
	606,	/* me_xend */
	221,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM131 = {
	ME_FREC,
	0,	/* me_pos */
	"No Raise On Break",
	0x100+'a',	/* me_rv */
	236,	/* me_xstart */
	221,	/* me_ystart */
	606,	/* me_xend */
	255,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM132 = {
	ME_VER,
	221,	/* me_pos */
	NULL,	/* me_cap */
	'x',	/* me_rv */
	236,	/* me_xstart */
	185,	/* me_ystart */
	606,	/* me_xend */
	255,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM130,	/* me_topleft */
	&cmdmen_MM131,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM133 = {
	ME_FREC,
	0,	/* me_pos */
	"Raise On Break",
	'j',	/* me_rv */
	236,	/* me_xstart */
	255,	/* me_ystart */
	606,	/* me_xend */
	290,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM134 = {
	ME_FREC,
	0,	/* me_pos */
	"Raise on Break\nLower On Run",
	'y',	/* me_rv */
	236,	/* me_xstart */
	290,	/* me_ystart */
	606,	/* me_xend */
	360,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM135 = {
	ME_FREC,
	0,	/* me_pos */
	"Raise on Break\nIconify On Run",
	0x100+'b',	/* me_rv */
	236,	/* me_xstart */
	360,	/* me_ystart */
	606,	/* me_xend */
	430,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM136 = {
	ME_VER,
	360,	/* me_pos */
	NULL,	/* me_cap */
	'y',	/* me_rv */
	236,	/* me_xstart */
	290,	/* me_ystart */
	606,	/* me_xend */
	430,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM134,	/* me_topleft */
	&cmdmen_MM135,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM137 = {
	ME_VER,
	290,	/* me_pos */
	NULL,	/* me_cap */
	'j',	/* me_rv */
	236,	/* me_xstart */
	255,	/* me_ystart */
	606,	/* me_xend */
	430,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM133,	/* me_topleft */
	&cmdmen_MM136,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM138 = {
	ME_VER,
	255,	/* me_pos */
	NULL,	/* me_cap */
	'x',	/* me_rv */
	236,	/* me_xstart */
	185,	/* me_ystart */
	606,	/* me_xend */
	430,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM132,	/* me_topleft */
	&cmdmen_MM137,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM139 = {
	ME_VER+ME_FREE,
	185,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	236,	/* me_xstart */
	10,	/* me_ystart */
	606,	/* me_xend */
	430,	/* me_yend */
	-100,	/* me_xcurs */
	-22,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM129,	/* me_topleft */
	&cmdmen_MM138,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM140 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_CREL,
	0,	/* me_pos */
	"Windows",
	'b',	/* me_rv */
	194,	/* me_xstart */
	-1,	/* me_ystart */
	336,	/* me_xend */
	32,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM139,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM141 = {
	ME_FREC,
	0,	/* me_pos */
	"cancel",
	'?',	/* me_rv */
	415,	/* me_xstart */
	19,	/* me_ystart */
	574,	/* me_xend */
	49,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM142 = {
	ME_FREC,
	0,	/* me_pos */
	"confirm",
	'q',	/* me_rv */
	415,	/* me_xstart */
	49,	/* me_ystart */
	574,	/* me_xend */
	80,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM143 = {
	ME_VER+ME_FREE,
	49,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	415,	/* me_xstart */
	19,	/* me_ystart */
	574,	/* me_xend */
	80,	/* me_yend */
	0,	/* me_xcurs */
	-13,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM141,	/* me_topleft */
	&cmdmen_MM142,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM144 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_CREL,
	0,	/* me_pos */
	"Quit",
	'a',	/* me_rv */
	336,	/* me_xstart */
	-1,	/* me_ystart */
	415,	/* me_xend */
	32,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM143,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU cmdmen_MM145 = {
	ME_HOR,
	336,	/* me_pos */
	NULL,	/* me_cap */
	'b',	/* me_rv */
	194,	/* me_xstart */
	-1,	/* me_ystart */
	415,	/* me_xend */
	32,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM140,	/* me_topleft */
	&cmdmen_MM144,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU cmd_men = {
	ME_HOR+ME_FREE,
	194,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	415,	/* me_xend */
	32,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&cmdmen_MM120,	/* me_topleft */
	&cmdmen_MM145,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for coremen.c --- */

static MENU coremen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Load library",
	'Z',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	131,	/* me_xend */
	47,	/* me_yend */
	1152,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU coremen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Save state",
	's',	/* me_rv */
	131,	/* me_xstart */
	-1,	/* me_ystart */
	239,	/* me_xend */
	47,	/* me_yend */
	1152,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU coremen_MM3 = {
	ME_HOR,
	131,	/* me_pos */
	NULL,	/* me_cap */
	'Z',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	239,	/* me_xend */
	47,	/* me_yend */
	1152,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&coremen_MM1,	/* me_topleft */
	&coremen_MM2,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU coremen_MM4 = {
	ME_FREC,
	0,	/* me_pos */
	"Load state",
	'l',	/* me_rv */
	239,	/* me_xstart */
	-1,	/* me_ystart */
	348,	/* me_xend */
	47,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU coremen_MM5 = {
	ME_FREC,
	0,	/* me_pos */
	"Drop state",
	'd',	/* me_rv */
	348,	/* me_xstart */
	-1,	/* me_ystart */
	457,	/* me_xend */
	47,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU coremen_MM6 = {
	ME_FREC,
	0,	/* me_pos */
	"Rescan init file",
	'i',	/* me_rv */
	457,	/* me_xstart */
	-1,	/* me_ystart */
	635,	/* me_xend */
	47,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU coremen_MM7 = {
	ME_HOR,
	457,	/* me_pos */
	NULL,	/* me_cap */
	'd',	/* me_rv */
	348,	/* me_xstart */
	-1,	/* me_ystart */
	635,	/* me_xend */
	47,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&coremen_MM5,	/* me_topleft */
	&coremen_MM6,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU coremen_MM8 = {
	ME_HOR,
	348,	/* me_pos */
	NULL,	/* me_cap */
	'l',	/* me_rv */
	239,	/* me_xstart */
	-1,	/* me_ystart */
	635,	/* me_xend */
	47,	/* me_yend */
	1152,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&coremen_MM4,	/* me_topleft */
	&coremen_MM7,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU core_men = {
	ME_HOR+ME_FREE,
	239,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	635,	/* me_xend */
	47,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&coremen_MM3,	/* me_topleft */
	&coremen_MM8,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for envheadmen.c --- */

static MENU envheadmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Expand",
	's',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	125,	/* me_xend */
	100,	/* me_yend */
	1456,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU envheadmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Collapse",
	'h',	/* me_rv */
	125,	/* me_xstart */
	-1,	/* me_ystart */
	252,	/* me_xend */
	100,	/* me_yend */
	1456,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU envheadmen_MM3 = {
	ME_HOR,
	125,	/* me_pos */
	NULL,	/* me_cap */
	's',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	252,	/* me_xend */
	100,	/* me_yend */
	1456,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&envheadmen_MM1,	/* me_topleft */
	&envheadmen_MM2,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU envheadmen_MM4 = {
	ME_FREC,
	0,	/* me_pos */
	"Add entry",
	'a',	/* me_rv */
	252,	/* me_xstart */
	-1,	/* me_ystart */
	379,	/* me_xend */
	100,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU envheadmen_MM5 = {
	ME_FREC,
	0,	/* me_pos */
	"Reset env",
	'r',	/* me_rv */
	379,	/* me_xstart */
	-1,	/* me_ystart */
	506,	/* me_xend */
	100,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU envheadmen_MM6 = {
	ME_HOR,
	379,	/* me_pos */
	NULL,	/* me_cap */
	'a',	/* me_rv */
	252,	/* me_xstart */
	-1,	/* me_ystart */
	506,	/* me_xend */
	100,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&envheadmen_MM4,	/* me_topleft */
	&envheadmen_MM5,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU envhead_men = {
	ME_HOR+ME_FREE,
	252,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	506,	/* me_xend */
	100,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&envheadmen_MM3,	/* me_topleft */
	&envheadmen_MM6,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for envmen.c --- */

static MENU envmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Hide",
	'h',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	132,	/* me_xend */
	97,	/* me_yend */
	1424,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU envmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Delete",
	'd',	/* me_rv */
	132,	/* me_xstart */
	-1,	/* me_ystart */
	266,	/* me_xend */
	97,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU envmen_MM3 = {
	ME_FREC,
	0,	/* me_pos */
	"Append entry",
	'a',	/* me_rv */
	266,	/* me_xstart */
	-1,	/* me_ystart */
	400,	/* me_xend */
	97,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU envmen_MM4 = {
	ME_HOR,
	266,	/* me_pos */
	NULL,	/* me_cap */
	'd',	/* me_rv */
	132,	/* me_xstart */
	-1,	/* me_ystart */
	400,	/* me_xend */
	97,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&envmen_MM2,	/* me_topleft */
	&envmen_MM3,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU env_men = {
	ME_HOR+ME_FREE,
	132,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	400,	/* me_xend */
	97,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&envmen_MM1,	/* me_topleft */
	&envmen_MM4,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for filmen.c --- */

static MENU filmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Like before",
	'B',	/* me_rv */
	109,	/* me_xstart */
	27,	/* me_ystart */
	254,	/* me_xend */
	71,	/* me_yend */
	2128,	/* me_xcurs */
	5,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Completely",
	'E',	/* me_rv */
	109,	/* me_xstart */
	71,	/* me_ystart */
	254,	/* me_xend */
	115,	/* me_yend */
	2128,	/* me_xcurs */
	5,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM3 = {
	ME_VER+ME_FREE,
	71,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	109,	/* me_xstart */
	27,	/* me_ystart */
	254,	/* me_xend */
	115,	/* me_yend */
	-7,	/* me_xcurs */
	-14,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&filmen_MM1,	/* me_topleft */
	&filmen_MM2,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM4 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_CREL,
	0,	/* me_pos */
	"Expand",
	'?',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	116,	/* me_xend */
	41,	/* me_yend */
	2128,	/* me_xcurs */
	5,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&filmen_MM3,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM5 = {
	ME_FREC,
	0,	/* me_pos */
	"Collapse",
	'c',	/* me_rv */
	116,	/* me_xstart */
	-1,	/* me_ystart */
	263,	/* me_xend */
	41,	/* me_yend */
	2128,	/* me_xcurs */
	5,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM6 = {
	ME_HOR,
	116,	/* me_pos */
	NULL,	/* me_cap */
	'?',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	263,	/* me_xend */
	41,	/* me_yend */
	2128,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&filmen_MM4,	/* me_topleft */
	&filmen_MM5,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM7 = {
	ME_FREC,
	0,	/* me_pos */
	"Add expr",
	'x',	/* me_rv */
	263,	/* me_xstart */
	-1,	/* me_ystart */
	394,	/* me_xend */
	41,	/* me_yend */
	2128,	/* me_xcurs */
	5,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM8 = {
	ME_FREC,
	0,	/* me_pos */
	"Source",
	'e',	/* me_rv */
	394,	/* me_xstart */
	-1,	/* me_ystart */
	521,	/* me_xend */
	41,	/* me_yend */
	2128,	/* me_xcurs */
	5,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM9 = {
	ME_FREC,
	0,	/* me_pos */
	"Used",
	's',	/* me_rv */
	551,	/* me_xstart */
	23,	/* me_ystart */
	660,	/* me_xend */
	68,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM10 = {
	ME_FREC,
	0,	/* me_pos */
	"Assumed",
	'p',	/* me_rv */
	551,	/* me_xstart */
	68,	/* me_ystart */
	660,	/* me_xend */
	114,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM11 = {
	ME_VER,
	68,	/* me_pos */
	NULL,	/* me_cap */
	's',	/* me_rv */
	551,	/* me_xstart */
	23,	/* me_ystart */
	660,	/* me_xend */
	114,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&filmen_MM9,	/* me_topleft */
	&filmen_MM10,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM12 = {
	ME_FREC,
	0,	/* me_pos */
	"Rematch",
	'r',	/* me_rv */
	551,	/* me_xstart */
	114,	/* me_ystart */
	660,	/* me_xend */
	160,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM13 = {
	ME_FREC,
	0,	/* me_pos */
	"Reload",
	'l',	/* me_rv */
	551,	/* me_xstart */
	160,	/* me_ystart */
	660,	/* me_xend */
	206,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM14 = {
	ME_FREC,
	0,	/* me_pos */
	"Dates",
	'm',	/* me_rv */
	551,	/* me_xstart */
	206,	/* me_ystart */
	660,	/* me_xend */
	254,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM15 = {
	ME_VER,
	206,	/* me_pos */
	NULL,	/* me_cap */
	'l',	/* me_rv */
	551,	/* me_xstart */
	160,	/* me_ystart */
	660,	/* me_xend */
	254,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&filmen_MM13,	/* me_topleft */
	&filmen_MM14,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM16 = {
	ME_VER,
	160,	/* me_pos */
	NULL,	/* me_cap */
	'r',	/* me_rv */
	551,	/* me_xstart */
	114,	/* me_ystart */
	660,	/* me_xend */
	254,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&filmen_MM12,	/* me_topleft */
	&filmen_MM15,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM17 = {
	ME_VER+ME_FREE,
	114,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	551,	/* me_xstart */
	23,	/* me_ystart */
	660,	/* me_xend */
	254,	/* me_yend */
	-59,	/* me_xcurs */
	-18,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&filmen_MM11,	/* me_topleft */
	&filmen_MM16,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM18 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_CREL,
	0,	/* me_pos */
	"Path",
	'a',	/* me_rv */
	521,	/* me_xstart */
	-1,	/* me_ystart */
	610,	/* me_xend */
	41,	/* me_yend */
	2128,	/* me_xcurs */
	5,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&filmen_MM17,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM19 = {
	ME_HOR,
	521,	/* me_pos */
	NULL,	/* me_cap */
	'e',	/* me_rv */
	394,	/* me_xstart */
	-1,	/* me_ystart */
	610,	/* me_xend */
	41,	/* me_yend */
	2128,	/* me_xcurs */
	5,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&filmen_MM8,	/* me_topleft */
	&filmen_MM18,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU filmen_MM20 = {
	ME_HOR,
	394,	/* me_pos */
	NULL,	/* me_cap */
	'x',	/* me_rv */
	263,	/* me_xstart */
	-1,	/* me_ystart */
	610,	/* me_xend */
	41,	/* me_yend */
	2128,	/* me_xcurs */
	5,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&filmen_MM7,	/* me_topleft */
	&filmen_MM19,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU fil_men = {
	ME_HOR+ME_FREE,
	263,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	610,	/* me_xend */
	41,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&filmen_MM6,	/* me_topleft */
	&filmen_MM20,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for gblmen.c --- */

MENU gbl_men = {
	ME_FREC+ME_FREE,
	0,	/* me_pos */
	"Collapse",
	'c',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	47,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for mfuncmen.c --- */

static MENU mfuncmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Hide",
	'h',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	161,	/* me_xend */
	83,	/* me_yend */
	1024,	/* me_xcurs */
	-1,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU mfuncmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Source",
	'e',	/* me_rv */
	161,	/* me_xstart */
	-1,	/* me_ystart */
	324,	/* me_xend */
	83,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU mfunc_men = {
	ME_HOR+ME_FREE,
	161,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	324,	/* me_xend */
	83,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&mfuncmen_MM1,	/* me_topleft */
	&mfuncmen_MM2,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for nolibmen.c --- */

MENU nolib_men = {
	ME_FREC+ME_FREE,
	0,	/* me_pos */
	"Load library",
	'l',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	469,	/* me_xend */
	123,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for outmen.c --- */

static MENU outmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Clear",
	'c',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	125,	/* me_xend */
	107,	/* me_yend */
	1592,	/* me_xcurs */
	1024,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU outmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Backwards",
	'b',	/* me_rv */
	61,	/* me_xstart */
	1,	/* me_ystart */
	184,	/* me_xend */
	104,	/* me_yend */
	1592,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU outmen_MM3 = {
	ME_FREC,
	0,	/* me_pos */
	"Forwards",
	'f',	/* me_rv */
	184,	/* me_xstart */
	1,	/* me_ystart */
	307,	/* me_xend */
	104,	/* me_yend */
	1592,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU outmen_MM4 = {
	ME_HOR+ME_FREE,
	184,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	61,	/* me_xstart */
	1,	/* me_ystart */
	307,	/* me_xend */
	104,	/* me_yend */
	1592,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&outmen_MM2,	/* me_topleft */
	&outmen_MM3,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU outmen_MM5 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"Search",
	'x',	/* me_rv */
	125,	/* me_xstart */
	-1,	/* me_ystart */
	252,	/* me_xend */
	107,	/* me_yend */
	1592,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&outmen_MM4,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU outmen_MM6 = {
	ME_HOR,
	125,	/* me_pos */
	NULL,	/* me_cap */
	'c',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	252,	/* me_xend */
	107,	/* me_yend */
	1592,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&outmen_MM1,	/* me_topleft */
	&outmen_MM5,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU outmen_MM7 = {
	ME_FREC,
	0,	/* me_pos */
	"Load",
	'l',	/* me_rv */
	252,	/* me_xstart */
	-1,	/* me_ystart */
	379,	/* me_xend */
	107,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU outmen_MM8 = {
	ME_FREC,
	0,	/* me_pos */
	"Save",
	's',	/* me_rv */
	379,	/* me_xstart */
	-1,	/* me_ystart */
	506,	/* me_xend */
	107,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU outmen_MM9 = {
	ME_FREC,
	0,	/* me_pos */
	"Close",
	'C',	/* me_rv */
	506,	/* me_xstart */
	-1,	/* me_ystart */
	631,	/* me_xend */
	107,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU outmen_MM10 = {
	ME_HOR,
	506,	/* me_pos */
	NULL,	/* me_cap */
	's',	/* me_rv */
	379,	/* me_xstart */
	-1,	/* me_ystart */
	631,	/* me_xend */
	107,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&outmen_MM8,	/* me_topleft */
	&outmen_MM9,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU outmen_MM11 = {
	ME_HOR,
	379,	/* me_pos */
	NULL,	/* me_cap */
	'l',	/* me_rv */
	252,	/* me_xstart */
	-1,	/* me_ystart */
	631,	/* me_xend */
	107,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&outmen_MM7,	/* me_topleft */
	&outmen_MM10,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU out_men = {
	ME_HOR+ME_FREE,
	252,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	631,	/* me_xend */
	107,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&outmen_MM6,	/* me_topleft */
	&outmen_MM11,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for sghmen.c --- */

static MENU sghmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Expand",
	'a',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	300,	/* me_xend */
	47,	/* me_yend */
	1352,	/* me_xcurs */
	-1,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU sghmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Collapse",
	'c',	/* me_rv */
	300,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	47,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU sgh_men = {
	ME_HOR+ME_FREE,
	300,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	47,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&sghmen_MM1,	/* me_topleft */
	&sghmen_MM2,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for shmen.c --- */

static MENU shmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Expand",
	'e',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	157,	/* me_xend */
	47,	/* me_yend */
	1304,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU shmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"First level",
	'c',	/* me_rv */
	296,	/* me_xstart */
	34,	/* me_ystart */
	459,	/* me_xend */
	57,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU shmen_MM3 = {
	ME_FREC,
	0,	/* me_pos */
	"Completely",
	'C',	/* me_rv */
	296,	/* me_xstart */
	57,	/* me_ystart */
	459,	/* me_xend */
	81,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU shmen_MM4 = {
	ME_VER+ME_FREE,
	57,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	296,	/* me_xstart */
	34,	/* me_ystart */
	459,	/* me_xend */
	81,	/* me_yend */
	-54,	/* me_xcurs */
	-13,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&shmen_MM2,	/* me_topleft */
	&shmen_MM3,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU shmen_MM5 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_NOSCALE+ME_CREL,
	0,	/* me_pos */
	"Collapse",
	'?',	/* me_rv */
	157,	/* me_xstart */
	-1,	/* me_ystart */
	350,	/* me_xend */
	47,	/* me_yend */
	1304,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&shmen_MM4,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU shmen_MM6 = {
	ME_FREC,
	0,	/* me_pos */
	"Add source path",
	'a',	/* me_rv */
	350,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	47,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU shmen_MM7 = {
	ME_HOR,
	350,	/* me_pos */
	NULL,	/* me_cap */
	'?',	/* me_rv */
	157,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	47,	/* me_yend */
	1304,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&shmen_MM5,	/* me_topleft */
	&shmen_MM6,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU sh_men = {
	ME_HOR+ME_FREE,
	157,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	47,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&shmen_MM1,	/* me_topleft */
	&shmen_MM7,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for sigmen.c --- */

static MENU sigmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Stop/Continue",
	's',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	120,	/* me_xend */
	123,	/* me_yend */
	1568,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU sigmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Accept/Ignore",
	'a',	/* me_rv */
	120,	/* me_xstart */
	-1,	/* me_ystart */
	241,	/* me_xend */
	123,	/* me_yend */
	1568,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU sigmen_MM3 = {
	ME_HOR,
	120,	/* me_pos */
	NULL,	/* me_cap */
	's',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	241,	/* me_xend */
	123,	/* me_yend */
	1568,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&sigmen_MM1,	/* me_topleft */
	&sigmen_MM2,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU sigmen_MM4 = {
	ME_FREC,
	0,	/* me_pos */
	"Redraw/Don't",
	'r',	/* me_rv */
	241,	/* me_xstart */
	-1,	/* me_ystart */
	363,	/* me_xend */
	123,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU sigmen_MM5 = {
	ME_FREC,
	0,	/* me_pos */
	"Hide entry",
	'h',	/* me_rv */
	363,	/* me_xstart */
	-1,	/* me_ystart */
	469,	/* me_xend */
	123,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU sigmen_MM6 = {
	ME_HOR,
	363,	/* me_pos */
	NULL,	/* me_cap */
	'r',	/* me_rv */
	241,	/* me_xstart */
	-1,	/* me_ystart */
	469,	/* me_xend */
	123,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&sigmen_MM4,	/* me_topleft */
	&sigmen_MM5,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU sig_men = {
	ME_HOR+ME_FREE,
	241,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	469,	/* me_xend */
	123,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&sigmen_MM3,	/* me_topleft */
	&sigmen_MM6,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for srcmen.c --- */

static MENU srcmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Back",
	'r',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	102,	/* me_xend */
	91,	/* me_yend */
	1448,	/* me_xcurs */
	5,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU srcmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Backwards",
	'b',	/* me_rv */
	45,	/* me_xstart */
	0,	/* me_ystart */
	153,	/* me_xend */
	99,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU srcmen_MM3 = {
	ME_FREC,
	0,	/* me_pos */
	"Forwards",
	'f',	/* me_rv */
	153,	/* me_xstart */
	0,	/* me_ystart */
	261,	/* me_xend */
	99,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU srcmen_MM4 = {
	ME_HOR+ME_FREE,
	153,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	45,	/* me_xstart */
	0,	/* me_ystart */
	261,	/* me_xend */
	99,	/* me_yend */
	1448,	/* me_xcurs */
	5,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&srcmen_MM2,	/* me_topleft */
	&srcmen_MM3,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU srcmen_MM5 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"Search",
	's',	/* me_rv */
	102,	/* me_xstart */
	-1,	/* me_ystart */
	206,	/* me_xend */
	91,	/* me_yend */
	1448,	/* me_xcurs */
	5,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&srcmen_MM4,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU srcmen_MM6 = {
	ME_HOR,
	102,	/* me_pos */
	NULL,	/* me_cap */
	'r',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	206,	/* me_xend */
	91,	/* me_yend */
	1448,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&srcmen_MM1,	/* me_topleft */
	&srcmen_MM5,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU srcmen_MM7 = {
	ME_FREC,
	0,	/* me_pos */
	"Up",
	'u',	/* me_rv */
	206,	/* me_xstart */
	-1,	/* me_ystart */
	309,	/* me_xend */
	91,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU srcmen_MM8 = {
	ME_FREC,
	0,	/* me_pos */
	"Down",
	'd',	/* me_rv */
	309,	/* me_xstart */
	-1,	/* me_ystart */
	413,	/* me_xend */
	91,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU srcmen_MM9 = {
	ME_HOR,
	309,	/* me_pos */
	NULL,	/* me_cap */
	'u',	/* me_rv */
	206,	/* me_xstart */
	-1,	/* me_ystart */
	413,	/* me_xend */
	91,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&srcmen_MM7,	/* me_topleft */
	&srcmen_MM8,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU src_men = {
	ME_HOR+ME_FREE,
	206,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	413,	/* me_xend */
	91,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&srcmen_MM6,	/* me_topleft */
	&srcmen_MM9,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for tgt_attach_men.c --- */

MENU tgt_attach_men = {
	ME_FREC+ME_FREE,
	0,	/* me_pos */
	"Attach",
	'v',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	100,	/* me_xend */
	86,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for tgt_cont_men.c --- */

MENU tgt_cont_men = {
	ME_FREC+ME_FREE,
	0,	/* me_pos */
	"Cont",
	'c',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	100,	/* me_xend */
	86,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for tgt_detach_men.c --- */

MENU tgt_detach_men = {
	ME_FREC+ME_FREE,
	0,	/* me_pos */
	"Detach",
	'w',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	100,	/* me_xend */
	86,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for tgt_kill_men.c --- */

static MENU tgt_kill_men_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Cancel",
	'?',	/* me_rv */
	100,	/* me_xstart */
	25,	/* me_ystart */
	224,	/* me_xend */
	63,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU tgt_kill_men_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Confirm",
	'k',	/* me_rv */
	100,	/* me_xstart */
	63,	/* me_ystart */
	224,	/* me_xend */
	101,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU tgt_kill_men_MM3 = {
	ME_VER+ME_FREE,
	63,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	100,	/* me_xstart */
	25,	/* me_ystart */
	224,	/* me_xend */
	101,	/* me_yend */
	-28,	/* me_xcurs */
	-12,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&tgt_kill_men_MM1,	/* me_topleft */
	&tgt_kill_men_MM2,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU tgt_kill_men = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_FREE+ME_CREL,
	0,	/* me_pos */
	"Kill",
	'k',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	128,	/* me_xend */
	37,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&tgt_kill_men_MM3,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for tgt_next_men.c --- */

MENU tgt_next_men = {
	ME_FREC+ME_FREE,
	0,	/* me_pos */
	"Next",
	'n',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	100,	/* me_xend */
	86,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for tgt_restart_men.c --- */

static MENU tgt_restart_men_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Cancel",
	'@',	/* me_rv */
	100,	/* me_xstart */
	25,	/* me_ystart */
	224,	/* me_xend */
	63,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU tgt_restart_men_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Confirm",
	'R',	/* me_rv */
	100,	/* me_xstart */
	63,	/* me_ystart */
	224,	/* me_xend */
	101,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU tgt_restart_men_MM3 = {
	ME_VER+ME_FREE,
	63,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	100,	/* me_xstart */
	25,	/* me_ystart */
	224,	/* me_xend */
	101,	/* me_yend */
	-28,	/* me_xcurs */
	-12,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&tgt_restart_men_MM1,	/* me_topleft */
	&tgt_restart_men_MM2,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU tgt_restart_men = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_FREE+ME_CREL,
	0,	/* me_pos */
	"Restart",
	'R',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	128,	/* me_xend */
	37,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&tgt_restart_men_MM3,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for tgt_start_men.c --- */

MENU tgt_start_men = {
	ME_FREC+ME_FREE,
	0,	/* me_pos */
	"Start",
	'r',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	100,	/* me_xend */
	86,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for tgt_step_men.c --- */

MENU tgt_step_men = {
	ME_POPUP+ME_FREC+ME_FREE,
	0,	/* me_pos */
	"Step",
	's',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	100,	/* me_xend */
	86,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for tgt_stop_men.c --- */

MENU tgt_stop_men = {
	ME_FREC+ME_FREE,
	0,	/* me_pos */
	"Stop",
	'S',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	100,	/* me_xend */
	86,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for varmen.c --- */

static MENU varmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"&",
	'd',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	35,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"*",
	'u',	/* me_rv */
	35,	/* me_xstart */
	-1,	/* me_ystart */
	72,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM3 = {
	ME_HOR,
	35,	/* me_pos */
	NULL,	/* me_cap */
	'd',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	72,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM1,	/* me_topleft */
	&varmen_MM2,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM4 = {
	ME_FREC,
	0,	/* me_pos */
	"::",
	'y',	/* me_rv */
	72,	/* me_xstart */
	-1,	/* me_ystart */
	109,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM5 = {
	ME_FREC,
	0,	/* me_pos */
	"Struct/Union",
	'E',	/* me_rv */
	180,	/* me_xstart */
	15,	/* me_ystart */
	325,	/* me_xend */
	45,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM6 = {
	ME_FREC,
	0,	/* me_pos */
	"Class Hierarchy",
	'y',	/* me_rv */
	180,	/* me_xstart */
	45,	/* me_ystart */
	325,	/* me_xend */
	75,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM7 = {
	ME_VER,
	45,	/* me_pos */
	NULL,	/* me_cap */
	'E',	/* me_rv */
	180,	/* me_xstart */
	15,	/* me_ystart */
	325,	/* me_xend */
	75,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM5,	/* me_topleft */
	&varmen_MM6,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM8 = {
	ME_FREC,
	0,	/* me_pos */
	"Static Members",
	'S',	/* me_rv */
	180,	/* me_xstart */
	75,	/* me_ystart */
	325,	/* me_xend */
	105,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM9 = {
	ME_FREC,
	0,	/* me_pos */
	"Dump Memory",
	'W',	/* me_rv */
	180,	/* me_xstart */
	105,	/* me_ystart */
	325,	/* me_xend */
	135,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM10 = {
	ME_FREC,
	0,	/* me_pos */
	"Show Size",
	'X',	/* me_rv */
	180,	/* me_xstart */
	135,	/* me_ystart */
	325,	/* me_xend */
	165,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM11 = {
	ME_VER,
	135,	/* me_pos */
	NULL,	/* me_cap */
	'W',	/* me_rv */
	180,	/* me_xstart */
	105,	/* me_ystart */
	325,	/* me_xend */
	165,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM9,	/* me_topleft */
	&varmen_MM10,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM12 = {
	ME_VER,
	105,	/* me_pos */
	NULL,	/* me_cap */
	'S',	/* me_rv */
	180,	/* me_xstart */
	75,	/* me_ystart */
	325,	/* me_xend */
	165,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM8,	/* me_topleft */
	&varmen_MM11,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM13 = {
	ME_VER+ME_FREE,
	75,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	180,	/* me_xstart */
	15,	/* me_ystart */
	325,	/* me_xend */
	165,	/* me_yend */
	-23,	/* me_xcurs */
	-11,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM7,	/* me_topleft */
	&varmen_MM12,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM14 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_CREL,
	0,	/* me_pos */
	"Expand",
	'E',	/* me_rv */
	109,	/* me_xstart */
	-1,	/* me_ystart */
	203,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM13,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM15 = {
	ME_HOR,
	109,	/* me_pos */
	NULL,	/* me_cap */
	'y',	/* me_rv */
	72,	/* me_xstart */
	-1,	/* me_ystart */
	203,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM4,	/* me_topleft */
	&varmen_MM14,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM16 = {
	ME_HOR,
	72,	/* me_pos */
	NULL,	/* me_cap */
	'd',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	203,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM3,	/* me_topleft */
	&varmen_MM15,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM17 = {
	ME_FREC,
	0,	/* me_pos */
	"First level",
	'c',	/* me_rv */
	279,	/* me_xstart */
	14,	/* me_ystart */
	417,	/* me_xend */
	42,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM18 = {
	ME_FREC,
	0,	/* me_pos */
	"Completely",
	'C',	/* me_rv */
	279,	/* me_xstart */
	42,	/* me_ystart */
	417,	/* me_xend */
	71,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM19 = {
	ME_VER+ME_FREE,
	42,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	279,	/* me_xstart */
	14,	/* me_ystart */
	417,	/* me_xend */
	71,	/* me_yend */
	-35,	/* me_xcurs */
	-12,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM17,	/* me_topleft */
	&varmen_MM18,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM20 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_CREL,
	0,	/* me_pos */
	"Collapse",
	'?',	/* me_rv */
	203,	/* me_xstart */
	-1,	/* me_ystart */
	314,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM19,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM21 = {
	ME_FREC,
	0,	/* me_pos */
	"Dup",
	'D',	/* me_rv */
	314,	/* me_xstart */
	-1,	/* me_ystart */
	371,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM22 = {
	ME_HOR,
	314,	/* me_pos */
	NULL,	/* me_cap */
	'?',	/* me_rv */
	203,	/* me_xstart */
	-1,	/* me_ystart */
	371,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM20,	/* me_topleft */
	&varmen_MM21,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM23 = {
	ME_FREC,
	0,	/* me_pos */
	"Del",
	'k',	/* me_rv */
	371,	/* me_xstart */
	-1,	/* me_ystart */
	436,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM24 = {
	ME_FREC,
	0,	/* me_pos */
	"Decimal",
	'T',	/* me_rv */
	342,	/* me_xstart */
	63,	/* me_ystart */
	435,	/* me_xend */
	88,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM25 = {
	ME_FREC,
	0,	/* me_pos */
	"Octal",
	'O',	/* me_rv */
	342,	/* me_xstart */
	88,	/* me_ystart */
	435,	/* me_xend */
	114,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM26 = {
	ME_VER,
	88,	/* me_pos */
	NULL,	/* me_cap */
	'T',	/* me_rv */
	342,	/* me_xstart */
	63,	/* me_ystart */
	435,	/* me_xend */
	114,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM24,	/* me_topleft */
	&varmen_MM25,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM27 = {
	ME_FREC,
	0,	/* me_pos */
	"Hex",
	'H',	/* me_rv */
	342,	/* me_xstart */
	114,	/* me_ystart */
	435,	/* me_xend */
	139,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM28 = {
	ME_FREC,
	0,	/* me_pos */
	"Binary",
	'B',	/* me_rv */
	342,	/* me_xstart */
	139,	/* me_ystart */
	435,	/* me_xend */
	165,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM29 = {
	ME_VER,
	139,	/* me_pos */
	NULL,	/* me_cap */
	'H',	/* me_rv */
	342,	/* me_xstart */
	114,	/* me_ystart */
	435,	/* me_xend */
	165,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM27,	/* me_topleft */
	&varmen_MM28,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM30 = {
	ME_VER+ME_FREE,
	114,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	342,	/* me_xstart */
	63,	/* me_ystart */
	435,	/* me_xend */
	165,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM26,	/* me_topleft */
	&varmen_MM29,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM31 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"Unsigned",
	'?',	/* me_rv */
	344,	/* me_xstart */
	31,	/* me_ystart */
	434,	/* me_xend */
	59,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM30,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM32 = {
	ME_FREC,
	0,	/* me_pos */
	"Decimal",
	't',	/* me_rv */
	432,	/* me_xstart */
	63,	/* me_ystart */
	525,	/* me_xend */
	89,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM33 = {
	ME_FREC,
	0,	/* me_pos */
	"Octal",
	'o',	/* me_rv */
	432,	/* me_xstart */
	89,	/* me_ystart */
	525,	/* me_xend */
	115,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM34 = {
	ME_FREC,
	0,	/* me_pos */
	"Hex",
	'h',	/* me_rv */
	432,	/* me_xstart */
	115,	/* me_ystart */
	525,	/* me_xend */
	141,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM35 = {
	ME_VER,
	115,	/* me_pos */
	NULL,	/* me_cap */
	'o',	/* me_rv */
	432,	/* me_xstart */
	89,	/* me_ystart */
	525,	/* me_xend */
	141,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM33,	/* me_topleft */
	&varmen_MM34,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM36 = {
	ME_VER+ME_FREE,
	89,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	432,	/* me_xstart */
	63,	/* me_ystart */
	525,	/* me_xend */
	141,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM32,	/* me_topleft */
	&varmen_MM35,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM37 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"Signed",
	'?',	/* me_rv */
	434,	/* me_xstart */
	31,	/* me_ystart */
	525,	/* me_xend */
	59,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM36,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM38 = {
	ME_FREC,
	0,	/* me_pos */
	"Default",
	'n',	/* me_rv */
	525,	/* me_xstart */
	63,	/* me_ystart */
	636,	/* me_xend */
	88,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM39 = {
	ME_FREC,
	0,	/* me_pos */
	"Ascii",
	'a',	/* me_rv */
	525,	/* me_xstart */
	88,	/* me_ystart */
	636,	/* me_xend */
	114,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM40 = {
	ME_FREC,
	0,	/* me_pos */
	"String",
	's',	/* me_rv */
	525,	/* me_xstart */
	114,	/* me_ystart */
	636,	/* me_xend */
	140,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM41 = {
	ME_VER,
	114,	/* me_pos */
	NULL,	/* me_cap */
	'a',	/* me_rv */
	525,	/* me_xstart */
	88,	/* me_ystart */
	636,	/* me_xend */
	140,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM39,	/* me_topleft */
	&varmen_MM40,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM42 = {
	ME_VER+ME_FREE,
	88,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	525,	/* me_xstart */
	63,	/* me_ystart */
	636,	/* me_xend */
	140,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM38,	/* me_topleft */
	&varmen_MM41,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM43 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"Other",
	'b',	/* me_rv */
	525,	/* me_xstart */
	31,	/* me_ystart */
	616,	/* me_xend */
	59,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM42,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM44 = {
	ME_HOR,
	525,	/* me_pos */
	NULL,	/* me_cap */
	'?',	/* me_rv */
	434,	/* me_xstart */
	31,	/* me_ystart */
	616,	/* me_xend */
	59,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM37,	/* me_topleft */
	&varmen_MM43,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM45 = {
	ME_HOR+ME_FREE,
	434,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	344,	/* me_xstart */
	31,	/* me_ystart */
	616,	/* me_xend */
	59,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM31,	/* me_topleft */
	&varmen_MM44,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM46 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"Format",
	'?',	/* me_rv */
	436,	/* me_xstart */
	-1,	/* me_ystart */
	527,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM45,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM47 = {
	ME_FREC,
	0,	/* me_pos */
	"Use typedefs",
	'U',	/* me_rv */
	600,	/* me_xstart */
	24,	/* me_ystart */
	742,	/* me_xend */
	49,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM48 = {
	ME_FREC,
	0,	/* me_pos */
	"Ignore typedefs",
	'I',	/* me_rv */
	600,	/* me_xstart */
	49,	/* me_ystart */
	742,	/* me_xend */
	75,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM49 = {
	ME_VER+ME_FREE,
	49,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	600,	/* me_xstart */
	24,	/* me_ystart */
	742,	/* me_xend */
	75,	/* me_yend */
	0,	/* me_xcurs */
	-2,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM47,	/* me_topleft */
	&varmen_MM48,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM50 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_NOSCALE+ME_CREL,
	0,	/* me_pos */
	"Decl",
	'q',	/* me_rv */
	527,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM49,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM51 = {
	ME_HOR,
	527,	/* me_pos */
	NULL,	/* me_cap */
	'?',	/* me_rv */
	436,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM46,	/* me_topleft */
	&varmen_MM50,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM52 = {
	ME_HOR,
	436,	/* me_pos */
	NULL,	/* me_cap */
	'k',	/* me_rv */
	371,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM23,	/* me_topleft */
	&varmen_MM51,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen_MM53 = {
	ME_HOR,
	371,	/* me_pos */
	NULL,	/* me_cap */
	'?',	/* me_rv */
	203,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM22,	/* me_topleft */
	&varmen_MM52,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU var_men = {
	ME_HOR+ME_FREE,
	203,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen_MM16,	/* me_topleft */
	&varmen_MM53,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for varmen2.c --- */

static MENU varmen2_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"&",
	'd',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	35,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"*",
	'u',	/* me_rv */
	35,	/* me_xstart */
	-1,	/* me_ystart */
	72,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM3 = {
	ME_HOR,
	35,	/* me_pos */
	NULL,	/* me_cap */
	'd',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	72,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM1,	/* me_topleft */
	&varmen2_MM2,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM4 = {
	ME_FREC,
	0,	/* me_pos */
	"::",
	'y',	/* me_rv */
	72,	/* me_xstart */
	-1,	/* me_ystart */
	109,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM5 = {
	ME_FREC,
	0,	/* me_pos */
	"Struct/Union",
	'E',	/* me_rv */
	180,	/* me_xstart */
	15,	/* me_ystart */
	325,	/* me_xend */
	45,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM6 = {
	ME_FREC,
	0,	/* me_pos */
	"Class Hierarchy",
	'y',	/* me_rv */
	180,	/* me_xstart */
	45,	/* me_ystart */
	325,	/* me_xend */
	75,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM7 = {
	ME_VER,
	45,	/* me_pos */
	NULL,	/* me_cap */
	'E',	/* me_rv */
	180,	/* me_xstart */
	15,	/* me_ystart */
	325,	/* me_xend */
	75,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM5,	/* me_topleft */
	&varmen2_MM6,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM8 = {
	ME_FREC,
	0,	/* me_pos */
	"Static Members",
	'S',	/* me_rv */
	180,	/* me_xstart */
	75,	/* me_ystart */
	325,	/* me_xend */
	105,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM9 = {
	ME_FREC,
	0,	/* me_pos */
	"Dump Memory",
	'W',	/* me_rv */
	180,	/* me_xstart */
	105,	/* me_ystart */
	325,	/* me_xend */
	135,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM10 = {
	ME_FREC,
	0,	/* me_pos */
	"Show Size",
	'X',	/* me_rv */
	180,	/* me_xstart */
	135,	/* me_ystart */
	325,	/* me_xend */
	165,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM11 = {
	ME_VER,
	135,	/* me_pos */
	NULL,	/* me_cap */
	'W',	/* me_rv */
	180,	/* me_xstart */
	105,	/* me_ystart */
	325,	/* me_xend */
	165,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM9,	/* me_topleft */
	&varmen2_MM10,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM12 = {
	ME_VER,
	105,	/* me_pos */
	NULL,	/* me_cap */
	'S',	/* me_rv */
	180,	/* me_xstart */
	75,	/* me_ystart */
	325,	/* me_xend */
	165,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM8,	/* me_topleft */
	&varmen2_MM11,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM13 = {
	ME_VER+ME_FREE,
	75,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	180,	/* me_xstart */
	15,	/* me_ystart */
	325,	/* me_xend */
	165,	/* me_yend */
	-23,	/* me_xcurs */
	-11,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM7,	/* me_topleft */
	&varmen2_MM12,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM14 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_CREL,
	0,	/* me_pos */
	"Expand",
	'E',	/* me_rv */
	109,	/* me_xstart */
	-1,	/* me_ystart */
	203,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM13,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM15 = {
	ME_FREC,
	0,	/* me_pos */
	"First level",
	'c',	/* me_rv */
	279,	/* me_xstart */
	14,	/* me_ystart */
	417,	/* me_xend */
	42,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM16 = {
	ME_FREC,
	0,	/* me_pos */
	"Completely",
	'C',	/* me_rv */
	279,	/* me_xstart */
	42,	/* me_ystart */
	417,	/* me_xend */
	71,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM17 = {
	ME_VER+ME_FREE,
	42,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	279,	/* me_xstart */
	14,	/* me_ystart */
	417,	/* me_xend */
	71,	/* me_yend */
	-35,	/* me_xcurs */
	-12,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM15,	/* me_topleft */
	&varmen2_MM16,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM18 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_CREL,
	0,	/* me_pos */
	"Collapse",
	'?',	/* me_rv */
	203,	/* me_xstart */
	-1,	/* me_ystart */
	314,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM17,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM19 = {
	ME_HOR,
	203,	/* me_pos */
	NULL,	/* me_cap */
	'E',	/* me_rv */
	109,	/* me_xstart */
	-1,	/* me_ystart */
	314,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM14,	/* me_topleft */
	&varmen2_MM18,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM20 = {
	ME_HOR,
	109,	/* me_pos */
	NULL,	/* me_cap */
	'y',	/* me_rv */
	72,	/* me_xstart */
	-1,	/* me_ystart */
	314,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM4,	/* me_topleft */
	&varmen2_MM19,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM21 = {
	ME_HOR,
	72,	/* me_pos */
	NULL,	/* me_cap */
	'd',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	314,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM3,	/* me_topleft */
	&varmen2_MM20,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM22 = {
	ME_FREC,
	0,	/* me_pos */
	"Dup",
	'D',	/* me_rv */
	314,	/* me_xstart */
	-1,	/* me_ystart */
	371,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM23 = {
	ME_FREC,
	0,	/* me_pos */
	"Del",
	'k',	/* me_rv */
	371,	/* me_xstart */
	-1,	/* me_ystart */
	436,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM24 = {
	ME_HOR,
	371,	/* me_pos */
	NULL,	/* me_cap */
	'D',	/* me_rv */
	314,	/* me_xstart */
	-1,	/* me_ystart */
	436,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM22,	/* me_topleft */
	&varmen2_MM23,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM25 = {
	ME_FREC,
	0,	/* me_pos */
	"Decimal",
	'T',	/* me_rv */
	342,	/* me_xstart */
	63,	/* me_ystart */
	435,	/* me_xend */
	88,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM26 = {
	ME_FREC,
	0,	/* me_pos */
	"Octal",
	'O',	/* me_rv */
	342,	/* me_xstart */
	88,	/* me_ystart */
	435,	/* me_xend */
	114,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM27 = {
	ME_VER,
	88,	/* me_pos */
	NULL,	/* me_cap */
	'T',	/* me_rv */
	342,	/* me_xstart */
	63,	/* me_ystart */
	435,	/* me_xend */
	114,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM25,	/* me_topleft */
	&varmen2_MM26,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM28 = {
	ME_FREC,
	0,	/* me_pos */
	"Hex",
	'H',	/* me_rv */
	342,	/* me_xstart */
	114,	/* me_ystart */
	435,	/* me_xend */
	139,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM29 = {
	ME_FREC,
	0,	/* me_pos */
	"Binary",
	'B',	/* me_rv */
	342,	/* me_xstart */
	139,	/* me_ystart */
	435,	/* me_xend */
	165,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM30 = {
	ME_VER,
	139,	/* me_pos */
	NULL,	/* me_cap */
	'H',	/* me_rv */
	342,	/* me_xstart */
	114,	/* me_ystart */
	435,	/* me_xend */
	165,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM28,	/* me_topleft */
	&varmen2_MM29,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM31 = {
	ME_VER+ME_FREE,
	114,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	342,	/* me_xstart */
	63,	/* me_ystart */
	435,	/* me_xend */
	165,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM27,	/* me_topleft */
	&varmen2_MM30,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM32 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"Unsigned",
	'?',	/* me_rv */
	344,	/* me_xstart */
	31,	/* me_ystart */
	434,	/* me_xend */
	59,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM31,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM33 = {
	ME_FREC,
	0,	/* me_pos */
	"Decimal",
	't',	/* me_rv */
	432,	/* me_xstart */
	63,	/* me_ystart */
	525,	/* me_xend */
	89,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM34 = {
	ME_FREC,
	0,	/* me_pos */
	"Octal",
	'o',	/* me_rv */
	432,	/* me_xstart */
	89,	/* me_ystart */
	525,	/* me_xend */
	115,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM35 = {
	ME_FREC,
	0,	/* me_pos */
	"Hex",
	'h',	/* me_rv */
	432,	/* me_xstart */
	115,	/* me_ystart */
	525,	/* me_xend */
	141,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM36 = {
	ME_VER,
	115,	/* me_pos */
	NULL,	/* me_cap */
	'o',	/* me_rv */
	432,	/* me_xstart */
	89,	/* me_ystart */
	525,	/* me_xend */
	141,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM34,	/* me_topleft */
	&varmen2_MM35,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM37 = {
	ME_VER+ME_FREE,
	89,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	432,	/* me_xstart */
	63,	/* me_ystart */
	525,	/* me_xend */
	141,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM33,	/* me_topleft */
	&varmen2_MM36,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM38 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"Signed",
	'?',	/* me_rv */
	434,	/* me_xstart */
	31,	/* me_ystart */
	525,	/* me_xend */
	59,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM37,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM39 = {
	ME_FREC,
	0,	/* me_pos */
	"Default",
	'n',	/* me_rv */
	525,	/* me_xstart */
	63,	/* me_ystart */
	636,	/* me_xend */
	88,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM40 = {
	ME_FREC,
	0,	/* me_pos */
	"Ascii",
	'a',	/* me_rv */
	525,	/* me_xstart */
	88,	/* me_ystart */
	636,	/* me_xend */
	114,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM41 = {
	ME_FREC,
	0,	/* me_pos */
	"String",
	's',	/* me_rv */
	525,	/* me_xstart */
	114,	/* me_ystart */
	636,	/* me_xend */
	140,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM42 = {
	ME_VER,
	114,	/* me_pos */
	NULL,	/* me_cap */
	'a',	/* me_rv */
	525,	/* me_xstart */
	88,	/* me_ystart */
	636,	/* me_xend */
	140,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM40,	/* me_topleft */
	&varmen2_MM41,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM43 = {
	ME_VER+ME_FREE,
	88,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	525,	/* me_xstart */
	63,	/* me_ystart */
	636,	/* me_xend */
	140,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM39,	/* me_topleft */
	&varmen2_MM42,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM44 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"Other",
	'b',	/* me_rv */
	525,	/* me_xstart */
	31,	/* me_ystart */
	616,	/* me_xend */
	59,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM43,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM45 = {
	ME_HOR,
	525,	/* me_pos */
	NULL,	/* me_cap */
	'?',	/* me_rv */
	434,	/* me_xstart */
	31,	/* me_ystart */
	616,	/* me_xend */
	59,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM38,	/* me_topleft */
	&varmen2_MM44,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM46 = {
	ME_HOR+ME_FREE,
	434,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	344,	/* me_xstart */
	31,	/* me_ystart */
	616,	/* me_xend */
	59,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM32,	/* me_topleft */
	&varmen2_MM45,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM47 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"Format",
	'?',	/* me_rv */
	436,	/* me_xstart */
	-1,	/* me_ystart */
	527,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM46,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM48 = {
	ME_FREC,
	0,	/* me_pos */
	"Use typedefs",
	'U',	/* me_rv */
	600,	/* me_xstart */
	24,	/* me_ystart */
	742,	/* me_xend */
	49,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM49 = {
	ME_FREC,
	0,	/* me_pos */
	"Ignore typedefs",
	'I',	/* me_rv */
	600,	/* me_xstart */
	49,	/* me_ystart */
	742,	/* me_xend */
	75,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM50 = {
	ME_VER+ME_FREE,
	49,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	600,	/* me_xstart */
	24,	/* me_ystart */
	742,	/* me_xend */
	75,	/* me_yend */
	0,	/* me_xcurs */
	-2,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM48,	/* me_topleft */
	&varmen2_MM49,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM51 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_NOSCALE+ME_CREL,
	0,	/* me_pos */
	"Decl",
	'q',	/* me_rv */
	527,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM50,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM52 = {
	ME_FREC,
	0,	/* me_pos */
	"Watch",
	'w',	/* me_rv */
	600,	/* me_xstart */
	-1,	/* me_ystart */
	682,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	NULL,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM53 = {
	ME_HOR,
	600,	/* me_pos */
	NULL,	/* me_cap */
	'q',	/* me_rv */
	527,	/* me_xstart */
	-1,	/* me_ystart */
	682,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM51,	/* me_topleft */
	&varmen2_MM52,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM54 = {
	ME_HOR,
	527,	/* me_pos */
	NULL,	/* me_cap */
	'?',	/* me_rv */
	436,	/* me_xstart */
	-1,	/* me_ystart */
	682,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM47,	/* me_topleft */
	&varmen2_MM53,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU varmen2_MM55 = {
	ME_HOR,
	436,	/* me_pos */
	NULL,	/* me_cap */
	'D',	/* me_rv */
	314,	/* me_xstart */
	-1,	/* me_ystart */
	682,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM24,	/* me_topleft */
	&varmen2_MM54,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU var_men2 = {
	ME_HOR+ME_FREE,
	314,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	682,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&varmen2_MM21,	/* me_topleft */
	&varmen2_MM55,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for wphmen.c --- */

static MENU wphmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Add new",
	'a',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	125,	/* me_xend */
	100,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	0,	/* me_topleft */
	0,	/* me_botrite */
	0,	/* me_parent */
	0,	/* me_omen */
	0,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU wphmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Remove all",
	'D',	/* me_rv */
	125,	/* me_xstart */
	-1,	/* me_ystart */
	252,	/* me_xend */
	100,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	0,	/* me_topleft */
	0,	/* me_botrite */
	0,	/* me_parent */
	0,	/* me_omen */
	0,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU wphmen_MM3 = {
	ME_HOR,
	125,	/* me_pos */
	0,
	'a',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	252,	/* me_xend */
	100,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&wphmen_MM1,	/* me_topleft */
	&wphmen_MM2,	/* me_botrite */
	0,	/* me_parent */
	0,	/* me_omen */
	0,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU wphmen_MM4 = {
	ME_FREC,
	0,	/* me_pos */
	"Enable",
	'e',	/* me_rv */
	252,	/* me_xstart */
	-1,	/* me_ystart */
	379,	/* me_xend */
	100,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	0,	/* me_topleft */
	0,	/* me_botrite */
	0,	/* me_parent */
	0,	/* me_omen */
	0,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU wphmen_MM5 = {
	ME_FREC,
	0,	/* me_pos */
	"Disable",
	'd',	/* me_rv */
	379,	/* me_xstart */
	-1,	/* me_ystart */
	506,	/* me_xend */
	100,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	0,	/* me_topleft */
	0,	/* me_botrite */
	0,	/* me_parent */
	0,	/* me_omen */
	0,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU wphmen_MM6 = {
	ME_HOR,
	379,	/* me_pos */
	0,
	'm',	/* me_rv */
	252,	/* me_xstart */
	-1,	/* me_ystart */
	506,	/* me_xend */
	100,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&wphmen_MM4,	/* me_topleft */
	&wphmen_MM5,	/* me_botrite */
	0,	/* me_parent */
	0,	/* me_omen */
	0,	/* me_toggle */
	0,	/* me_select_val */
};
MENU wph_men = {
	ME_HOR+ME_FREE,
	252,	/* me_pos */
	0,
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	506,	/* me_xend */
	100,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&wphmen_MM3,	/* me_topleft */
	&wphmen_MM6,	/* me_botrite */
	0,	/* me_parent */
	0,	/* me_omen */
	0,	/* me_toggle */
	0,	/* me_select_val */
};


/* --- Cut here for wptmen.c --- */

static MENU wptmen_MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Remove",
	'd',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	132,	/* me_xend */
	97,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	0,	/* me_topleft */
	0,	/* me_botrite */
	0,	/* me_parent */
	0,	/* me_omen */
	0,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU wptmen_MM2 = {
	ME_FREC,
	0,	/* me_pos */
	"Activate",
	'x',	/* me_rv */
	132,	/* me_xstart */
	-1,	/* me_ystart */
	266,	/* me_xend */
	97,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	0,	/* me_topleft */
	0,	/* me_botrite */
	0,	/* me_parent */
	0,	/* me_omen */
	0,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU wptmen_MM3 = {
	ME_FREC,
	0,	/* me_pos */
	"Inactivate",
	'y',	/* me_rv */
	266,	/* me_xstart */
	-1,	/* me_ystart */
	400,	/* me_xend */
	97,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	0,	/* me_topleft */
	0,	/* me_botrite */
	0,	/* me_parent */
	0,	/* me_omen */
	0,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU wptmen_MM4 = {
	ME_HOR,
	266,	/* me_pos */
	0,
	'x',	/* me_rv */
	132,	/* me_xstart */
	-1,	/* me_ystart */
	400,	/* me_xend */
	97,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&wptmen_MM2,	/* me_topleft */
	&wptmen_MM3,	/* me_botrite */
	0,	/* me_parent */
	0,	/* me_omen */
	0,	/* me_toggle */
	0,	/* me_select_val */
};
MENU wpt_men = {
	ME_HOR+ME_FREE,
	132,	/* me_pos */
	0,
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	400,	/* me_xend */
	97,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&wptmen_MM1,	/* me_topleft */
	&wptmen_MM4,	/* me_botrite */
	0,	/* me_parent */
	0,	/* me_omen */
	0,	/* me_toggle */
	0,	/* me_select_val */
};
