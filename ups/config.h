/* config.h - public header file for config.c */

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


/* @(#)config.h	1.2 09 Sep 1994 (UKC) */

int save_state PROTO((const char *state_dir, const char *state_path,
		      bool want_save_sigs));

void make_config_paths PROTO((const char *state_dir, const char *basepath,
			      const char **p_state_path,
			      const char **p_user_path));

void load_config PROTO((const char *state_path, const char *user_path,
			bool *p_want_auto_start));

int read_config_file PROTO((const char *path, bool from_statefile,
			    bool must_exist, bool breakpoints_only,
			    bool ignore_breakpoints, bool *p_want_auto_start,
			    bool no_statefile_errors));

void load_state_file PROTO((void));

void save_state_file PROTO((void));
