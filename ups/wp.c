/* wp.c - low level watchpoint list handling */

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


#include <mtrprog/ifdefs.h>

#include <stdlib.h>

#include <local/ukcprog.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "watchpoint.h"
#include "dx.h"

struct watchpoint_s {
	struct watchpoint_s *wp_next;
	struct watchpoint_s *wp_prev;
	taddr_t wp_addr;
	size_t wp_size;
	bool wp_enabled;
	bool wp_installed;
	bool wp_activated;
	int wp_watchnum;
	char *wp_val;
};

static watchpoint_t Wphead = { &Wphead, &Wphead, 0, 0, FALSE, FALSE, FALSE, 0, NULL };

watchpoint_t *
dx_add_watchpoint(xp, addr, size)
target_t *xp;
taddr_t addr;
size_t size;
{
	watchpoint_t *wp;

	wp = (watchpoint_t *)e_malloc(sizeof(watchpoint_t));

	wp->wp_next = Wphead.wp_next;
	wp->wp_prev = &Wphead;
	Wphead.wp_next->wp_prev = wp;
	Wphead.wp_next = wp;

	wp->wp_addr = addr;
	wp->wp_size = size;
	wp->wp_enabled = TRUE;
	wp->wp_installed = FALSE;
	wp->wp_activated = FALSE;
	wp->wp_watchnum = -1;
	wp->wp_val = e_malloc(wp->wp_size);

	return wp;
}

int
dx_remove_watchpoint(xp, wp)
target_t *xp;
watchpoint_t *wp;
{
	wp->wp_prev->wp_next = wp->wp_next;
	wp->wp_next->wp_prev = wp->wp_next;

	free(wp);

	return 0;
}

void
dx_enable_watchpoint(xp, wp)
target_t *xp;
watchpoint_t *wp;
{
	wp->wp_enabled = TRUE;
}

void
dx_disable_watchpoint(xp, wp)
target_t *xp;
watchpoint_t *wp;
{
	wp->wp_enabled = FALSE;
}

int
install_all_watchpoints(xp)
target_t *xp;
{
	watchpoint_t *wp;
	int n;

	for (wp = Wphead.wp_next; wp != &Wphead; wp = wp->wp_next) {
		if (!wp->wp_enabled)
			continue;

		if ((n = xp_install_watchpoint(xp, wp->wp_addr, wp->wp_size)) >= 0) {
			wp->wp_installed = TRUE;
			wp->wp_watchnum = n;
		}

		if (xp_read_data(xp, wp->wp_addr, wp->wp_val, wp->wp_size) < 0) {
			errf("Bad watch area 0x%x\n", wp->wp_addr);
			return -1;
		}
	}

	return activate_all_watchpoints(xp);
}

int
uninstall_all_watchpoints(xp)
target_t *xp;
{
	watchpoint_t *wp;

	if (deactivate_all_watchpoints(xp) < 0)
		return -1;

	for (wp = Wphead.wp_next; wp != &Wphead; wp = wp->wp_next) {
		if (wp->wp_installed &&
		    xp_uninstall_watchpoint(xp, wp->wp_addr, wp->wp_size, wp->wp_watchnum) == 0) {
			wp->wp_installed = FALSE;
			wp->wp_watchnum = -1;
		}
	}

	return 0;
}

int
activate_all_watchpoints(xp)
target_t *xp;
{
	watchpoint_t *wp;
	int activated = 0;

	for (wp = Wphead.wp_next; wp != &Wphead; wp = wp->wp_next) {
		if (wp->wp_enabled && !wp->wp_installed && !wp->wp_activated) {
			if (xp_set_protection(xp, wp->wp_addr, wp->wp_size, PT_RDONLY) < 0)
				return -1;

			wp->wp_activated = TRUE;
			activated++;
		}
	}

	return activated;
}

int
deactivate_all_watchpoints(xp)
target_t *xp;
{
	watchpoint_t *wp;

	for (wp = Wphead.wp_next; wp != &Wphead; wp = wp->wp_next) {
		if (wp->wp_activated &&
		    xp_set_protection(xp, wp->wp_addr, wp->wp_size, PT_ALL) < 0)
			return -1;
		else
			wp->wp_activated = FALSE;
	}

	return 0;
}

watchpoint_t *
check_all_watchpoints(xp)
target_t *xp;
{
	watchpoint_t *wp;
	static char *buf = NULL;
	static int buflen = 0;

	for (wp = Wphead.wp_next; wp != &Wphead; wp = wp->wp_next) {
		if (!wp->wp_enabled)
			continue;

		if (wp->wp_size > buflen) {
			buf = e_realloc(buf, wp->wp_size);
			buflen = wp->wp_size;
		}

		if (xp_read_data(xp, wp->wp_addr, buf, wp->wp_size) < 0) {
			errf("Bad watch area 0x%x", wp->wp_addr);
		}
		else if (memcmp(buf, wp->wp_val, wp->wp_size) != 0) {
			errf("Watch area at 0x%x has changed", wp->wp_addr);
			return wp;
		}
	}

	return NULL;
}
