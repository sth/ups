/* render.c - convert logical lines into display lines */

/*  Copyright 1994 Mark Russell, University of Kent at Canterbury.
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


/* @(#)render.c	1.5 24/5/95 (UKC) */
char edit_render_c_rcsid[] = "$Id$";

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <local/ukcprog.h>

#include "edit.h"
#include "display.h"
#include "props.h"
#include "render.h"

typedef struct Render_block Render_block;

struct Render_context {
	int tabspaces;
	bool stop_at_newline;
	bool fold_lines;  /* RGA set to false to have mult-line with no wrap */
	int lmore_width;
	int lmore_rmargin;
	int rmore_width;
	int rmore_lmargin;
	int more_vmargin;
};

struct Render_block {
	int xpos;
	int offset;
	size_t point;
	Edit_fontinfo *fi;
	Edit_flags flags;
	Render_block *next;
};

struct Render_desc {
	Render_block *rblist;
	Render_block *lastrb;
	int refcount;
	char *linebuf;		/* Buffer for converting a display line */
	size_t linebuf_size;	/* Current maximum line length */
	bool isfree;
	Render_desc *nextfree;	/* Free list management */
};

typedef struct {
	bool want_cnum;
	size_t point;
	bool want_point;
	int cnum;
	int xpos;
	Edit_fontinfo *fi;
	Edit_flags flags;
} Cnum_to_point;

static Render_block *Rb_freelist = NULL;
static Render_desc *Rd_freelist = NULL;
	
static void convert_line PROTO((Edit_display *d, Lmap *lm, Lmap *nextlm, 
				Cnum_to_point *cp, Edit_propchange **p_pc));
static void add_render_block PROTO((Render_desc *rd, Edit_propchange *pc, 
				    int char_offset, int xpos, size_t point,
				    int *p_height, int *p_baseline));

static void expand_linebuf PROTO((Render_desc *rd, char **p_obuf, char **p_optr, 
                                  char **p_olim));
static void convert_char PROTO((size_t cnum, int ch, int tabspaces,
				const char **p_charstr, int *p_charlen));
static void free_rblist PROTO((Render_block *rblist));
static Render_desc *reset_rdesc PROTO((Lmap *lm));
static int paint_changed_text PROTO((Edit_display *d, int ypos, int baseline, 
                                     int height, Render_block *old_rb, 
                                     Render_block *new_rb, 
                                     const char *old_linebuf, 
                                     const char *new_linebuf));
static void render_text PROTO((Edit_display *d, Render_block *rb, int x, int y, 
                               int baseline, int width, int height, 
                               const char *text, int count));
static bool check_cp PROTO((Cnum_to_point *cp, int cnum, int xpos, 
                            int char_width, Edit_fontinfo *fi, 
                            Edit_flags flags, size_t point, int ch_offset));
static void next_props PROTO((Edit_buffer *buffer, size_t point, 
                              Edit_propchange **p_pc, Render_block **p_rb,
			      Edit_fontinfo **p_fi, Edit_flags *p_flags,
			      size_t *p_next_pc_point));
static bool point_to_cnum PROTO((Edit_display *d, Lmap *lm, size_t point, 
				 const char **p_line, int *p_cnum,
				 int *p_xpos, int *p_height, int *p_baseline,
				 Edit_fontinfo **p_fi, Edit_flags *p_flags));


Render_context *
edit__make_render_context(tabspaces, stop_at_newline, fold_lines,
			  lmore_width, lmore_rmargin,
			  rmore_width, rmore_lmargin, more_vmargin)
int tabspaces;
bool stop_at_newline, fold_lines;
int lmore_width, lmore_rmargin, rmore_width, rmore_lmargin, more_vmargin;
{
	Render_context *rc;

	rc = (Render_context *)e_malloc(sizeof(Render_context));
	rc->tabspaces = tabspaces;
	rc->stop_at_newline = stop_at_newline;
	rc->fold_lines = fold_lines ;
	rc->lmore_width = lmore_width;
	rc->lmore_rmargin = lmore_rmargin;
	rc->rmore_width = rmore_width;
	rc->rmore_lmargin = rmore_lmargin;
	rc->more_vmargin = more_vmargin;

	return rc;
}

