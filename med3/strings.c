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

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <local/wn.h>
#include <local/menu3.h>
#include <ctype.h>
#include "bmenu.h"
#include "font.h"
#include "med3.h"
#ifndef lint
static char sccsid[] = "%W% (UKC) %G%";
#endif  /* lint */

extern int wn;			/* window number */
extern font_t *plainfont;

/*  Registered string types
 */
#define SR_FONT		1	/* font name */
#define SR_VCAP		2	/* visible menu caption */
#define SR_ICAP		3	/* invisible menu caption */
#define SR_NCAP		4	/* an unselectable menu caption */
#define SR_MFIL		5	/* menu file name */
#define SR_MNAM		6	/* menu name */

#define DEL	0177
#define CON_U	('U'&0x1f)
#define CON_C	('C'&0x1f)
#define RIGHT_ARROW -105
#define LEFT_ARROW -106
#define CON_A  ('A'&0x1f)
#define CON_B  ('B'&0x1f)
#define CON_D  ('D'&0x1f)
#define CON_E  ('E'&0x1f)
#define CON_F  ('F'&0x1f)
#define CON_H  ('H'&0x1f)
#define BACKSPACE CON_H
#define CON_K  ('K'&0x1f)
#define CON_Y  ('Y'&0x1f)


/*  structure describing a registered string
 */
struct sregst {
	char **sr_sp;
	int sr_user;	/* user supplied info */
	short sr_type;	/* the string type */
	short sr_len;	/* the length of the malloced array */
	short sr_x;	/* the box in which the string is displayed */
	short sr_y;
	short sr_width;
	short sr_height;
	short sr_fnum;	/* the font number */
	short sr_xstart;/* string start for filenames etc. */
	struct clpst *sr_lp; /* individual line positions for captions */
	struct butst *sr_button;
	struct sregst *sr_next;
};

/*  structure describing the position of an individual caption line.
 */
struct clpst {
	short cl_x;
	short cl_y;
	short cl_height;
	short cl_index;
	struct clpst *cl_next;
};

static struct sregst *srlist = NULL;	/* the list of registered strings */
static struct sregst *srcur = NULL;	/* the currently selected string */
static int srindex;			/* index into the current string */
static char *oldstring = NULL;		/* the original string from the
					   current selection */


static int  special_chars PROTO((int c, int l, char* s));
static void fnedit PROTO((int c));
static void ciedit PROTO((int c));
static void mcedit PROTO((int c));
static void showsr PROTO((struct sregst *sr));
static void drawbox PROTO((int left, int top, int width, int height));
static void setcl PROTO((struct sregst *sr));
static void tcurs PROTO((int x, int y, int height, struct wrectst *wrp));

/*  Add the caption to the list of registered strings.
 */
void
reg_caption(sp,x,y,width,height,button)
char **sp;
int x, y, width, height;
struct butst *button;
{
	struct sregst *sr;
	int def_font();

	sr = (struct sregst *)malloc(sizeof(struct sregst));

	sr->sr_sp = sp;
	sr->sr_len = strlen(*sp) + 1;
	sr->sr_user = 0;
	sr->sr_x = x;
	sr->sr_y = y;
	sr->sr_height = height;
	sr->sr_width = width;
	sr->sr_type = SR_VCAP;
	sr->sr_fnum = bmfont(button);
	sr->sr_button = button;
	sr->sr_next = srlist;
	sr->sr_lp = NULL;
	srlist = sr;
}

/*  Set the string box coordinates and possibly redraw.
 */
void
sr_box(s,x,y,width,height,redraw)
char *s;
int x, y, width, height,redraw;
{
	struct sregst *sr;

	for (sr = srlist; sr != NULL; sr = sr->sr_next)
		if (s == *sr->sr_sp)
			break;
	if (sr == NULL)
		abort();
	sr->sr_x = x;
	sr->sr_y = y;
	sr->sr_width = width;
	sr->sr_height = height;
	if (redraw)
		showsr(sr);
}

