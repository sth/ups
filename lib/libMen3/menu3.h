/* menu3.h - menu library public header file */

/*  Copyright 1991 John Bovey, University of Kent at Canterbury.
 *  All Rights Reserved.
 *
 *  This file is part of MED3.
 *
 *  MED3 is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  MED3 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with UPS; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* @(#)menu3.h	1.11 4/5/92 (UKC) */

#ifdef __STDC__
#define MPROTO(x) x
#else
#define MPROTO(x) ()
#define const
#endif /* !__STDC__ */

/*  Structures and constants used in a menu tree structure
 */
typedef struct mnode {
	unsigned short me_flags;
	short me_pos;	/*  The position at which the field is divided */
	const char *me_cap;	/*  Caption to be displayed in the field */
	short me_rv; 		/* result to be returned */
	short me_xstart;	/* The window area of this node */
	short me_ystart;
	short me_xend;
	short me_yend;
	short me_xcurs;		/* cursor coordinates for cursor */
	short me_ycurs;		/* relative popup menus */
	short me_colour;
  	long me_fg_colour;
  	long me_bg_colour;
	long me_save;		/* For saving popup backgrounds */
	struct mnode * me_topleft; /* pointer to top or left sub-menu */
	struct mnode * me_botrite; /* pointer to b. or r. sub_menu */
	struct mnode * me_parent;  /* pointer to parent */
	struct omenst *me_omen;	   /* pointer to open menu structure */
	int*   me_toggle;       /* For toggle items */
	int    me_select_val;   /* if >= 0, select value for exclusive group */
} MENU;

/* flags for me_flags
 */
#define ME_FONT		0x3	/* mask for font */
#define ME_VER		0x4	/* node is divided vertically */
#define ME_HOR		0x8	/* node divided horizontally */
#define ME_DIV		0xc	/* node is divided */
#define ME_NSEL		0x10	/* This node can't be selected */
#define ME_POPUP	0x20	/* use in pop-up mode */
#define ME_FREC		0x40	/* me_cap can be free()'d */
#define ME_FREN		0x80	/* the node itself can be free()'d */
#define ME_OPEN		0x100	/* the node is open */
#define ME_ISUB		0x200	/* has an indivisible submenu */
#define ME_FREE		0x400	/* free standing node */
#define ME_BGLINE	0x800	/* draw lines in background colour */
#define ME_NOSCALE	0x1000	/* don't scale submenu */
#define ME_CREL		0x2000	/* cursor relative submenu */
#define ME_REDRAW	0x4000  /* flag to force menu redrawing */
#define ME_LEFT		0x8000	/* left justify caption rather than centre it */

#ifndef NULL
#define NULL	0
#endif

#define MAXMEN 40	/* the maximum number of open menus */

/*  menerr error values
 */
#define MENOK	0
#define MTABFL	1	/* already MAXMEN open menus */
#define MNOTMEN	2	/* menu descriptor is not an open menu */
#define MBADMD	3	/* menu descriptor out of range */
#define MBADFIL 4	/* can't read menu from menu file */
#define MNOFIL	5	/* can't open menu file */
#define MBADWD	6	/* bad window file descriptor */
#define MNOTDIS 7	/* the menu is not currently displayed */
#define MBADARG 8	/* bad function arguments */
#define MAERROR 9	/* message area error */

/*  structure returned by Mopenerr() to report diagnostics on errors in
 *  reading C menu files.
 */
#define MAXMERR	128
struct merrst {
	char mr_message[MAXMERR];
	int  mr_line;		/* line number */
};

/*  modes for Mselect.
 */
#define MS_PRESSED	0
#define MS_RELEASED	1
#define MS_CONTINUE	2

/*  style flags for Mmake.
 */
#define MM_DRAG		1	/* select on drag */
#define MM_NOLINE	2	/* don't draw the background lines */
#define MM_LEFT		4	/* Left justify rather than centre */

/*  Macros for extracting Mselect return values
 */
#define MS_md(x) (((x) >> 16) & 0177)
#define MS_rv(x) ((x) & 077777)

