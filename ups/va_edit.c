/* va_edit.c - editing of variable declarations */

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


/* @(#)va_edit.c	1.31 04 Jun 1995 (UKC) */
char ups_va_edit_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <local/wn.h>
#include <local/obj/obj.h>
#include <local/edit/edit.h>

#include "ups.h"
#include "symtab.h"
#include "objtypes.h"
#include "obj_buildf.h"
#include "target.h"
#include "st.h"
#include "obj_target.h"
#include "obj_util.h"
#include "obj_misc.h"
#include "state.h"
#include "va.h"
#include "ci.h"
#include "ci_util.h" /* RCB: For ci_effective_typecode() */
#include "data.h"
#include "va_priv.h"
#include "ui.h"
#include "tdr.h"

#ifndef __STDC__
double strtod();
#endif

static void var_draw PROTO((draw_t *p_dets));
static bool var_quitfunc PROTO((objid_t obj, char *new_text,
				const char **p_display_string));
static bool value_quitfunc PROTO((objid_t obj, char *new_text,
				  const char **p_display_string));
static void var_edit PROTO((draw_t dets));
static void value_edit PROTO((draw_t dets));
static const char *start_editable PROTO((dvar_t *dv, const char *s));
static bool update_ilist PROTO((dvar_t *dv, const char *decl));
static int get_value PROTO((bool words_big_endian, type_t *type,
			    const char *vstr, value_t *vl,
			    vformat_t *p_vformat));
static int get_integer PROTO((const char *vstr, uvlong *p_val,
			      vformat_t *p_vformat, bool boolean));
static enum_member_t *find_enum_member PROTO((aggr_or_enum_def_t *ae, const char *name));
static int get_float PROTO((const char *vstr, double *p_dval));
static void set_fieldval PROTO((bool bits_big_endian, vlong *p_word, vlong value,
				bitfield_t *bf));
static int get_c_string PROTO((const char *vstr, char **p_s, int *p_len));
static int get_new_string PROTO((const char *orig, const char *vstr,
				 dvar_t *dv, char **p_s, size_t *p_len));
static int write_value PROTO((target_t *xp, vlong val, taddr_t addr, type_t *type));
static int write_new_value PROTO((target_t *xp, taddr_t addr, type_t *type,
				  value_t value, const char *vstr,
				  bool *p_done_mesg));
static int check_addr_for_edit PROTO((target_t *xp, dvar_t *dv));
static int index_offset PROTO((dvar_t *dv));
static int get_hex_float PROTO((bool words_big_endian,
				const char *vstr, bool is_float, int *p_ints));
static void move_var_edit_point PROTO((Edit_display *display, objid_t obj, 
                                       size_t point));
static bool get_index PROTO((const char *iptr,
			     const char **p_iptr, int *p_val));
static void inc_index PROTO((Edit_display *display));
static void dec_index PROTO((Edit_display *display));

/*char Var_format[] = "%50cv%[-]30cV\n";*/
/* RGA use elastic format ... */
char Var_format[] = "%[-]40cv%[-]:400cV\n";

Edit_history* subscript_history = NULL;
Edit_history* value_history = NULL;
fnamemap_t Var_fnamemap[] = {
	{ FN_VAR_DECL,	"subscript",	TRUE,	var_quitfunc, &subscript_history	},
	{ FN_VAR_VALUE,	"value",	TRUE,	value_quitfunc, &value_history	},
	{ 0,		NULL,		FALSE,	NULL,           NULL		},
};

fdef_t Var_fdefs[] = {
	{ 'v', var_draw, var_edit, var_or_expr_getwidth	},
	{ 'V', e_draw, value_edit, NULL			},
	{ '\0', NULL, NULL, NULL			}
};

size_t
get_decl_len(len)
size_t len;
{
	static size_t lentab[] = { 35, 50, 65, 80 };
	int i;

	if (!len)
	  len = lentab[1];
	for (i = 0; i < sizeof lentab / sizeof *lentab; ++i)
		if (lentab[i] > len)
			return lentab[i];
	return len + 1;
}

static void
var_draw(dets)
draw_t *dets;
{
	static int want_ul = -1, want_color = -1;
	static char uldef[] = "UnderlineVars";
	static char colorvar[] = "ColorVars";
	char *def, *s, tmp;
	int x1, x2, y, len, z;
	long hi_fg, hi_bg;
	static long p_fg, p_bg;
	dvar_t *dv;

	if (want_color == -1)
	{
	  want_color = 0;
	  get_variable_colors(dets->dr_wn, &p_bg, &p_fg);
	  def = (char *)wn_get_default(colorvar);
	  if (def && strcmp(def, "off") == 0)
	    want_color = 0;
	  else
	    want_color = 1;
	}
	    
	dv = (dvar_t *) dets->dr_code;
	s = (char *)dets->dr_fval;
	dets->dr_width = get_decl_len(strlen(s)) * wn_get_sysfont()->ft_width;
/* AMC enhancements */
	if (dets->selected)
	  wn_wd_ttext(dets->dr_wn, s, dets->dr_x, dets->dr_y, dets->dr_width,
		      dets->dr_bg, dets->dr_fg);
	else
	  wn_wd_ttext(dets->dr_wn, s, dets->dr_x, dets->dr_y, dets->dr_width,
		      dets->dr_fg, dets->dr_bg);

	if (want_color == 1)
	{
	  len = strlen(dv->dv_var->va_name);
	  get_value_colors(dets->dr_wn, TRUE, &hi_fg, &hi_bg);
	  if (!(hi_fg == dets->dr_fg && hi_bg == dets->dr_bg))
	  {/* turn off when highlighting */
	    wn_text_capture(0, -1, &z); /* turn off */
	    tmp = *(s + dv->dv_ul_start + len);
	    *(s + dv->dv_ul_start + len) = 0;
	    x1 = dets->dr_x + wn_strnwidth(s, dv->dv_ul_start, (font_t *)NULL);
/* AMC enhancements */
	    if (dets->selected)
	      wn_wd_ttext(dets->dr_wn, s + dv->dv_ul_start,
			  x1, dets->dr_y, dets->dr_width-(x1-dets->dr_x),
			  p_bg, p_fg);
	    else
	      wn_wd_ttext(dets->dr_wn, s + dv->dv_ul_start,
			  x1, dets->dr_y, dets->dr_width-(x1-dets->dr_x),
			  p_fg, p_bg);
	    *(s + dv->dv_ul_start + len) = tmp;
	    wn_text_capture(-1, 0, &z); /* resume */
	  }
	}
	if (want_ul == -1)
		want_ul = (def = (char *)wn_get_default(uldef)) != NULL &&
							strcmp(def, "on") == 0;
	if (want_ul) {
		dv = (dvar_t *) dets->dr_code;
		x1 = dets->dr_x + wn_strnwidth(s, dv->dv_ul_start, (font_t *)NULL);
		x2 = x1 + wn_strwidth(dv->dv_var->va_name, (font_t *)NULL);
		y = dets->dr_y + wn_get_sysfont()->ft_height - 2;
		wn_draw_line(dets->dr_wn, x1, y, x2, y, dets->dr_fg);
	}
}

