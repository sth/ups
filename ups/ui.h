/* ui.h - declarations of public user interface routines */

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


/* @(#)ui.h	1.25 24 May 1995 (UKC) */

#ifdef REG_H_INCLUDED
Region *divide_window_into_regions PROTO((int wn, bool want_outwin));
Region *divide_window_into_regions1 PROTO((int wn, bool want_outwin));
Region *divide_window_into_regions2 PROTO((int wn, bool want_outwin));
void menu_input PROTO((Region *region, event_t *ev));
int want_split_output PROTO((int* p_screen));
int get_split_out_win PROTO((void));
#endif

#ifdef EDIT_H_INCLUDED
extern Edit_history* typing_line_history;
extern Edit_history* prompt_history;
Edit_display *make_text_display PROTO((int wn, int fg, int bg,
				       const char *text));

char *get_buffer_contents PROTO((Edit_buffer *buffer));

bool search_buffer PROTO((Edit_buffer *buffer, const char *pattern,
			  size_t start_point, bool forwards,
			  size_t *p_start, size_t *p_lim, bool file));
#endif

void display_message PROTO((const char *mesg));
void redraw_message PROTO((int));

char *get_typing_line_string PROTO((void));

#ifdef TARGET_H_INCLUDED
void update_target_menu_state PROTO((tstate_t tstate, int attached));
#endif

typedef void (*abort_func_t)PROTO((void));
abort_func_t set_user_abort_func PROTO((abort_func_t func));
bool user_wants_stop PROTO((bool peek_at_event));

#ifdef SYMTAB_H_INCLUDED
bool show_source PROTO((fil_t *fil, int lnum));
#ifdef SRCWIN_H_INCLUDED
void show_func_source PROTO((Srcwin *sw, func_t *f));
#endif
bool set_and_display_stopline PROTO((fil_t *fil, int lnum));
fil_t *get_displayed_fil PROTO((void));
language_t get_mainfunc_lang PROTO((void));
language_t get_user_lang PROTO((void));
#endif

#ifdef OBJ_H_INCLUDED
void dynamic_menu_func PROTO((char *data, int md, int cmd));
int ups_get_object_type PROTO((objid_t obj));
void add_to_new_selection PROTO((objid_t obj));
void ensure_visible PROTO((objid_t obj));
bool change_field PROTO((objid_t obj, int fnum, const char *new));
void n_draw PROTO((struct drawst *dets));
bool can_select PROTO((objid_t obj));
void gen_select PROTO((int wn, objid_t obj, int x, int y,
					int width, int height, int flags));
#endif /* OBJ_H_INCLUDED */

int prompt_for_choice PROTO((const char *what, const char *prompt,
			     const char *default_choice, const char *options,
			     int *p_choice));
int prompt_for_string PROTO((const char *what, const char *prompt,
			     const char *default_string, char **p_string));
int prompt_for_output_file PROTO((const char *prompt,
				  char **p_path, const char **p_mode));

#ifdef CI_H_INCLUDED
typedef struct {
	parse_id_t cr_parse_id;
	machine_t *cr_machine;
	bool cr_code_has_func_calls;
} compile_res_t;

compile_res_t *compile_code PROTO((const char **lines, int nlines,
				   block_t *block, char *grp_arg,
				   lexinfo_t *error_lx,
				   const char *firstline, const char *lastline,
				   const char *repl_text, const char *repl_val));

machine_t *recompile_code PROTO((parse_id_t parse_id, char *grp_arg));
void free_parse_id_and_machine PROTO((parse_id_t parse_id, machine_t *ma));
#endif

/*  The following stuff would be in ui_priv.h were it not for the fact
 *  that tdr.c needs access to it.
 */
void source_window_menu_func PROTO((char *data, int md, int rv));
void target_menu_func PROTO((char *data, int md, int cmd));
void permanent_menu_func PROTO((char *data, int md, int cmd));
void do_debug_command PROTO((const char *s));
void set_bphead_menu_state PROTO((bool enable));
void set_dynamic_bphead_menu_state PROTO((int md, bool enable));
void set_wphead_menu_state PROTO((bool enable));
void set_dynamic_wphead_menu_state PROTO((int md, bool enable));
void init_menu_toggle_items PROTO((int md));
#ifdef SRCWIN_H_INCLUDED
void do_output_window_command PROTO((Outwin *ow, int cmd));
Outwin *get_or_create_outwin_unmapped PROTO((void));
Outwin *get_or_create_outwin PROTO((void));
#endif

/*  State for set_dynamic_menu_updating_state().
 */
typedef enum { DMU_OFF, DMU_ON } dmu_state_t;

void set_dynamic_menu_updating_state PROTO((dmu_state_t new_state));
int list_matches_to_outwin PROTO((int set, int reset));
void handle_display_area_char PROTO((int key_char, bool meta));
void handle_typing_line_char PROTO((int key_char, bool meta));
bool stop_pressed PROTO((int set, int reset));
void redo_displayed_source_files PROTO((void));
bool target_menu_search_disabled PROTO((int set, int reset));
void indicate_target_menu_search PROTO((int set));
void dump_memory PROTO((const char* text));
int rational_products_running PROTO((void));

int  get_run_alarm_time PROTO((void));
void run_alarm_time_expired PROTO((void));
char *get_target_name PROTO((void));

void run_done PROTO((int suggest_raise));
void reset_pathlist PROTO((char *textpath));
void update_icon_name PROTO((char *textname));
const char *hold_default_source_path PROTO((const char *path, bool set));
void load_library_input PROTO((void *handle, char *arg));
