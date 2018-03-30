/* wn_font.c - font loading and text display */

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


/* @(#)wn_font.c	1.19 4/11/93 (UKC) */
char wn_wn_font_c_rcsid[] = "$Id$";

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include "wn.h"
#include "wn_priv.h"
#include "wn_misc.h"
#include "wn_sc.h"

#ifdef X11
typedef XFontStruct *machfont_t;
#endif /* X11 */
#ifdef SUNVIEW
typedef struct pixfont *machfont_t;
#endif /* SUNVIEW */

#ifndef X11
static machfont_t load_font_from_path PROTO((const char *name));
#endif
static void fontinfo PROTO((machfont_t machfont, font_t *font));
static font_t *get_font PROTO((machfont_t machfont));
static int read_font_file PROTO((const char *fontfile, machfont_t *p_xfont));
static void wn_xtext_internal PROTO((int wn, font_t *font, const char *s, int x, int y, int width,  int ropfunc, int fg_color, int bg_color, int use_which, int mono));

char *_wn_Sysfont_file = NULL;

struct flistst {
	struct flistst *fl_next;
	font_t *fl_font;
};

static struct flistst *Head_flist = NULL, *Sysfont_el = NULL;

static font_t *Sysfont = NULL;

#ifndef X11
typedef struct fontpathst {
	char *fp_path;
	struct fontpathst *fp_next;
} fontpath_t;

#ifdef SUNVIEW
static fontpath_t Fontpathbuf = { "/usr/lib/fonts/fixedwidthfonts", NULL };
static fontpath_t *Fontpaths = &Fontpathbuf;
#else
static fontpath_t *Fontpaths = NULL;
#endif
#endif /* !X11 */

int
wn_add_font_path(path)
const char *path;
{
#ifdef X11
#if 0
	char **dirs, **newdirs;
	int ndirs, i;

	dirs = XGetFontPath(wn__Dpy, &ndirs);
	newdirs = (char **) wn__e_malloc((ndirs + 1) * sizeof(char *));
	for (i = 0; i < ndirs; ++i)
		newdirs[i] = dirs[i];
	newdirs[i] = path;
	XSetFontPath(wn__Dpy, newdirs, ndirs + 1); /* BUG: no check for BadValue */
	XFreeFontPath(dirs);
	free((char *)newdirs);
#endif
	return 0;
#else
	fontpath_t *fp;

	fp = (fontpath_t *) wn__e_malloc(sizeof(fontpath_t));
	fp->fp_path = strcpy(wn__e_malloc(strlen(path) + 1), path);
	fp->fp_next = Fontpaths;
	Fontpaths = fp;
	return 0;
#endif
}

#ifndef X11

#ifdef SUNVIEW
#define LOADFONT(name)	pf_open(name)
#endif

static machfont_t
load_font_from_path(name)
char *name;
{
	machfont_t machfont;
	fontpath_t *fp;
	int namelen;
	char path[512];

	if (Fontpaths == NULL || index(name, '/') != NULL)
		return LOADFONT(name);
	
	namelen = strlen(name);
	for (fp = Fontpaths; fp != NULL; fp = fp->fp_next) {
		if (strlen(fp->fp_path) + 1 + strlen(name) < sizeof(path)) {
			(void) sprintf(path, "%s/%s", fp->fp_path, name);
			if ((machfont = LOADFONT(path)) != NULL)
				return machfont;
		}
	}
}

#undef LOADFONT

#endif /* !X11 */
		
