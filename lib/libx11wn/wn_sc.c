/* wn_sc.c - special cursors */

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


/* @(#)wn_sc.c	1.14 21 Apr 1994 (UKC) */
char wn_wn_sc_c_rcsid[] = "$Id$";

#include <stdio.h>
#include <sys/types.h>

#include "wn.h"
#include "wn_priv.h"
#include "wn_sc.h"
#include "wn_misc.h"

/*  This file implements crosshair and rubber band cursors.
 *  These were provided as a backwards compatibility aid for
 *  code originally written on the ICL Perq.  The Perq has
 *  long since gone.  Old machines never die - they linger on
 *  as backwards compatibility routines ...
 */

static void sc_line PROTO((int wn, sc_t *sc, int x, int y));
static void sc_box PROTO((int wn, sc_t *sc, int x, int y));
static void sc_xhair PROTO((int wn, sc_t *sc, int x, int y));

static void
sc_line(wn,sc,x,y)
int wn;
register sc_t *sc;
int x, y;
{
	register int rflags;

	rflags = sc->sc_rflags;
	sc->sc_cx1 = sc->sc_x1 + ((rflags & WN_SC_X1REL) ? x : 0);
	sc->sc_cy1 = sc->sc_y1 + ((rflags & WN_SC_Y1REL) ? y : 0);
	sc->sc_cx2 = sc->sc_x2 + ((rflags & WN_SC_X2REL) ? x : 0);
	sc->sc_cy2 = sc->sc_y2 + ((rflags & WN_SC_Y2REL) ? y : 0);
	wn_invert_line(wn, sc->sc_cx1, sc->sc_cy1, sc->sc_cx2, sc->sc_cy2);
}

static void
sc_box(wn,sc,x,y)
int wn;
sc_t *sc;
int x, y;
{
	int x1, x2, y1, y2, t;
	register int rflags;

	rflags = sc->sc_rflags;
	x1 = sc->sc_x1 + ((rflags & WN_SC_X1REL) ? x : 0);
	y1 = sc->sc_y1 + ((rflags & WN_SC_Y1REL) ? y : 0);
	x2 = sc->sc_x2 + ((rflags & WN_SC_X2REL) ? x : 0);
	y2 = sc->sc_y2 + ((rflags & WN_SC_Y2REL) ? y : 0);
	if (x1 > x2) {
		t = x1;
		x1 = x2;
		x2 = t;
	}
	if (y1 > y2) {
		t = y1;
		y1 = y2;
		y2 = t;
	}
	wn_invert_line(wn, x1, y1, x2-1, y1);
	wn_invert_line(wn,         x2,   y1, x2, y2-1);
	wn_invert_line(wn,                   x2, y2,   x1+1, y2);
	wn_invert_line(wn,                             x1,   y2, x1, y1+1);

	sc->sc_cx1 = x1;
	sc->sc_cy1 = y1;
	sc->sc_cx2 = x2;
	sc->sc_cy2 = y2;
}

static void
sc_xhair(wn,sc,x,y)
int wn;
register sc_t *sc;
int x, y;
{
	int cx, cy, hw, hh;
	register int rflags;

	rflags = sc->sc_rflags;
	cx = sc->sc_x1 + ((rflags & WN_SC_X1REL) ? x : 0);
	cy = sc->sc_y1 + ((rflags & WN_SC_Y1REL) ? y : 0);
	hw = sc->sc_x2 + ((rflags & WN_SC_X2REL) ? x : 0);
	hh = sc->sc_y2 + ((rflags & WN_SC_Y2REL) ? y : 0);

	wn_invert_line(wn, cx - hw, cy, cx + hw, cy);

	/*  Don't want a white spot at the crossover point, so draw the
	 *  vertical line in two parts.
	 */
	wn_invert_line(wn, cx, cy - hh, cx, cy - 1);
	wn_invert_line(wn, cx, cy + 1, cx, cy + hh);

	sc->sc_cx1 = cx - hw;
	sc->sc_cy1 = cy - hh;
	sc->sc_cx2 = cx + hw;
	sc->sc_cy2 = cy + hh;
}

