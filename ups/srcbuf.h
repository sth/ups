/* srcbuf.h - public header file for srcbuf.c */

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


/* @(#)srcbuf.h	1.2 09 Apr 1995 (UKC) */

#define SRCBUF_H_INCLUDED

typedef struct Srcbuf Srcbuf;
typedef struct Editblock Editblock;

void srcbuf_destroy PROTO((Srcbuf *sb));

size_t srcbuf_nlines_in_file PROTO((Srcbuf *sb));

bool srcbuf_point_to_file_lnum PROTO((Srcbuf *sb, size_t point, int *p_lnum));

bool srcbuf_file_lnum_to_point_range PROTO((Srcbuf *sb, int lnum, 
					    size_t *p_start, size_t *p_lim));


void srcbuf_count_pixels PROTO((Srcbuf *sb,
				size_t max_point, int max_ypos,
				int editline_height, int srcline_height,
				int *p_lnum, int *p_ypos));

int srcbuf_get_editblock_height PROTO((Editblock *eb, size_t lim_point,
				       int editline_height,
				       int srcline_height));

void srcbuf_select_editblock PROTO((Editblock *eb, bool selected));
void srcbuf_accel_editblock_change PROTO((Editblock *eb)); /* RCB */

#ifdef EDIT_H_INCLUDED
Srcbuf *srcbuf_visit_file PROTO((alloc_pool_t *ap,
				 const char *what, const char *path,
				 Edit_fontinfo *fi, Edit_flags flags));

Edit_buffer *srcbuf_get_buffer PROTO((Srcbuf *sb));
Edit_propchange* srcbuf_get_propchange_at_point PROTO((Srcbuf* sb, size_t point));
#endif

const char *srcbuf_get_path PROTO((Srcbuf *sb));

typedef enum {
	EDL_SELECT,
	EDL_DESELECT,
	EDL_REMOVE,
	EDL_ACCELERATOR_ACTION /* RCB*/
} Editblock_action;

typedef void (*Editblock_callback)PROTO((Editblock *editblock,
					 Editblock_action action));

typedef bool (*Editblock_change_callback)PROTO((Editblock *editblock,
						const char *new_text,
						bool force,
						size_t *p_error_pos));

char *srcbuf_get_editblock_data PROTO((Editblock *eb));

#ifdef EDIT_H_INCLUDED
Editblock *srcbuf_add_editblock PROTO((Srcbuf *srcbuf, int fil_lnum,
				       Edit_flags flagmask, Edit_flags flags,
				       Edit_fontinfo *start_fontinfo,
				       Edit_fontinfo *end_fontinfo,
				       char *text,
				       Editblock_callback callback,
				       Editblock_change_callback change_callback,
				       char *user_data));
void srcbuf_editblock_change_flags PROTO((Editblock *editblock,
					  Edit_flags flags));
#endif

void srcbuf_remove_editblock PROTO((Editblock *editblock, 
                                    bool call_callback));

bool srcbuf_change_editblock PROTO((Editblock *editblock, bool force,
				    size_t *p_error_point));

const char *srcbuf_get_editblock_text PROTO((Editblock *editblock));
void srcbuf_get_editblock_point_range PROTO((Editblock *editblock,
					     size_t *p_start, size_t *p_lim));
const char *srcbuf_get_file_line PROTO((Srcbuf *sb, int lnum));
void srcbuf_get_buffer_modtime PROTO((Srcbuf *sb, long *mtime));

int *srcbuf_get_editblock_in_tracking_file PROTO((Editblock *eb1,
						  Srcbuf *sb,
						  Srcbuf *sb1));
int *srcbuf_update_tracking_editblock PROTO((Editblock *eb1,
					     Srcbuf *sb,
					     const char **lines,
					     int nlines));
void srcbuf_reset_file_handle PROTO((void));
