/* wn_misc.h - header file for wn_misc.c */

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


/* @(#)wn_misc.h	1.6 25/4/92 (UKC) */

void wn__do_xflush PROTO((void));
char *wn__e_malloc PROTO((size_t nbytes));
char *wn__e_realloc PROTO((char *ptr, size_t nbytes));
void wn__panic PROTO((const char *mesg));
#ifdef X11
void wn__send_selection PROTO((XEvent *event));
#endif
#ifdef SUNVIEW
int wn__setup_sunview_wakeup_pipe PROTO((void));
extern int wn__Lost_selection;
#endif
