/* wn_event.c - input event processing */

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


/* @(#)wn_event.c	1.46 25 May 1995 (UKC) */
char wn_wn_event_c_rcsid[] = "$Id$";

#if defined(__svr4__) || defined(__SVR4)
#define SVR4 1
#endif

#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>

#if defined(SVR4)
#define USE_STREAMS
#endif

#ifdef USE_STREAMS
#include <stropts.h>
#include <sys/conf.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#ifdef __STDC__
#include <unistd.h>
#endif

#ifdef X11
#include <local/menu3.h>
#endif

#include "wn.h"
#include "wn_priv.h"
#include "wn_init.h"  /* RCB, for process_client_message */
#include "wn_sc.h"
#include "wn_win.h"
#include "wn_misc.h"
#include "wn_event.h"
#include "wn_replay.h"

#ifdef X11
#include <X11/keysym.h>
#endif

#if defined(__sun__) && defined(SVR4) && !defined(SA_RESTART)
#define SA_RESTART	0x00000004
#endif

#define MAX( x, y ) (( x > y ) ? x : y)
#define MIN( x, y ) (( x < y ) ? x : y)

typedef void (*Sigfunc)PROTO((int));

static int get_event PROTO((int wn, event_t *ev, int *p_xyset));
static int select_on_mask PROTO((long win_mask, int sample, long *p_resmask));
static void set_event_win PROTO((event_t *ev));
static void catch_sigio PROTO((int ignored_sig));
static void set_intr_event_mask PROTO((swin_t *w));
static void set_normal_event_mask PROTO((swin_t *w));

#ifdef SUNVIEW
static void repair_window PROTO((swin_t *w));
static swin_t *sun_new_size PROTO((void));
static void sun_to_wn_event PROTO((Event *sun_event, event_t *ev));
#endif /* SUNVIEW */

#ifdef X11
static bool event_wanted PROTO((Display *display, XEvent *x_event, char *arg));
static void set_event_fields PROTO((event_t *ev, int x, int y,
				    unsigned xbstate, unsigned xbflipped));
#endif
static Sigfunc safe_signal PROTO((int sig, Sigfunc func));

/*  Space for a single pushed back event.
 *  See wn_pushback_event() and wn_next_event().
 */
static event_t Pushedback_event;
static wn_iconify_notify_funct icon_notify_funct;

/*  Timeout when doing input in sample mode.  Set by wn_set_input_timeout().
 *  Default is no timeout (e.g. wait indefinitely).
 */
static struct timeval *Request_mode_timeout = NULL;

#ifdef X11
/*  See extern declaration in wn_event.h.
 */
Window wn__Last_event_window;

/* FIXME: need to go through the stupid contortions to find out which
 *        modifier ALT is on this keyboard.
 */
static unsigned Alt_mask = Mod1Mask;

/*  Multi click stuff by Russ Browne (RCB),
**  Applied MicroSystems Corp.
**  russ@amc.com
*/

#define DEFAULT_MULTICLICK_TIME 250	/* Match xterm default */
static int multiClickTime = DEFAULT_MULTICLICK_TIME;
static int numClicks;
static long lastButtonTime;
static int  lastButtonsFlags;
#endif

/*  BUG: this typedef means the user can only set the first 32 fds.
 */
typedef long fdmask_t;

#define MAX_FDS		(sizeof(fdmask_t) * 8)

static long Wn_fd_mask = 0;
static long User_fd_mask = 0;

#ifndef __FDS_BITS
#define __FDS_BITS(set)	((set)->fds_bits)
#endif

#ifdef SUNVIEW
swin_t *_wn_Fdtab[MAX_FDS];
static int Wakeup_fd = -1;

void
_wn_set_sunview_wakeup_fd(fd)
int fd;
{
	Wakeup_fd = fd;
}
#endif /* SUNVIEW */

void
_wn_change_wn_fdmask(fd)
int fd;
{
	Wn_fd_mask ^= 1 << fd;
}

fdmask_t
wn_get_wn_fds()
{
	return Wn_fd_mask;
}

fdmask_t
wn_get_fd_mask()
{
	return User_fd_mask;
}

void
wn_set_fd_mask(mask)
fdmask_t mask;
{
	User_fd_mask = mask;
}

long
wn_set_input_timeout(msec)
long msec;
{
	static long last_msec = 0;
	long res;

	if (msec == 0) {
		Request_mode_timeout = NULL;
	}
	else {
		static struct timeval tvbuf;

		tvbuf.tv_sec = msec / 1000;
		tvbuf.tv_usec = (msec % 1000) * 1000;
		Request_mode_timeout = &tvbuf;
	}

	res = last_msec;
	last_msec = msec;
	return res;
}