void
edit__update_rdesc_points(rd, pos, delta)
Render_desc *rd;
size_t pos;
long delta;
{
	Render_block *rb;
	
	if (rd != NULL) {
		for (rb = rd->rblist; rb != NULL; rb = rb->next) {
			if (rb->point >= pos) {
				if (delta < 0 && -delta > rb->point)
					rb->point = 0;
				else
					rb->point += delta;
			}
		}
	}
}

void
edit__check_rdescs_same(rd1, rd2)
Render_desc *rd1, *rd2;
{
	Render_block *rb1, *rb2;

	if ((rd1 == NULL) != (rd2 == NULL))
		panic("rd1/rd2 NULLness mismatch in crs");

	if (rd1 == NULL)
		return;
	
	rb2 = rd2->rblist;

	for (rb1 = rd1->rblist; rb1 != NULL; rb1 = rb1->next) {
		if (rd2 == NULL)
			panic("short rd2 count in crs");

		if (rb1->xpos != rb2->xpos ||
		    rb1->offset != rb2->offset ||
		    rb1->point != rb2->point ||
		    rb1->fi != rb2->fi ||
		    rb1->flags != rb2->flags) {
			panic("rb mismatch in crs");
		}

		rb2 = rb2->next;
	}

	if (rb2 != NULL)
		panic("long rd2 count in crs");

	if (memcmp(rd1->linebuf,rd2->linebuf, (size_t)rd1->lastrb->offset) != 0)
		panic("text mismatch in crs");
}

static void
add_render_block(rd, pc, char_offset, xpos, point, p_height, p_baseline)
Render_desc *rd;
Edit_propchange *pc;
int char_offset, xpos;
size_t point;
int *p_height, *p_baseline;
{
	Render_block *rb;
	
	if (Rb_freelist != NULL) {
		rb = Rb_freelist;
		Rb_freelist = rb->next;

		if (rb->offset != -1)
			panic("rb ref botch in arb");
	}
	else {
		rb = (Render_block *)e_malloc(sizeof(Render_block));
	}
	
	rb->xpos = xpos;
	rb->offset = char_offset;
	rb->point = point;
	
	if (pc != NULL) {
		rb->fi = pc->fi;
		rb->flags = pc->flags;
		
		if (pc->fi->height > *p_height)
			*p_height = pc->fi->height;

		if (pc->fi->baseline > *p_baseline)
			*p_baseline = pc->fi->baseline;

	}
	else {
		rb->fi = NULL;
		rb->flags = 0;
	}

	if (rd->lastrb != NULL)
		rd->lastrb->next = rb;
	else
		rd->rblist = rb;
	rd->lastrb = rb;
	
	rb->next = NULL;
}

static Render_desc *
reset_rdesc(lm)
Lmap *lm;
{
	Render_desc *rd;
	
	if (Rd_freelist != NULL) {
		rd = Rd_freelist;
		Rd_freelist = rd->nextfree;
	}
	else {
		rd = (Render_desc *)e_malloc(sizeof(Render_desc));
		
		rd->linebuf_size = 80;
		rd->linebuf = e_malloc(rd->linebuf_size);
	}

	rd->rblist = NULL;
	rd->lastrb = NULL;
	rd->refcount = 1;

	edit__free_rdesc(lm);
	lm->rdesc = rd;

	return rd;
}

bool
edit__extra_char_would_fit(d, lm, ch)
Edit_display *d;
Lmap *lm;
int ch;
{
	Render_block *rb, *lastrb;
	Edit_fontinfo *fi;

	lastrb = lm->rdesc->lastrb;
	fi = NULL;		/* to satisfy gcc */
	
	for (rb = lm->rdesc->rblist; rb != lastrb; rb = rb->next)
		fi = rb->fi;

	return rb->xpos + fi->char_width_tab[ch] <= d->display_width;
}

