/* va.h - public header file for the va_* package */

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


/* @(#)va.h	1.12 04 Jun 1995 (UKC) */

#ifdef SYMTAB_H_INCLUDED
const char *type_to_decl PROTO((const char *name, type_t *type, class_t class,
				language_t language, bool want_typedefs));
#endif

#ifdef OBJTYPES_H_INCLUDED
const char *var_getobjname PROTO((objid_t obj));
char *var_format_obj PROTO((objid_t code));
int check_single_expr PROTO((bool is_add_expr_cmd));
void do_var_or_expr PROTO((objid_t obj, int command, char *arg));
const char *expr_getobjname PROTO((objid_t obj));
char *expr_format_obj PROTO((objid_t code));
void free_displayed_var PROTO((objid_t obj));
void free_displayed_expr PROTO((objid_t obj));
void var_getcolor PROTO((objid_t obj, int wn, long *p_fg, long *p_bg));
void expr_getcolor PROTO((objid_t obj, int wn, long *p_fg, long *p_bg));
void update_vars_of PROTO((objid_t par, int change_caused_by_edit));

objid_t add_blocks_as_needed PROTO((objid_t par, block_t *rootbl, block_t *bl));

#ifdef SYMTAB_H_INCLUDED

objid_t find_var PROTO((objid_t par, var_t *v));
objid_t add_var_object PROTO((objid_t par, var_t *v, int poscode,
			      bool restoring));

objid_t add_expr_object PROTO((objid_t par, block_t *bl, const char *text,
			       language_t language, int poscode));

int get_member_of_aggr PROTO((objid_t par, const char *name,
			      var_t **p_v, taddr_t *p_addr, type_t **btype));

void update_var_state PROTO((symtab_t *oldst, symtab_t *newst));
#endif

#ifdef TARGET_H_INCLUDED
void update_stack_entry PROTO((Stack *stk));
bool restore_stack_entry_display PROTO((Stack *stk, bool force));
void note_stack_entry_collapsed PROTO((Stack *stk));
#endif

void update_file_entry PROTO((objid_t obj));
bool restore_file_display PROTO((objid_t obj, bool force));
bool restore_files PROTO((objid_t obj));
void reset_file_restore_states PROTO((void));
void note_file_collapsed PROTO((objid_t obj, bool dummy));

extern char Var_format[], Expr_format[];
extern fdef_t Var_fdefs[], Expr_fdefs[];
extern fnamemap_t Var_fnamemap[], Expr_fnamemap[];
#endif

#ifdef EDIT_H_INCLUDED
extern Edit_history* value_history;
extern Edit_history* subscript_history;
extern Edit_history* expr_history;

/*  This is used only by tdr.c.
 */
void adjust_index PROTO((Edit_display *display, bool up));
#endif

void restore_file_displays PROTO((void));

void save_var_display_state PROTO((bool just_started, bool set_been_displayed));

#ifdef EOF
int write_var_state_to_file PROTO((const char *path, FILE *fp));

int handle_file_command PROTO((const char *cmd, char **args, int nargs,
			       bool from_statefile, FILE *fp, int *p_lnum));

int handle_function_command PROTO((const char *cmd, char **args, int nargs,
				   bool from_statefile, FILE *fp, int *p_lnum));

int handle_format_command PROTO((const char *cmd, char **args, int nargs,
				 bool is_hint));
#endif

void free_vslist PROTO((void));

void free_vflist PROTO((void));

void free_ftlist PROTO((void));

void remove_all_vars PROTO((void));
