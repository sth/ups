/* ups-if.c - UPS interface to GNU demangler. */

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
 *  MED3 is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with UPS; if not, write to the Free Software Foundation, Inc., 59 Temple
 *  Place, Suite 330, Boston, MA  02111-1307  USA
 */


char ups_ups_if_c_rcsid[] = "$Id$";

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if STDC_HEADERS
#include <stdlib.h>
#endif

#include <demangle.h>

#include "libiberty.h"

/*
 *  Demangle a GNU C++ function name into a a C++ representation of the name.
 *  Returns NULL or a pointer to a malloc'd string which the caller must free.
 *
 *	char *
 *	gnu_cplus_demangle (const char *mangled, int options);
 */

#if 0
/* COPY THIS CHUNK INTO cplus-dem.c */
/*
 *  Added for UPS.
 *  Entry point to use the gnu demangler to demangle type encodings as used
 *  in cast operators.
 *  Returns NULL or a pointer to a malloc'd string which the caller must free.
 */
char *
gnu_demangle_type (mangled, options)
const char **mangled;
int options;
{
    string decl;
    int success = 0;
    struct work_stuff work[1];
    char *result = NULL;

    memset ((char *) work, 0, sizeof (work));
    work->options = options;
    string_init (&decl);
    success = do_type(work, mangled, &decl);
    result = mop_up (work, &decl, success);
    return result;
}
#endif

PTR
xmalloc (size)
size_t size;
{
    return malloc (size);
}

PTR
xrealloc (ptr, size)
PTR ptr;
size_t size;
{
    return realloc (ptr, size);
}

char *
xstrdup (ptr)
const char *ptr;
{
    return strdup (ptr);
}