void
edit__render_line(d, lm, ypos, oldlm)
Edit_display *d;
Lmap *lm;
int ypos;
Lmap *oldlm;
{
	Render_context *rc;
	Render_desc *rd;
	Render_block *rb;
	int xpos, lim_xpos, marker_height;

	if (oldlm != NULL && oldlm->rdesc == NULL)
		oldlm = NULL;
	
	rc = d->render_context;
	rd = lm->rdesc;

	marker_height = lm->height - (rc->more_vmargin * 2);

	if (marker_height < 1)
		marker_height = 1;
		
	if (rc->lmore_width != 0 &&
	    (oldlm == NULL || ((oldlm->column == 0) != (lm->column == 0)))) {
		(d->render_ops->set_area)(d->render_data, 0, ypos,
					  rc->lmore_width + rc->lmore_rmargin,
					  lm->height, FALSE);
		
		(d->render_ops->set_area)(d->render_data,
					  0, ypos + rc->more_vmargin,
					  rc->lmore_width, marker_height,
					  lm->column != 0);
		
	}
	    
	if (oldlm != NULL) {
		Render_desc *oldrd;

		oldrd = oldlm->rdesc;
		
		xpos = paint_changed_text(d, ypos, lm->baseline, lm->height,
					  oldrd->rblist, rd->rblist,
					  oldrd->linebuf, rd->linebuf);

		lim_xpos = oldrd->lastrb->xpos;
	}
	else {
		lim_xpos = d->display_width - rc->rmore_width;
	
		for (rb = rd->rblist; rb != rd->lastrb; rb = rb->next) {
			if (rb->offset == -1)
				panic("using free rb");

			render_text(d, rb, rb->xpos, ypos, lm->baseline,
				    rb->next->xpos - rb->xpos, lm->height,
				    rd->linebuf + rb->offset,
				    rb->next->offset - rb->offset);
		}

		xpos = rb->xpos;
	}
	
	if (xpos < lim_xpos) {
		(d->render_ops->set_area)(d->render_data,
					  xpos, ypos,
					  lim_xpos - xpos, lm->height, FALSE);
	}
	
	if (rc->rmore_width != 0 &&
	    (oldlm == NULL || (oldlm->line_continues != lm->line_continues))) {
		int cwidth;

		cwidth = rc->rmore_lmargin + rc->rmore_width;
		
		(d->render_ops->set_area)(d->render_data,
					  d->display_width - cwidth, ypos,
					  cwidth, lm->height, FALSE);
					  
		(d->render_ops->set_area)(d->render_data,
					  d->display_width - rc->rmore_width,
					  ypos + rc->more_vmargin,
					  rc->rmore_width, marker_height,
					  lm->line_continues);
	}
}

static void
render_text(d, rb, x, y, baseline, width, height, text, count)
Edit_display *d;
Render_block *rb;
int x, y, baseline, width, height;
const char *text;
int count;
{
	(*d->render_ops->render_text)(d->render_data,
				      rb->fi->font_handle, rb->flags,
				      x, y, baseline, width, height,
				      text, count);
}

static int
paint_changed_text(d, ypos, baseline, height, old_rb, new_rb,
		   old_linebuf, new_linebuf)
Edit_display *d;
int ypos, baseline, height;
Render_block *old_rb, *new_rb;
const char *old_linebuf, *new_linebuf;
{
	int offset, old_lim_offset, new_lim_offset, start_offset;
	int old_xpos, new_xpos, start_xpos;
	Edit_fontinfo *old_fi, *new_fi;
	Edit_flags old_flags, new_flags;
	bool still_matching;
	
	offset = old_lim_offset = new_lim_offset = start_offset = 0;

	start_xpos = old_xpos = new_xpos = 0;	/* to satisfy gcc */
	old_fi = new_fi = NULL;			/* to satisfy gcc */
	old_flags = new_flags = 0;		/* to satisfy gcc */
	still_matching = TRUE;			/* to satisfy gcc */
	
	for (;;) {
		if (offset == old_lim_offset) {
			if (offset != 0)
				old_rb = old_rb->next;

			if (old_rb->next != NULL) {
				old_fi = old_rb->fi;
				old_xpos = old_rb->xpos;
				old_flags = old_rb->flags;
				old_lim_offset = old_rb->next->offset;
			}
			else {
				old_rb = NULL;
			}
		}

		if (offset == new_lim_offset) {
			if (offset > start_offset) {
				render_text(d, new_rb,
					    start_xpos, ypos, baseline,
					    new_xpos - start_xpos, height,
					    new_linebuf + start_offset,
					    offset - start_offset);
			}
			
			if (offset != 0)
				new_rb = new_rb->next;

			if (new_rb->next == NULL ||
			    new_rb->next->offset == new_rb->offset)
				break;
			
			start_xpos = new_rb->xpos;
			start_offset = new_rb->offset;
			still_matching = TRUE;

			new_fi = new_rb->fi;
			new_xpos = new_rb->xpos;
			new_flags = new_rb->flags;
			new_lim_offset = new_rb->next->offset;
		}

		if (old_rb != NULL && still_matching && old_xpos == new_xpos &&
		    old_fi == new_fi && old_flags == new_flags &&
		    old_linebuf[offset] == new_linebuf[offset]) {
			++start_offset;
			start_xpos +=
				new_fi->char_width_tab[new_linebuf[offset]];
		}
		else {
			still_matching = FALSE;
		}

		if (old_fi != NULL)
			old_xpos += old_fi->char_width_tab[new_linebuf[offset]];
		
		new_xpos += new_fi->char_width_tab[new_linebuf[offset]];
		
		++offset;
	}

	return new_rb->xpos;
}
	