/*  Enable the menu caption and redisplay it if redraw is non-zero
 */
void
cap_on(s,redraw)
char *s;
int redraw;
{
	struct sregst *sr;

	for (sr = srlist; sr != NULL; sr = sr->sr_next)
		if (s == *sr->sr_sp)
			break;
	if (sr == NULL)
		abort();
	sr->sr_type = SR_VCAP;
	if (redraw)
		showsr(sr);
}

/*  Disable the menu caption, and hide it if redraw is non-zero.
 */
void
cap_off(s,redraw)
char *s;
{	struct sregst *sr;

	for (sr = srlist; sr != NULL; sr = sr->sr_next)
		if (s == *sr->sr_sp)
			break;
	if (sr == NULL)
		abort();
	sr->sr_type = SR_ICAP;
	if (srcur == sr) {
		srcur = NULL;
		setcurb((struct butst *)NULL);
	}
	if (redraw)
		showsr(sr);
}

/*  Make all the menu captions unselectable
 */
void
cap_nsel()
{
	register struct sregst *sr;

	for (sr = srlist; sr != NULL; sr = sr->sr_next)
		if (sr->sr_type == SR_VCAP)
			sr->sr_type = SR_NCAP;
}

/*  Deregister all the registered caption strings.
 */
void
deregallcaps()
{
	struct sregst *srl, *sr;
	struct clpst *cl;

	srl = srlist;
	srlist = NULL;

	while (srl != NULL) {
		sr = srl;
		srl = srl->sr_next;
		if ((sr->sr_type == SR_VCAP) || (sr->sr_type == SR_ICAP)) {
			if (srcur == sr)
				srcur = NULL;
			for (cl = sr->sr_lp; cl != NULL; cl = cl->cl_next)
				free((char *)cl);
			free((char *)sr);
		} else {
			sr->sr_next = srlist;
			srlist = sr;
		}
	}
}

/*  Add a font name string to the list of registered strings
 */
void
reg_fontname(sp,x,y,width,n)
char **sp;
int x, y, n;
{
	struct sregst *sr;

	sr = (struct sregst *)malloc(sizeof(struct sregst));

	sr->sr_sp = sp;
	sr->sr_len = strlen(*sp) + 1;
	sr->sr_user = n;
	sr->sr_x = x;
	sr->sr_y = y;
	sr->sr_height = FONTHEIGHT(plainfont);
	sr->sr_width = width;
	sr->sr_type = SR_FONT;
	sr->sr_fnum = -1;
	sr->sr_next = srlist;
	srlist = sr;
}

/* Redraw the string.
 */
void
srredraw(s)
char *s;
{
	struct sregst *sr;

	for (sr = srlist; sr != NULL; sr = sr->sr_next)
		if (s == *sr->sr_sp)
			break;
	if (sr == NULL)
		abort();
	showsr(sr);
}

/*  Add a menu file name or menu name string to the list of registered strings.
 *  If length is non zero it is used as the string length. If isname is nonzero
 *  the string is registered as a name, otherwise it is registered as a file name.
 */
void
reg_name(sp,length,isname)
char **sp;
{
	struct sregst *sr;

	sr = (struct sregst *)malloc(sizeof(struct sregst));

	sr->sr_sp = sp;
	if (length != 0)
		sr->sr_len = length;
	else
		sr->sr_len = strlen(*sp) + 1;
	sr->sr_user = 0;
	sr->sr_x = 0;
	sr->sr_y = 0;
	sr->sr_height = FONTHEIGHT(plainfont);
	sr->sr_width = 0;
	sr->sr_type = isname ? SR_MNAM : SR_MFIL;
	sr->sr_fnum = -1;
	sr->sr_next = srlist;
	srlist = sr;
}

/*  Remove the string from the registered list.
 */
