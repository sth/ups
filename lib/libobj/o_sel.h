/* o_sel.h - header file for o_sel.c */

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


/* @(#)o_sel.h	1.4 29/6/92 (UKC) */

void sel_obj_tree PROTO((struct objst *par, int sel_self, int sel_children, int sel_descendents, int new_state));
void no_edit PROTO((struct drawst fdets));
fval_t iget_field_value PROTO((struct objst *obj, int fnum));
void obj_pos PROTO((struct objst *stop_obj, int *p_x, int *p_y));
void do_pending_selection PROTO((void));
objid_t get_object_at PROTO((int puck_x, int puck_y));
struct fvalst *get_p_fval PROTO((struct objst *obj, int fnum));
void deselect PROTO((struct objst *obj));
struct objst *select_list obj__PROTO((int x_threshold, int *threshold_exceeded));
