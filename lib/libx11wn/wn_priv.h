/* wn_priv.h - private header file for the wn library */

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


/* @(#)wn_priv.h	1.17 21 Apr 1994 (UKC) */

#ifdef __STDC__
#define PROTO(a)	a
#else
#define PROTO(a)	()
#define const
#endif

#define TRUE	1
#define FALSE	0
typedef int bool;

#ifndef NULL
#define NULL 0
#endif

#ifdef X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef unsigned long xpixel_t;
typedef unsigned long planemask_t;
typedef unsigned xdim_t;

extern int _Xdebug;
#endif /* X11 */

#ifdef SUNVIEW
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/pixfont.h>
#include <sunwindow/defaults.h>
#include <sunwindow/rect.h>
#include <sunwindow/rectlist.h>
#include <sunwindow/pixwin.h>
#include <sunwindow/win_input.h>
#include <sunwindow/win_cursor.h>
#include <sunwindow/win_struct.h>
#include <sys/file.h>
struct pixrect *mem_create();
typedef int planemask_t;
#endif /* SUNVIEW */

#ifndef FD_SET
typedef int my_fd_set;
#define fd_set	my_fd_set
#undef FD_CLR
#undef FD_ISSET
#undef FD_ZERO
#define FD_SET(n, p)	(*(p) |= (1 << (n)))
#define FD_CLR(n, p)	(*(p) &= ~(1 << (n)))
#define FD_ISSET(n, p)	(*(p) & (1 << (n)))
#define FD_ZERO(p)	(*(p) = 0)
#endif

/*  Structure describing a special cursor.  Used only in wn_sc.c
 */
typedef struct scst {
	unsigned short sc_rflags; /* Not sc_flags: name clash on HPUX */
	void (*sc_func)PROTO((int wn, ...));
	short sc_x1;
	short sc_y1;
	short sc_x2;
	short sc_y2;
	short sc_cx1;
	short sc_cy1;
	short sc_cx2;
	short sc_cy2;
	short sc_last_x;
	short sc_last_y;
} sc_t;


/* (IDE) If we have two screens we put the color into all of the
 * colormaps.  This is because UPS code caches its colours without
 * much regard to the window, and doing this gets better results
 * (but only if all the colomaps started in a similar state).
 */
#define WN_DO_ALL_CMAPS 1
/*
*/
#if WANT_MULTI_SCREEN
#define PIX(w,p)	_wn_get_x_pixel(W_SCRP(w),(xpixel_t)p)
#else
#define PIX(w,p)	((xpixel_t)p)
#endif

/*  Per screen structure.
 */
typedef struct screenst {
	struct screenst *s_next;
	int s_screen;
	planemask_t s_planemask;
#ifdef X11
	xpixel_t s_fg_pixel, s_bg_pixel;
	Colormap s_cmap;
#if WANT_MULTI_SCREEN
	int s_npixel;
	xpixel_t *s_pixels;
#endif
	Pixmap s_grey_pixmap;
	unsigned short s_last_bitmap[16];
#endif /* X11 */
#ifdef SUNVIEW
	static struct pixrect *s_grey_ixmap;
#endif /* SUNVIEW */
} screen_t;
screen_t * _wn_get_screen PROTO((int s_screen));
screen_t * _wn_first_screen PROTO((void));
xpixel_t _wn_get_x_pixel PROTO((screen_t *scr, xpixel_t pixel));

/*  Structure describing a base window (as opposed to a subwindow)
 */
typedef struct bwinst {
#ifdef X11
	struct swinst *bw_assocw;
	GC    bw_gc;
	GC    bw_gc_noclip;
	short bw_is_icon;
	short bw_is_mapped;
	short bw_last_rop_was_damaged;
#endif /* X11 */
#ifdef SUNVIEW
	short bw_resized;
	short bw_can_refresh;
	int bw_planes;
#endif /* SUNVIEW */
	const char *bw_name;
	bitmap_t *bw_cursor;
	screen_t *bw_screen;
	cursor_t bw_cid;
	short bw_inmode;
	short bw_upd_level;
	unsigned long bw_winch_event;
	short bw_had_lost_selection_event;
	short bw_mouse_x;
	short bw_mouse_y;
	short bw_buttons;
	short bw_char;
	void (*bw_draw_icon_func)PROTO((int wn));
	int (*bw_deiconise_func)PROTO((int wn));
	sc_t bw_scbuf;
	int bw_have_sc;
	int bw_sc_drawn;
} bwin_t;

typedef struct swinst {
	bwin_t *w_bw;
#ifdef X11
	Window w_win;
	Region w_clip;
#endif /* X11 */
#ifdef SUNVIEW
	struct pixwin *w_pw;
#endif /* SUNVIEW */
	long w_user_data;
	short w_wn;
	short w_type;	/* WN_OUTPUT_ONLY or WN_INPUT_OUTPUT */
	short w_x_offs;
	short w_y_offs;
	short w_width;
	short w_height;
#ifndef xwindowsNOTYET
	struct swinst *w_next;
	struct swinst *w_base;
	struct swinst *w_parent;
	struct swinst *w_child;
#endif /* !xwindowsNOTYET */
} swin_t;

void     _wn_set_x_gc PROTO((bwin_t* bw));

#define WN_TO_W(wn)		(_wn_Windowtab[wn])

#define IS_BASEWIN(w)		((w)->w_parent == NULL)

/*  Under X windows and Suntools, the system handles coordinate adjustment
 *  for us.  On other systems, we must do it.
 */
