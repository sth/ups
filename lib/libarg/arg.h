/* arg.h - public header file for the arg library */

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


/* @(#)arg.h	1.7 17 Apr 1994 (UKC) */

/* Define PROTO as in gendefs.h.  We don't mandate gendefs.h for
 * wn.h, so we have to define PROTO here.
 */
#ifndef PROTO
#ifdef __STDC__
#define PROTO(x) x
#else
#define PROTO(x) ()
#define const
#endif /* !__STDC__ */
#endif /* !PROTO */

/*  #defines for "set of char" implemented as a bit array, with each
 *  seven bits in a char.  We use only seven bits, these bitmaps are
 *  embedded in strings, which means we must always set the top
 *  bit to avoid terminating the string.
 *
 *  We handle 7 bit ascii only, so 128 characters at 8 bits per byte
 *  gives a 16 byte map.
 */
#define SEQMAPSIZE	((128 / 7) + 1)
#define SETSEQ(s,i)	((s)[(i) / 7] |= 1 << ((i) % 7))
#define TSTSEQ(s,i)	((s)[(i) / 7] & (1 << ((i) % 7)))

/*  Internal metacharacter codes for match().
 */
#define PAT_STAR	'\001'	/* '*' */
#define PAT_QUERY	'\002'	/* '?' */
#define PAT_SEQ		'\003'	/* SEQMAPSIZE byte sequence follows */
#define PAT_OPENBR	'\004'	/* '{' */
#define PAT_CLOSEBR	'\005'	/* '}' */
#define PAT_COMMA	'\006'	/* ',' (inside {} only) */
#define PAT_TWIDDLE	'\007'	/* '~' */

/*  Directory routines for glob.
 */
typedef struct dirfuncsst {
	int (*df_isdir) PROTO((const char *path));
	int (*df_opendir) PROTO((const char *path, long *p_dirid));
	const char *(*df_readdir) PROTO((long dirid));
	void (*df_closedir) PROTO((long dirid));
} dirfuncs_t;

/*  Redirection types from arg_get_redir().
 */
typedef enum redirtypeen {
	RD_NOT_REDIR,	/* argument is not a redirection */
	RD_CREATE,	/* create file for redirection(">") */
	RD_APPEND,	/* create if nonexistent, otherwize append (">>") */
	RD_READ,	/* open for reading - error if nonexistent ("<") */
	RD_NONE,	/* no file opening necessary (">&4") */
	RD_ERROR	/* badly formed redirection (e.g. "<&" and no digit) */
} redirtype_t;

typedef long dvec_t;

/*  Types of the dvec routines.
 */
void add_to_dvec PROTO((dvec_t dv, const char *s));
void free_dvec PROTO((dvec_t dv));
void free_dvec_and_strings PROTO((dvec_t dv));
dvec_t make_dvec PROTO((void));
const char **get_dvec_vec PROTO((dvec_t dv));
int get_dvec_size PROTO((dvec_t dv));

/*  Types of the globbing routines.
 */
char *arg_expand_twiddle PROTO((const char *path, int twiddle_char));
dvec_t arg_expand_braces PROTO((const char *s, int lch, int rch, int sep));
char *arg_do_quoting PROTO((const char **p_s, const char *glob_chars,
			    bool no_special_chars));
dvec_t arg_glob_cpat PROTO((const char *cpat, char *buf, int buflen));
dvec_t arg_gen_glob_cpat PROTO((const char *cpat, char *buf, int buflen, dirfuncs_t *dirfuncs));
int arg_has_globchars PROTO((const char *s));
int arg_match PROTO((const char *name, const char *pattern));
dvec_t arg_glob PROTO((const char **p_pat));
dvec_t arg_gen_glob PROTO((const char **p_pat, dirfuncs_t *dirfuncs));
redirtype_t arg_get_redir PROTO((const char **p_s, int **p_fdaddr,
				 long *p_lrdl));
int arg_convert_redir_to_shell_syntax PROTO((const char **p_s,
					     const char **p_redir_str,
					     const char **p_postarg_str));
int arg_open_redir_file PROTO((const char *name, redirtype_t redirtype));
void arg_tidy_redirs_in_parent PROTO((long lrdl));
void arg_do_redirs_in_child PROTO((long lrdl));
void arg_save_open_fds PROTO((void));
int arg_lparse PROTO((const char *s, dvec_t *p_dv, long *p_rdlist));
int arg_make_shell_line PROTO((const char *line, char **p_shell_line));
const char *arg_version PROTO((void));
