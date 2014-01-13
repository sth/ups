/* va_expr.c - C interpreter expressions in the stack trace */

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


/* @(#)va_expr.c	1.26 24 May 1995 (UKC) */
char ups_va_expr_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <local/ukcprog.h>
#include <mtrprog/alloc.h>
#include <local/wn.h>
#include <local/obj/obj.h>
#include <local/edit/edit.h>

#include "objtypes.h"
#include "ups.h"
#include "symtab.h"
#include "reg.h"
#include "ci.h"
#include "ui.h"
#include "va.h"
#include "target.h"
#include "st.h"
#include "data.h"
#include "exec.h"	/* for SET_EXPR_RESULT_ADDR */
#include "obj_stack.h"
#include "obj_util.h"
#include "obj_misc.h"
#include "obj_wpt.h"
#include "menudata.h"
#include "va_priv.h"
#include "tdr.h"
#include "state.h"

/*  Structure representing a displayed expression
 */
typedef struct dexprst {
	parse_id_t de_parse_id;
	machine_t *de_machine;
	block_t *de_block;
	type_t *de_type;
	int de_typelevel;
	typecode_t de_basecode;
	type_t *de_aggrtype;
	value_t de_value;
	vformat_t de_format;
	bool de_val_changed;
	struct dexprst *de_nextfree;	/* for free list management only */
} dexpr_t;

static void modify_expr PROTO((dexpr_t *de, const char *modifier));
static void expr_edit PROTO((struct drawst dets));
static ci_exec_result_t set_expr_result PROTO((machine_t *ma, taddr_t addr,
					       taddr_t *args,
					       int nwords, taddr_t *p_res,
					       typecode_t ));
static const char *exprval_to_string PROTO((dexpr_t *de, ci_exec_result_t res));
static void expr_draw PROTO((draw_t *dets));
static dexpr_t *dup_dexpr PROTO((dexpr_t *de));
static dexpr_t *make_dexpr PROTO((block_t *bl));
static bool compile_dexpr_code PROTO((dexpr_t *de, const char *estr,
				      int *p_cnum));
static int read_data PROTO((taddr_t addr, void *buf, size_t nbytes));
static int nowrite PROTO((taddr_t unused_addr, const void *unused_buf,
							size_t unused_nbytes));
static bool expr_quitfunc PROTO((objid_t obj, char *new_text,
				 const char **p_display_string));
static bool is_hex_digit PROTO((int c));
static void inc_exprnum PROTO((Edit_display *display));
static void dec_exprnum PROTO((Edit_display *display));
static void adjust_expr_number PROTO((Edit_display *display, bool up));
static void dump_de_to_outwin PROTO((dexpr_t* de));
static taddr_t get_expr_address PROTO((dexpr_t *de));
static size_t get_expr_size PROTO((dexpr_t *de));


/*char Expr_format[] = "%[-]60cx%[-]30cs\n";*/
/* RGA use elastic format ... */
char Expr_format[] = "%[-]60cx%[-]:400cs\n";

/*  These are used to pass back the result of an expression from
 *  the builtin function $set_expr_result() to update_var().
 */
static type_t *Expr_type;
static value_t Expr_value;

#define FN_EXPR_STR	0
#define FN_EXPR_VAL	1

fdef_t Expr_fdefs[] = {
	{ 'x', expr_draw, expr_edit, var_or_expr_getwidth 	},
	{ '\0', NULL, NULL, NULL				}
};

Edit_history* expr_history = NULL;
fnamemap_t Expr_fnamemap[] = {
	{ FN_EXPR_STR,	"expression",	FALSE,	expr_quitfunc, &expr_history	},
	{ 0,		NULL,		FALSE,	NULL,          NULL		}
};

ALLOC_NEW_FREE(static,dexpr_t,dexpr,de_nextfree)

taddr_t
var_or_expr_addr(obj)
objid_t obj;
{
	dexpr_t *de;

	switch (get_object_type(obj)) {
	case OT_VAR:
		return dvar_addr((dvar_t *)obj);
	case OT_EXPR:
		break;
	default:
		panic("objtype botch in vea");
	}

	de = (dexpr_t *)obj;

	if (de->de_type->ty_code != DT_PTR_TO)
		panic("vea on non ptr");

	return de->de_value.vl_addr;
}

/*  Return TRUE iff par is a structure and name is the name of a member
 *  of the structure.
 *
 *  If this is a the case, point *p_v at the var representing the structure
 *  member, and *p_addr to the address of the parent structure.
 *  Also fully dereference the parent.
 */
int
get_member_of_aggr(par, name, p_v, p_addr, btype)
objid_t par;
const char *name;
var_t **p_v;
taddr_t *p_addr;
type_t **btype;
{
	var_t *v;
	int objtype;

	switch (objtype = get_object_type(par)) {
	case OT_VAR:
		*btype = get_basetype(((dvar_t *)par)->dv_var->va_type);
		break;
	case OT_EXPR:
		*btype = ((dexpr_t *)par)->de_type;

		if (*btype == NULL) {
			/*  This happens if we failed to evaluate the
			 *  parent expression (e.g. when a struct pointer
			 *  points to an invalid address after a
			 *  restore_expr().
			 */
			return -1;
		}
		
		if ((*btype)->ty_code == DT_PTR_TO)
			*btype = (*btype)->ty_base;
		break;
	default:
		panic("bad objtype in gma");
		*btype = NULL;	/* to satisfy gcc */
	}

	if ((*btype)->ty_code == TY_STRUCT || (*btype)->ty_code == TY_UNION) {
		for (v = (*btype)->ty_aggr_or_enum->ae_aggr_members; v != NULL; v = v->va_next) {
			if (strcmp(v->va_name, name) == 0) {
				*p_v = v;
				if (objtype == OT_VAR)
					deref_aggr((dvar_t *)par);
				*p_addr = var_or_expr_addr(par);
				return 0;
			}
		}
	}
	return -1;
}

