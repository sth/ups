/* win.h - header file for win.c */

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


/* @(#)win.h	1.7 09 Apr 1995 (UKC) */

Edit_fontinfo *edit_make_wn_fontinfo PROTO((font_t *font));

Edit_display *edit_create_wn_display PROTO((int wn, int fg, int bg,
					    bool want_cursor,
					    bool keep_cursor_visible));

void edit_handle_wn_key_event PROTO((Edit_display *d, event_t *ev));

bool edit_update_wn_window_size PROTO((Edit_display *d, int wn));
void edit_update_wn_window_colors PROTO((Edit_display *d, int fg, int bg));

void edit_set_wn_selection PROTO((Edit_buffer *buffer,
				  size_t start, size_t count));
