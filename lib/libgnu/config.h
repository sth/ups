/* config.h - fake 'config.h' for GNU files. */

/*  Copyright 2001 Ian Edwards
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


/* $Id$ */

#include "ifdefs.h"

/*
 *  'cpus_demangle()' is the name of the SunPRO demangler supplied with
 *  the SunPRO compiler.  To prevent name collisions, we rename 
 *  the GNU routine to 'gnu_cplus_demangle()'. (RCB)
 */
#define cplus_demangle gnu_cplus_demangle

