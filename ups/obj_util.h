/* obj_util.h - public header file for obj_util.c */

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


/* @(#)obj_util.h	1.4 09 Apr 1995 (UKC) */

typedef struct Field_edit_info Field_edit_info;

typedef void (*Field_edit_move_point_func)PROTO((Edit_display *display,
						 objid_t obj, size_t point));

void edit_field_obj PROTO((objid_t obj, int fnum));

Edit_display *field_edit_start PROTO((struct drawst *dr,
				      const char *what,
				      char *textcopy));

Edit_display* field_edit_get_display PROTO(( char* handle));

void field_edit_set_move_point_func PROTO((Field_edit_info *fe,
					   Field_edit_move_point_func func));

bool field_edit_finish PROTO((char *handle, bool force));

objid_t field_edit_get_object PROTO((Field_edit_info *fe));

void field_edit_update_orig_text PROTO((Field_edit_info *fe, const char *text));

#ifdef REG_H_INCLUDED
void field_edit_handle_key_event PROTO((Region *region,
					char *handle, event_t *ev));
#endif

void field_edit_redraw_display PROTO((char *handle));
objid_t find_or_add_object PROTO((objid_t par, objid_t wanted, 
                                  void (*add_object)PROTO((objid_t wobj))));
void field_edit_handle_insert_text PROTO((char *handle, char *text));
void crush_whitespace PROTO((char *text));
bool do_replacements PROTO((const char *oldstr, ebuf_t *eb));
