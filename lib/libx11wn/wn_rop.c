/* wn_rop.c - on screen rasterops and line drawing */

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


/* @(#)wn_rop.c	1.18 1/3/93 (UKC) */
char wn_wn_rop_c_rcsid[] = "$Id$";

#include <sys/types.h>
#include <string.h>

#include "wn.h"
#include "wn_priv.h"
#include "wn_sc.h"
#include "wn_xrop.h"
#include "wn_bm.h"
#include "wn_misc.h"
#include "wn_rop.h"
#include "wn_color.h"

static int default_get_cur_posn_func PROTO((void));
static int default_get_cur_posn_func() {return 0;}

static wn_get_cur_posn_func_t Get_cur_posn_func = default_get_cur_posn_func;

static void default_overlay_insert_string_func PROTO((const char *text, int pos,
						      int len,
						      int prepend_newline,
						      int fg_color,
						      int var_coloring_only));

static wn_overlay_insert_string_func_t
Overlay_insert_string_func = default_overlay_insert_string_func;

#ifdef X11
static Bool get_gexpose_event PROTO((Display *unused_display, XEvent *xev, char *cwin));
#endif

/*  Rasterop lookup tables for the systems wn runs over.
 */
int _wn_Roptab[] = {
#ifdef X11
	GXcopy,				/* R_RPL	*/
	GXcopyInverted,			/* R_NOT	*/
	GXand,				/* R_AND	*/
	GXandInverted,			/* R_ANDNOT	*/
	GXor,				/* R_OR		*/
	GXorInverted,			/* R_ORNOT	*/
	GXxor,				/* R_XOR	*/
	GXequiv,			/* R_XNOR	*/
#endif /* X11 */
#ifdef SUNVIEW
	PIX_SRC,			/* R_RPL	*/
	PIX_NOT(PIX_SRC),		/* R_NOT	*/
	PIX_SRC & PIX_DST,		/* R_AND	*/
	PIX_NOT(PIX_SRC) & PIX_DST,	/* R_ANDNOT	*/
	PIX_SRC | PIX_DST,		/* R_OR		*/
	PIX_NOT(PIX_SRC) | PIX_DST,	/* R_ORNOT	*/
	PIX_SRC ^ PIX_DST,		/* R_XOR	*/
	PIX_NOT(PIX_SRC) ^ PIX_DST	/* R_XNOR	*/
#endif /* SUNVIEW */
};

#ifdef X11
/*  On a monochrome display we have no choice about pixel to colour
 *  mapping - a set bit is a white pixel and a clear bit is black.
 *  To get round this, we use a rather peculiar rasterop function
 *  mapping, which is set up below.
 */
void
_wn_fix_ropfuncs()
{
	static int munged_roptab[] = {
		GXcopy,				/* R_RPL	*/
		GXcopyInverted,			/* R_NOT	*/
		GXor,				/* R_AND	*/
		GXorInverted,			/* R_ANDNOT	*/
		GXand,				/* R_OR		*/
		GXandInverted,			/* R_ORNOT	*/
		GXequiv,			/* R_XOR	*/
		GXxor,				/* R_XNOR	*/
	};
	int i;

	for (i = 0; i < 8; i++)
		_wn_Roptab[i] = munged_roptab[i];
}
#endif /* X11 */