#if (defined(X11) && defined(XSUBWINS)) || (defined(SUNVIEW) && defined(SUNSUBWINS))
#define ADJ_COORDS(w,x,y)	/* No action */
#else
#define ADJ_COORDS(w,x,y)	{ x += w->w_x_offs; y += w->w_y_offs; }
#endif

#ifdef X11
extern Display *wn__Dpy;

#define REQUEST_MASK (KeyPressMask | ButtonPressMask | ButtonReleaseMask | \
			PointerMotionMask | \
			ExposureMask | EnterWindowMask | LeaveWindowMask | \
			StructureNotifyMask)

#define ICON_MASK	(ButtonPressMask | ExposureMask)

#define MENU_ID(w)	((w)->w_win)
#define X_UPDATE(w)	{ if ((w)->w_bw->bw_upd_level==0) wn__do_xflush(); }

#define W_SCRP(w)	((w)->w_bw->bw_screen)
#define W_SCREEN(w)	((w)->w_bw->bw_screen->s_screen)
#define W_GC(w)		((w)->w_bw->bw_gc)
#define W_GC_NOCLIP(w)	((w)->w_bw->bw_gc_noclip)
#define W_CMAP(w)	((w)->w_bw->bw_screen->s_cmap)
#define W_PLANEMASK(w)	((w)->w_bw->bw_screen->s_planemask)
#if WANT_MULTI_SCREEN
#define W_FG_PIXEL(w)	_wn_get_x_pixel(W_SCRP(w),(xpixel_t)WN_FG(0))
#define W_BG_PIXEL(w)	_wn_get_x_pixel(W_SCRP(w),(xpixel_t)WN_BG(0))
#else
#define W_FG_PIXEL(w)	((w)->w_bw->bw_screen->s_fg_pixel)
#define W_BG_PIXEL(w)	((w)->w_bw->bw_screen->s_bg_pixel)
#endif
#define W_GREYPIXMAP(w)	((w)->w_bw->bw_screen->s_grey_pixmap)
#define W_LAST_BM(w)	((w)->w_bw->bw_screen->s_last_bitmap)
#endif /* X11 */

#ifdef SUNVIEW
#define MENU_ID(w)	((w)->w_pw)
#endif /* SUNVIEW */


/*  Macros for special cursor stuff.
 */
void _wn_sc_refresh PROTO((int wn, int x, int y));
void _wn_sc_undraw PROTO((int wn));
void _wn_sc_redraw PROTO((int wn));
Pixmap bm_to_pixmap PROTO((bitmap_t *bm));

#define SC_REFRESH(wn,x,y)	{ if (WN_TO_W(wn)->w_bw->bw_have_sc) \
							_wn_sc_refresh(wn,x,y); }
#define SC_MUST_UNDRAW(wn)	(WN_TO_W(wn)->w_bw->bw_sc_drawn)
#define SC_UNDRAW(wn)		{ if (WN_TO_W(wn)->w_bw->bw_sc_drawn) \
								_wn_sc_undraw(wn); }
#define SC_REDRAW(wn)		{ if (WN_TO_W(wn)->w_bw->bw_have_sc) \
								_wn_sc_redraw(wn); }

extern int _wn_Nwin;
extern int wn_NRootwin;

#define BADWIN(wn)		((wn)<0 || (wn) >= _wn_Nwin || WN_TO_W(wn) == NULL)

extern char wn__Badmesg[];

#define W_CHECK(wn)		{ if (BADWIN(wn)) wn__panic(wn__Badmesg); }

/*  The table of open windows.
 */
extern swin_t **_wn_Windowtab;
extern swin_t** wn_RootWintab;

/*  The table translating wn rasterop functions to machine dependent ones.
 */
extern int _wn_Roptab[];

/*  Structure to save a bitmap format in.
 */
typedef struct formatst {
	short fm_used;
	char fm_bit_order;
	char fm_byte_order;
	char fm_pixel_format;
	int fm_lineinc;
} format_t;


/*  Flag bits in bm_pflags
 */
#define B_LINEINC_OK		0x1	/* bm_lineinc ok for direct machine use */
#define B_BIT_ORDER_OK		0x2	/* bit order ok for direct machine use */
#define B_DISP_BYTE_ORDER	0x4	/* byte order ok for display */
#define B_XROP_BYTE_ORDER	0x8	/* byte order ok for our rasterop */
#define B_PIXEL_FORMAT_OK      0x10	/* pixel format ok for direct machine use */
#define B_DATA_ALIGNED	       0x20	/* bm_data address suitably aligned */

#define COMMON_FLAGS		(B_DATA_ALIGNED | B_LINEINC_OK | \
					B_BIT_ORDER_OK | B_PIXEL_FORMAT_OK)

#define DISP_OK		(COMMON_FLAGS | B_DISP_BYTE_ORDER)
#define XROP_OK		(COMMON_FLAGS | B_XROP_BYTE_ORDER)

#define FORMAT_WRONG(bm, flags)	(((bm)->bm_pflags & (flags)) != (flags))

void _wn_set_machine_format PROTO((bitmap_t *bm, format_t *fm, int flags));
void _wn_restore_format PROTO((bitmap_t *bm, format_t *fm));

#define SAVE_FORMAT(bm, format, flags)	\
		{ \
			if (FORMAT_WRONG(bm, flags)) \
				_wn_set_machine_format(bm, &format, flags); \
			else \
				format.fm_used = FALSE; \
		}

#define RESTORE_FORMAT(bm, format) \
		{ \
			if (format.fm_used && !((bm)->bm_flags & BM_CHOOSE_FORMAT)) \
				_wn_restore_format(bm, &format); \
		}
