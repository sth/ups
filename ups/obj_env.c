/* obj_env.c - target environment handling routines */

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


/* @(#)obj_env.c	1.15 09 Apr 1995 (UKC) */
char ups_obj_env_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <local/ukcprog.h>
#include <local/wn.h>
#include <local/obj/obj.h>
#include <local/edit/edit.h>

#include "objtypes.h"
#include "ui.h"
#include "obj_env.h"
#include "obj_util.h"
#include "menudata.h"
#include "tdr.h"

#if !HAVE_DEFINED_ENVIRON
extern const char **environ;
#endif

/*  An element in the circular doubly linked list of environment entries.
 */
typedef struct enventryst {
	struct enventryst *ee_prev;
	struct enventryst *ee_next;
	char *ee_value;
	bool ee_hidden;
} enventry_t;

static void make_envtab PROTO((void));
static void add_env_object PROTO((enventry_t *ee));
static void free_envtab PROTO((void));
static void env_edit PROTO((draw_t fdets));
static enventry_t *search_envtab PROTO((const char *s, size_t len,
							enventry_t *orig_ee));
static void add_env_entry PROTO((enventry_t *orig));
static bool env_quitfunc PROTO((objid_t obj, char *new_text,
				 const char **p_display_string));

/*  Maximum length of an individual environment string.
 */
#define MAX_ENV_LEN	4096

const char Env_format[] = "%[-]:200cE  \n";

fdef_t Env_fdefs[] = {
	{ 'E', e_draw, env_edit, NULL	},
	{ '\0', NULL, NULL, NULL	},
};

static enventry_t *Envtab = NULL;

#define FN_ENV_VALUE		0

Edit_history* env_history = NULL;
fnamemap_t Env_fnamemap[] = {
	{ FN_ENV_VALUE,	"env-var-value",	TRUE,	env_quitfunc,	&env_history	},
	{ 0,		NULL,			FALSE,	NULL,		NULL		},
};

const char Envhead_format[] = "Environment\n";

#define ENVHEAD_OBJCODE	((objid_t)Envhead_format)

const char *
env_getobjname(obj)
objid_t obj;
{
	static char *last = NULL;
	enventry_t *ee, *ee2;
	size_t len;
	int count;
	char *name;
	const char *s;

	ee = (enventry_t *)obj;

	for (s = ee->ee_value; *s != '\0' && *s != '='; ++s)
		;
	if (*s == '=')
		++s;
	len = s - ee->ee_value;

	if (len == 0) {
		name = strsave("[BLANK]");
	}
	else {
		name = e_malloc(len + 1);
		memcpy(name, ee->ee_value, len);
		name[len] = '\0';
	}

	count = 1;
	for (ee2 = Envtab->ee_next; ee2 != Envtab; ee2 = ee2->ee_next) {
		if (ee2 == ee)
			break;
		if (strncmp(ee2->ee_value, name, len) == 0)
			++count;
	}

	if (last == NULL)
		free(last);

	if (count == 1) {
		last = name;
	}
	else {
		last = strf("%d-%s", count, name);
		free(name);
	}

	return last;
}

/*  Add the signal list header to the form.
 */
void
add_env_header(par)
objid_t par;
{
	new_object(ENVHEAD_OBJCODE, OT_ENVHEAD, par, OBJ_CHILD);
}

void
free_env(obj)
objid_t obj;
{
	enventry_t *ee;

	ee = (enventry_t *)obj;

	if (!ee->ee_hidden) {
		ee->ee_prev->ee_next = ee->ee_next;
		ee->ee_next->ee_prev = ee->ee_prev;
		free(ee->ee_value);
		free((char *)ee);
	}
}

void
env_getsize(obj, unused_par, sz)
objid_t obj, unused_par;
struct szst *sz;
{
	font_t *font;
	const char *s;

	font = wn_get_sysfont();

	sz->sz_depth = font->ft_height;

	/*  We don't want a zero width object for an empty environment string.
	 */
	s = ((enventry_t *)obj)->ee_value;
	sz->sz_width = *s == '\0' ? font->ft_width / 2
				  : font->ft_width * strlen(s);
}

