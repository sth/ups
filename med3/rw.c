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
/*#include <local/menu_priv.h>*/
#include "layout.h"
#include "bmenu.h"
#include "cursors.h"
#include "font.h"
#include "med3.h"
#ifndef lint
static char sccsid[] = "%W% (UKC) %G%";
#endif  /* lint */

extern int errno;
extern struct wrectst mwr;		/* main area */
extern font_t *plainfont;

#define MMMAX	64		/* Max MENU variable name length */
#define MNDEFAULT "med3_menu"	/* default menu name */

static char *mfname;		/* menu file name */
static char *mmname;		/* actual menu name */
static int nodenum;		/* for creating unique node names */

static void mplace PROTO((MENU * menu, int root, int x, int y));
static void mfree PROTO((MENU *menu));
static MENU * bmtom PROTO((struct butst *bm, int lcolour));
static struct butst * mtobm PROTO((MENU *m, int ndiv));
static void bmtidy PROTO((struct butst *bm));
static struct butst * bmsplit PROTO((struct butst *bm));
static void clearbreaks PROTO((struct butst *bm));
static void bmsize PROTO((struct butst *bm, int *wp, int *hp));
static int mwrite PROTO((MENU* menu, const char* fname, const char* mname));
static int prntmen PROTO((MENU* mp,const char* name,FILE* fp,int first));

/*  Initialise the menu file name. Called just once. Should eventually
 *  take a filename argument TO_DO
 */
void
rwinit(name)
char *name;
{
	char *nullcap();

	if (name != NULL) {
		mfname = malloc(strlen(name) + 1);
		strcpy(mfname,name);
	} else
		mfname = nullcap();
	reg_name(&mfname,0,0);
	mmname = malloc(MMMAX);
	if (name == NULL)
		strcpy(mmname,MNDEFAULT);
	else
		*mmname = 0;
	reg_name(&mmname,MMMAX,1);
	
}

/*  Redraw the filename. Should be called after a window size change.
 */
void
rwsetup(wwidth)
int wwidth;
{
	int x, y, width;
	extern font_t *boldfont;

	x = 25;
	y = MATOP - 34;
	wtext(x, y + 1, "Menu file:",boldfont, NULL);
	x += 85;
	width = wwidth - x - RVWIDTH - 190;
	sr_box(mfname,x,y,width,FONTHEIGHT(plainfont),1);
	y += 16;
	x = 25;
	wtext(x, y + 1, "Menu name:",boldfont,NULL);
	x += 85;
	width = wwidth - x - RVWIDTH - 190;
	sr_box(mmname,x,y,width,FONTHEIGHT(plainfont),1);
}

/*  Convert the menu to library form and then write it to the menu file
 */
void
bmwrite()
{
	MENU *bmtom();
	MENU *m;
	struct butst *bmtop();

	if (*mfname == 0) {
		mprintf("No menu file");
		bell(0);
		return;
	}
	if (*mmname == 0) {
		mprintf("No menu name");
		bell(0);
		return;
	}
	if (isdigit(*mmname)) {
		mprintf("Menu name can't start with a digit");
		bell(0);
		return;
	}
	if (bmtop() == NULL) {
		mprintf("No menu to write");
		bell(0);
		return;
	}
	setcp(CP_WAIT);
	m = bmtom(bmtop(),1);
	mplace(m,0,0,1);
	if (mwrite(m,mfname,mmname) < 0)
		bell(0);
	else
		mprintf("Menu file written");
	mfree(m);
	clearbreaks(bmtop());
}

/*  read the menu in and convert it to bmenu format
 */
