/* menudata.h - definitions of menu return values for menudata.c */

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


/* @(#)menudata.h	1.7 04 Jun 1995 (UKC) */

/*  The ups menus were created using the UKC med3 menu editor.
 *  This uses character values like 'a' as return values.
 *  Unfortunately there is no way to automatically tie these to
 *  a C #define or enum, so we do this by hand in this file.
 */

/*  Return values from menus/varmen.c
 */
#define MR_VAR_STRING			's'
#define MR_VAR_SIGNED_DECIMAL		't'
#define MR_VAR_UNSIGNED_DECIMAL		'T'
#define MR_VAR_SIGNED_OCTAL		'o'
#define MR_VAR_UNSIGNED_OCTAL		'O'
#define MR_VAR_SIGNED_HEX		'h'
#define MR_VAR_UNSIGNED_HEX		'H'
#define MR_VAR_UNSIGNED_BINARY		'B'
#define MR_VAR_DEFAULT_FORMAT		'n'
#define MR_VAR_ASCII_BYTE		'a'
#define MR_VAR_DEREF	 		'u'
#define MR_VAR_ADDRESS			'd'
#define MR_VAR_EXPAND_LIKE_BEFORE	'e'
#define MR_VAR_EXPAND_COMPLETELY	'E'
#define MR_VAR_DUP			'D'
#define MR_VAR_DELETE			'k'
#define MR_VAR_COLLAPSE			'c'
#define MR_VAR_COLLAPSE_COMPLETELY	'C'
#define MR_VAR_WANT_TYPEDEFS		'U'
#define MR_VAR_NO_TYPEDEFS		'I'
#define MR_VAR_SHOW_HIERARCHY		'y' /* RCB */
#define MR_VAR_SHOW_STATICS		'S' /* RCB */
#define MR_VAR_ACCELERATOR_ACTION	'q' /* RCB */
#define MR_VAR_DUMP_TO_OUTWIN		'W' /* RCB */
#define MR_VAR_SHOW_SIZE		'X' /* RGA */
#define MR_VAR_ADD_WATCHPOINT           'w'

/*  Return values from menus/srcmen.c
 */
#define MR_SRCWIN_BACK			'r'
#define MR_SRCWIN_SEARCH_FORWARDS	'f'
#define MR_SRCWIN_SEARCH_BACKWARDS	'b'
#define MR_SRCWIN_UP_STACK		'u'
#define MR_SRCWIN_DOWN_STACK		'd'
#define MR_SRCWIN_SEARCH                's'

/*  Return values from menus/outmen.c
 */
#define MR_OUTWIN_LOAD			'l'
#define MR_OUTWIN_SAVE			's'
#define MR_OUTWIN_CLEAR			'c'
#define MR_OUTWIN_SEARCH_FORWARDS	'f'
#define MR_OUTWIN_SEARCH_BACKWARDS	'b'
#define MR_OUTWIN_CLOSE			'C'
#define MR_OUTWIN_SEARCH                'x'

/*  Return values from menus/bphmen.c
 */
#define MR_ADD_BREAKPOINT		'a'
#define MR_REMOVE_ALL_BREAKPOINTS	'D'
#define MR_ENABLE_ALL_BREAKPOINTS	'e'
#define MR_DISABLE_ALL_BREAKPOINTS	'd'
#define MR_RESTORE_BREAKPOINTS		'r'
#define MR_LOAD_BREAKPOINTS_EXC		'l'
#define MR_LOAD_BREAKPOINTS_INC		'm'
#define MR_BP_ACCELERATOR_ACTION	'q' /* RCB */

/*  Return values from menus/bptmen.c
 */
#define MR_BPTMEN_REMOVE		'd'
#define MR_BPTMEN_SOURCE		'e'
#define MR_BPTMEN_SAVE			's'
#define MR_BPTMEN_ACTIVE		'x'
#define MR_BPTMEN_INACTIVE		'y'
#define MR_BPTMEN_EXECUTE		'z'
#define MR_BPTMEN_ACCELERATOR_ACTION	'q'	/* RCB */

/*  Return values from menus/gblmen.c
 */
#define MR_SHOW_UNTYPED_VARS		'a'
#define MR_HIDE_UNTYPED_VARS		'c'

/*  Return values from menus/cbhmen.c
 */
#define MR_SHOW_COMMON_BLOCKS		'e'
#define MR_HIDE_ALL_COMMON_BLOCKS	'c'

