/* srcpath.h - header file for srcpath.c */

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


/* @(#)srcpath.h	1.2 20 Jun 1995 (UKC) */

typedef struct pathlist_s {
	const char *path;
	struct pathlist_s *next;
} Pathentry;

void srcpath_add_path PROTO((const char *path));

void srcpath_ignore_path_hints PROTO((void));

bool srcpath_resolve_path PROTO((const char *what, const char *path_hint,
				 const char *name, const char **p_fullpath));

struct Srcbuf *srcpath_visit_file PROTO((alloc_pool_t *ap,
					 const char *path_hint,
					 const char *name));

bool srcpath_file_exists PROTO((const char *path_hint, const char *name));

Pathentry *get_source_path_root PROTO((void));

Pathentry *get_source_path_tail PROTO((void));

void srcpath_check_and_add_path PROTO((const char *ro_paths));

void srcpath_free_pathlist PROTO((void));
