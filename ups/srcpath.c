/* srcpath.c - locate source files in the source search path */

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


/* @(#)srcpath.c	1.3 20 Jun 1995 (UKC) */
char ups_srcpath_c_rcsid[] = "$Id$";

#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <local/ukcprog.h>

#include "srcbuf.h"
#include "srcwin.h"
#include "srcpath.h"

static bool locate_file PROTO((const char *path_hint, const char *path,
			       const char **p_fullpath));
static bool search_source_path PROTO((const char *path_hint, const char *name,
				      const char **p_fullpath));

static Pathentry *Pathlist = NULL, *Pathlist_tail;

static bool Use_path_hints = TRUE;

void
srcpath_free_pathlist()
{
  Pathentry *pl, *next;

  for (pl = Pathlist; pl != NULL; pl = next)
  {
    next = pl->next;
    free((char *)pl->path);
    free(pl);
  }
  Pathlist = NULL;
  Pathlist_tail = NULL;
}

Pathentry *
get_source_path_root()
{
  return (Pathlist);
}


Pathentry *
get_source_path_tail()
{
  return (Pathlist_tail);
}

void
srcpath_add_path(path)
const char *path;
{
	Pathentry *pl;

	pl = (Pathentry *)e_malloc(sizeof(Pathentry));
	pl->path = strf("%s/", path);
	pl->next = NULL;

	if (Pathlist == NULL)
		Pathlist = pl;
	else
		Pathlist_tail->next = pl;
	Pathlist_tail = pl;
}

void
srcpath_check_and_add_path(ro_paths)
const char *ro_paths;
{
  Pathentry *pl;
  char *last, *paths, *pathcopy;
  
  pathcopy = paths = strsave(ro_paths);
  last = paths;
  for (;;) {
    while (*paths == ':' && *paths != '\0')
      paths++;
    if (*paths == '\0')
      break;
    last = paths;
    while (*paths != ':' && *paths != '\0')
      paths++;
    if (*paths == ':')
      *paths++ = '\0';
    for (pl = Pathlist; pl != NULL; pl = pl->next)
      if (!strcmp(pl->path, last))
      {
	errf("`%s' already in search path", last);
	free(pathcopy);
	return;
      }
    srcpath_add_path(last);
    errf("\bAdding `%s' to search path", last);
  }
  free(pathcopy);
}

void
srcpath_ignore_path_hints()
{
	Use_path_hints = FALSE;
}

bool
srcpath_resolve_path(what, path_hint, name, p_fullpath)
const char *what, *path_hint, *name, **p_fullpath;
{
	const char *prefix, *sepstr;

	if (locate_file(path_hint, name, p_fullpath))
		return TRUE;
	
	if (path_hint == NULL) {
		prefix = sepstr = "";
	}
	else {
		if (*name != '/' && *path_hint != '\0' &&
		    path_hint[strlen(path_hint) - 1] != '/') {
			sepstr = "/";
		}
		else {
			sepstr = "";
		}
		prefix = path_hint;
	}
			
	if (*prefix == '/' || (*prefix == '\0' && *name == '/')) {
		errf("\b%c%s `%s%s%s' nonexistent",
		     toupper(*what), &what[1], prefix, sepstr, name);
	}
	else {
		errf("\bCan't find %s `%s%s%s'", what, prefix, sepstr, name);
	}

	return FALSE;
}

Srcbuf *
srcpath_visit_file(ap, path_hint, name)
alloc_pool_t *ap;
const char *path_hint, *name;
{
	const char *fullpath;

	if (!srcpath_resolve_path("source file", path_hint, name, &fullpath))
		return NULL;
	
	return srcwin_visit_file(ap, "source file ", fullpath);
}

bool
srcpath_file_exists(path_hint, name)
const char *path_hint, *name;
{
	const char *fullpath;
	
	return locate_file(path_hint, name, &fullpath) &&
	       access(fullpath, F_OK) == 0;
}

static bool
locate_file(path_hint, name, p_fullpath)
const char *path_hint, *name;
const char **p_fullpath;
{
	bool found;

	found = FALSE;

	if (*name != '/' && path_hint != NULL && Use_path_hints)
		found = search_source_path(path_hint, name, p_fullpath);

	if (!found) {
		found = search_source_path((const char *)NULL,
					   name, p_fullpath);
	}

	/*  If the file doesn't exist try knocking any directory
	 *  components off it.
	 */
	if (!found) {
		const char *filename = base_name(name);

		if (filename != name) {
			found = search_source_path((const char *)NULL,
						   filename, p_fullpath);
			/* RGA try with path hint */
			if (!found) 
			  found = search_source_path(path_hint,
						     filename, p_fullpath);
		}
	}

	return found;
}

static bool
search_source_path(path_hint, name, p_fullpath)
const char *path_hint, *name;
const char **p_fullpath;
{
	static ebuf_t *eb = NULL;
	const char *fullpath;
	Pathentry *pl;

	eb = ebuf_start(eb, TRUE);
	
	if (path_hint != NULL) {
		size_t len;
		
		ebuf_addstr(eb, path_hint);

		len = strlen(path_hint);
		
		if (*name != '/' && len != 0 && path_hint[len - 1] != '/')
			ebuf_add(eb, "/", 1);
	}
	
	ebuf_addstr(eb, name);
	ebuf_add(eb, "", 1);
	
	fullpath = ebuf_get(eb, (int *)NULL);
	
	if (*fullpath == '/') {
		*p_fullpath = fullpath;
		return access(fullpath, F_OK) == 0;
	}

	for (pl = Pathlist; pl != NULL; pl = pl->next) {
		ebuf_reset(eb);
	
		ebuf_addstr(eb, pl->path);
		
		if (path_hint != NULL) {
			ebuf_addstr(eb, path_hint);
			ebuf_add(eb, "/", 1);
		}
		
		ebuf_addstr(eb, name);
		ebuf_add(eb, "",  1);
		
		fullpath = ebuf_get(eb, (int *)NULL);

		if (access(fullpath, F_OK) == 0) {
			*p_fullpath = fullpath;
			return TRUE;
		}
	}

	return FALSE;
}