void
expr_getcolor(obj, wn, p_fg, p_bg)
objid_t obj;
int wn;
long *p_fg, *p_bg;
{
	get_value_colors(wn, ((dexpr_t *)obj)->de_val_changed, p_fg, p_bg);
}

static void
expr_draw(dets)
draw_t *dets;
{
	const char *s;

	s = (const char *)dets->dr_fval;
	  /* RGA use wn_wd_ttext() do expr values rescale */
	dets->dr_width = get_decl_len(strlen(s)) * wn_get_sysfont()->ft_width;
	if (dets->selected)
	  wn_wd_ttext(dets->dr_wn, s, dets->dr_x, dets->dr_y, dets->dr_width, dets->dr_bg, dets->dr_fg);
	else
	  wn_wd_ttext(dets->dr_wn, s, dets->dr_x, dets->dr_y, dets->dr_width, dets->dr_fg, dets->dr_bg);
}

static bool
is_hex_digit(c)
int c;
{
	return c != '\0' && (isdigit(c) || strchr("abcdefABCDEF", c) != NULL);
}

static void
inc_exprnum(display)
Edit_display *display;
{
	adjust_expr_number(display, TRUE);
}

static void
dec_exprnum(display)
Edit_display *display;
{
	adjust_expr_number(display, FALSE);
}

static void
adjust_expr_number(display, up)
Edit_display *display;
bool up;
{
	Edit_buffer *buffer;
	Field_edit_info *fe;
	objid_t obj;
	char *text;
	const char *pos, *endnum, *fmt;
	char *new_text;
	bool seen_hex_digit, non_octal;
	int radix, inc, dig_offset, i, junk_cnum;
	long origval;
	size_t point;
	
	buffer = edit_get_buffer(display);

	text = get_buffer_contents(buffer);
	point = edit_get_point(display);
	
	pos = text + point;

	seen_hex_digit = non_octal = FALSE;
	for (dig_offset = 0; is_hex_digit(pos[dig_offset]); ++dig_offset) {
		if (pos[dig_offset] == '8' || pos[dig_offset] == '9')
			non_octal = TRUE;
		else if (!isdigit(pos[dig_offset]))
			seen_hex_digit = TRUE;
	}
	endnum = pos + dig_offset;

	while (pos > text && is_hex_digit(pos[-1])) {
		--pos;
		if (*pos == '8' || *pos == '9')
			non_octal = TRUE;
		if (!isdigit(*pos))
			seen_hex_digit = TRUE;
	}
	
	/*  Determine the radix of the number.
	 */
	if (pos > text + 1 && strncmp(pos - 2, "0x", 2) == 0) {
		pos -= 2;
		radix = 16;
		fmt = "%.*s0x%lx%s";
	}
	else if (*pos == '0' && isdigit(pos[1])) {
		radix = 8;
		fmt = "%.*s0%lo%s";
	}
	else {
		radix = 10;
		fmt = "%.*s%ld%s";
	}

	if (isalpha(*endnum) || !is_hex_digit(*pos) ||
	    (seen_hex_digit && radix != 16) || (non_octal && radix == 8)) {
		errf("Not pointing at a number");
		return;
	}

	if (pos > text && pos[-1] == '-')
		--pos;
	
	inc = 1;
	for (i = 0; i < dig_offset; ++i)
		inc *= radix;	

	origval = strtol(pos, (char **)NULL, 0);

	new_text = strf(fmt,
			pos - text, text, origval + (up ? inc : -inc), endnum);
	
	/*  Point pos at the end of the number in the new string.
	 */
	pos = new_text + (pos - text);
	if (*pos == '-')
		++pos;
	if (radix == 16)
		pos += 2;
	while (is_hex_digit(*pos))
		++pos;

	/*  Try to maintain the same number of digits as before after
	 *  the marker bar.
	 */
	while (pos > new_text && is_hex_digit(pos[-1]) && dig_offset > 0) {
		--dig_offset;
		--pos;
	}

	point = pos - new_text;
	
	wn_updating_off(WN_STDWIN);
	
	edit_delete(buffer, 0, edit_get_buffer_length(buffer));
	edit_insert(buffer, 0, new_text, strlen(new_text), TRUE);

	fe = (Field_edit_info *)edit_get_user_data(display);
	obj = field_edit_get_object(fe);
	
	change_field(obj, FN_EXPR_STR, new_text);
	field_edit_update_orig_text(fe, new_text);
	
	compile_dexpr_code((dexpr_t *)obj, new_text, &junk_cnum);
	update_expr(obj, DF_NONE, TRUE);

	edit_move_point(display, point);

	wn_updating_on(WN_STDWIN);

	free(new_text);
}

