/* filebuf.h - header file for filebuf.c */

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


/* @(#)filebuf.h	1.2 31 Aug 1994 (UKC) */

#ifndef TEXT_BUFFER_DEFINED
typedef struct Text_buffer Text_buffer;
#define TEXT_BUFFER_DEFINED
#endif /* !TEXT_BUFFER_DEFINED */

bool edit_filebuf_visit_file PROTO((alloc_pool_t *ap,
				    const char *what, const char *path,
				    size_t maxblocks, size_t blocksize,
				    Text_buffer **p_buffer));
void edit_reset_file_handle PROTO((void));
