/*  Copyright 1993 John Bovey, University of Kent at Canterbury.
 *  All Rights Reserved.
 *
 *  This file is part of MED3.
 *
 *  MED3 is free software; you can redistribute it and/or modify it under
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

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#
#include <local/wn.h>
#include <signal.h>

#include <local/menu3.h>
#include "layout.h"
#include "bmenu.h"
#include "cursors.h"
#include "font.h"
#include "med3.h"

#ifndef lint
static char sccsid[] = "%W% (UKC) %G%";
#endif  /* lint */

#define BMCHECK		/* perform stringent checks on the structure */

#define MINHEIGHT 200	/* minimum window dimensions */
#define MINWIDTH  580

#define CMD_MENU "med3.men"
#define BMASK (EV_KEY|EV_BUTTON_DOWN|EV_BUTTON_UP|EV_MOUSE_MOVED|EV_WINDOW_RESIZED|\
			EV_WINDOW_EXPOSED)

extern int errno;

int wn;			/* window number */

struct wrectst mwr;		/* main area */
static int mad = -1;		/* message area descriptor */
static int wschanged = 0;	/* flag for window size changes */
static int toosmall = 0;	/* flag set if window is too small */
static int wwidth, wheight;	/* the current window dimensions */
static int cmd;			/* command menu descriptor */

font_t *boldfont;		/* font used for headings etc. */
font_t *plainfont;		/* font used for mousehole captions, file names etc. */

static void wcentre PROTO((char *msg, int width, int height, font_t *font));
static void wredraw PROTO((void));
static void wcentre PROTO((char *msg, int width, int height, font_t *font));