static bool
expr_quitfunc(obj, new_text, p_display_string)
objid_t obj;
char *new_text;
const char **p_display_string;
{
	dexpr_t *de;
	int cnum;

	de = (dexpr_t *)obj;
	
	if (!compile_dexpr_code(de, new_text, &cnum)) {
		/* TODO: do something with cnum */
		change_field((objid_t)de, FN_EXPR_VAL, "<error>");
		return FALSE;
	}

	td_set_obj_updating(OBJ_UPDATING_OFF);
	change_field((objid_t)de, FN_EXPR_STR, new_text);
	update_expr((objid_t)de, DF_NONE, TRUE);
	td_set_obj_updating(OBJ_UPDATING_ON);

	note_var_change((objid_t)de);

	return TRUE;
}

static bool
compile_dexpr_code(de, estr, p_cnum)
dexpr_t *de;
const char *estr;
int *p_cnum;
{
	objid_t par;
	taddr_t junk;
	fil_t *fil;
	int lnum;
	char *expansion;
	compile_res_t *cr;
	lexinfo_t lxbuf;

        par = get_code((objid_t)de, OBJ_PARENT);
        if (ups_get_object_type(par) == OT_SFILE)
           fil = (fil_t *)par;
        else
           fil = get_stack_func(par, &junk, &junk, &junk, &junk)->fu_fil;

	lnum = de->de_block->bl_start_lnum;
        
	if ((expansion = macro_expand_string(fil, lnum, estr)) != NULL)
		estr = expansion;

	cr = compile_code(&estr, 1, de->de_block, (char *)de, &lxbuf,
			  "void $start(void) { $set_expr_value((", ")); }",
			  (const char *)NULL, (const char *)NULL);

	if (expansion)
		free(expansion);
	
	if (cr->cr_machine == NULL || cr->cr_parse_id == NULL) {
		free_parse_id_and_machine(cr->cr_parse_id, cr->cr_machine);
		*p_cnum = lxbuf.lx_cnum;
		return FALSE;
	}

	free_parse_id_and_machine(de->de_parse_id, de->de_machine);

	de->de_machine = cr->cr_machine;
	de->de_parse_id = cr->cr_parse_id;

	return TRUE;
}

static void
expr_edit(fdets)
struct drawst fdets;
{
	static Edit_function down_arrow_function = NULL;
	static Edit_function up_arrow_function = NULL;
	Edit_display *display;
	Edit_keymap *keymap;

	display = field_edit_start(&fdets, "expression", (char *)NULL);

	/*  If we are replaying events there is no display object.
	 */
	if (display == NULL)
		return;

	if (down_arrow_function == NULL || up_arrow_function == NULL) {

		const char *rastr = wn_get_default("ReverseArrows");

		if (rastr != NULL && strcmp(rastr, "yes") == 0) {
			down_arrow_function = dec_exprnum;
			up_arrow_function = inc_exprnum;
		}
		else {
			down_arrow_function = inc_exprnum;
			up_arrow_function = dec_exprnum;
		}
	}

	keymap = edit_create_keymap(edit_get_keymap(display));

	edit_add_keymap_control_entry(keymap, 'n', inc_exprnum);
	
	edit_add_keymap_control_entry(keymap, 'p', dec_exprnum);

	edit_add_keymap_entry(keymap, 0, EDIT_DOWN_ARROW, down_arrow_function);
	edit_add_keymap_entry(keymap, 0, EDIT_UP_ARROW, up_arrow_function);

	edit_set_keymap(display, keymap);
}

const char *
expr_getobjname(obj)
objid_t obj;
{
	static char *last = NULL;
	const char *name;
	objid_t obj2;
	int count;

	name = get_field_value(obj, FN_EXPR_STR);

	if (*name == '\0')
		name = "[BLANK]";

	count = 1;
	obj2 = get_code(get_code(obj, OBJ_PARENT), OBJ_CHILD);

	for (; obj2 != NULL; obj2 = get_code(obj2, OBJ_NEXT)) {
		if (obj2 == obj)
			break;
		if (get_object_type(obj2) != OT_EXPR)
			continue;
		if (strcmp(name, get_field_value(obj2, FN_EXPR_STR)) == 0)
			++count;
	}

	if (last != NULL)
		free(last);

	if (count == 1) {
		last = NULL;
		return name;
	}

	last = strf("@%d:%s", count, name);
	return last;
}

char *
expr_format_obj(code)
objid_t code;
{
	return strf("%20s %s", get_field_value(code, FN_EXPR_STR),
					 get_field_value(code, FN_EXPR_VAL));
}

int
check_single_expr(is_add_expr_command)
bool is_add_expr_command;
{
	if (is_add_expr_command && get_num_selected() != 1) {
		errf("Can only add expressions one at a time");
		return -1;
	}

	return 0;
}

/*  Process the return from a var menu.
 */