#ifdef X11
static void
fontinfo(xfont,font)
XFontStruct *xfont;
register font_t *font;
{
	int i, maxwidth, nchars, offset;
	register short *width_tab;

	if (xfont->min_byte1 != 0 || xfont->max_byte1 != 0)
		wn__panic("can't cope with two byte font");

	offset = xfont->min_char_or_byte2;
	nchars = xfont->max_char_or_byte2 + 1;
	width_tab = (short *) wn__e_malloc(nchars * sizeof(short));
	maxwidth = xfont->max_bounds.width;

	if (xfont->per_char == NULL) {
		for (i = 0; i < nchars; i++)
			width_tab[i] = maxwidth;
	}
	else {
		/*  Documentation is vague here, but we assume the font widths
		 *  array is set up such that widths[i] is the width of character
		 *  (i + firstchar).
		 */
		for (i = 0; i < offset; i++)
			width_tab[i] = 0;
		for (; i <= xfont->max_char_or_byte2; i++)
			width_tab[i] = xfont->per_char[i - offset].width;
	}
	font->ft_width_tab = width_tab;
	font->ft_nchars = nchars;
	font->ft_width = maxwidth;
/*	font->ft_height = xfont->max_bounds.ascent + xfont->max_bounds.descent;*/
/*	font->ft_baseline = xfont->max_bounds.ascent;*/
/*  RGA for 16pt font, max_bounds.ascent is 12, max_bounds.descent is 4 */
/*  but ascent is 11, descent is 4. So call to wn_set_area() in */
/*  wt_show_cursor() uses height of 16, baseline 12, which results in a */
/*  pixel left unerased as top as cursor is moved. This gives a height of */
/*  15, baseline 11, which draws cleanly */
	font->ft_height = xfont->ascent + xfont->descent;
	font->ft_baseline = xfont->ascent;
	font->ft_is_fixed_width = xfont->per_char == NULL;
	font->ft_mdfont = (mdfont_t) xfont;
}
#endif /* X11 */

#ifdef SUNVIEW
#define SUNFONT_NCHARS	256

static void
fontinfo(sunfont,font)
struct pixfont *sunfont;
register font_t *font;
{
	int fixedwidth, is_fixed, width, i, baseline;
	int maxwidth;
	register short *width_tab;

	width_tab = (short *) wn__e_malloc(SUNFONT_NCHARS * sizeof(short));
	is_fixed = TRUE;
	
	/*  The baseline in SUNVIEW fonts seems to be a negative offset
	 *  from the top of the character. Each character has its
	 *  own baseline, but in all fonts seen so far, this has
	 *  had the same value for all characters in the font.
	 *  (nb - this is given in the SUNVIEW documentation as a requirement
	 *  for pw_text().)
	 *  However, we must use a character that is actually there,
	 *  so we look for the first one with a non null pixrect
	 */
	baseline = sunfont->pf_defaultsize.y;
	for (i = 0; i < SUNFONT_NCHARS; i++)
		if (sunfont->pf_char[i].pc_pr != NULL) {
			baseline = -sunfont->pf_char[i].pc_home.y;
			break;
		}
	
	/*  The standard Sunview fonts are inconsistent over whether the
	 *  baseline should be on or below the line of pixels at the base
	 *  of a character like 'b'.
	 *
	 *  Wn's definition is that the baseline is below the line, so
	 *  if we find pixels set on the baseline, we move it down.
	 */
	if (sunfont->pf_char['b'].pc_pr != NULL) {
		struct pixrect *pr;
		int x;

		pr = sunfont->pf_char['b'].pc_pr;
		for (x = 0; x < pr->pr_width; ++x) {
			if (pr_get(pr, x, baseline) != 0) {
				char *buf;

				/*  We want to change a field in the font, but
				 *  some Sun fonts are shared and read only.
				 *  Assume the worst and make a copy.
				 */
				buf = wn__e_malloc(sizeof(struct pixfont));
				memcpy(buf, (char *)sunfont, sizeof(struct pixfont));
				sunfont = (struct pixfont *)buf;

				++baseline;
				for (i = 0; i < SUNFONT_NCHARS; ++i)
					--sunfont->pf_char[i].pc_home.y; 
				break;
			}
		}
	}
	
	fixedwidth = maxwidth = 0;
	for (i = 0; i < SUNFONT_NCHARS; i++) {
		width = sunfont->pf_char[i].pc_adv.x;
		if (i > 32 && i < 127) {
			if (fixedwidth == 0)
				fixedwidth = width;
			else if (width != fixedwidth)
				is_fixed = FALSE;
		}
		width_tab[i] = width;
		if (width > maxwidth)
			maxwidth = width;
	}
	font->ft_width_tab = width_tab;
	font->ft_nchars = SUNFONT_NCHARS;
	font->ft_width = maxwidth;
	font->ft_height = sunfont->pf_defaultsize.y;
	font->ft_baseline = baseline;
	font->ft_is_fixed_width = is_fixed;
	font->ft_mdfont = (mdfont_t) sunfont;
}
#endif /* SUNVIEW */

