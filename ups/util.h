/* util.h - public header file for util.c */

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


/* @(#)util.h	1.4 04 Jun 1995 (UKC) */

bool parse_line PROTO((const char *line, char ***p_words, int *p_nwords));

#ifdef EOF
void put_quoted_string PROTO((FILE *fp, const char *str));
void dump_text PROTO((const char *text, FILE *fp));
FILE *fopen_with_twiddle PROTO((const char *path, const char *mode));
#endif

int open_with_twiddle PROTO((const char *path, int mode));
bool same_string PROTO((const char *s1, const char *s2));

bool get_debug_output_path PROTO((const char *cmdline, const char *defpath,
				  const char **p_cmdline, const char **p_path,
				  bool *p_overwrite));
