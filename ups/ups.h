/* ups.h - typedefs etc that appear in most source files */

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


/* @(#)ups.h	1.7 22 Dec 1993 (UKC) */

/*  Type used for target addresses.
 */
typedef unsigned long taddr_t;

/*  Bit pattern of a NULL pointer as read from the target address space.
 *
 *  Note that ups has never been tested (and probably would not work)
 *  with this value anything other than zero.
 */
#define TADDR_NULL	((taddr_t)0)

void save_tty PROTO((void));
void reset_tty PROTO((void));