/*  Alternative names for the above, when used for modules.
 */
#define MR_SHOW_MODULES			'e'
#define MR_HIDE_ALL_MODULES		'c'

/*  Return values from menus/cblockmen.c
 */
#define MR_EXPAND_COMMON_BLOCK_LIKE_BEFORE	'e'
#define MR_EXPAND_COMMON_BLOCK_COMPLETELY	'E'
#define MR_COLLAPSE_COMMON_BLOCK		'c'
#define MR_HIDE_COMMON_BLOCK			'h'

/*  Alternative names for the above, when used for modules.
 */
#define MR_EXPAND_MODULE		'e'
#define MR_COLLAPSE_MODULE		'c'
#define MR_HIDE_MODULE			'h'

/*  Return values from menus/mfuncmen.c
 */
#define MR_MODULE_FUNC_DISPLAY_SOURCE	'e'
#define MR_MODULE_FUNC_HIDE		'h'

/*  Return values from menus/shmen.c
 */
#define MR_SHOW_SOURCE_FILES		'e'
#define MR_HIDE_SOURCE_FILES		'c'
#define MR_HIDE_SOURCE_FILES_COMPLETELY 'C'
#define MR_ADD_SOURCE_PATH		'a'

/*  Return values from menus/nolibmen.c
 */
#define MR_LOAD_LIBRARY			'l'

/*  Return values from menus/varmen.c
 */
#define MR_EXPAND_LIKE_BEFORE		'B'
#define MR_EXPAND_COMPLETELY		'E'
#define MR_COLLAPSE			'c'
#define MR_DISPLAY_SOURCE		'e'
#define MR_ADD_EXPRESSION		'x'
#define MR_SHOW_PATH			'p'
#define MR_SHOW_SOURCE_PATH		's'
#define MR_REMATCH_SOURCE_FILE		'r'
#define MR_RELOAD_SOURCE_FILE		'l'
#define MR_FILE_DATES			'm'

/*  Return values from menus/blockmen.c
 */
#define MR_BLOCK_EXPAND			'e'
#define MR_BLOCK_COLLAPSE		'c'
#define MR_BLOCK_DELETE			'd'
#define MR_BLOCK_DISPLAY_SOURCE		's'
#define MR_BLOCK_ADD_EXPRESSION		'x'


/*  Return values from menus/sghmen.c
 */
#define MR_SHOW_ALL_SIGNALS		'a'
#define MR_HIDE_ALL_SIGNALS		'c'

/*  Return values from menus/sigmen.c
 */
#define MR_SIG_TOGGLE_STOP_CONT		's'
#define MR_SIG_TOGGLE_ACCEPT_IGNORE	'a'
#define MR_SIG_TOGGLE_REDRAW		'r'
#define MR_HIDE_SIGNAL			'h'

/*  Return values from menus/cmdmen.c
 */
#define MR_QUIT_UPS			'q'
#define MR_SNAPSHOT_ALL			'w'
#define MR_SNAPSHOT_SELECTED		'c'
#define MR_TOGGLE_LOGGING		'x'
	/* These are no longer accessible through cmdmen.c but retained
	** for use in command scripts: */
