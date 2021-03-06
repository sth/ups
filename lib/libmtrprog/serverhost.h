/* serverhost.h - header file for serverhost.c */

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


/* @(#)serverhost.h	1.2 4/7/91 (UKC) */

struct serverhostent {
	const char *sh_servname;	/* name of the service (arbitrary) */
	const char **sh_hosts;		/* NULL terminated list of hosts */
};

typedef enum { SH_CLOSED, SH_OPEN, SH_EOF, SH_ERROR } serverhost_status_t;

serverhost_status_t getserverhoststatus PROTO((void));
int setserverhostent PROTO((void));
struct serverhostent *getserverhostent PROTO((void));
struct serverhostent *getserverhostbyservname PROTO((const char *servname));
void endserverhostent PROTO((void));