/*  Process the return from the signal header menu. Expand or collapse
 *  the signal list
 */
void
do_envhead(obj, command, arg)
objid_t obj;
int command;
char *arg;
{
	enventry_t *ee;

	switch(command) {
	case MR_ADD_ENV_ENTRY:
		if (Envtab == NULL)
			make_envtab();
		add_env_entry(Envtab);
		break;
	case MR_RESET_ENVTAB:
		do_envhead(obj, MR_HIDE_ALL_ENV_ENTRIES, (char *)NULL);
		if (Envtab != NULL)
			free_envtab();
		do_envhead(obj, MR_SHOW_ALL_ENV_ENTRIES, (char *)NULL);
		break;
	case MR_SHOW_ALL_ENV_ENTRIES:
		if (Envtab == NULL)
			make_envtab();
		for (ee = Envtab->ee_next; ee != Envtab; ee = ee->ee_next) {
			if (ee->ee_hidden)
				add_env_object(ee);
		}
		break;
	case MR_HIDE_ALL_ENV_ENTRIES:
		if (Envtab != NULL) {
			for (ee = Envtab->ee_next; ee != Envtab; ee = ee->ee_next)
				ee->ee_hidden = TRUE;
			remove_object(obj, OBJ_DESCENDENTS);
		}
		break;
	default:
		panic("bad cmd in dsh");
	}
}

const char **
get_environment()
{
	static const char **etab = NULL;
	static int etab_size = 0;
	enventry_t *ee;
	int nenv, i;

	if (Envtab == NULL)
		return (const char **)environ;
	
	nenv = 1;	/* For the final NULL */
	for (ee = Envtab->ee_next; ee != Envtab; ee = ee->ee_next)
		++nenv;
	
	if (nenv > etab_size) {
		if (etab != NULL)
			free((char *)etab);
		etab_size = nenv + 10;		/* add a little slop */
		etab = (const char **)e_malloc(etab_size * sizeof(char *));
	}

	i = 0;
	for (ee = Envtab->ee_next; ee != Envtab; ee = ee->ee_next)
		etab[i++] = ee->ee_value;
	etab[i] = NULL;

	return etab;
}

/*  Build an initial environment list from the environ supplied
 *  environment.
 */
static void
make_envtab()
{
	const char **envp;

	if (Envtab != NULL)
		panic("et botch in me");
	
	Envtab = (enventry_t *)e_malloc(sizeof(enventry_t));
	Envtab->ee_next = Envtab->ee_prev = Envtab;

	for (envp = (const char **)environ; *envp != NULL; ++envp) {
		enventry_t *ee;

		ee = (enventry_t *)e_malloc(sizeof(enventry_t));
		ee->ee_value = strsave(*envp);
		ee->ee_hidden = TRUE;

		/*  Link the new entry in at the tail of the list.
		 *  This is so we end up with the list in the same order
		 *  as the entries in environ.
		 */
		ee->ee_next = Envtab;
		ee->ee_prev = Envtab->ee_prev;
		ee->ee_prev->ee_next = ee;
		Envtab->ee_prev = ee;
	}
}

static void
add_env_entry(orig)
enventry_t *orig;
{
	enventry_t *ee;
	int oldstate;

	if (Envtab == NULL)
		panic("et botch in mkee");

	ee = (enventry_t *)e_malloc(sizeof(enventry_t));
	ee->ee_value = strsave("");
	ee->ee_hidden = TRUE;

	ee->ee_prev = orig;
	ee->ee_next = orig->ee_next;
	ee->ee_next->ee_prev = ee;
	orig->ee_next = ee;

	add_env_object(ee);
	ensure_visible((objid_t)ee);

	oldstate = td_set_obj_updating(OBJ_UPDATING_ON);
	edit_field_obj((objid_t)ee, FN_ENV_VALUE);
	if (ee->ee_hidden) {
		ee->ee_hidden = FALSE;
		remove_object((objid_t)ee, OBJ_SELF);
	}
	td_set_obj_updating(oldstate);
}

