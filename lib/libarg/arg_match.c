/* arg_match.c - match a single pathname component against a pattern */

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


/* @(#)arg_match.c	1.9 26/4/92 (UKC) */
char arg_match_rcsid[] = "$Id$";


#include <local/ukcprog.h>

#include "arg.h"

static int rmatch PROTO((const char **p_name, const char **p_pattern));

/*  Recursive regular expression match algorithm.
 *
 *  Recognises the metacharacters '*', '[' and ']' and '?' which have
 *  the same meanings as in csh(1).
 *
 *  It expects the metacharacters to have been translated to PAT_STAR,
 *  PAT_SEQ etc - see arg_quote().
 *
 *  We return TRUE for a match, FALSE for no match.
 *  If (and only if) we get a match, we set *p_name and *p_pattern to
 *  the new values of name and pattern (i.e. move them over the part
 *  we have just matched).
 *
 *  This is called by arg_match() only.
 */
static int
rmatch(p_name, p_pattern)
const char **p_name, **p_pattern;
{
	const char *cptr, *name, *pattern;
	
	name = *p_name;
	pattern = *p_pattern;
	while (*pattern != '/' && *pattern != '\0') {
		switch(*pattern) {
		case PAT_SEQ:
			if (*name == '/' || *name == '\0')
				return FALSE;
			if (!TSTSEQ(pattern + 1, *name))
				return FALSE;
			pattern += SEQMAPSIZE + 1;
			name++;
			break;
		case PAT_QUERY:
			if (*name == '\0' || *name == '/')
				return FALSE;
			pattern++;
			name++;
			break;
		case PAT_STAR:
			pattern++;
			for (cptr = name; *cptr != '\0' && *cptr != '/'; cptr++)
				;
			while (!rmatch(&cptr, &pattern))
				if (--cptr < name)
					return FALSE;
			name = cptr;
			break;
		default:
			if (*name++ != *pattern++)
				return FALSE;
			break;
		}
	}
	if (*name != '/' && *name != '\0')
		return FALSE;
	*p_name = name;
	*p_pattern = pattern;
	return TRUE;
}

/*  Return TRUE if name matches pattern. Pattern is a csh(1) filename
 *  pattern, that has been passed through arg_quote() above.
 *  We deal with files starting with '.' (must be matched explicitly)
 *  and return TRUE immediately of the pattern is "*", otherwise
 *  call rmatch().
 */
int
arg_match(name, pattern)
const char *name, *pattern;
{
	if (*name == '.') {
		if (*pattern == '.') {
			name++;
			pattern++;
		}
		else
			return FALSE;
	}
	if (*pattern == PAT_STAR && (pattern[1] == '/' || pattern[1] == '\0'))
		return TRUE;
	return rmatch(&name, &pattern);
}
