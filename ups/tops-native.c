/* tops-native.c - ups using the native target driver for binaries */

/*  Copyright 1994 Mark Russell, University of Kent at Canterbury.
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


/* @(#)tops-native.c	1.2 24 May 1995 (UKC) */
char ups_tops_native_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "ao.h"		/* for Ao_ops */
#include "ao_ifdefs.h"
#include "xc.h"		/* for Xc_ops and Cc_ops */
#include "gd.h"		/* for Gd_ops */

xp_ops_t **
get_target_drivers()
{
	static xp_ops_t *drivers[] = {
		&Xc_ops,
		&Cc_ops,
#ifdef AO_TARGET
		&Ao_ops,
#endif
#ifdef GDB_TARGET
		&Gd_ops,
#endif
		NULL
	};

	return drivers;
}
