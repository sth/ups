#include <stdio.h>
#include <local/menu3.h>

static MENU MM1 = {
	ME_FREC,
	0,	/* me_pos */
	"Web site URL",
	'o',	/* me_rv */
	55,	/* me_xstart */
	21,	/* me_ystart */
	350,	/* me_xend */
	55,	/* me_yend */
	1064,	/* me_xcurs */
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
	"cut and paste",
	'0',	/* me_rv */
	55,	/* me_xstart */
	55,	/* me_ystart */
	350,	/* me_xend */
	90,	/* me_yend */
	1064,	/* me_xcurs */
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
	ME_VER,
	55,	/* me_pos */
	NULL,	/* me_cap */
	'o',	/* me_rv */
	55,	/* me_xstart */
	21,	/* me_ystart */
	350,	/* me_xend */
	90,	/* me_yend */
	1064,	/* me_xcurs */
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
	ME_FREC,
	0,	/* me_pos */
	"saving state",
	't',	/* me_rv */
	55,	/* me_xstart */
	90,	/* me_ystart */
	350,	/* me_xend */
	125,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
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
	"in source",
	'A',	/* me_rv */
	316,	/* me_xstart */
	125,	/* me_ystart */
	588,	/* me_xend */
	159,	/* me_yend */
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
	"by name",
	'B',	/* me_rv */
	316,	/* me_xstart */
	159,	/* me_ystart */
	588,	/* me_xend */
	194,	/* me_yend */
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
	159,	/* me_pos */
	NULL,	/* me_cap */
	'A',	/* me_rv */
	316,	/* me_xstart */
	125,	/* me_ystart */
	588,	/* me_xend */
	194,	/* me_yend */
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
	"conditional",
	'd',	/* me_rv */
	316,	/* me_xstart */
	194,	/* me_ystart */
	588,	/* me_xend */
	228,	/* me_yend */
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
	"saving breakpoints",
	'u',	/* me_rv */
	316,	/* me_xstart */
	228,	/* me_ystart */
	588,	/* me_xend */
	263,	/* me_yend */
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
	"C++ syntax",
	'C',	/* me_rv */
	316,	/* me_xstart */
	263,	/* me_ystart */
	588,	/* me_xend */
	298,	/* me_yend */
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
	263,	/* me_pos */
	NULL,	/* me_cap */
	'u',	/* me_rv */
	316,	/* me_xstart */
	228,	/* me_ystart */
	588,	/* me_xend */
	298,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
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
	228,	/* me_pos */
	NULL,	/* me_cap */
	'd',	/* me_rv */
	316,	/* me_xstart */
	194,	/* me_ystart */
	588,	/* me_xend */
	298,	/* me_yend */
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
	ME_VER,
	194,	/* me_pos */
	NULL,	/* me_cap */
	'A',	/* me_rv */
	316,	/* me_xstart */
	125,	/* me_ystart */
	588,	/* me_xend */
	298,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
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
	ME_FREC,
	0,	/* me_pos */
	"auto completion",
	'D',	/* me_rv */
	316,	/* me_xstart */
	298,	/* me_ystart */
	588,	/* me_xend */
	332,	/* me_yend */
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
	ME_FREC,
	0,	/* me_pos */
	"listing matches",
	'E',	/* me_rv */
	316,	/* me_xstart */
	332,	/* me_ystart */
	588,	/* me_xend */
	367,	/* me_yend */
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
static MENU MM16 = {
	ME_VER,
	332,	/* me_pos */
	NULL,	/* me_cap */
	'D',	/* me_rv */
	316,	/* me_xstart */
	298,	/* me_ystart */
	588,	/* me_xend */
	367,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM14,	/* me_topleft */
	&MM15,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM17 = {
	ME_FREC+ME_NOSCALE,
	0,	/* me_pos */
	"activation",
	'F',	/* me_rv */
	316,	/* me_xstart */
	367,	/* me_ystart */
	588,	/* me_xend */
	401,	/* me_yend */
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
	"execute",
	'G',	/* me_rv */
	316,	/* me_xstart */
	401,	/* me_ystart */
	588,	/* me_xend */
	436,	/* me_yend */
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
	ME_FREC,
	0,	/* me_pos */
	"duplicates",
	'H',	/* me_rv */
	316,	/* me_xstart */
	436,	/* me_ystart */
	588,	/* me_xend */
	471,	/* me_yend */
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
static MENU MM20 = {
	ME_VER,
	436,	/* me_pos */
	NULL,	/* me_cap */
	'G',	/* me_rv */
	316,	/* me_xstart */
	401,	/* me_ystart */
	588,	/* me_xend */
	471,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM18,	/* me_topleft */
	&MM19,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM21 = {
	ME_VER,
	401,	/* me_pos */
	NULL,	/* me_cap */
	'F',	/* me_rv */
	316,	/* me_xstart */
	367,	/* me_ystart */
	588,	/* me_xend */
	471,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM17,	/* me_topleft */
	&MM20,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM22 = {
	ME_VER,
	367,	/* me_pos */
	NULL,	/* me_cap */
	'D',	/* me_rv */
	316,	/* me_xstart */
	298,	/* me_ystart */
	588,	/* me_xend */
	471,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM16,	/* me_topleft */
	&MM21,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM23 = {
	ME_VER+ME_FREE,
	298,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	316,	/* me_xstart */
	125,	/* me_ystart */
	588,	/* me_xend */
	471,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM13,	/* me_topleft */
	&MM22,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM24 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"breakpoints",
	'1',	/* me_rv */
	55,	/* me_xstart */
	125,	/* me_ystart */
	350,	/* me_xend */
	159,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM23,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM25 = {
	ME_VER,
	125,	/* me_pos */
	NULL,	/* me_cap */
	't',	/* me_rv */
	55,	/* me_xstart */
	90,	/* me_ystart */
	350,	/* me_xend */
	159,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM4,	/* me_topleft */
	&MM24,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM26 = {
	ME_VER,
	90,	/* me_pos */
	NULL,	/* me_cap */
	'o',	/* me_rv */
	55,	/* me_xstart */
	21,	/* me_ystart */
	350,	/* me_xend */
	159,	/* me_yend */
	1064,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM3,	/* me_topleft */
	&MM25,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM27 = {
	ME_FREC,
	0,	/* me_pos */
	"accelerators",
	'2',	/* me_rv */
	55,	/* me_xstart */
	159,	/* me_ystart */
	350,	/* me_xend */
	194,	/* me_yend */
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
	"typing shortcuts",
	'p',	/* me_rv */
	55,	/* me_xstart */
	194,	/* me_ystart */
	350,	/* me_xend */
	229,	/* me_yend */
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
	194,	/* me_pos */
	NULL,	/* me_cap */
	'2',	/* me_rv */
	55,	/* me_xstart */
	159,	/* me_ystart */
	350,	/* me_xend */
	229,	/* me_yend */
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
	ME_FREC,
	0,	/* me_pos */
	"general",
	'I',	/* me_rv */
	319,	/* me_xstart */
	229,	/* me_ystart */
	589,	/* me_xend */
	263,	/* me_yend */
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
static MENU MM31 = {
	ME_FREC,
	0,	/* me_pos */
	"step button",
	'J',	/* me_rv */
	319,	/* me_xstart */
	263,	/* me_ystart */
	589,	/* me_xend */
	298,	/* me_yend */
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
static MENU MM32 = {
	ME_FREC,
	0,	/* me_pos */
	"casting",
	'K',	/* me_rv */
	319,	/* me_xstart */
	298,	/* me_ystart */
	589,	/* me_xend */
	333,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
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
	ME_VER,
	298,	/* me_pos */
	NULL,	/* me_cap */
	'J',	/* me_rv */
	319,	/* me_xstart */
	263,	/* me_ystart */
	589,	/* me_xend */
	333,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM31,	/* me_topleft */
	&MM32,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM34 = {
	ME_VER,
	263,	/* me_pos */
	NULL,	/* me_cap */
	'I',	/* me_rv */
	319,	/* me_xstart */
	229,	/* me_ystart */
	589,	/* me_xend */
	333,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM30,	/* me_topleft */
	&MM33,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM35 = {
	ME_FREC,
	0,	/* me_pos */
	"tags",
	'L',	/* me_rv */
	319,	/* me_xstart */
	333,	/* me_ystart */
	589,	/* me_xend */
	367,	/* me_yend */
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
static MENU MM36 = {
	ME_FREC,
	0,	/* me_pos */
	"base classes",
	'n',	/* me_rv */
	319,	/* me_xstart */
	367,	/* me_ystart */
	589,	/* me_xend */
	402,	/* me_yend */
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
static MENU MM37 = {
	ME_FREC,
	0,	/* me_pos */
	"breakpoint code",
	'M',	/* me_rv */
	319,	/* me_xstart */
	402,	/* me_ystart */
	589,	/* me_xend */
	437,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
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
static MENU MM38 = {
	ME_VER,
	402,	/* me_pos */
	NULL,	/* me_cap */
	'n',	/* me_rv */
	319,	/* me_xstart */
	367,	/* me_ystart */
	589,	/* me_xend */
	437,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM36,	/* me_topleft */
	&MM37,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM39 = {
	ME_VER,
	367,	/* me_pos */
	NULL,	/* me_cap */
	'L',	/* me_rv */
	319,	/* me_xstart */
	333,	/* me_ystart */
	589,	/* me_xend */
	437,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM35,	/* me_topleft */
	&MM38,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM40 = {
	ME_VER+ME_FREE,
	333,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	319,	/* me_xstart */
	229,	/* me_ystart */
	589,	/* me_xend */
	437,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM34,	/* me_topleft */
	&MM39,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM41 = {
	ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"C++ support",
	'3',	/* me_rv */
	55,	/* me_xstart */
	229,	/* me_ystart */
	350,	/* me_xend */
	263,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM40,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM42 = {
	ME_FREC,
	0,	/* me_pos */
	"source files",
	'4',	/* me_rv */
	55,	/* me_xstart */
	263,	/* me_ystart */
	350,	/* me_xend */
	298,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
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
static MENU MM43 = {
	ME_FREC,
	0,	/* me_pos */
	"general",
	'S',	/* me_rv */
	303,	/* me_xstart */
	298,	/* me_ystart */
	536,	/* me_xend */
	332,	/* me_yend */
	0,	/* me_xcurs */
	628,	/* me_ycurs */
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
static MENU MM44 = {
	ME_FREC,
	0,	/* me_pos */
	"pointers",
	'T',	/* me_rv */
	303,	/* me_xstart */
	332,	/* me_ystart */
	536,	/* me_xend */
	367,	/* me_yend */
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
static MENU MM45 = {
	ME_VER,
	332,	/* me_pos */
	NULL,	/* me_cap */
	'S',	/* me_rv */
	303,	/* me_xstart */
	298,	/* me_ystart */
	536,	/* me_xend */
	367,	/* me_yend */
	0,	/* me_xcurs */
	285,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM43,	/* me_topleft */
	&MM44,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM46 = {
	ME_FREC,
	0,	/* me_pos */
	"arrays",
	'U',	/* me_rv */
	303,	/* me_xstart */
	367,	/* me_ystart */
	536,	/* me_xend */
	402,	/* me_yend */
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
static MENU MM47 = {
	ME_FREC,
	0,	/* me_pos */
	"structures",
	'V',	/* me_rv */
	303,	/* me_xstart */
	402,	/* me_ystart */
	536,	/* me_xend */
	436,	/* me_yend */
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
	ME_VER,
	402,	/* me_pos */
	NULL,	/* me_cap */
	'U',	/* me_rv */
	303,	/* me_xstart */
	367,	/* me_ystart */
	536,	/* me_xend */
	436,	/* me_yend */
	0,	/* me_xcurs */
	663,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM46,	/* me_topleft */
	&MM47,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM49 = {
	ME_VER,
	367,	/* me_pos */
	NULL,	/* me_cap */
	'S',	/* me_rv */
	303,	/* me_xstart */
	298,	/* me_ystart */
	536,	/* me_xend */
	436,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM45,	/* me_topleft */
	&MM48,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM50 = {
	ME_FREC,
	0,	/* me_pos */
	"linked data",
	'W',	/* me_rv */
	303,	/* me_xstart */
	436,	/* me_ystart */
	536,	/* me_xend */
	471,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
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
static MENU MM51 = {
	ME_FREC,
	0,	/* me_pos */
	"changing values",
	'X',	/* me_rv */
	303,	/* me_xstart */
	471,	/* me_ystart */
	536,	/* me_xend */
	506,	/* me_yend */
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
static MENU MM52 = {
	ME_VER,
	471,	/* me_pos */
	NULL,	/* me_cap */
	'W',	/* me_rv */
	303,	/* me_xstart */
	436,	/* me_ystart */
	536,	/* me_xend */
	506,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM50,	/* me_topleft */
	&MM51,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM53 = {
	ME_FREC,
	0,	/* me_pos */
	"typedefs",
	'Y',	/* me_rv */
	303,	/* me_xstart */
	506,	/* me_ystart */
	536,	/* me_xend */
	540,	/* me_yend */
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
static MENU MM54 = {
	ME_FREC,
	0,	/* me_pos */
	"expressions",
	'Z',	/* me_rv */
	303,	/* me_xstart */
	540,	/* me_ystart */
	536,	/* me_xend */
	575,	/* me_yend */
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
static MENU MM55 = {
	ME_FREC,
	0,	/* me_pos */
	"dump memory",
	'a',	/* me_rv */
	303,	/* me_xstart */
	575,	/* me_ystart */
	536,	/* me_xend */
	610,	/* me_yend */
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
static MENU MM56 = {
	ME_VER,
	575,	/* me_pos */
	NULL,	/* me_cap */
	'Z',	/* me_rv */
	303,	/* me_xstart */
	540,	/* me_ystart */
	536,	/* me_xend */
	610,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM54,	/* me_topleft */
	&MM55,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM57 = {
	ME_VER,
	540,	/* me_pos */
	NULL,	/* me_cap */
	'Y',	/* me_rv */
	303,	/* me_xstart */
	506,	/* me_ystart */
	536,	/* me_xend */
	610,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM53,	/* me_topleft */
	&MM56,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM58 = {
	ME_VER,
	506,	/* me_pos */
	NULL,	/* me_cap */
	'W',	/* me_rv */
	303,	/* me_xstart */
	436,	/* me_ystart */
	536,	/* me_xend */
	610,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM52,	/* me_topleft */
	&MM57,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM59 = {
	ME_VER+ME_FREE,
	436,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	303,	/* me_xstart */
	298,	/* me_ystart */
	536,	/* me_xend */
	610,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM49,	/* me_topleft */
	&MM58,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM60 = {
	ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"examing data",
	'5',	/* me_rv */
	55,	/* me_xstart */
	298,	/* me_ystart */
	350,	/* me_xend */
	333,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM59,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM61 = {
	ME_VER,
	298,	/* me_pos */
	NULL,	/* me_cap */
	'4',	/* me_rv */
	55,	/* me_xstart */
	263,	/* me_ystart */
	350,	/* me_xend */
	333,	/* me_yend */
	0,	/* me_xcurs */
	3,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM42,	/* me_topleft */
	&MM60,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM62 = {
	ME_VER,
	263,	/* me_pos */
	NULL,	/* me_cap */
	'3',	/* me_rv */
	55,	/* me_xstart */
	229,	/* me_ystart */
	350,	/* me_xend */
	333,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM41,	/* me_topleft */
	&MM61,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM63 = {
	ME_VER,
	229,	/* me_pos */
	NULL,	/* me_cap */
	'2',	/* me_rv */
	55,	/* me_xstart */
	159,	/* me_ystart */
	350,	/* me_xend */
	333,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM29,	/* me_topleft */
	&MM62,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM64 = {
	ME_VER,
	159,	/* me_pos */
	NULL,	/* me_cap */
	'o',	/* me_rv */
	55,	/* me_xstart */
	21,	/* me_ystart */
	350,	/* me_xend */
	333,	/* me_yend */
	1064,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM26,	/* me_topleft */
	&MM63,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM65 = {
	ME_FREC,
	0,	/* me_pos */
	"general",
	'6',	/* me_rv */
	305,	/* me_xstart */
	333,	/* me_ystart */
	539,	/* me_xend */
	367,	/* me_yend */
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
static MENU MM66 = {
	ME_FREC,
	0,	/* me_pos */
	"special characters",
	'e',	/* me_rv */
	305,	/* me_xstart */
	367,	/* me_ystart */
	539,	/* me_xend */
	402,	/* me_yend */
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
static MENU MM67 = {
	ME_FREC,
	0,	/* me_pos */
	"edit history",
	'i',	/* me_rv */
	305,	/* me_xstart */
	402,	/* me_ystart */
	539,	/* me_xend */
	437,	/* me_yend */
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
static MENU MM68 = {
	ME_VER,
	402,	/* me_pos */
	NULL,	/* me_cap */
	'e',	/* me_rv */
	305,	/* me_xstart */
	367,	/* me_ystart */
	539,	/* me_xend */
	437,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM66,	/* me_topleft */
	&MM67,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM69 = {
	ME_VER+ME_FREE,
	367,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	305,	/* me_xstart */
	333,	/* me_ystart */
	539,	/* me_xend */
	437,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM65,	/* me_topleft */
	&MM68,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM70 = {
	ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"editable fields",
	0,	/* me_rv */
	55,	/* me_xstart */
	333,	/* me_ystart */
	350,	/* me_xend */
	367,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM69,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM71 = {
	ME_FREC,
	0,	/* me_pos */
	"command arguments",
	'7',	/* me_rv */
	55,	/* me_xstart */
	367,	/* me_ystart */
	350,	/* me_xend */
	402,	/* me_yend */
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
static MENU MM72 = {
	ME_VER,
	367,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	55,	/* me_xstart */
	333,	/* me_ystart */
	350,	/* me_xend */
	402,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM70,	/* me_topleft */
	&MM71,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM73 = {
	ME_FREC,
	0,	/* me_pos */
	"custom menu",
	'8',	/* me_rv */
	55,	/* me_xstart */
	402,	/* me_ystart */
	350,	/* me_xend */
	437,	/* me_yend */
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
static MENU MM74 = {
	ME_FREC,
	0,	/* me_pos */
	"init file",
	'9',	/* me_rv */
	55,	/* me_xstart */
	437,	/* me_ystart */
	350,	/* me_xend */
	471,	/* me_yend */
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
static MENU MM75 = {
	ME_VER,
	437,	/* me_pos */
	NULL,	/* me_cap */
	'8',	/* me_rv */
	55,	/* me_xstart */
	402,	/* me_ystart */
	350,	/* me_xend */
	471,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM73,	/* me_topleft */
	&MM74,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM76 = {
	ME_VER,
	402,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	55,	/* me_xstart */
	333,	/* me_ystart */
	350,	/* me_xend */
	471,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM72,	/* me_topleft */
	&MM75,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM77 = {
	ME_FREC,
	0,	/* me_pos */
	"attach / detach",
	'!',	/* me_rv */
	55,	/* me_xstart */
	471,	/* me_ystart */
	350,	/* me_xend */
	506,	/* me_yend */
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
static MENU MM78 = {
	ME_FREC,
	0,	/* me_pos */
	"signals",
	'h',	/* me_rv */
	55,	/* me_xstart */
	506,	/* me_ystart */
	350,	/* me_xend */
	541,	/* me_yend */
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
static MENU MM79 = {
	ME_VER,
	506,	/* me_pos */
	NULL,	/* me_cap */
	'!',	/* me_rv */
	55,	/* me_xstart */
	471,	/* me_ystart */
	350,	/* me_xend */
	541,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM77,	/* me_topleft */
	&MM78,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM80 = {
	ME_FREC,
	0,	/* me_pos */
	"About X Resources",
	0x100+'c',	/* me_rv */
	305,	/* me_xstart */
	541,	/* me_ystart */
	570,	/* me_xend */
	575,	/* me_yend */
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
static MENU MM81 = {
	ME_FREC,
	0,	/* me_pos */
	"Fonts and Colors",
	0x100+'d',	/* me_rv */
	305,	/* me_xstart */
	575,	/* me_ystart */
	570,	/* me_xend */
	610,	/* me_yend */
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
static MENU MM82 = {
	ME_VER,
	575,	/* me_pos */
	NULL,	/* me_cap */
	0x100+'c',	/* me_rv */
	305,	/* me_xstart */
	541,	/* me_ystart */
	570,	/* me_xend */
	610,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM80,	/* me_topleft */
	&MM81,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM83 = {
	ME_FREC,
	0,	/* me_pos */
	"Split Screens",
	0x100+'e',	/* me_rv */
	305,	/* me_xstart */
	610,	/* me_ystart */
	570,	/* me_xend */
	645,	/* me_yend */
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
static MENU MM84 = {
	ME_FREC,
	0,	/* me_pos */
	"Raise/Lower",
	0x100+'f',	/* me_rv */
	305,	/* me_xstart */
	645,	/* me_ystart */
	570,	/* me_xend */
	680,	/* me_yend */
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
static MENU MM85 = {
	ME_VER,
	645,	/* me_pos */
	NULL,	/* me_cap */
	0x100+'e',	/* me_rv */
	305,	/* me_xstart */
	610,	/* me_ystart */
	570,	/* me_xend */
	680,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM83,	/* me_topleft */
	&MM84,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM86 = {
	ME_VER,
	610,	/* me_pos */
	NULL,	/* me_cap */
	0x100+'c',	/* me_rv */
	305,	/* me_xstart */
	541,	/* me_ystart */
	570,	/* me_xend */
	680,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM82,	/* me_topleft */
	&MM85,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM87 = {
	ME_FREC,
	0,	/* me_pos */
	"Breakpoint Menu",
	0x100+'g',	/* me_rv */
	305,	/* me_xstart */
	680,	/* me_ystart */
	570,	/* me_xend */
	714,	/* me_yend */
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
static MENU MM88 = {
	ME_FREC,
	0,	/* me_pos */
	"Scrolling",
	0x100+'h',	/* me_rv */
	305,	/* me_xstart */
	714,	/* me_ystart */
	570,	/* me_xend */
	749,	/* me_yend */
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
static MENU MM89 = {
	ME_VER,
	714,	/* me_pos */
	NULL,	/* me_cap */
	0x100+'g',	/* me_rv */
	305,	/* me_xstart */
	680,	/* me_ystart */
	570,	/* me_xend */
	749,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM87,	/* me_topleft */
	&MM88,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM90 = {
	ME_FREC,
	0,	/* me_pos */
	"Resizing Windows",
	'g',	/* me_rv */
	305,	/* me_xstart */
	749,	/* me_ystart */
	570,	/* me_xend */
	784,	/* me_yend */
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
static MENU MM91 = {
	ME_FREC,
	0,	/* me_pos */
	"No Mousehole",
	0x100+'i',	/* me_rv */
	305,	/* me_xstart */
	784,	/* me_ystart */
	570,	/* me_xend */
	819,	/* me_yend */
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
static MENU MM92 = {
	ME_VER,
	784,	/* me_pos */
	NULL,	/* me_cap */
	'g',	/* me_rv */
	305,	/* me_xstart */
	749,	/* me_ystart */
	570,	/* me_xend */
	819,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM90,	/* me_topleft */
	&MM91,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM93 = {
	ME_VER,
	749,	/* me_pos */
	NULL,	/* me_cap */
	0x100+'g',	/* me_rv */
	305,	/* me_xstart */
	680,	/* me_ystart */
	570,	/* me_xend */
	819,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM89,	/* me_topleft */
	&MM92,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM94 = {
	ME_VER+ME_FREE,
	680,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	305,	/* me_xstart */
	541,	/* me_ystart */
	570,	/* me_xend */
	819,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM86,	/* me_topleft */
	&MM93,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM95 = {
	ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"preferences",
	0,	/* me_rv */
	55,	/* me_xstart */
	541,	/* me_ystart */
	350,	/* me_xend */
	575,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM94,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM96 = {
	ME_FREC,
	0,	/* me_pos */
	"ups version",
	'v',	/* me_rv */
	312,	/* me_xstart */
	575,	/* me_ystart */
	558,	/* me_xend */
	610,	/* me_yend */
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
static MENU MM97 = {
	ME_FREC,
	0,	/* me_pos */
	"`step' action",
	's',	/* me_rv */
	312,	/* me_xstart */
	610,	/* me_ystart */
	558,	/* me_xend */
	645,	/* me_yend */
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
static MENU MM98 = {
	ME_VER,
	610,	/* me_pos */
	NULL,	/* me_cap */
	'v',	/* me_rv */
	312,	/* me_xstart */
	575,	/* me_ystart */
	558,	/* me_xend */
	645,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM96,	/* me_topleft */
	&MM97,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM99 = {
	ME_FREC,
	0,	/* me_pos */
	"scrollbars",
	'O',	/* me_rv */
	312,	/* me_xstart */
	645,	/* me_ystart */
	558,	/* me_xend */
	680,	/* me_yend */
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
static MENU MM100 = {
	ME_FREC,
	0,	/* me_pos */
	"assembler",
	'P',	/* me_rv */
	312,	/* me_xstart */
	680,	/* me_ystart */
	558,	/* me_xend */
	715,	/* me_yend */
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
static MENU MM101 = {
	ME_FREC,
	0,	/* me_pos */
	"authors",
	'Q',	/* me_rv */
	312,	/* me_xstart */
	715,	/* me_ystart */
	558,	/* me_xend */
	751,	/* me_yend */
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
static MENU MM102 = {
	ME_VER,
	715,	/* me_pos */
	NULL,	/* me_cap */
	'P',	/* me_rv */
	312,	/* me_xstart */
	680,	/* me_ystart */
	558,	/* me_xend */
	751,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM100,	/* me_topleft */
	&MM101,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM103 = {
	ME_VER,
	680,	/* me_pos */
	NULL,	/* me_cap */
	'O',	/* me_rv */
	312,	/* me_xstart */
	645,	/* me_ystart */
	558,	/* me_xend */
	751,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM99,	/* me_topleft */
	&MM102,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM104 = {
	ME_VER+ME_FREE,
	645,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	312,	/* me_xstart */
	575,	/* me_ystart */
	558,	/* me_xend */
	751,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM98,	/* me_topleft */
	&MM103,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM105 = {
	ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"miscellaneous",
	'i',	/* me_rv */
	55,	/* me_xstart */
	575,	/* me_ystart */
	350,	/* me_xend */
	610,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM104,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM106 = {
	ME_FREC,
	0,	/* me_pos */
	"append text",
	'l',	/* me_rv */
	309,	/* me_xstart */
	611,	/* me_ystart */
	582,	/* me_xend */
	645,	/* me_yend */
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
static MENU MM107 = {
	ME_FREC,
	0,	/* me_pos */
	"always clear",
	'm',	/* me_rv */
	309,	/* me_xstart */
	645,	/* me_ystart */
	582,	/* me_xend */
	680,	/* me_yend */
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
static MENU MM108 = {
	ME_VER+ME_FREE,
	645,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	309,	/* me_xstart */
	611,	/* me_ystart */
	582,	/* me_xend */
	680,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM106,	/* me_topleft */
	&MM107,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM109 = {
	ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"help window",
	'j',	/* me_rv */
	55,	/* me_xstart */
	610,	/* me_ystart */
	350,	/* me_xend */
	645,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM108,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM110 = {
	ME_VER,
	610,	/* me_pos */
	NULL,	/* me_cap */
	'i',	/* me_rv */
	55,	/* me_xstart */
	575,	/* me_ystart */
	350,	/* me_xend */
	645,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM105,	/* me_topleft */
	&MM109,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM111 = {
	ME_VER,
	575,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	55,	/* me_xstart */
	541,	/* me_ystart */
	350,	/* me_xend */
	645,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM95,	/* me_topleft */
	&MM110,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM112 = {
	ME_VER,
	541,	/* me_pos */
	NULL,	/* me_cap */
	'!',	/* me_rv */
	55,	/* me_xstart */
	471,	/* me_ystart */
	350,	/* me_xend */
	645,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM79,	/* me_topleft */
	&MM111,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM113 = {
	ME_VER,
	471,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	55,	/* me_xstart */
	333,	/* me_ystart */
	350,	/* me_xend */
	645,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM76,	/* me_topleft */
	&MM112,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM114 = {
	ME_VER+ME_FREE,
	333,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	55,	/* me_xstart */
	21,	/* me_ystart */
	350,	/* me_xend */
	645,	/* me_yend */
	-21,	/* me_xcurs */
	-11,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM64,	/* me_topleft */
	&MM113,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM115 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_CREL,
	0,	/* me_pos */
	"Help",
	'e',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	76,	/* me_xend */
	32,	/* me_yend */
	1064,	/* me_xcurs */
	-22,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM114,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM116 = {
	ME_FREC,
	0,	/* me_pos */
	"Backwards",
	'b',	/* me_rv */
	2,	/* me_xstart */
	0,	/* me_ystart */
	134,	/* me_xend */
	35,	/* me_yend */
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
static MENU MM117 = {
	ME_FREC,
	0,	/* me_pos */
	"Forwards",
	'f',	/* me_rv */
	134,	/* me_xstart */
	0,	/* me_ystart */
	291,	/* me_xend */
	35,	/* me_yend */
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
static MENU MM118 = {
	ME_HOR+ME_FREE,
	134,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	2,	/* me_xstart */
	0,	/* me_ystart */
	291,	/* me_xend */
	35,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM116,	/* me_topleft */
	&MM117,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM119 = {
	ME_POPUP+ME_FREC+ME_ISUB,
	0,	/* me_pos */
	"Search",
	0x100+'j',	/* me_rv */
	76,	/* me_xstart */
	-1,	/* me_ystart */
	194,	/* me_xend */
	32,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM118,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM120 = {
	ME_HOR,
	76,	/* me_pos */
	NULL,	/* me_cap */
	'e',	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	194,	/* me_xend */
	32,	/* me_yend */
	1064,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM115,	/* me_topleft */
	&MM119,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM121 = {
	ME_FREC,
	0,	/* me_pos */
	"Snapshot Selected Object",
	'c',	/* me_rv */
	236,	/* me_xstart */
	10,	/* me_ystart */
	606,	/* me_xend */
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
static MENU MM122 = {
	ME_FREC,
	0,	/* me_pos */
	"Snapshot All Objects",
	'w',	/* me_rv */
	236,	/* me_xstart */
	45,	/* me_ystart */
	606,	/* me_xend */
	80,	/* me_yend */
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
static MENU MM123 = {
	ME_VER,
	45,	/* me_pos */
	NULL,	/* me_cap */
	'c',	/* me_rv */
	236,	/* me_xstart */
	10,	/* me_ystart */
	606,	/* me_xend */
	80,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM121,	/* me_topleft */
	&MM122,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM124 = {
	ME_FREC,
	0,	/* me_pos */
	"Raise Display",
	'R',	/* me_rv */
	236,	/* me_xstart */
	80,	/* me_ystart */
	606,	/* me_xend */
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
static MENU MM125 = {
	ME_FREC,
	0,	/* me_pos */
	"Raise Source",
	'r',	/* me_rv */
	236,	/* me_xstart */
	115,	/* me_ystart */
	606,	/* me_xend */
	150,	/* me_yend */
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
static MENU MM126 = {
	ME_FREC,
	0,	/* me_pos */
	"Raise Output",
	'z',	/* me_rv */
	236,	/* me_xstart */
	150,	/* me_ystart */
	606,	/* me_xend */
	185,	/* me_yend */
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
static MENU MM127 = {
	ME_VER,
	150,	/* me_pos */
	NULL,	/* me_cap */
	'r',	/* me_rv */
	236,	/* me_xstart */
	115,	/* me_ystart */
	606,	/* me_xend */
	185,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM125,	/* me_topleft */
	&MM126,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM128 = {
	ME_VER,
	115,	/* me_pos */
	NULL,	/* me_cap */
	'R',	/* me_rv */
	236,	/* me_xstart */
	80,	/* me_ystart */
	606,	/* me_xend */
	185,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM124,	/* me_topleft */
	&MM127,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM129 = {
	ME_VER,
	80,	/* me_pos */
	NULL,	/* me_cap */
	'c',	/* me_rv */
	236,	/* me_xstart */
	10,	/* me_ystart */
	606,	/* me_xend */
	185,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM123,	/* me_topleft */
	&MM128,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM130 = {
	ME_FREC,
	0,	/* me_pos */
	"Message Logging",
	'x',	/* me_rv */
	236,	/* me_xstart */
	185,	/* me_ystart */
	606,	/* me_xend */
	221,	/* me_yend */
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
static MENU MM131 = {
	ME_FREC,
	0,	/* me_pos */
	"No Raise On Break",
	0x100+'a',	/* me_rv */
	236,	/* me_xstart */
	221,	/* me_ystart */
	606,	/* me_xend */
	255,	/* me_yend */
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
static MENU MM132 = {
	ME_VER,
	221,	/* me_pos */
	NULL,	/* me_cap */
	'x',	/* me_rv */
	236,	/* me_xstart */
	185,	/* me_ystart */
	606,	/* me_xend */
	255,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM130,	/* me_topleft */
	&MM131,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM133 = {
	ME_FREC,
	0,	/* me_pos */
	"Raise On Break",
	'j',	/* me_rv */
	236,	/* me_xstart */
	255,	/* me_ystart */
	606,	/* me_xend */
	290,	/* me_yend */
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
static MENU MM134 = {
	ME_FREC,
	0,	/* me_pos */
	"Raise on Break\nLower On Run",
	'y',	/* me_rv */
	236,	/* me_xstart */
	290,	/* me_ystart */
	606,	/* me_xend */
	360,	/* me_yend */
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
static MENU MM135 = {
	ME_FREC,
	0,	/* me_pos */
	"Raise on Break\nIconify On Run",
	0x100+'b',	/* me_rv */
	236,	/* me_xstart */
	360,	/* me_ystart */
	606,	/* me_xend */
	430,	/* me_yend */
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
static MENU MM136 = {
	ME_VER,
	360,	/* me_pos */
	NULL,	/* me_cap */
	'y',	/* me_rv */
	236,	/* me_xstart */
	290,	/* me_ystart */
	606,	/* me_xend */
	430,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM134,	/* me_topleft */
	&MM135,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM137 = {
	ME_VER,
	290,	/* me_pos */
	NULL,	/* me_cap */
	'j',	/* me_rv */
	236,	/* me_xstart */
	255,	/* me_ystart */
	606,	/* me_xend */
	430,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM133,	/* me_topleft */
	&MM136,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM138 = {
	ME_VER,
	255,	/* me_pos */
	NULL,	/* me_cap */
	'x',	/* me_rv */
	236,	/* me_xstart */
	185,	/* me_ystart */
	606,	/* me_xend */
	430,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM132,	/* me_topleft */
	&MM137,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM139 = {
	ME_VER+ME_FREE,
	185,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	236,	/* me_xstart */
	10,	/* me_ystart */
	606,	/* me_xend */
	430,	/* me_yend */
	-100,	/* me_xcurs */
	-22,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM129,	/* me_topleft */
	&MM138,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM140 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_CREL,
	0,	/* me_pos */
	"Windows",
	'b',	/* me_rv */
	194,	/* me_xstart */
	-1,	/* me_ystart */
	336,	/* me_xend */
	32,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM139,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM141 = {
	ME_FREC,
	0,	/* me_pos */
	"cancel",
	'?',	/* me_rv */
	415,	/* me_xstart */
	19,	/* me_ystart */
	574,	/* me_xend */
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
static MENU MM142 = {
	ME_FREC,
	0,	/* me_pos */
	"confirm",
	'q',	/* me_rv */
	415,	/* me_xstart */
	49,	/* me_ystart */
	574,	/* me_xend */
	80,	/* me_yend */
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
static MENU MM143 = {
	ME_VER+ME_FREE,
	49,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	415,	/* me_xstart */
	19,	/* me_ystart */
	574,	/* me_xend */
	80,	/* me_yend */
	0,	/* me_xcurs */
	-13,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM141,	/* me_topleft */
	&MM142,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM144 = {
	ME_POPUP+ME_FREC+ME_ISUB+ME_CREL,
	0,	/* me_pos */
	"Quit",
	'a',	/* me_rv */
	336,	/* me_xstart */
	-1,	/* me_ystart */
	415,	/* me_xend */
	32,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM143,	/* me_topleft */
	NULL,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
static MENU MM145 = {
	ME_HOR,
	336,	/* me_pos */
	NULL,	/* me_cap */
	'b',	/* me_rv */
	194,	/* me_xstart */
	-1,	/* me_ystart */
	415,	/* me_xend */
	32,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM140,	/* me_topleft */
	&MM144,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
MENU cmd_men = {
	ME_HOR+ME_FREE,
	194,	/* me_pos */
	NULL,	/* me_cap */
	0,	/* me_rv */
	-1,	/* me_xstart */
	-1,	/* me_ystart */
	415,	/* me_xend */
	32,	/* me_yend */
	0,	/* me_xcurs */
	0,	/* me_ycurs */
	0,	/* me_colour */
	0,	/* me_fg_colour */
	0,	/* me_bg_colour */
	0,	/* me_save */
	&MM120,	/* me_topleft */
	&MM145,	/* me_botrite */
	NULL,	/* me_parent */
	NULL,	/* me_omen */
	NULL,	/* me_toggle */
	0,	/* me_select_val */
};