struct butst *
bmread()
{
	struct butst *bm;
	struct butst *mtobm();
	struct merrst *merrp;
	MENU *m;
	extern int menerr;
	MENU *getmenu();
	struct merrst *Mopenerr();

	if (*mfname == 0) {
		mprintf("No menu");
		return(NULL);
	}
	setcp(CP_WAIT);
	if ((m = getmenu(mfname,mmname)) == NULL) {
		if (menerr == MBADFIL) {
			merrp = Mopenerr();
			mprintf("%s line:%d",merrp->mr_message,merrp->mr_line);
		} else if (menerr == MNOFIL)
			mprintf("can't open %s",mfname);
		return(NULL);
	}
	if (*mmname == 0)
		strcpy(mmname,MNDEFAULT);
	srredraw(mmname);
	mplace(m,mwr.wr_x + 1,mwr.wr_y + 1,1);
	bm = mtobm(m,1);
	mfree(m);
	deregallcaps();
	rvclearmain();
	bsclear();
	setcurb((struct butst *)NULL);
	bmtidy(bm);
	bm->but_a = NULL;
	bm->but_flags |= BM_NDIV;
	bmregall(bm);
	bmreset(bm,0);
	return(bm);
}

/*  Write out lib menu to file fname with name mname. If successful 0 is
 *  returned, if unsuccessful -1 is returned.
 */
static int
mwrite(menu,fname,mname)
MENU *menu;
const char* fname;
const char* mname;
{
	FILE *fp;

	if ((fp = fopen(fname,"w")) == NULL) {
		mprintf("Can't open menu file '%s'",fname);
		return(-1);
	}
	fprintf(fp,"#include <stdio.h>\n");
	fprintf(fp,"#include <local/menu3.h>\n\n");
	nodenum = 1;
	prntmen(menu,mname,fp,1);
	fclose(fp);
	return(0);
}

static void
print_flags(fp, flags)
FILE* fp;
int flags;
{
    static struct named_value
    {
	const char* name;
	unsigned short value;
    } values[] = 
    {
	{ "ME_FONT", ME_FONT },
	{ "ME_VER", ME_VER },
	{ "ME_HOR", ME_HOR },
	{ "ME_DIV", ME_DIV },
	{ "ME_NSEL", ME_NSEL },
	{ "ME_POPUP", ME_POPUP },
	{ "ME_FREC", ME_FREC },
	{ "ME_FREN", ME_FREN },
	{ "ME_OPEN", ME_OPEN },
	{ "ME_ISUB", ME_ISUB },
	{ "ME_FREE", ME_FREE },
	{ "ME_BGLINE", ME_BGLINE },
	{ "ME_NOSCALE", ME_NOSCALE },
	{ "ME_CREL", ME_CREL },
	{ "ME_REDRAW", ME_REDRAW },
	{ "ME_LEFT", ME_LEFT },
    };
    int did_print = 0;
    const char* plus = "";
    int i;
    fprintf(fp,"\t");
    for(i=0; flags && i < sizeof(values)/sizeof(values[0]); i++)
    {
	if (( flags & values[i].value) == values[i].value)
	{
	    fprintf(fp,"%s%s", plus, values[i].name);
	    flags -= values[i].value;
	    plus = "+";
	    did_print=1;
	}
    }
    if ( flags || !did_print)
    {
	fprintf(fp,"%s0x%x", plus, flags);
    }
    fprintf(fp,",\n");
}

