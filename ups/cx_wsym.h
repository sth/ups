/* cx_wsym.h - header file for cx_wsym.c */

/*  Copyright 1993 Mark Russell, University of Kent at Canterbury.
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


/* @(#)cx_wsym.h	1.1 22/12/93 (UKC) */

typedef struct syminfo_s syminfo_t;

syminfo_t *make_syminfo PROTO((alloc_pool_t *ap, FILE *fp, ebuf_t *eb));
int write_block PROTO((syminfo_t *si, block_t *bl));
int write_types PROTO((syminfo_t *si, size_t *p_ntypes));
