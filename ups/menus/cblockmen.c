#include <stdio.h>
#include <local/menu3.h>

static MENU MM1 = {
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
static MENU MM2 = {
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
static MENU MM3 = {
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
static MENU MM6 = {
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
static MENU MM7 = {
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
	&MM5,	/* me_topleft */
	&MM6,	/* me_botrite */
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
	&MM4,	/* me_topleft */
	&MM7,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