void
deregister(s)
char *s;
{
	struct sregst *sr, **srp;
	struct clpst *cl;

	for (srp = &srlist; *srp != NULL; srp = &((*srp)->sr_next)) {
		if (*(*srp)->sr_sp == s)
			break;
	}
	if (*srp == NULL)
		abort();
	if (srcur == *srp) {
		srcur = NULL;
		setcurb((struct butst *)NULL);
		srindex = 0;
	}
	sr = *srp;
	for (cl = sr->sr_lp; cl != NULL; cl = cl->cl_next)
		free((char *)cl);
	*srp = sr->sr_next;
	free((char *)sr);
}

/*  Attempt to select a position within a displayed string. If we cannot
 *  deselect the previous selection -1 is returned. If everything is ok
 *  then 0 is returned.
 */
int
srsel(x, y)
int x, y;
{
	register struct sregst *sr;
	font_t *fnt;
	struct clpst *cl;
	char *s;
	int cx;
	register i, w, lastw;

	Mpushsu(wn);
	for (sr = srlist; sr != NULL; sr = sr->sr_next) {
		if ((y >= sr->sr_y) && 
		    (y < sr->sr_y + sr->sr_height) &&
		    (x >= sr->sr_x) &&
		    (x < sr->sr_x + sr->sr_width) &&
		    (sr->sr_type != SR_ICAP) &&
		    (sr->sr_type != SR_NCAP))
			break;
	}

	if ((srcur != NULL) && (srcur != sr)) {
		/* Try and remove the current selection
		 */
		if (srcur->sr_type == SR_FONT)
			if (strcmp(oldstring,*srcur->sr_sp))
				if (newfont(srcur->sr_user) < 0) {
					Mpopsu(wn);
					bell(wn);
					return(-1);
				}
		if (srcur->sr_type == SR_MNAM)
			if (isdigit(**srcur->sr_sp)) {
				Mpopsu(wn);
				mprintf("Menu name can't start with a digit");
				bell(wn);
				return(-1);
			}
		if (srcur->sr_type == SR_VCAP)
			setcurb((struct butst *)NULL);
		if (oldstring != NULL) {
			free(oldstring);
			oldstring = NULL;
		}
		srindex = -1;
		showsr(srcur);
	}
	if ((sr != NULL) && (sr != srcur)){
		oldstring = malloc(strlen(*sr->sr_sp) + 1);
		strcpy(oldstring,*sr->sr_sp);
	}
	srcur = sr;
	if (sr == NULL) {
		Mpopsu(wn);
		return(0);
	}

	/*  Find the new index position
	 */
	if ((sr->sr_type == SR_FONT) ||
	    (sr->sr_type == SR_MNAM) ||
	    (sr->sr_type == SR_MFIL)) {
		cx = sr->sr_xstart;
		lastw = 0;
		s = *sr->sr_sp;
		for (i = 0; s[i]; i++) {
			w = CHWIDTH(s[i],plainfont);
			cx += w / 2 + lastw;
			lastw = w - w/2;
			if (cx >= x)
				break;
		}
		srindex = i;
		showsr(sr);
	}
	if (sr->sr_type == SR_VCAP) {
		setcurb(sr->sr_button);
		/* first find the line. Assumes the current line structures
		 * are in place.
		 */
		fnt = perqfont(sr->sr_fnum);
		for (cl = sr->sr_lp; cl != NULL; cl = cl->cl_next)
			if (cl->cl_y + cl->cl_height >= y)
				break;
		if (cl == NULL)
			srindex = strlen(*sr->sr_sp);
		else {
			s = *sr->sr_sp + cl->cl_index;
			cx = cl->cl_x;
			lastw = 0;
			for (i = 0; s[i]; i++) {
				if (s[i] == '\n')
					break;
				w = CHWIDTH(s[i],fnt);
				cx += w / 2 + lastw;
				lastw = w - w/2;
				if (cx >= x)
					break;
			}
			srindex = cl->cl_index + i;
		}
		showsr(sr);
	}
	Mpopsu(wn);
	return(0);
}