void
do_expr(obj, cmd, arg)
objid_t obj;
int cmd;
char *arg;
{
	dexpr_t *de;

	de = (dexpr_t *)obj;

	if ( cmd == MR_VAR_ACCELERATOR_ACTION )
	  cmd = va_accelerator_action_from_type(de->de_type, de->de_format);


	switch(cmd) {
	case MR_VAR_STRING:
	    {
		int oldstate;
		typecode_t typecode;

		if (de->de_type == NULL) {
			errf("Can't display illegal expression as string");
			break;
		}

		if (de->de_type->ty_code != DT_PTR_TO &&
		    de->de_type->ty_code != DT_ARRAY_OF)
			typecode = TY_NOTYPE;
		else
			typecode = de->de_type->ty_base->ty_code;

		if (typecode == TY_NOTYPE) {
			errf("Can't display expression as a string");
			break;
		}

		de->de_format = DF_STRING;
		oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
		update_expr(obj, DF_NONE, TRUE);
		td_set_obj_updating(oldstate);
		break;
	    }
	case MR_VAR_SIGNED_DECIMAL:
	case MR_VAR_UNSIGNED_DECIMAL:
	case MR_VAR_SIGNED_OCTAL:
	case MR_VAR_UNSIGNED_OCTAL:
	case MR_VAR_SIGNED_HEX:
	case MR_VAR_UNSIGNED_HEX:
	case MR_VAR_UNSIGNED_BINARY:
	case MR_VAR_ASCII_BYTE:
        case MR_VAR_DEFAULT_FORMAT:
	    {
		int oldstate;

		de->de_format = mval_to_vformat(cmd);

		if (de->de_format == DF_NONE) {
			de->de_format = default_format(de->de_type,
						       de->de_type);
		}
		
		oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
		update_expr(obj, DF_NONE, TRUE);
		td_set_obj_updating(oldstate);
		break;
	    }
	case MR_VAR_DEREF:
		modify_expr(de, "*");
		break;
	case MR_VAR_ADDRESS:
		modify_expr(de, "&");
		break;
	case MR_VAR_EXPAND_LIKE_BEFORE:
		errf("elb NYI");
		break;
	case MR_VAR_EXPAND_COMPLETELY:
		if (de->de_type == NULL) {
			errf("Can't expand illegal expression");
			break;
		}
		if (de->de_type->ty_code != DT_PTR_TO) {
			errf("Expression is not a struct or union pointer");
			break;
		}

		switch(de->de_type->ty_base->ty_code) {
		case TY_STRUCT:
		case TY_UNION:
		    {
			var_t *v;
			v = de->de_type->ty_base->ty_aggr_or_enum->ae_aggr_members;
			for (; v != NULL; v = v->va_next) {
			    /*
			    **   Skip static members, which we display as globals.
			    **   Skip vector tables, if thats what the user wants to see
			    **   the user should use the expand_hierarchy command.
			    */
			    if (   !( v->va_flags & (VA_STATIC_MEMBER|VA_VTBLPTR))
			        && find_var((objid_t)de, v) == NULL)
				    add_var_object(obj, v, OBJ_FIRST_CHILD, FALSE);
			}
			break;
		    }
		case TY_U_STRUCT:
			errf("Can't expand undefined structure");
			break;
		case TY_U_UNION:
			errf("Can't expand undefined structure");
			break;
		default:
			errf("Expression is not a struct or union pointer");
			break;
		}

		break;
	case MR_VAR_SHOW_HIERARCHY:
		expand_hierarchy(obj,de->de_type);
		break;
	case MR_VAR_SHOW_STATICS:
	        expand_statics(de->de_type);
		break;
	case MR_VAR_DUMP_TO_OUTWIN:
		dump_de_to_outwin(de);
		break;
	case MR_VAR_SHOW_SIZE:
	        errf("Cannot show size of an expression");
		break;
	case MR_VAR_DUP:
		add_to_new_selection((objid_t)dup_dexpr(de));
	    	note_var_change((objid_t)de);
		break;
	case MR_VAR_DELETE:
		note_var_change(obj);
		remove_object(obj, OBJ_DESCENDENTS);
		remove_object(obj, OBJ_SELF);
		break;
	case MR_VAR_COLLAPSE:
		note_var_change(obj);
		remove_object(obj, OBJ_CHILDREN);
		break;
	case MR_VAR_COLLAPSE_COMPLETELY:
		note_var_change(obj);
		remove_object(obj, OBJ_DESCENDENTS);
		break;
	case MR_VAR_WANT_TYPEDEFS:
	case MR_VAR_NO_TYPEDEFS:
		/*  Typedefs don't apply to expressions, as the declaration
		 *  is supplied by the user.
		 */
		break;
	case MR_VAR_ADD_WATCHPOINT:
		if ( de->de_basecode == TY_NOTYPE )
		{
			errf( "Can't watch an expression of unknown type" );
		}
		else
		{
			taddr_t addr = get_expr_address(de);
			size_t size = get_expr_size(de);
			const char *decl = get_field_value(obj, FN_EXPR_STR);
			
			add_watchpoint_object(addr, size, decl);
		}
		break;
	case MR_VAR_ACCELERATOR_ACTION:
		/*  if case va_accelerator_action_from_type fails
		 */
		break;
	default:
		errf("bad cmd in de"); /* RGA from panic */
	}
}

void
get_expr_info(obj, p_bl, p_text, p_format)
objid_t obj;
block_t **p_bl;
const char **p_text;
vformat_t *p_format;
{
	dexpr_t *de;

	de = (dexpr_t *)obj;

	*p_bl = de->de_block;
	*p_text = get_field_value(obj, FN_EXPR_STR);
	*p_format = de->de_format;
}

type_t *
get_expr_type(obj)
objid_t obj;
{
	return ((dexpr_t *)obj)->de_type;
}