void
wn_tile_area(wn, x, y, width, height, bm, ropfunc)
int wn;
int x, y, width, height;
bitmap_t *bm;
int ropfunc;
{
	swin_t *w;
	format_t format;
	int changed;
	size_t l;

	W_CHECK(wn);
	if (bm->bm_width != 16 || bm->bm_height != 16 || bm->bm_nplanes != 1)
		wn__panic("bitmap not 16 x 16 x 1 in wn_tile_area");
	if (width <= 0 || height <= 0)
		wn__panic("Bad width or height in shade");

	w = WN_TO_W(wn);
	l = sizeof(W_LAST_BM(w));
	ADJ_COORDS(w, x, y);
	SAVE_FORMAT(bm, format, DISP_OK);
	if (W_GREYPIXMAP(w) == 0)
		changed = TRUE;
	else
		changed = memcmp((char *)&W_LAST_BM(w), (char *)bm->bm_data, l) != 0;
	if (changed)
		memcpy((char *)&W_LAST_BM(w), (char *)bm->bm_data, l);
	SC_UNDRAW(wn);

#ifdef X11
	if (changed) {
		if (W_GREYPIXMAP(w) != 0)
			XFreePixmap(wn__Dpy, W_GREYPIXMAP(w));
		W_GREYPIXMAP(w) = XCreatePixmap(wn__Dpy, w->w_win,
				16, 16, 
				(unsigned)DisplayPlanes(wn__Dpy, W_SCREEN(w)));
		_wn_send_bm(bm, 0, 0, 16, 16, W_GREYPIXMAP(w), W_GC_NOCLIP(w),
			    0, 0, R_RPL, W_FG_PIXEL(w), W_BG_PIXEL(w));
	}
	XSetTile(wn__Dpy, W_GC(w), W_GREYPIXMAP(w));
	XSetState(wn__Dpy, W_GC(w), W_FG_PIXEL(w), W_BG_PIXEL(w),
				      _wn_Roptab[ropfunc], AllPlanes);
	XSetFillStyle(wn__Dpy, W_GC(w), FillTiled);
	XFillRectangle(wn__Dpy, w->w_win, W_GC(w), x, y,
						(xdim_t)width, (xdim_t)height);
	XSetFillStyle(wn__Dpy, W_GC(w), FillSolid);
	X_UPDATE(w);
#endif /* X11 */

#ifdef SUNVIEW
	if (changed) {
		if (grey_pr != NULL)
			pr_destroy(grey_pr);
		grey_pr = mem_point(16, 16, 1, last);
	}
	w->w_bw->bw_can_refresh = FALSE;
	pw_replrop(w->w_pw, x, y, width, height, _wn_Roptab[ropfunc], W_GREYPIXMAP(w), 0, 0);
	w->w_bw->bw_can_refresh = TRUE;
#endif /* SUNVIEW */

	SC_REDRAW(wn);
	RESTORE_FORMAT(bm, format);
}

void
wn_shade_area(wn, x, y, width, height, orig_shade, ropfunc)
int wn;
int x, y, width, height;
int orig_shade;
int ropfunc;
{
	bitmap_t *bm;
	unsigned short data[16], *sptr;
	int i, j, shade; 
	register unsigned short pat;

	sptr = data;
	for (i = 0; i < 4; i++) {
		shade = orig_shade;
		for (j = 0; j < 4; j++) {
			pat = shade & 0xf;
			pat |= pat << 4;
			pat |= pat << 8;
			*sptr++ = pat;
			shade >>= 4;
		}
	}
	bm = wn_make_bitmap_from_data(16, 16, 1, data,
						BM_BIT0_RIGHT, BM_XY_PIXELS, 2);
	wn_tile_area(wn, x, y, width, height, bm, ropfunc);
	wn_free_bitmap(bm);
}

