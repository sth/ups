/* o_mkobj.h - header file for o_mkobj.c */

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


/* @(#)o_mkobj.h	1.4 4/7/91 (UKC) */

void enter_obj PROTO((struct objst *obj, objid_t code));
void set_likely PROTO((struct objst *obj));
struct objst *code_to_obj PROTO((objid_t code));
void link_object PROTO((struct objst *obj, struct objst *par, struct objst *next, struct objst *prev));
void unlink_object PROTO((struct objst *obj));
void make_obj PROTO((struct objst *par, struct objst *next, struct objst *prev, objid_t code, int type));
void rm_obj_tree PROTO((struct objst *obj, int kill_children, int kill_parent, int kill_descendents));
void obj_hash_stats PROTO((void));
struct fvalst *new_fval PROTO((void));
struct fvalst *new_fval_list PROTO((int nfields));
void free_fval_list PROTO((struct fvalst *fval));