static int
select_on_mask(win_mask, sample, p_resmask)
fdmask_t win_mask;
int sample;
fdmask_t *p_resmask;
{
	static int first_call = TRUE;
	static struct timeval zero_timeout = { 0, 0 };
	struct timeval *timeout;
	fd_set mask;
	int sel_res;

	if (first_call) {
		if (getenv("TIMEOUT") != NULL)
			zero_timeout.tv_usec = atoi(getenv("TIMEOUT")) * 1000;
		first_call = FALSE;
	}
		
	timeout = sample ? &zero_timeout : Request_mode_timeout;

	for (;;) {
		struct timeval tvbuf, *stm;

		FD_ZERO(&mask);
		__FDS_BITS(&mask)[0] = win_mask | User_fd_mask;
#ifdef SUNVIEW
		if (Wakeup_fd != -1)
			FD_SET(Wakeup_fd, &mask);
#endif

		/*  Beware of select modifying the timeout parameter.
		 */
	 	if (timeout != NULL) {
			 tvbuf = *timeout;
			 stm = &tvbuf;
		}
		else {
			stm = timeout;
		}
			
		sel_res = select(32, &mask,(fd_set *)NULL,(fd_set *)NULL, stm);
		switch(sel_res) {
		case -1:
			if (errno != EINTR)
				wn__panic("select failed in wn_next_event");
			return EV_INTERRUPT;
		case 0:
			if (timeout == Request_mode_timeout)
				return EV_INTERRUPT;
			return EV_MOUSE_MOVED;
		default:
#ifdef SUNVIEW
			if (Wakeup_fd != -1 && FD_ISSET(Wakeup_fd, &mask)) {
				char junkch;

				read(Wakeup_fd, &junkch, 1);
				FD_CLR(Wakeup_fd, &mask);
				if (wn__Lost_selection)
					return EV_OTHER;
			}
#endif
			if (__FDS_BITS(&mask)[0] & win_mask) {
				*p_resmask = __FDS_BITS(&mask)[0] & win_mask;
				return EV_OTHER;
			}
			if (__FDS_BITS(&mask)[0] & User_fd_mask) {
				*p_resmask = __FDS_BITS(&mask)[0] & User_fd_mask;
				return EV_OTHER_INPUT;
			}
		}
	}
	/* NOTREACHED */
}

#ifdef SUNVIEW
static int Got_sigwinch = FALSE;

/*  Refresh a window's contents from its backing pixrect.
 */
static void
repair_window(w)
swin_t *w;
{
	if (w->w_bw->bw_upd_level != 0)
		pw_batch_off(w->w_pw);
	pw_damaged(w->w_pw);
	pw_repairretained(w->w_pw);
	pw_donedamaged(w->w_pw);
	if (w->w_bw->bw_upd_level != 0)
		pw_batch_on(w->w_pw);
}

void
_wn_catch_sigwinch()
{
	register swin_t **wt;

	for (wt = _wn_Windowtab; wt < _wn_Windowtab + _wn_Nwin; wt++)
		if (*wt != NULL && IS_BASEWIN(*wt) && (*wt)->w_bw->bw_can_refresh)
			repair_window(*wt);
	Got_sigwinch = TRUE;
}

static swin_t *
sun_new_size()
{
	Rect r;
	swin_t **wt, *resized_w;
	register swin_t *w;

	resized_w = NULL;
	for (wt = _wn_Windowtab; wt < _wn_Windowtab + _wn_Nwin; wt++) {
		w = *wt;
		if (w == NULL || !IS_BASEWIN(w))
			continue;
		repair_window(w);
		win_getsize(w->w_pw->pw_windowfd,&r);
		if (r.r_width != w->w_width || r.r_height != w->w_height) {
			_wn_make_retained_pixrect(w->w_pw, r.r_width, r.r_height);
			w->w_width = r.r_width;
			w->w_height = r.r_height;
			w->w_bw->bw_resized = TRUE;
			resized_w = w;
		}
	}
	return resized_w;
}

static void
sun_to_wn_event(sun_event,ev)
register Event *sun_event;
register event_t *ev;
{
	int flipped;

	ev->ev_x = event_x(sun_event);
	ev->ev_y = event_y(sun_event);
	flipped = 0;
	switch(sun_event->ie_code) {
	case LOC_WINEXIT:
		ev->ev_buttons = 0;
		ev->ev_type = EV_WINDOW_DESELECTED;
		break;
	case LOC_WINENTER:
		ev->ev_type = EV_WINDOW_SELECTED;
		break;
	case LOC_MOVE:
	case LOC_STILL:
		ev->ev_type = EV_MOUSE_MOVED;
		break;
	case MS_LEFT:
		flipped = B_LEFT;
		break;
	case MS_MIDDLE:
		flipped = B_MIDDLE;
		break;
	case MS_RIGHT:
		flipped = B_RIGHT;
		break;
	default:
		ev->ev_type = event_is_ascii(sun_event) ? EV_KEY : EV_OTHER;
		if (ev->ev_type == EV_KEY)
			ev->ev_char = sun_event->ie_code;
		break;
	}
	
	if (flipped != 0) {
		ev->ev_flags = flipped;
		if (win_inputnegevent(sun_event)) {
			ev->ev_type = EV_BUTTON_UP;
			ev->ev_buttons &= ~flipped;
		}
		else {
			ev->ev_type = EV_BUTTON_DOWN;
			ev->ev_buttons |= flipped;
		}

		ev->ev_buttons &= ~(B_SHIFT_MASK | B_CONTROL_MASK);
		if (event_shift_is_down(sun_event))
			ev->ev_buttons |= B_SHIFT_MASK;
		if (event_ctrl_is_down(sun_event))
			ev->ev_buttons |= B_CONTROL_MASK;
	}
}

/* sun_get_event()
 */
