/* objtypes.h - ot_t structure and #defines for the various object types */

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


/* @(#)objtypes.h	1.13 09 Apr 1995 (UKC) */

#define OBJTYPES_H_INCLUDED

/*  The object types.  These are used to index an array, so don't change
 *  them.
 */
#define OT_COM		0
#define OT_GLOBALS	1
#define OT_SRCHEAD	2
#define OT_SFILE	3
#define OT_FHEAD	4
#define OT_FUNC		5
#define OT_BLOCK	6
#define OT_FSIG		7
#define OT_BADFUNC	8
#define OT_BADFRAME	9
#define OT_VAR		10
#define OT_EXPR		11
#define OT_BPHEAD	12
#define OT_BPT		13
#define OT_SGHEAD	14
#define OT_SIG		15
#define OT_CBHEAD	16
#define OT_CBLOCK	17
#define OT_MHEAD	18
#define OT_MODULE	19
#define OT_MODULE_FUNC	20
#define OT_ENVHEAD	21
#define OT_ENV		22
#define OT_UNLOADED_LIB	23
#define OT_WPHEAD	24
#define OT_WPT		25

#define OT_MAXTYPE	25

/*  A special type meaning that there are no objects selected.
 */
#define OT_NO_TYPE	(-1)

/*  Ugly special case - expanded source file has a different format
 *  to a collapsed one.
 */
#define OT_SFILE_EX	(OT_MAXTYPE + 1)
#define OT_MODULE_EX	(OT_MAXTYPE + 2)

/*  Field definition table structure.
 *  Table is terminated by a NUL fd_char.
 */
typedef struct fdefst {
	char fd_char;
	obj_draw_func_t fd_draw;
	obj_edit_func_t fd_edit;
	obj_getwidth_func_t fd_getwidth;
} fdef_t;

typedef bool (*Field_edit_quitfunc)PROTO((objid_t obj, char *text,
					  const char **p_display_string));

/*  Structure mapping field names to field numbers.
 *  This is used by the test code in tdr.c.
 */
typedef struct fnamemap_s {
	int fnum;
	const char *name;
	bool null_string_ok;
	Field_edit_quitfunc quitfunc;
	struct Edit_history** edit_history;
} fnamemap_t;

/*  Element in the table giving per object type information.
 */
typedef struct {
	struct mnode *ot_men; /* should be MENU *, but don't want menu3.h */
	const char *ot_menuname;
	const char *ot_menupath;
	int ot_md;

	int (*ot_pre_mfunc) PROTO((int command, char **p_arg));
	void (*ot_mfunc) PROTO((objid_t obj, int command, char *arg));
	void (*ot_post_mfunc) PROTO((int command, char *arg));
	
	const char *ot_format;
	fdef_t *ot_fdefs;
	fnamemap_t *ot_fnamemap;

	obj_select_func_t ot_select;
	obj_can_select_func_t ot_can_select;
	obj_free_obj_func_t ot_free;
	obj_get_size_func_t ot_get_size;
	obj_get_color_func_t ot_get_color;

	obj_get_name_func_t ot_get_name;
	obj_format_func_t ot_format_func;
} ot_t;

/*  The table of object types.
 */
extern ot_t Objtab[];

int get_cur_objtype PROTO((void));