/*  Set the font for the currently displayed caption.
 */
void
setcapfont(n)
int n;
{
	if ((srcur == NULL) || (srcur->sr_type != SR_VCAP))
		return;
	srcur->sr_fnum = n;
	showsr(srcur);
}

/*  Use the character to edit the current string
 */
void
sr_edit(c)
int c;
{
	if (srcur == NULL) {
		bell(0);
		return;
	}
	switch (srcur->sr_type) {
		case SR_FONT :
		case SR_MFIL :
			fnedit(c);
			break;
		case SR_MNAM :
			ciedit(c);
			break;
		case SR_VCAP :
			mcedit(c);
			break;
	}
}

static int
special_chars( c, l, s)
int c;
int l;
char* s;
{
    int i;
    switch (c )
    {
    case DEL:
    case CON_H:
	/* delete char at srindex */
	if (srindex > 0)
	{
		for (i = srindex; i <= l; i++)
			s[i - 1] = s[i];
		srindex--;
	}
	break;
    case CON_U:
	/* delete to start of line */
	if (srindex > 0)
	{
		for (i = srindex; i <= l; i++)
			s[i - srindex] = s[i];
		srindex = 0;
	}
	break;
    case CON_C:
	/* restore the string and deselect it
	*/
	strcpy(s,oldstring);
	l = strlen(s);
	srindex = (l < srindex) ? l : srindex;
	return CON_C;
    case CON_A:
	/* Move to start of line */
	srindex = 0;
	break;
    case CON_B:
    case LEFT_ARROW:
	/* Move to left */
	if ( srindex )
	    srindex--;
	break;
    case CON_D:
	/* Delete next char */
	for (i = srindex+1; i <= l; i++)
	    s[i - 1] = s[i];
	break;

    case CON_E:
	/* Move to end of line */
	srindex = l;
	break;
    case CON_F:
    case RIGHT_ARROW:
	/* Move to right */
	if ( srindex<l )
	    srindex++;
	break;
    case CON_K:
	/* Delete to end of line */
	s[srindex] = 0;
	break;
    case CON_Y:
	/* Paste the X selection */
	{
	    const char* selection = NULL;
	    int len;
	    wn_get_selection(wn,&selection, &len);
	    for ( i = 0; i < len; i++)
	    {
		if ( selection[i] < ' ')
		    break;  /* don't paste control chars */
		sr_edit(selection[i]);
	    }
	}
	break;
    default:
	return 0;
    }
    return 1;
}
/*  Edit the current string as a file name
 */
static
void
fnedit(c)
int c;
{
	register l, i;
	register char *s;
	struct sregst *sr;

	s = *srcur->sr_sp;
	l = strlen(s);
	special_chars( c, l, s);
	if ( c == CON_C)
	{
	    sr = srcur;
	    srcur = NULL;
	    showsr(sr);
	    return;
	} else if (((c == '\r')||(c == '\n')) && (srcur->sr_type == SR_FONT)) {
		if (newfont(srcur->sr_user) == 0) {
			if (oldstring != NULL)
				free(oldstring);
			s = *srcur->sr_sp;
			oldstring = malloc(strlen(s) + 1);
			strcpy(oldstring,s);
		} else
			bell(0);
		return;
	} else if ((c > ' ') && (c < 0177)) {
		if (l + 1 == srcur->sr_len) {
			srcur->sr_len += 10;
			s = malloc(srcur->sr_len);
			strcpy(s,*srcur->sr_sp);
			free(*srcur->sr_sp);
			*srcur->sr_sp = s;
		}
		for (i = l; i >= srindex; i--)
			s[i + 1] = s[i];
		s[srindex] = c;
		srindex++;
	}
	showsr(srcur);
}

/*  Edit the current string as a valid c identifier
 */
