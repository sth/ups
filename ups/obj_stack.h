/* obj_stack.h - header file for stack.c */

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


/* @(#)obj_stack.h	1.12 20 Jun 1995 (UKC) */

/*  Type of the pos arguments to interate_over_stack_funcs().
 *
 *  If there is exactly one stack frame selected, STK_SELECTED refers to it.
 *  Otherwise it means the same as STK_CURRENT.
 */
typedef enum stackposen {
	STK_INNER,	/* innermost frame */
	STK_OUTER,	/* outermost frame */
	STK_CURRENT,	/* `current' frame (for which source is displayed) */
	STK_SELECTED	/* see above */
} stackpos_t;

taddr_t get_outer_fp PROTO((void));

#ifdef OBJ_H_INCLUDED
const char *block_getobjname PROTO((objid_t obj));

#ifdef TARGET_H_INCLUDED
taddr_t get_reg_addr PROTO((target_t *xp, objid_t par, int reg, size_t size));
objid_t add_functions PROTO((target_t *xp, objid_t par));
#endif

void maybe_show_stack_entry_source PROTO((objid_t obj));
void update_local_variable_values PROTO((void));
void delete_functions PROTO((objid_t par));

void change_displayed_stack_level PROTO((bool up));

const char *func_getobjname PROTO((objid_t code));
char *func_format_obj PROTO((objid_t code));
int pre_do_func PROTO((int command, char **p_arg));
void do_func PROTO((objid_t par, int command, char *arg));
void do_nolibfunc PROTO((objid_t par, int command, char *arg));
void post_do_nolibfunc PROTO((int command, char *arg));
char *block_format_obj PROTO((objid_t code));
int pre_do_block PROTO((int command, char **p_arg));
void do_block PROTO((objid_t obj, int command, char *arg));
char *badstack_format_obj PROTO((objid_t code));
char *fsig_format_obj PROTO((objid_t code));
void free_func PROTO((objid_t obj));
void free_badfunc PROTO((objid_t obj));
void free_fsig PROTO((objid_t obj));
void free_block PROTO((objid_t obj));

#ifdef SYMTAB_H_INCLUDED
block_t *obj_to_block PROTO((objid_t obj));
func_t *get_stack_func PROTO((objid_t obj, taddr_t *p_fp, taddr_t *p_ap));
objid_t find_block PROTO((objid_t par, block_t *block));
objid_t add_block_object PROTO((objid_t par, block_t *block));
bool iterate_over_stack_funcs PROTO((
			    bool (*func)(objid_t stack_obj, func_t *f, char *args),
			    char *args, stackpos_t startpos, stackpos_t endpos));
bool back_iterate_over_stack_files PROTO((int sw_id, char *name,
					  stackpos_t startpos, stackpos_t limpos));
#endif /* SYMTAB_H_INCLUDED */
int match_name_start_in_obj PROTO((char *name, objid_t given_obj, 
				   int *obj_exists, int **obj_var));
void delete_variable PROTO((objid_t obj));
#endif /* OBJ_H_INCLUDED */

#ifdef SYMTAB_H_INCLUDED
void get_current_func PROTO((func_t **p_func, taddr_t *p_fp,
					taddr_t *p_pc, taddr_t *p_adjusted_pc));
void get_current_func_2 PROTO((func_t **p_func, taddr_t *p_fp,
			       taddr_t *p_pc, fil_t **p_fil, int *p_lnum));
#endif

char *get_current_stack_data PROTO((void));

const char *stack_addr_label PROTO((taddr_t addr, bool *p_is_labelled));
int match_name_end_in_func PROTO((char *name, int *obj_exists, int **obj_var));
int display_var_in_mangled_form PROTO((const char *buf, int *this_obj_exists,
				   int **this_obj_var));

extern const char Func_format[];
extern const char Fsig_format[];
extern const char Badfunc_format[];
extern const char Nolibfunc_format[];
extern const char Block_format[];

#ifdef OBJTYPES_H_INCLUDED
extern fdef_t Fsig_fdefs[];
extern fdef_t Func_fdefs[];
#endif