main(argc,argv)
int argc;
char *argv[];
{
	int x, y, rv;
	int btns, lastbtns;
	struct butst *b;
	extern MENU cmd_men;

	wn_munge_args(argc,(const char **)argv);
	if ((wn = wn_open_stdwin()) < 0) {
		fprintf(stderr, "%s: Not running in a window\n",argv[0]);
		exit(1);
	}

	wn_get_window_size(wn,&wwidth,&wheight);
	if (wwidth < MINWIDTH || wheight < MINHEIGHT) {
		fprintf(stderr,"med3: window too small\n");
		exit(1);
	}

	boldfont = Mstdfont();
	plainfont = wn_get_sysfont();
	cmd = Minsert(&cmd_men);
	mwr.wr_x = MALEFT + 1;
	mwr.wr_y = MATOP + 1;
	mwr.wr_width = wwidth;
	mwr.wr_height = wheight - mwr.wr_y;
	Msize(cmd,mwr.wr_width - MHWIDTH,MHEIGHT);
	Mplace(cmd,0,0);
	Mfmodes(cmd,MH_WHITE,MH_BLACK,MH_GREY);

	rvinit();
	init_fonts();
	initcps();
	rwinit((argc > 1) ? argv[1] : NULL);

	winblank();
	Mpushsu(wn);
	Mdisplay(cmd,wn,0);
	mad = Msopen(wn,0,MHEIGHT+1,mwr.wr_width - MHWIDTH - COWIDTH,
			MHHEIGHT - 2 - MHEIGHT);
	/* flush any message that may be waiting
	 */
	mprintf((char *)NULL);
	mhset();
	rvsetup(wwidth);
	set_fonts();
	rwsetup(wwidth);
	coordset(wwidth);

	if ((b = bmread()) != NULL) {
		bmroot(b);
		rvsetup(wwidth);
		bmredraw();
	}
	Mpopsu(wn);
	for(lastbtns = 0;;lastbtns = btns) {
		setcp(CP_COMMAND);
		if (!toosmall)
			clearcoords();
		btns = getpuck(&x,&y);
		if (toosmall)
			continue;
		if (btns & lastbtns & B_LEFT) {
			/*  mouse moved with the left button down
			 */
			Mselect(x,y,wn,MS_CONTINUE,0);
		} else if (btns & ~lastbtns & B_LEFT) {
			/* Left button just pressed
			 */
			mprintf("\r");
			Mselect(x,y,wn,MS_PRESSED,0);
			selstyle(x,y);
			selrv(x,y);
			if ((x >= mwr.wr_x) && (y >= mwr.wr_y)) {
				/*  Walk the bmenu
				 */
				Mpushsu(wn);
				if ((b = findbut(x,y)) != NULL) {
					if (b->but_u != NULL)
						bmopen(b);
					else {
						mprintf("no submenu");
						bell(wn);
					}
				} else {
					bmreset(bmtop(),0);
					bmredraw();
				}
				Mpopsu(wn);
			}
		} else if (~btns & lastbtns & B_LEFT) {
			/* button A has been released
			 */
			if ((rv = Mselect(x,y,wn,MS_RELEASED,0)) == 0)
				continue;
			switch (MS_rv(rv)) {
			    case 'q' :	/* quit */
				wn_close_window(wn);
				exit(0);
			    case 'n' :	/* create a new menu */
				blankma();
				setcp(CP_NEW);
				if (getbox(&mwr, &x, &y) >= 0) {
					rvclearmain();
					rvsetup(0);
					deregallcaps();
					setcurb((struct butst *)NULL);
					bsclear();
					newbmen(&mwr,x,y);
				}
				bmshow(bmtop());
				break;
			    case 'd' :	/* subdivide a button */
				while (bmdivide(&mwr) >= 0)
					bmshow(bmtop());
				break;
			    case 'b' :	/* add a bound submenu */
				Mpushsu(wn);
				blankma();
				bmshowshort();
				bmshow(bmtop());
				Mpopsu(wn);
				addsubrect();
				Mpushsu(wn);
				bmshownormal();
				bmredraw();
				Mpopsu(wn);
				break;
			    case 'f' :	/* add a free submenu */
				addsubbox();
				break;
			    case 'c' :	/* cover a rectangle */
				bmshowshort();
				do {
					Mpushsu(wn);
					blankma();
					bmshow(bmtop());
					Mpopsu(wn);
				} while (rectobut() >= 0);
				bmshownormal();
				bmredraw();
				break;
			    case 'u' :	/* uncover a submenu */
				bmuncover();
				break;
			    case 'e' :	/* equal spacing */
				bmshowshort();
				do {
					Mpushsu(wn);
					blankma();
					bmshow(bmtop());
					Mpopsu(wn);
				} while (equispace() >= 0);
				bmshownormal();
				bmredraw();
				break;
			    case 'C' :	/* cut */
				bmcut();
				break;
			    case 'p' :	/* paste */
				bmpaste();
				break;
			    case 'r' :	/* read a menu */
				if ((b = bmread()) != NULL) {
					blankma();
					bmroot(b);
					rvsetup(0);
					bmredraw();
				} else
					bell(wn);
				break;
			    case 'w' :	/* write a menu */
				bsclear();
				bmreset(bmtop(),0);
				bmwrite();
				bmredraw();
				break;
			    case 'm' :	/* move a division */
				bmshowshort();
				do {
					Mpushsu(wn);
					blankma();
					bmredraw();
					Mpopsu(wn);
				} while (bmmove() == 0);
				bmshownormal();
				bmredraw();
				break;
			}
			Mreset(cmd);
#ifdef BMCHECK
			bmcheck(bmtop());
#endif BMCHECK
		} else {
			/* Button A is up so just update the mousehole
			 */
			if ((x >= mwr.wr_x) && (y >= mwr.wr_y))
				mhcaps("submenu","select","");
			else if (inmmain(x,y))
				mhcaps("do","select","");
			else if (inrvals(x,y) || inrbutton(x,y))
				mhcaps("set","select","");
			else
				mhcaps("","select","");
		}
	}
}

/*  Return true if the coordinates are in the command menu. This function
 *  should really be performed by the menu package.
 */
int
inmmain(x,y)
int x, y;
{
	return ((x <= mwr.wr_width - MHWIDTH) && (y <= MHEIGHT));
}

/*  Redraw the window after a size change.
 */
