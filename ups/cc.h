/* cc.h - header file for cc.c */

/*  Copyright 1993 Mark Russell, University of Kent at Canterbury.
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


/* @(#)cc.h	1.1 22/12/93 (UKC) */

#define CC_H_INCLUDED

typedef struct ccstate_s ccstate_t;

ccstate_t *cc_create_ccstate PROTO((const char *extra_flags,
				    unsigned compile_flags));
void cc_free_ccstate PROTO((ccstate_t *cs));
const char *cc_checkarg PROTO((ccstate_t *cs, char **argv));
bool cc_handle_arg PROTO((ccstate_t *cs, char ***p_argv));
linkinfo_t *cc_parse_and_compile PROTO((ccstate_t *cs, const char *srcpath,
					parse_id_t *p_parse_id,
					ci_checkarg_proc_t checkarg_proc));
void cc_get_libinfo PROTO((size_t *p_nlibfuncs, size_t *p_nlibvars));
bool cc_report_error PROTO((lexinfo_t *lx, const char *mesg));
ci_nametype_t cc_getaddr PROTO((const char *name, taddr_t *p_addr));

const char *cc_get_usage PROTO((ccstate_t *cs));
bool cc_get_syminfo_flag PROTO((ccstate_t *cs));
