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

/* @(#)med3.h	1.0 24 Sep 1998 */

#ifndef MED3_H_DEFINED
#define MED3_H_DEFINED

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __STDC__
#ifndef PROTO
#define PROTO(a)	a
#endif	/* !PROTO */

typedef void *voidptr;
typedef const void *constvoidptr;

#include <stddef.h>	/* needed for size_t */

#else /* !__STDC__ */

#include <sys/types.h>	/* size_t for old C */

#ifndef PROTO
#define PROTO(a)	()
#endif	/* !PROTO */

/*  Patch up for things that are missing without ANSI C. */
#ifndef const
#define const
#endif
#ifndef volatile
#define volatile
#endif
#ifndef signed
#define signed
#endif

typedef char *voidptr;
typedef char *constvoidptr;

#endif /* !__STDC__ */

/*  Defining boolean type.  This might cause problems for some ...  */
#ifndef FALSE
#define FALSE	0
#endif
#ifndef TRUE
#define TRUE	1
#endif
#ifndef bool
#define bool int
#endif

#define FORMF_ARGS(fpos, argpos)	/* nothing */

/* med3.c */
void blankma PROTO((void));
void rblank PROTO((int x, int y, int width, int height));
void winblank PROTO((void));
void rblank PROTO((int x, int y, int width, int height));
void bell PROTO((int wn));
int getpuck PROTO((int *px, int *py));
void ungetpuck PROTO((int buttons, int x, int y));
int inmmain PROTO((int x, int y));
void mprintf PROTO((char *fmt, ...)) FORMF_ARGS(1, 2);

/* rvals.c */
void rvinit PROTO((void));
void rvsetup PROTO((int width));
void newlayer PROTO((int layer));
int curlayer PROTO((void));
void setrv PROTO((int rv));
void selrv PROTO((int x, int y));
int inrvals PROTO((int x, int y));
int rvnext PROTO((void));
void rvmove PROTO((int rv, int tomain));
void rvfree PROTO((int rv));
void rvclearmain PROTO((void));
void rvtaken PROTO((int rv));

/* fonts.c */
int def_font PROTO((void));
font_t * perqfont PROTO((int n));
void init_fonts PROTO((void));
void set_fonts PROTO((void));
void showlrb PROTO((void));
int newfont PROTO((int i));
font_t * getfont PROTO((char *name));
int inrbutton PROTO((int x, int y));
void selstyle PROTO((int x, int y));
void setstyle PROTO((int n));

/* cursor.c */
void initcps PROTO((void));
int setcp PROTO((int cnum));

/* rw.c */
void rwinit PROTO((char *name));
void rwsetup PROTO((int wwidth));
void bmwrite PROTO((void));
struct butst * bmread PROTO((void));
char * tidy PROTO((char *s));

/* mhole.c */
void mhset PROTO((void));
void mhcaps PROTO((char *capa, char *capb, char *capc));

/* coords.c */
void coordset PROTO((int wwidth));
void showcoords PROTO((int x, int y));
void clearcoords PROTO((void));

/* bmenu.c */
void setcurb PROTO((struct butst *b));
void bmsetrv PROTO((int rv));
void setbutstyle PROTO((int n));
int bmfont PROTO((struct butst *bm));
void bmcut PROTO((void));
void bmpaste PROTO((void));
void bmregall PROTO((struct butst *bm));
void bmdereg PROTO((struct butst *bm));
void bmuncover PROTO((void));
int addhidden PROTO((void));
void bmreset PROTO((struct butst *bm, int conceal));
struct butst * newbmen PROTO((struct wrectst *wrp, int width, int height));
char * nullcap PROTO((void));

/* bmstack.c */
void bspop PROTO((void));
void bsclear PROTO((void));
struct butst * bmtop PROTO((void));
void bmredraw PROTO((void));
struct butst * findbut PROTO((int x, int y));
struct butst * findedge PROTO((int x, int y, int w));
int bmmove PROTO((void));
void bmopen PROTO((struct butst *bm));
void bmshowshort PROTO((void));
void bmshownormal PROTO((void));
void bmshow PROTO((struct butst *bm));
void bmcheck PROTO((struct butst *bm));
void bmroot PROTO((struct butst *bm));

/* box.c */
void addsubrect PROTO((void));
void addsubbox PROTO((void));
int rectobut PROTO((void));
int equispace PROTO((void));
int getbox PROTO((struct wrectst *wr, int *xp, int *yp));

/* strings.c */
void reg_caption PROTO((char **sp, int x, int y, int width, int height,
			struct butst *button));
void sr_box PROTO((char *s, int x, int y, int width, int height, int redraw));
void cap_on PROTO((char *s, int redraw));
void cap_off PROTO((char *s, int redraw));
void cap_nsel PROTO((void));
void deregallcaps PROTO((void));
void reg_fontname PROTO((char **sp,int x, int y, int width, int n));
void srredraw PROTO((char *s));
void reg_name PROTO((char **sp, int length, int isname));
void deregister PROTO((char *s));
int srsel PROTO((int x, int y));
void setcapfont PROTO((int n));
void sr_edit PROTO((int c));
void wtext PROTO((int x, int y, char *str, font_t *font, struct wrectst *wrp));

/* bmdiv.c */
int bmdivide PROTO((struct wrectst *wr));
void movebm PROTO((struct butst *bm, int dx, int dy));
void scalebmh PROTO((struct butst *bm, int nh, int oh));
void scalebmw PROTO((struct butst *bm, int nw, int ow));


#ifdef __cplusplus
}
#endif

#endif	/* !MED3_H_DEFINED */


