/* edit.h - most of the public interface to the libedit library */

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


/* @(#)edit.h	1.11 16 Sep 1994 (UKC) */

#define EDIT_H_INCLUDED

#ifndef TEXT_BUFFER_DEFINED
typedef struct Text_buffer Text_buffer;
#define TEXT_BUFFER_DEFINED
#endif /* !TEXT_BUFFER_DEFINED */

typedef unsigned Edit_flags;
/*  RCB: Define what the flags mean */
#define FLAG_HIGHLIGHT1  0x0001
#define FLAG_HIGHLIGHT2  0x0002
#define FLAG_HIGHLIGHTS  (FLAG_HIGHLIGHT1|FLAG_HIGHLIGHT2)
#define FLAG_EDITLINE    0x0004
#define FLAG_VARIABLE    0x0008
#define FLAG_VALUE       0x0010
#define FLAG_DISABLED    0x0020
#define FLAG_BPT         0x0040
#define FLAG_ENV         0x0080
#define FLAG_SRC         0x0100
#define FLAG_FUNC        0x0200
#define FLAG_SIG         0x0400
#define FLAG_WPT         0x0800


typedef struct Edit_display Edit_display;
typedef struct Edit_fontinfo Edit_fontinfo;
typedef struct Edit_buffer Edit_buffer;
typedef struct Edit_propchange Edit_propchange;
typedef struct Edit_keymap Edit_keymap;
typedef struct Edit_lineinfo Edit_lineinfo;
typedef struct Edit_history Edit_history;

#define EDIT_MAX_POINT	((size_t)(((unsigned long)-1) >> 1))

typedef struct {
	void (*set_updating)PROTO((char *data, bool on));
	bool (*get_selection)PROTO((char *data,
				    const char **p_bytes, size_t *p_nbytes));
	void (*render_text)PROTO((char *data, char *font_handle,
				  Edit_flags flags,
				  int x, int y, int baseline,
				  int width, int height,
				  const char *text, int len));
	void (*set_area)PROTO((char *data, int x, int y,
			       int width, int height, bool on));
	void (*show_cursor)PROTO((char *data, char *font_handle,
				  Edit_flags flags,
				  int x, int y, int baseline,
				  int width, int height,
				  int ch, bool on));
	bool (*copy_area)PROTO((char *data,
				int old_x, int old_y, int new_x, int new_y,
				int width, int height));
} Edit_render_ops;

typedef struct Edit_history_item
{
    const char* name;
    struct Edit_history** history_addr;
    int    default_size;
} Edit_history_item;


#define EDIT_SHIFT	((unsigned)1<<0)
#define EDIT_CONTROL	((unsigned)1<<1)
#define EDIT_ALT	((unsigned)1<<2)

/*  Special key values
 */
#define EDIT_SK(n)		(0x1000 + (n))
#define EDIT_LEFT_ARROW		EDIT_SK(0)
#define EDIT_RIGHT_ARROW	EDIT_SK(1)
#define EDIT_UP_ARROW		EDIT_SK(2)
#define EDIT_DOWN_ARROW		EDIT_SK(3)
#define EDIT_FUNC_KEY(n)	EDIT_SK(n + 4)

typedef void (*Edit_function)PROTO((Edit_display *d));

/*  Builtin editing functions.
 */
void edit_f_beginning_of_buffer PROTO((Edit_display *d));
void edit_f_end_of_buffer PROTO((Edit_display *d));
void edit_f_beginning_of_line PROTO((Edit_display *d));
void edit_f_end_of_line PROTO((Edit_display *d));
void edit_f_forward_char PROTO((Edit_display *d));
void edit_f_backward_char PROTO((Edit_display *d));
void edit_f_delete_char PROTO((Edit_display *d));
void edit_f_kill_line PROTO((Edit_display *d));
void edit_f_backward_kill_line PROTO((Edit_display *d));
void edit_f_kill_word PROTO((Edit_display *d));
void edit_f_backward_word PROTO((Edit_display *d));
void edit_f_forward_word PROTO((Edit_display *d));
void edit_f_backward_kill_word PROTO((Edit_display *d));
void edit_f_delete_backward_char PROTO((Edit_display *d));
void edit_f_previous_line PROTO((Edit_display *d));
void edit_f_next_line PROTO((Edit_display *d));
void edit_f_previous_line_or_history_item PROTO((Edit_display *d));
void edit_f_next_line_or_history_item PROTO((Edit_display *d));
void edit_f_newline PROTO((Edit_display *d));
void edit_f_indent_for_tab_command PROTO((Edit_display *d));
void edit_f_scroll_up PROTO((Edit_display *d));
void edit_f_scroll_down PROTO((Edit_display *d));
void edit_f_set_mark_command PROTO((Edit_display *d));
void edit_f_kill_region PROTO((Edit_display *d));
void edit_f_request_quit PROTO((Edit_display *d));
void edit_f_self_insert PROTO((Edit_display *d));
void edit_f_back_to_indentation PROTO((Edit_display *d));
void edit_f_delete_horizontal_space PROTO((Edit_display *d));
void edit_f_yank PROTO((Edit_display *d));
void edit_f_undef PROTO((Edit_display *d));

