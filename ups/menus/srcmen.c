#include <stdio.h>
#include <local/menu3.h>

static MENU MM1 = {
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
static MENU MM2 = {
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
static MENU MM3 = {
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
static MENU MM4 = {
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
	&MM4,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM6 = {
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
static MENU MM8 = {
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
static MENU MM9 = {
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
	&MM7,	/* me_topleft */
	&MM8,	/* me_botrite */
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
	&MM6,	/* me_topleft */
	&MM9,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
