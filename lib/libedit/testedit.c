/* testedit.c - test the editing routines */

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


/* @(#)testedit.c	1.12 09 Apr 1995 (UKC) */
char edit_testedit_c_rcsid[] = "$Id$";

#include <sys/types.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <local/wn.h>
#include <local/ukcprog.h>
#include <mtrprog/utils.h>

#include "edit.h"
#include "textbuf.h"
#include "simplebuf.h"
#include "lbuf.h"
#include "filebuf.h"
#include "win.h"

#define MARGIN 2

int main PROTO((int argc, char **argv));
static Edit_display *add_display PROTO((Edit_buffer *buffer, bool want_checks,
					int wn, int fg, int bg,
					bool want_cursor,
					bool keep_cursor_visible,
					size_t start_point, int pixel_offset,
					size_t point, int tabspaces));
static void make_subwins PROTO((int parwn, bool want_two_wins, int sline_height,
				int *p_wn1, int *p_wn2, int *p_wn_sline));
static void box_and_clear PROTO((int wn, int bg));
static void handle_new_window_size PROTO((Edit_display *d, int wn, int bg));
static void scroll PROTO((event_t *ev, Edit_display *d));
static int get_value PROTO((char **argv, const char *what));
static void drag PROTO((event_t *ev, Edit_display *d));
static void loop_scroll PROTO(( Edit_display *d, int delta, int count));
static void do_selection PROTO((event_t *ev, Edit_display *d,
				bool restrict_to_sel,
				Edit_fontinfo *fi1, Edit_fontinfo *fi2,
				Edit_propchange **p_startpc,
				Edit_propchange **p_endpc));
static const char *checkarg PROTO((char **argv, const char *what));
static Edit_fontinfo *make_fontinfo PROTO((const char *name,
					   int nlines, int nchars));
static void event_loop PROTO((Edit_display *d1, Edit_display *d2,
			      Edit_display *ds,
			      int wn, int wn1, int wn2, int wn_sline,
			      Edit_fontinfo *fi1, Edit_fontinfo *fi2,
			      Edit_propchange *pc1, Edit_propchange *pc2,
			      int up_delta, int down_delta, int loop_count));
static Text_buffer *make_simplebuf PROTO((const char *path));
static const char *lookup_fontname PROTO((const char *name));
static const char *make_fontname PROTO((const char *origpat));
static void move_pc PROTO((Edit_display *d, size_t *p_selpos, int delta, 
                           Edit_propchange *pc1, Edit_propchange *pc2, 
                           Edit_propchange **p_movepc));
static void remove_selection PROTO((event_t *ev, Edit_display *d));
static void show_error_and_beep PROTO((const char *mesg));
static void add_mfont_props PROTO((Edit_buffer *buffer, const char *fontname, 
                                   Edit_fontinfo *basefi,
				   const char *sample_text));
static const char *get_file_contents PROTO((const char *path));
static int make_subwin PROTO((int par, int xstart, int xlim, int ystart, 
                              int ylim));
static void dump_buffer_by_lines PROTO((Edit_display *display));
static void dump_buffer_by_words PROTO((Edit_display *display));
static void dump_buffer PROTO((Edit_display *display, int endc));
static void dump_buffer_backwards_by_lines PROTO((Edit_display *display));
static void dump_buffer_backwards_by_words PROTO((Edit_display *display));
static void dump_buffer_backwards PROTO((Edit_display *display, int endc));

static const char *
checkarg(argv, what)
char **argv;
const char *what;
{
	if (*argv == NULL) {
		errf("Missing %s after %s", what, argv[-1]);
		exit(1);
	}

	return *argv;
}

static int
get_value(argv, what)
char **argv;
const char *what;
{
	int n;
	const char *s;
	char *ends;

	s = checkarg(argv, what);

	n = strtol(s, &ends, 0);
	
	if (s == ends || *ends != '\0') {
		errf("%s `%s' is not a decimal number", what, *argv);
		exit(1);
	}

	return n;
}

static const char *
lookup_fontname(name)
const char *name;
{
	static struct {
		const char *tag;
		const char *name;
	} fonttab[] = {
		{ "bv",	"-*-times-medium-r-normal--25-*-*-*-*-*-*-*"	},
		{ "bf",	"-*-courier-medium-r-normal--25-*-*-*-*-*-*-*"	},
		{ "v",	"-*-times-medium-r-normal--14-*-*-*-*-*-*-1"	},
		{ "10",	"-*-fixed-medium-r-normal--10-*-*-*-*-*-*-1"	},
		{ "sys", NULL						},
	};
	int i;
		
	for (i = 0; i < sizeof(fonttab) / sizeof(*fonttab); ++i) {
		if (strcmp(name, fonttab[i].tag) == 0) {
			return fonttab[i].name;
		}
	}

	return name;
}
	