static int
prntmen(mp,name,fp,first)
MENU* mp;
const char* name;
FILE* fp;
int first;
{
	int tlnum=0, brnum=0, num=0;
	int rv = Mrv_ascii(mp->me_rv);
	char *tidy();

	if (mp->me_topleft != NULL)
		tlnum = prntmen(mp->me_topleft,NULL,fp,0);
	if (mp->me_botrite != NULL)
		brnum = prntmen(mp->me_botrite,NULL,fp,0);
	if (!first) {
		num = nodenum++;
		fprintf(fp,"static MENU MM%d = {\n",num);
	} else
		fprintf(fp,"MENU %s = {\n",name);

#if 0
	fprintf(fp,"\t0x%x,\t/* me_flags */\n",mp->me_flags);
#endif
	print_flags(fp, mp->me_flags);

	fprintf(fp,"\t%d,\t/* me_pos */\n",mp->me_pos);
	if (mp->me_cap != NULL)
		fprintf(fp,"\t\"%s\",\n",tidy((char *)mp->me_cap));
	else
		fprintf(fp,"\tNULL,\t/* me_cap */\n");
	
	if ( Mrv_layer(mp->me_rv) )
	{
	    fprintf(fp,"\t0x%x+", Mrvalue(Mrv_layer(mp->me_rv),0));
	} else
	{
	    fprintf(fp,"\t");
	}
	if ( rv == '\'' || rv == '\\')
	        fprintf(fp,"'\\%c',\t/* me_rv */\n",rv);
	else if ( rv >= ' ' && rv < 0x80)
	        fprintf(fp,"'%c',\t/* me_rv */\n",rv);
	else
	        fprintf(fp,"%d,\t/* me_rv */\n",rv);

	fprintf(fp,"\t%d,\t/* me_xstart */\n",mp->me_xstart);
	fprintf(fp,"\t%d,\t/* me_ystart */\n",mp->me_ystart);
	fprintf(fp,"\t%d,\t/* me_xend */\n",mp->me_xend);
	fprintf(fp,"\t%d,\t/* me_yend */\n",mp->me_yend);
	fprintf(fp,"\t%d,\t/* me_xcurs */\n",mp->me_xcurs);
	fprintf(fp,"\t%d,\t/* me_ycurs */\n",mp->me_ycurs);
	fprintf(fp,"\t0,\t/* me_colour */\n");		/* colour */
	fprintf(fp,"\t0,\t/* me_fg_colour */\n");	/* fg_colour */
	fprintf(fp,"\t0,\t/* me_bg_colour */\n");	/* bg_colour */
	fprintf(fp,"\t0,\t/* me_save */\n");		/* save */
	if (mp->me_topleft != NULL)
		fprintf(fp,"\t&MM%d,\t/* me_topleft */\n",tlnum);
	else
		fprintf(fp,"\tNULL,\t/* me_topleft */\n");
	if (mp->me_botrite != NULL)
		fprintf(fp,"\t&MM%d,\t/* me_botrite */\n",brnum);
	else
		fprintf(fp,"\tNULL,\t/* me_botrite */\n");
	fprintf(fp,"\tNULL,\t/* me_parent */\n");	/* parent */
	fprintf(fp,"\tNULL,\t/* me_omen */\n");		/* omen */
	fprintf(fp,"\tNULL,\t/* me_toggle */\n");
	fprintf(fp,"\t0,\t/* me_select_val */\n");
	fprintf(fp,"};\n");
	return(num);
}

/*  Tidy up a string converting control characters into escape sequences.
 */
char *
tidy(s)
char *s;
{
	register char *sp, *bp;
	register c;
	static char buf[1024];

	bp = buf;
	for (sp = s; *sp != 0; sp++) {
		c = *sp & 0177;
		if ((c >= ' ') && (c < 0177) && (c != '\\') && (c != '"'))
			*(bp++) = c;
		else 
			switch (*sp) {
			    case '\n' :
				*(bp++) = '\\';
				*(bp++) = 'n';
				break;
			    case '\t' :
				*(bp++) = '\\';
				*(bp++) = 't';
				break;
			    case '\f' :
				*(bp++) = '\\';
				*(bp++) = 'f';
				break;
			    case '\b' :
				*(bp++) = '\\';
				*(bp++) = 'b';
				break;
			    case '\r' :
				*(bp++) = '\\';
				*(bp++) = 'r';
				break;
			    case '\\' :
				*(bp++) = '\\';
				*(bp++) = '\\';
				break;
			    case '"' :
				*(bp++) = '\\';
				*(bp++) = '"';
				break;
			    default :
				sprintf(bp,"\\%03o",c);
				bp += 4;
				break;
			}
	}
	*bp = 0;
	return(buf);
}

