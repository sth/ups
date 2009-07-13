/* obj_stack.c - functions for displaying the stack */

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


/* @(#)obj_stack.c	1.21 20 Jun 1995 (UKC) */
char ups_obj_stack_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <signal.h>

#include <local/ukcprog.h>
#include <mtrprog/alloc.h>
#include <mtrprog/hash.h>

#include <local/wn.h>
#include <local/obj/obj.h>
#include <local/obj/o_priv.h>
#include <local/obj/o_sel.h>
#include <local/obj/o_mkobj.h>

#include "objtypes.h"
#include "ups.h"
#include "symtab.h"
#include "ao_syms.h"
#include "srcwin.h"
#include "target.h"
#include "st.h"
#include "obj_stack.h"
#include "data.h"
#include "ui.h"
#include "va.h"
#include "expr.h"
#include "obj_signal.h"
#include "obj_misc.h"
#include "menudata.h"
#include "debug.h"
#include "tdr.h"
#include "state.h"
#include "exec.h"

#ifdef AO_ELF
#include "ao_elflib.h"
#endif
#if defined(AO_AOUT_SHLIBS)
#include "ao_shlib.h"
#endif

/*  Arguments block for label_if_var_addr.
 */
typedef struct label_if_addr_argsst {
	func_t *li_func;
	Stack *li_stk;
	taddr_t li_addr;
	char *li_buf;
} label_if_var_addr_args_t;

/*  A block of a function in a stack.
 */
typedef struct stackblockst {
	Stack *sb_stk;
	block_t *sb_block;
} stackblock_t;

#ifdef CHECK_STACK
static void check_stack PROTO((void));
#endif
static char *badfuncname PROTO((taddr_t addr));
static void line_draw PROTO((struct drawst *dets));
static objid_t add_sig_markers PROTO((target_t *xp));
static void add_func_vars PROTO((objid_t par));
static int remove_stack_object_if_not_a_func PROTO((objid_t par, fval_t unused_arg));
static void set_func_fields PROTO((func_t *f, fil_t *fil,
				   int lnum, fval_t *fields));
static void expand_block PROTO((objid_t par, block_t *block));
static void label_if_var_addr PROTO((var_t *v, char *args));
static Stack *stackpos_to_stk PROTO((stackpos_t pos));
static void pathname_draw PROTO((struct drawst *dets));
static bool is_non_empty_block PROTO((block_t *par));
static Stack *obj_to_stack PROTO((objid_t obj));
static void check_for_f90_rterr PROTO((target_t *xp, Stack *stk));

/*  Pointers to the outermost and innermost frame in the list of stack frames.
 *  NULL if there is no stack trace.
 */
static Stack *Outer_stack = NULL;
static Stack *Inner_stack = NULL;

/*  Pointer to the stack frame of the "current function".  This is the
 *  function that next and step refer to, and where local variables
 *  are initially searched for.
 */
static Stack *Current_stack = NULL;

taddr_t
get_outer_fp()
{
	if (Outer_stack == NULL)
		panic("bad stack in gofp");
	return Outer_stack->stk_fp;
}

static void
label_if_var_addr(v, args)
var_t *v;
char *args;
{
	label_if_var_addr_args_t *li;

	li = (label_if_var_addr_args_t *)args;

	if ((v->va_class == CL_REF || v->va_class == CL_ARG) &&
				li->li_stk->stk_ap + v->va_addr == li->li_addr)
		(void) sprintf(li->li_buf, "%s:%s", li->li_func->fu_name,v->va_name);
	if (v->va_class == CL_AUTO && li->li_stk->stk_fp + v->va_addr == li->li_addr)
		(void) sprintf(li->li_buf, "%s:%s", li->li_func->fu_name,v->va_name);
}

/*  If address addr is in the stack and matches a local variable, parameter, etc
 *  then return a label for the address.  This function is used to make
 *  debugging dumps of target stacks more useful.
 *
 *  We return a zero length string if there is no label.
 */
const char *
stack_addr_label(addr, p_is_labelled)
taddr_t addr;
bool *p_is_labelled;
{
	static char buf[100];
	Stack *stk, *above_stk, *below_stk;
	label_if_var_addr_args_t libuf;
	func_t *f;

	*p_is_labelled = FALSE; /* but may be changed later */

	above_stk = below_stk = NULL;
	for (stk = Outer_stack; stk != NULL; stk = stk->stk_inner) {
		f = stk->stk_func;
		if (stk->stk_fp == addr) {
			(void) sprintf(buf, "[%4x %s:fp]",
					     (int)addr & 0xffff, f->fu_demangled_name);
			return buf;
		}
		if (stk->stk_ap == addr) {
			(void) sprintf(buf, "%s:<ap>", f->fu_demangled_name);
			*p_is_labelled = TRUE;
			return buf;
		}

		*buf = '\0';
		libuf.li_func = f;
		libuf.li_stk = stk;
		libuf.li_addr = addr;
		libuf.li_buf = buf;
		iterate_over_vars_of_block(FU_BLOCKS(f), label_if_var_addr,
								(char *)&libuf);
		if (*buf != '\0') {
			*p_is_labelled = TRUE;
			return buf;
		}

		if (stk->stk_fp > addr)
			above_stk = stk;
		if (stk->stk_fp < addr && below_stk == NULL)
			below_stk = stk;
	}

	if (above_stk != NULL && above_stk->stk_fp - addr < 256)
		(void) sprintf(buf, "%4x+%02x ",
			       (int)above_stk->stk_fp & 0xffff,
			       (int)(above_stk->stk_fp - addr));
	else
		(void) sprintf(buf, "%8s", "");

	if (below_stk != NULL && addr - below_stk->stk_fp < 256)
		(void) sprintf(buf + strlen(buf), "%4x-%02x",
					       (int)below_stk->stk_fp & 0xffff,
					       (int)(addr - below_stk->stk_fp));
	else
		(void) sprintf(buf, "%7s", "");

	return buf;
}