static const char *
make_fontname(pat)
const char *pat;
{
	static const char *fndry[] = {
		"adobe:|adobe", "b&h:|b&h", "bitstream:|bitstream",
		"dec:|dec", "misc:|misc", "ncd:|ncd",
		"schumacher:|schumacher", "sony:|sony", "sun:|sun", NULL
	};
	static const char *family[] = {
		"clean", "fixed", "helvetica", "lucida", "menu",
		"new century schoolbook", "te|terminal", "times", NULL
	};
	static const char *weight[] = {
		"medium", "bold", "demibold", "*", "|medium", NULL
	};
	static const char *slant[] = {
		"r", "i", "o", "*", "|r", NULL
	};
	static const char *swdth[] = {
		"normal", "DW|double wide", "N|narrow", "W|wide",
		"SC|semicondensed", "*", "|normal", NULL
	};
	static const char *registry[] = {
		"/adobe|adobe", "/dec|dec", "/iso|iso8859",
		"/j76|jisx0201.1976", "/j83|jisx0201.1983",
		"/suncur|sololcursor", "/sunglyph|sunolglyph", NULL
	};
	static const char *pxlsz[] = {
		NULL
	};
	static const char **comps[] = {
		fndry, family, weight, slant, swdth, NULL, pxlsz, NULL,
		NULL, NULL, NULL, NULL, registry, NULL
	};
	static ebuf_t *eb = NULL;
	const char *origpat;
	int i;

	eb = ebuf_start(eb, TRUE);
	origpat = pat;

	for (i = 0; i < sizeof(comps) / sizeof(*comps); ++i) {
		ebuf_add(eb, "-", 1);

		if (comps[i] == NULL) {
			ebuf_add(eb, "*", 1);
		}
		else if (comps[i] == pxlsz) {
			const char *numstr;

			for (numstr = pat; isdigit(*pat); ++pat)
				;

			if (pat != numstr)
				ebuf_add(eb, numstr, (size_t)(pat - numstr));
			else
				ebuf_add(eb, "*", 1);
		}
		else {
			const char **sptr, *pipec, *field;
			size_t taglen;

			field = NULL;		/* to satisfy gcc */
			
			for (sptr = comps[i]; *sptr != NULL; ++sptr) {
				if ((pipec = strchr(*sptr, '|')) != NULL) {
					taglen = pipec - *sptr;
					field = pipec + 1;
				}
				else {
					taglen = 1;
					field = *sptr;
				}
				
				if (strlen(pat) >= taglen &&
				    memcmp(pat, *sptr, taglen) == 0) {
					pat += taglen;
					break;
				}
			}

			ebuf_addstr(eb, (*sptr != NULL) ? field : "*");
		}
	}

	if (*pat != '\0') {
		errf("Unknown tag starting at `%s' in font pattern `%s'",
		     pat, origpat);
		return NULL;
	}

	ebuf_add(eb, "", 1);
	
	return ebuf_get(eb, (int *)NULL);
}

static Edit_fontinfo *
make_fontinfo(name, nlines, nchars)
const char *name;
int nlines, nchars;
{
	font_t *font;
	const char *pat;
	bool verbose;

	pat = NULL;
	verbose = FALSE;
	
	if (name != NULL) {
		if (*name == '/') {
			pat = name + 1;
			
			if (*pat == '/') {
				verbose = TRUE;
				++pat;
			}
			
			if ((name = make_fontname(pat)) == NULL)
				exit(1);
		}
		else {
			name = lookup_fontname(name);
		}
	}

	if (name == NULL) {
		font = wn_get_sysfont();
	}
	else if ((font = wn_open_font(name)) == NULL) {
		errf("Can't open font `%s'", name);
		exit(1);
	}

	
	if (verbose) {
		int i, count;
		char **names;
		
		names = wn_list_fonts(name, 500, &count);

		printf("%s =>\n", pat);

		for (i = 0; i < count; ++i)
			printf("  %s\n", names[i]);

		wn_free_font_names(names);
	}
		
	if (nlines != 0 || nchars != 0) {
		if (nlines == 0)
			nlines = 3;
		
		if (nchars == 0)
			nchars = 15;

		wn_suggest_window_size(nchars * font->ft_width + 4,
				       nlines * font->ft_height + 4);
	}

	return edit_make_wn_fontinfo(font);
}

static void
add_mfont_props(buffer, fontname, basefi, sample_text)
Edit_buffer *buffer;
const char *fontname;
Edit_fontinfo *basefi;
const char *sample_text;
{
	int i, count;
	char **names;
	size_t point;

	if (*fontname == '/') {
		if ((fontname = make_fontname(fontname + 1)) == NULL)
			exit(1);
	}
	
	names = wn_list_fonts(fontname, 500, &count);

	point = 0;

	for (i = 0; i < count; ++i) {
		font_t *font;

		printf("  %s\n", names[i]);
		
		if ((font = wn_open_font(names[i])) == NULL) {
			errf("Can't open font `%s' - skipping it", names[i]);
			continue;
		}
		
		edit_add_propchange(buffer, point, (char *)NULL,
				    basefi, 1, 1, TRUE);
		
		edit_insert(buffer, point, names[i], strlen(names[i], TRUE));
		point += strlen(names[i]);
		edit_insert(buffer, point++, "\n", 1, TRUE);

		edit_add_propchange(buffer, point, (char *)NULL,
				    edit_make_wn_fontinfo(font), 1, 0,
				    TRUE);

		edit_insert(buffer, point, sample_text, strlen(sample_text), TRUE);
		point += strlen(sample_text);
	}
		
	wn_free_font_names(names);
}
	
