#include <stdio.h>
#include <local/menu3.h>

static MENU MM1 = {
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
static MENU MM2 = {
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
static MENU MM3 = {
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
static MENU MM4 = {
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
	&MM2,	/* me_topleft */
	&MM3,	/* me_botrite */
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
	&MM1,	/* me_topleft */
	&MM4,	/* me_botrite */
	0,	/* me_parent */
	0,	/* me_omen */
	0,	/* me_toggle */
	0,	/* me_select_val */
};