static
void
wredraw()
{
	int width, height;

	wn_get_window_size(wn,&width,&height);
	Mpushsu(wn);
	winblank();
	if (width < MINWIDTH || height < MINHEIGHT) {
		toosmall = 1;
		wcentre("Window too small",width,height,boldfont);
		Mpopsu(wn);
		return;
	} else
		toosmall = 0;

	mwr.wr_width = width;
	mwr.wr_height = height - mwr.wr_y;
	Msize(cmd,mwr.wr_width - MHWIDTH,MHEIGHT);
	Msclose(mad);
	mad = Msopen(wn,0,MHEIGHT+1,mwr.wr_width - MHWIDTH - COWIDTH,
			MHHEIGHT - 2 - MHEIGHT);
	Mredraw(wn);
	mhset();
	rvsetup(width);
	rwsetup(width);
	coordset(width);
	set_fonts();
	bmredraw();
	mhcaps((char *)NULL,(char *)NULL,(char *)NULL);
	Mpopsu(wn);
	wschanged = 0;
}

/*  Print the message centred in the window.
 */
static
void
wcentre(msg,width,height,font)
char *msg;
int width,height;
font_t *font;
{
	register char *sp;
	int x, y, len = 0;

	for (sp = msg; *sp; sp++)
		len += CHWIDTH(*sp,font);
	x = (width - len) / 2;
	y = (height - FONTHEIGHT(font)) / 2;
	wtext(x,y,msg,boldfont,NULL);
}

/*  Blank the main editing area.
 */
void
blankma()
{
	rblank(mwr.wr_x,mwr.wr_y,mwr.wr_width,mwr.wr_height);
}

/*  For puck reads stored by ungetpuck
 */
static int lastbuttons;
int lastpuckx, lastpucky;
static int pucksaved = 0;

/*  Get the next puck input and return the button state and coordinates. Middle
 *  button presses, keyboard input and window damage are handled automatically
 *  and do not cause a return.
 */
int
getpuck(px,py)
int *px, *py;
{
	event_t ev;

	if (pucksaved) {
		pucksaved = 0;
		*px = lastpuckx;
		*py = lastpucky;
		return(lastbuttons);
	}
	for (;;) {
		wn_next_event(wn,BMASK,&ev);
		if (ev.ev_type & (EV_WINDOW_RESIZED|EV_WINDOW_EXPOSED)) {
			wredraw();
			continue;
		}
		if (ev.ev_type == EV_BUTTON_DOWN && (ev.ev_flags & B_MIDDLE)) {
			srsel(ev.ev_x,ev.ev_y);
			continue;
		}
		if (ev.ev_type == EV_KEY) {
			sr_edit(ev.ev_char);
			continue;
		}
		*px = ev.ev_x;
		*py = ev.ev_y;
		return(ev.ev_buttons);
	}
}

/*  Save the buttons so that they can be returned by getpuck.
 */
void
ungetpuck(buttons, x, y)
int buttons, x, y;
{
	lastbuttons = buttons;
	lastpuckx = x;
	lastpucky = y;
	pucksaved = 1;
}

/*  blank the entire window
 */
void
winblank()
{
	int width, height;

	wn_get_window_size(wn,&width,&height);
	wn_set_area(wn, 0, 0, width, height, WN_BG(wn));
}

/*  blank out the rectangle
 */
void
rblank(x, y, width, height)
int x, y, width, height;
{
	wn_set_area(wn,x, y, width, height, WN_BG(wn));
}

/*  Display a message in the message area
 */
#ifdef __STDC__
void
mprintf(char *fmt, ...)
{

#else /* !__STDC__ */
void
mprintf(va_alist)
va_dcl
{
	char *fmt;
#endif /* !__STDC__ */
	va_list args;
	static char buf[200];

#ifdef __STDC__
	va_start(args, fmt);
#else
	va_start(args);
	fmt = va_arg(args, char *);
#endif

	if (fmt != NULL)
		vsprintf(buf,fmt,args);

	va_end(args);

	if (mad >= 0) {
		Mpushsu(wn);
		Mputc(mad,'\r');
		Mputs(mad,buf);
		Mpopsu(wn);
	}
}

void
bell(wn)
int wn;
{
	wn_bell(wn);
}

