#include <stdio.h>
#include <local/menu3.h>

static MENU MM1 = {
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
static MENU MM2 = {
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
static MENU MM3 = {
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
static MENU MM4 = {
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
	&MM2,	/* me_topleft */
	&MM3,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM5 = {
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
	&MM1,	/* me_topleft */
	&MM4,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM6 = {
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
static MENU MM7 = {
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
static MENU MM8 = {
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
static MENU MM9 = {
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
	&MM7,	/* me_topleft */
	&MM8,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM10 = {
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
	&MM6,	/* me_topleft */
	&MM9,	/* me_botrite */
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
	&MM5,	/* me_topleft */
	&MM10,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
