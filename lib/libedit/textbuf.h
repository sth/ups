/* textbuf.h - generic text buffer interface */

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


/* @(#)textbuf.h	1.2 09 Sep 1994 (UKC) */

#define TEXTBUF_H_INCLUDED

#ifndef TEXT_BUFFER_DEFINED
typedef struct Text_buffer Text_buffer;
#define TEXT_BUFFER_DEFINED
#endif /* !TEXT_BUFFER_DEFINED */

typedef struct Text_bufdata Text_bufdata;

typedef struct {
	void (*destroy)PROTO((Text_bufdata *tb));

	void (*insert)PROTO((Text_bufdata *tb, size_t pos,
			     const char *text, size_t len));
	void (*delete)PROTO((Text_bufdata *tb, size_t pos, size_t len));

	bool (*get_bytes)PROTO((Text_bufdata *tb, size_t pos,
				const char **p_line, const char **p_lim));
	bool (*get_bytes_before)PROTO((Text_bufdata *tb, size_t pos,
				       const char **p_line,
				       const char **p_lim));

	size_t (*get_length)PROTO((Text_bufdata *tb));
	void (*set_debug_flag)PROTO((Text_bufdata *tb));
} Text_buffer_ops;

struct Text_buffer {
	Text_bufdata *bufdata;
	Text_buffer_ops *ops;
};

/*  I/O callbacks used with some buffer types.
 */
typedef bool (*Text_info_func)PROTO((char *handle,
				     size_t *p_size, time_t *p_mtime));
typedef bool (*Text_read_func)PROTO((char *handle, char *buf,
				     long pos, size_t count));
typedef void (*Text_close_func)PROTO((char *handle));
