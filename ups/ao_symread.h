/* ao_symread.h - header file for ao_symread.c */

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


/* @(#)ao_symread.h	1.4 04 Jun 1995 (UKC) */

void get_func_coff_lno_offsets PROTO((symio_t *si, fil_t *sfiles));
lno_t *read_func_coff_lnos PROTO((symio_t *si, func_t *f, lno_t **p_last));

symio_t *make_symio PROTO((alloc_pool_t *ap, const char *textpath, int fd,
			   off_t syms_offset, int nsyms, off_t strings_offset));
int get_symio_nsyms PROTO((symio_t *si));
void close_symio PROTO((symio_t *si));
int get_symio_fd PROTO((symio_t *si));

void getsym PROTO((symio_t *si, int symno, nlist_t *p_nm));
int findsym PROTO((symio_t *si, int symno, nlist_t *p_res, const char *symset));

void mark_sym PROTO((symio_t *si, int symno));
int next_unmarked_sym PROTO((symio_t *si, int symno));

const char *si_get_string PROTO((symio_t *si, off_t offset));
const char *get_cont_symstring PROTO((Symrec *sr));
const char *symstring PROTO((symio_t *si, int symno));

void add_extra_string_offset PROTO((symio_t *si, int symno, off_t offset));
