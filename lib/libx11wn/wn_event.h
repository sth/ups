/* wn_event.h - header file for wn_event.c */

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


/* @(#)wn_event.h	1.4 4/7/91 (UKC) */

void _wn_catch_sigwsts PROTO((void));
void _wn_change_wn_fdmask PROTO((int fd));
void _wn_catch_sigwinch PROTO((void));
void _wn_enable_self_deiconise PROTO((void));
void _wn_set_inmode PROTO((swin_t *w, int mode));
void _wn_set_sunview_wakeup_fd PROTO((int wakeup_fd));

#ifdef X11
/*  Last window that we got an event on.  Used to speed up lookups
 *  in get_event.  We have to have it here so wn_close_window can
 *  set it to NULL, to avoid referencing destroyed windows.
 */
extern Window wn__Last_event_window;
extern void wn_set_MultiClickTime PROTO((void)); /* RCB */
#endif