static void
get_event(wn, ev, p_xyset)
int wn;
register event_t *ev;
int *p_xyset;
{
	Event sun_event;
	int sample;
	fdmask_t mask, resmask;
	register int fd;
	register swin_t *w;
	swin_t *resized_w;

	if (wn == WN_ANY) {
		w = NULL;
		sample = FALSE;
		mask = Wn_fd_mask;
	}
	else {
		W_CHECK(wn);
		w = WN_TO_W(wn)->w_base;
		sample = w->w_bw->bw_inmode == WN_SAMPLE;
		mask = 1 << w->w_pw->pw_windowfd;
	}

	if (Got_sigwinch) {
		resized_w = sun_new_size();
		Got_sigwinch = FALSE;
	}
	else
		resized_w = NULL;

	if (resized_w != NULL) {
		resized_w->w_bw->bw_resized = FALSE;
		ev->ev_wn = resized_w->w_wn;
		ev->ev_type = EV_WINDOW_RESIZED;
		return;
	}

	if (wn__Lost_selection) {
		ev->ev_type = EV_LOST_SELECTION;
		wn__Lost_selection = FALSE;
	} else
	{
		ev->ev_type = select_on_mask(mask, sample, &resmask);
	}
	if (wn__Lost_selection) {
		ev->ev_type = EV_LOST_SELECTION;
		wn__Lost_selection = FALSE;
	}

	if (w != NULL)
		fd = w->w_pw->pw_windowfd;
	else {
		if (ev->ev_type == EV_OTHER) {
			for (fd = 0; (resmask & 1) == 0; resmask >>= 1, ++fd)
				;
			w = _wn_Fdtab[fd];
		}
		else {
			w = WN_TO_W(WN_STDWIN);
			fd = w->w_pw->pw_windowfd;
		}
	}
	ev->ev_wn = w->w_wn;

	ev->ev_buttons = w->w_bw->bw_buttons;
	if (ev->ev_type == EV_INTERRUPT && Got_sigwinch)
		ev->ev_type = 0;
	if (ev->ev_type == EV_OTHER) {
		if (input_readevent(fd, &sun_event) == 0) {
			sun_to_wn_event(&sun_event,ev);
			*p_xyset = TRUE;
		}
		else if (errno != EINTR)
			wn__panic("input_readevent failed in get_event");
	}
	else if (ev->ev_type == EV_OTHER_INPUT)
		ev->ev_fdmask = resmask;
}
#endif /* SUNVIEW */

/*  This and the following function are hacks for ICL guide.
 *  See get_event below.
 */
static int Self_deiconise_enabled = FALSE;

void
_wn_enable_self_deiconise()
{
	Self_deiconise_enabled = TRUE;
}

#ifdef X11
#define SAMPLE_MASK	((REQUEST_MASK) & ~(PointerMotionMask))

void
wn_set_MultiClickTime()
{
   /*
   **   We recognize both
   **	  MultiClickTime     = time in ms, as in xterm
   **	  MultiClickTimeout  = time in 1/10 sec, as in openwin
   */
   int scale = 1;
   int timeMs = 0;
   const char* timeSpec = wn_get_default("MultiClickTime");
   if ( timeSpec == NULL )
   {
	timeSpec = wn_get_default("MultiClickTimeout");
	scale = 100;
   }
   if ( timeSpec )
	timeMs = scale*atoi(timeSpec);

   if ( timeMs > 0 )
     multiClickTime = timeMs;		/* RCB */
}

static bool
event_wanted(display, x_event, arg)
Display *display;
XEvent *x_event;
char *arg;
{
	return ((XAnyEvent *)x_event)->window == (Window)arg ||
	       x_event->type == SelectionRequest ||
	       x_event->type == SelectionClear;
}

/* x_get_event()
 */