static bool
point_to_cnum(d, lm, point, p_line, p_cnum, p_xpos, p_height, p_baseline,
	      p_fi, p_flags)
Edit_display *d;
Lmap *lm;
size_t point;
const char **p_line;
int *p_cnum, *p_xpos, *p_height, *p_baseline;
Edit_fontinfo **p_fi;
Edit_flags *p_flags;
{
	Cnum_to_point cpbuf;
	size_t buflen;

	buflen = edit_get_buffer_length(d->buffer);

	if (point > buflen)
		return FALSE;

	if (lm->rdesc == NULL && point == buflen) {
		Edit_propchange *pc;

		pc = edit_get_propchange_at_point(d->buffer, point);
		*p_line = " ";
		*p_cnum = 0;
		*p_xpos = 0;
		*p_fi = pc->fi;
		*p_flags = pc->flags;

		*p_height = pc->fi->height;
		*p_baseline = pc->fi->baseline;
	
		return TRUE;
	}
	
	cpbuf.want_cnum = TRUE;
	cpbuf.cnum = 0;		/* for tidyness */
	cpbuf.want_point = FALSE;
	cpbuf.point = point;

	convert_line(d, lm, (Lmap *)NULL, &cpbuf, (Edit_propchange **)NULL);
	
	*p_line = lm->rdesc->linebuf;
	*p_cnum = cpbuf.cnum;
	*p_xpos = cpbuf.xpos;
	*p_fi = cpbuf.fi;
	*p_flags = cpbuf.flags;

	*p_height = lm->height;
	*p_baseline = lm->baseline;
	
	return !cpbuf.want_cnum;

}

bool
edit__lmap_point_to_pixel(d, lm, point, p_x, p_width, p_height, p_baseline)
Edit_display *d;
Lmap *lm;
size_t point;
int *p_x, *p_width, *p_height, *p_baseline;
{
	int cnum;
	const char *line;
	Edit_fontinfo *fi;
	Edit_flags flags;
	
	if (point_to_cnum(d, lm, point, &line, &cnum,
			  p_x, p_height, p_baseline, &fi, &flags)) {
		*p_width = fi->char_width_tab[line[cnum]];
		return TRUE;
	}

	return FALSE;
}

bool
edit__xpos_to_point(d, lm, xpos, p_point)
Edit_display *d;
Lmap *lm;
int xpos;
size_t *p_point;
{
	Cnum_to_point cpbuf;
	
	cpbuf.want_cnum = FALSE;
	cpbuf.want_point = TRUE;
	cpbuf.xpos = xpos;

	convert_line(d, lm, (Lmap *)NULL, &cpbuf, (Edit_propchange **)NULL);

	*p_point = cpbuf.point;
	return !cpbuf.want_point;
}

void
edit__draw_lmap_cursor(d, lm, point, ypos, on)
Edit_display *d;
Lmap *lm;
size_t point;
int ypos;
bool on;
{
	const char *line;
	Edit_fontinfo *fi;
	Edit_flags flags;
	int cnum, xpos, height, baseline;
	
	if (point_to_cnum(d, lm, point, &line, &cnum,
			  &xpos, &height, &baseline, &fi, &flags)) {
		int ch;

		ch = line[cnum];
		
		(*d->render_ops->show_cursor)(d->render_data,
					      fi->font_handle, flags,
					      xpos, ypos,
					      baseline,
					      fi->char_width_tab[ch],
					      height, ch, on);
	}
}

