/* obj.h - private header file for the obj routines */

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


/* @(#)o_priv.h	1.7 09 Apr 1995 (UKC) */

/* typedefs, structure definitions and externs used with objects */

/*  An object
 */
struct objst {
	struct objst *ob_parent;
	struct objst *ob_child;
	struct objst *ob_next;
	struct objst *ob_prev;
	objid_t ob_code;
	struct fvalst *ob_fval;
	short ob_type;
	short ob_flags;
	short ob_width;
	short ob_depth;
	struct lptrst *ob_lptr;
};

/* bits in ob_flags above
 */
#define OB_NL_AFTER		  01
#define OB_NL_BEFORE		  02
#define OB_LAST_ON_LINE		  04
#define OB_HIGHLIGHTED		 010
#define OB_IN_SEL_LIST		 020
#define OB_USERSEL_PENDING	 040
#define OB_UPDATE_LPTRS		0100
#define OB_SELECTED		0200
#define OB_ONE_FVAL		0400
#define OB_FIXED_SIZE	       01000
#define OB_NO_INDENT	       02000
#define OB_RESIZES	       04000 /* AMC enhancements */

/*  Object type descriptor. Gives information about an object that
 *  is common to all objects of a given type.
 */
struct odescst {
	short od_width,od_depth;
	short od_child_indent;
	short od_nfields;
	short od_flags;
	struct fldlnst *od_fldln;

	obj_can_select_func_t od_can_select;
	obj_select_func_t od_select;
	obj_free_obj_func_t od_free_obj;
	obj_get_size_func_t od_getsize;
	obj_get_color_func_t od_get_color;
	obj_get_name_func_t od_get_name;
	obj_format_func_t od_format_obj;
};
typedef struct odescst odescst_t;

/*  Structure pointing to a line of fdescst structures describing fields.
 *  List of these is pointed to by the Odescst structure for a type.
 *  One per line of fields in the object.
 */
struct fldlnst {
	struct fldlnst *fl_next;
	struct fdescst *fl_fdesc;
	short fl_ldepth;
};

/*  Structure describing a single field of an object type.
 *  One per field.
 */
struct fdescst {
	short fd_width;
	struct fdescst *fd_next;
	short fd_fnum;
	char *fd_user_info;
	obj_edit_func_t fd_edit;
	obj_draw_func_t fd_draw;
	obj_getwidth_func_t fd_getwidth;
	/* AMC enhancements */
	short fd_resizes;	 /* If none-zero, then max width */
	short fd_min_width;      /* If none-zero, then min width */
};

/*  An lptr - see lptrs.c
 */
struct lptrst {
	struct objst *lp_obj;
	int lp_ypos;
	short lp_ldepth;
	struct lptrst *lp_next;
	struct lptrst *lp_prev;
};

/*  Element of the per-object list of fields.
 */
struct fvalst {
	struct fvalst *fv_next;
	fval_t fv_val;
};

/*  Entry in the format character table. Gives the functions to handle
 *  a given format character fo_fch.
 */
struct formatst {
	char fo_fch;
	obj_edit_func_t fo_edit;
	obj_draw_func_t fo_draw;
	obj_getwidth_func_t fo_getwidth;
};

/*  Maximum number of different types of object
 */
#define MAX_OBJTYPES 32

/*  Pointer into the object type descriptor table
 */
extern struct odescst *Odesc;

/*  Maximum number of format characters
 */
#define MAX_FORMATS 50

/*  The format character table
 */
extern struct formatst Formats[];

/*  Number of format characters currently defined
 */
extern int Num_formats;

/*  The viewport used for displaying objects.
 */
extern window_t Obj_wn;

/*  The root of the object tree
 */
extern struct objst Rootobj;

/*  The change callback and data.
 */
extern obj_change_callback_t obj__Change_callback;
extern char *obj__Change_callback_data;

/*  Default foreground and background colors.
 */
extern int _ob_Default_fg_pixel, _ob_Default_bg_pixel;

void _obj_error PROTO((const char *fmt, ...));

void resize_format PROTO((struct odescst *p_odesc, int width));
void update_display PROTO((struct objst *obj));

/* LINTLIBRARY */