Edit_keymap *edit_create_keymap PROTO((Edit_keymap *next));
void edit_destroy_keymap PROTO((Edit_keymap *keymap));
Edit_keymap *edit_get_next_keymap PROTO((Edit_keymap *keymap));
void edit_add_keymap_entry PROTO((Edit_keymap *km, unsigned modifiers, int key,
                                  Edit_function function));
Edit_keymap *edit_get_default_keymap PROTO((void));
Edit_function edit_lookup_key PROTO((Edit_keymap *km,
				     unsigned modifiers, int key));


Edit_fontinfo *edit_make_fontinfo PROTO((char *font_handle,
					 short *char_width_tab,
					 int line_height, int baseline));

Edit_buffer *edit_create_buffer PROTO((Text_buffer *tb,
				       Edit_fontinfo *fi, Edit_flags flags));

void edit_destroy_buffer PROTO((Edit_buffer *buffer));

Edit_display *edit_create_display PROTO((bool want_cursor,
					 bool keep_cursor_visible,
					 char *render_data,
					 Edit_render_ops *ops));

void edit_close_display PROTO((Edit_display *display));

void edit_use_folded_lines PROTO((Edit_display *d, int tabspaces,
				  bool want_checksrcs, bool fold_lines));
void edit_use_single_line PROTO((Edit_display *d));

bool edit_set_want_cursor PROTO((Edit_display *d, bool want_cursor));

bool edit_set_propchange_updates PROTO((Edit_buffer *buffer, bool on));

Edit_propchange *edit_add_propchange PROTO((Edit_buffer *buffer,
					    size_t point, char *user_data,
					    Edit_fontinfo *fi,
					    Edit_flags flagmask,
					    Edit_flags flags,
					    bool backsliding));

Edit_buffer *edit_get_buffer PROTO((Edit_display *d));
void edit_set_buffer PROTO((Edit_display *d, Edit_buffer *buffer));

Edit_keymap *edit_get_keymap PROTO((Edit_display *d));
void edit_set_keymap PROTO((Edit_display *d, Edit_keymap *keymap));

Edit_propchange *edit_get_propchange_at_point PROTO((Edit_buffer *buffer,
						     size_t point));

size_t edit_get_propchange_point PROTO((Edit_propchange *pc));
void edit_move_propchange_pointt PROTO((Edit_propchange *pc, int delta));

char *edit_get_propchange_data PROTO((Edit_propchange *pc));
void edit_set_propchange_data PROTO((Edit_propchange *pc, char *data));

void edit_get_propchange_info PROTO((Edit_propchange *pc,
				     size_t *p_point, char **p_user_data,
				     Edit_fontinfo **p_fontinfo,
				     Edit_flags *p_flags));

void edit_change_propchange_info PROTO((Edit_propchange *pc,
				     size_t *p_point, char **p_user_data,
				     Edit_fontinfo **p_fontinfo,
				     Edit_flags *p_flags));

Edit_propchange *edit_get_next_propchange PROTO((Edit_buffer *buffer,
						 Edit_propchange *pc,
						 size_t point,
						 size_t *p_next_pc_point));

void edit_delete_propchange PROTO((Edit_buffer *buffer,
				   Edit_propchange *pc));

void edit_move_propchange PROTO((Edit_buffer *buffer, Edit_propchange *pc,
				 size_t newpoint));

char* edit_set_global_selection PROTO((Edit_display*,
				       Edit_propchange* startpc,
				       Edit_propchange* endpc,
				       long timestamp,
				       bool want_text));
void edit_unselect_global_selection PROTO((Edit_display*));

bool edit_set_display_size PROTO((Edit_display *d, int width, int height));

void edit_get_display_size PROTO((Edit_display *d,
				  int *p_width, int *p_height));

bool edit_pixel_to_point PROTO((Edit_display *d, int x, int y, size_t *point));