static
void
ciedit(c)
int c;
{
	register l, i;
	register char *s;
	struct sregst *sr;

	s = *srcur->sr_sp;
	l = strlen(s);
	special_chars( c, l, s);
	if ( c == CON_C)
	{
	    sr = srcur;
	    srcur = NULL;
	    showsr(sr);
	    return;
	} else if (isdigit(c) || isalpha(c) || (c == '_')) {
		/*  a valid character to put in an identifier
		 */
		if (isdigit(c) && (srindex == 0)) {
			/* digit at start
			 */
			mprintf("Identifier can't start with a digit");
			bell(0);
			return;
		}
		if (l + 1 == srcur->sr_len) {
			srcur->sr_len += 10;
			s = malloc(srcur->sr_len);
			strcpy(s,*srcur->sr_sp);
			free(*srcur->sr_sp);
			*srcur->sr_sp = s;
		}
		for (i = l; i >= srindex; i--)
			s[i + 1] = s[i];
		s[srindex] = c;
		srindex++;
	}
	showsr(srcur);
}

/*  Edit the current string as a menu caption
 */
static
void
mcedit(c)
int c;
{
	register l, i;
	register char *s;

	s = *srcur->sr_sp;
	l = strlen(s);
	special_chars( c, l, s);
	if (c == '\r')
		c = '\n';
	if (((c >= ' ') && (c < 0177)) || (c == '\n')){
		if (l + 1 == srcur->sr_len) {
			srcur->sr_len += 10;
			s = malloc(srcur->sr_len);
			strcpy(s,*srcur->sr_sp);
			free(*srcur->sr_sp);
			*srcur->sr_sp = s;
		}
		for (i = l; i >= srindex; i--)
			s[i + 1] = s[i];
		s[srindex] = c;
		srindex++;
	}
	showsr(srcur);
}

/*  display a registered string.
 */
static
void
showsr(sr)
struct sregst *sr;
{
	struct wrectst wr;
	font_t *fnt;
	register struct clpst *cl;
	register char *s;
	int l, cx, y, i;

	Mpushsu(wn);
	/*  first clear the box
	 */
	rblank(sr->sr_x,sr->sr_y,sr->sr_width,sr->sr_height);
	
	wr.wr_x = sr->sr_x;
	wr.wr_y = sr->sr_y;
	wr.wr_width = sr->sr_width;
	wr.wr_height = sr->sr_height;
	if ((sr->sr_type == SR_FONT) ||
	    (sr->sr_type == SR_MNAM) ||
	    (sr->sr_type == SR_MFIL)){
		/*  the easy one, do it here. Display the string so its
		 *  right hand end is visible
		 */
		sr->sr_xstart = sr->sr_x + 5;
		l = 5;
		for (s = *sr->sr_sp; *s; s++)
			l += CHWIDTH(*s,plainfont);
		if (l > wr.wr_width)
			sr->sr_xstart -= l - wr.wr_width;
		y = sr->sr_y;
		wtext(sr->sr_xstart, y, *sr->sr_sp, plainfont, &wr);
		if ((srcur == sr) && (srindex >= 0)){
			/*  insert the text cursor
			 */
			s = *sr->sr_sp;
			cx = sr->sr_xstart - 2;
			for (i = 0; i < srindex; i++)
				cx += CHWIDTH(s[i],plainfont);
			tcurs(cx, y, FONTHEIGHT(plainfont), &wr);
		}
	}
	if ((sr->sr_type == SR_VCAP) || (sr->sr_type == SR_NCAP)) {
		fnt = perqfont(sr->sr_fnum);
		setcl(sr);	/* do the line starts */
		s = *sr->sr_sp;
		for (cl = sr->sr_lp; cl != NULL; cl = cl->cl_next)
			wtext(cl->cl_x,cl->cl_y,s + cl->cl_index,fnt,&wr);
		if ((srcur == sr) && (srindex >= 0)) {
			/*  Highlight the box
			 */
			drawbox(sr->sr_x + 1,sr->sr_y + 1,sr->sr_width - 2,
							sr->sr_height - 2);

			/*  insert the text cursor
			 */
			for (cl = sr->sr_lp; cl->cl_next != NULL;
							cl = cl->cl_next)
				if (cl->cl_next->cl_index > srindex)
					break;
			cx = cl->cl_x;
			for (i = cl->cl_index; i < srindex; i++)
				cx += CHWIDTH(s[i],fnt);
			tcurs(cx - 2, cl->cl_y, cl->cl_height, &wr);
		}
	}
	Mpopsu(wn);
}