/*  Return the width of the declaration field.  This is the getwidth function
 *  for the %v format character.
 */
/* ARGSUSED */
int
var_or_expr_getwidth(obj, fnum, fval)
objid_t obj;
int fnum;
fval_t fval;
{
	return get_decl_len(strlen((const char *)fval)) * wn_get_sysfont()->ft_width;
}

/*  If the user has pointed at a character with the mouse, force
 *  the marker bar to be within an index number.
 */
static void
move_var_edit_point(display, obj, point)
Edit_display *display;
objid_t obj;
size_t point;
{
	language_t language;
	char *text, *pos, *min, *max, *lo, *hi;

	language = ((dvar_t *)obj)->dv_var->va_language;

	text = get_buffer_contents(edit_get_buffer(display));

	/*  First force the marker bar to be in the editable part of the
	 *  declaration.
	 */
	switch (language) {
	case LANG_C:
	case LANG_CC:
		min = strchr(text, C_AOPEN);
		max = (min == NULL) ? NULL : strchr(min, C_ACLOSE);
		break;
	case LANG_F77:
	case LANG_F90:
		min = strrchr(text, FORTRAN_AOPEN);
		max = strrchr(text, FORTRAN_ACLOSE);
		break;
	default:
		panic("bad language in var_puckfunc");
		min = max = 0; /* to satisfy gcc */
	}
	if (min == NULL || max == NULL)
		panic("missing character in declaration");

	pos = text + point;
	if (pos <= min)
		pos = min + 1;
	if (pos > max)
		pos = max;

	if (language == LANG_C || language == LANG_CC) {
		/*  Find the nearest LBRAC or RBRAC in either direction.
		 */
		for (lo = pos - 1; lo > min; --lo)
			if (IS_LBRAC(*lo) || IS_RBRAC(*lo))
				break;
		for (hi = pos; hi < max; hi++)
			if (IS_LBRAC(*hi) || IS_RBRAC(*hi))
				break;
		
		/*  Force pos to lie between a LBRAC and a RBRAC
		 */
		if (lo <= min && IS_LBRAC(*hi))
			pos = hi + 1;
		else if (IS_RBRAC(*lo) && hi >= max)
			pos = lo;
		else if (IS_RBRAC(*lo) && IS_LBRAC(*hi))
			pos = (hi - pos > pos - lo) ? lo -1 : hi + 1;
		else if (!(IS_LBRAC(*lo) && IS_RBRAC(*hi)))
			panic("bad declaration syntax");

		for (min = pos; !IS_LBRAC(*min); --min)
			;
		for (max = pos; !IS_RBRAC(*max); ++max)
			;
	}

	edit_set_point_limits(display,
			      (size_t)(min + 1 - text),
			      (size_t)(max - text));
	
	edit_move_point(display, (size_t)(pos - text));

	free(text);
}

static const char *
start_editable(dv, s)
dvar_t *dv;
const char *s;
{
	const char *pos;

	switch(dv->dv_var->va_language) {
	case LANG_C:
	case LANG_CC:
		pos = strchr(s, C_AOPEN);
		break;
	case LANG_F77:
	case LANG_F90:
		pos = strchr(s, FORTRAN_AOPEN);
		break;
	default:
		panic("bad language in var_quitfunc");
		pos = 0; /* to satisfy gcc */
	}
	return pos;
}

static bool
get_index(iptr, p_iptr, p_val)
const char *iptr;
const char **p_iptr;
int *p_val;
{
	char *ends;
	int val;
	
	val = strtol(iptr, &ends, 10);
	
	if (ends == iptr || !IS_RBRAC(*ends)) {
		while (*ends != '\0' && !IS_RBRAC(*ends))
			++ends;
		
		errf("Subscript `%.*s' is not a decimal number",
		     ends - iptr, iptr);
		
		return FALSE;
	}

	if (p_iptr != NULL)
		*p_iptr = ends;

	if (p_val != NULL)
		*p_val = val;

	return TRUE;
}

static void
inc_index(display)
Edit_display *display;
{
	adjust_index(display, TRUE);
}

static void
dec_index(display)
Edit_display *display;
{
	adjust_index(display, FALSE);
}