bool edit_point_to_pixel PROTO((Edit_display *d, size_t point,
				int *p_x, int *p_y,
				int *p_width, int *p_height, int *p_baseline));

void edit_get_render_data_and_ops PROTO((Edit_display *d,
					 char **p_render_data,
					 Edit_render_ops **p_ops));

void edit_move_point PROTO((Edit_display *d, size_t pos));

size_t edit_set_point PROTO((Edit_display *d, size_t pos));
size_t edit_get_point PROTO((Edit_display *d));

void edit_set_point_limits PROTO((Edit_display *d, size_t start, size_t lim));
void edit_get_point_limits PROTO((Edit_display *d,
				  size_t *p_start, size_t *p_lim));

void edit_set_mark PROTO((Edit_display *d, size_t mark));
size_t edit_get_mark PROTO((Edit_display *d));


bool edit_ensure_visible PROTO((Edit_display *d, size_t pos));

bool edit_insert PROTO((Edit_buffer *b, size_t pos,
			const char *text, size_t len, bool update_proplist));
bool edit_delete PROTO((Edit_buffer *b, size_t pos, size_t len));

void edit_handle_key_event PROTO((Edit_display *d, int key,
				  unsigned modifiers));

int edit_scroll_display PROTO((Edit_display *d, int npixels));

int edit_scroll_history PROTO((Edit_display *d, int direction));

void edit_get_display_info PROTO((Edit_display *d, int *p_pixel_offset,
				  size_t *p_start_point,
				  size_t *p_lim_point));
void edit_display_from PROTO((Edit_display *d, int pixel_offset, size_t point));
void edit_redraw_display PROTO((Edit_display *d));

bool edit_find_char_forwards PROTO((Edit_buffer *buffer,
				    size_t start, size_t lim,
				    const char *pat, size_t *p_pos));
bool edit_find_char_backwards PROTO((Edit_buffer *buffer,
				     size_t start, size_t lim,
				     const char *pat, size_t *p_pos));

size_t edit_get_buffer_length PROTO((Edit_buffer *buffer));

bool edit_get_bytes PROTO((Edit_buffer *buffer, size_t pos,
			   const char **p_line, const char **p_lim));
bool edit_get_bytes_before PROTO((Edit_buffer *buffer, size_t pos,
				  const char **p_line, const char **p_lim));

void edit_get_buffer_modtime PROTO((Edit_buffer *buffer, long *mtime));
 

const char *edit_version PROTO((void));

void edit_set_user_data PROTO((Edit_display *d, char *user_data));
char *edit_get_user_data PROTO((Edit_display *d));

bool edit_quit_requested PROTO((Edit_display *d));

size_t edit_restrict_point PROTO((Edit_display *d, size_t point));

void edit_set_goal_column PROTO((Edit_display *d, size_t point));

/*  Utility routines.
 */
void edit_update_propchange_pair_pos PROTO((Edit_buffer *buffer, size_t point, 
					    Edit_propchange *pc1, 
					    Edit_propchange *pc2,
					    Edit_propchange **p_movepc));

char *edit_copy_bytes PROTO((Edit_buffer *buffer, size_t start, size_t lim,
			     char *obuf));

void edit_add_keymap_control_entry PROTO((Edit_keymap *keymap, int key,
					  Edit_function function));

Edit_lineinfo *edit_make_lineinfo PROTO((Edit_buffer *buffer, int endc));
void edit_set_lineinfo_pos PROTO((Edit_lineinfo *li, size_t pos));
size_t edit_get_lineinfo_pos PROTO((Edit_lineinfo *li));
bool edit_nextline PROTO((Edit_lineinfo *li, const char **p_line,
			  size_t *p_len, bool *p_have_eol));
bool edit_prevline PROTO((Edit_lineinfo *li, const char **p_line,
			  size_t *p_len, bool *p_have_eol));
void edit_free_lineinfo PROTO((Edit_lineinfo *li));
char *edit_get_writable_copy PROTO((Edit_lineinfo *li, const char *line,
				    size_t len));

void  edit_set_history_pointer PROTO((Edit_display *d, int offset));
void  edit_destroy_history PROTO((Edit_history* h));
void  edit_set_history PROTO((Edit_display *d, Edit_history* h));
void  edit_save_in_history PROTO((Edit_display *d));
void  edit_append_history PROTO((Edit_history* h, char* text));
void  edit_initialize_history PROTO((const char* state_dir, const char* state_file, const char* home_file,Edit_history_item*));
void  edit_get_recent_history PROTO((Edit_display *d, int* count, const char** history));
void  edit_terminate_history PROTO((void));