/*  Shift the menu to position x, y.
 */
static
void
mplace(menu,x,y,root)
MENU * menu;
int root,x,y;
{
	static short dx,dy;

	if (menu == NULL)
		return;
	if (root == 1) {
		dx = x - menu->me_xstart - 1;
		dy = y - menu->me_ystart - 1;
	}
	menu->me_xstart += dx;
	menu->me_xend += dx;
	menu->me_ystart += dy;
	menu->me_yend += dy;
	if (menu->me_flags & ME_VER)
		menu->me_pos += dy;
	if (menu->me_flags & ME_HOR)
		menu->me_pos += dx;
	mplace(menu->me_topleft,x,y,0);
	mplace(menu->me_botrite,x,y,0);
	return;
}

/*  free the menu
 */
static
void
mfree(menu)
MENU *menu;
{
	if (menu == NULL)
		return;
	mfree(menu->me_topleft);
	mfree(menu->me_botrite);
	free((char *)menu);
}

/*  Convert the bmenu to a binary menu as used by the menu library. lcolour
 *  is the line colour: 0 for background and 1 for foreground.
 */
static MENU *
bmtom(bm,lcolour)
struct butst *bm;
int lcolour;
{
	register struct butst *bu, *b;
	MENU *m, *m1;
	int width,height;
	struct butst *bmsplit();

	if (bm == NULL)
		return(NULL);

	m = (MENU *)malloc(sizeof(MENU));
	m->me_cap = NULL;
	m->me_topleft = NULL;
	m->me_botrite = NULL;
	m->me_flags = bm->but_style & SM_FONT;
	if (lcolour == 0)
		m->me_flags |= ME_BGLINE;
	m->me_colour = 0;
	m->me_xstart = bm->but_left;
	m->me_ystart = bm->but_top;
	bmsize(bm,&width,&height);
	m->me_xend = m->me_xstart + width;
	m->me_yend = m->me_ystart + height;
	m->me_rv = bm->but_rv;
	if ((bm->but_b == NULL) || (bm->but_flags & BM_BREAK)) {
		bu = bm->but_u;
		if ((bm->but_flags & BM_CLEAR) == 0) {
			m->me_cap = malloc(strlen((char *)bm->but_caption) + 1);
			strcpy((char *)m->me_cap,(char *)bm->but_caption);
			m->me_flags |= ME_FREC;
			lcolour = (bm->but_style & SM_BGLINE) ? 0 : 1;
			if (lcolour == 0)
				m->me_flags |= ME_BGLINE;
			else
				m->me_flags &= ~ME_BGLINE;
		} else
			m->me_cap = NULL;
		if (bm->but_flags & BM_FREE) {
			m->me_flags |= ME_FREE;
			if (bm->but_a == NULL)
				lcolour = (bm->but_style & SM_BGLINE) ? 0 : 1;
			if (lcolour == 0)
				m->me_flags |= ME_BGLINE;
			else
				m->me_flags &= ~ME_BGLINE;
		}
		if (bm->but_style & SM_POPUP)
			m->me_flags |= ME_POPUP;
		if (bm->but_style & SM_NOSCALE)
			m->me_flags |= ME_NOSCALE;
		if (bu == NULL)
			return(m);
		if (bu->but_b == NULL) {
			m->me_flags |= ME_ISUB;
			m->me_topleft = bmtom(bu,lcolour);
			m->me_botrite = NULL;
			if (bm->but_style & SM_CREL) {
				m->me_flags |= ME_CREL;
				if ((m1 = m->me_topleft) != NULL) {
				    m1->me_xcurs = m1->me_xstart - m->me_xend;
				    m1->me_ycurs = m1->me_ystart - m->me_yend;
				}
			}
			return(m);
		}
	} else {
		bu = bm;
		m->me_cap = NULL;
	}

	b = bmsplit(bu);
	bmsize(bu,&width,&height);
	if (bu->but_flags & BM_HOR) {
		m->me_flags |= ME_HOR;
		m->me_pos = m->me_xstart + width;
	} else {
		m->me_flags |= ME_VER;
		m->me_pos = m->me_ystart + height;
	}
	m->me_topleft = bmtom(bu,lcolour);
	m->me_botrite = bmtom(b,lcolour);
	return(m);
}