void
adjust_index(display, up)
Edit_display *display;
bool up;
{
	Edit_buffer *buffer;
	int val, level, inc, dig_offset;
	size_t point;
	const char *decl, *pos, *minpos, *maxpos, *cptr;
	dvar_t *dv;
	ilist_t *ilist;
	char *text;
	objid_t obj;
	Field_edit_info *fe;

	fe = (Field_edit_info *)edit_get_user_data(display);
	obj = field_edit_get_object(fe);
	buffer = edit_get_buffer(display);
	
	text = get_buffer_contents(buffer);
	point = edit_get_point(display);
	
	pos = text + point;

	/*  What power of ten should we adjust by?
	 */
	inc = 1;
	for (cptr = pos; isdigit(*cptr); ++cptr)
		inc *= 10;
	dig_offset = cptr - pos;

	/*  Find the start of the number.
	 */
	for (--pos; !IS_LBRAC(*pos); --pos)
		;

	if (!get_index(pos + 1, (const char **)NULL, &val))
		return;

	if (!up)
		inc = -inc;

	val += inc;
	
	dv = (dvar_t *)obj;
	ilist = dv->dv_ilist;
	level = 0;

	/*  First update the ilist so we don't lose existing edits
	 *  to other indices.
	 */
	if (!update_ilist(dv, text))
		return;
	
	switch (dv->dv_var->va_language) {
	case LANG_C:
	case LANG_CC:
		minpos = start_editable(dv, text);
		for (--pos; pos > minpos; --pos) {
			if (IS_LBRAC(*pos)) {	
				level++;
				ilist = ilist->il_next;
			}
		}
		ilist->il_index = val;
		decl = mkdecl(dv);

		pos = start_editable(dv, decl);
		for (;;) {
			if (IS_LBRAC(*pos))
				if (--level < 0)
					break;
			pos++;
		}
		while (!IS_RBRAC(*pos))
			pos++;
		break;

	case LANG_F77:
	case LANG_F90:
		maxpos = text + strlen(text) - 1;
		for (cptr = maxpos; cptr > pos; --cptr) {
			if (*cptr == ',') {
				++level;
				ilist = ilist->il_next;
			}
		}
		ilist->il_index = val;
		decl = mkdecl(dv);

		for (pos = decl + strlen(decl) - 1; ; --pos)
			if (*pos == ',' || *pos == FORTRAN_ACLOSE)
				if (--level < 0)
					break;
		break;

	default:
		panic("bad language in adjust_index");
		decl = NULL;	/* to satisfy gcc */
	}

	while (pos > decl && isdigit(pos[-1]) && dig_offset > 0) {
		--dig_offset;
		--pos;
	}

	free(text);

	/*  BUG: there's an awful lot of display updating going on here.
	 */

	wn_updating_off(WN_STDWIN);
	
	edit_delete(buffer, 0, edit_get_buffer_length(buffer));
	edit_insert(buffer, 0, decl, strlen(decl), TRUE);

	change_field(obj, FN_VAR_DECL, decl);
	field_edit_update_orig_text(fe, get_field_value(obj, FN_VAR_DECL));
	update_var(obj, TRUE);

	td_record_field_edit(obj, FN_VAR_DECL, decl);

	move_var_edit_point(display, obj, (size_t)(pos - decl));

	wn_updating_on(WN_STDWIN);
	
	note_var_change(obj);
}

static bool
update_ilist(dv, decl)
dvar_t *dv;
const char *decl;
{
	const char *cptr;
	ilist_t *ilist;
	int level, ndims, nsubs;
        var_t *v;

        v = dv->dv_var;

        ndims = 0;
	for (ilist = dv->dv_ilist; ilist != NULL; ilist = ilist->il_next)
                ++ndims;

        switch(v->va_language) {
	case LANG_C:
	case LANG_CC:
		/*  Verify that all the indices are legal before updating
		 *  any of the ilist entries.
		 */
                nsubs = 0;
		for (cptr = decl; *cptr != '\0'; ++cptr) {
			if (IS_LBRAC(*cptr)) {
				++cptr;
				if (!get_index(cptr, &cptr, (int *)NULL))
					return FALSE;
                                ++nsubs;
			}

                }
		
		if (nsubs != ndims) {
                        errf("%s subscripts (expected %d, got %d)",
                             (nsubs > ndims) ? "Too many" : "Not enough",
                             ndims, nsubs);
                        return FALSE;
                }
		
		level = 0;
                ilist = dv->dv_ilist;
		
		for (cptr = decl; *cptr != '\0'; ++cptr) {
			if (IS_LBRAC(*cptr)) {
				++cptr;
				if (!get_index(cptr, &cptr, &ilist->il_index))
					panic("decl syntax botch");
				
				ilist = ilist->il_next;
				++level;
			}
		}
		
		if (level != dv->dv_ilevel)
			panic("wrong # indices in ui");
		
		break;
	case LANG_F77:
	case LANG_F90:
		if (IS_FORTRAN(v->va_language) &&
                    get_basetype(v->va_type)->ty_code == TY_CHARACTER) {
                        --ndims;
                }

                /*  Verify that all the indices are legal and that we have the
                 *  right number of indices before updating any of the ilist
                 *  entries.
		 */
                nsubs = 0;
                
		cptr = decl + strlen(decl) - 1;
		do {
			--cptr;
			
			while (*cptr != ',' && *cptr != FORTRAN_AOPEN)
				--cptr;

			if (!get_index(cptr + 1, (const char **)NULL,
				       (int *)NULL))
				return FALSE;

                        ++nsubs;
			
		} while (*cptr != FORTRAN_AOPEN);

                if (nsubs != ndims) {
                        errf("%s subscripts (expected %d, got %d)",
                             (nsubs > ndims) ? "Too many" : "Not enough",
                             ndims, nsubs);
                        return FALSE;
                }
		
		cptr = decl + strlen(decl) - 1;
                ilist = dv->dv_ilist;
                
		do {
			--cptr;
			
			while (*cptr != ',' && *cptr != FORTRAN_AOPEN)
				--cptr;
			
			if (!get_index(cptr + 1, (const char **)NULL,
				       &ilist->il_index))
				return FALSE;

			ilist = ilist->il_next;
		} while (*cptr != FORTRAN_AOPEN);
		break;
	default:
		panic("bad language in ui");
	}

	return TRUE;
}

static bool
var_quitfunc(obj, new_text, p_display_string)
objid_t obj;
char *new_text;
const char **p_display_string;
{
	dvar_t *dv;
	const char *old_decl, *new_decl;

	dv = (dvar_t *)obj;

	if (!update_ilist(dv, new_text))
		return FALSE;

	old_decl = (const char *)get_field_value((objid_t)dv, FN_VAR_DECL);
	new_decl = mkdecl(dv);
	
	if (strcmp(old_decl, new_decl) != 0) {
		set_field_value((objid_t)dv, FN_VAR_DECL,
				(fval_t)strsave(new_decl));
		free((char *)old_decl);
	}
	else if (strcmp(new_text, new_decl) != 0) {
		obj_has_changed((objid_t)dv);
	}
	
	update_var((objid_t)dv, TRUE);

	note_var_change((objid_t)dv);

	*p_display_string = new_decl;
	
	return TRUE;
}