objid_t
restore_displayed_expr(par, bl, text, format)
objid_t par;
block_t *bl;
const char *text;
vformat_t format;
{
	dexpr_t *newde;
	int junk_cnum;

	newde = make_dexpr(bl);

	/*  Note that we must create the new object before calling
	 *  compile_dexpr_code because that eventually calls
	 *  get_regaddr_for_ci() which needs the object to exist.
	 */
	new_object((objid_t)newde, OT_EXPR, par, OBJ_FIRST_CHILD);

	if (!compile_dexpr_code(newde, text, &junk_cnum)) {
		remove_object((objid_t)newde, OBJ_SELF);
		return NULL;
	}

	set_field_value((objid_t)newde, FN_EXPR_STR, (fval_t)strsave(text));
	set_field_value((objid_t)newde, FN_EXPR_VAL, (fval_t)strsave("0"));

	update_expr((objid_t)newde, format, TRUE);

	return (objid_t)newde;
}

static dexpr_t *
dup_dexpr(de)
dexpr_t *de;
{
	dexpr_t *newde;
	char *estr;
	int junk_cnum;

	newde = make_dexpr(de->de_block);

	/*  Note that we must create the new object before calling
	 *  compile_dexpr_code because that eventually calls
	 *  get_regaddr_for_ci() which needs the object to exist.
	 */
	new_object((objid_t)newde, OT_EXPR, (objid_t)de, OBJ_AFTER);

	estr = (char *)get_field_value((objid_t)de, FN_EXPR_STR);
	if (!compile_dexpr_code(newde, estr, &junk_cnum))
		errf("cdc botch in dd");

	set_field_value((objid_t)newde, FN_EXPR_STR, (fval_t)strsave(estr));
	set_field_value((objid_t)newde, FN_EXPR_VAL, (fval_t)strsave("0"));

	update_expr((objid_t)newde, de->de_format, TRUE);

	return newde;
}

static dexpr_t *
make_dexpr(bl)
block_t *bl;
{
	dexpr_t *de;

	de = new_dexpr();

	de->de_parse_id = NULL;
	de->de_machine = NULL;
	de->de_block = bl;
	de->de_type = NULL;
	de->de_basecode = TY_NOTYPE;
	de->de_typelevel = 0;
	de->de_aggrtype = NULL;
	/* Don't set de_format as we don't have a type yet */
	de->de_val_changed = FALSE;
	de->de_nextfree = NULL;		/* for safety */

	return de;
}

objid_t
add_expr_object(par, bl, text, language, poscode)
objid_t par;
block_t *bl;
const char *text;
language_t language;
int poscode;
{
	dexpr_t *de;
	objid_t obj;
	int junk_cnum;

	/* RGA allow expns in C++ header files of type ".h" - if
	 type is ".H" or ".hh", language will be LANG_CC */

	if (!(language == LANG_C || language == LANG_CC
	      || get_user_lang() == LANG_CC
	      || (language == LANG_UNKNOWN &&
		  get_mainfunc_lang() == LANG_CC))) {
		errf("Sorry, can only add expressions in C or C++");
		return NULL;
	}

	if (*text != '\0') {
	   for (obj = get_code(par, OBJ_CHILD); obj; obj = get_code(obj, OBJ_NEXT)) {
	      if (get_object_type(obj) == OT_EXPR &&
		  strcmp(text, get_field_value(obj, FN_EXPR_STR)) == 0) {
		 return obj;
	      }
	   }
	}

	de = make_dexpr(bl);
	obj = (objid_t)de;

	td_set_obj_updating(OBJ_UPDATING_OFF);
	
	new_object(obj, OT_EXPR, par, poscode);
	set_field_value(obj, FN_EXPR_STR, (fval_t)strsave(text));
	set_field_value(obj, FN_EXPR_VAL, (fval_t)strsave("0"));
	
	if (*text != '\0') {
		de->de_format = DF_NONE;
		
		if (!compile_dexpr_code(de, text, &junk_cnum)) {
			change_field((objid_t)de, FN_EXPR_VAL, "<error>");
			text = NULL;
		}
	}
	
	ensure_visible(obj);

	td_set_obj_updating(OBJ_UPDATING_ON);

	if (text == NULL || *text == '\0') {
		edit_field_obj(obj, FN_EXPR_STR);
	}
	else {
		update_expr(obj, de->de_format, TRUE);
		select_object(obj, TRUE, OBJ_SELF);
	}

	return obj;
}

void
free_displayed_expr(obj)
objid_t obj;
{
	dexpr_t *de;

	de = (dexpr_t *)obj;
	free_parse_id_and_machine(de->de_parse_id, de->de_machine);
	free_dexpr((dexpr_t *)obj);
}

static int
nowrite(unused_addr, unused_buf, unused_nbytes)
taddr_t unused_addr;
constvoidptr unused_buf;
size_t unused_nbytes;
{
	return -1;
}

static int
read_data(addr, buf, nbytes)
taddr_t addr;
void *buf;
size_t nbytes;
{
	return dread(get_current_target(), addr, buf, nbytes);
}