/*  Draw a box of the given place and size.
 */
static
void
drawbox(left,top,width,height)
int left,top,width,height;
{
	wn_box_round(wn,left,top,width,height,WN_FG(wn));
}

/*  Build the list of line start structures.
 */
static
void
setcl(sr)
struct sregst *sr;
{
	register struct clpst *cl=0, **clp;
	font_t *fnt;
	register char *s;
	int i, j, l, lines, y, height;

	fnt = perqfont(sr->sr_fnum);
	/*  first find out how many lines there are in the caption.
	 */
	lines = 1;
	for (s = *sr->sr_sp; *s; s++)
		if (*s == '\n')
			lines++;
	height = FONTHEIGHT(fnt) + 2;

	clp = &sr->sr_lp;
	s = *sr->sr_sp;
	i = 0;
	y = sr->sr_y + sr->sr_height/2 - (lines*height)/2;
	for (j = 0; j < lines; j++) {
		if (*clp == NULL) {
			cl = (struct clpst *)malloc(sizeof(struct clpst));
			cl->cl_next = NULL;
			*clp = cl;
		} else
			cl = *clp;
		clp = &cl->cl_next;
		cl->cl_y = y;
		cl->cl_height = height;
		y += height;
		cl->cl_index = i;
		l = 0;
		
		for (; (s[i] != 0) && (s[i] != '\n'); i++) {
			l += CHWIDTH(s[i],fnt);
		}
		cl->cl_x = sr->sr_x + (sr->sr_width - l) / 2;
		if (cl->cl_x < sr->sr_x + 5)
			cl->cl_x = sr->sr_x + 5;
		i++;
	}
	if (cl->cl_next != NULL) {
		free((char *)cl->cl_next);
		cl->cl_next = NULL;
	}
}
		
/*  Paint in the text cursor at position x, y. The rectangle is used for clipping.
 */
static
void
tcurs(x, y, height, wrp)
int x, y, height;
struct wrectst *wrp;
{
	if ((x >= wrp->wr_x) && (y >= wrp->wr_y) &&
	    (x + 2 <= wrp->wr_x + wrp->wr_width) &&
	    (y + height <= wrp->wr_y + wrp->wr_height))
		wn_invert_area(wn,x,y,2,height);
}

/*  Display the line at the coordinates. All characters except zeros and
 *  linefeeds are displayed. The coordinates are of the top left hand corner
 *  of the first character.
 */
void
wtext(x,y,str,font,wrp)
char *str;
font_t *font;
struct wrectst *wrp;
{
	char *s;
	register char *sp;
	int maxx, xstart;

	if ((wrp != NULL) && ((y < wrp->wr_y) || 
	    (y + FONTHEIGHT(font) > wrp->wr_y + wrp->wr_height)))
		return;

	s = malloc(strlen(str) + 1);
	xstart = x;
	if (wrp != NULL)
		while (xstart < wrp->wr_x && *str)
			xstart += CHWIDTH(*str++,font);

	maxx = xstart;
	for (sp = s; *str; str++) {
		if (*str == '\n')
			break;
		if (wrp != NULL) {
			maxx += CHWIDTH(*str,font);
			if (maxx > wrp->wr_x + wrp->wr_width)
				break;
		}
		*sp++ = *str;
	}
	*sp = 0;

	wn_text(wn,font,s,xstart,y,WN_FG(wn),WN_BG(wn),WN_USE_TOP);
	free(s);
}

