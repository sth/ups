#include <stdio.h>
#include <local/menu3.h>

static MENU MM1 = {
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
static MENU MM2 = {
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
static MENU MM3 = {
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
	&MM1,	/* me_topleft */
	&MM2,	/* me_botrite */
	0,	/* me_parent */
	0,	/* me_omen */
	0,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM4 = {
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
static MENU MM5 = {
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
static MENU MM6 = {
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
	&MM4,	/* me_topleft */
	&MM5,	/* me_botrite */
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
	&MM3,	/* me_topleft */
	&MM6,	/* me_botrite */
	0,	/* me_parent */
	0,	/* me_omen */
	0,	/* me_toggle */
	0,	/* me_select_val */
};