void
get_value_colors(wn, val_changed, p_fg, p_bg)
int wn;
bool val_changed;
long *p_fg, *p_bg;
{
  	static color_t colors[] = {
  		{ -1, 0, 0, 0 }, 
   		{ -1, 0, 0, 0 }
	};
	unsigned long pixel, planes;

	if (wn_use_mono())
	{
	  colors[0].co_pixel = WN_BG(wn);
	  colors[1].co_pixel = WN_FG(wn);
	}
	else
	  if (colors[0].co_pixel == -1) {
		const char *colorname;

		colorname = wn_get_default("HighlightColor");
		if ( colorname == NULL)
		    colorname = "red";
		wn_parse_color(wn, colorname, &colors[1]);

		colors[0].co_pixel = WN_BG(wn);

		if (!wn_use_alloc_color(0))
		{
		  wn_get_pixel_colors(wn, colors, 1);
		  if (wn_get_nplanes(wn) > 1 &&
		      wn_get_pixels_and_planes(wn, 1, 1, TRUE, &pixel,
					       &planes) == 0) {
		    colors[0].co_pixel = pixel;
		    colors[1].co_pixel = pixel | planes;
		    wn_set_pixel_colors(wn, colors, 2);
		  }
		  else
		  {
		    wn_parse_color(wn, colorname, &colors[1]);
		    if (!wn_alloc_color(wn, &colors[1], colorname))
		    {
		      wn_use_alloc_color_for_highlight(1);
		      colors[1].co_pixel = WN_FG(wn);
		    }
		  }
		}
		else
		{
		  wn_parse_color(wn, colorname, &colors[1]);
		  if (!wn_alloc_color(wn, &colors[1], colorname))
		  {
		    wn_use_alloc_color_for_highlight(1);
		    colors[1].co_pixel = WN_FG(wn);
		  }
		}
	}

	if (val_changed) {
		*p_bg = colors[0].co_pixel;
		*p_fg = colors[1].co_pixel;
	}
	else {
		*p_bg = WN_BG(wn);
		*p_fg = WN_FG(wn);
	}
}

void
var_getcolor(obj, wn, p_fg, p_bg)
objid_t obj;
int wn;
long *p_fg, *p_bg;
{
	bool val_changed;

	val_changed = (((dvar_t *)obj)->dv_flags & DVF_VAL_CHANGED) != 0;
	get_value_colors(wn, val_changed, p_fg, p_bg);
}

void
get_variable_colors(wn, p_bg, p_fg)
int wn;
long *p_bg, *p_fg;
{
	static color_t colors[] = {
		{ -1, 0, 0, 0},
		{ -1, 0, 0, 0},
	};

	unsigned long pixel, planes;

	if (wn_use_mono())
	{
	  colors[0].co_pixel = WN_BG(wn);
	  colors[1].co_pixel = WN_FG(wn);
	}
	else
	  if (colors[0].co_pixel == -1) {
		const char *colorname;

		colorname = wn_get_default("VariableColor");
		if ( colorname == NULL)
		  colorname = "purple";

		wn_parse_color(wn, colorname, &colors[1]);

		colors[0].co_pixel = WN_BG(wn);

		if (!wn_use_alloc_color(0))
		{
		  wn_get_pixel_colors(wn, colors, 1);
		  if (wn_get_nplanes(wn) > 1 &&
		      wn_get_pixels_and_planes(wn, 1, 1, TRUE, &pixel, &planes) == 0) {
		    colors[0].co_pixel = pixel;
		    colors[1].co_pixel = pixel | planes;
		    wn_set_pixel_colors(wn, colors, 2);
		  }
		  else
		  {
		    wn_parse_color(wn, colorname, &colors[1]);
		    if (!wn_alloc_color(wn, &colors[1], colorname))
		    {
		      wn_use_alloc_color_for_highlight(1);
		      colors[1].co_pixel = WN_FG(wn);
		    }
		  }
		}
		else
		{
		  wn_parse_color(wn, colorname, &colors[1]);
		  if (!wn_alloc_color(wn, &colors[1], colorname))
		  {
		    wn_use_alloc_color_for_highlight(1);
		    colors[1].co_pixel = WN_FG(wn);
		  }

		}
	  }

	*p_bg = colors[0].co_pixel;
	*p_fg = colors[1].co_pixel;
}

static void
set_fieldval(bits_big_endian, p_word, value, bf)
bool bits_big_endian;
vlong *p_word, value;
bitfield_t *bf;
{
	int nbits;
	int shift;
	uvlong mask;

	nbits = sizeof(uvlong) * 8;

	if (bits_big_endian)
		shift = (bf->bf_type->ty_size * 8) - (bf->bf_offset + bf->bf_width);
	else
		shift = bf->bf_offset;

	mask = 0;
	mask = (~mask >> (nbits - bf->bf_width)) << shift;
	value <<= shift;
	
	*p_word = (*p_word & ~mask) | (value & mask);
}

static int
index_offset(dv)
dvar_t *dv;
{
	int orig_index, level;
	taddr_t addr, base_addr;
	ilist_t *il;

	addr = dvar_addr(dv);

	il = dv->dv_ilist;
	for (level = 0; level < dv->dv_ilevel - 1; ++level) {
		il = il->il_next;
		if (il == NULL)
			panic("level botch in io");
	}
	
	if (!il->il_low_known)
		 panic("il botch in io");
		
	if (il->il_index == il->il_low)
		return 0;
	
	orig_index = il->il_index;

	il->il_index = il->il_low;
	base_addr = dvar_addr(dv);
	il->il_index = orig_index;

	return addr - base_addr;
}

