#include <stdio.h>
#include <local/menu3.h>

static MENU MM1 = {
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
static MENU MM2 = {
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
static MENU MM3 = {
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
	&MM1,	/* me_topleft */
	&MM2,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM4 = {
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
static MENU MM5 = {
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
static MENU MM6 = {
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
static MENU MM7 = {
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
	&MM5,	/* me_topleft */
	&MM6,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM8 = {
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
	&MM4,	/* me_topleft */
	&MM7,	/* me_botrite */
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
	&MM3,	/* me_topleft */
	&MM8,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
