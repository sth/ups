/* menu_priv.h - private header file for the menu library */

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


/* @(#)menu_priv.h	1.6 17 Apr 1994 (UKC) */

/*  define these to make the source look more like jdb's version
 */
typedef int WOTYPE, WITYPE;
typedef struct fontst FONT;

#define ISCLEAR(m)	(((m)->me_cap == NULL) || \
			(((m)->me_flags & (ME_OPEN|ME_ISUB)) == ME_OPEN))

/*  Definitions of structures used to replace the Perq ClipCtl structure
 *  in the menu library.  Will someday get rid of these.
 */
typedef struct posst {
	short x;
	short y;
} Pos_t;

typedef struct sizest {
	short w;
	short h;
} Size_t;

typedef struct boxst {
	Pos_t pos;
	Size_t size;
} Box_t;

/*  structure defining an opened menu
 */
struct omenst {
	MENU *om_root;		/* root node of the menu tree */
	MENU *om_last;		/* last node displayed (NULL if not displayed) */
	WOTYPE om_wout;		/* file descriptor of display window */
	short om_md;		/* menu descriptor of the menu */
	Box_t om_area;		/* location of saved screen area */ 
	long om_rect;		/* pointer to saved screen area */
	short om_fback[3];	/* feedback colours used */
	FONT *om_font[4];	/* fonts for captions */
	int om_wantbox;		/* do we want a box round the menu? */
};

#define NULLRECT	0

extern int menerr;
extern struct omenst _menu_[];

extern window_t _Last_wid;
extern int _Last_wn;

int wid_to_wn MPROTO((WOTYPE wid));
int wblank MPROTO((WOTYPE wout, Box_t *clip, long colour));
int mshift MPROTO((MENU * menu, int x, int y, int root));
void momen MPROTO((MENU *menu, struct omenst *omen));
int mshow MPROTO((MENU *menu, int drawall));
int mposition_popup MPROTO((MENU* menu, int wn, int* pcx, int* pcy, int may_warp_cursor));
void mdelete MPROTO((MENU* menu));