static int
get_new_string(orig, vstr, dv, p_s, p_len)
const char *orig, *vstr;
dvar_t *dv;
char **p_s;
size_t *p_len;
{
	char *s;
	type_t *type, *btype;
	var_t *v;
	int len, oldlen;
	bool longer_ok;

	v = dv->dv_var;
	type = get_type_at_level(v, dv->dv_ilevel);

	oldlen = UNKNOWN_SIZE;

	longer_ok = vstr[0] == '>' && vstr[1] == '>' && vstr[2] == '"';

	if (longer_ok) {
		vstr += 2;
	}
	else {
		/*  If the variable is an array then we can find out how
		 *  many bytes are safe to store.
		 */
		if (dv->dv_ilevel > 0) {
			btype = get_type_at_level(v, dv->dv_ilevel - 1);

			if (btype->ty_code == DT_ARRAY_OF) {
				oldlen = typesize(btype) - index_offset(dv);
				if (oldlen < 0)
					oldlen = UNKNOWN_SIZE;
			}
		}
	
		if (oldlen == UNKNOWN_SIZE) {
			char *oldval;

			if (get_c_string(orig, &oldval, &oldlen) != 0)
				panic("old string botch");
			free(oldval);
		}
	}

	if (get_c_string(vstr, &s, &len) != 0)
		return -1;
	
	/*  If the `string' is contained in a register (i.e. an
	 *  integer being shown as a string) then we can't write
	 *  more that a register's worth back.
	 */
	if (v->va_class == CL_REG && type == v->va_type && len > sizeof(int)) {
		errf("Can't store more than %ld bytes a register",
		     (long)sizeof(int));
		return -1;
	}

	if (len > oldlen && !longer_ok) {
		int xs;

		xs = len - oldlen;
		errf("String %d byte%s too long (use >>\"xxx\" to override)",
						     xs, (xs == 1) ? "" : "s");

		free(s);
		return -1;
	}

	*p_s = s;
	*p_len = len;
	return 0;
}

#define DWRITE_VAR(xp, addr, val) \
			dwrite(xp, addr, (const char *)&val, sizeof(val))

static int
write_value(xp, val, addr, type)
target_t *xp;
vlong val;
taddr_t addr;
type_t *type;
{
	int res = 0;
	value_t vl;
	
	switch (type->ty_size)
	{
	case 1:
		vl.vl_char = val;
		res = DWRITE_VAR(xp, addr, vl.vl_char);
		break;
	case 2:
		vl.vl_short = val;
		res = DWRITE_VAR(xp, addr, vl.vl_short);
		break;
	case 4:
		vl.vl_long = val;
		res = DWRITE_VAR(xp, addr, vl.vl_long);
		break;
#if HAVE_LONG_LONG
	case 8:
		vl.vl_longlong = val;
		res = DWRITE_VAR(xp, addr, vl.vl_longlong);
		break;
#endif
	}

	return res;
}

static int
write_new_value(xp, addr, type, value, vstr, p_done_mesg)
target_t *xp;
taddr_t addr;
type_t *type;
value_t value;
const char *vstr;
bool *p_done_mesg;
{
	int res;
	vlong val;

	*p_done_mesg = FALSE;

	switch (ci_effective_typecode(type->ty_code)) {
	case TY_BITFIELD:
		if (!addr_to_number(&val, addr, type->ty_bitfield->bf_type)) {
			failmesg("Error reading old bitfield value", "", "");
			*p_done_mesg = TRUE;
			res = -1;
			break;
		}
		set_fieldval(xp->xp_bits_big_endian, &val,
#if HAVE_LONG_LONG
			     value.vl_longlong,
#else
			     value.vl_long,
#endif
			     type->ty_bitfield);
		res = write_value(xp, val, addr, type->ty_bitfield->bf_type);
		break;
	case TY_FLOAT:
		res = DWRITE_VAR(xp, addr, value.vl_float);
		break;
#if HAVE_LONG_DOUBLE
 	case TY_LONGDOUBLE:
 		res = DWRITE_VAR(xp, addr, value.vl_longdouble);
 		break;
#endif
	case TY_DOUBLE:
		res = DWRITE_VAR(xp, addr, value.vl_double);
		break;
	case TY_LONG:
	case TY_ULONG:
		res = DWRITE_VAR(xp, addr, value.vl_long);
		break;
#if HAVE_LONG_LONG
 	case TY_LONGLONG:
 	case TY_ULONGLONG:
 		res = DWRITE_VAR(xp, addr, value.vl_longlong);
 		break;
#endif
	case TY_ENUM:
	case TY_U_ENUM:
	case TY_BOOLEAN:
#if HAVE_LONG_LONG
		res = write_value(xp, value.vl_longlong, addr, type);
#else
		res = write_value(xp, value.vl_long, addr, type);
#endif
		break;
	case TY_INT:
	case TY_UINT:
		res = DWRITE_VAR(xp, addr, value.vl_int);
		break;
	case TY_SHORT:
	case TY_USHORT:
		res = DWRITE_VAR(xp, addr, value.vl_short);
		break;
	case TY_CHAR:
	case TY_UCHAR:
		res = DWRITE_VAR(xp, addr, value.vl_char);
		break;
	case DT_PTR_TO:
		res = DWRITE_VAR(xp, addr, value.vl_addr);
		break;
	default:
		errf("Ignoring new value %s", vstr);
		*p_done_mesg = TRUE;
		res = -1;
	}

	return res;
}

static bool
value_quitfunc(obj, new_text, p_display_string)
objid_t obj;
char *new_text;
const char **p_display_string;
{
	dvar_t *dv;
	taddr_t addr;
	type_t *type;
	const char *vstr;
	value_t value;
	int res;
	bool done_mesg;
	target_t *xp;

	dv = (dvar_t *)obj;
	
	xp = get_current_target();
	type = get_type_at_level(dv->dv_var, dv->dv_ilevel);

	addr = dvar_addr(dv);
	if (addr == BAD_ADDR)
		panic("bad addr in vq");
	
#if 0  /* RCB: Not needed, the parser gets the address right */
	if (xp->xp_words_big_endian) {
		int size = typesize(type);
		if (dv->dv_var->va_class == CL_ARG && dv->dv_ilevel == 0 &&
		    size < sizeof(int))
			addr += sizeof(int) - size;
	}
#endif
	
	for (vstr = new_text; isspace(*vstr); ++vstr)
		;

	if (dv->dv_format == DF_STRING) {
		const char *orig;
		char *s;
		size_t len;

		orig = get_field_value((objid_t)dv, FN_VAR_VALUE);
		
		if (get_new_string(orig, vstr, dv, &s, &len) != 0)
			return FALSE;

		res = dwrite(xp, addr, s, len) != 0;
		done_mesg = FALSE;
		free(s);
	}
	else {
		if (get_value(xp->xp_words_big_endian,
			      type, vstr, &value, &dv->dv_format) != 0)
			return FALSE;
		
		res = write_new_value(xp, addr, type, value, vstr, &done_mesg);
	}

	if (res != 0) {
		if (!done_mesg)
			failmesg("Error writing new value to target address space",
				 "", "");
		return FALSE;
	}

	/*  This change may potentially affect any other variable
	 *  in the display (e.g. duplicates displays of this variable,
	 *  expressions using the value of this variable).
	 *  Thus we update all variable values.
	 */
	change_field((objid_t)dv, FN_VAR_VALUE, new_text);
	update_variable_values();

	*p_display_string = (char *)get_field_value((objid_t)dv, FN_VAR_VALUE);
	
	return TRUE;
}