/*  Different ways of highlighting submenus
 */
#define MH_WHITE 0	/* no highlighting */
#define MH_GREY  1	/* shaded light grey */
#define MH_BLACK 2	/* inverted */
#define MH_BOLD  4	/* text emboldened, single box only */

/*  Macros used to construct return values from ASCII part and layer
 */
#define Mrv_ascii(rv)	((rv) & 0177)
#define Mrv_layer(rv)	(((rv) >> 8) & 3)
#define Mrvalue(l,c)	(((l) << 8) | (c))

/*  Offset styles used by Mlink.
 */
#define ME_MLBUTR	1
#define ME_MLBUTB	2
#define ME_MLPOPR	4
#define ME_MLPOPB	8
#define ME_MLCREL	16

/*  Function prototypes.
 */

int Msopen MPROTO((int wn, int x, int y, int width, int height));
int Msclose MPROTO((int mad));
int Mputs MPROTO((int mad, const char *str));
int Mgets MPROTO((int mad, char *str));
int Mputc MPROTO((int mad, int c));
int Merase MPROTO((int mad, int n));

int Mclose MPROTO((int md));
int Mmake MPROTO((const char *name, const char **captions, const int *rvalues,
		  const int *fonts, int style, int width));
void Mcheck MPROTO((int md));
int Mpushsu MPROTO((int wn));
int Mpopsu MPROTO((int wn));
int Mclear MPROTO((int md));
int Mrepair MPROTO((int wn));
int Mredraw MPROTO((int wn));
void Mmredraw MPROTO((int md));
#ifdef WN_STDWIN
int Mfonts MPROTO((int md, font_t *font0, font_t *font1,
			  font_t *font2, font_t *font3));
#endif
int Mnobox MPROTO((int md));
int Mnonsel MPROTO((int md, const char *rvs, int display));
int Mdelete_entry MPROTO((int md,int rv));
int Mmaketoggle MPROTO((int md, int rv, int* p_value));
int Mmakegroup MPROTO((int md, int* p_value, ...));
int Mreset MPROTO((int md));
int Mcansel MPROTO((int x, int y, int wout));
int Mselect MPROTO((int x, int y, int mode, int wout, int ignore));
int Mdisplay MPROTO((int md, int wout, int save));
int Mremove MPROTO((int md));
int Mdup MPROTO((int md));
int Mfmodes MPROTO((int md, int reveal, int toact, int acting));
int Mleft MPROTO((int md));
int Mright MPROTO((int md));
int Mtop MPROTO((int md));
int Mbottom MPROTO((int md));
int Mwidth MPROTO((int md));
int Mheight MPROTO((int md));
int Minsert MPROTO((MENU *menu));
void Mget_menu_colors MPROTO((const char* name, long *p_fg, long *p_bg,
			     int *fg_specified, int *bg_specified));
MENU* Mpropagate_colors MPROTO((MENU *main, long fg, long bg));
int Mlink MPROTO((int mainmd, int submd, int rv, int xoff, int yoff, int mlstyle));
void Mdump MPROTO((int md));
int Mchange MPROTO((int md, int oldrv, int newrv, const char *str, int fontno));
int Mopen MPROTO((const char *name));
struct merrst *Mopenerr MPROTO((void));
int Mperror MPROTO((const char *str));
int Msize MPROTO((int md, int width, int height));
int Mplace MPROTO((int md, int x, int y));
int Mposition_popup MPROTO((int md, int wn, int* pcx, int* pcy, int may_warp_cursor));
struct fontst *Mstdfont MPROTO((void));
void Msetstdfont MPROTO((struct fontst *font));
const char *Mversion MPROTO((void));

/*  No parameter names for fpush and fpop as they give g++ heartburn.
 *  I think this is a bug in g++.
 */
void Msetpushpop MPROTO((int (*)(int wn), int (*)(int wn)));
void Mactivate_submenu MPROTO((MENU *menu));
void Mdeactivate_submenu MPROTO((MENU *menu));
int Mselectable MPROTO((MENU *menu));
int Mgetmd MPROTO((int x, int y, int wn));