void
_wn_sc_refresh(wn,x,y)
int wn;
int x, y;
{
	register swin_t *w;
	register sc_t *sc;

	w = WN_TO_W(wn)->w_base;
	sc = &w->w_bw->bw_scbuf;
	if (w->w_bw->bw_sc_drawn && x == sc->sc_last_x && y == sc->sc_last_y)
		return;
	wn_updating_off(wn);

	w->w_bw->bw_have_sc = FALSE;
	if (w->w_bw->bw_sc_drawn) {
		w->w_bw->bw_sc_drawn = FALSE;
		(*sc->sc_func)(w->w_wn,sc,sc->sc_last_x,sc->sc_last_y);
	}
	(*sc->sc_func)(w->w_wn,sc,x,y);
	w->w_bw->bw_sc_drawn = w->w_bw->bw_have_sc = TRUE;

	wn_updating_on(w->w_wn);
	sc->sc_last_x = x;
	sc->sc_last_y = y;
}

void
_wn_sc_undraw(wn)
int wn;
{
	register swin_t *w;
	register sc_t *sc;

	w = WN_TO_W(wn)->w_base;
	sc = &w->w_bw->bw_scbuf;
	if (!w->w_bw->bw_sc_drawn)
		wn__panic("_wn_sc_undraw called with w_sc_drawn FALSE");
	w->w_bw->bw_sc_drawn = w->w_bw->bw_have_sc = FALSE;
	(*sc->sc_func)(w->w_wn,sc,sc->sc_last_x,sc->sc_last_y);
	w->w_bw->bw_have_sc = TRUE;
}

void
_wn_sc_redraw(wn)
int wn;
{
	register swin_t *w;
	register sc_t *sc;

	w = WN_TO_W(wn)->w_base;
	sc = &w->w_bw->bw_scbuf;
	if (!w->w_bw->bw_sc_drawn) {
		w->w_bw->bw_have_sc = FALSE;
		(*sc->sc_func)(w->w_wn,sc,sc->sc_last_x,sc->sc_last_y);
		w->w_bw->bw_sc_drawn = w->w_bw->bw_have_sc = TRUE;
	}
}

/*  Set or turn off a special cursor for window wn.
 *
 *  The wn special cursor routines are modelled on the Perq facilities.
 */
void
wn_spcu(wn,type_and_flags,x1,y1,x2,y2)
int wn;
unsigned type_and_flags;
int x1,y1,x2,y2;
{
	register swin_t *w;
	static void (*ftab[])PROTO((int fwn, sc_t *sc, int x, int y)) =
					{ NULL, sc_box, sc_line, sc_xhair };
	sc_t *sc;
	unsigned type, rflags;

	W_CHECK(wn);
	w = WN_TO_W(wn);
	type = type_and_flags & WN_SC_TYPE;
	rflags = type_and_flags & WN_SC_FLAGS;

	if (type >= sizeof(ftab) / sizeof(ftab[0]))
		wn__panic("bad type in wn_spcu");

	if ((type & ~(unsigned)(WN_SC_TYPE | WN_SC_FLAGS)) != 0)
		wn__panic("bad type_and_flags in wn_spcu");

	if (!(rflags & WN_SC_X1REL))
		x1 += w->w_x_offs;
	if (!(rflags & WN_SC_Y1REL))
		y1 += w->w_y_offs;
	if (!(rflags & WN_SC_X2REL))
		x2 += w->w_x_offs;
	if (!(rflags & WN_SC_Y2REL))
		y2 += w->w_y_offs;

	sc = &w->w_bw->bw_scbuf;
	if (w->w_bw->bw_sc_drawn) {
		w->w_bw->bw_have_sc = FALSE;
		(*sc->sc_func)(w->w_base->w_wn,sc,sc->sc_last_x,sc->sc_last_y);
	}
	sc->sc_func = (void (*)PROTO((int fwn, ...))) ftab[type];
	w->w_bw->bw_have_sc = (type != WN_SC_OFF);
	w->w_bw->bw_sc_drawn = FALSE;
	sc->sc_x1 = x1;
	sc->sc_y1 = y1;
	sc->sc_x2 = x2;
	sc->sc_y2 = y2;
	sc->sc_rflags = rflags;
}
