#include <stdio.h>
#include <local/menu3.h>

static MENU MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"&",
	'd',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	35,	/* me_xend */
	26,	/* me_yend */
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
	"*",
	'u',	/* me_rv */
	35,	/* me_xstart */
	-1,	/* me_ystart */
	72,	/* me_xend */
	26,	/* me_yend */
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
	ME_HOR,
	35,	/* me_pos */
	NULL,	/* me_cap */
	'd',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	72,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
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
	"::",
	'y',	/* me_rv */
	72,	/* me_xstart */
	-1,	/* me_ystart */
	109,	/* me_xend */
	26,	/* me_yend */
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
	"Struct/Union",
	'E',	/* me_rv */
	180,	/* me_xstart */
	15,	/* me_ystart */
	325,	/* me_xend */
	45,	/* me_yend */
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
	"Class Hierarchy",
	'y',	/* me_rv */
	180,	/* me_xstart */
	45,	/* me_ystart */
	325,	/* me_xend */
	75,	/* me_yend */
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
	ME_VER,
	45,	/* me_pos */
	NULL,	/* me_cap */
	'E',	/* me_rv */
	180,	/* me_xstart */
	15,	/* me_ystart */
	325,	/* me_xend */
	75,	/* me_yend */
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
	ME_FREC,
	0,	/* me_pos */
	"Static Members",
	'S',	/* me_rv */
	180,	/* me_xstart */
	75,	/* me_ystart */
	325,	/* me_xend */
	105,	/* me_yend */
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
	"Dump Memory",
	'W',	/* me_rv */
	180,	/* me_xstart */
	105,	/* me_ystart */
	325,	/* me_xend */
	135,	/* me_yend */
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
	"Show Size",
	'X',	/* me_rv */
	180,	/* me_xstart */
	135,	/* me_ystart */
	325,	/* me_xend */
	165,	/* me_yend */
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
	135,	/* me_pos */
	NULL,	/* me_cap */
	'W',	/* me_rv */
	180,	/* me_xstart */
	105,	/* me_ystart */
	325,	/* me_xend */
	165,	/* me_yend */
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
	ME_VER,
	105,	/* me_pos */
	NULL,	/* me_cap */
	'S',	/* me_rv */
	180,	/* me_xstart */
	75,	/* me_ystart */
	325,	/* me_xend */
	165,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM8,	/* me_topleft */
	&MM11,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM13 = {
	ME_VER+ME_FREE,
	75,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	180,	/* me_xstart */
	15,	/* me_ystart */
	325,	/* me_xend */
	165,	/* me_yend */
	-23,	/* me_xcurs */
	-11,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM7,	/* me_topleft */
	&MM12,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM14 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_CREL,
	0,	/* me_pos */
	"Expand",
	'E',	/* me_rv */
	109,	/* me_xstart */
	-1,	/* me_ystart */
	203,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM13,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM15 = {
	ME_HOR,
	109,	/* me_pos */
	NULL,	/* me_cap */
	'y',	/* me_rv */
	72,	/* me_xstart */
	-1,	/* me_ystart */
	203,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM4,	/* me_topleft */
	&MM14,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM16 = {
	ME_HOR,
	72,	/* me_pos */
	NULL,	/* me_cap */
	'd',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	203,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM3,	/* me_topleft */
	&MM15,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM17 = {
	ME_FREC,
	0,	/* me_pos */
	"First level",
	'c',	/* me_rv */
	279,	/* me_xstart */
	14,	/* me_ystart */
	417,	/* me_xend */
	42,	/* me_yend */
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
static MENU MM18 = {
	ME_FREC,
	0,	/* me_pos */
	"Completely",
	'C',	/* me_rv */
	279,	/* me_xstart */
	42,	/* me_ystart */
	417,	/* me_xend */
	71,	/* me_yend */
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
static MENU MM19 = {
	ME_VER+ME_FREE,
	42,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	279,	/* me_xstart */
	14,	/* me_ystart */
	417,	/* me_xend */
	71,	/* me_yend */
	-35,	/* me_xcurs */
	-12,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM17,	/* me_topleft */
	&MM18,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM20 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_CREL,
	0,	/* me_pos */
	"Collapse",
	'?',	/* me_rv */
	203,	/* me_xstart */
	-1,	/* me_ystart */
	314,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM19,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM21 = {
	ME_FREC,
	0,	/* me_pos */
	"Dup",
	'D',	/* me_rv */
	314,	/* me_xstart */
	-1,	/* me_ystart */
	371,	/* me_xend */
	26,	/* me_yend */
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
static MENU MM22 = {
	ME_HOR,
	314,	/* me_pos */
	NULL,	/* me_cap */
	'?',	/* me_rv */
	203,	/* me_xstart */
	-1,	/* me_ystart */
	371,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM20,	/* me_topleft */
	&MM21,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM23 = {
	ME_FREC,
	0,	/* me_pos */
	"Del",
	'k',	/* me_rv */
	371,	/* me_xstart */
	-1,	/* me_ystart */
	436,	/* me_xend */
	26,	/* me_yend */
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
static MENU MM24 = {
	ME_FREC,
	0,	/* me_pos */
	"Decimal",
	'T',	/* me_rv */
	342,	/* me_xstart */
	63,	/* me_ystart */
	435,	/* me_xend */
	88,	/* me_yend */
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
static MENU MM25 = {
	ME_FREC,
	0,	/* me_pos */
	"Octal",
	'O',	/* me_rv */
	342,	/* me_xstart */
	88,	/* me_ystart */
	435,	/* me_xend */
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
static MENU MM26 = {
	ME_VER,
	88,	/* me_pos */
	NULL,	/* me_cap */
	'T',	/* me_rv */
	342,	/* me_xstart */
	63,	/* me_ystart */
	435,	/* me_xend */
	114,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM24,	/* me_topleft */
	&MM25,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM27 = {
	ME_FREC,
	0,	/* me_pos */
	"Hex",
	'H',	/* me_rv */
	342,	/* me_xstart */
	114,	/* me_ystart */
	435,	/* me_xend */
	139,	/* me_yend */
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
static MENU MM28 = {
	ME_FREC,
	0,	/* me_pos */
	"Binary",
	'B',	/* me_rv */
	342,	/* me_xstart */
	139,	/* me_ystart */
	435,	/* me_xend */
	165,	/* me_yend */
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
static MENU MM29 = {
	ME_VER,
	139,	/* me_pos */
	NULL,	/* me_cap */
	'H',	/* me_rv */
	342,	/* me_xstart */
	114,	/* me_ystart */
	435,	/* me_xend */
	165,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM27,	/* me_topleft */
	&MM28,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM30 = {
	ME_VER+ME_FREE,
	114,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	342,	/* me_xstart */
	63,	/* me_ystart */
	435,	/* me_xend */
	165,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM26,	/* me_topleft */
	&MM29,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM31 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"Unsigned",
	'?',	/* me_rv */
	344,	/* me_xstart */
	31,	/* me_ystart */
	434,	/* me_xend */
	59,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM30,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM32 = {
	ME_FREC,
	0,	/* me_pos */
	"Decimal",
	't',	/* me_rv */
	432,	/* me_xstart */
	63,	/* me_ystart */
	525,	/* me_xend */
	89,	/* me_yend */
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
static MENU MM33 = {
	ME_FREC,
	0,	/* me_pos */
	"Octal",
	'o',	/* me_rv */
	432,	/* me_xstart */
	89,	/* me_ystart */
	525,	/* me_xend */
	115,	/* me_yend */
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
static MENU MM34 = {
	ME_FREC,
	0,	/* me_pos */
	"Hex",
	'h',	/* me_rv */
	432,	/* me_xstart */
	115,	/* me_ystart */
	525,	/* me_xend */
	141,	/* me_yend */
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
static MENU MM35 = {
	ME_VER,
	115,	/* me_pos */
	NULL,	/* me_cap */
	'o',	/* me_rv */
	432,	/* me_xstart */
	89,	/* me_ystart */
	525,	/* me_xend */
	141,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM33,	/* me_topleft */
	&MM34,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM36 = {
	ME_VER+ME_FREE,
	89,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	432,	/* me_xstart */
	63,	/* me_ystart */
	525,	/* me_xend */
	141,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM32,	/* me_topleft */
	&MM35,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM37 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"Signed",
	'?',	/* me_rv */
	434,	/* me_xstart */
	31,	/* me_ystart */
	525,	/* me_xend */
	59,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM36,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM38 = {
	ME_FREC,
	0,	/* me_pos */
	"Default",
	'n',	/* me_rv */
	525,	/* me_xstart */
	63,	/* me_ystart */
	636,	/* me_xend */
	88,	/* me_yend */
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
static MENU MM39 = {
	ME_FREC,
	0,	/* me_pos */
	"Ascii",
	'a',	/* me_rv */
	525,	/* me_xstart */
	88,	/* me_ystart */
	636,	/* me_xend */
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
static MENU MM40 = {
	ME_FREC,
	0,	/* me_pos */
	"String",
	's',	/* me_rv */
	525,	/* me_xstart */
	114,	/* me_ystart */
	636,	/* me_xend */
	140,	/* me_yend */
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
static MENU MM41 = {
	ME_VER,
	114,	/* me_pos */
	NULL,	/* me_cap */
	'a',	/* me_rv */
	525,	/* me_xstart */
	88,	/* me_ystart */
	636,	/* me_xend */
	140,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM39,	/* me_topleft */
	&MM40,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM42 = {
	ME_VER+ME_FREE,
	88,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	525,	/* me_xstart */
	63,	/* me_ystart */
	636,	/* me_xend */
	140,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM38,	/* me_topleft */
	&MM41,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM43 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"Other",
	'b',	/* me_rv */
	525,	/* me_xstart */
	31,	/* me_ystart */
	616,	/* me_xend */
	59,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM42,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM44 = {
	ME_HOR,
	525,	/* me_pos */
	NULL,	/* me_cap */
	'?',	/* me_rv */
	434,	/* me_xstart */
	31,	/* me_ystart */
	616,	/* me_xend */
	59,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM37,	/* me_topleft */
	&MM43,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM45 = {
	ME_HOR+ME_FREE,
	434,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	344,	/* me_xstart */
	31,	/* me_ystart */
	616,	/* me_xend */
	59,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM31,	/* me_topleft */
	&MM44,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM46 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"Format",
	'?',	/* me_rv */
	436,	/* me_xstart */
	-1,	/* me_ystart */
	527,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM45,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM47 = {
	ME_FREC,
	0,	/* me_pos */
	"Use typedefs",
	'U',	/* me_rv */
	600,	/* me_xstart */
	24,	/* me_ystart */
	742,	/* me_xend */
	49,	/* me_yend */
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
static MENU MM48 = {
	ME_FREC,
	0,	/* me_pos */
	"Ignore typedefs",
	'I',	/* me_rv */
	600,	/* me_xstart */
	49,	/* me_ystart */
	742,	/* me_xend */
	75,	/* me_yend */
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
static MENU MM49 = {
	ME_VER+ME_FREE,
	49,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	600,	/* me_xstart */
	24,	/* me_ystart */
	742,	/* me_xend */
	75,	/* me_yend */
	0,	/* me_xcurs */
	-2,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM47,	/* me_topleft */
	&MM48,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM50 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_NOSCALE+ME_CREL,
	0,	/* me_pos */
	"Decl",
	'q',	/* me_rv */
	527,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM49,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM51 = {
	ME_HOR,
	527,	/* me_pos */
	NULL,	/* me_cap */
	'?',	/* me_rv */
	436,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM46,	/* me_topleft */
	&MM50,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM52 = {
	ME_HOR,
	436,	/* me_pos */
	NULL,	/* me_cap */
	'k',	/* me_rv */
	371,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM23,	/* me_topleft */
	&MM51,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM53 = {
	ME_HOR,
	371,	/* me_pos */
	NULL,	/* me_cap */
	'?',	/* me_rv */
	203,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM22,	/* me_topleft */
	&MM52,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU var_men = {
	ME_HOR+ME_FREE,
	203,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	600,	/* me_xend */
	26,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM16,	/* me_topleft */
	&MM53,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
