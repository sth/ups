/* obj_target.h - public header file for obj_target.c */

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


/* @(#)obj_target.h	1.11 09 Apr 1995 (UKC) */

#define OBJ_TARGET_H_INCLUDED

extern struct Edit_history* cmd_history;
int setup_shellcmd PROTO((bool want_execfile, char **p_shell_line));
int setup_args PROTO((const char ***p_argv, long *p_rdlist));

#ifdef OBJ_H_INCLUDED
void add_target_object PROTO((objid_t par, const char *textpath,
			      const char *minus_a_cmdline,
			      bool use_full_path));
char *target_format_obj PROTO((objid_t code));
void do_target PROTO((objid_t par, int command, char *arg));
void free_com PROTO((objid_t obj));
#endif

extern const char Com_format[];

#ifdef OBJTYPES_H_INCLUDED
extern fdef_t Com_fdefs[];
extern fnamemap_t Com_fnamemap[];
#endif
