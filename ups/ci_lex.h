/* ci_lex.h - header file for ci_lex.c */

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


/* @(#)ci_lex.h	1.3 26/4/92 (UKC) */

/*  Environment for yylex.
 */
typedef struct lex_envst {
	const char *le_lptr;
	const char *le_filename;
	int le_lnum;
	bool le_had_error;
	const char *le_line;
	const char *(*le_getline)PROTO((char *arg));
	ci_report_error_func_t le_report_func;
	char *le_getline_arg;
	bool le_abort_parse;
} lex_env_t;

typedef int token_t;

extern lex_env_t *Lex_env;

int yylex PROTO((void));
void yyerror PROTO((const char *s));
void ci_lex_error PROTO((char *arg, errtype_t errtype,
					lexinfo_t *lx, const char *mesg));

/*  is_typedef() is actually defined in ci_parse.y, but it is only used
 *  from ci_lex.c, so we put it here for minumum visibility.
 */
token_t name_type PROTO((const char *name));
