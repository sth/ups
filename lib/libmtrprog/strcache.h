/* strcache.h - header file for strcache.c */

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


/* @(#)strcache.h	1.7 22/12/93 (UKC) */

/*  Opaque type for strcache ids.
 */
typedef struct strcache_idst { int strcache_id_dummy; } *strcache_id_t;

/*  The return value here ought to be ssize_t, but it's just too painful
 *  to reliably get ssize_t defined in K&R and ANSI C.
 */
typedef long (*sc_input_func_t)PROTO((char *arg, off_t pos,
						void *buf, size_t nbytes));

typedef void (*sc_close_func_t)PROTO((char *arg));

strcache_id_t sc_make_strcache PROTO((sc_input_func_t input_func,
				      sc_close_func_t close_func,
				      char *input_func_arg));
strcache_id_t sc_make_fd_strcache PROTO((int fd));
strcache_id_t sc_dup_strcache PROTO((strcache_id_t strcache));
void sc_close_strcache PROTO((strcache_id_t strcache_id));

void sc_set_offset PROTO((strcache_id_t strcache_id, off_t offset));
off_t sc_get_offset PROTO((strcache_id_t strcache_id));

int sc_set_bufs PROTO((strcache_id_t strcache_id, int nbufs, size_t bufsize));
void sc_forget_buffers PROTO((strcache_id_t strcache_id));
void sc_file_has_grown PROTO((strcache_id_t strcache_id));

const char *sc_get_string PROTO((strcache_id_t strcache_id,
				 off_t offset, int endchar, size_t *p_len));
char *sc_get_bytes PROTO((strcache_id_t strcache_id,
				off_t offset, size_t nbytes, size_t *p_len));

void sc_dump_stats PROTO((strcache_id_t strcache_id));
void sc_dump_stats_of_newest_sc PROTO((void));