void
wn_mono_rop(wn, x, y, width, height, new_x, new_y, ropfunc)
int wn;
int x, y, width, height, new_x, new_y, ropfunc;
{
	register swin_t *w = WN_TO_W(wn);
#ifdef X11
	XEvent xevent;
#endif /* X11 */

	W_CHECK(wn);
	ADJ_COORDS(w, x, y);
	ADJ_COORDS(w, new_x, new_y);
	SC_UNDRAW(wn);
#ifdef X11
	XSetState(wn__Dpy, W_GC(w),
		  W_FG_PIXEL(w), W_BG_PIXEL(w),
		  _wn_Roptab[ropfunc], W_PLANEMASK(w));
	XSetGraphicsExposures(wn__Dpy, W_GC(w), x != new_x || y != new_y);
	XCopyArea(wn__Dpy, w->w_win, w->w_win, W_GC(w), x, y,
		  (xdim_t)width, (xdim_t)height, new_x, new_y);
	if (x != new_x || y != new_y) {
		XIfEvent(wn__Dpy, &xevent, get_gexpose_event, (char *)w->w_win);
		w->w_bw->bw_last_rop_was_damaged = xevent.type == GraphicsExpose;
	}
	else
		w->w_bw->bw_last_rop_was_damaged = FALSE;
	X_UPDATE(w);
#endif /* X11 */
#ifdef SUNVIEW
	w->w_bw->bw_can_refresh = FALSE;

	if (w->w_pw->pw_pixrect->pr_depth != 1) {
		pw_putattributes(w->w_pw, &W_PLANEMASK(w));
		pr_putattributes(w->w_pw->pw_prretained, &W_PLANEMASK(w));
	}
	pw_copy(w->w_pw, new_x, new_y, width, height,
					_wn_Roptab[ropfunc], w->w_pw, x, y);
	if (w->w_pw->pw_pixrect->pr_depth != 1) {
		pw_putattributes(w->w_pw, &w->w_bw->bw_planes);
		pr_putattributes(w->w_pw->pw_prretained, &w->w_bw->bw_planes);
	}

	w->w_bw->bw_can_refresh = TRUE;
#endif /* SUNVIEW */
	SC_REDRAW(wn);
}

#ifdef X11
/* BUG: no check on cwin
 */
/* ARSUSED */
static Bool
get_gexpose_event(unused_display, xev, cwin)
Display *unused_display;
XEvent *xev;
char *cwin;
{
	if (xev->type == GraphicsExpose || xev->type == NoExpose) {
		return True;
	}
	else {
		return False;
	}
}
#endif /* X11 */

int
wn_last_rop_was_damaged(wn)
int wn;
{
	W_CHECK(wn);
#ifdef X11
	return WN_TO_W(wn)->w_bw->bw_last_rop_was_damaged;
#else
	return FALSE;
#endif /* !X11 */
}

/*   Copy area (x,y,width,height) of the window to (new_x,new_y). Use
 *   ROP function R_RPL
 */
void
wn_rop(wn,x,y,width,height,new_x,new_y)
int wn;
int x,y,width,height,new_x,new_y;
{
	register swin_t *w = WN_TO_W(wn);
#ifdef X11
	XEvent xevent;
#endif /* X11 */
	int last_y;

	if (wn_text_capture(0, 0, &last_y))
	  return;

	W_CHECK(wn);
	if (width <= 0 || height <= 0)
		return;
	ADJ_COORDS(w, x, y);
	ADJ_COORDS(w, new_x, new_y);
	SC_UNDRAW(wn);
#ifdef X11
	XSetState(wn__Dpy, W_GC(w),
			   W_FG_PIXEL(w), W_BG_PIXEL(w),
			   _wn_Roptab[R_RPL], AllPlanes);
	if (x != new_x || y != new_y)
		XSetGraphicsExposures(wn__Dpy, W_GC(w), TRUE);
	XCopyArea(wn__Dpy, w->w_win, w->w_win, W_GC(w), x, y,
				(xdim_t)width, (xdim_t)height, new_x, new_y);
	if (x != new_x || y != new_y) {
		XIfEvent(wn__Dpy, &xevent, get_gexpose_event, (char *)w->w_win);
		w->w_bw->bw_last_rop_was_damaged = xevent.type == GraphicsExpose;
	}
	else
		w->w_bw->bw_last_rop_was_damaged = FALSE;
	X_UPDATE(w);
#endif /* X11 */
#ifdef SUNVIEW
	w->w_bw->bw_can_refresh = FALSE;
	pw_copy(w->w_pw, new_x, new_y, width,height,
						_wn_Roptab[R_RPL], w->w_pw, x, y);
	w->w_bw->bw_can_refresh = TRUE;
#endif /* SUNVIEW */
	SC_REDRAW(wn);
}

