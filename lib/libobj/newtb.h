/* newtb.h - type declarations for the thumb bar routines in newtb.c */

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


/* @(#)newtb.h	1.5 09 Sep 1994 (UKC) */

#define TB_H_INCLUDED

typedef struct { int dummy_member; } *tbar_id_t;

typedef enum { TB_GOTO, TB_SCROLL, TB_SHOW_POS } tb_action_t;

typedef int (*tb_callback_t)PROTO((char *arg, tb_action_t action, int val));

void tb_set_callback_and_data PROTO((tbar_id_t tbar_id,
					tb_callback_t callback, char *data));
void tb_destroy PROTO((tbar_id_t tbar_id));
tbar_id_t tb_create PROTO((int wn));
void tb_set_box_pos_and_size PROTO((tbar_id_t tbar_id, bool box_is_vertical,
				    int box_fstart, int box_fsize));
void tb_set_marks_pos_and_size PROTO((tbar_id_t tbar_id,
						int box_fstart, int box_fsize));
void tb_expose PROTO((tbar_id_t tbar_id));
void tb_set_visible_units PROTO((tbar_id_t tbar_id, int visible_units));
int tb_get_visible_units PROTO((tbar_id_t tbar_id));
void tb_set_total_units PROTO((tbar_id_t tbar_id, int total_units));
int tb_get_total_units PROTO((tbar_id_t tbar_id));
void tb_clear_marks PROTO((tbar_id_t tbar_id));
void tb_refresh PROTO((tbar_id_t tbar_id));
void tb_mark PROTO((tbar_id_t tbar_id, int pos, int size, int color, int update));
void tb_goto PROTO((tbar_id_t tbar_id, int pos, bool call_callback));
void tb_scroll PROTO((tbar_id_t tbar_id, int n_units, bool call_callback));
void tb_show_pos PROTO((tbar_id_t tbar_id, int unit));
int tb_tbpos_to_unit PROTO((tbar_id_t tbar_id, int wn, int coord, bool centre));