static int
get_event(wn, ev, p_xyset)
int wn;
register event_t *ev;
int *p_xyset;
{
	static swin_t *last_w;
	XEvent x_event;
	XKeyEvent *xk;
	XButtonEvent *xb;
	int sample;
	/* UPS split windows. */
	int split = (wn_NRootwin>1);
	int old_width, old_height, xbuttons, nbytes;
	char ch;
	char *keystr;
	register swin_t *w;
	extern int _wn_Autoraise;

	if (wn == WN_ANY) {
		w = NULL;
		sample = FALSE;
	}
	else {
		W_CHECK(wn);
		w = WN_TO_W(wn)->w_base;
		sample = w->w_bw->bw_inmode == WN_SAMPLE;
	}

	if (!sample && QLength(wn__Dpy) == 0) {
		fdmask_t mask, resmask;

		mask = 1 << ConnectionNumber(wn__Dpy);
		ev->ev_type = select_on_mask(mask, sample, &resmask);
		if (ev->ev_type == EV_MOUSE_MOVED) {
			ev->ev_wn = w->w_wn;
			ev->ev_buttons = w->w_bw->bw_buttons;
			return TRUE;
		}
		if (ev->ev_type != EV_OTHER) {
			ev->ev_wn = WN_STDWIN;
			ev->ev_buttons = WN_TO_W(WN_STDWIN)->w_bw->bw_buttons;
			if (ev->ev_type == EV_OTHER_INPUT)
				ev->ev_fdmask = resmask;
			return TRUE;
		}
	}
	if (sample) {
	    if (!XCheckWindowEvent(wn__Dpy, w->w_win, REQUEST_MASK, &x_event)) {
	      ev->ev_type = EV_MOUSE_MOVED;
	      ev->ev_wn = w->w_wn;
	      ev->ev_buttons = w->w_bw->bw_buttons;
	      return TRUE;
	    }
	}
	else {
		if (w != NULL && !split) {
			Window win;

			if (w->w_bw->bw_is_mapped)
				win = w->w_win;
			else
				win = w->w_bw->bw_assocw->w_win;
			XIfEvent(wn__Dpy, &x_event, event_wanted, (char *)win);
		}
		else
			XNextEvent(wn__Dpy, &x_event);
	}

	if (((XAnyEvent *)&x_event)->window == wn__Last_event_window)
		w = last_w;
	else {
		extern swin_t **_wn_Windowtab;
		extern int _wn_Nwin;
		register swin_t **wp;

		for (wp = _wn_Windowtab; wp < _wn_Windowtab + _wn_Nwin; wp++)
			if (*wp != NULL &&
				    (*wp)->w_win == ((XAnyEvent *)&x_event)->window)
				break;
		if (wp == _wn_Windowtab + _wn_Nwin) {
			ev->ev_type = 0;
			return TRUE;
		}

		last_w = w = *wp;
		wn__Last_event_window = ((XAnyEvent *)&x_event)->window;
	}

	/* If this is a dummy key press event sent by us to prevent
	 * blocking then exit now indicating that we've run out of
	 * events to process.
	 */
	if (x_event.type == KeyPress &&
	    x_event.xkey.send_event &&
	    x_event.xkey.keycode == 0) {
		return FALSE;
	}

	ev->ev_wn = w->w_wn;
	ev->ev_buttons = w->w_bw->bw_buttons;
	if (w->w_bw->bw_is_icon) {
		swin_t *main_w;

		main_w = w->w_bw->bw_assocw;
		if (x_event.type == Expose)
			(*main_w->w_bw->bw_draw_icon_func)(w->w_wn);

		/*  This stuff is a hack for ICL guide.
		 */
		if (Self_deiconise_enabled && x_event.type == ButtonPress) {
			xbuttons = ((XButtonPressedEvent *)&x_event)->button;
			if (xbuttons & Button1) {
				XSelectInput(wn__Dpy, w->w_win, (unsigned long)0);
				(*main_w->w_bw->bw_deiconise_func)(main_w->w_wn);
				XSelectInput(wn__Dpy, w->w_win, ICON_MASK);
			}
		}

		ev->ev_type = EV_OTHER;
		return TRUE;
	}
	switch(x_event.type) {
	case EnterNotify:
		ev->ev_type = EV_WINDOW_SELECTED;
		ev->ev_time = ((XEnterWindowEvent *)&x_event)->time;
		ev->ev_x    = ((XEnterWindowEvent *)&x_event)->x;
		ev->ev_y    = ((XEnterWindowEvent *)&x_event)->y;
		*p_xyset = TRUE;
		if (_wn_Autoraise) {
			XRaiseWindow(wn__Dpy, w->w_win);
			XFlush(wn__Dpy);
		}
		break;
	case LeaveNotify:
		ev->ev_type = EV_WINDOW_DESELECTED;
		ev->ev_time = ((XLeaveWindowEvent *)&x_event)->time;
		ev->ev_x    = ((XLeaveWindowEvent *)&x_event)->x;
		ev->ev_y    = ((XLeaveWindowEvent *)&x_event)->y;
		*p_xyset = TRUE;
		break;
	case Expose:
#ifdef X11
		/*  Discard all except the last Expose event.
		 */
		ev->ev_xpos = ((XExposeEvent *)&x_event)->x;
		ev->ev_ypos = ((XExposeEvent *)&x_event)->y;
		ev->ev_width = ((XExposeEvent *)&x_event)->width;
		ev->ev_height = ((XExposeEvent *)&x_event)->height;

		for(;;) {
			XEvent peek_event;
         
			if (QLength(wn__Dpy) == 0)
				break;
			XPeekEvent(wn__Dpy, &peek_event);
			if (peek_event.type != Expose)
			{
				break;
			}
			else
			{
				/* set our redraw region */
				short x2pos = ev->ev_xpos + ev->ev_width;
				short y2pos = ev->ev_ypos + ev->ev_height;
				ev->ev_xpos = MIN( ((XExposeEvent *)&peek_event)->x, ev->ev_xpos );
				ev->ev_ypos = MIN( ((XExposeEvent *)&peek_event)->y, ev->ev_ypos );
				x2pos = MAX( ((XExposeEvent *)&peek_event)->x +
                               	     ((XExposeEvent *)&peek_event)->width, x2pos );
				ev->ev_width = x2pos - ev->ev_xpos;
				y2pos = MAX( ((XExposeEvent *)&peek_event)->y +
					     ((XExposeEvent *)&peek_event)->height, y2pos );
				ev->ev_height = y2pos - ev->ev_ypos;
			}
			XNextEvent(wn__Dpy, &x_event);
		}
#endif /* X11 */
		old_width = w->w_width;
		old_height = w->w_height;
		wn__update_size(w);
		if (w->w_width == old_width && w->w_height == old_height)
			ev->ev_type = EV_WINDOW_EXPOSED;
		else
			ev->ev_type = EV_WINDOW_RESIZED;
		break;
	case MotionNotify:
		/*  Discard all except the last MotionNotify event.
		 */
		for(;;) {
			XEvent peek_event;

			if (QLength(wn__Dpy) == 0)
				break;
			XPeekEvent(wn__Dpy, &peek_event);
			if (peek_event.type != MotionNotify)
				break;
			XNextEvent(wn__Dpy, &x_event);
		}
		ev->ev_type = EV_MOUSE_MOVED;
		ev->ev_time = ((XMotionEvent *)&x_event)->time;
		ev->ev_x    = ((XMotionEvent *)&x_event)->x;
		ev->ev_y    = ((XMotionEvent *)&x_event)->y;
		*p_xyset = TRUE;
		break;
	case ButtonPress:
		/* Monitor multiple clicks on ButtonPress
		** Then fall into ButtonRelease case ( RCB)
		*/
		xb = (XButtonPressedEvent *)&x_event;
		if (   numClicks < 3
		    && (xb->time - lastButtonTime)
			< multiClickTime)
			numClicks++;
		else
			numClicks = 0;
		/* fall through */
	case ButtonRelease:
		xb = (XButtonPressedEvent *)&x_event;
		lastButtonTime = ev->ev_time = xb->time;

		if (xb->button == 0){
			ev->ev_type = EV_MOUSE_MOVED;
		}
		else {
			ev->ev_type = (x_event.type == ButtonPress) ?
						EV_BUTTON_DOWN : EV_BUTTON_UP;
		}
		set_event_fields(ev, xb->x, xb->y, xb->state, xb->button);
		*p_xyset = TRUE;
		/*
		**  If the buttons changed, clear the click counter.
		**  The OR it into the ev_buttons field.
		*/
		if ( ev->ev_flags != lastButtonsFlags)
		   numClicks = 0;

		lastButtonsFlags = ev->ev_flags;
		ev->ev_buttons |= (numClicks<<B_CLICK_SHIFT);
		break;
	case KeyPress:
		xk = (XKeyEvent *)&x_event;
		keystr = &ch;
		nbytes = XLookupString(xk, keystr, 1,
					(KeySym *)NULL, (XComposeStatus *)NULL);
		if (nbytes == 0) {
			KeySym keysym;

			keysym = XLookupKeysym(xk, 0);
			nbytes = 1;
			switch (keysym) {
			case XK_Left:
				ch = WN_CH_LEFT_ARROW;
				break;
			case XK_Right:
				ch = WN_CH_RIGHT_ARROW;
				break;
			case XK_Up:
				ch = WN_CH_UP_ARROW;
				break;
			case XK_Down:
				ch = WN_CH_DOWN_ARROW;
				break;
			default:
				if (keysym >= XK_F1 && keysym <= XK_F35)
					ch = WN_CH_FUNCKEY((keysym + 1)-XK_F1);
				else
					nbytes = 0;
				break;
			}
		}
				
		ev->ev_type = (nbytes > 0) ? EV_KEY : EV_OTHER;
		ev->ev_time = xk->time;
		
		if (nbytes > 0) /* BUG: what if nbytes > 1 ? */
			ev->ev_char = *keystr;

		set_event_fields(ev, xk->x, xk->y, xk->state, 0);
		*p_xyset = TRUE;
		break;

	case SelectionRequest:
		wn__send_selection(&x_event);
		ev->ev_type = EV_OTHER;
		break;
	
	case SelectionClear:
		ev->ev_type = EV_LOST_SELECTION;
		break;

	case ClientMessage:
		process_client_message(&x_event, w->w_win, w->w_wn);
		ev->ev_type = EV_MOUSE_MOVED;
		break;
	case MapNotify:
		if (icon_notify_funct)
		   (*icon_notify_funct)(w->w_wn,FALSE);
		ev->ev_type = EV_OTHER;
		break;
	case UnmapNotify:
		if (icon_notify_funct)
		   (*icon_notify_funct)(w->w_wn,TRUE);
		ev->ev_type = EV_OTHER;
		break;
    
	default:
		ev->ev_type = EV_OTHER;
	}

	return TRUE;
}