/*  Convert the library menu into a bmenu. The but_a pointers and BM_FIRST
 *  flags are left to tidy up later. If ndiv is nonzero top button is 
 *  indivisible. 
 */
static struct butst *
mtobm(m,ndiv)
MENU *m;
int ndiv;
{
	struct butst *b1, *b2, *b;

	if ((m->me_flags & ME_DIV) == 0) {
		b = (struct butst *)malloc(sizeof(struct butst));
		b->but_left = m->me_xstart;
		b->but_top = m->me_ystart;
		b->but_width = m->me_xend - m->me_xstart;
		b->but_height = m->me_yend - m->me_ystart;
		b->but_rv = m->me_rv;
		b->but_style = m->me_flags & ME_FONT;
		if (m->me_flags & ME_POPUP)
			b->but_style |= SM_POPUP;
		if (m->me_flags & ME_NOSCALE)
			b->but_style |= SM_NOSCALE;
		if (m->me_flags & ME_BGLINE)
			b->but_style |= SM_BGLINE;
		if (m->me_flags & ME_CREL)
			b->but_style |= SM_CREL;
		b->but_b = NULL;
		b->but_flags = 0;
		if (m->me_flags & ME_ISUB) {
			b->but_u = mtobm(m->me_topleft,1);
		} else
			b->but_u = NULL;
		b->but_caption = malloc(strlen(m->me_cap) + 1);
		strcpy(b->but_caption,m->me_cap);
		if (ndiv)
			b->but_flags |= BM_NDIV;
		if (m->me_flags & ME_FREE)
			b->but_flags |= BM_FREE;
		return(b);
	}
	b1 = mtobm(m->me_topleft,0);
	b2 = mtobm(m->me_botrite,0);
	if (m->me_flags & ME_VER) {
		if (b1->but_flags & BM_HOR) {
			b = b1;
			b1 = (struct butst *)malloc(sizeof(struct butst));
			b1->but_left = m->me_xstart;
			b1->but_top = m->me_ystart;
			b1->but_width = m->me_xend - m->me_xstart;
			b1->but_height = m->me_pos - m->me_ystart;
			b1->but_flags = BM_VER | BM_CLEAR;
			b1->but_style = 0;
			b1->but_u = b;
			b1->but_b = NULL;
		}
		if (b2->but_flags & BM_HOR) {
			b = b2;
			b2 = (struct butst *)malloc(sizeof(struct butst));
			b2->but_left = m->me_xstart;
			b2->but_top = m->me_pos;
			b2->but_width = m->me_xend - m->me_xstart;
			b2->but_height = m->me_yend - m->me_pos;
			b2->but_flags = BM_VER | BM_CLEAR;
			b2->but_style = 0;
			b2->but_u = b;
			b2->but_b = NULL;
		}
		b1->but_flags |= BM_VER;
		b2->but_flags |= BM_VER;
	} else {
		if (b1->but_flags & BM_VER) {
			b = b1;
			b1 = (struct butst *)malloc(sizeof(struct butst));
			b1->but_left = m->me_xstart;
			b1->but_top = m->me_ystart;
			b1->but_width = m->me_pos - m->me_xstart;
			b1->but_height = m->me_yend - m->me_ystart;
			b1->but_flags = BM_HOR | BM_CLEAR;
			b1->but_style = 0;
			b1->but_u = b;
			b1->but_b = NULL;
		}
		if (b2->but_flags & BM_VER) {
			b = b2;
			b2 = (struct butst *)malloc(sizeof(struct butst));
			b2->but_left = m->me_pos;
			b2->but_top = m->me_ystart;
			b2->but_width = m->me_xend - m->me_pos;
			b2->but_height = m->me_yend - m->me_ystart;
			b2->but_flags = BM_HOR | BM_CLEAR;
			b2->but_style = 0;
			b2->but_u = b;
			b2->but_b = NULL;
		}
		b1->but_flags |= BM_HOR;
		b2->but_flags |= BM_HOR;
	}
	for (b = b1; b->but_b != NULL; b = b->but_b)
		;
	b->but_b = b2;
	if ((m->me_cap == NULL) && !ndiv)
		return(b1);
	b = (struct butst *)malloc(sizeof(struct butst));
	b->but_left = m->me_xstart;
	b->but_top = m->me_ystart;
	b->but_width = m->me_xend - m->me_xstart;
	b->but_height = m->me_yend - m->me_ystart;
	b->but_flags = 0;
	b->but_style = m->me_flags & ME_FONT;
	if (m->me_flags & ME_POPUP)
		b->but_style |= SM_POPUP;
	if (m->me_flags & ME_NOSCALE)
		b->but_style |= SM_NOSCALE;
	if (m->me_flags & ME_BGLINE)
		b->but_style |= SM_BGLINE;
	if (m->me_flags & ME_CREL)
		b->but_style |= SM_CREL;
	b->but_u = b1;
	b->but_b = NULL;
	if (m->me_cap != NULL) {
		b->but_caption = malloc(strlen(m->me_cap) + 1);
		strcpy(b->but_caption,m->me_cap);
		b->but_rv = m->me_rv;
	} else
		b->but_flags |=  BM_CLEAR;
	if (ndiv)
		b->but_flags |= BM_NDIV;
	if (m->me_flags & ME_FREE)
		b->but_flags |= BM_FREE;
	return(b);
}

