/* keymap.c - maintain key->action maps */

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


/* @(#)keymap.c	1.2 09 Apr 1995 (UKC) */
char edit_keymap_c_rcsid[] = "$Id$";

#include <stdlib.h>

#include <local/ukcprog.h>

#include "edit.h"

typedef struct Keymap_list Keymap_list;

typedef struct {
	unsigned modifiers;
	int key;
	Edit_function function;
} Keymap_entry;

struct Keymap_list {
	Keymap_entry entry;
	Keymap_list *next;
};

struct Edit_keymap {
	Keymap_list *klist;
	Keymap_entry *ktab;
	size_t ktab_size;
	Edit_function default_function;
	Edit_keymap *next;
};

/*  Short names to make the table more readable.
 */
#define CTRL	EDIT_CONTROL
#define ALT	EDIT_ALT
#define SHIFT	EDIT_SHIFT

#define CTRL_ENTRY(key, func) \
	{ CTRL,		key,	func,	}, \
	{ CTRL|SHIFT,	key,	func,	}

static Keymap_entry Default_keymap_tab[] = {
	CTRL_ENTRY('a',		edit_f_beginning_of_line),
        CTRL_ENTRY('e', 	edit_f_end_of_line),
	{ ALT, 'm',		edit_f_back_to_indentation },

	{ CTRL, '@',		edit_f_set_mark_command },
	{ CTRL, ' ',		edit_f_set_mark_command },
	CTRL_ENTRY('w',		edit_f_kill_region),
	
	CTRL_ENTRY('p', 	edit_f_previous_line_or_history_item),
	{ 0, EDIT_UP_ARROW, 	edit_f_previous_line_or_history_item },
	CTRL_ENTRY('n', 	edit_f_next_line_or_history_item),
	{ 0, EDIT_DOWN_ARROW, 	edit_f_next_line_or_history_item },
	
	CTRL_ENTRY('f', 	edit_f_forward_char),
	{ 0, EDIT_RIGHT_ARROW, 	edit_f_forward_char },
	{ ALT, 'f', 		edit_f_forward_word },
	{ ALT, EDIT_RIGHT_ARROW, edit_f_forward_word },
	
	CTRL_ENTRY('b', 	edit_f_backward_char),
	{ 0, EDIT_LEFT_ARROW, 	edit_f_backward_char },
	{ ALT, 'b', 		edit_f_backward_word },
	{ ALT, EDIT_LEFT_ARROW, edit_f_backward_word },
	
	CTRL_ENTRY('j', 	edit_f_newline),
	CTRL_ENTRY('m',		edit_f_newline),
	
	CTRL_ENTRY('i',		edit_f_indent_for_tab_command),
	
	CTRL_ENTRY('v', 	edit_f_scroll_up),
	{ ALT, 'v',  		edit_f_scroll_down },
	
	CTRL_ENTRY('k', 	edit_f_kill_line),
	CTRL_ENTRY('y', 	edit_f_yank),
	
	CTRL_ENTRY('d', 	edit_f_delete_char),
	CTRL_ENTRY('h',		edit_f_delete_backward_char),
	{ ALT, 'd',		edit_f_kill_word },

	{ ALT | SHIFT, '>',	edit_f_end_of_buffer },
	{ ALT | SHIFT, '<',	edit_f_beginning_of_buffer },

	{ ALT, 0177,		edit_f_backward_kill_word },
	{ ALT, '\\',		edit_f_delete_horizontal_space },

	{ CTRL, '[',		edit_f_request_quit }
};

static Edit_keymap Default_keymap = {
	NULL,
	Default_keymap_tab,
	sizeof(Default_keymap_tab) / sizeof(*Default_keymap_tab),
	edit_f_self_insert,
	NULL
};

Edit_keymap *
edit_create_keymap(next)
Edit_keymap *next;
{
	Edit_keymap *km;

	km = (Edit_keymap *)e_malloc(sizeof(Edit_keymap));
	km->klist = NULL;
	km->ktab = NULL;
	km->ktab_size = 0;
	km->default_function = edit_f_undef;
	km->next = next;

	return km;
}

void
edit_destroy_keymap(km)
Edit_keymap *km;
{
	Keymap_list *kl, *next;
	
	for (kl = km->klist; kl != NULL; kl = next) {
		next = kl->next;
		free((char *)kl);
	}

	free((char *)km);
}

void
edit_add_keymap_entry(km, modifiers, key, function)
Edit_keymap *km;
unsigned modifiers;
int key;
Edit_function function;
{
	Keymap_list *kl;

	kl = (Keymap_list *)e_malloc(sizeof(Keymap_list));
	kl->entry.key = key;
	kl->entry.modifiers = modifiers;
	kl->entry.function = function;
	kl->next = km->klist;
	km->klist = kl;
}

Edit_keymap *
edit_get_default_keymap()
{
	return &Default_keymap;
}

Edit_keymap *
edit_get_next_keymap(km)
Edit_keymap *km;
{
	return km->next;
}

Edit_function
edit_lookup_key(keymap, modifiers, key)
Edit_keymap *keymap;
unsigned modifiers;
int key;
{
	Edit_keymap *km;
	Edit_function function;

	function = edit_f_undef;
	
	for (km = keymap; km != NULL; km = km->next) {
		int i;
		Keymap_list *kl;

		for (kl = km->klist; kl != NULL; kl = kl->next) {
			if (kl->entry.key == key &&
			    kl->entry.modifiers == modifiers)
				return kl->entry.function;
		}
		
		for (i = 0; i < km->ktab_size; ++i) {
			if (km->ktab[i].key == key &&
			    km->ktab[i].modifiers == modifiers)
				return km->ktab[i].function;
		}

		function = km->default_function;
	}

	return function;
}