void
update_expr(obj, new_format, change_caused_by_edit)
objid_t obj;
vformat_t new_format;
bool change_caused_by_edit;
{
	taddr_t fp, ap, cfa, junk;
	dexpr_t *de;
	bool lose_descendents;
	type_t *type;
	typecode_t basecode;
	ci_exec_result_t res;
	int typelevel;
	objid_t par;
	const char *newval;

	de = (dexpr_t *)obj;

	if (de->de_machine == NULL) {
		change_field(obj, FN_EXPR_VAL, "<syntax error>");

		/*  This change is always caused by an edit.
		 */
		de->de_val_changed = FALSE;

		return;
	}

	par = get_code(obj, OBJ_PARENT);
	if (ups_get_object_type(par) == OT_SFILE)
		fp = ap = 0;
	else
		get_stack_func(par, &fp, &ap, &junk, &cfa);

	Expr_type = NULL;
	ci_initialise_machine(de->de_machine, TRUE, FALSE,
					(char **)NULL, (char **)NULL);

	res = ci_execute_machine(de->de_machine, fp, ap, cfa,
					read_data, nowrite, set_expr_result);

	if (res != CI_ER_TRAP) {
		switch (res) {
		case CI_ER_READDATA_FAILED:
		case CI_ER_DIVISION_BY_ZERO:
			break;
		default:
			errf("\b%s in expression", ci_exec_result_to_string(res));
			break;
		}
		Expr_type = NULL;
	}

	basecode = TY_NOTYPE;

	typelevel = 0;
	for (type = Expr_type; type != NULL; type = type->ty_base) {
		basecode = type->ty_code;
		++typelevel;
	}

	if (basecode != de->de_basecode || de->de_typelevel != typelevel) {
		if (Expr_type == NULL)
			de->de_format = DF_NONE;
		else if (new_format != DF_NONE)
			de->de_format = new_format;
		else if (Expr_type->ty_code == DT_PTR_TO &&
		         Expr_type->ty_base->ty_code == TY_CHAR)
			de->de_format = DF_STRING;
		else if (Expr_type->ty_code == DT_ARRAY_OF &&
		         Expr_type->ty_base->ty_code == TY_CHAR)
			de->de_format = DF_STRING;
		else
			de->de_format = default_format(Expr_type, Expr_type);
	}

	de->de_type = Expr_type;
	de->de_typelevel = typelevel;
	de->de_basecode = basecode;
	de->de_value = Expr_value;

	lose_descendents = TRUE;
	if (de->de_type == NULL)
		de->de_aggrtype = NULL;
	else if (de->de_type->ty_code == DT_PTR_TO) {
		type_t *aggrtype;

		aggrtype = de->de_type->ty_base;
		if (aggrtype->ty_code == TY_STRUCT ||
						aggrtype->ty_code == TY_UNION) {
			if (de->de_aggrtype != aggrtype)
				de->de_aggrtype = aggrtype;
			else
				lose_descendents = FALSE;
		}
		else
			de->de_aggrtype = NULL;
	}
			
	if (lose_descendents)
		remove_object(obj, OBJ_DESCENDENTS);

	newval = exprval_to_string(de, res);
	de->de_val_changed = change_field(obj, FN_EXPR_VAL, newval) &&
							!change_caused_by_edit;
	
	if (de->de_aggrtype != NULL)
		update_struct(obj, change_caused_by_edit);

	note_var_change(obj);
}

static void
modify_expr(de, modifier)
dexpr_t *de;
const char *modifier;
{
	const char *old;
	int        oldlen;
	char       *new;
	int        junk_cnum;
   
	old = get_field_value( (objid_t)de, FN_EXPR_STR );
	oldlen = strlen(old);
	
	if (old[1] == '(' && old[oldlen-1] == ')' &&
	    ((old[0] == '*' && strcmp(modifier, "&") == 0) ||
	     (old[0] == '&' && strcmp(modifier, "*") == 0))) {
		new = malloc(oldlen - 3);
		memcpy(new, old + 2, oldlen - 3);
		new[oldlen - 3] = '\0';
	}
	else {
		new = malloc(oldlen + strlen(modifier) + 3);
		strcpy(new, modifier);
		strcat(new, "(");
		strcat(new, old);
		strcat(new, ")");
	}

	if (compile_dexpr_code(de, new, &junk_cnum)) {
		td_set_obj_updating(OBJ_UPDATING_OFF);
		change_field((objid_t)de, FN_EXPR_STR, new);
		update_expr((objid_t)de, DF_NONE, TRUE);
		td_set_obj_updating(OBJ_UPDATING_ON);
	}
	else {
		free(new);
	}

	return;
}

	
/*  Construct a value string for the displayed expression de.
 */