/*  Return the "current" function, and the pc and fp values for that
 *  function.
 */
void
get_current_func(p_func, p_fp, p_pc, p_adjusted_pc)
func_t **p_func;
taddr_t *p_fp, *p_pc, *p_adjusted_pc;
{
	if (Current_stack == NULL) {
		*p_func = NULL;
	}
	else {
		*p_func = Current_stack->stk_func;
		*p_pc = Current_stack->stk_pc;
		*p_fp = Current_stack->stk_fp;
		*p_adjusted_pc = *p_pc;

		/*  If this is not the innermost function the pc is
		 *  pointing just after a call instruction.  This means
		 *  it may already be pointing at the next line, so
		 *  drop it by one to ensure that it's pointing at the
		 *  right line.
		 */
		if (Current_stack->stk_inner != NULL)
			--*p_adjusted_pc;
	}
}

/*  Return the "current" function, and the pc and fp values for that
 *  function.
 */
void
get_current_func_2(p_func, p_fp, p_pc, p_fil, p_lnum)
func_t **p_func;
taddr_t *p_fp, *p_pc;
fil_t **p_fil;
int *p_lnum;
{
	if (Current_stack == NULL) {
		*p_func = NULL;
	}
	else {
	  Stack *stk;

	  for (stk = Current_stack; stk != NULL; stk = stk->stk_inner)
	  {
	    if (!stk->stk_inner)
	      /*	    if (stk->stk_lnum != 0 && stk->stk_fil != NULL)*/
	    {
	      *p_func = stk->stk_func;
	      *p_pc = stk->stk_pc;
	      *p_fp = stk->stk_fp;
	      /*	      *p_fil = stk->stk_fil;*/
	      /*	      *p_lnum = stk->stk_lnum;*/
	    }
	  }
	}
}

char *
get_current_stack_data()
{
	return (Current_stack != NULL) ? Current_stack->stk_data : NULL;
}

static char *
badfuncname(addr)
taddr_t addr;
{
	char buf[256];
	char *unload_lib = NULL;

#ifdef AO_ELF
	unload_lib = get_unloaded_lib(addr);
#endif 
#if defined(AO_AOUT_SHLIBS)
	unload_lib = get_unloaded_lib(addr);
#endif 
	if (unload_lib)
	  (void) sprintf(buf, "<%s>", unload_lib);
	if (!unload_lib)
	  (void) sprintf(buf, "<bad text address 0x%lx>", addr);
	return strsave(buf);
}

/*  The draw routine for the format character %l.
 */
static void
line_draw(dets)
register struct drawst *dets;
{
  if (dets->selected)
    wn_draw_line(dets->dr_wn,
		 dets->dr_x,
		 dets->dr_y+dets->dr_depth/2,
		 dets->dr_x + dets->dr_width,
		 dets->dr_y+dets->dr_depth/2,
		 WN_BG(dets->dr_wn));
  else
    wn_draw_line(dets->dr_wn,
		 dets->dr_x,
		 dets->dr_y+dets->dr_depth/2,
		 dets->dr_x + dets->dr_width,
		 dets->dr_y+dets->dr_depth/2,
		 WN_FG(dets->dr_wn));
}

fdef_t Fsig_fdefs[] = {
	{ 'l', line_draw, no_edit, NULL	},
	{ '\0', NULL, NULL, NULL	},
};

const char Fsig_format[] = "%*80l %20cs %*80l\n";
#define FN_FSIG_SIGNAME 0

char *
fsig_format_obj(code)
objid_t code;
{
	return strf("%s", get_field_value(code, FN_FSIG_SIGNAME));
}

void
free_fsig(obj)
objid_t obj;
{
	free((char *)get_field_value(obj, FN_FSIG_SIGNAME));
}

static objid_t
add_sig_markers(xp)
target_t *xp;
{
	Stack *stk, *last;
	int lastsig;

	last = NULL;
	for (stk = Outer_stack; stk != NULL; stk = stk->stk_inner) {
		last = stk;
		if (stk->stk_siginfo != NULL) {
			Siginfo *si;
			const char *tag;

			si = stk->stk_siginfo;
			new_object((objid_t)si, OT_FSIG, (objid_t)stk,
								OBJ_BEFORE);
			tag = xp_get_signal_tag(xp, si->si_signo, NULL);
			set_field_value((objid_t)si, FN_FSIG_SIGNAME,
							(fval_t)strsave(tag));
		}
	}

	lastsig = xp_get_lastsig(xp);
	if (last != NULL && lastsig != 0) {
		const siginfo_t *lastsiginfo;
		const char *tag;

		lastsiginfo = xp_get_lastsiginfo(xp);
		tag = xp_get_signal_tag(xp, lastsig, lastsiginfo);
		new_object((objid_t)tag, OT_FSIG, (objid_t)last, OBJ_AFTER);
		set_field_value((objid_t)tag, FN_FSIG_SIGNAME,
							(fval_t)strsave(tag));

		return (objid_t)tag;
	}

	return NULL;
}


/*  Delete stack objects other then functions.  This function is passed
 *  to visit_objects() to clean the old stack before building a new one.
 */
static int
remove_stack_object_if_not_a_func(obj, unused_arg)
objid_t obj;
fval_t unused_arg;
{
	int objtype; 

	objtype = get_object_type(obj);
	if (objtype != OT_FUNC && objtype != OT_BADFUNC &&
	    objtype != OT_UNLOADED_LIB) {
		remove_object(obj, OBJ_DESCENDENTS);
		remove_object(obj, OBJ_SELF);
	}
	return 0;
}

