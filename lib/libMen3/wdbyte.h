/* wdbyte.h - header file for the Perq wdbyte emulation */

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


/* @(#)wdbyte.h	1.4 26/4/92 (UKC) */

#ifndef __STDC__
#define const
#endif

struct DBCtl                   /*  WDBYTE control block */
{
        short   DBX            ;  /* destination X co-ord	     	*/
        short   DBByteOffset   ;  /* start char offset from String   	*/
        short   DBFunc         ;  /* rasterop function             	*/
        short   DBY            ;  /* destination Y co-ord of baseline 	*/
        short   DBMaxX         ;  /* max X co-ord  for whole chars 	*/
        short   DBMaxByte      ;  /* max char offset from String	*/
        const char   *DBSrcString    ;  /* source string                 	*/
        int    *DBScreen       ;  /* Base of dest area (0 = window)	*/
        /* struct font */font_t *DBFont ;  /* address of font              */
	short	DBDstInc       ;  /* scan line increment		*/
};
	/* NB both MaxX and MaxByte are defined as 1 greater than the 	*/
	/*    last allowed value, ie as lengths if starting from zero.	*/

#define HIT_MAXBYTE 0
#define HIT_MAX_X 1
#define HIT_CTL_CHAR 2

int wn_wdbyte MPROTO((int fd, struct DBCtl *dbargp, Box_t *clipp,
		      long fg_color, long bg_color));
