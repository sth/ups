/* simplebuf.h - header file for simplebuf.c */

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


/* @(#)simplebuf.h	1.3 09 Sep 1994 (UKC) */

#ifndef TEXT_BUFFER_DEFINED
typedef struct Text_buffer Text_buffer;
#define TEXT_BUFFER_DEFINED
#endif /* !TEXT_BUFFER_DEFINED */

Text_buffer *text_create_simple_buffer PROTO((void));

Text_buffer *text_create_readonly_buffer PROTO((const char *text, size_t len));
