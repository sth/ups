/* o_lptrs.h - header file for o_lptrs.c */

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


/* @(#)o_lptrs.h	1.2 4/7/91 (UKC) */

int get_info_depth PROTO((void));
void do_lptrs PROTO((int no_skip));
void set_update_lptrs PROTO((struct objst *obj, int recurse));
int get_indent PROTO((struct objst *obj));
int get_indent_from_objtype PROTO((struct objst *obj, int type));
struct lptrst *new_lptr PROTO((void));
void free_lptr_list PROTO((struct lptrst *lptr));