/*  The draw routine for the format character %p.
 */
static void
pathname_draw(dets)
struct drawst *dets;
{
	int x, len, maxlen;
	bool too_long;
	char *s;
	font_t *font;
	
	font = wn_get_sysfont();

	s = (char *)dets->dr_fval;
	len = strlen(s);
	maxlen = dets->dr_width / font->ft_width;

	too_long = len > maxlen;
	if (too_long) {
		s += len - maxlen;
		len = maxlen;
	}

	x = dets->dr_x;
	if (dets->dr_user_info == NULL || dets->dr_user_info[0] != '-')
		x += dets->dr_width - len * font->ft_width;

	if (too_long) {
		static const char dots[] = "...";

		s += sizeof(dots) - 1;
		if (*s == '/' && s[1] != '/') {
			++s;
			x += font->ft_width;
		}
/* AMC enhancements */
	  if (dets->selected)
	    wn_wd_ttext(dets->dr_wn, dots, x, dets->dr_y,
			dets->dr_width, dets->dr_bg, dets->dr_fg);
	  else
	    wn_wd_ttext(dets->dr_wn, dots, x, dets->dr_y,
			dets->dr_width, dets->dr_fg, dets->dr_bg);
	  x += (sizeof(dots) - 1) * font->ft_width;
	}

/* AMC enhancements */
	if (dets->selected)
	  wn_wd_ttext(dets->dr_wn, s, x, dets->dr_y,dets->dr_width, dets->dr_bg, dets->dr_fg);
	else
	  wn_wd_ttext(dets->dr_wn, s, x, dets->dr_y,dets->dr_width, dets->dr_fg, dets->dr_bg);
}

fdef_t Func_fdefs[] = {
	{ 'p', pathname_draw, no_edit, NULL	},
	{ '\0', NULL, NULL, NULL		},
};

/*const char Func_format[] = "%[-]32cp%25cp%[-]10cs\n";*/
/* AMC enhancements */
/*   Important to allow at least 4 chars in pathname so insertion of
**   ... in pathname_draw does not back up to before start of string!
*/
const char Func_format[] = "%[-]:4c:50cp%25cp%[-]17cs\n";

#define FN_FUNC_NAME	0
#define FN_FUNC_FILE	1
#define FN_FUNC_LNUM	2
#define FN_FUNC_LAST	3

static void
set_func_fields(f, fil, lnum, fields)
func_t *f;
fil_t *fil;
int lnum;
fval_t *fields;
{
	fields[FN_FUNC_NAME] = (fval_t)f->fu_demangled_name; /* RGA */

	/*  The test for lnum==0 etc is really just to make the test scripts
	 *  easier.  Without this frig you get stack entries like
	 *  `abort.s:0'.  Doing this is easier than putting {?xxx|yyy}
	 *  in all the scripts.
	 */
	if ((f->fu_flags & FU_BAD) != 0) {
		fields[FN_FUNC_FILE] = (fval_t)"";
		fields[FN_FUNC_LNUM] = (fval_t)strsave("");
	}
	else if ((f->fu_flags & FU_NOSYM) != 0 || lnum == 0) {
		const char *objname = base_name(f->fu_symtab->st_path);

		fields[FN_FUNC_FILE] = (fval_t)objname;

		if ((f->fu_flags & FU_NOSYM) != 0)
			fields[FN_FUNC_LNUM] = (fval_t)strsave(" (no symbols)");
		else
			fields[FN_FUNC_LNUM] = (fval_t)strsave(" (no line numbers)");
	}
	else {
		fields[FN_FUNC_FILE] = (fval_t)fil->fi_name_only;
		fields[FN_FUNC_LNUM] = (fval_t)strf(":%d", lnum);
	}

	fields[FN_FUNC_LAST] = (fval_t) NULL;
}

const char Badfunc_format[] = "%[-]30cs\n";
const char Nolibfunc_format[] = "%[-]60cs\n";
#define FN_BADFUNC_NAME	0

/*  Add function objects to the display tree.
 */
