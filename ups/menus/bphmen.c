#include <stdio.h>
#include <local/menu3.h>

static MENU MM1 = {
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
static MENU MM2 = {
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
static MENU MM3 = {
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
static MENU MM4 = {
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
	&MM2,	/* me_topleft */
	&MM3,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM5 = {
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
static MENU MM7 = {
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
static MENU MM8 = {
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
	&MM6,	/* me_topleft */
	&MM7,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM9 = {
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
	&MM8,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM10 = {
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
static MENU MM11 = {
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
static MENU MM12 = {
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
	&MM10,	/* me_topleft */
	&MM11,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM13 = {
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
	&MM9,	/* me_topleft */
	&MM12,	/* me_botrite */
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
	&MM5,	/* me_topleft */
	&MM13,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