static Text_buffer *
make_simplebuf(path)
const char *path;
{
	Text_buffer *tb;
	FILE *fp;
	size_t pos;
	int ch;
	
	if ((fp = fopen(path, "r")) == NULL) {
		failmesg("Can't open", "", path);
		exit(1);
	}

	tb = text_create_simple_buffer();
	pos = 0;

	while ((ch = getc(fp)) != EOF) {
		char c;

		c = ch;
		(*tb->ops->insert)(tb->bufdata, pos++, &c, 1);
	}

	return tb;
}
	
int
main(argc, argv)
int argc;
char **argv;
{
	const char *path, *mesg, *font1name, *font2name, *mfontname;
	const char *sample_text_file, *ro_text;
	int wn, wn1, wn2, wn_sline;
	int nlines, nchars, loop_count, up_delta, down_delta;
	int pixel_offset, tabspaces, sline_height;
	size_t blocksize, sel_start, sel_count, startpos, point_pos;
	bool want_two_wins, want_simplebuf, want_debug;
	bool want_cursor, keep_cursor_visible;
	bool want_checks, grey_on_startup, want_control_u;
	Edit_buffer *buffer;
	Edit_fontinfo *fi1, *fi2;
	Edit_display *d1, *d2, *ds;
	Edit_propchange *pc1, *pc2;
	Text_buffer *tb;
	static const char default_sample_text[] = "\
This is a line of text to demonstrate this font.  This line is about eighty\n\
characters long, so the right hand side of an xvt window would be here ===>\n\
This is some more text.\n\n";
	
	errf_set_progname(*argv);
	wn_munge_args(argc, (const char **)argv);
	
	want_simplebuf = FALSE;		/* -s */
	want_debug = FALSE;		/* -debug */
	sel_start = sel_count = 0;	/* -ss, -sc */
	blocksize = 1024;		/* -b blocksize */
	want_two_wins = FALSE;		/* -2 */
	sline_height = 0;		/* -sh npixels */
	down_delta = 1;			/* -j n, -jd n */
	up_delta = 1;			/* -j n, -ju n */
	font1name = NULL;	       	/* -f */
	font2name = NULL;	       	/* -f2 */
	nlines = 0;			/* -nl n */
	nchars = 0;			/* -nc n */
	path = NULL;			/* path */
	want_cursor = TRUE;		/* -no-cursor */
	loop_count = 1;			/* -lc n */
	want_checks = TRUE;		/* -no-checks */
	grey_on_startup = FALSE;	/* -gs */
	startpos = 0;			/* -sp pos */
	pixel_offset = 0;		/* -po offset */
	mfontname = NULL;		/* -fp pat */
	sample_text_file = NULL; 	/* -st path */
	keep_cursor_visible = TRUE;	/* -nocv */
	point_pos = 0;			/* -pp point*/
	tabspaces = 8;			/* -ts tabspaces */
	ro_text = NULL;			/* -ro text */
	want_control_u = FALSE;		/* -cu */
	
	for (++argv; *argv != NULL; ++argv) {
		if (**argv != '-') {
			if (path != NULL) {
				errf("filename given twice");
				break;
			}
			path = *argv;
		}
		else if (strcmp(*argv, "-s") == 0) {
			want_simplebuf = TRUE;
		}
		else if (strcmp(*argv, "-debug") == 0) {
			want_debug = TRUE;
		}
		else if (strcmp(*argv, "-nocv") == 0) {
			keep_cursor_visible = FALSE;
		}
		else if (strcmp(*argv, "-no-checks") == 0) {
			want_checks = FALSE;
		}
		else if (strcmp(*argv, "-cu") == 0) {
			want_control_u = TRUE;
		}
		else if (strcmp(*argv, "-nl") == 0) {
			nlines = get_value(++argv, "nlines");
		}
		else if (strcmp(*argv, "-nc") == 0) {
			nchars = get_value(++argv, "nchars");
		}
		else if (strcmp(*argv, "-ss") == 0) {
			sel_start = get_value(++argv, "sel-start");
		}
		else if (strcmp(*argv, "-sp") == 0) {
			startpos = get_value(++argv, "start pos");
		}
		else if (strcmp(*argv, "-pp") == 0) {
			point_pos = get_value(++argv, "point pos");
		}
		else if (strcmp(*argv, "-po") == 0) {
			pixel_offset = get_value(++argv, "pixel offset");
		}
		else if (strcmp(*argv, "-ts") == 0) {
			tabspaces = get_value(++argv, "tabspaces");
		}
		else if (strcmp(*argv, "-sc") == 0) {
			sel_count = get_value(++argv, "sel-count");
		}
		else if (strcmp(*argv, "-lc") == 0) {
			loop_count = get_value(++argv, "loop-count");
		}
		else if (strcmp(*argv, "-b") == 0) {
			blocksize = get_value(++argv, "blocksize");
		}
		else if (strcmp(*argv, "-j") == 0) {
			up_delta = down_delta = get_value(++argv, "jump-delta");
		}
		else if (strcmp(*argv, "-jd") == 0) {
			down_delta = get_value(++argv, "down-delta");
		}
		else if (strcmp(*argv, "-ju") == 0) {
			up_delta = get_value(++argv, "up-delta");
		}
		else if (strcmp(*argv, "-2") == 0) {
			want_two_wins = TRUE;
		}
		else if (strcmp(*argv, "-sh") == 0) {
			sline_height = get_value(++argv, "sline npixels");
		}
		else if (strcmp(*argv, "-gs") == 0) {
			grey_on_startup = TRUE;
		}
		else if (strcmp(*argv, "-no-cursor") == 0) {
			want_cursor = FALSE;
		}
		else if (strcmp(*argv, "-f") == 0) {
			font1name = checkarg(++argv, "font name");
		}
		else if (strcmp(*argv, "-f2") == 0) {
			font2name = checkarg(++argv, "font name");
		}
		else if (strcmp(*argv, "-fp") == 0) {
			mfontname = checkarg(++argv, "font pattern");
		}
		else if (strcmp(*argv, "-st") == 0) {
			sample_text_file = checkarg(++argv, "sample text file");
		}
		else if (strcmp(*argv, "-file") == 0) {
			path = checkarg(++argv, "filename");
		}
		else if (strcmp(*argv, "-ro") == 0) {
			ro_text = checkarg(++argv, "read-only text");
		}
		else {
			errf("Unknown flag `%s'", *argv);
			break;
		}
	}

	if ((path == NULL && ro_text == NULL) || *argv != NULL) {
		errf_usage("[-s] [-debug] [-no-checks] [-l] [-b block-size] [-j scroll-delta] [-jd down-delta] [-lc loop-count] [-ju up-delta] [-2] [-sh npixels] [-f font-name] [-f2 font-name] [-fp font-pattern] [-st sample-text-file] [-nl nlines] [-nc nchars] [-file path] [-no-cursor] [-nocv] [-pc] [-ss sel-start] [-sc sel-count] [-gs] [-sp start-pos] [-po pixel-offset] [-ts tabspaces] [-cu] filename");
	}
	
	if ((mesg = wn_open_display()) != NULL) {
		errf("%s", mesg);
		exit(1);
	}

	if (want_control_u) {
		edit_add_keymap_control_entry(edit_get_default_keymap(),
					      'u', edit_f_backward_kill_line);
	}

	edit_add_keymap_entry(edit_get_default_keymap(), 0,
			      EDIT_FUNC_KEY(8), dump_buffer_by_lines);
	edit_add_keymap_entry(edit_get_default_keymap(), 0,
			      EDIT_FUNC_KEY(9), dump_buffer_by_words);

	edit_add_keymap_entry(edit_get_default_keymap(), EDIT_SHIFT,
			      EDIT_FUNC_KEY(8), dump_buffer_backwards_by_lines);
	edit_add_keymap_entry(edit_get_default_keymap(), EDIT_SHIFT,
			      EDIT_FUNC_KEY(9), dump_buffer_backwards_by_words);
	
	fi1 = make_fontinfo(font1name, nlines, nchars);
	fi2 = (font2name != NULL) ? make_fontinfo(font2name, 0, 0) : NULL;

	if (ro_text != NULL) {
		tb = text_create_readonly_buffer(ro_text, strlen(ro_text));
	}
	else if (want_simplebuf) {
		tb = make_simplebuf(path);
	}
	else {
		if (!edit_filebuf_visit_file(alloc_create_pool(), "", path, 0,
					     blocksize, &tb))
			exit(1);
	}

	if (want_debug)
		(*tb->ops->set_debug_flag)(tb->bufdata);

	buffer = edit_create_buffer(tb, fi1, 0);

	if (mfontname != NULL) {
		const char *sample_text;
		
		if (sample_text_file != NULL) {
			sample_text = get_file_contents(sample_text_file);
			if (sample_text == NULL)
				exit(1);
		}
		else {
			sample_text = default_sample_text;
		}
		
		add_mfont_props(buffer, mfontname, fi1, sample_text);
	}
	
	if ((wn = wn_open_stdwin()) == -1) {
		errf("Can't create window");
		exit(1);
	}

	make_subwins(wn, want_two_wins, sline_height, &wn1, &wn2, &wn_sline);

	d1 = add_display(buffer, want_checks, wn1, WN_FG, WN_BG,
			 want_cursor, keep_cursor_visible,
			 startpos, pixel_offset, point_pos, tabspaces);

	if (wn2 != -1)
		d2 = add_display(buffer, want_checks, wn2, WN_BG, WN_FG,
				 want_cursor, keep_cursor_visible, 0, 0, 0, 8);
	else
		d2 = NULL;

	if (wn_sline != -1)
		ds = add_display(buffer, FALSE, wn_sline, WN_FG, WN_BG,
				 want_cursor, keep_cursor_visible,
				 0, 0, point_pos, 0);
	else
		ds = NULL;
	
	if (grey_on_startup) {
		int width, height;

		wn_get_window_size(wn, &width, &height);
		wn_shade_area(wn, 0, 0, width, height, 0x8000, R_OR);
	}
	
	if (sel_count != 0) {
		edit_set_propchange_updates(buffer, FALSE);

		if (fi2 == NULL) {
			pc1 = edit_add_propchange(buffer, sel_start, (char *)NULL,
						  (Edit_fontinfo *)NULL, 1, 1,
						  TRUE);
			pc2 = edit_add_propchange(buffer, sel_start + sel_count,
						  (char *)NULL,
						  (Edit_fontinfo *)NULL, 1, 0,
						  FALSE);
		}
		else {
			pc1 = edit_add_propchange(buffer, sel_start, (char *)NULL,
						  fi2, 0, 0, TRUE);
			pc2 = edit_add_propchange(buffer, sel_start + sel_count,
						  (char *)NULL,
						  fi1, 0, 0, FALSE);
		}
		
		edit_set_propchange_updates(buffer, TRUE);
	}
	else {
		pc1 = pc2 = NULL;
	}

	errf_set_ofunc(show_error_and_beep);
	
	event_loop(d1, d2, ds, wn, wn1, wn2, wn_sline, fi1, fi2, pc1, pc2,
		   up_delta, down_delta, loop_count);

	/*  Give the destroy code a workout.
	 */
	
	edit_close_display(d1);

	if (d2 != NULL)
		edit_close_display(d2);

	if (ds != NULL)
		edit_close_display(ds);

	edit_destroy_buffer(buffer);

	exit(0);
}

