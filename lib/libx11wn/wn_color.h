/* wn_color.h - header file for wn_color.c */

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


/* @(#)wn_color.h	1.3 4/7/91 (UKC) */

void _wn_set_fgbg PROTO((int is_fg, char *sred, char *sgreen, char *sblue));
void _wn_init_sunview_colors PROTO((swin_t *w, int is_mainwin));

extern int wn__Use_mono;
extern int wn__Use_alloc_color;
extern int wn__Use_alloc_color_for_highlight;
extern int wn__Use_menu_colors;
extern int wn__Use_extended_text_colors;