static const char *
exprval_to_string(de, res)
dexpr_t *de;
ci_exec_result_t res;
{
	value_t vl;
	target_t *xp;
	type_t *type;
	static char buf[128];

	xp = get_current_target();
	type = de->de_type;
	vl = de->de_value;

	switch (res) {
	case CI_ER_TRAP:
		break;
	case CI_ER_WRITEDATA_FAILED:
		return "<illegal data modification>";
	case CI_ER_READDATA_FAILED:
		return "<bad address>";
	default:
		strnf(buf, sizeof(buf), "<%s>", ci_exec_result_to_string(res));
		return buf;
	}

	if (type == NULL)
		return "<illegal expression type>";

	switch (type->ty_code) {
	case DT_ARRAY_OF:
	case DT_PTR_TO:
		addr_to_string(buf, sizeof(buf) - 1, vl.vl_addr,
			       type, de->de_format);
		break;
	case TY_INT:
	case TY_UINT:
		if (is_signed_format(de->de_format)) {
		    int_to_string(buf, sizeof(buf),
				  (vlong)vl.vl_int,
				  de->de_format);
		}
		else {
		    int_to_string(buf, sizeof(buf),
				  (vlong)(vl.vl_int & (unsigned int)~0),
				  de->de_format);
		}
		break;
	case TY_LONG:
	case TY_ULONG:
		if (is_signed_format(de->de_format)) {
		    int_to_string(buf, sizeof(buf),
				  (vlong)vl.vl_long,
				  de->de_format);
		}
		else {
		    int_to_string(buf, sizeof(buf),
				  (vlong)(vl.vl_long & (unsigned long)~0),
				  de->de_format);
		}
		break;
#if HAVE_LONG_LONG
	case TY_LONGLONG:
	case TY_ULONGLONG:
		int_to_string(buf, sizeof(buf), (vlong)vl.vl_longlong, de->de_format);
		break;
#endif
	case TY_FLOAT:
		strcpy(buf, get_real(xp->xp_words_big_endian, vl, FALSE,
				     sizeof (float)));
		break;
	case TY_DOUBLE:
		strcpy(buf, get_real(xp->xp_words_big_endian, vl, FALSE,
				     sizeof (double)));
		break;
#if HAVE_LONG_DOUBLE
 	case TY_LONGDOUBLE:
 		strcpy(buf, get_real(xp->xp_words_big_endian, vl, FALSE,
 				     sizeof (long double)));
		break;
#endif
	case TY_ENUM:
	case TY_U_ENUM:
		enumval_to_string(buf, sizeof(buf), vl.vl_int, type);
		break;
	default:
		/* panic("unknown type in es"); */
		return "<illegal expression type>";
	}

	return buf;
}

static ci_exec_result_t
set_expr_result(ma, addr, args, nwords, p_res, restype)
machine_t *ma;
taddr_t addr;
taddr_t *args;
int nwords;
taddr_t *p_res;
typecode_t restype;
{
	int argslots;

	if (addr != SET_EXPR_RESULT_ADDR)
		return call_target_function(ma, addr, args, nwords, p_res, restype);
	if (Expr_type != NULL)
		panic("dup call of ser");
	if (nwords < 1)
		panic("bad nwords in ser");

	Expr_type = (type_t *)args[nwords];

	switch (Expr_type->ty_code) {
	case TY_FLOAT:
		argslots = sizeof(float) / sizeof(long);
		Expr_value.vl_double = *(float *)args;
		break;
	case TY_DOUBLE:
		argslots = sizeof(double) / sizeof(long);
		memcpy((char *)&Expr_value.vl_double, (char *)args,
		       sizeof(double));
		break;
#if HAVE_LONG_DOUBLE
	case TY_LONGDOUBLE:
		argslots = sizeof(long double) / sizeof(long);
		memcpy((char *)&Expr_value.vl_longdouble, (char *)args,
		       sizeof(long double));
		break;
#endif
#if HAVE_LONG_LONG
	case TY_LONGLONG:
	case TY_ULONGLONG:
		argslots = sizeof(long long) / sizeof(long);
		memcpy((char *)&Expr_value.vl_longlong, (char *)args,
		       sizeof(long long));
		break;
#endif
	case DT_ARRAY_OF:
	case DT_PTR_TO:
		argslots = sizeof(char *) / sizeof(long);
		Expr_value.vl_addr = args[0];
		break;
	case TY_INT:
	case TY_UINT:
	case TY_ENUM:
	case TY_U_ENUM:
		argslots = 1;
		Expr_value.vl_int = args[0];
		break;
	case TY_LONG:
	case TY_ULONG:
		argslots = 1;
		Expr_value.vl_long = args[0];
		break;
	case TY_STRUCT:
	case TY_UNION:
	case TY_U_UNION:
	case TY_U_STRUCT:
		errf("\bCan't display struct/union types");
		argslots = typesize(Expr_type) / sizeof(long);
		Expr_type = NULL;
		break;
	default:
		/* panic("unknown typecode in ser"); */
		errf("\bCan't display unknown type");
		argslots = 0;	/* to satisfy gcc */
		break;
	}

	if (argslots != nwords)
	        /* panic("argslots botch in ser");*/
		errf("\bargslots botch in ser");
	
	return CI_ER_CONTINUE;
}

static void dump_de_to_outwin(de)
dexpr_t *de;
{
    char buff[80];
    type_t *type  = de->de_type;
    int length = 0;
    char grpchar = ' ';
    taddr_t addr = de->de_value.vl_addr;

    if (   type->ty_code != DT_PTR_TO)
    {
	errf("Can only dump pointer expressions");
	return;
    }
    type = type->ty_base;

    if ( IS_BASIC_TYPE(type->ty_code)
	    || type->ty_code == TY_NOTYPE)    /* For void */
    {
	length = 256;
	switch (ci_typesize(NULL, type))
	{
	case 1:
	    grpchar = 'b'; break;
	case 2:
	    grpchar = 's'; break;
	case 4:
	case 8:
	case 10: /* for long doubles */
	    grpchar = 'l'; break;
	default:
	    grpchar = ' '; break;
	}
    }
    else
    {
	length = ci_typesize(NULL, type);
	if (length < 0 )
	    length = 16;
	switch (type->ty_code)
	{
	case DT_PTR_TO:
	case DT_FUNC_RETURNING:
	case DT_ARRAY_OF:
	case TY_U_ENUM:
	    grpchar = 'l';
	    break;
	default:
            break;
	}
    }

    sprintf(buff,"%%d%c %lx %d", grpchar,addr,length);
    dump_memory (buff);
}