static void
dump_buffer_by_lines(display)
Edit_display *display;
{
	dump_buffer(display, '\n');
}

static void
dump_buffer_by_words(display)
Edit_display *display;
{
	dump_buffer(display, ' ');
}

static void
dump_buffer_backwards_by_lines(display)
Edit_display *display;
{
	dump_buffer_backwards(display, '\n');
}

static void
dump_buffer_backwards_by_words(display)
Edit_display *display;
{
	dump_buffer_backwards(display, ' ');
}

static void
dump_buffer(display, endc)
Edit_display *display;
int endc;
{
	Edit_lineinfo *li;
	int lnum;
	const char *line;
	size_t len;
	bool have_eol;

	li = edit_make_lineinfo(edit_get_buffer(display), endc);

	putchar('\n');
	for (lnum = 0; edit_nextline(li, &line, &len, &have_eol); ++lnum)
		printf("%4d: |%.*s|%s\n", lnum, (int)len, line,
		       		          have_eol ? "\\n" : "");

	edit_free_lineinfo(li);
}

static void
dump_buffer_backwards(display, endc)
Edit_display *display;
int endc;
{
	Edit_lineinfo *li;
	const char *line;
	int lnum;
	size_t len;
	bool have_eol;

	li = edit_make_lineinfo(edit_get_buffer(display), endc);
	edit_set_lineinfo_pos(li, edit_get_point(display));

	putchar('\n');
	for (lnum = 0; edit_prevline(li, &line, &len, &have_eol); ++lnum)
		printf("%4d: |%.*s|%s\n", lnum, (int)len, line,
		       			  have_eol ? "\\n" : "");
	
	edit_free_lineinfo(li);
}