static font_t *
get_font(machfont)
machfont_t machfont;
{
	int want_sysfont;
	struct flistst *flist_el;
	font_t *font;

	want_sysfont = machfont == NULL;
	if (want_sysfont) {
		if (Sysfont_el != NULL)
			return Sysfont_el->fl_font;
		(void) read_font_file((char *)NULL,&machfont);
	}

	flist_el = (struct flistst *) wn__e_malloc(sizeof(struct flistst));
	font = (font_t *) wn__e_malloc(sizeof(struct fontst));
	fontinfo(machfont,font);

	flist_el->fl_font = font;
	flist_el->fl_next = Head_flist;
	Head_flist = flist_el;

	if (want_sysfont)
		Sysfont_el = flist_el;

	return font;
}

font_t *
wn_get_sysfont()
{
	if (Sysfont != NULL)
		return Sysfont;
	if (Sysfont_el == NULL)
		(void) get_font((machfont_t)NULL);
	return Sysfont_el->fl_font;
}

void
wn_set_sysfont(font)
font_t *font;
{
	if (font == NULL) {
		if (Sysfont_el == NULL)
			(void) get_font((machfont_t)NULL);
		Sysfont = Sysfont_el->fl_font;
	}
	else
		Sysfont = font;
}

font_t *
wn_install_mdfont(mdfont)
mdfont_t mdfont;
{
	register struct flistst *flist_el;
	
	for (flist_el = Head_flist; flist_el != NULL; flist_el = flist_el->fl_next)
		if (flist_el->fl_font->ft_mdfont == mdfont)
			return flist_el->fl_font;
	return get_font((machfont_t)mdfont);
}			

font_t *
wn_open_font(fontfile)
const char *fontfile;
{
	machfont_t machfont;

	if (read_font_file(fontfile,&machfont) == 0)
		return get_font(machfont);
	return NULL;
}

#ifdef X11
static int
read_font_file(fontfile, p_xfont)
const char *fontfile;
machfont_t *p_xfont;
{
#ifdef X11
	static char sysfontname[] = "fixed";
#define EXTRA_LEN 10
#endif /* X11 */

	if (fontfile == NULL) {
		if (_wn_Sysfont_file != NULL) {
			if ((*p_xfont = XLoadQueryFont(wn__Dpy, _wn_Sysfont_file)) != NULL)
				return 0;
		}
		if ((*p_xfont = XLoadQueryFont(wn__Dpy, sysfontname)) == NULL)
			wn__panic("can't open system font");
		return 0;
	}
	else {
#ifdef X11
		*p_xfont = XLoadQueryFont(wn__Dpy, (char *)fontfile);
#endif /* X11 */
		return ((*p_xfont != NULL) ? 0 : -1);
	}
}
#endif /* X11 */

#ifdef SUNVIEW
static int
read_font_file(fontfile, p_sunfont)
const char *fontfile;
struct pixfont **p_sunfont;
{
	if (fontfile == NULL) {
		if (_wn_Sysfont_file != NULL &&
				(*p_sunfont = pf_open(_wn_Sysfont_file)) != NULL)
			return 0;
		if ((*p_sunfont = pw_pfsysopen()) == NULL)
			wn__panic("can't open system font");
		return 0;
	}
	else {
		*p_sunfont = load_font_from_path(fontfile);
		return ((*p_sunfont != NULL) ? 0 : -1);
	}
}
#endif /* SUNVIEW */

char **
wn_list_fonts(pattern, maxcount, p_count)
const char *pattern;
int maxcount, *p_count;
{
#ifdef X11
	return XListFonts(wn__Dpy, pattern, maxcount, p_count);
#endif
#ifdef SUNVIEW
	return NULL;
#endif
}

void
wn_free_font_names(names)
char **names;
{
#ifdef X11
	XFreeFontNames(names);
#endif
}