static void
free_rblist(rblist)
Render_block *rblist;
{
	Render_block *rb, *next;
	
	for (rb = rblist; rb != NULL; rb = next) {
		if (rb->offset == -1)
			panic("freeing free rb");
		rb->offset = -1;
		
		next = rb->next;
		rb->next = Rb_freelist;
		Rb_freelist = rb;
	}
}

void
edit__init_rdesc(lm)
Lmap *lm;
{
	lm->rdesc = NULL;
}

void
edit__copy_lmap(dst, src)
Lmap *dst, *src;
{
	if (dst != src) {
		edit__free_rdesc(dst);
		*dst = *src;

		if (dst->rdesc != NULL)
			++dst->rdesc->refcount;
	}
}

void
edit__free_rdesc(lm)
Lmap *lm;
{
	Render_desc *rd;
	
	if ((rd = lm->rdesc) == NULL)
		return;

	--rd->refcount;

	if (rd->refcount == 0) {
		free_rblist(lm->rdesc->rblist);

		rd->nextfree = Rd_freelist;
		Rd_freelist = rd;
	}

	lm->rdesc = NULL;
}
	
void
edit__set_lmap(lm, point)
Lmap *lm;
size_t point;
{
	edit__free_rdesc(lm);
	
	lm->point = point;
	lm->ch_offset = 0;
	lm->column = 0;

	lm->height = 0;
	lm->baseline = 0;
	lm->line_continues = FALSE;
}

void
edit__convert_line(d, lm, nextlm, p_pc)
Edit_display *d;
Lmap *lm, *nextlm;
Edit_propchange **p_pc;
{
	convert_line(d, lm, nextlm, (Cnum_to_point *)NULL, p_pc);
}