objid_t
add_functions(xp, par)
target_t *xp;
objid_t par;
{
	Stack *stk, *old_current_stack, *source_stk, *next_stk, *old_stk;
	Stack *first_new_stk;
	Stack *prev_stk;
	objid_t outer, obj, next, newcode, obj_to_make_visible;
	bool old_current_stack_was_innermost;
	fval_t fields[FN_FUNC_LAST + 1];

	source_stk = NULL;

	visit_objects(par, OBJ_CHILDREN, remove_stack_object_if_not_a_func,
						(fval_t)0, (objid_t *)NULL);

	old_stk = Outer_stack;
	
	Outer_stack = xp_get_stack_trace(xp);
	check_for_f90_rterr(xp, Outer_stack);
	
	outer = NULL;
	first_new_stk = NULL;
	old_current_stack = Current_stack;
	old_current_stack_was_innermost = Current_stack == NULL ||
					  Current_stack->stk_inner == NULL;
	Current_stack = NULL;

	/*  Is Current_stack the innermost stack frame possible?  We need
	 *  to know this because if so, we will maintain that - i.e. we
	 *  will move Current_stack in if new inner stack frames appear.
	 *
	 *  We can't just check for Current_stack->stk_inner being NULL
	 *  as there may be stack frames without source below Current_stack.
	 */
	old_current_stack_was_innermost = TRUE;
	if (Current_stack != NULL) {
		for (stk = Current_stack->stk_inner; stk != NULL;
							 stk = stk->stk_inner) {
			if (stk->stk_lnum != 0 && stk->stk_fil != NULL) {
				old_current_stack_was_innermost = FALSE;
				break;
			}
		}
	}
	
	for (stk = Outer_stack; stk != NULL; stk = next_stk) {
		next_stk = stk->stk_inner;
		set_func_fields(stk->stk_func, stk->stk_fil, stk->stk_lnum,
				fields);
		newcode = NULL;
		
		if (first_new_stk == NULL) {
			if (old_stk == NULL ||
			    old_stk->stk_func != stk->stk_func ||
			    (old_stk->stk_func->fu_flags & FU_BAD) != 0) {
				first_new_stk = stk;
			}
			else {
				*old_stk = *stk;
				stk->stk_siginfo = NULL;

				if (outer == NULL)
					Outer_stack = old_stk;
				else
					((Stack *)outer)->stk_inner = old_stk;

				newcode = (objid_t)old_stk;
				
				if (old_stk == old_current_stack)
					Current_stack = old_stk;
				old_stk = (Stack *)get_code(newcode, OBJ_NEXT);
				
				destroy_stk(stk);
			}
		}

		if (newcode != NULL) {
			free((char *)get_field_value(newcode, FN_FUNC_LNUM));
		}
		else {
			newcode = (objid_t)stk;
			
			if (outer == NULL)
				new_object(newcode, OT_FUNC, par, OBJ_CHILD);
			else
				new_object(newcode, OT_FUNC, outer, OBJ_AFTER);
		}

		stk = (Stack *) newcode;

		if (stk->stk_lnum != 0 && stk->stk_fil != NULL)
			source_stk = stk;

		if ((stk->stk_func->fu_flags & FU_BAD) != 0) {
		  char *unload_lib = NULL;
#ifdef AO_ELF
		  unload_lib = get_unloaded_lib(stk->stk_pc);
#endif 
#if defined(AO_AOUT_SHLIBS)
		  unload_lib = get_unloaded_lib(stk->stk_pc);
#endif 
		  if (unload_lib)
		    change_type(newcode, OT_UNLOADED_LIB);
		  else
		    change_type(newcode, OT_BADFUNC);
		  set_field_value(newcode, FN_BADFUNC_NAME,
				  badfuncname(stk->stk_pc));
		  free((char *)fields[FN_FUNC_LNUM]);
		}
		else {
			set_all_fields(newcode, fields, (fval_t)NULL);
		}

		outer = newcode;
	}

	obj_to_make_visible = add_sig_markers(xp);

	if (Current_stack == NULL || old_current_stack_was_innermost)
		Current_stack = source_stk;

	if (source_stk != NULL) {
		set_and_display_stopline(source_stk->stk_fil,
					 source_stk->stk_lnum);
	}
	else {
		set_and_display_stopline((fil_t *)NULL, 0);
	}

	/*  Remove any objects remaining from the old stack trace.
	 */
	for (obj = (objid_t)old_stk; obj != NULL; obj = next) {
		next = get_code(obj, OBJ_NEXT);
		remove_object(obj, OBJ_DESCENDENTS);
		remove_object(obj, OBJ_SELF);
	}

	/*  If the stack went bad, add an object to indicate that.
	 */
	if (Outer_stack == NULL || Outer_stack->stk_bad) {
		static char badstackbuf[50]; /* RGA make static for purify */

		(void) sprintf(badstackbuf, "<bad frame pointer 0x%lx>", 
				(Outer_stack != NULL) ? Outer_stack->stk_fp
						      : xp_getreg(xp, UPSREG_FP));
		new_object((objid_t)badstackbuf, OT_BADFRAME, par, OBJ_CHILD);
		set_field_value((objid_t)badstackbuf, 0, strsave(badstackbuf));
	}

	/*  Build the backwards links.
	 */
	prev_stk = NULL;
	for (stk = Outer_stack; stk != NULL; stk = stk->stk_inner) {
		stk->stk_outer = prev_stk;
		prev_stk = stk;
	}
	Inner_stack = prev_stk;

	for (stk = first_new_stk; stk != NULL; stk = stk->stk_inner)
		restore_stack_entry_display(stk, FALSE);

	return obj_to_make_visible;
}

/*  Try to patch up the stack trace in the face of the way the epcf90
 *  compiler handles run time diagnostics.  These are handled by jumping
 *  to some epilogue code which calls f_rterr() to report the error.
 *  Because of the initial jump we would end up reporting the wrong
 *  line (usually the end statement) as the location of the error.
 *  epcf90 (when runtime checks are switched on) stores the current line
 *  number at -24(fp) on a SPARC (this comes from looking at object files,
 *  not from any documentation), so we look there for the line number.
 *
 *  BUG: what about other f90 compilers and other architectures?
 */
static void
check_for_f90_rterr(xp, stk)
target_t *xp;
Stack *stk;
{
	Stack *last;
	int lnum;
	lno_t *ln;
	taddr_t addr;

	last = NULL;
	
	for (; stk != NULL; stk = stk->stk_inner) {
		if (last != NULL && !last->stk_bad &&
		    last->stk_func->fu_language == LANG_F90 &&
		    strcmp(stk->stk_func->fu_name, "f_rterr") == 0)
			break;
		last = stk;
	}

	if (stk == NULL)
		return;

	/*  Are we at the last line of the function?
	 */
	for (ln = FU_LNOS(last->stk_func); ln != NULL; ln = ln->ln_next)
		if (ln->ln_num > last->stk_lnum)
			return;

	if (xp_read_data(xp, last->stk_fp - 0x18,
			 (char *)&lnum, sizeof(lnum)) != 0)
		return;
	
	addr = lnum_and_fil_to_addr(last->stk_func, last->stk_fil, lnum);

	if (addr != 0) {
		last->stk_lnum = lnum;
		last->stk_pc = addr;
	}
}

