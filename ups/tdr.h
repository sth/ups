/* tdr.h - public header file for tdr.c */

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


/* @(#)tdr.h	1.9 24 May 1995 (UKC) */

void td_record_menu_command PROTO((const char *menu_name, int rv));
int td_record_to PROTO((const char *filename));

bool td_have_window PROTO((void));
bool td_replaying PROTO((void));
int td_init_from PROTO((void));
int td_restore_replay_fp PROTO((void));

int td_replay_from PROTO((const char *filename));
void td_set_no_window_flag PROTO((void));
void td_set_window_flag PROTO((void));
int td_set_replay_mode PROTO((const char *cmd));
const char *td_get_replay_mode_list PROTO((void));
void td_set_replay_idstr PROTO((const char *idstr));
int td_event_loop PROTO((bool *p_eof));
int td_load_loop PROTO((bool *p_eof));

bool td_set_select_recording PROTO((bool val));
bool td_set_obj_updating PROTO((bool val));

void td_check PROTO((const char *objpath));

#ifdef OBJ_H_INCLUDED
void td_record_field_edit PROTO((objid_t obj, int fnum, const char *value));
void td_record_bpt_code_edit PROTO((objid_t obj, const char *text));
void td_record_select PROTO((objid_t obj, bool on));
void td_record_object_removal PROTO((objid_t obj));
objid_t path_to_obj PROTO((const char *path));
#endif

void td_set_default_obj_to_selection PROTO((void));

#ifdef SYMTAB_H_INCLUDED
void td_record_func_and_lnum_cmd PROTO((func_t *f, fil_t *fil, int lnum,
					const char *op));
void td_record_show_var PROTO((fil_t *fil, int lnum, const char *name));
bool td_set_displayed_source PROTO((fil_t *fil, int lnum, const char *op));
int td_get_displayed_fil PROTO((fil_t **p_fil));
#endif

void td_record_refresh PROTO((void));
void td_record_debug_command PROTO((const char *cmd));
void rescan_init_file PROTO((void));