#define MR_LOGGING_ON			('x'&0xf)
#define MR_LOGGING_OFF			('y'&0xf)
#define MR_RAISE_DISPLAY_WIN		'R'
#define MR_RAISE_SRC_WIN		'r'
#define MR_RAISE_OUT_WIN		'z'
#define MR_NO_RAISE_ON_BREAK		Mrvalue(1,'a')
#define MR_RAISE_ON_BREAK		'j'
#define MR_LOWER_ON_RUN			'y'
#define MR_ICONIFY_ON_RUN		Mrvalue(1,'b')
#define MR_DONT_QUIT			'?'
#define MR_DISWIN_SEARCH_FORWARDS	'f'
#define MR_DISWIN_SEARCH_BACKWARDS	'b'
#define MR_HELP_00			't'
#define MR_HELP_0A			'o'
#define MR_HELP_0B			'0'
#define MR_HELP_1A			'A'
#define MR_HELP_1B			'B'
#define MR_HELP_1C			'u'
#define MR_HELP_1D			'C'
#define MR_HELP_1E			'D'
#define MR_HELP_1F			'E'
#define MR_HELP_1G			'F'
#define MR_HELP_1H			'G'
#define MR_HELP_1I			'H'
#define MR_HELP_1J			'd'
#define MR_HELP_2A			'2'
#define MR_HELP_2B			'p'
#define MR_HELP_3A			'I'
#define MR_HELP_3B			'J'
#define MR_HELP_3C			'K'
#define MR_HELP_3D			'L'
#define MR_HELP_3E			'M'
#define MR_HELP_3F			'N'
#define MR_HELP_3G			'n'
#define MR_HELP_4			'4'
#define MR_HELP_5A			'S'
#define MR_HELP_5B			'T'
#define MR_HELP_5C			'U'
#define MR_HELP_5D			'V'
#define MR_HELP_5E			'W'
#define MR_HELP_5F			'X'
#define MR_HELP_5G			'Y'
#define MR_HELP_5H			'Z'
#define MR_HELP_5I			'a'
#define MR_HELP_6A			'6'
#define MR_HELP_6B			'e'
#define MR_HELP_6C			'i'
#define MR_HELP_7			'7'
#define MR_HELP_8			'8'
#define MR_HELP_9			'9'
#define MR_HELP_10			'!'
#define MR_HELP_11A		Mrvalue(1,'c') /* About X resources */
#define MR_HELP_11B		Mrvalue(1,'d') /* fonts & colors */
#define MR_HELP_11C		Mrvalue(1,'e') /* split screens */
#define MR_HELP_11D		Mrvalue(1,'f') /* Raise/Lower */
#define MR_HELP_11E		Mrvalue(1,'g') /* Breakpoint Menu */
#define MR_HELP_11F		Mrvalue(1,'h') /* Scrolling */
#define MR_HELP_11G			'g'    /* resizing windows */
#define MR_HELP_11H		Mrvalue(1,'i') /* No Mousehole */
#define MR_HELP_12			'h'
#define MR_HELP_13A			'v'
#define MR_HELP_13B			's'
#define MR_HELP_13C			'O'
#define MR_HELP_13D			'P'
#define MR_HELP_13E			'Q'
#define MR_HELP_14A			'l'
#define MR_HELP_14B			'm'

/*  Return values from menus/coremen.c
 */
#define MR_LOAD_ONE_LIBRARY		'Z'
#define MR_SAVE_STATE			's'
#define MR_LOAD_STATE			'l'
#define MR_DROP_STATE			'd'
#define MR_RESCAN_INIT_FILE		'i'

/*  Return values from the target menus.  The target captions
 *  are done as seperate menus to support the stop button.
 */
#define MR_TGT_START			'r'
#define MR_TGT_RESTART			'R'
#define MR_TGT_DONT_RESTART		'@'
#define MR_TGT_CONT			'c'
#define MR_TGT_STEP			's'
#define MR_TGT_NEXT			'n'
#define MR_TGT_STOP			'S'
#define MR_TGT_ATTACH			'v'
#define MR_TGT_DETACH			'w'
#define MR_TGT_KILL			'k'
#define MR_TGT_DONT_KILL		'?'
#define MR_TGT_INIT			'i' /* RGA used when editing target name
					     - not a menu option */

/*  Return values from menus/envheadmen.c
 */
#define MR_RESET_ENVTAB			'r'
#define MR_SHOW_ALL_ENV_ENTRIES		's'
#define MR_HIDE_ALL_ENV_ENTRIES		'h'
#define MR_ADD_ENV_ENTRY		'a'

/*  Return values from menus/envmen.c
 */
#define MR_DELETE_ENV_ENTRY		'd'
#define MR_HIDE_ENV_ENTRY		'h'
#define MR_APPEND_ENV_ENTRY		'a'

/*  Return values from menus/wphmen.c
 */
#define MR_ADD_WATCHPOINT		'a'
#define MR_REMOVE_ALL_WATCHPOINTS	'D'
#define MR_ENABLE_ALL_WATCHPOINTS	'e'
#define MR_DISABLE_ALL_WATCHPOINTS	'd'
#define MR_WP_ACCELERATOR_ACTION	'q'

/*  Returns from menus/wptmen.c
 */
#define MR_WPTMEN_REMOVE		'd'
#define MR_WPTMEN_ACTIVE		'x'
#define MR_WPTMEN_INACTIVE		'y'
#define MR_WPTMEN_ACCELERATOR_ACTION	'q'