void
wn_close_font(font)
font_t *font;
{
	register struct flistst *flist_el, *prev;

	prev = NULL;
	for (flist_el = Head_flist; flist_el != NULL; flist_el = flist_el->fl_next){
		if (flist_el->fl_font == font)
			break;
		prev = flist_el;
	}
	if (prev != NULL)
		prev->fl_next = flist_el->fl_next;
	else
		Head_flist = flist_el->fl_next;
	if (flist_el == NULL)
		wn__panic("bad font in wn_close_font");
	if (font == Sysfont)
		Sysfont = NULL;
	if (flist_el == Sysfont_el)
		Sysfont_el = NULL;
#ifdef X11
	XUnloadFont(wn__Dpy, ((XFontStruct *)font->ft_mdfont)->fid);
#endif /* X11 */
#ifdef SUNVIEW
	pf_close((struct pixfont *)font->ft_mdfont);
#endif /* SUNVIEW; */
	free(font->ft_width_tab);
	free(font);
}

/*  Return the width in pixels of string s when drawn in font font.
 *  Stop at a NUL byte in s, or after the first nchars characters, whichever
 *  is sooner. If nchars is -1, assume it is infinite.
 */
int
wn_strnwidth(s,nchars,font)
register const char *s;
int nchars;
font_t *font;
{
	register const char *lim;
	register int res;
	register short *wtab;
	
	if (font == NULL || font == Sysfont) {
		if (Sysfont == NULL) {
			if (Sysfont_el == NULL)
				(void) get_font((machfont_t)NULL);
			Sysfont = Sysfont_el->fl_font;
		}
		font = Sysfont;
	}
	wtab = font->ft_width_tab;
	lim = s + ((nchars == -1) ? strlen(s) : nchars);
	res = 0;
	while (s < lim && *s != '\0')
		res += wtab[(unsigned char)*s++];
	return res;
}

/*  Return index in string s drawn in font font of the character
 *  that the point x pixels to the right of the start of the string
 *  points to.
 */
int
wn_strpos(s, x, font, halfshift)
const char *s;
int x;
font_t *font;
int halfshift;
{
	register short *wtab;
	const char *cptr;
	int w;
	
	if (font == NULL || font == Sysfont) {
		if (Sysfont == NULL) {
			if (Sysfont_el == NULL)
				(void) get_font((machfont_t)NULL);
			Sysfont = Sysfont_el->fl_font;
		}
		font = Sysfont;
	}
	wtab = font->ft_width_tab;
	w = 0;
	for (cptr = s; *cptr != '\0' && w + wtab[(unsigned char)*cptr] < x; cptr++)
		w += wtab[(unsigned char)*cptr];
	if (halfshift && *cptr != '\0' && x > w + wtab[(unsigned char)*cptr]/2)
		cptr++;
	return cptr - s;
}

/*  For setting and getting text capture when using wn_xtext state
 */
int
wn_text_capture(set, reset, y)
     int set;
     int reset;
     int *y;
{
  static int capture, old_capture, last_y;

  if (set)
  {
    if (set == -1)
      capture = old_capture;
    else
    {
      old_capture = 0;
      capture = set;
      last_y = *y;
    }
  }
  else
    if (reset)
    {
      if (reset == -1)
      {
	old_capture = capture;
	if (capture > 0)
	  capture = -1;		/* don't redraw a colored variable */
	else
	  capture = 0;
      }
      else
      {
	capture = old_capture = 0;
	last_y = -1;
      }
    }
    else
      *y = last_y;
  return(capture);
}

static void
wn_xtext_internal(wn,font,s,x,y, width, ropfunc,fg_color,bg_color,use_which,mono)
int wn;
font_t *font;
const char *s;
int x, y, width,  ropfunc, fg_color, bg_color, use_which, mono;
{
  int len = strlen(s);
  if ( width >= 0 && width != WN_MAXWIDTH )
  {
    int swidth = wn_strwidth(s, font);
    while ( swidth > width )
    {
       swidth = wn_strnwidth(s, --len, font);
    }
  }
  wn_xwrite(wn,font,s,len,x,y,ropfunc,fg_color,bg_color,use_which,mono);
}

