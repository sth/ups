/* utils.h - header file for utils.c */

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


/* @(#)utils.h	1.16 04 Jun 1995 (UKC) */

typedef void (*message_output_func_t)PROTO((const char *mesg));

message_output_func_t set_message_output_func PROTO((message_output_func_t func));
void message PROTO((const char *fmt, ...));
void error PROTO((const char *fmt, ...));

int query_user PROTO((const char *prompt, const char *yes, const char *no,
		      bool *p_isyes));

void failmesg PROTO((const char *mesg, const char *what, const char *path));
const char *get_errno_str PROTO((void));

const char *mtrprog_version PROTO((void));
int is_number PROTO((const char *s));
int string_to_bufsize PROTO((const char *s, size_t *p_res));
int string_to_offset PROTO((const char *s, long *p_res));
void swap_longs PROTO((long *base, size_t count));

char *onpath PROTO((const char *dirs, const char *seps, const char *name));

const char *filemode_to_string PROTO((int mode));
const char *filetype_to_string PROTO((int mode));
int e_close PROTO((int fd, const char *filename));
int remove_directory_tree PROTO((const char *name));
int apply_to_files_in PROTO((const char *name, int (*func)(const char *dirname,
				     const char *filename, char *farg), char *arg));

#ifdef EOF
bool fopen_new_file PROTO((const char *what, const char *name, bool overwrite,
			   FILE **p_fp));
bool fclose_new_file PROTO((const char *what, const char *name, bool ok,
			    FILE *fp));
#endif

/*  This macro will is intended for use on constant expressions.
 *  It will work for variables, but it evaluates its argument four
 *  times, and is inefficient.
 */
#define SWAP_LONG(n)	(((n) << 24) | (((n) << 8) & 0xff0000) | \
					((n) >> 8 & 0xff00) | (((n) >> 24) & 0xff)) 

/*  Return TRUE if mode (as returned by stat(2)) represents a directory.
 */
#define isdir(mode)	(((mode) & S_IFMT) == S_IFDIR)
