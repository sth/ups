/* wn_xrop.h - header file for wn_xrop.c */

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


/* @(#)wn_xrop.h	1.2 4/7/91 (UKC) */

void _wn_z_rasterop PROTO((bitmap_t *sbm, int src_x, int src_y, int width, int height, bitmap_t *dbm, int dst_x, int dst_y, int ropfunc));
void _wn_xy_rasterop PROTO((bitmap_t *sbm, int src_x, int src_y, int width, int height, bitmap_t *dbm, int dst_x, int dst_y, int ropfunc));
void _wn_send_bm PROTO((bitmap_t *bm, int sx, int sy, int width, int height, Window win, GC gc, int dx, int dy, int ropfunc, xpixel_t fg, xpixel_t bg));