static void
free_envtab()
{
	enventry_t *ee, *next;

	for (ee = Envtab->ee_next; ee != Envtab; ee = next) {
		next = ee->ee_next;
		free(ee->ee_value);
		free((char *)ee);
	}

	free((char *)Envtab);
	Envtab = NULL;
}

static void
add_env_object(ee)
enventry_t *ee;
{
	if (ee->ee_prev == Envtab)
		new_object((objid_t)ee, OT_ENV, ENVHEAD_OBJCODE, OBJ_FIRST_CHILD);
	else
		new_object((objid_t)ee, OT_ENV, (objid_t)ee->ee_prev, OBJ_AFTER);

	set_field_value((objid_t)ee, FN_ENV_VALUE, (fval_t)ee->ee_value);
	ee->ee_hidden = FALSE;
}

static bool
env_quitfunc(obj, new_text, p_display_string)
objid_t obj;
char *new_text;
const char **p_display_string;
{
	enventry_t *ee;
	const char *pos;

	ee = (enventry_t *)obj;

	free(ee->ee_value);
	ee->ee_value = strsave(new_text);
	set_field_value((objid_t)ee, FN_ENV_VALUE, (fval_t)ee->ee_value);

	if (*ee->ee_value == '\0') {
		errf("Warning: zero length enviroment entry");
	}
	else if ((pos = strchr(ee->ee_value, '=')) == NULL ||
		 pos == ee->ee_value) {
		errf("Warning: entry not in name=value format");
	}
	else if (search_envtab(ee->ee_value, (size_t)(pos - ee->ee_value),
								  ee) != NULL) {
		errf("Warning: duplicate entry for %.*s",
		     pos - ee->ee_value, ee->ee_value);
	}

	return TRUE;
}

static enventry_t *
search_envtab(s, len, orig_ee)
const char *s;
size_t len;
enventry_t *orig_ee;
{
	enventry_t *ee;
	const char *pos;

	for (ee = Envtab->ee_next; ee != Envtab; ee = ee->ee_next) {
		if (ee != orig_ee &&
		    (pos = strchr(ee->ee_value, '=')) != NULL &&
		    pos - ee->ee_value == len &&
		    memcmp(s, ee->ee_value, len) == 0)
			return ee;
	}

	return NULL;
}

static void
env_edit(fdets)
draw_t fdets;
{
	field_edit_start(&fdets, "environment variable", (char *)NULL);
}

char *
env_format_obj(code)
objid_t code;
{
	return strf("%s", ((enventry_t *)code)->ee_value);
}

/*  Process the return from a signal menu. Switching attributes on and off.
 */
void
do_env(obj, command, arg)
objid_t obj;
int command;
char *arg;
{
	switch(command) {
	case MR_APPEND_ENV_ENTRY:
		add_env_entry((enventry_t *)obj);
		break;
	case MR_DELETE_ENV_ENTRY:
		remove_object(obj, OBJ_SELF);
		break;
	case MR_HIDE_ENV_ENTRY:
		((enventry_t *)obj)->ee_hidden = TRUE;
		remove_object(obj, OBJ_SELF);
		break;
	default:
		panic("bad cmd in de");
	}
}

void
env_getcolor(obj, wn, p_fg, p_bg)
objid_t obj;
int wn;
long *p_fg, *p_bg;
{
  static color_t color= { -1, 0, 0, 0 };

  if (color.co_pixel == -1)
  {
    const char *colorname;
    
    color.co_pixel = WN_FG(wn);
    if (wn_use_extended_text_colors() == TRUE)
    {
      colorname = wn_get_default("EnvironmentTextColor");
      if (colorname != NULL)
      {
	wn_parse_color(wn, colorname, &color);
	if (!wn_alloc_color(wn, &color,colorname))
	  color.co_pixel = WN_FG(wn);
      }
    }
  }
  *p_fg = color.co_pixel;
  *p_bg = WN_BG(wn);
}