static enum_member_t *
find_enum_member(ae, name)
aggr_or_enum_def_t *ae;
const char *name;
{
	enum_member_t *em;

	for (em = ae->ae_enum_members; em != NULL; em = em->em_next) {
		if (strcmp(em->em_name, name) == 0)
			return em;
	}

	return NULL;
}

/*  BUG: we should have `size_t *p_len' here.
 */
static int
get_c_string(vstr, p_s, p_len)
const char *vstr;
char **p_s;
int *p_len;
{
	char *obuf, *optr;

	if (*vstr++ != '"') {
		errf("Missing `\"' at start of string");
		return -1;
	}
		
	/*  We rely on the fact that all C escape sequences are longer
	 *  than the character they represent, so the real string is
	 *  never longer than the C representation.
	 */
	optr = obuf = e_malloc(strlen(vstr) + 1);

	for (; *vstr != '\0' && *vstr != '"'; ++vstr) {
		int ch;

		if (*vstr == '\\')
			vstr = ci_translate_escape(vstr + 1, &ch);
		else
			ch = *vstr;

		*optr++ = ch;
	}

	if (*vstr == '"') {
		*optr++ = '\0';

		if (vstr[1] != '\0') {
			errf("Extra characters after final `\"' in string");
			free(obuf);
			return -1;
		}
	}

	*p_s = obuf;
	*p_len = optr - obuf;
	return 0;
}

static int
get_value(words_big_endian, type, vstr, vl, p_vformat)
bool words_big_endian;
type_t *type;
const char *vstr;
value_t *vl;
vformat_t *p_vformat;
{
	uvlong val;
	double d;
	int res;
	int boolean = (type->ty_code==TY_BOOLEAN);

	res = 0;

	if (*vstr == '\0') {
		errf("Illegal zero length value");
		return -1;
	}

	switch (ci_effective_typecode(type->ty_code)) {
	case TY_FLOAT:
		if (*vstr == '<') {
			res = get_hex_float(words_big_endian, vstr, TRUE,
					    &vl->vl_int);
		}
		else if (get_float(vstr, &d) == 0) {
			vl->vl_float = d;
		}
		else {
			res = -1;
		}
		break;
	case TY_DOUBLE:
		if (*vstr == '<') {
			res = get_hex_float(words_big_endian, vstr, FALSE,
					    vl->vl_ints);
		}
		else if (get_float(vstr, &d) == 0) {
			vl->vl_double = d;
		}
		else {
			res = -1;
		}
		break;

	case TY_LONG:
	case TY_ULONG:
	case TY_BOOLEAN:
		if (get_integer(vstr, &val, p_vformat, boolean) == 0)
			vl->vl_long = val;
		else
			res = -1;
		break;

#if HAVE_LONG_LONG
	case TY_LONGLONG:
	case TY_ULONGLONG:
		if (get_integer(vstr, &val, p_vformat, boolean) == 0)
			vl->vl_longlong = val;
		else
			res = -1;
		break;
#endif

	case TY_INT:
	case TY_UINT:
		if (get_integer(vstr, &val, p_vformat, boolean) == 0)
			vl->vl_int = val;
		else
			res = -1;
		break;

	case TY_BITFIELD:
	case TY_U_ENUM:
		if (get_integer(vstr, &val, p_vformat, boolean) == 0)
#if HAVE_LONG_LONG
			vl->vl_longlong = val;
#else
			vl->vl_long = val;
#endif
		else
			res = -1;
		break;

	case TY_SHORT:
	case TY_USHORT:
		if (get_integer(vstr, &val, p_vformat, boolean) == 0)
			vl->vl_short = val;
		else
			res = -1;
		break;

	case TY_CHAR:
	case TY_UCHAR:
		if (get_integer(vstr, &val, p_vformat, boolean) == 0)
			vl->vl_char = val;
		else
			res = -1;
		break;
	case TY_ENUM:
		if (isalpha(*vstr) || *vstr == '_') {
			enum_member_t *em;

			em = find_enum_member(type->ty_aggr_or_enum, vstr);
			if (em != NULL)
				vl->vl_int = em->em_val;
			else {
				errf("`%s' is not a member of %s",
						vstr, ci_basetype_name(type));
				res = -1;
			}
		}
		else {
			if (get_integer(vstr, &val, p_vformat, boolean) == 0)
#if HAVE_LONG_LONG
				vl->vl_longlong = val;
#else
				vl->vl_long = val;
#endif
			else
				res = -1;
		}
		break;
	case DT_PTR_TO:
		if (type->ty_base->ty_code == DT_FUNC_RETURNING &&
					(isalpha(*vstr) || *vstr == '_')) {
			func_t *f, *f1;
			const char *pos;
			char *new;

			if ((pos = strchr(vstr, '(')) == NULL)
				new = NULL;
			else {
				new = strf("%.*s", pos - vstr, vstr);
				vstr = new;
			}

			if (find_func_by_name(vstr, &f, &f1, TRUE) != 0) {
				res = -1;
			}
			else {
				vl->vl_addr = st_addr_to_funcptr(f->fu_symtab,
								 f->fu_addr);
			}

			if (new != NULL)
				free(new);
		}
		else {
			if (get_integer(vstr, &val, p_vformat, boolean) == 0)
				vl->vl_addr = val;
			else
				res = -1;
		}
		break;
	default:
		errf("Unknown type");
		res = -1;
	}

	return res;
}