static void
convert_line(d, lm, nextlm, cp, p_pc)
Edit_display *d;
Lmap *lm, *nextlm;
Cnum_to_point *cp;
Edit_propchange **p_pc;
{
	const char *iptr, *ilim;
	size_t point, next_pc_point;
	char *obuf, *optr, *olim;
	Render_context *rc;
	Render_desc *rd;
	Render_block *rb;
	int xpos, xlim, height, baseline, ch_offset, column;
	bool seen_newline, need_render_block, line_continues;
	Edit_propchange *pc;
	Edit_fontinfo *fi;
	Edit_flags flags;

	if (cp != NULL) {
		pc = NULL;
		rd = lm->rdesc;
		rb = rd->rblist;
		fi = rb->fi;
		flags = rb->flags;
		next_pc_point = rb->next->point;
		need_render_block = FALSE;
	}
	else {
		if (p_pc == NULL || *p_pc == NULL) {
			pc = edit_get_next_propchange(d->buffer,
						      (Edit_propchange *)NULL,
						      lm->point,
						      &next_pc_point);
		}
		else {
			pc = *p_pc;
			next_pc_point = (pc->next != NULL) ? pc->next->point
							   : EDIT_MAX_POINT;
		}
		
		rd = reset_rdesc(lm);
		rb = NULL;
		fi = pc->fi;
		flags = pc->flags;
		need_render_block = TRUE;
	}
	
	point = lm->point;
	ch_offset = lm->ch_offset;
	column = lm->column;
	
	rc = d->render_context;
	seen_newline = FALSE;
	iptr = ilim = NULL;
	optr = obuf = rd->linebuf;
	olim = optr + rd->linebuf_size;
	xpos = (column != 0) ? (rc->lmore_width + rc->lmore_rmargin) : 0;
	xlim = d->display_width - xpos - (rc->rmore_lmargin + rc->rmore_width);
	
	height = fi->height;
	baseline = fi->baseline;
	
	for (;;) {
		if (iptr == ilim &&
		    !edit_get_bytes(d->buffer, point, &iptr, &ilim)) {
				break;
		}

		if (isprint(*iptr)) {
			int char_width;

			if (fi == NULL && pc == NULL)
				break;
			
			char_width = fi->char_width_tab[*iptr];
			
			if (!rc->fold_lines && xpos  >= xlim)
			{
			  while(*iptr != '\n' && iptr != ilim)
			  {
			    xpos += char_width;
			    iptr++;
			    point++;
			  }
			  break;
			}
			if (rc->fold_lines && xpos + char_width > xlim)
			{
			  break;
			}

			if (cp != NULL && check_cp(cp, optr - obuf,
						   xpos, char_width, fi, flags,
						   point, ch_offset)) {
				cp = NULL;
			}

			if (need_render_block) {
				add_render_block(rd, pc, optr - obuf, xpos,
						 point, &height, &baseline);
				need_render_block = FALSE;
			}
			
			xpos += char_width;

			if (optr == olim)
				expand_linebuf(rd, &obuf, &optr, &olim);
			*optr++ = *iptr++;

			if (++point == next_pc_point) {
				next_props(d->buffer, point, &pc, &rb,
					   &fi, &flags, &next_pc_point);
				need_render_block = pc != NULL;
			}
		}
		else if (   rc->stop_at_newline
			 && ( *iptr == '\n'
			     || (*iptr == '\r' && iptr[1] == '\n')))
		{
		    /* RCB: For files with DOS end-of-line records */
		    if ( *iptr == '\r')
		    {
			point++;
			iptr++;
		    }
		    break;
		}
		else {
			const char *charstr, *sptr, *slim;
			int charlen;
			size_t cnum, nbytes;

			if (need_render_block) {
				add_render_block(rd, pc, optr - obuf, xpos,
						 point, &height, &baseline);
				need_render_block = FALSE;
			}

			cnum = column + (optr - obuf) - ch_offset;
			convert_char(cnum, (unsigned char)*iptr, rc->tabspaces,
				     &charstr, &charlen);
		
			if (charlen <= ch_offset)
				panic("cc botch in cl");
			nbytes = charlen - ch_offset;

			sptr = charstr + ch_offset;
			slim = charstr + charlen;

			while (sptr < slim) {
				int char_width;
				
				if (fi == NULL && pc == NULL)
					break;
				
				char_width = fi->char_width_tab[*sptr];
				
				if (xpos + char_width > xlim)
					break;
				
				if (cp != NULL && check_cp(cp, optr - obuf,
						   xpos, char_width, fi, flags,
						   point, ch_offset)) {
					cp = NULL;
				}
				
				xpos += char_width;
				
				if (optr == olim)
					expand_linebuf(rd, &obuf, &optr, &olim);
				*optr++ = *sptr++;
			}
			
			ch_offset = sptr - charstr;

			if (ch_offset == charlen) {
				++iptr;
				
				if (++point == next_pc_point) {
					next_props(d->buffer, point, &pc, &rb,
						   &fi, &flags, &next_pc_point);
					need_render_block = pc != NULL;
				}
				
				ch_offset = 0;
			}
			else {
				break;
			}
		}
	}

	if (iptr == ilim)
		edit_get_bytes(d->buffer, point, &iptr, &ilim);

	line_continues = iptr != ilim &&
		         !(   rc->stop_at_newline
			       && (   *iptr == '\n'
			           || (*iptr == '\r' && iptr[1] == '\n')));
	if (line_continues && !rc->fold_lines)
	{
	  int char_width;
	  
	  char_width = fi->char_width_tab[*iptr];
	  while(*iptr != '\n' && iptr != ilim)
	  {
	    xpos += char_width;
	    iptr++;
	    point++;
	  }
	  line_continues = 0;
	}
	
	if (line_continues) {
		column += optr - obuf;
	}
	else {
		if (cp != NULL)
			check_cp(cp, optr - obuf,
				 xpos, d->display_width - xpos, 
				 fi, flags, point, ch_offset);
		
		if (need_render_block)
			add_render_block(rd, pc, optr - obuf, xpos,
					 point, &height, &baseline);
		
		if (optr == olim)
			expand_linebuf(rd, &obuf, &optr, &olim);
		*optr++ = ' ';
		
		if (iptr != ilim) {
			++iptr;
			
			if (++point == next_pc_point) {
				next_props(d->buffer, point, &pc, &rb,
					   &fi, &flags, &next_pc_point);
				
			}
		}
			
		column = 0;
	}

	if (pc != NULL) {
		add_render_block(rd, (Edit_propchange *)NULL, optr - obuf, xpos,
				 point, (int *)NULL, (int *)NULL);

		lm->height = height;
		lm->baseline = baseline;
		lm->line_continues = line_continues;

		if (p_pc != NULL)
			*p_pc = pc;
	}
	
	if (nextlm != NULL) {
		nextlm->point = point;
		nextlm->ch_offset = ch_offset;
		nextlm->column = column;

		/*  We don't know what the following fields should be, but we
		 *  set them just to be deterministic.  For one thing the
		 *  check_lmap() test can fail if these fields have random
		 *  values.
		 */
		edit__free_rdesc(nextlm);
		nextlm->height = 0;
		nextlm->baseline = 0;
		nextlm->line_continues = FALSE;
	}
}

