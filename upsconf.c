/* $Id$ */

/*  Copyright 1999 Ian Edwards <ian@concerto.demon.co.uk>
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

/*
 * UPSCONF  -  check what 'configure' produced
 */

#if HAVE_CONFIG_H
#include <ifdefs.h>
#else
#error No ifdefs.h !
#endif

#if STDC_HEADERS
#include <float.h>
#endif
#if HAVE_LIMITS_H
#include <limits.h>
#endif

#define str(x) #x

#define TESTIT(what,string) \
 printf ("%-8.8s have " string "\n", (what == 0 ? "Does not" : "Does"))
#define YES_NO(what,string) \
 printf ("%-8.8s " string "\n", (what == 0 ? "Is not" : "Is"))
#define NUMBER(what,string) \
 printf ("%-8.8s " string " %d\n", "", what)


/*===========================================================================

    main	~~	entry point

----------------------------------------------------------------------------*/
int
main (int	argc,
      char **	argv)
/*--------------------------------------------------------------------------*/
{						/* main */

    char           *routine = "function()";
    const int       x = 3;

    /*
     *  CPU ARCHITECTURE & OPERATING SYSYEM
     */

    printf ("\n");
    printf ("Running on " HOST_CPU " architecture, on " HOST_OS ", supplier is " HOST_VENDOR ",\n");
    printf ("i.e. " THIS_ARCH "\n");
    printf ("\n");

    /*
     *  C LANGUAGE FEATURES
     */

#if defined(__STDC__)
    printf ("__STDC__ = %d\n", __STDC__);
#else
    printf ("Not __STDC__\n");
#endif
#if defined(__STRICT_ANSI__)
    printf ("__STRICT_ANSI__ = %d  (gcc -ansi)\n", __STRICT_ANSI__);
#else
    printf ("Not __STRICT_ANSI__  (gcc -ansi)\n");
#endif
#if defined(__GNUC__)
    printf ("__GNUC__ = %d\n", __GNUC__);
#else
    printf ("Not __GNUC__\n");
#endif
#if defined(__GLIBC__) && defined(__GLIBC_MINOR__)
    printf ("__GLIBC__ = %d.%d\n", __GLIBC__, __GLIBC_MINOR__);
#elif defined(__GLIBC__)
    printf ("__GLIBC__ = %d\n", __GLIBC__);
#else
    printf ("Not __GLIBC__\n");
#endif
    printf ("\n");


    /*
     *  LONG INTEGERS
     */

    NUMBER (SIZEOF_LONG,	"'long' byte length :");
    printf ("%-8.8s  LONG_MAX = %ld\n", "", LONG_MAX);

    /*
     *  LONG-LONG SUPPORT
     */

    TESTIT (HAVE_LONG_LONG,	"'long long' data type.");
    NUMBER (SIZEOF_LONG_LONG,	"'long long' byte length :");
#if HAVE_LONG_LONG
    {
	long long llx;
	llx = LONG_MAX;
	llx = llx * 10;
	printf ("%-8.8s 'long long' printed with %%" LONG_LONG_FMT "d, e.g.  %" LONG_LONG_FMT "d  (LONG_MAX*10)\n", "", llx);
	printf ("%-8.8s  lld = %lld  (SunOS & Solaris)\n", "", llx);
	printf ("%-8.8s  qd = %qd  (others)\n", "", llx);
    }
#endif

    NUMBER (SIZEOF_DOUBLE,	"'double' byte length :");

    /*
     *  LONG-DOUBLE SUPPORT
     */

    TESTIT (HAVE_LONG_DOUBLE,	"'long double' data type.");
    NUMBER (SIZEOF_LONG_DOUBLE,	"'long double' byte length :");
#if HAVE_LONG_DOUBLE
    {
	long double ldx;
	ldx = LDBL_MAX;
	printf ("%-8.8s 'long double' printed with %%" LONG_DOUBLE_FMT "g, e.g.  %" LONG_DOUBLE_FMT "g  (LDBL_MAX)\n", "", ldx);
    }
#endif

    YES_NO (IS_BIG_ENDIAN,	"big-endian (Motorola & SPARC are, Intel is not)");
    printf ("\n");

    /*
     *  STDIO FEATURES
     */

    TESTIT (HAVE_GLOBAL_CTYPE,	"character classification array" NAME_CTYPE);
    TESTIT (HAVE_GLOBAL_IOB,	"open file array " NAME_IOB);
    printf ("\n");

    /*
     *  VARIOUS ROUTINES
     */

    TESTIT (HAVE_STRERROR,	"strerror()");
    printf ("%-8.8s strerror(%d) = %s\n", "", 1, strerror(1));
    TESTIT (HAVE_POLL,		"poll()");
    TESTIT (HAVE_SELECT,	"select()");
    TESTIT (HAVE_VPRINTF,	"vprintf()");
    TESTIT (HAVE_DOPRNT,	"_doprnt()");
    TESTIT (HAVE_STRCHR,	"strchr()");
    TESTIT (HAVE_STRTOD,	"strtod()");
    TESTIT (HAVE_STRTOL,	"strtol()");
    TESTIT (HAVE_BCOPY,		"bcopy()");
    TESTIT (HAVE_MEMCPY,	"memcpy()");
    TESTIT (HAVE_MEMMOVE,	"memmove()");
    TESTIT (HAVE_MEMCMP,	"memcmp() that works (SunOS 4.1.3 does not)");
    printf ("\n");

    TESTIT (HAVE_STROPTS_H,	"streams IPC.");
    printf ("\n");

    exit (0);

}						/* main */


/* END */