static const char *
get_file_contents(path)
const char *path;
{
	int fd, nread;
	struct stat stbuf;
	char *buf;

	fd = -1;
	buf = NULL;
	
	if ((fd = open(path, O_RDONLY)) == -1) {
		failmesg("Can't open", "", path);
	}
	else if (fstat(fd, &stbuf) != 0) {
		failmesg("Can't fstat", "", path);
	}
	else if ((buf = e_malloc((size_t)stbuf.st_size)) == NULL) {
		failmesg("Can't allocate space to read", "", path);
	}
	else if ((nread = read(fd, buf, (size_t)stbuf.st_size)) < 0) {
		failmesg("Error reading", "", path);
	}
	else if (nread != stbuf.st_size) {
		failmesg("Unexpected EOF reading", "", path);
	}
	else if (close(fd) != 0) {
		failmesg("Error closing", "", path);
	}
	else {
		return buf;
	}

	if (fd != -1)
		close(fd);

	if (buf != NULL)
		free(buf);

	return NULL;
}

static void
show_error_and_beep(mesg)
const char *mesg;
{
	fprintf(stderr, "\007%s\n", mesg);
	fflush(stderr);
}

static void
event_loop(d1, d2, ds, wn, wn1, wn2, wn_sline, fi1, fi2, pc1, pc2,
	   up_delta, down_delta, loop_count)