/*  Tidy up a bmenu after constructing it from a library menu. log return
 *  values and fill in the but_a pointers and BM_FIRST flags in the bmenu
 */
static
void
bmtidy(bm)
struct butst *bm;
{
	register struct butst *b;

	bm->but_flags |= BM_FIRST;
	for(b = bm; b != NULL; b = b->but_b) {
		if ((b->but_flags & BM_CLEAR) == 0)
			rvtaken(b->but_rv);
		if (b->but_b != NULL)
			b->but_b->but_a = b;
		if (b->but_u != NULL) {
			b->but_u->but_a = b;
			bmtidy(b->but_u);
		}
	}
}

/*  Attempt to split a bmenu into two halves of equal length and return a
 *  pointer to the second half.
 */
static struct butst *
bmsplit(bm)
struct butst *bm;
{
	register struct butst *b;
	register int n, i;

	n = 1;
	for (b = bm; b->but_b != NULL; b = b->but_b) {
		if (b->but_flags & BM_BREAK)
			break;
		n++;
	}
	if (n <= 1)
		return(NULL);
	n >>= 1;
	b = bm;
	for (i = 1; i < n; i++)
		b = b->but_b;
	b->but_flags |= BM_BREAK;
	return(b->but_b);
}

static
void
clearbreaks(bm)
struct butst *bm;
{
	if (bm == NULL)
		return;
	clearbreaks(bm->but_u);
	clearbreaks(bm->but_b);
	bm->but_flags &= ~BM_BREAK;
}

/*  Calculate the overall width and height of the bmenu.
 */
static
void
bmsize(bm,wp,hp)
struct butst *bm;
int *wp, *hp;
{
	register struct butst *b;

	if (bm == NULL){
		*wp = 0;
		*hp = 0;
		return;
	}
	for (b = bm; b->but_b != NULL; b = b->but_b)
		if (b->but_flags & BM_BREAK)
			break;

	*wp = b->but_left + b->but_width - bm->but_left;
	*hp = b->but_top + b->but_height - bm->but_top;
}
