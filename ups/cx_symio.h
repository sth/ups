/* cx_symio.h - header file for cx_symio.c */

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


/* @(#)cx_symio.h	1.1 22/12/93 (UKC) */

int save_string PROTO((ebuf_t *str_eb, const char *s, FILE *fp));
size_t note_string PROTO((ebuf_t *str_eb, const char *s));
int write_strings PROTO((ebuf_t *eb, FILE *fp, size_t *p_nbytes));

int fp_write PROTO(( FILE *fp, const char *buf, size_t count));
int readfp PROTO((FILE *fp, const char *path, char *buf, size_t nbytes));

#ifdef XC_LOAD_H_INCLUDED
int write_symtab PROTO((FILE *fp, const char *path, ebuf_t *str_eb,
				       linkinfo_t *li, long *p_symtab_size));
int read_symtab PROTO((FILE *fp, const char *path, size_t symtab_size,
							     linkinfo_t *li));
int get_symtab_size PROTO((o_syminfo_t *os));

int fp_write_val PROTO((FILE *fp, size_t val));
#endif