static void
set_event_fields(ev, x, y, xbstate, xbflipped)
event_t *ev;
int x, y;
unsigned xbstate, xbflipped;
{
	int buttons, flipped, modflags;

	buttons = 0;
	if (xbstate & Button1Mask)
		buttons |= B_LEFT;
	if (xbstate & Button2Mask)
		buttons |= B_MIDDLE;
	if (xbstate & Button3Mask)
		buttons |= B_RIGHT;
	if (xbstate & Button4Mask)
		buttons |= B_BUTTON4;
	if (xbstate & Button5Mask)
		buttons |= B_BUTTON5;

	modflags = 0;
	if (xbstate & ShiftMask)
		modflags |= B_SHIFT_MASK;
	if (xbstate & ControlMask)
		modflags |= B_CONTROL_MASK;
	if (xbstate & Alt_mask)
		modflags |= B_ALT_MASK;

	switch(xbflipped) {
	case Button1:
		flipped = B_LEFT;
		break;
	case Button2:
		flipped = B_MIDDLE;
		break;
	case Button3:
		flipped = B_RIGHT;
		break;
	case Button4:
		flipped = B_BUTTON4;
		break;
	case Button5:
		flipped = B_BUTTON5;
		break;
	case 6:
		flipped = B_BUTTON6;
		break;
	case 7:
		flipped = B_BUTTON7;
		break;
	case 8:
		flipped = B_BUTTON8;
		break;
	case 9:
		flipped = B_BUTTON9;
		break;
	case 10:
		flipped = B_BUTTON10;
		break;
	default:
		flipped = 0;
		break;
	}
	
	ev->ev_x = x;
	ev->ev_y = y;
	ev->ev_buttons = (buttons ^ flipped) | modflags;
	ev->ev_flags = flipped;
}
#endif /* X11 */

int
wn_inmode(wn,mode)
int wn;
int mode;
{
	register swin_t *w = WN_TO_W(wn);
	int oldmode;

	W_CHECK(wn);
	oldmode = w->w_bw->bw_inmode;
	if (mode != oldmode)
		_wn_set_inmode(w, mode);
	return oldmode;
}

