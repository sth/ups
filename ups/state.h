/* state.h - public header file for state.c */

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


/* @(#)state.h	1.10 09 Apr 1995 (UKC) */

typedef enum {
	TM_START, TM_NEXT, TM_STEP, TM_CONT, TM_STOP, TM_ATTACH, TM_DETACH,
	TM_KILL, TM_NTAGS
} target_menu_index_t;

typedef struct {
	struct {
		int md;
		int wn;
		struct Region *region;
	} tm_mdtab[TM_NTAGS];
	int tm_current_md;
} target_menu_info_t;

void set_target_menu_info PROTO((target_menu_info_t *tm));
target_menu_info_t *get_target_menu_info PROTO((void));

void set_start_target_menu PROTO((int md));
int get_start_target_menu PROTO((void));

void set_restart_target_menu PROTO((int md));
int get_restart_target_menu PROTO((void));

void set_message_wn PROTO((int wn));
int get_message_wn PROTO((void));

#ifdef TARGET_H_INCLUDED
target_t *get_current_target PROTO((void));
void set_current_target PROTO((target_t *xp));
#endif

#ifdef SRCWIN_H_INCLUDED
Srcwin *get_current_srcwin PROTO((void));

void set_current_outwin PROTO((Outwin *ow));
Outwin *get_current_outwin PROTO((void));

void  set_display_area_overlay PROTO((Outwin *dw));
Outwin *get_display_area_overlay PROTO((void));
#endif

void set_current_srcwin_menu PROTO((int md));
int get_current_srcwin_menu PROTO((void));

#ifdef REG_H_INCLUDED
void set_current_srcwin_region PROTO((Region *region));
Region *get_current_srcwin_region PROTO((void));

void set_dynamic_menu_region PROTO((Region *region));
Region *get_dynamic_menu_region PROTO((void));

void set_display_area_region PROTO((Region *region));
Region *get_display_area_region PROTO((void));

void set_typing_line_region PROTO((Region *region));
Region *get_typing_line_region PROTO((void));
void set_extended_typing_line_region PROTO((Region *region));
Region *get_extended_typing_line_region PROTO((void));
#endif

#ifdef CC_H_INCLUDED
void set_ccstate PROTO((ccstate_t *cs));
ccstate_t *get_ccstate PROTO((void));
#endif

int demangling_enabled PROTO((int set, int reset));

