/* obj_buildf.h - header file for obj_buildf.c */

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


/* @(#)obj_buildf.h	1.9 12/22/93 (UKC) */

void do_formats PROTO((bool have_window));
void close_target_display PROTO((void));
void update_variable_values PROTO((void));
void restore_expanded_file_entries PROTO((void));

#ifdef TARGET_H_INCLUDED
void initialise_display_area PROTO((target_t *xp, const char *args, bool use_full_path));
objid_t rebuild_display PROTO((target_t *xp));
#endif
