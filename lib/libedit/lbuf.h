/* lbuf.h - header file for lbuf.c */

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


/* @(#)lbuf.h	1.3 09 Sep 1994 (UKC) */

#ifdef TEXTBUF_H_INCLUDED
Text_buffer *text_create_lbuf_buffer PROTO((alloc_pool_t *ap,
					    size_t maxblocks, size_t blocksize,
					    void *handle,
					    Text_info_func info_func,
					    Text_read_func read_func,
					    Text_close_func close_func,
					    bool apool_is_scratch));
#endif

#ifndef TEXT_BUFFER_DEFINED
typedef struct Text_buffer Text_buffer;
#define TEXT_BUFFER_DEFINED
#endif /* !TEXT_BUFFER_DEFINED */

Text_buffer *text_create_empty_lbuf_buffer PROTO((alloc_pool_t *ap,
						  size_t maxblocks,
						  size_t blocksize));

void text_get_buffer_modtime PROTO((Text_buffer *tb, long *mtime));