void
_wn_set_inmode(w, mode)
swin_t *w;
int mode;
{
#ifdef X11
	switch(mode) {
	case WN_REQUEST:
		XSelectInput(wn__Dpy, w->w_win, REQUEST_MASK);
		break;
	case WN_SAMPLE:
		/* XSelectInput(wn__Dpy, w->w_win, SAMPLE_MASK); */
		break;
	case WN_NOINPUT:
		XSelectInput(wn__Dpy, w->w_win, ExposureMask);
		break;
	case _WN_MININPUT:
		XSelectInput(wn__Dpy, w->w_win, NoEventMask);
		break;
	default:
		wn__panic("bad mode in _wn_set_inmode");
	}
	XSync(wn__Dpy, FALSE);
#endif /* X11 */
	w->w_bw->bw_inmode = mode;
}

static void
set_event_win(ev)
event_t *ev;
{
	register int x, y;
	register swin_t *w, *par;

	w = WN_TO_W(ev->ev_wn);
	x = ev->ev_x + w->w_x_offs;
	y = ev->ev_y + w->w_y_offs;
	par = w->w_base;
	for (;;) {
		for (w = par->w_child; w != NULL; w = w->w_next) {
			if (w->w_type == WN_INPUT_OUTPUT &&
			    x >= w->w_x_offs && y >= w->w_y_offs &&
			    x < w->w_x_offs + w->w_width &&
			    y < w->w_y_offs + w->w_height)
			break;
		}
		if (w == NULL) {
			ev->ev_wn = par->w_wn;
			ev->ev_x = x - par->w_x_offs;
			ev->ev_y = y - par->w_y_offs;
			return;
		}
		par = w;
	}
}

void
wn_warp_mouse(wn, x, y)
int wn;
int x, y;
{
	register swin_t *w;

	W_CHECK(wn);
	w = WN_TO_W(wn);
	ADJ_COORDS(w, x, y);
#ifdef X11
	XWarpPointer(wn__Dpy, w->w_win, w->w_win, 0, 0, 0, 0, x, y);
	X_UPDATE(w);
#endif /* X11 */
#ifdef SUNVIEW
	win_setmouseposition(w->w_pw->pw_windowfd, x, y);
#endif /* SUNVIEW */
}

static wn_event_handler_func_t Event_handler = NULL;

wn_event_handler_func_t
wn_interpose_event_handler(func)
wn_event_handler_func_t func;
{
	wn_event_handler_func_t old;

	old = Event_handler;
	Event_handler = func;
	return old;
}

void
wn_next_event(wn, mask, ev)
int wn;
int mask;
event_t *ev;
{
	register swin_t *w;
	register bwin_t *bw;
	int new_x, new_y, xyset;

	for (;;) {
		ev->ev_flags = 0;
		xyset = FALSE;
		
		if (Pushedback_event.ev_type != 0) {
			*ev = Pushedback_event;
			xyset = TRUE;
			Pushedback_event.ev_type = 0;
		}
		else {
			if (_wn_Replaying) {
				if (wn_get_recorded_event(ev) == 0)
					xyset = TRUE;
				else
					get_event(wn, ev, &xyset);
			}
			else
				get_event(wn, ev, &xyset);
		}

		if (ev->ev_type == 0)
			continue;

		w = WN_TO_W(ev->ev_wn);
		bw = w->w_bw;
		if (!xyset) {
			ev->ev_x = bw->bw_mouse_x + w->w_x_offs;
			ev->ev_y = bw->bw_mouse_y + w->w_y_offs;
		}

		if (_wn_Recording && (mask & ev->ev_type) != 0)
			wn_record_event(ev);

		new_x = ev->ev_x + w->w_x_offs;
		new_y = ev->ev_y + w->w_y_offs;

		if (wn == WN_ANY) {
			set_event_win(ev);
			w = WN_TO_W(ev->ev_wn);
		}
		else {
			/*  If the caller asked for a particular window
			 *  and the event is in the base window of the
			 *  requested window, the translate the event
			 *  into the requested window.
			 */
			if (ev->ev_wn != wn && w->w_base->w_wn == ev->ev_wn) {
				w = WN_TO_W(wn);
				ev->ev_wn = wn;
				ev->ev_x = new_x - w->w_x_offs;
				ev->ev_y = new_y - w->w_y_offs;
			}
		}

		if (new_x != bw->bw_mouse_x || new_y != bw->bw_mouse_y) {
			SC_REFRESH(w->w_wn,new_x,new_y);
			bw->bw_mouse_x = new_x;
			bw->bw_mouse_y = new_y;
		}
		bw->bw_buttons = ev->ev_buttons;

		if (ev->ev_type == EV_LOST_SELECTION)
		{
			wn_clr_xclipboard();
		}

		if ((mask & ev->ev_type) == 0)
			continue;

		if (ev->ev_type == EV_WINDOW_RESIZED)
			bw->bw_winch_event = EV_WINDOW_RESIZED;
		else if ((ev->ev_type == EV_WINDOW_EXPOSED) &&
			 bw->bw_winch_event == 0)
			bw->bw_winch_event = EV_WINDOW_EXPOSED;
		else if (ev->ev_type == EV_LOST_SELECTION)
		{
			bw->bw_had_lost_selection_event = TRUE;
			wn_clr_xclipboard();
		}

		if (Event_handler == NULL || (*Event_handler)(ev) == 0)
			break;
	}
}