Edit_display *d1, *d2, *ds;
int wn, wn1, wn2, wn_sline;
Edit_fontinfo *fi1, *fi2;
Edit_propchange *pc1, *pc2;
int up_delta, down_delta, loop_count;
{
	Edit_propchange *movepc;
	size_t selpos;

	movepc = NULL;
	selpos = (pc2 != NULL) ? edit_get_propchange_point(pc2) : 0;
	
	for (;;) {
		Edit_display *d;
		event_t event;
		
		wn_next_event(WN_ANY,
			      EV_KEY | EV_BUTTON_UP | EV_BUTTON_DOWN |
			      EV_WINDOW_EXPOSED | EV_WINDOW_RESIZED,
			      &event);

		if (event.ev_type == EV_WINDOW_RESIZED) {
			int sline_height;
			
			wn_close_window(wn1);
			if (wn2 != -1)
				wn_close_window(wn2);
			
			if (wn_sline != -1) {
				int width, height;

				wn_get_window_size(wn_sline, &width, &height);
				sline_height = height + 2 * MARGIN;
				wn_close_window(wn_sline);
			}
			else {
				sline_height = 0;
			}
			
			make_subwins(wn, wn2 != -1, sline_height,
				     &wn1, &wn2, &wn_sline);

			handle_new_window_size(d1, wn1, WN_BG);
			
			if (d2 != NULL)
				handle_new_window_size(d2, wn2, WN_FG);

			if (ds != NULL)
				handle_new_window_size(ds, wn_sline, WN_BG);
		}
		
		if (event.ev_type == EV_WINDOW_EXPOSED) {
			box_and_clear(wn1, WN_BG);
			edit_redraw_display(d1);
			
			if (d2 != NULL) {
				box_and_clear(wn2, WN_FG);
				edit_redraw_display(d2);
			}
			
			if (ds != NULL) {
				box_and_clear(wn_sline, WN_BG);
				edit_redraw_display(ds);
			}
		}

		if ((d = (Edit_display *)wn_get_win_data(event.ev_wn)) == NULL)
			continue;
		
		if (event.ev_type == EV_KEY) {
			edit_handle_wn_key_event(d, &event);

			if (edit_quit_requested(d))
				return;
		}
		
		if (event.ev_type == EV_BUTTON_DOWN) {
			int width, height;

			switch (event.ev_buttons) {
			case B_LEFT:
				do_selection(&event, d, FALSE,
					     (Edit_fontinfo *)NULL,
					     (Edit_fontinfo *)NULL,
					     (Edit_propchange **)NULL,
					     (Edit_propchange **)NULL);
				break;

			case B_SHIFT_MASK | B_LEFT:
				do_selection(&event, d, FALSE,
					     (Edit_fontinfo *)NULL,
					     (Edit_fontinfo *)NULL,
					     &pc1, &pc2);
				break;

			case B_CONTROL_MASK | B_LEFT:
				do_selection(&event, d, FALSE, fi1, fi2,
					     (Edit_propchange **)NULL,
					     (Edit_propchange **)NULL);
				break;
				
			case B_SHIFT_MASK | B_CONTROL_MASK | B_LEFT:
				do_selection(&event, d, TRUE, fi1, fi2,
					     (Edit_propchange **)NULL,
					     (Edit_propchange **)NULL);
				break;
				
			case B_ALT_MASK | B_CONTROL_MASK | B_LEFT:
				remove_selection(&event, d);
				break;


			case B_ALT_MASK | B_SHIFT_MASK | B_LEFT:
				wn_get_window_size(event.ev_wn,
						   &width, &height);
				wn_shade_area(event.ev_wn, 0, 0, width, height,
					      0x8000, R_OR);
				break;


			case B_MIDDLE:
				scroll(&event, d);
				break;
				
			case B_SHIFT_MASK | B_MIDDLE:
				drag(&event, d);
				break;

			case B_ALT_MASK | B_SHIFT_MASK | B_MIDDLE:
				move_pc(d, &selpos, -down_delta,
					pc1, pc2, &movepc);
				break;

			case B_ALT_MASK | B_SHIFT_MASK | B_RIGHT:
				move_pc(d, &selpos, up_delta,
					pc1, pc2, &movepc);
				break;


			case B_SHIFT_MASK | B_CONTROL_MASK | B_MIDDLE:
				loop_scroll(d, down_delta, loop_count);
				break;

			case B_SHIFT_MASK | B_CONTROL_MASK | B_RIGHT:
				loop_scroll(d, -up_delta, loop_count);
				break;

			case B_ALT_MASK | B_CONTROL_MASK | B_MIDDLE:
				edit_scroll_display(d, down_delta);
				break;

			case B_ALT_MASK | B_CONTROL_MASK | B_RIGHT:
				edit_scroll_display(d, -up_delta);
				break;

			case B_RIGHT:
				return;
			
			}
		}
		
	}
}