/*  Get the address of a displayed expression.
 */
static taddr_t
get_expr_address(de)
dexpr_t *de;
{
	objid_t par;
	taddr_t fp, ap, cfa, junk;
	fil_t *fil;
	int lnum;
	compile_res_t *cr;
	lexinfo_t lxbuf;
	const char *estr;
	char *expansion;
	taddr_t retval;

	if (de->de_type->ty_code == DT_PTR_TO ||
	    de->de_type->ty_code == DT_ARRAY_OF) {
		return de->de_value.vl_ulong;
	}

	par = get_code((objid_t)de, OBJ_PARENT);
	if (ups_get_object_type(par) == OT_SFILE)
		fil = (fil_t *)par;
        else
		fil = get_stack_func(par, &fp, &ap, &junk, &cfa)->fu_fil;

	lnum = de->de_block->bl_start_lnum;

	estr = get_field_value((objid_t)de, FN_EXPR_STR);

	if ((expansion = macro_expand_string(fil, lnum, estr)) != NULL)
		estr = expansion;

	cr = compile_code(&estr, 1, de->de_block, (char *)de, &lxbuf,
			  "void $start(void) { $set_expr_value(( &(", ") )); }",
			  (const char *)NULL, (const char *)NULL);

	retval = 0;

	if (cr->cr_machine != NULL && cr->cr_parse_id != NULL) {
		taddr_t fp, ap, cfa, junk;
		ci_exec_result_t res;
		objid_t par;
		
		par = get_code((objid_t)de, OBJ_PARENT);
		if (ups_get_object_type(par) == OT_SFILE)
                	fp = ap = 0;
		else
			get_stack_func(par, &fp, &ap, &junk, &cfa);
		
		Expr_type = NULL;
		ci_initialise_machine(cr->cr_machine, TRUE, FALSE,
				      (char **)NULL, (char **)NULL);
		
		res = ci_execute_machine(cr->cr_machine, fp, ap, cfa,
					 read_data, nowrite, set_expr_result);
		
		if (res != CI_ER_TRAP) {
			switch (res) {
			case CI_ER_READDATA_FAILED:
			case CI_ER_DIVISION_BY_ZERO:
				break;
			default:
				errf("%s in expression", ci_exec_result_to_string(res));
				break;
			}
			Expr_type = NULL;
		}
		
		retval = Expr_value.vl_ulong;
	}

	if (expansion)
		free(expansion);

	free_parse_id_and_machine(cr->cr_parse_id, cr->cr_machine);

        return retval;
}

/*  Get the size of a displayed expression.
 */
static size_t
get_expr_size(de)
dexpr_t *de;
{
	objid_t par;
	taddr_t fp, ap, cfa, junk;
	fil_t *fil;
	int lnum;
	compile_res_t *cr;
	lexinfo_t lxbuf;
	const char *estr;
	char *expansion;
	taddr_t retval;

	par = get_code((objid_t)de, OBJ_PARENT);
	if (ups_get_object_type(par) == OT_SFILE)
		fil = (fil_t *)par;
        else
		fil = get_stack_func(par, &fp, &ap, &junk, &cfa)->fu_fil;

	lnum = de->de_block->bl_start_lnum;

	estr = get_field_value((objid_t)de, FN_EXPR_STR);

	if ((expansion = macro_expand_string(fil, lnum, estr)) != NULL)
		estr = expansion;

	if (de->de_type->ty_code == DT_PTR_TO) {
		cr = compile_code(&estr, 1, de->de_block, (char *)de, &lxbuf,
				  "void $start(void) { $set_expr_value(( sizeof(*(", ")) )); }",
				  (const char *)NULL, (const char *)NULL);
	}
	else {
		cr = compile_code(&estr, 1, de->de_block, (char *)de, &lxbuf,
				  "void $start(void) { $set_expr_value(( sizeof(", ") )); }",
				  (const char *)NULL, (const char *)NULL);
	}

	retval = 0;

	if (cr->cr_machine != NULL && cr->cr_parse_id != NULL) {
		ci_exec_result_t res;
		objid_t par;
		
		par = get_code((objid_t)de, OBJ_PARENT);
		if (ups_get_object_type(par) == OT_SFILE)
                	fp = ap = 0;
		else
			get_stack_func(par, &fp, &ap, &junk, &cfa);
		
		Expr_type = NULL;
		ci_initialise_machine(cr->cr_machine, TRUE, FALSE,
				      (char **)NULL, (char **)NULL);
		
		res = ci_execute_machine(cr->cr_machine, fp, ap, cfa,
					 read_data, nowrite, set_expr_result);
		
		if (res != CI_ER_TRAP) {
			switch (res) {
			case CI_ER_READDATA_FAILED:
			case CI_ER_DIVISION_BY_ZERO:
				break;
			default:
				errf("%s in expression", ci_exec_result_to_string(res));
				break;
			}
			Expr_type = NULL;
		}
		
		retval = Expr_value.vl_ulong;
	}

	if (expansion)
		free(expansion);

	free_parse_id_and_machine(cr->cr_parse_id, cr->cr_machine);

        return retval;
}
