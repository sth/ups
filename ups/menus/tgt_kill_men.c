#include <stdio.h>
#include <local/menu3.h>

static MENU MM1 = {
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
static MENU MM2 = {
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
static MENU MM3 = {
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
	&MM1,	/* me_topleft */
	&MM2,	/* me_botrite */
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
	&MM3,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