static void
do_selection(ev, d, restrict_to_sel, fi1, fi2, p_startpc, p_endpc)
event_t *ev;
Edit_display *d;
bool restrict_to_sel;
Edit_fontinfo *fi1, *fi2;
Edit_propchange **p_startpc, **p_endpc;
{
	size_t point;
	Edit_propchange *startpc, *endpc, *movepc;
	Edit_buffer *buffer;
	bool had_cursor;
	int wn;
	bool leave_highlighted;

	wn = ev->ev_wn;
	buffer = edit_get_buffer(d);
	leave_highlighted = fi1 != NULL;

	if (p_startpc == NULL) {
		startpc = endpc = NULL;
	}
	else {
		startpc = *p_startpc;
		endpc = *p_endpc;
		movepc = NULL;
	}

	had_cursor = edit_set_want_cursor(d, FALSE);

        for (;;) {
		if (!edit_pixel_to_point(d, ev->ev_x, ev->ev_y, &point)) {
			;
		}
		else if (startpc == NULL) {
			Edit_flags flagmask;

			flagmask = (fi1 != NULL) ? 0 : 2;
			
			edit_set_propchange_updates(buffer, FALSE);
			
			startpc = edit_add_propchange(buffer, point,
						      (char *)NULL,
						      fi2, flagmask, 2, TRUE);

			endpc = edit_add_propchange(buffer, point, (char *)NULL,
						    fi1, flagmask, 0, FALSE);

			if (leave_highlighted) {
				edit_set_propchange_data(startpc,
							 (char *)endpc);
			}

			movepc = NULL;
			
			edit_set_propchange_updates(buffer, TRUE);
		}
		else {
			edit_update_propchange_pair_pos(buffer, point,
							startpc, endpc,
							&movepc);
		}
		
		wn_next_event(wn, EV_BUTTON_UP | EV_MOUSE_MOVED, ev);

		if (ev->ev_type == EV_BUTTON_UP)
			break;
	}

	if (endpc != NULL) {
		size_t start_point, end_point;
		
		start_point = edit_get_propchange_point(startpc);
		end_point = edit_get_propchange_point(endpc);

		if (start_point != end_point) {
			char *obuf;

			if (restrict_to_sel)
				edit_set_point_limits(d, start_point, end_point);
			obuf = edit_copy_bytes(buffer, start_point, end_point,
					       (char *)NULL);
			wn_set_selection(obuf, (int)(end_point - start_point));
			free(obuf);
		}
		else {
			if (restrict_to_sel)
				edit_set_point_limits(d, 0, EDIT_MAX_POINT);
		}
		
		if (p_startpc != NULL) {
			*p_startpc = startpc;
			*p_endpc = endpc;
		}
		else if (leave_highlighted) {
			if (start_point == end_point) {
				size_t save_point;
				
				save_point = edit_get_point(d);
				edit_insert(buffer, start_point, "#new", 4, TRUE);
				edit_set_point(d, save_point);
			}
		}
		else {
			edit_set_propchange_updates(buffer, FALSE);
			edit_delete_propchange(buffer, startpc);
			edit_delete_propchange(buffer, endpc);
			edit_set_propchange_updates(buffer, TRUE);

			edit_set_mark(d, start_point);
			edit_set_point(d, end_point);
		}
	}

	edit_set_want_cursor(d, had_cursor);
}

static void
remove_selection(ev, d)
event_t *ev;
Edit_display *d;
{
	Edit_buffer *buffer;
	size_t point;
	Edit_propchange *startpc, *endpc;

	buffer = edit_get_buffer(d);
	
	if (!edit_pixel_to_point(d, ev->ev_x, ev->ev_y, &point)) {
		errf("Not pointing at a character");
		return;
	}

	startpc = edit_get_propchange_at_point(buffer, point);
	endpc = (Edit_propchange *)edit_get_propchange_data(startpc);

	if (endpc == NULL) {
		errf("Not pointing at a removable selection");
		return;
	}

	edit_set_propchange_updates(buffer, FALSE);
	edit_delete_propchange(buffer, startpc);
	edit_delete_propchange(buffer, endpc);
	edit_set_propchange_updates(buffer, TRUE);
}

static void
move_pc(d, p_selpos, delta, startpc, endpc, p_movepc)
Edit_display *d;
size_t *p_selpos;
int delta;
Edit_propchange *startpc, *endpc, **p_movepc;
{
	if (startpc == NULL) {
		errf("No selection set");
	}
	else {
		if (delta < 0 && -delta > *p_selpos) {
			errf("Can't move sel to before buffer start");
		}
		else {
			edit_update_propchange_pair_pos(edit_get_buffer(d),
							*p_selpos + delta,
							startpc, endpc,
							p_movepc);
			
			*p_selpos = edit_get_propchange_point(*p_movepc);
		}
	}
}

static void
loop_scroll(d, delta, count)
Edit_display *d;
int delta, count;
{
	int i;
	
	for (i = 0; i < count; ++i) {
		if (edit_scroll_display(d, delta) == 0)
			break;
	}
}
			
