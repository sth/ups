#include <stdio.h>
#include <local/menu3.h>

static MENU MM1 = {
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
static MENU MM2 = {
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
static MENU MM3 = {
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
static MENU MM4 = {
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
	&MM2,	/* me_topleft */
	&MM3,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM5 = {
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
	&MM4,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM6 = {
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
	&MM1,	/* me_topleft */
	&MM5,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM7 = {
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
static MENU MM8 = {
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
static MENU MM9 = {
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
static MENU MM10 = {
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
	&MM8,	/* me_topleft */
	&MM9,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM11 = {
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
	&MM7,	/* me_topleft */
	&MM10,	/* me_botrite */
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
	&MM6,	/* me_topleft */
	&MM11,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