#define ABS(x) ((x) > 0 ? (x) : -(x))

void
wn_invert_line(wn, x1, y1, x2, y2)
int wn;
int x1, y1, x2, y2;
{
	register swin_t *w = WN_TO_W(wn);

	W_CHECK(wn);
	SC_UNDRAW(wn);
	ADJ_COORDS(w, x1, y1);
	ADJ_COORDS(w, x2, y2);
#ifdef X11
	XSetState(wn__Dpy, W_GC(w), W_FG_PIXEL(w), W_BG_PIXEL(w),
				_wn_Roptab[R_XOR], W_PLANEMASK(w));
	XDrawLine(wn__Dpy, w->w_win, W_GC(w), x1, y1, x2, y2);
#endif /* X11 */
#ifdef X11
	X_UPDATE(w);
#endif /* X11 */
#ifdef SUNVIEW
	w->w_bw->bw_can_refresh = FALSE;

	if (w->w_pw->pw_pixrect->pr_depth != 1)
		pw_putattributes(w->w_pw, &W_PLANEMASK(w));
	pw_vector(w->w_pw, x1, y1, x2, y2, PIX_SRC ^ PIX_DST, 1);
	if (w->w_pw->pw_pixrect->pr_depth != 1)
		pw_putattributes(w->w_pw, &w->w_bw->bw_planes);

	w->w_bw->bw_can_refresh = TRUE;
#endif /* SUNVIEW */
	SC_REDRAW(wn);
}

void
wn_draw_line(wn,x1,y1,x2,y2,colour)
int wn;
int x1,y1,x2,y2,colour;
{
	register swin_t *w = WN_TO_W(wn);
	int last_y, cap;

	cap = wn_text_capture(0, 0, &last_y);
	if (cap)
	{
	  char *dashes = "----------------------------------------";
	  int ft_wd, ft_ht, adjusted_y, y_offset;


	  if (cap == 2)
	    return;		/* no painting at all */

	  ft_wd = wn_get_sysfont()->ft_width;
	  ft_ht = wn_get_sysfont()->ft_height;
	  y_offset = (*Get_cur_posn_func)()%ft_ht;
	  adjusted_y = (int)(y1 + (int)ft_ht/2)/ft_ht * ft_ht - y_offset;
	  wn_text_capture(1, 0, &adjusted_y);	/* set last y */
	  (*Overlay_insert_string_func)
	    (dashes, (int)x1/ft_wd, (int)(x2-x1)/ft_wd,
	     (last_y != adjusted_y), 0, 0);
	  return;
	}

	if (colour == WN_INVERT) {
		wn_invert_line(wn, x1, y1, x2, y2);
		return;
	}

	W_CHECK(wn);
	ADJ_COORDS(w, x1, y1);
	ADJ_COORDS(w, x2, y2);
	SC_UNDRAW(wn);
#ifdef X11
#ifdef X11
	XSetState(wn__Dpy, W_GC(w), PIX(w,colour), W_BG_PIXEL(w),
					_wn_Roptab[R_RPL], AllPlanes);
	XDrawLine(wn__Dpy, w->w_win, W_GC(w), x1, y1, x2, y2);
#endif /* X11 */
	X_UPDATE(w);
#endif /* X11 */
#ifdef SUNVIEW
	w->w_bw->bw_can_refresh = FALSE;
	pw_vector(w->w_pw, x1, y1, x2, y2, PIX_SRC, colour);
	w->w_bw->bw_can_refresh = TRUE;
#endif /* SUNVIEW */
	SC_REDRAW(wn);
}
	
/*   Move an area (x,y,width,height) of the window to (new_x,new_y) and set
 *   the vacated area to colour. Useful for scrolling
 */
