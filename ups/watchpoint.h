/* watchpoint.h - public header file for wp.c */

/*  Copyright 2002 Tom Hughes <thh@cyberscience.com>
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


#define WATCHPOINT_H_INCLUDED

#ifdef TARGET_H_INCLUDED

int install_all_watchpoints PROTO((target_t *xp));
int uninstall_all_watchpoints PROTO((target_t *xp));
int activate_all_watchpoints PROTO((target_t *xp));
int deactivate_all_watchpoints PROTO((target_t *xp));

#ifdef SYMTAB_H_INCLUDED
watchpoint_t *check_all_watchpoints PROTO((target_t *xp));
#endif

#endif