void
update_local_variable_values()
{
	Stack *stk;

	/*  Refresh the variable values for stack frames which were
	 *  there before.
	 */
	for (stk = Outer_stack; stk != NULL; stk = stk->stk_inner)
		update_vars_of((objid_t)stk, FALSE);
}

char *
badstack_format_obj(code)
objid_t code;
{
	return strf("%s", get_field_value(code, 0));
}

void
free_badfunc(obj)
objid_t obj;
{
	free((char *)get_field_value(obj, FN_BADFUNC_NAME));
}

void
delete_functions(par)
objid_t par;
{
	remove_object(par, OBJ_DESCENDENTS);
	
	Outer_stack = NULL;
	Inner_stack = NULL;
}

const char *
func_getobjname(obj)
objid_t obj;
{
	static char *last = NULL;
	Stack *stk;
	const char *filename, *funcname, *lnumstr;
	int level;

	filename = get_field_value(obj, FN_FUNC_FILE);
	funcname = get_field_value(obj, FN_FUNC_NAME);
	lnumstr = get_field_value(obj, FN_FUNC_LNUM);

	if (last != NULL)
		free(last);

	level = 0;
	for (stk = (Stack *)obj; stk != NULL; stk = stk->stk_outer)
		++level;

	last = strf("%d-%s:%s%s", level, filename, funcname, lnumstr);

	return last;
}

void
free_func(obj)
objid_t obj;
{
	Stack *stk;

	stk = (Stack *) obj;
	free((char *)get_field_value(obj, FN_FUNC_LNUM));
	destroy_stk(stk);
}

char *
func_format_obj(code)
objid_t code;
{
	const char *funcname, *filename, *lnumstr;
	
	funcname = get_field_value(code, FN_FUNC_NAME);
	filename = get_field_value(code, FN_FUNC_FILE);
	lnumstr = get_field_value(code, FN_FUNC_LNUM);

	return strf("%-20s%25s%-17s", funcname, filename, lnumstr);
}

int
pre_do_func(command, p_arg)
int command;
char **p_arg;
{
	return check_single_expr(command == MR_ADD_EXPRESSION);
}

void 
post_do_nolibfunc (command, arg)
int command;
char *arg;
{
  if (can_get_target_vars(get_current_target()))
    refresh_target_display(get_current_target(), SR_BPT, TRUE);
}

void
do_nolibfunc(par, command, arg)
objid_t par;
int command;
char *arg;
{
  char *str, *lib;
  
  switch(command) {
  case MR_LOAD_LIBRARY:
    str = badstack_format_obj(par);
    lib = strsave(str + 1);	/* remove '<' */
    lib[strlen(lib)-1] = 0;	/* remove '>' */
    add_to_load_path(lib, 0, 1); /* load requested */
#ifdef AO_TARGET
#if defined(AO_AOUT_SHLIBS)
    unload_shared_library_symtabs(get_current_target());
    load_shared_library_symtabs(get_current_target(), TRUE);
#endif /* AO_AOUT_SHLIBS */
    if (can_get_target_vars(get_current_target()))
      rescan_dynamic_solibs(get_current_target(), FALSE);
    /*    load_from_solibs(get_current_target(), lib);*/
#endif /* AO_TARGET */
    free(lib);
    break;
    
  default:
    panic("bad cmd in dbf");
  }
}

/*  Process the return from a function line menu.
 *  Add the variables to the display or conceal them.
 *  Tell the editor where we are.
 */
void
do_func(par, command, arg)
objid_t par;
int command;
char *arg;
{
	fil_t *fil;
	Stack *stk;

	stk = (Stack *)par;

	switch(command) {
	case MR_EXPAND_COMPLETELY:
	case MR_EXPAND_LIKE_BEFORE:
		if (command == MR_EXPAND_COMPLETELY ||
		    !restore_stack_entry_display(stk, TRUE)) {
			add_func_vars(par);
		}
		break;
		
	case MR_COLLAPSE:
		update_stack_entry(stk);
		remove_object(par, OBJ_DESCENDENTS);
		note_stack_entry_collapsed(stk);
		break;
		
	case MR_DISPLAY_SOURCE:
		fil = stk->stk_fil;

		if (fil == NULL) {
			errf("`%s' was not compiled with the -g flag",
			     stk->stk_func->fu_demangled_name);
			return;
		}

		if (set_and_display_stopline(fil, stk->stk_lnum))
			Current_stack = stk;
		
		break;

	case MR_ADD_EXPRESSION:
		fil = stk->stk_fil;

		if (fil == NULL) {
			errf("`%s' was not compiled with the -g flag",
			     stk->stk_func->fu_demangled_name);
			return;
		}

		st_get_fi_vars(stk->stk_fil);	/* ensure globals loaded */
		add_expr_object(par, FU_BLOCKS(stk->stk_func), "",
				stk->stk_func->fu_language, OBJ_LAST_CHILD);
		break;

	case MR_SHOW_PATH:
		fil = stk->stk_fil;
		if (!show_source_path_assumed(fil, FALSE))
		  errf("\b%s %s", "library", stk->stk_func->fu_symtab->st_path);
		break;
	case MR_SHOW_SOURCE_PATH:
		fil = stk->stk_fil;
		if (!show_source_path_used(fil, FALSE))
		  errf("\b%s %s", "library", stk->stk_func->fu_symtab->st_path);
		break;
	case MR_REMATCH_SOURCE_FILE:
		fil = stk->stk_fil;
		if (fil->fi_name)
		  open_source_file(fil, TRUE, FALSE);
		show_source(fil, 1);
		break;
	case MR_RELOAD_SOURCE_FILE:
		fil = stk->stk_fil;
		if (fil->fi_name)
		  open_source_file(fil, FALSE, TRUE);
		show_source(fil, 1);
		break;
	case MR_FILE_DATES:
		fil = stk->stk_fil;
		show_file_and_target_dates(fil, stk->stk_func->fu_symtab);
		break;
	default:
		panic("bad cmd in df");
	}
}

