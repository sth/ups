/* cursors.h - defines for cursor numbers (machine generated) */

/*  Copyright 1991 Mark Russell, University of Kent at Canterbury.
 *  All Rights Reserved.
 *
 *  This file is part of UPS.
 *
 *  UPS is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  UPS is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with UPS; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */


/* @(#)cursors.h	1.5 4/7/91 (UKC) */

/*  These #defines index an array - don't change them without changing
 *  the coressponding stuff in cursors.c.
 */
#define CU_DEAD		0
#define CU_DISPLAY	1
#define CU_DRAG_BOX	2
#define CU_MENU		3
#define CU_SBAR		4
#define CU_SC_PRESSED	5
#define CU_SCROLL_DOWN	6
#define CU_SCROLL_UP	7
#define CU_WAIT		8
#define CU_LINE		9
#define CU_DRAGBOX	10

void set_bm_cursor PROTO((int wn, int cno));