void
wn_xtext(wn,font,s,x,y, width, ropfunc,fg_color,bg_color,use_which,mono)
int wn;
font_t *font;
const char *s;
int x, y, width,  ropfunc, fg_color, bg_color, use_which, mono;
{
  int last_y, cap;

  cap = wn_text_capture(0, 0, &last_y);
  switch (cap)
  {
  case -1:		/* variable drawn in color */
    wn_overlay_insert_string(s, (int)x/Sysfont->ft_width, strlen(s),
			     0, fg_color, 1); /* colored variable */
    return;
  case 1:		/* capture text */
    wn_text_capture(1, 0, &y);	/* set last y */
    wn_overlay_insert_string(s, (int)x/Sysfont->ft_width, strlen(s),
			     (last_y != y), fg_color, 0);
  case 2:		/* no painting */
    return;

  default:
    wn_xtext_internal(wn,font,s,x,y, width, ropfunc,fg_color,
		      bg_color,use_which,mono);
  }
}

/*  Draw null-terminated string s in window wn at (x,y), using rop function
 *  ropfunc and pixels of color color
 *  If mono is non zero, restrict the output to the mono plane on color displays.
 */
void
wn_xwrite(wn,font,s,len,x,y,ropfunc,fg_color,bg_color,use_which,mono)
int wn;
font_t *font;
const char *s;
int len, x, y, ropfunc, fg_color, bg_color, use_which, mono;
{
#ifdef SUNVIEW
	register struct pixfont *pf;
	register int op;
#endif /* SUNVIEW */
	register swin_t *w = WN_TO_W(wn);
	int last_y;

	if (wn_text_capture(0, 0, &last_y))
	  return;
 	W_CHECK(wn);
	ADJ_COORDS(w, x, y);
	if (font == NULL || font == Sysfont) {
		if (Sysfont == NULL) {
			if (Sysfont_el == NULL)
				(void) get_font((machfont_t)NULL);
			Sysfont = Sysfont_el->fl_font;
		}
		font = Sysfont;
	}
#ifdef X11
	if (use_which == WN_USE_TOP)
		y += font->ft_baseline;

	SC_UNDRAW(wn);
	XSetState(wn__Dpy, W_GC(w),
			PIX(w,fg_color), PIX(w,bg_color),
			_wn_Roptab[ropfunc],
			mono ? W_PLANEMASK(w) : (unsigned)AllPlanes);
	XSetFont(wn__Dpy, W_GC(w), ((XFontStruct *)font->ft_mdfont)->fid);
	if (bg_color == WN_TRANSPARENT)
		XDrawString(wn__Dpy, w->w_win, W_GC(w), x, y, s, len);
	else
		XDrawImageString(wn__Dpy, w->w_win, W_GC(w), x, y, s, len);
	SC_REDRAW(wn);
	X_UPDATE(w);
#endif /* X11 */

#ifdef SUNVIEW
	if (use_which == WN_USE_TOP)
		y += font->ft_baseline;
	SC_UNDRAW(wn);
	op = _wn_Roptab[ropfunc] | PIX_COLOR(fg_color);
	pf = (machfont_t)font->ft_mdfont;

	w->w_bw->bw_can_refresh = FALSE;
	if (bg_color == W_BG_PIXEL(w))
		pw_text(w->w_pw, x, y, op, pf, s);
	else {
		if (bg_color != WN_TRANSPARENT) {
			pw_rop(w->w_pw,
			       x, y - font->ft_baseline,
			       wn_strwidth(s, font), font->ft_height,
			       PIX_SRC | PIX_COLOR(bg_color),
			       (struct pixrect *)NULL,
			       0, 0);
		}
		if (bg_color != WN_TRANSPARENT && ropfunc == R_RPL) {
			op = (PIX_SRC ^ PIX_DST) | PIX_COLOR(fg_color ^ bg_color);
			pw_text(w->w_pw, x, y, op, pf, s);
		}
		else
			pw_ttext(w->w_pw, x, y, op, pf, s);
	}
	w->w_bw->bw_can_refresh = TRUE;
	SC_REDRAW(wn);
#endif /* SUNVIEW */
}
