/* edump.c - convert a binary wn event record file to ASCII */

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


/* @(#)edump.c	1.5 4/7/91 (UKC) */
char wn_edump_c_rcsid[] = "$Id$";

#include <stdio.h>

#include "wn.h"

void main PROTO((int argc, const char **argv));
static void dump_events PROTO((FILE *fp));
static const char *button_name PROTO((int flags));
static const char *event_name PROTO((int type));

void
main(argc, argv)
int argc;
const char **argv;
{
	if (argc == 1) {
		if (wn_set_replay_file("-") != 0) {
			perror("stdin");
			exit(1);
		}
		dump_events(stdout);
	}
	else {
		while (*++argv != NULL) {
			if (wn_set_replay_file(*argv) != 0) {
				perror(*argv);
				exit(1);
			}
			dump_events(stdout);
		}
	}
}

static const char *
button_name(flags)
int flags;
{
	if (flags & B_LEFT)
		return "left";
	if (flags & B_MIDDLE)
		return "middle";
	if (flags & B_RIGHT)
		return "right";
	return "<no button>";
}

static const char *
event_name(type)
int type;
{
	switch(type) {
		case EV_WINDOW_RESIZED:		return "resized";
		case EV_WINDOW_EXPOSED:		return "exposed";
		case EV_WINDOW_SELECTED:	return "selected";
		case EV_WINDOW_DESELECTED:	return "deselected";
		case EV_INTERRUPT:		return "interrupt";
		case EV_OTHER_INPUT:		return "other_input";
		case EV_SELECTION_REQUEST:	return "selection_request";
		case EV_LOST_SELECTION:		return "lost_selection";
		case EV_OTHER:			return "other";
	}
	return "<unknown event type>";
}

static void
dump_events(fp)
FILE *fp;
{
	int last_x, last_y, dx, dy;
	int type, last_type;
	int n_mouse_moved;
	event_t event;

	n_mouse_moved = 0;
	last_x = last_y = 0;
	last_type = EV_MOUSE_MOVED;
	while (wn_get_recorded_event(&event) == 0) {

		type = event.ev_type;

		if (n_mouse_moved > 15 ||
		    (type != EV_MOUSE_MOVED && last_type == EV_MOUSE_MOVED)) {
			n_mouse_moved = 0;
			putc('\n', fp);
		}

		dx = event.ev_x - last_x;
		dy = event.ev_y - last_y;
		if (dx == 0 && dy == 0)
			fputs("z ", fp);
		else
			fprintf(fp, "%d,%d ", dx, dy);

		switch(type) {
		case EV_KEY:
			fprintf(fp, "k%c\n", event.ev_char);
			break;
		case EV_BUTTON_UP:
			fprintf(fp, "%s up\n", button_name(event.ev_flags));
			break;
		case EV_BUTTON_DOWN:
			fprintf(fp, "%s down\n", button_name(event.ev_flags));
			break;
		case EV_MOUSE_MOVED:
			++n_mouse_moved;
			break;
		default:
			fprintf(fp, "%s\n", event_name(event.ev_type));
			break;
		}

		last_x = event.ev_x;
		last_y = event.ev_y;
		last_type = type;
	}
}