void
wn_move_area(wn, x, y, width, height, new_x, new_y, colour)
int wn;
int x, y, width, height, new_x, new_y, colour;
{
	int dist, new;
	
	SC_UNDRAW(wn);
	wn_rop(wn, x, y, width, height, new_x, new_y);
	if (x != new_x) {
		dist  = x - new_x;
		new = (dist < 0) ? x : x + width - dist;
		wn_set_area(wn, new, y, ((dist < 0) ? -dist : dist), height, colour);
	}
	if (y != new_y) {
		dist  = y - new_y;
		new = (dist < 0) ? y : y + height - dist;
		wn_set_area(wn, x, new, width, ((dist<0) ? -dist : dist), colour);
	}
	SC_REDRAW(wn);
}

void
wn_set_area(wn,x,y,width,height,colour)
int wn;
int x,y,width,height,colour;
{
	swin_t *w;
	int last_y;

	if (wn_text_capture(0, 0, &last_y))
	  return;

	if (colour == WN_INVERT) {
		wn_invert_area(wn, x, y, width, height);
		return;
	}
	W_CHECK(wn);
	w = WN_TO_W(wn);
	ADJ_COORDS(w, x, y);
	SC_UNDRAW(wn);
#ifdef X11
	XSetState(wn__Dpy, W_GC(w), PIX(w,colour), W_BG_PIXEL(w),
					_wn_Roptab[R_RPL], AllPlanes);
	XFillRectangle(wn__Dpy, w->w_win, W_GC(w), x, y,
						(xdim_t)width, (xdim_t)height);
	X_UPDATE(w);
#endif /* X11 */
#ifdef SUNVIEW
	pw_rop(w->w_pw, x, y, width,height, PIX_SRC | PIX_COLOR(colour),
						(struct pixrect *)NULL, 0, 0);
#endif /* SUNVIEW */
	SC_REDRAW(wn);
}

/*   draw rectangle with origin x-1,y-1, width width+2, height height+2
 */
void
wn_box_round(wn,x,y,width,height,colour)
int wn;
int x,y,width,height,colour;
{
	SC_UNDRAW(wn);
	wn_draw_line(wn,	x-1,	 y-1,	   x+width-1, y-1,        colour);
	wn_draw_line(wn,	x+width, y-1,	   x+width,   y+height-1, colour);
	wn_draw_line(wn,	x+width, y+height, x,  	      y+height,   colour);
	wn_draw_line(wn,	x-1,	 y+height, x-1,       y,          colour);
	SC_REDRAW(wn);
}

/*  Like wn_box_round(), but draw inverting lines - see wn_invert_line().
 */
void
wn_invert_box(wn, x, y, width, height)
int wn;
int x, y, width, height;
{
	wn_invert_line(wn,	x-1,	 y-1,	   x+width-1, y-1	);
	wn_invert_line(wn,	x+width, y-1,	   x+width,   y+height-1);
	wn_invert_line(wn,	x+width, y+height, x,  	      y+height	);
	wn_invert_line(wn,	x-1,	 y+height, x-1,       y		);
}

void
wn_set_cur_posn_func(func)
wn_get_cur_posn_func_t func;
{
	Get_cur_posn_func = func;
}

static 
void 
default_overlay_insert_string_func(text, pos, len, prepend_newline,
				   fg_color, var_coloring_only)
const char *text;
int pos;
int len;
int prepend_newline;
int fg_color;
int var_coloring_only;
{
}

void
wn_set_overlay_insert_string_func(func)
wn_overlay_insert_string_func_t func;
{
	Overlay_insert_string_func = func;
}

void
wn_overlay_insert_string(text, pos, len, prepend_newline,
			 fg_color, var_coloring_only)
const char *text;
int pos;
int len;
int prepend_newline;
int fg_color;
int var_coloring_only;
{
  (*Overlay_insert_string_func)(text, pos, len, prepend_newline,
				fg_color, var_coloring_only);
}