int
wn_lost_selection(wn)
int wn;
{
	register swin_t *w = WN_TO_W(wn);
	bool res;

	W_CHECK(wn);
	res = w->w_bw->bw_had_lost_selection_event;
	w->w_bw->bw_had_lost_selection_event = FALSE;
	return res;
}

unsigned long
wn_get_resize_event(wn)
int wn;
{
	register swin_t *w = WN_TO_W(wn);
	int res;

	W_CHECK(wn);
	res = w->w_bw->bw_winch_event;
	w->w_bw->bw_winch_event = 0;
	return res;
}

/*  Push back an event.  We make the event window the main window
 *  to avoid problems with the event window being deleted between
 *  here and the next call of wn_next_event().
 */
void
wn_pushback_event(ev)
event_t *ev;
{
	swin_t *w;

	W_CHECK(ev->ev_wn);
	w = WN_TO_W(ev->ev_wn);
	if (ev->ev_type == EV_WINDOW_RESIZED)
		w->w_bw->bw_winch_event = EV_WINDOW_RESIZED;
	else if (ev->ev_type == EV_WINDOW_EXPOSED && w->w_bw->bw_winch_event == 0)
		w->w_bw->bw_winch_event = EV_WINDOW_EXPOSED;
	else if (ev->ev_type == EV_LOST_SELECTION)
	{
		w->w_bw->bw_had_lost_selection_event = TRUE;
		wn_clr_xclipboard();
	}
	ev->ev_x += w->w_x_offs;
	ev->ev_y += w->w_y_offs;
	ev->ev_wn = w->w_base->w_wn;
	if ((ev->ev_type & (EV_BUTTON_UP | EV_BUTTON_DOWN)) == 0)
		ev->ev_buttons = w->w_bw->bw_buttons;
	Pushedback_event = *ev;
}

/*   Return zero if the are no characters queued on the standard input,
 *   otherwise return non-zero and set *p_ch to the next character read.
 *   p_ch must be a *character* pointer.
 */
int
wn_getc(wn,p_ch)
int wn;
char *p_ch;
{
	register swin_t *w = WN_TO_W(wn);

	W_CHECK(wn);

	if (w->w_bw->bw_char == -1)
		return FALSE;
	*p_ch = w->w_bw->bw_char;
	w->w_bw->bw_char = -1;
	return TRUE;
}

#define GP_MASK (EV_KEY | EV_BUTTONS | EV_MOUSE_MOVED | EV_WINDOW_RESIZED | EV_WINDOW_EXPOSED | EV_INTERRUPT | EV_LOST_SELECTION)

void
wn_await_window_size_change(wn)
int wn;
{
	event_t evbuf;

	wn_next_event(wn, EV_WINDOW_RESIZED, &evbuf);
	(void) wn_get_resize_event(wn);
}

int
wn_getpuck(wn, p_xpos, p_ypos)
int wn;
int *p_xpos, *p_ypos;
{
	event_t evbuf;

	wn_next_event(wn, GP_MASK, &evbuf);
	if (evbuf.ev_type == EV_KEY)
		WN_TO_W(wn)->w_bw->bw_char = evbuf.ev_char;
	*p_xpos = evbuf.ev_x;
	*p_ypos = evbuf.ev_y;
	return evbuf.ev_buttons | evbuf.ev_type |
					(evbuf.ev_flags & EV_WAS_PUSHED_BACK);
}

void
wn_ungetpuck(wn, x, y, buttons)
int wn;
int x, y, buttons;
{
	event_t evbuf;

	W_CHECK(wn);
	evbuf.ev_flags = EV_WAS_PUSHED_BACK;
	evbuf.ev_type = EV_MOUSE_MOVED;
	evbuf.ev_x = x;
	evbuf.ev_y = y;
	evbuf.ev_buttons = buttons & (B_ANY | B_ANY_MODIFIER);
	evbuf.ev_wn = wn;
	wn_pushback_event(&evbuf);
}

void
wn_wait_for_release_of(wn, buttons)
int wn;
int buttons;
{
	event_t evbuf;
	
	W_CHECK(wn);
	do {
		wn_next_event(wn,  GP_MASK, &evbuf);
	} while (evbuf.ev_buttons & buttons);
}


static wn_abort_func_t Abort_func = NULL;
static swin_t *Abort_w;
static int Abort_pending;

static void
catch_sigio(unused_sig)
int unused_sig;
{
	if (Abort_pending) {
		Abort_pending = 0;
		Abort_func();
	}
}

/*  This function is based on the one in Stevens.
 */
static Sigfunc
safe_signal(sig, func)
int sig;
Sigfunc func;
{
	struct sigaction act, oact;

	/*  God knows what the actual type of sa_handler will be on any
	 *  given system ...
	 */
	*(Sigfunc *)&act.sa_handler = func;
	sigemptyset(&act.sa_mask);

#ifdef SA_RESTART
	act.sa_flags = SA_RESTART;
#else
	act.sa_flags = 0;
#endif

	if (sigaction(sig, &act, &oact) < 0)
		wn__panic("sigaction failed");

	return (Sigfunc)oact.sa_handler;
}