static int
get_float(vstr, p_dval)
const char *vstr;
double *p_dval;
{
	char *ends;

	*p_dval = strtod(vstr, &ends);

	if (*ends != '\0' || ends == vstr) {
		errf("`%s' is not a floating point number", vstr);
		return -1;
	}

	return 0;
}

static int
get_hex_float(words_big_endian, vstr, is_float, p_ints)
bool words_big_endian;
const char *vstr;
bool is_float;
int *p_ints;
{
#define HEX_FLOAT_NDIG		(sizeof(float) * 2)
	/*  See get_real().
	 */
	static const char badf[] = "illegal float ";
	static const char badd[] = "illegal double ";
	size_t ndig, maxdig, lsw, msw;
	char hexbuf[HEX_FLOAT_NDIG + 1];
	const char *vptr, *s, *what;

	vptr = vstr + 1;

	if (strncmp(vptr, badf, sizeof(badf) - 1) == 0)
		vptr += sizeof(badf) - 1;
	else if (strncmp(vptr, badd, sizeof(badd) - 1) == 0)
		vptr += sizeof(badd) - 1;
	
	if (strncmp(vptr, "0x", 2) == 0)
		vptr += 2;
	
	/*  Don't count leading zeroes.
	 */
	while (*vptr == '0' && isxdigit(vptr[1]))
		++vptr;

	for (s = vptr; isxdigit(*s); ++s)
		;
	ndig = s - vptr;
		
	if (*s != '>' || s[1] != '\0' || ndig < 1) {
		errf("`%s' is not a legal hex format floating point number",
									vstr);
		return -1;
	}

	if (is_float) {
		what = "float";
		maxdig = HEX_FLOAT_NDIG;
	}
	else {
		what = "double";
		maxdig = HEX_FLOAT_NDIG * 2;
	}

	if (ndig > maxdig) {
		errf("`%.*s' has %ld digits - max for a %s is %ld",
		     (int)ndig, vptr, (long)ndig, what, (long)maxdig);
		return -1;
	}

	if (ndig < HEX_FLOAT_NDIG) {
		strncpy(hexbuf, vptr, ndig);
		hexbuf[ndig] = '\0';
		lsw = strtol(hexbuf, (char **)NULL, 16);
		msw = 0;
	}
	else {
		memcpy(hexbuf, vptr + (ndig - HEX_FLOAT_NDIG), HEX_FLOAT_NDIG);
		hexbuf[HEX_FLOAT_NDIG] = '\0';
		lsw = strtol(hexbuf, (char **)NULL, 16);

		memcpy(hexbuf, vptr, ndig - HEX_FLOAT_NDIG);
		hexbuf[ndig - HEX_FLOAT_NDIG] = '\0';
		msw = strtol(hexbuf, (char **)NULL, 16);
	}

	if (is_float) {
		p_ints[0] = lsw;
	}
	else  {
		if (words_big_endian) {
			p_ints[0] = msw;
			p_ints[1] = lsw;
		}
		else {
			p_ints[1] = msw;
			p_ints[0] = lsw;
		}
	}

	return 0;
}

static int
get_integer(vstr, p_val, p_vformat, boolean)
const char *vstr;
uvlong *p_val;
vformat_t *p_vformat;
bool boolean;
{
	const char *s, *endpos;
	int radix;
	uvlong val;
	vformat_t vformat;
	bool negative;

	s = vstr;

	negative = *s == '-';
	if (negative)
		++s;

	if (*s == '\'') {
		int res;

		++s;
		if (*s == '\\')
			s = ci_translate_escape(s + 1, &res) + 1;
		else
			res = *s++;

		if (*s != '\'') {
			errf("\"%s\" is not a valid character constant", vstr);
			return -1;
		}
		*p_val = negative ? -res : res;
		*p_vformat = DF_ASCII;
		return 0;
	}

	endpos = NULL;
	radix = 0;		/* to satisfy gcc */
	vformat = DF_SDEC;	/* to satisfy gcc */

	if (*s == '0' && s[1] != '\0') {
		++s;
		if (*s == 'x' || *s == 'X') {
			radix = 16;
			vformat = negative ? DF_SHEX : DF_UHEX;
			++s;
		}
		else if (*s == 'b' || *s == 'B') {
			radix = 2;
			vformat = DF_UBIN;
			++s;
		}
		else if (isdigit(*s)) {
			radix = 8;
			vformat = negative ? DF_SOCT : DF_UOCT;
			++s;
		}
		else
			endpos = s;
	}
	else if (isdigit(*s)) {
		radix = 10;
		vformat = negative ? DF_SDEC : DF_UDEC;
	}
	else
		endpos = s;

	val = 0;
	if (endpos == NULL) {
		for (; *s != '\0'; ++s) {
			int digit;

			/*  We allow spaces in binary format because we
			 *  use them on output, and we want the user to
			 *  be able to submit the output value as input.
			 */
			if (radix == 2 && isspace(*s))
				continue;
			if (!isalnum(*s))
				break;

			digit = *s - '0';

			if (strchr("abcdef", *s) != NULL)
				digit -= 'a' - ('9' + 1);
			if (strchr("ABCDEF", *s) != NULL)
				digit -= 'A' - ('9' + 1);
			
			if (digit < 0 || digit >= radix)
				break;
				
			val = val * radix + digit;
		}
		endpos = s;
	}
	
	if (*endpos != '\0') {
	  if (boolean == TRUE)
	  {
	    if (!strcmp(vstr, "false"))
	    {
	      *p_vformat = vformat;
	      *p_val = 0;
	      return 0;
	    }
	    else
	      if (!strcmp(vstr, "true"))
	      {
		*p_vformat = vformat;
		*p_val = 1;
		return 0;
	      }
	      else
		{
		  errf("`%s' must be `true', `false', `0' or `1'", vstr);
		  return -1;
		}
	  }
	  else
	    {
	      errf("`%s' is not an integral value", vstr);
	      return -1;
	    }
	}

	*p_vformat = vformat;
	*p_val = negative ? -val : val;
	
	return 0;
}