void
maybe_show_stack_entry_source(obj)
objid_t obj;
{
	Stack *stk;

	stk = (Stack *)obj;

	if (stk->stk_fil == NULL) {
		errf("\bNo source file known for function %s",
		     stk->stk_func->fu_demangled_name);
	}
	else {
		do_func(obj, MR_DISPLAY_SOURCE, (char *)NULL);
	}
}
		
void
change_displayed_stack_level(up)
bool up;
{
	Stack *stk;
	const char *withsource;

	stk = Current_stack;
	
	if (stk == NULL) {
		errf("No stack entry currently displayed");
		return;
	}

	withsource = "";
	
	for (;;) {
		stk = up ? stk->stk_outer : stk->stk_inner;

		if (stk == NULL) {
			errf("Already looking at %smost stack frame%s",
			     up ? "outer" : "inner", withsource);
			return;
		}

		if (stk->stk_func->fu_fil != NULL)
			break;

		withsource = " with source";
	}

	if (set_and_display_stopline(stk->stk_fil, stk->stk_lnum)) {
		Current_stack = stk;
		clear_selection();
		select_object((objid_t)stk, TRUE, OBJ_SELF);
		ensure_visible((objid_t)stk);
	}
}

#define FN_BLOCK_NAME 0
const char Block_format[] = "%*50l%15cs%*50l\n";

void
free_block(obj)
objid_t obj;
{
	free((char *)get_field_value(obj, FN_BLOCK_NAME));
	free((stackblock_t *)obj);
}


objid_t
find_block(par, block)
objid_t par;
block_t *block;
{
	objid_t obj;

	obj = get_code(par, OBJ_CHILD);
	for (; obj != NULL; obj = get_code(obj, OBJ_NEXT))
		if (((stackblock_t *)obj)->sb_block == block)
			return obj;
	return NULL;
}

/*  Get the stack from an OT_FUNC or OT_BLOCK object.
 */
static Stack *
obj_to_stack(obj)
objid_t obj;
{
	Stack *stk;

	switch(get_object_type(obj)) {
	case OT_FUNC:
		stk = (Stack *)obj;
		break;
	case OT_BLOCK:
		stk = ((stackblock_t *)obj)->sb_stk;
		break;
	default:
		panic("unknown obj type in ots");
		stk = NULL;	/* to satsfy gcc */
	}
	return stk;
}

block_t *
obj_to_block(obj)
objid_t obj;
{
	if (get_object_type(obj) == OT_BLOCK)
		return ((stackblock_t *)obj)->sb_block;

	panic("bad objtype in otb");
	return NULL;		/* to satisfy gcc */
}

objid_t
add_block_object(par, block)
objid_t par;
block_t *block;
{
	objid_t obj, prev;
	stackblock_t *sb;
	char buf[100];

	sb = (stackblock_t *)e_malloc(sizeof(stackblock_t));
	sb->sb_stk = obj_to_stack(par);
	sb->sb_block = block;

	/*  Add the block before any other blocks, but after any variables.
	 */
	prev = NULL;
	obj = get_code(par, OBJ_CHILD);
	for (; obj != NULL; obj = get_code(obj, OBJ_NEXT)) {
		if (get_object_type(obj) == OT_BLOCK &&
		    ((stackblock_t *)obj)->sb_block->bl_start_lnum >
								block->bl_start_lnum)
			break;
		prev = obj;
	}
	if (prev != NULL)
		new_object((objid_t)sb, OT_BLOCK, prev, OBJ_AFTER);
	else
		new_object((objid_t)sb, OT_BLOCK, par, OBJ_FIRST_CHILD);

	(void) sprintf(buf, " lines %d..%d ",
				block->bl_start_lnum, block->bl_end_lnum);
	set_field_value((objid_t)sb, FN_BLOCK_NAME, (fval_t)strsave(buf));

	return (objid_t)sb;
}

const char *
block_getobjname(obj)
objid_t obj;
{
	static char *last = NULL;
	stackblock_t *sb;
	int count, start_lnum, end_lnum;

	sb = (stackblock_t *)obj;
	start_lnum = sb->sb_block->bl_start_lnum;
	end_lnum = sb->sb_block->bl_end_lnum;

	count = 1;
	obj = get_code((objid_t)sb->sb_stk, OBJ_CHILD);
	for (; obj != NULL; obj = get_code(obj, OBJ_NEXT)) {
		stackblock_t *sb2;

		if (get_object_type(obj) != OT_BLOCK)
			continue;

		sb2 = (stackblock_t *)obj;
		if (sb2 == sb)
			break;
		
		if (sb2->sb_block->bl_start_lnum == start_lnum &&
		    sb2->sb_block->bl_end_lnum == end_lnum)
			++count;
	}

	if (last != NULL)
		free(last);

	if (count == 1)
		last = strf("%d..%d", start_lnum, end_lnum);
	else
		last = strf("%d-%d..%d", count, start_lnum, end_lnum);
	
	return last;
}

char *
block_format_obj(code)
objid_t code;
{
	const char *name;

	name = get_field_value(code, FN_BLOCK_NAME);

	if (*name == ' ')
		++name;

	return strf("%s", name);
}

int
pre_do_block(command, p_arg)
int command;
char **p_arg;
{
	return check_single_expr(command == MR_BLOCK_ADD_EXPRESSION);
}

/*  Process the return from a block header menu.
 */
