/* obj_env.h - header file for obj_env.c */

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


/* @(#)obj_env.h	1.7 09 Apr 1995 (UKC) */

extern struct Edit_history* env_history;
const char **get_environment PROTO((void));

#ifdef OBJ_H_INCLUDED
const char *env_getobjname PROTO((objid_t obj));
void add_env_header PROTO((objid_t par));
char *env_format_obj PROTO((objid_t code));
void do_envhead PROTO((objid_t obj, int command, char *arg));
void do_env PROTO((objid_t obj, int command, char *arg));
void free_env PROTO((objid_t obj));
void env_getsize PROTO((objid_t obj, objid_t unused_par, sz_t *sz));
void env_getcolor PROTO((objid_t obj, int wn, long *p_fg, long *p_bg));

extern const char Envhead_format[];
extern const char Env_format[];

#ifdef OBJTYPES_H_INCLUDED
extern fdef_t Env_fdefs[];
extern fnamemap_t Env_fnamemap[];
#endif

#endif