static int
check_addr_for_edit(xp, dv)
target_t *xp;
dvar_t *dv;
{
	taddr_t addr;
	char c;

	addr = dvar_addr(dv);

	if (addr == 0) {
		errf("Can't edit target of NULL pointer");
		return -1;
	}

	if (addr == BAD_ADDR || dread(xp, addr, &c, 1) != 0) {
		errf("Can't edit target of illegal address");
		return -1;
	}

	if (dwrite(xp, addr, &c, 1) != 0) {
		errf("Can't edit value as it is in read-only memory");
		return -1;
	}

	return 0;
}

static void
value_edit(fdets)
draw_t fdets;
{
	dvar_t *dv;
	type_t *type;
	bool can_edit;
	target_t *xp;

	dv = (dvar_t *)fdets.dr_code;
	xp = get_current_target();

	if (xp_get_state(xp) == TS_CORE) {
		errf("Can't edit variable values in a core file");
		wn_wait_for_release_of(fdets.dr_wn, B_ANY);
		return;
	}

	type = get_type_at_level(dv->dv_var, dv->dv_ilevel);
	switch(type->ty_code) {
	case TY_CHAR:
	case TY_UCHAR:
	case TY_SHORT:
	case TY_INT:
	case TY_LONG:
 	case TY_BITFIELD:
	case TY_USHORT:
	case TY_UINT:
	case TY_ULONG:
	case TY_ENUM:
	case TY_U_ENUM:
	case DT_PTR_TO:
	case TY_FLOAT:
	case TY_DOUBLE:
	case TY_LONGLONG:
	case TY_ULONGLONG:
	/* case TY_LONGDOUBLE: */
	case TY_BOOLEAN:
		can_edit = TRUE;
		break;
	default:
		errf("Can't edit value of variable %s", dv->dv_var->va_name);
		can_edit = FALSE;
		break;
	}

	if (!can_edit || check_addr_for_edit(xp, dv) != 0) {
		wn_wait_for_release_of(fdets.dr_wn, B_ANY);
	}
	else {
		field_edit_start(&fdets, "value", (char *)NULL);
	}
}

static void
var_edit(fdets)
draw_t fdets;
{
	static Edit_function down_arrow_function = NULL;
	static Edit_function up_arrow_function = NULL;
	dvar_t *dv;
	Edit_display *display;
	Edit_keymap *keymap;
	Field_edit_info *fe;

	dv = (dvar_t *) fdets.dr_code;
	
	if (dv->dv_ilevel == 0) {
		errf("No editable subscripts in the declaration you pointed at");
		wn_wait_for_release_of(fdets.dr_wn, B_ANY);
		return;
	}

	display = field_edit_start(&fdets, "subscript", (char *)NULL);

	if (display == NULL)
		return;

	if (down_arrow_function == NULL || up_arrow_function == NULL) {

		const char *rastr = wn_get_default("ReverseArrows");

		if (rastr != NULL && strcmp(rastr, "yes") == 0) {
			down_arrow_function = dec_index;
			up_arrow_function = inc_index;
		}
		else {
			down_arrow_function = inc_index;
			up_arrow_function = dec_index;
		}
	}

	fe = (Field_edit_info *)edit_get_user_data(display);
	field_edit_set_move_point_func(fe, move_var_edit_point);

	keymap = edit_create_keymap(edit_get_keymap(display));

	edit_add_keymap_entry(keymap, 0, '>', inc_index);
	edit_add_keymap_entry(keymap, 0, 'j', inc_index);
	edit_add_keymap_control_entry(keymap, 'n', inc_index);
	edit_add_keymap_entry(keymap, 0, EDIT_DOWN_ARROW, down_arrow_function);

	edit_add_keymap_entry(keymap, 0, '<', dec_index);
	edit_add_keymap_entry(keymap, 0, 'k', dec_index);
	edit_add_keymap_control_entry(keymap, 'p', dec_index);
	edit_add_keymap_entry(keymap, 0, EDIT_UP_ARROW, up_arrow_function);

	edit_set_keymap(display, keymap);
	
	move_var_edit_point(display, (objid_t)fdets.dr_code,
			    edit_get_point(display));
}

objid_t
find_var(par, v)
objid_t par;
var_t *v;
{
	dvar_t *dv;

	dv = (dvar_t *) get_code(par, OBJ_CHILD);
	for (; dv != NULL; dv = (dvar_t *) get_code((objid_t)dv, OBJ_NEXT))
		if (dv->dv_var == v)
			return (objid_t)dv;
	return NULL;
}

void show_dv_size(dv)
     dvar_t* dv;
{
  type_t *type, *btype;
  var_t *v;
  int len;

  v = dv->dv_var;
  type = get_type_at_level(v, dv->dv_ilevel);
  
  len = UNKNOWN_SIZE;
  /*  If the variable is an array then we can find out how
		 *  many bytes are safe to store.
		 */
  if (dv->dv_ilevel > 0)
  {
    btype = get_type_at_level(v, dv->dv_ilevel - 1);

    if (btype->ty_code == DT_ARRAY_OF) 
      len = typesize(btype) - index_offset(dv);
    else
      len = typesize(btype);
    if (len < 0)
      len = UNKNOWN_SIZE;
  }
  else
  {
    btype = get_type_at_level(v, 0);
    if (btype->ty_code == DT_ARRAY_OF) 
      len = typesize(btype) - index_offset(dv);
    else
      len = typesize(btype);
    if (len < 0)
      len = UNKNOWN_SIZE;
  }
  if (len == UNKNOWN_SIZE)
    errf("\bUnknown size");
  else
    errf("\bSize is %d bytes", len);
}

void
remove_all_vars()
{
  objid_t obj, next_obj = NULL;
  int oldstate;
  
  oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
  for (obj = get_code(SRCHEAD_OBJCODE, OBJ_CHILD); obj != NULL;)
  {
    next_obj = get_code(obj, OBJ_NEXT);
    remove_object(obj, OBJ_DESCENDENTS);
    remove_object(obj, OBJ_SELF);
    obj = next_obj;
  }
  td_set_obj_updating(oldstate);
}