void
do_block(par, command, arg)
objid_t par;
int command;
char *arg;
{
	fil_t *fil;
	stackblock_t *sb;

	sb = (stackblock_t *)par;
	switch(command) {
	case MR_BLOCK_EXPAND:
		expand_block(par, sb->sb_block);
		break;
	case MR_BLOCK_COLLAPSE:
		remove_object(par, OBJ_DESCENDENTS);
		break;
	case MR_BLOCK_DELETE:
		remove_object(par, OBJ_DESCENDENTS);
		remove_object(par, OBJ_SELF);
		break;
	case MR_BLOCK_DISPLAY_SOURCE:
		fil = sb->sb_stk->stk_fil;
		if (fil == NULL)
			errf("No source file known");
		else
			show_source(fil, sb->sb_block->bl_start_lnum);
		break;
	case MR_BLOCK_ADD_EXPRESSION:
		st_get_fi_vars(sb->sb_stk->stk_fil); /* get globals loaded */
		add_expr_object(par, sb->sb_block, "",
				sb->sb_stk->stk_func->fu_language,
				OBJ_LAST_CHILD);
		break;
	default:
		panic("bad cmd in db");
	}
}

static void
add_func_vars(par)
objid_t par;
{
	func_t *f;
	block_t *blocks;
	taddr_t junk;

	f = get_stack_func(par, &junk, &junk, &junk, &junk);
#ifdef ARCH_MIPS
	if (!(f->fu_language == LANG_C || f->fu_language == LANG_CC)) {
		errf("Sorry, can only show variables of C functions");
		return;
	}
#endif
	if (f->fu_flags & FU_NOSYM) {
		errf("Function %s has no variable information",
		     f->fu_demangled_name);
		return;
	}

	blocks = FU_BLOCKS(f);
	if (blocks == NULL ||
	    (blocks->bl_vars == NULL && blocks->bl_blocks == NULL)) {
		errf("Function %s has no parameters or local variables",
		     f->fu_demangled_name);
		return;
	}

	expand_block(par, blocks);
}

static bool
is_non_empty_block(par)
block_t *par;
{
	block_t *bl;

	if (par->bl_vars != NULL)
		return TRUE;

	for (bl = par->bl_blocks; bl != NULL; bl = bl->bl_next) {
		if (is_non_empty_block(bl))
			return TRUE;
	}

	return FALSE;
}

static void
expand_block(par, parblock)
objid_t par;
block_t *parblock;
{
	block_t *bl;
	var_t *v;

	for (bl = parblock->bl_blocks; bl != NULL; bl = bl->bl_next) {
		if (is_non_empty_block(bl)) {
			objid_t obj;
			
			if ((obj = find_block(par, bl)) == NULL)
				obj = add_block_object(par, bl);
			expand_block(obj, bl);
		}
	}

	for (v = parblock->bl_vars; v != NULL; v = v->va_next)
		if (find_var(par, v) == NULL)
			(void) add_var_object(par, v, OBJ_FIRST_CHILD, FALSE);
}

func_t *
get_stack_func(obj, p_fp, p_ap, p_sp, p_cfa)
objid_t obj;
taddr_t *p_fp, *p_ap, *p_sp, *p_cfa;
{
	Stack *stk;

	stk = obj_to_stack(obj);
	*p_fp = stk->stk_fp;
	*p_ap = stk->stk_ap;
	*p_sp = stk->stk_sp;
	*p_cfa = stk->stk_cfa;
	return stk->stk_func;
}

/*  Map a stackpos_t argument to a stack frame pointer.
 */
static Stack *
stackpos_to_stk(pos)
stackpos_t pos;
{
	struct selst *sel;
	Stack *stk;

	switch(pos) {
	case STK_OUTER:
		stk = Outer_stack;
		break;
	case STK_CURRENT:
		stk =  Current_stack;
		break;
	case STK_INNER:
		stk = Inner_stack;
		break;
	case STK_SELECTED:
		if ((sel = get_selection()) != NULL && sel->se_next == NULL &&
		    get_object_type(sel->se_code) == OT_FUNC)
			stk =  (Stack *)sel->se_code;
		else
			stk =  Current_stack;
		break;
	default:
		panic("bad stackpos in sts");
		stk = NULL;	/* to satisfy gcc */
	}
	return stk;
}

/*  Call func for each stack frame, starting from the frame specified by
 *  start, and ending after func has been called on the frame specified
 *  by lim.  See stackpos_to_stk() for the meanings of the pos arguments.
 *
 *  We always work outwards from the startpos to limpos.  Startpos must
 *  refer to a frame that is inside the frame referred to by endpos.
 *
 *  If (*func)() ever returns TRUE, we stop at that point and return TRUE.
 *  Otherwise we return FALSE.
 */
bool
iterate_over_stack_funcs(func, args, startpos, limpos)
bool (*func)PROTO((objid_t stack_obj, func_t *f, char *a_args));
char *args;
stackpos_t startpos, limpos;
{
	Stack *stk, *lim;
	bool found = FALSE;

	stk = stackpos_to_stk(startpos);
	lim = stackpos_to_stk(limpos);

	while (stk != NULL) {
	  	if (stop_pressed(0, 0))
		{
		  found = TRUE;
		  break;
		}
		if ((*func)((objid_t)stk, stk->stk_func, args))
		{
		  found = TRUE;
		  break;
		}
		if (stk == lim)
			break;
		stk = stk->stk_outer;
	}
	return found;
}