static void
scroll(ev, d)
event_t *ev;
Edit_display *d;
{
	static char scrolldist[] = { 0, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4 };
	int wn, height, dist;
	int orig_y, old_inmode;

	wn = ev->ev_wn;
	height = wn_get_height(wn);
	
	orig_y = ev->ev_y;
	old_inmode = wn_inmode(wn, WN_SAMPLE);
		
	for (;;) {
		wn_next_event(wn, EV_BUTTON_UP | EV_MOUSE_MOVED, ev);
		
		if (ev->ev_type == EV_BUTTON_UP)
			break;
		
		if (ev->ev_y < 0 || ev->ev_y >= height)
			continue;
		
		dist = (orig_y - ev->ev_y) / 3;
		
		if (dist >= 0 && dist < sizeof(scrolldist))
			dist = scrolldist[dist];
		else if (dist < 0 && -dist < sizeof(scrolldist))
			dist = -scrolldist[-dist]; 

		wn_updating_off(wn);
		edit_scroll_display(d, dist);
		wn_updating_on(wn);
	}
		
	wn_inmode(wn, old_inmode);
}

static void
drag(ev, d)
event_t *ev;
Edit_display *d;
{
	int wn, last_y;

	wn = ev->ev_wn;
	last_y = ev->ev_y;
		
	for (;;) {
		wn_next_event(wn, EV_BUTTON_UP | EV_MOUSE_MOVED, ev);
		
		if (ev->ev_type == EV_BUTTON_UP)
			break;

		if (ev->ev_y != last_y) {
			wn_updating_off(wn);
			edit_scroll_display(d, last_y - ev->ev_y);
			wn_updating_on(wn);

			last_y = ev->ev_y;
		}
	}
}

static void
handle_new_window_size(d, wn, bg)
Edit_display *d;
int wn, bg;
{
	size_t start_point, lim_point;
	int pixel_offset;

	edit_get_display_info(d, &pixel_offset, &start_point, &lim_point);
	
	box_and_clear(wn, bg);
	
	edit_update_wn_window_size(d, wn);
	edit_display_from(d, pixel_offset, start_point);
	
	wn_set_win_data(wn, (long)d);
}

static void
make_subwins(parwn, want_two_wins, sline_height, p_wn1, p_wn2, p_wn_sline)
int parwn;
bool want_two_wins;
int sline_height;
int *p_wn1, *p_wn2, *p_wn_sline;
{
	int top, width, height;
	
	wn_get_window_size(parwn, &width, &height);

	if (sline_height != 0) {
		top = sline_height + 2 * MARGIN + 1;
		*p_wn_sline = make_subwin(parwn, 20, width - 10, 0, top - 1);
	}
	else {
		top = 0;
		*p_wn_sline = -1;
	}
		
	if (want_two_wins) {
		*p_wn1 = make_subwin(parwn, 0, width, top, height / 2);
		*p_wn2 = make_subwin(parwn, 
				     0, (width * 2) / 3,
				     height / 2 + 1, height);
	}
	else {
		*p_wn1 = make_subwin(parwn, 0, width, top, height);
		*p_wn2 = -1;
	}
}

static int
make_subwin(par, xstart, xlim, ystart, ylim)
int par, xstart, xlim, ystart, ylim;
{
	return wn_create_subwin(par,
				xstart + MARGIN,
				ystart + MARGIN,
				(xlim - xstart) - 2 * MARGIN,
				(ylim - ystart) - 2 * MARGIN,
				WN_INPUT_OUTPUT);
}

static void
box_and_clear(wn, bg)
int wn, bg;
{
	int width, height;
		
	wn_get_window_size(wn, &width, &height);
	
	wn_box_round(wn, -MARGIN, -MARGIN,
		     width + 2 * MARGIN, height + 2 * MARGIN, WN_FG);
	wn_set_area(wn, -MARGIN, -MARGIN,
		    width + 2 * MARGIN, height + 2 * MARGIN, bg);
}
	
static Edit_display *
add_display(buffer, want_checks, wn, fg, bg, want_cursor, keep_cursor_visible,
	    start_point, pixel_offset, point, tabspaces)
Edit_buffer *buffer;
bool want_checks;
int wn;
int fg, bg;
bool want_cursor, keep_cursor_visible;
size_t start_point;
int pixel_offset;
size_t point;
int tabspaces;
{
	Edit_display *d;
	
	box_and_clear(wn, bg);

	d = edit_create_wn_display(wn, fg, bg,
				   want_cursor, keep_cursor_visible);

	if (tabspaces != 0)
		edit_use_folded_lines(d, tabspaces, want_checks, TRUE);
	else
		edit_use_single_line(d);
	       
	edit_update_wn_window_size(d, wn);
	wn_set_win_data(wn, (long)d);
	
	edit_set_buffer(d, buffer);
	edit_display_from(d, pixel_offset, start_point);
	edit_move_point(d, point);

	return d;
}