wn_abort_func_t
wn_set_abort_func(wn,func)
int wn;
wn_abort_func_t func;
{
	static void (*old_io_func)PROTO((int sig));
	wn_abort_func_t old_abort_func;
	int fd;
	swin_t *w;
	int flags;

	W_CHECK(wn);
	w = WN_TO_W(wn);
	old_abort_func = Abort_func;
	Abort_func = func;
	Abort_w = w;
	Abort_pending = 1;

#ifdef X11
	fd = ConnectionNumber(wn__Dpy);
#endif
#ifdef SUNVIEW
	fd = w->w_pw->pw_windowfd;
#endif

	if (func != NULL) {
		set_intr_event_mask(w);
		old_io_func = safe_signal(SIGIO, catch_sigio);
#ifndef __hp9000s800
#if defined(USE_STREAMS) && !defined(FASYNC)
		if (ioctl(fd, I_SETSIG, S_RDNORM) != 0) {
			perror("wn: ioctl I_SETSIG S_RDNORM");
			wn__panic("I_SETSIG S_RDNORM botch");
		}
#else
		if ((flags = fcntl(fd, F_GETFL, 0)) != -1) {
			flags |= FASYNC;
			if (fcntl(fd, F_SETFL, flags) != 0)
				wn__panic("FASYNC on botch");
		}
#endif
#endif /* __hp9000s800 */
	}
	else {
#ifdef FASYNC			/* RGA linux merge */
#ifndef __hp9000s800
#ifdef USE_STREAMS
		if (ioctl(fd, I_SETSIG, 0) != 0) {
			perror("wn: ioctl I_SETSIG 0");
			wn__panic("I_SETSIG 0 botch");
		}
#else
		if ((flags = fcntl(fd, F_GETFL, 0)) != -1) {
			flags &= ~FASYNC;
			if (fcntl(fd, F_SETFL, flags) != 0)
				wn__panic("FASYNC off botch");
		}
#endif
#endif /* __hp9000s800 */
		(void) safe_signal(SIGIO, old_io_func);
#endif /* RGA linux merge */
		set_normal_event_mask(w);
	}

	return old_abort_func;
}

static void
set_intr_event_mask(w)
swin_t *w;
{
#ifdef X11
	XSelectInput(wn__Dpy, w->w_win,
	 ButtonPressMask | ButtonReleaseMask | ExposureMask | StructureNotifyMask);
	XSync(wn__Dpy, FALSE);
#endif
#ifdef SUNVIEW
	Inputmask mask;
	
	input_imnull(&mask);
	mask.im_flags |= IM_ASCII|IM_NEGEVENT;
	win_setinputcodebit(&mask, MS_LEFT);
	win_setinputcodebit(&mask, MS_MIDDLE);
	win_setinputcodebit(&mask, MS_RIGHT);
	win_set_pick_mask(w->w_pw->pw_windowfd, &mask);
#endif
}

static void
set_normal_event_mask(w)
swin_t *w;
{
#ifdef X11
	XSelectInput(wn__Dpy, w->w_win,
		     (w->w_bw->bw_inmode == WN_SAMPLE) ? SAMPLE_MASK : REQUEST_MASK);
	XFlush(wn__Dpy);
#endif
#ifdef SUNVIEW
	wn__setmode(w->w_wn);
#endif
}

int
wn_expose_or_press_queued(wn, md, p_x, p_y, exposed, resized, e_x, e_y, e_w, e_h)
     int wn;
     int md;
     int *p_x;
     int *p_y;
     int *exposed;
     int *resized;
     int *e_x;
     int *e_y;
     int *e_w;
     int *e_h;
{
#ifdef X11
  XKeyEvent ev;
  swin_t *w;
  event_t event;
  int x, y, xyset, ret = 0;

  if (_wn_Windowtab)
  {
    /* first send a dummy event so we are guaranteed not to block */
    memset((char *)&ev, 0, sizeof(XKeyEvent));
    ev.type = KeyPress;
    ev.display = wn__Dpy;
    ev.window = WN_TO_W(wn)->w_win;
    XSendEvent(wn__Dpy, ev.window, False, 0L, (XEvent *)&ev);
    XFlush(wn__Dpy);
    *exposed = *resized = 0;

    /* read all events till we get the send event at the end */
    while (get_event(wn, &event, &xyset))
    {
      if (event.ev_type == EV_WINDOW_EXPOSED || event.ev_type == EV_WINDOW_RESIZED) {
	if (!*exposed && !*resized) {
	  *e_x = event.ev_xpos;
	  *e_y = event.ev_ypos;
	  *e_w = event.ev_width;
	  *e_h = event.ev_height;
	}
	else {
	  int x2pos = *e_x + *e_w;
	  int y2pos = *e_y + *e_h;

	  *e_x = MIN(*e_x, event.ev_xpos);
	  *e_y = MIN(*e_y, event.ev_ypos);
	  *e_w = MAX(x2pos, event.ev_xpos + event.ev_width) - *e_x;
	  *e_h = MAX(y2pos, event.ev_ypos + event.ev_height) - *e_y;
	}

	if (event.ev_type == EV_WINDOW_EXPOSED)
	  *exposed = 1;
	if (event.ev_type == EV_WINDOW_RESIZED)
	  *resized = 1;
      }
      else if (event.ev_type == EV_BUTTON_DOWN) {
	w = WN_TO_W(wn);
	x = event.ev_x - w->w_x_offs;
	y = event.ev_y - w->w_y_offs;

	if (Mgetmd(x, y, wn) == md)
	{
	  *p_x = x;
	  *p_y = y;
	  ret = 1;
	}
      }
    }
  }
#endif /* X11 */
  return ret;
}
void
wn_set_icon_notify_func(func)
wn_iconify_notify_funct func;
{
    icon_notify_funct = func;
}