bool
back_iterate_over_stack_files(sw, name, startpos, limpos)
     void *sw;
     char *name;
     stackpos_t startpos, limpos;
{
  Stack *stk, *stk1, *stk_start, *lim;
  int skip;
  
  stk_start = stk = stackpos_to_stk(startpos);
  lim = stackpos_to_stk(limpos);
  
  while (stk != NULL)
  {
    if (stk->stk_func->fu_fil)
    {
      skip = 0;
      /* check if we have already done this file */
      for (stk1 = stk_start; stk1 != NULL;)
      {
	if (stk1 == stk)
	  break;
	if (stk->stk_func->fu_fil == stk1->stk_func->fu_fil)
	{
	  skip = 1;
	  break;	
	}
	stk1 = stk1->stk_inner;
     }
      if (!skip && show_global
	  (sw, stk->stk_func->fu_fil, (func_t*)NULL,
	   (common_block_t *)NULL,
	   name, TRUE, FALSE, FALSE, (objid_t *)NULL, TRUE, FALSE))
	return TRUE;
    }
    if (stk == lim)
      break;
    stk = stk->stk_inner;
  }
  return FALSE;
}

/*  Find the address at which the register variable has been saved.
 *  par is the parent object of the variable (which must be a function
 *  or block).
 */
taddr_t
get_reg_addr(xp, par, reg, size)
target_t *xp;
objid_t par;
int reg;
size_t size;
{
	taddr_t addr;
	Stack *stk;

	stk = obj_to_stack(par);

	switch (reg) {
	case UPSREG_PC:
		addr = (taddr_t)&stk->stk_pc;
		break;
	case UPSREG_AP:
		addr = (taddr_t)&stk->stk_ap;
		break;
	case UPSREG_FP:
		addr = (taddr_t)&stk->stk_fp;
		break;
	case UPSREG_SP:
		addr = (taddr_t)&stk->stk_sp;
		break;
	default:
		if ((addr = xp_get_reg_addr(xp, stk, reg)) == 0)
			addr = regno_to_addr(reg);
		else
			addr = adjust_saved_reg_addr(xp, addr, size);
		break;
	}

	return addr;
}
int match_name_end_in_func(name, obj_exists, obj_var)
char *name;
int *obj_exists;
int **obj_var;
{
  objid_t par, var_obj;
  block_t *block;
  var_t *v;
  int found = 0, len, str_match;
  func_t *f;
  block_t *blocks;
  taddr_t junk;

  *obj_exists = 0;
  *obj_var = 0;
  if (!can_get_target_vars(get_current_target()))
    return found;

  par = (objid_t) Current_stack;
  if (!par)
    return found;
  f = get_stack_func(par, &junk, &junk, &junk, &junk);
  
#ifdef ARCH_MIPS
  if (!(f->fu_language == LANG_C || f->fu_language == LANG_CC)) {
    return found;
  }
#endif
  if (f->fu_flags & FU_NOSYM) {
    return found;
  }
  
  blocks = FU_BLOCKS(f);
  if (blocks == NULL ||
      (blocks->bl_vars == NULL && blocks->bl_blocks == NULL)) {
    return found;
  }
  
  for (block = blocks->bl_blocks; block != NULL; block = block->bl_next) {
    if (block->bl_vars != NULL || block->bl_blocks != NULL) {
      if (find_block(par, block) == NULL)
	add_block_object(par, block);
    }
  }
  len = strlen(name);  
  for (v = blocks->bl_vars; v != NULL; v = v->va_next)
  {
    if (demangling_enabled(0, 0))
      str_match = !strcmp(v->va_name, name);     
    else
      str_match = !!strstr(v->va_name, name);
    if (str_match)
    {
      var_obj = find_var(par, v);
      if (var_obj)
	*obj_exists = 1;
      else
	var_obj = add_var_object(par, v, OBJ_FIRST_CHILD, FALSE);
      *obj_var = (int *)var_obj;
      clear_selection();
      select_object(var_obj, TRUE, OBJ_SELF);
      ensure_visible(var_obj);
      found = 1;
      break;
    }
   }
  return found;
}

int
display_var_in_mangled_form(buf, this_obj_exists, this_obj_var)
     const char *buf;
     int *this_obj_exists;
     int **this_obj_var;
{
  char **names, **nptr;
  int found = 0, found2 = 0, oldstate, obj_exists, *obj_var, *obj_var1;
  
  names = ssplit(buf, ".-> \t");
  oldstate = td_set_obj_updating(OBJ_UPDATING_OFF);
  
  /* look for pre mangled name base: "something___<*nptr>" */
  /* then look for list of post mangled fields */
  /* "...-><*nptr>___something-><*(nptr++)>___something" etc */
  
  if (match_name_end_in_func
      ("this", this_obj_exists, this_obj_var))
  {
    for (nptr = names; *nptr != NULL; ++nptr)
      found2 = match_name_start_in_obj
	(*nptr, 0, &obj_exists, &obj_var1);
    if (found && !found2)
    {				/* deselect "this" */
      deselect(code_to_obj(*this_obj_var));
    }
  }

  /* RGA - keep looking in case same name is not used in class */
  found = match_name_end_in_func(*names, &obj_exists, &obj_var);
  if (found)
    for (nptr = names, ++nptr; *nptr != NULL; ++nptr)
      found = match_name_start_in_obj
	(*nptr, 0, &obj_exists, &obj_var);

  td_set_obj_updating(oldstate);
  free((char *)names);
  return (found || found2);
}

/*  Return the width in pixels of an uloaded lib object.
 *
 *  A pointer to this function is passed to the obj library.
 */
/* ARGSUSED */

void
nolibfunc_getsize(obj, unused_par, sz)
objid_t obj, unused_par;
struct szst *sz;
{
	char *str;
	int width = 100;

	str = badstack_format_obj(obj);
	if (str)
	  width = wn_strwidth(str, (font_t *)NULL);
	sz->sz_depth = wn_get_sysfont()->ft_height;
	sz->sz_width = width + wn_get_sysfont()->ft_width;
	if (str)
	  free(str);
}