static void
next_props(buffer, point, p_pc, p_rb, p_fi, p_flags, p_next_pc_point)
Edit_buffer *buffer;
size_t point;
Edit_propchange **p_pc;
Render_block **p_rb;
Edit_fontinfo **p_fi;
Edit_flags *p_flags;
size_t *p_next_pc_point;
{
	if (*p_pc != NULL) {
		*p_pc = edit_get_next_propchange(buffer, *p_pc, point,
						 p_next_pc_point);
		*p_fi = (*p_pc)->fi;
		*p_flags = (*p_pc)->flags;
	}
	else {
		*p_rb = (*p_rb)->next;

		if ((*p_rb)->fi != NULL) {
			*p_fi = (*p_rb)->fi;
			*p_flags = (*p_rb)->flags;
		}
		
		*p_next_pc_point = (*p_rb)->next != NULL
					? (*p_rb)->next->point : EDIT_MAX_POINT;
	}
}
					
static void
expand_linebuf(rd, p_obuf, p_optr, p_olim)
Render_desc *rd;
char **p_obuf, **p_optr, **p_olim;
{
	size_t offset;

	offset = *p_optr - rd->linebuf;
	
	rd->linebuf_size *= 2;
	rd->linebuf = e_realloc(rd->linebuf, rd->linebuf_size);

	*p_obuf = rd->linebuf;
	*p_optr = rd->linebuf + offset;
	*p_olim = rd->linebuf + rd->linebuf_size;
}

static bool
check_cp(cp, cnum, xpos, char_width, fi, flags, point, ch_offset)
Cnum_to_point *cp;
int cnum, xpos, char_width;
Edit_fontinfo *fi;
Edit_flags flags;
size_t point;
int ch_offset;
{
	if (cp->want_point && ch_offset == 0 &&
	    cp->xpos >= xpos && cp->xpos < xpos + char_width) {
		cp->point = point;
		cp->want_point = FALSE;
		return TRUE;
	}

	if (cp->want_cnum && point == cp->point && ch_offset == 0) {
		cp->cnum = cnum;
		cp->xpos = xpos;
		cp->fi = fi;
		cp->flags = flags;
		cp->want_cnum = FALSE;
		return TRUE;
	}

	return FALSE;
}

static void
convert_char(cnum, ch, tabspaces, p_charstr, p_charlen)
size_t cnum;
int ch, tabspaces;
const char **p_charstr;
int *p_charlen;
{
	static char static_buf[10], *buf = static_buf;
	static size_t buflen = sizeof(static_buf);
	size_t n;

	if (ch == '\t' && tabspaces != 0) {
		n = tabspaces - cnum % tabspaces;

		if (n > buflen) {
			if (buf != static_buf)
				free(buf);

			buflen = n + 10;        /* a little slop */
			buf = e_malloc(buflen);
		}
		
		memset(buf, ' ', n);
	}
	else if (ch < 0x20) {
		switch (ch) {
		case '\n':
			memcpy(buf, "\\n", 2);
			break;
		case '\r':
			memcpy(buf, "\\r", 2);
			break;
		case '\t':
			memcpy(buf, "\\t", 2);
			break;
		case '\b':
			memcpy(buf, "\\b", 2);
			break;
		default:
			buf[0] = '^';
			buf[1] = ch | ('A' - 1);
			break;
		}
		n = 2;
	}
	else if (ch == 0177) {
		buf[0] = '^';
		buf[1] = '?';
		n = 2;
	}
	else {
		sprintf(buf, "\\%03o", ch);
		n = 4;
	}

	*p_charstr = buf;
	*p_charlen = n;
}
