/* props.h - header file for props.c - private to the edit library */

/*  Copyright 1994 Mark Russell, University of Kent at Canterbury.
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


/* @(#)props.h	1.2 31 Aug 1994 (UKC) */

struct Edit_fontinfo {
	char *font_handle;
	short *char_width_tab;
	int height;
	int baseline;
};

struct Edit_propchange {
	size_t point;
	char *user_data;
	bool new_font;
	Edit_fontinfo *fi;
	Edit_flags flags;
	Edit_flags flagmask;
	bool backsliding;
	Edit_propchange *next;
};

void edit__update_proplist_points PROTO((Proplist *pl, size_t pos, long delta));
