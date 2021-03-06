/* basename.c - routines for manipulating file names */

/*  Copyright 2002 Ian Edwards
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

char ukcprog_basename_rcsid[] = "$Id$";

#include <string.h>

#include "ukcprog.h"

/*
 * Get the file name out of a path.
 * If path is empty or ends in a '/' it is returned as-is.
 *
 * Some systems have basename(3) but it does not behave in
 * quite the same way.
 */
const char *
base_name(path)
const char *path;
{
    char *ptr;

    if ((path == NULL) || (*path == '\0'))
	return path;
    if ((ptr = strrchr(path, '/')) == NULL)
	return path;
    if (*(ptr+1) == '\0')
	return path;
    return ptr+1;
}

