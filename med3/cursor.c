/*  Copyright 1993 John Bovey, University of Kent at Canterbury.
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

#include <stdio.h>
#include <local/wn.h>
#include "cursors.h"
#ifndef lint
static char sccsid[] = "%W% (UKC) %G%";
#endif  /* lint */

extern int wn;				/* window number */
bitmap_t *cursor[CP_NUM];
extern short *cpats[];
extern short cp_xhot[];
extern short cp_yhot[];

/*  Initialise the cursors. Should be called just once.
 */
void
initcps()
{
	int i;

	for (i = 0; i < CP_NUM; i++) {
		cursor[i] = wn_make_bitmap_from_data
		  (16,16,1, (unsigned short *)cpats[i],BM_BIT0_RIGHT,
		   BM_XY_PIXELS,2);
		cursor[i]->bm_xhot = cp_xhot[i];
		cursor[i]->bm_yhot = cp_yhot[i];
	}
}

/* set cursor number cnum
 */
int
setcp(int cnum)
{
	static int lastcnum = -1;
	int temp;

	if (cnum == lastcnum)
		return 0;
	(void)wn_set_cursor(wn,cursor[cnum]);
	temp = lastcnum;
	lastcnum = cnum;
	return(temp);
}
