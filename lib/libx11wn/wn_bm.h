/* wn_bm.h - header file for wn_bm.c */

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


/* @(#)wn_bm.h	1.2 4/7/91 (UKC) */

void _wn_define_machine_bitmap_format PROTO((void));
struct pixrect *_wn_bm_to_pixrect PROTO((bitmap_t *bm));
void _wn_set_machine_format PROTO((bitmap_t *bm, format_t *fm, int flags));
void _wn_restore_format PROTO((bitmap_t *bm, format_t *fm));

#ifdef EOF
void wn_dump_bitmap PROTO((FILE *fp, bitmap_t *bm, int x, int y, int width, int height));
void _wn_dump_bits PROTO((FILE *fp, bitmap_t *bm, int orig_x, int orig_y, int width, int height, int margin));
void _wn_dump2_bits PROTO((FILE *fp, bitmap_t *bm1, int x1, int y1, bitmap_t *bm2, int x2, int y2, int width, int height, int margin));
#endif
