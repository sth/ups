/* exec.h - header file for exec.c */

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


/* @(#)exec.h	1.13 24 May 1995 (UKC) */

int attach_to_process PROTO((const char *name, int pid));
void do_menu_target_command PROTO((int rv));

/*  Address of builtin ups functions.  Can be any addresses that can't be
 *  mistaken for target function address.
 */
#define STOP_ADDR 		(-1)
#define PRINTF_ADDR		(-2)
#define SET_EXPR_RESULT_ADDR	(-3)

extern const char Stop_funcname[];
extern const char Set_expr_result_funcname[];

#ifdef SYMTAB_H_INCLUDED
#ifdef CI_H_INCLUDED

/*  Return values for special functions.
 */
#define STOP	CI_ER_USER1

ci_exec_result_t call_target_function PROTO((machine_t *ma, taddr_t addr,
				taddr_t *args, int nargs, taddr_t *p_res,
				typecode_t restype));
#endif
void exec_to_lnum PROTO((func_t *f, fil_t *fil, int lnum));
void jump_to_lnum PROTO((func_t *f, fil_t *fil, int lnum));

#endif

#ifdef TARGET_H_INCLUDED
int get_restart_sig PROTO((target_t *xp));
void refresh_target_display PROTO((target_t *xp, stopres_t stopres,
				   bool just_started));
void check_for_dynamic_libs PROTO((target_t *xp, stopres_t *p_stopres));
void check_for_dynamic_libs_sstep PROTO((target_t *xp, stopres_t *p_stopres));
#endif
int check_target_name PROTO((const char *textpath, int *p_index, int *p_fd,
			     long *p_mtime, bool check_textfile));
void set_quit_on_exit PROTO((bool quit_on_exit));
