/*  panic.c -- Call a user user-defined panic handling routine and abort()  */

/*  Copyright 1992  Godfrey Paul, University of Kent at Canterbury.
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


/* @(#)panic.c	1.9 30/5/93 UKC */
char ukcprog_panic_rcsid[] = "$Id$";

#include <stdio.h>
#include <stdlib.h>

#include "ukcprog.h"


static panic_handler_t User_panic_handler = NULL;


/*
 *  install_panic_handler()
 *  Installs a new panic handler, returns the old one.
 */
panic_handler_t
install_panic_handler(handler)
panic_handler_t handler;
{
	panic_handler_t old;

	old = User_panic_handler;
	User_panic_handler = handler;

	return old;
}


/*
 *  panic()
 *  Called when the world has ended.  If a user-defined routine exists,
 *  call it with the given message as an argument.  If not, or if it
 *  returns, print a suitable message and abort.
 */
void
panic(message)
const char *message;
{
	if (User_panic_handler != NULL)
		(*User_panic_handler)(message);

	fprintf(stderr, "Fatal internal error: %s (aborting) ...\n", message);
	fflush(stderr);
	abort();
}
