#include <stdio.h>
#include <local/menu3.h>

static MENU MM1 = {
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
static MENU MM2 = {
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
static MENU MM3 = {
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
	&MM1,	/* me_topleft */
	&MM2,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM4 = {
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
	&MM3,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM5 = {
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
static MENU MM6 = {
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
	&MM4,	/* me_topleft */
	&MM5,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM7 = {
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
static MENU MM8 = {
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
static MENU MM9 = {
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
static MENU MM10 = {
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
static MENU MM11 = {
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
	&MM9,	/* me_topleft */
	&MM10,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM12 = {
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
static MENU MM13 = {
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
static MENU MM14 = {
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
static MENU MM15 = {
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
	&MM13,	/* me_topleft */
	&MM14,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM16 = {
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
	&MM12,	/* me_topleft */
	&MM15,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM17 = {
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
	&MM11,	/* me_topleft */
	&MM16,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM18 = {
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
	&MM17,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM19 = {
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
	&MM8,	/* me_topleft */
	&MM18,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM20 = {
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
	&MM7,	/* me_topleft */
	&MM19,	/* me_botrite */
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
	&MM6,	/* me_topleft */
	&MM20,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
