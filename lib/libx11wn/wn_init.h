/* wn_init.h - header file for wn_init.c */

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


/* @(#)wn_init.h	1.2 4/7/91 (UKC) */

void _wn_add_to_unmunge_list PROTO((const char **args, int nargs));
const char *_wn_reason PROTO((void));
int _wn_init PROTO((void));
int _wn_make_window PROTO((swin_t *w, const char *name, int is_mainwin));
void _wn_map_X_window PROTO((swin_t *w, swin_t *iw));

#ifdef SUNVIEW
void _wn_make_retained_pixrect PROTO((struct pixwin *pw, int width, int height));
#endif

#ifdef X11
void wn_get_X11_info PROTO((Display **p_display, Window *p_win));
void process_client_message PROTO((XEvent *ev, Window p_win, int wn));
#endif

extern const char *_wn_Sysfont_file;

