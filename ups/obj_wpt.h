/* obj_wpt.h - public header file for obj_wpt.c */

/*  Copyright 2002 Tom Hughes <thh@cyberscience.com>
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


#ifdef OBJ_H_INCLUDED
int wpt_accelerator_action PROTO((objid_t obj));
objid_t add_watchpoint_object PROTO((taddr_t address, size_t size, const char *text));
void remove_watchpoint_object PROTO((objid_t obj));
char *wpt_format_obj PROTO((objid_t code));
void do_wpt PROTO((objid_t obj, int command, char *arg));
void add_watchpoint_header PROTO((objid_t par));
int get_watchpoint_refresh_interval PROTO((void));
void do_wps PROTO((objid_t obj, int command, char *arg));
int global_wp_enabled PROTO((int set, int reset));
void wpt_getcolor PROTO((objid_t obj, int wn, long *p_fg, long *p_bg));
#endif

#ifdef OBJTYPES_H_INCLUDED
extern const char Wphead_format[];
extern const char Wpt_format[];

extern fdef_t Wpt_fdefs[];
extern fnamemap_t Wpt_fnamemap[];

extern fdef_t Wphead_fdefs[];
extern fnamemap_t Wphead_fnamemap[];
#endif
