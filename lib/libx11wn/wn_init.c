/* wn_init.c - initialisation, including opening the main window */

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


/* @(#)wn_init.c	1.53 25 May 1995 (UKC) */
char wn_wn_init_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#if defined(__svr4__) || defined(__SVR4)
#define SVR4 1
#endif

#include <sys/file.h>
#include <sys/param.h>		/* for MAXPATHLEN */
#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>

#include "wn.h"
#include "wn_priv.h"

#if HAVE_X_WINDOWS
#include <X11/Xresource.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#endif

#include "wn_bm.h"
#include "wn_event.h"
#include "wn_cu.h"
#include "wn_rop.h"
#include "wn_misc.h"
#include "wn_win.h"
#include "wn_init.h"
#include "wn_replay.h"
#include "wn_color.h"
#include "sccsdata.h"

static void default_close_func PROTO((int wn));
static void default_close_func(wn) {}

static wn_close_func_t Close_func = default_close_func;

#if !defined(__STDC__) || (defined(WANT_STRERROR) && !defined(strerror))
extern char *sys_errlist[];
extern int sys_nerr;
#define strerror(n) \
    (((n) >= 0 && (n) < sys_nerr) ? sys_errlist[n] : "unknown error")
#endif

static int change_env_var PROTO((const char *s));
static screen_t *new_screen PROTO((int s_screen));

#ifdef X11
static const char *get_xdef PROTO((const char *name, const char *def));
static int create_x_window PROTO((swin_t *w, const char *name,
						const char *geometry));
static void set_x_colors PROTO((screen_t *scr));
static Display *open_display PROTO((const char *displayname, const char **p_errmsg));
static void get_def_color PROTO((const char *defaultname, xpixel_t def_pixel, int def_rgb_val, const char *argspec, XColor *color, Colormap cmap));
static int   /* RCB: Changed from type void to match X spec */
non_fatal_x_error_handler PROTO((Display *displayname, XErrorEvent *err));
static void initialize_atoms PROTO((Window p_win));
#endif

#ifdef SUNVIEW
static int my_error_handler PROTO((int errnum, int winopnum));
#endif

static int add_wm_fd PROTO((int fd, char *errbuf));

#ifdef SUNVIEW
static int (*Old_sun_error_handler)();

static int
my_error_handler(errnum, winopnum)
int errnum, winopnum;
{
	(*Old_sun_error_handler)(errnum, winopnum);
	if (errnum != 0)
		wn__panic("SUNVIEW graphical call failed");
}
#endif /* SUNVIEW */

static bool App_suggests_xpos = FALSE, App_suggests_ypos = FALSE;
static bool App_suggests_width = FALSE, App_suggests_height = FALSE;
static int App_xpos, App_ypos, App_width, App_height;

static int App_width_inc, App_height_inc, App_base_width, App_base_height;
static bool App_suggests_resize_inc = FALSE;

int wn__Batching_enabled = TRUE;

#ifdef X11
/*  Kludge for ICL guide - see _wn_dont_map_windows_on_create() and _wn_map_stdwin().
 */
static int Map_windows_on_create = TRUE;
static short atomic_world_initialized= FALSE;
static Atom
  atom_wm_protocols, atom_wm_delete_window, atom_wm_change_state;

#endif /* X11 */

/*  BUG: need a comment explaining why these aren't all ifdef'ed.
 */
static const char *Stdwin_geometry = NULL;
static bool Want_blanket = FALSE;
static const char Default_appname[] = "wn";
static const char *Appname = Default_appname;
static const char Default_appclass[] = "Wn";
static const char *Appclass = Default_appclass;

static int Want_to_start_as_icon = FALSE;

#ifdef X11
Display *wn__Dpy = NULL;
#endif /* X11 */

static const char *Default_display = NULL;
static screen_t *Head_screen = NULL;

static const char *Bg_color_name = NULL, *Fg_color_name = NULL;
int _wn_Autoraise = FALSE;
int _wn__Use_install = FALSE;
const char *_wn_Icon_geometry = NULL;

static bool Want_reverse_video = FALSE;

#ifdef X11
static int   /* RCB: Changed from type void to match X spec */
non_fatal_x_error_handler(d, err)
     Display *d;
     XErrorEvent *err;

{
  char str[100];

  XGetErrorText(d, (int)err->error_code, str, 80);
  fprintf(stderr, "UPS trapped non fatal X error: %s\n",str);
  return 0;
}

/*
 * Process close on frame menu
 */
void
process_client_message(event, frame, wn)
     XEvent *event;
     Window frame;
     int wn;

{
  XClientMessageEvent *xclient	= &event->xclient;
  Atom protocol;
  
  if( atomic_world_initialized 
     && xclient->message_type == atom_wm_protocols)
  {
    protocol = event->xclient.data.l[0];
    if( protocol == atom_wm_delete_window)
      (*Close_func)(wn);
  }
}

/*
 * Initialize atoms to catch quit on frame menu
 */
static void initialize_atoms(frame)
     Window frame;
     
{
  Atom atomic_interest[1];
  atom_wm_change_state = XInternAtom (wn__Dpy, "WM_CHANGE_STATE", False);
  atom_wm_protocols = XInternAtom(wn__Dpy, "WM_PROTOCOLS", FALSE);
  
  if(atom_wm_protocols != None)
  {
    atomic_world_initialized= TRUE;
    atom_wm_delete_window = XInternAtom(wn__Dpy, "WM_DELETE_WINDOW", FALSE);
    atomic_interest[0] = atom_wm_delete_window;
    XChangeProperty(wn__Dpy, frame, atom_wm_protocols, XA_ATOM,
		    32, PropModeReplace, (unsigned char*)atomic_interest, 1);
  }
}

#endif /* X11 */

void
set_window_error_handler()
{
#ifdef X11
  XSetErrorHandler(non_fatal_x_error_handler);
#endif /* X11 */
}

const char *
wn_version()
{
	return wn__sccsdata[0];
}

void
wn_set_classname(name)
const char *name;
{
	Appclass = name;
}

#ifdef X11
XrmDatabase resourcesDB;
static  void wn_merge_databases()
{
   /*  The code to read the app-defaults database was originally
    *  by Greg McFarlane <gregm@iname.com>
    *
    *  Enhanced by RCB following guidelines in "XLib Programming Manual"
    *     by Adrian Nye, O'Rielly & Associates, 1990
    *     ( Version 11, Release 4, Page 371)
    */
    XrmDatabase applicationDB, aux_applicationDB;
    char path[MAXPATHLEN];
    const char* xuserfilesearchpath = getenv("XUSERFILESEARCHPATH");
    const char* xapplresdir = getenv("XAPPLRESDIR");
    const char* xenvironment = getenv("XENVIRONMENT");
    XrmDatabase serverDB = 0, homeDB = 0;

    /* Get the defaults from the standard location. */

    sprintf(path, "/usr/X11R6/lib/X11/app-defaults/%s", Appclass);
    if (access (path, F_OK) != 0)
	sprintf(path, "/usr/lib/X11/app-defaults/%s", Appclass);
    if (access (path, F_OK) != 0)
	sprintf(path, "/usr/openwin/lib/app-defaults/%s", Appclass);
    applicationDB = XrmGetFileDatabase(path);
    XrmMergeDatabases(applicationDB,&resourcesDB);

    /* Merge in any resource files in $XUSERFILESEARCHPATH
     *  XLib Programming Manual does not say this is supposed to be : separated,
     *  but the name suggests it
     */
    while ( xuserfilesearchpath && *xuserfilesearchpath )
    {
	const char* pathend = strchr(xuserfilesearchpath,':');
	int len;
	if ( pathend)
	{
	    len = pathend-xuserfilesearchpath;
	    strncpy(path, xuserfilesearchpath,len);
	    xuserfilesearchpath = pathend+1;
	}
	else
	{
	    strcpy(path, xuserfilesearchpath);
	    len = strlen(path);
	    xuserfilesearchpath = NULL;
	}

	path[len++] = '/';
	strcpy(path+len,Appclass);
	aux_applicationDB = XrmGetFileDatabase(path);
	if ( aux_applicationDB)
	   XrmMergeDatabases(aux_applicationDB,&resourcesDB);
    }
    /* Merge in any resource files in $XAPPLRESDIR
    ** XLib Programming Manual suggests that at most one of $XUSERFILESEARCHPATH
    ** and $XAPPLRESDIR should be defined.  What to do if both are defined
    ** is not specified.
    */
    if ( xapplresdir && *xapplresdir )
    {
	sprintf(path,"%s/%s", xapplresdir, Appclass);
	aux_applicationDB = XrmGetFileDatabase(path);
	if ( aux_applicationDB)
	   XrmMergeDatabases(aux_applicationDB,&resourcesDB);
    }

    /*  Merge in any resources set by xdrb */
    if ( XResourceManagerString(wn__Dpy))
	serverDB = XrmGetStringDatabase(XResourceManagerString(wn__Dpy));
    else
    {
	const char* home = getenv("HOME");
	if ( home && *home)
	{
	    sprintf(path,"%s/.Xdefaults", home);
	    serverDB = XrmGetFileDatabase(path);
	}
    }
    if ( serverDB)
	XrmMergeDatabases(serverDB,&resourcesDB);

    /* Merge XENVIRONMENT file */

    if ( xenvironment && *xenvironment)
    {
	homeDB = XrmGetFileDatabase(xenvironment);
    } else
    {
	const char* home = getenv("HOME");
	if ( home && *home)
	{
	    int len;
	    sprintf(path,"%s/.Xdefaults-", home);
	    len = strlen(path);
	    if (!gethostname(path+len, sizeof(path)-len-1))
		homeDB = XrmGetFileDatabase(path);
	}
    }
    if ( homeDB)
	XrmMergeDatabases(homeDB,&resourcesDB);
}
#endif

const char *
wn_get_default(name)
const char *name;
{
#ifdef X11
	const char *value = NULL;


	if (wn__Dpy == NULL)
		return NULL;

	if (resourcesDB != NULL) {
		char app_name[512], class_name[512];
		XrmValue xrm_value;
		char *str_type;

		sprintf(app_name, "%s.%s", Appname, name);
		sprintf(class_name, "%s.%s", Appclass, name);

		if (XrmGetResource (resourcesDB, app_name, class_name,
							&str_type, &xrm_value))
			value = xrm_value.addr;
	}

	return value;
#else
#ifdef SUNVIEW
	char option_name[100];

	/*  We want space for "/Appname/name" and a NUL.
	 */
	if (1 + strlen(Appname) + 1 + strlen(name) + 1 >= sizeof(option_name))
		return NULL;
	sprintf(option_name, "/%s/%s", Appname, name);

	if (defaults_exists(option_name, (int *)NULL))
		return defaults_get_string(option_name, "", (int *)NULL);
	return NULL;
#else
	return NULL;
#endif /* !SUNVIEW */
#endif /* !X11 */
}

void
wn_set_default(name, value)
const char* name;
const char* value;
{
#ifdef X11
    char buff[100];
    sprintf(buff,"%s.%s:%s", Appname, name,value);
    XrmPutLineResource(&resourcesDB, buff);
#endif /* X11 */
}

int
wn_use_mono()
{
	return wn__Use_mono;
}

void
wn_use_color_hint(use_color)
int use_color;
{
	wn__Use_mono = !use_color;
}

static const char **Unmunge_args;
static int N_unmunge_args = 0;
static int Unmunge_args_size = 0;

void
_wn_add_to_unmunge_list(args, nargs)
const char **args;
int nargs;
{
	const char *s;
	int i;

	if (Unmunge_args_size == 0) {
		Unmunge_args_size = 16;
		Unmunge_args = (const char **) wn__e_malloc(Unmunge_args_size *
								sizeof(char *));
	}
	if (N_unmunge_args + nargs + 1 >= Unmunge_args_size) {
		Unmunge_args_size *= 2;
		Unmunge_args = (const char **)wn__e_realloc((char *)Unmunge_args,
						Unmunge_args_size * sizeof(char *));
	}
	for (i = 0; i < nargs; ++i) {
		s = strcpy(wn__e_malloc(strlen(args[i]) + 1), args[i]);
		Unmunge_args[N_unmunge_args++] = s;
	}
	Unmunge_args[N_unmunge_args] = NULL;
}

const char **
wn_unmunge_args(args, pos)
const char **args;
int pos;
{
	const char **uargs, **dst;
	int i, nargs, argn;

	for (nargs = 0; args[nargs] != NULL; ++nargs)
		;
	uargs = (const char **)wn__e_malloc((nargs + N_unmunge_args + 1) * sizeof(char *));
	dst = uargs;

	for (argn = 0; argn < pos && args[argn] != NULL; ++argn)
		*dst++ = args[argn];
	for (i = 0; i < N_unmunge_args; ++i)
		*dst++ = Unmunge_args[i];
	for (; args[argn] != NULL; ++argn)
		*dst++ = args[argn];
	*dst++ = NULL;

	return uargs;
}

int
wn_munge_args(argc, argv)
int argc;
const char **argv;
{
	const char **src, **dst, **lim, *s;
	int len;
#ifdef X11
	const char *font;
#endif /* X11 */

	if ((Appname = strrchr(*argv, '/')) != NULL && Appname[1] != '\0')
		Appname++;
	else
		Appname = *argv;
	src = dst = argv;
	lim = argv + argc;
#ifdef X11
	/*  Under X11, XGetDefault requires a display argument, so we must
	 *  fish this out first.  We must also get any -name flag.
	 */
	for (src = dst; src < lim - 1; ++src) {
		if (strlen(*src) > (size_t)1 && strncmp(*src, "-display",
						         strlen(*src)) == 0)
			Default_display = *++src;
		else if (strcmp(*src, "-name") == 0)
			Appname = *++src;
	}
	wn__Dpy = open_display(Default_display, (const char **)NULL);
#endif /* X11 */

#ifdef X11
	/*  Merge the various sources of X resources
	*/
	if (wn__Dpy)
		wn_merge_databases();

	/* XrmGetDefault("Font", "font"); */
	if ((font = wn_get_default("Font")) != NULL) 
		_wn_Sysfont_file = font;
	Stdwin_geometry = wn_get_default("Geometry");
	if (wn_get_default("ReverseVideo") != NULL)
		Want_reverse_video = TRUE;
	if ((s = wn_get_default("ForceMono")) != NULL && strcmp(s, "yes") == 0)
		wn__Use_mono = TRUE;

	if ((s = wn_get_default("WantMenuColors")) != NULL &&
	    strcmp(s, "yes") == 0 && wn__Use_mono == FALSE)
		wn__Use_menu_colors = TRUE;

	if ((s = wn_get_default("WantExtendedTextColors")) != NULL &&
	    strcmp(s, "yes") == 0 && wn__Use_mono == FALSE)
		wn__Use_extended_text_colors = TRUE;

	if ((s = wn_get_default("WantInstalledColormap")) != NULL &&
	    strcmp(s, "yes") == 0 && wn__Use_mono == FALSE)
		_wn__Use_install = TRUE;

	/*  We allow auto raise to be set by an environment variable because
	 *  passing arguments is hard for ICL guide.
	 */
	if (getenv("WN_AUTORAISE") != NULL)
		_wn_Autoraise = TRUE;
	else
		_wn_Autoraise = wn_get_default("AutoRaise") != NULL;

       wn_set_MultiClickTime();	/* RCB */
#endif /* X11 */

	if (getenv("KSTMONO") != NULL)
	{
		wn__Use_mono = TRUE;
		wn__Use_alloc_color = FALSE;
		wn__Use_menu_colors = FALSE;
		wn__Use_extended_text_colors = FALSE;
	}
	if (getenv("KSTCOLOR") != NULL || getenv("KSTCOLOUR") != NULL)
		wn__Use_mono = FALSE;

	src = dst = argv;
	while (src < lim) {
		s = *src;
		len = strlen(s);
		if (strcmp(s, "-blanket") == 0) {
			Want_blanket = TRUE;
			src++;
		}
#ifdef X11
		else if (*s == '#' && s[1] == '+' && isdigit(s[2]))
			_wn_Icon_geometry = *src++ + 1; /* +1 drops the '#' */
		else if (strcmp(s, "-iconic") == 0) {
			Want_to_start_as_icon = TRUE;
			++src;
		}
#ifdef X11
		else if (len > 1 && strncmp(s, "-display", (size_t)len) == 0 &&
								src[1] != NULL) {
			_wn_add_to_unmunge_list(src, 2);
			Default_display = src[1];
			src += 2;
		}
		else if (len > 1 && strncmp(s, "-geometry", (size_t)len) == 0 &&
								src[1] != NULL) {
			Stdwin_geometry = src[1];
			src += 2;
		}
		else if (strcmp(s, "-name") == 0 && src[1] != NULL) {
			Appname = src[1];
			src += 2;
		}
		else if (strcmp(s, "-xdebug") == 0) {
			_Xdebug = TRUE;
			++src;
		}
#endif /* X11 */
		else if (strcmp(s, "-fn") == 0 && src[1] != NULL) {
			_wn_add_to_unmunge_list(src, 1);
			_wn_Sysfont_file = src[1];
			src += 2;
		}
		else if (strcmp(s, "-fg") == 0 && src[1] != NULL) {
			_wn_add_to_unmunge_list(src, 2);
			Fg_color_name = src[1];
			src += 2;
		}
		else if (strcmp(s, "-bg") == 0 && src[1] != NULL) {
			_wn_add_to_unmunge_list(src, 2);
			Bg_color_name = src[1];
			src += 2;
		}
		else if (strcmp(s, "-rv") == 0 || strcmp(s, "-reverse") == 0) {
			Want_reverse_video = !Want_reverse_video;
			++src;
		}
		else if (strcmp(s, "-wn_autoraise") == 0) {
			_wn_Autoraise = TRUE;
			++src;
		}
#endif /* X11 */
		else if (strcmp(s, "-color") == 0 || strcmp(s, "-colour") == 0) {
			_wn_add_to_unmunge_list(src, 1);
			wn__Use_mono = FALSE;
			++src;
		}
		else if (strcmp(s, "-mono") == 0) {
			_wn_add_to_unmunge_list(src, 1);
			wn__Use_mono = TRUE;
			wn__Use_alloc_color = FALSE;
			wn__Use_menu_colors = FALSE;
			wn__Use_extended_text_colors = FALSE;
			++src;
		}
		else if ((strcmp(s, "-install") == 0) && wn__Use_mono == FALSE) {
		  _wn__Use_install = TRUE;
		  ++src;
		}
		else if (strcmp(s, "-noinstall") == 0) {
		  _wn__Use_install = FALSE;
		  ++src;
		}
		else if (strcmp(s, "-wn_debug") == 0) {
			wn__Batching_enabled = FALSE;
			++src;
		}
		else if (strcmp(s, "-wn_record") == 0 && src[1] != NULL) {
			if (wn_set_record_file(src[1]) != 0)
				fprintf(stderr, "wn: can't open %s (%s) so not recording\n",
							src[1], _wn_reason());
			src += 2;
		}
		else if (strcmp(s, "-wn_replay") == 0 && src[1] != NULL) {
			if (wn_set_replay_file(src[1]) != 0)
				fprintf(stderr, "wn: can't open %s (%s) so not replaying\n",
							src[1], _wn_reason());
			src += 2;
		}
		else if (strcmp(s, "-wn_replay_warp") == 0) {
			wn_set_replay_cursor_mode(WN_RP_WARP_MOUSE);
			++src;
		}
		else
			*dst++ = *src++;
	}
	*dst = NULL;
	argc = dst - argv;
#ifdef SUNVIEW
	argc = _wn_grab_sun_args(argc, argv);
#endif /* SUNVIEW */
	return argc;
}

int
wn_open_stdwin()
{
	return wn_create_window(Appname, -1);
}

int
wn_open_scr_stdwin(name,xscreen)
const char* name;
int xscreen;
{
	return wn_create_window(name, xscreen);
}

/*  return sys_errlist[errno] if in range
 */
const char *
_wn_reason()
{
	return (strerror (errno));
} 


screen_t *
_wn_first_screen()
{
	return (Head_screen);
}

int
wn_multi_screen()
{
	screen_t *ptr;

	if (((ptr = _wn_first_screen()) == NULL) || (ptr->s_next == NULL))
		return FALSE;
	return TRUE;
}

static screen_t *
new_screen(s_screen)
int s_screen;
{
	screen_t *scr;
	int i;

	scr = (screen_t *)wn__e_malloc(sizeof(screen_t));
	scr->s_screen = s_screen;
	scr->s_planemask = 0x1;
	scr->s_bg_pixel = 0;
	scr->s_fg_pixel = 1;

#ifdef X11
	/* Must do colours first as used in setting up GC. */
	set_x_colors (scr);
	scr->s_grey_pixmap = 0;
#if WANT_MULTI_SCREEN
	scr->s_npixel = 16;
	scr->s_pixels = (xpixel_t *) wn__e_malloc(scr->s_npixel * sizeof(xpixel_t));
	for (i = 0; i < scr->s_npixel; i++)
		scr->s_pixels[i] = WN_MY_PIXEL;
	scr->s_pixels[0] = scr->s_fg_pixel;
	scr->s_pixels[1] = scr->s_bg_pixel;
#endif
#endif /* X11 */
	scr->s_next = Head_screen;
	Head_screen = scr;
	return (scr);
}

screen_t *
_wn_get_screen(s_screen)
int s_screen;
{
	screen_t *ptr;

	ptr = Head_screen;
	while (ptr != NULL) {
		if (ptr->s_screen == s_screen)
			return (ptr);
		ptr = ptr->s_next;
	}
	return (new_screen (s_screen));
}

#ifdef X11
static void
get_def_color(defaultname, def_pixel, def_rgb_val, argspec, color, cmap)
const char *defaultname;
xpixel_t def_pixel;
int def_rgb_val;
const char *argspec;
XColor *color;
Colormap cmap;
{
	const char *colorspec;

	colorspec = (argspec != NULL) ? argspec : wn_get_default(defaultname);
	if (colorspec == NULL ||
			XParseColor(wn__Dpy, cmap, colorspec, color) == 0) {
		color->pixel = def_pixel;

		/*  Set up a default color in case XQueryColor fails.
		 */
		color->red = color->green = color->blue = def_rgb_val;

		(void) XQueryColor(wn__Dpy, cmap, color);
	}
}
		
/*  Wn was originally written for monochrome displays, and thus offers
 *  facilities like a XOR line mode.
 *
 *  To allow this on color displays, we have to have two contiguous pixel
 *  values, such that WN_FG = WN_BG | _wn_Planemask.
 */
static void
set_x_colors(scr)
screen_t *scr;
{
	XColor fg_color, bg_color;
	Visual *v;
	xpixel_t black_pixel, white_pixel;
	planemask_t plane_mask;
	bool want_mono;
	xpixel_t rw_pixel;

	scr->s_cmap = DefaultColormap(wn__Dpy, scr->s_screen);

	if (_wn__Use_install)
	  scr->s_cmap = XCopyColormapAndFree(wn__Dpy, scr->s_cmap);

	black_pixel = BlackPixel(wn__Dpy, scr->s_screen);
	white_pixel = WhitePixel(wn__Dpy, scr->s_screen);

	get_def_color("Foreground", black_pixel, 0x0000,
					Fg_color_name, &fg_color, scr->s_cmap);
	get_def_color("Background", white_pixel, 0xffff,
					Bg_color_name, &bg_color, scr->s_cmap);

	if (Want_reverse_video) {
		XColor tmp;

		tmp = fg_color;
		fg_color = bg_color;
		bg_color = tmp;
	}

	/*  The -mono flag under X means "do not disturb the color map".
	 *  This is useful for example if you are using ups to debug a
	 *  color allocation problem - you don't want ups to perturb the
	 *  color map by allocating cells.
	 */
	v = DefaultVisual(wn__Dpy, scr->s_screen);
	if (v == NULL)
	{
	      /* RCB: Error handling */
	      fprintf(stderr, "%s: Cannot access screen %d\n", Appname, scr->s_screen);
	      exit(1);
	}
	want_mono = wn__Use_mono || (v->class != PseudoColor &&
						    v->class != DirectColor);

	if (!want_mono && wn__Use_alloc_color == FALSE) {
	  if (XAllocColorCells(wn__Dpy, scr->s_cmap, TRUE,
			       &plane_mask, 1, &rw_pixel, 1) == 0)
	  {
	    if (v->class == PseudoColor)
	    {
	      want_mono = TRUE;
	      wn__Use_alloc_color = TRUE;
	      wn__Use_alloc_color_for_highlight = TRUE;
	    }
	    else
	    {
	      fprintf(stderr, "%s: Using monochrome as there are no free colormap entries\n", Appname);
	      
	      want_mono = wn__Use_mono = TRUE;
	      wn__Use_alloc_color = TRUE;
	      wn__Use_menu_colors = FALSE;
	      wn__Use_extended_text_colors = FALSE;
	    }
	  }
	}

	if (want_mono) {
		/*  The code to handle StaticGray and GrayScale visuals
		 *  sent to me by Jur van der Burg <vdburg@utrtsc.enet.dec.com>.
		 *  I an unable to test it as we have no displays of this
		 *  type locally.
		 *  RGA: added TrueColor clause from Craig Amey 
		 *  <craiga@MetaSW.COM>
		 */
		if (v->class == StaticGray || v->class == GrayScale ||
		    v->class == TrueColor) {
			int i, j;

			j = DisplayPlanes(wn__Dpy, scr->s_screen);
			for (i = 0, plane_mask = 1; i < j; ++i)
				plane_mask <<= 1;
			--plane_mask;
		}
		else {
			plane_mask = 1;
		}

		if (wn__Use_mono ||
			    XAllocColor(wn__Dpy, scr->s_cmap, &fg_color) == 0)
			fg_color.pixel = Want_reverse_video ? white_pixel
							    : black_pixel;

		if (wn__Use_mono ||
			    XAllocColor(wn__Dpy, scr->s_cmap, &bg_color) == 0)
			bg_color.pixel = Want_reverse_video ? black_pixel
							    : white_pixel;

		if (!wn__Use_mono)
		{
		  wn__Use_alloc_color = TRUE;
		  wn__Use_alloc_color_for_highlight = TRUE;
		}
		scr->s_fg_pixel = fg_color.pixel;
		scr->s_bg_pixel = bg_color.pixel;

		/*  Wn has the assumption that logical operations work
		 *  on monochrome displays as if a set pixel was a one
		 *  bit.  We have to frig things to make this work if
		 *  the foreground pixel is zero.
		 *
		 *  BUG: what if the fg and bg pixels differ by more than
		 *       one bit?
		 */
		if (fg_color.pixel < bg_color.pixel)
			_wn_fix_ropfuncs();
	}
	else {
		scr->s_fg_pixel = fg_color.pixel = rw_pixel | plane_mask;
		scr->s_bg_pixel = bg_color.pixel = rw_pixel;
		fg_color.flags = bg_color.flags = DoRed | DoGreen | DoBlue;
		XStoreColor(wn__Dpy, scr->s_cmap, &fg_color);
		XStoreColor(wn__Dpy, scr->s_cmap, &bg_color);
	}

	wn__set_x11_cursor_colors(&fg_color, &bg_color);

	scr->s_planemask = plane_mask;
}

/*  Open the display, and set up an error handler which aborts with a core
 *  dump rather than just exiting.
 */
static Display *
open_display(displayname, p_errmsg)
const char *displayname;
const char **p_errmsg;
{
	static char errbuf[128];
	static Display *display;
	static int need_display = TRUE;
	static char deq[] = "DISPLAY=";
	const char *real_displayname;
	char *buf;
	int display_fd;

	if (need_display) {
		display = XOpenDisplay(displayname);
		if (display == NULL) {
			const char *name;

			if ((name = displayname) == NULL)
				name = getenv("DISPLAY");
			if (name != NULL)
				sprintf(errbuf, "Can't open display %s", name);
			else
				strcpy(errbuf, "Can't open display (DISPLAY environment variable unset)");
		}
		else {
			if (getenv("XSYNCHRONISE") != NULL)
				(void) XSynchronize(display, TRUE);
			real_displayname = DisplayString(display);
			display_fd = ConnectionNumber(display);

			buf = wn__e_malloc(strlen(real_displayname) +
					   sizeof(deq));
			strcpy(buf, deq);
			strcat(buf, real_displayname);
			change_env_var(buf);

			if (add_wm_fd(display_fd, errbuf) != 0) {
				XCloseDisplay(display);
				display = NULL;
			}
		}
		need_display = FALSE;
	}
	if (p_errmsg != NULL)
		*p_errmsg = (display != NULL) ? NULL : errbuf;
	return display;
}
#endif /* X11 */

static int
add_wm_fd(fd, errbuf)
int fd;
char *errbuf;
{
#if defined(F_SETOWN) && !defined(SVR4)
	if (fcntl(fd, F_SETFD, 1) != 0) {
		if (errbuf != NULL) {
			sprintf(errbuf,
				"Can't set close-on-exec flag for display fd %d (%s)\n",
				fd, _wn_reason());
		}
		return -1;
	}

#ifdef FASYNC			/* RGA linux merge */
	if (fcntl(fd, F_SETOWN, getpid()) != 0) {
		if (errbuf != NULL) {
			sprintf(errbuf,
				"Can't set ownership of display fd %d (%s)\n",
			 	fd, _wn_reason());
		}
		return -1;
#endif /* RGA linux merge */
	}
#endif /* !SVR4 */

	_wn_change_wn_fdmask(fd);
	return 0;
}

/*  This function has the same interface as putenv(), but the #ifdefs to
 *  use putenv() only where it exists are more hassle than they are worth.
 */
static int
change_env_var(s)
const char *s;
{
	size_t nlen;
	const char *cptr;
	char **nenv, **eptr;
	extern char **environ;	  /* as specified by POSIX.1 section 2.6*/

	/*  First see if there is an existing 'name=value' with the
	 *  same name as s.
	 */
	for (cptr = s; *cptr != '=' && *cptr != '\0'; cptr++)
		;
	
	if (*cptr == '=' && cptr > s) {
		nlen = cptr - s + 1;
		for (eptr = environ; *eptr != NULL; eptr++) {
			if (strncmp(*eptr, s, nlen) == 0) {
				*eptr = (char *)s;
				return 0;
			}
		}
	}
	
	/*  New name, so must change environ.
	 */
	for (eptr = environ; *eptr != NULL; eptr++)
		;
	
	nenv = (char **)malloc((eptr - environ + 2) * sizeof(char *));
	if (nenv == NULL)
		return -1;
	
	eptr = environ;
	environ = nenv;
	
	while ((*nenv++ = *eptr++) != NULL)
		;
	
	*nenv = (char *)s;
	nenv[1] = NULL;
	
	return 0;
}


#ifdef X11
void
_wn_set_x_gc(bw)
bwin_t* bw;
{
	XGCValues   gcv;
	Window rootwin;
        screen_t *scr = bw->bw_screen;
#define GCMASK	(GCForeground | GCBackground)

	rootwin = RootWindow(wn__Dpy, scr->s_screen);

	gcv.foreground = scr->s_fg_pixel;
	gcv.background = scr->s_fg_pixel;
	bw->bw_gc = XCreateGC(wn__Dpy, rootwin, (unsigned)GCMASK, &gcv);
	bw->bw_gc_noclip = XCreateGC(wn__Dpy, rootwin, (unsigned)GCMASK, &gcv);
}

void
wn_get_X11_info(p_display, p_win)
Display **p_display;
Window *p_win;
{
	_wn_init();
	*p_display = wn__Dpy;
	*p_win = wn_is_open(WN_STDWIN) ? WN_TO_W(WN_STDWIN)->w_win : 0;
}
#endif /* X11 */

const char *
wn_open_display()
{
	const char *errmsg;

	(void) _wn_init();
#ifdef X11
	(void) open_display(Default_display, &errmsg);
#endif
#ifdef SUNVIEW
	if (getenv("WINDOW_PARENT") != NULL)
		errmsg = NULL;
	else
		errmsg = "Can't open window (WINDOW_PARENT environment variable unset)";
#endif
	return errmsg;
}

/*  Do any initialisation prior to creating the main window.
 *  Under X, this means making a connection to the server.
 *
 *  This function is called once before opening any windows.
 *
 *  We return -1 for failure, 0 for success.
 */
int
_wn_init()
{
#ifdef SUNVIEW
	int (*win_errorhandler())();
	int wakeup_fd;
#endif /* SUNVIEW */
	static int done_init = FALSE;
	window_t wn;

	if (done_init)
		return 0;
#ifdef X11
	if ((wn__Dpy = open_display(Default_display, (const char **)NULL)) == NULL)
		return -1;
#if 0
	wn_npixels_hint(1);
	if ((wn = wn_open_stdwin()) == -1) {
	  fprintf(stderr, "Can't open window");
	  return 0;
	}
#endif
	new_screen (DefaultScreen(wn__Dpy));
	if (getenv("XDEBUG") != NULL)
		_Xdebug = TRUE;
#endif /* X11 */
#ifdef SUNVIEW
	Old_sun_error_handler = win_errorhandler(my_error_handler);
	new_screen (0);
	if ((wakeup_fd = wn__setup_sunview_wakeup_pipe()) != -1)
		_wn_set_sunview_wakeup_fd(wakeup_fd);
#endif /* SUNVIEW */
	if (getenv("WN_DEBUG") != NULL)
		wn__Batching_enabled = FALSE;
	_wn_define_machine_bitmap_format();
	done_init = TRUE;
	return 0;
}

/*  Create the standard window, and fill in the machine dependent
 *  parts of w (usually the window identifier).
 *  Also fill in the size and root relative position of the window.
 *
 *  Since this function is completely machine dependent, we have
 *  seperate versions for each system.
 *
 *  Return -1 for failure, 0 for success.
 */

#ifdef SUNVIEW
void
_wn_make_retained_pixrect(pw, width, height)
struct pixwin *pw;
int width, height;
{
	struct pixrect *pr;
	struct mprp_data *mp;
	int depth;
	extern struct pixrectops mem_ops;

	depth = pw->pw_pixrect->pr_depth;
	pr = pw->pw_prretained;
	
	if (depth == 8) {
		if (pr != NULL) {
			mp = (struct mprp_data *) pr->pr_data;
			free((char *)mp->mpr.md_image);
		}
		else {
			pr = (struct pixrect *) wn__e_malloc(sizeof(struct pixrect));
			pr->pr_ops = &mem_ops;
			pr->pr_depth = depth;
			mp = (struct mprp_data *)
					wn__e_malloc(sizeof(struct mprp_data));
			mp->mpr.md_offset.x = 0;
			mp->mpr.md_offset.y = 0;
			mp->mpr.md_primary = 1;
			mp->mpr.md_flags = MP_PLANEMASK;
			mp->planes = 0xff;
			pr->pr_data = (char *) mp;
			pw->pw_prretained = pr;
		}
		pr->pr_size.x = width;
		pr->pr_size.y = height;
		mp->mpr.md_linebytes = width + (width & 1);
		mp->mpr.md_image = (short *) wn__e_malloc(mp->mpr.md_linebytes *
									height);
	}
	else {
		if (pr != NULL)
			pr_destroy(pr);
		pw->pw_prretained = mem_create(width, height, depth);
	}
}

/*  Sunview version of _wn_make_window().
 */
int
_wn_make_window(w, name, is_mainwin)
swin_t *w;
const char *name;
int is_mainwin;
{
	char wname[WIN_NAMESIZE];
	int pfd, wfd;
	int planes;
	struct rect wr;
	extern swin_t *_wn_Fdtab[];
	
	if (is_mainwin && Want_blanket) {
		pfd = (we_getgfxwindow(wname) != 0) ? -1 : open(wname, O_RDWR);
		if (pfd == -1)
			return -1;
		wfd = win_getnewwindow();
		if (wfd == -1 || win_insertblanket(wfd, pfd) == -1)
			return -1;
		close(pfd);
	}
	else
		wfd = _wn_create_sun_window(name);
	(void) fcntl(wfd, F_SETFD, 1);

	if ((w->w_pw = pw_open(wfd)) == NULL) {
		close(wfd);
		return -1;
	}

	if (wn__Use_mono || w->w_pw->pw_pixrect->pr_depth == 1)
		pw_use_fast_monochrome(w->w_pw);
	else
		_wn_init_sunview_colors(w, is_mainwin);

	win_getsize(wfd, &wr);
	w->w_width = wr.r_width;
	w->w_height = wr.r_height;
	w->w_bw->bw_resized = FALSE;

	_wn_make_retained_pixrect(w->w_pw, wr.r_width, wr.r_height);
	pr_putattributes(w->w_pw->pw_prretained, &w->w_bw->bw_planes);

	if (is_mainwin)
		signal(SIGWINCH, _wn_catch_sigwinch);

	w->w_bw->bw_can_refresh = FALSE;
	pw_exposed(w->w_pw);
	w->w_bw->bw_can_refresh = TRUE;
	pw_getattributes(w->w_pw, &w->w_bw->bw_planes);
	w->w_bw->bw_planes &= 0xff;

	_wn_Fdtab[wfd] = w;
	_wn_change_wn_fdmask(wfd);
	return 0;
}
#endif /* SUNVIEW */

#ifdef X11
static const char *
get_xdef(name, def)
const char *name, *def;
{
	char *buf;
	const char *res;

	if (name == NULL || *name == '\0')
		return wn_get_default(def);
	
	buf = wn__e_malloc(strlen(name) + 1 + strlen(def) + 1);
	(void) sprintf(buf, "%s.%s", name, def);
	res = wn_get_default(buf);
	free(buf);
	return res;
}

/*  X windows version of _wn_make_window().
 */
int
_wn_make_window(w, name, is_mainwin)
swin_t *w;
const char *name;
int is_mainwin;
{
	const char *geometry;

	if (is_mainwin && Stdwin_geometry != NULL)
		geometry = Stdwin_geometry;
	else
		geometry = get_xdef(name, "Geometry");

	return create_x_window(w, name, geometry);
}

static int
create_x_window(w, name, geometry)
swin_t *w;
const char *name, *geometry;
{
	XClassHint class_hint;
	XWMHints xwmh;
	Window win;
	const char *bwstr;
	XTextProperty nameprop/*, rga_nameprop*/;
	XSizeHints sh;
	int x, y;
	unsigned flags, width, height, border_width;

	sh.flags = PPosition | PSize;
	sh.x = App_suggests_xpos ? App_xpos : 0;
	sh.y = App_suggests_ypos ? App_ypos : 0;
	sh.width = App_suggests_width ? App_width : 650;
	sh.height = App_suggests_height ? App_height : 550;

	/*  Dubious - we want an application supplied xpos or ypos to place
	 *  the window without prompting for position, and twm won't do this
	 *  unless USPosition is set, so set it.  Must look at ICCCM sometime.
	 */
	if (App_suggests_xpos || App_suggests_ypos)
		sh.flags = (sh.flags & ~PPosition) | USPosition;

	App_suggests_xpos = App_suggests_ypos = FALSE;
	App_suggests_width = App_suggests_height = FALSE;
	
	border_width = 0;
	if ((bwstr = wn_get_default("BorderWidth")) != NULL)
		border_width = atoi(bwstr);
	if (border_width == 0)
		border_width = 2;

	if (geometry != NULL)
		flags = XParseGeometry(geometry, &x, &y, &width, &height);
	else
		flags = 0;

	if ((flags & WidthValue) != 0) {
		sh.width = width;
		sh.flags |= USSize;
	}
	if ((flags & HeightValue) != 0) {
		sh.height = height;
		sh.flags |= USSize;
	}

	if (App_suggests_resize_inc) {
		sh.base_width = App_base_width;
		sh.base_height = App_base_height;
		sh.width_inc = App_width_inc;
		sh.height_inc = App_height_inc;

		sh.flags |= PResizeInc | PBaseSize;

		sh.width = sh.width * sh.width_inc + sh.base_width;
		sh.height = sh.height * sh.height_inc + sh.base_height;
		sh.min_width = sh.width_inc + sh.base_width;
		sh.min_height = sh.height_inc + sh.base_height;

		App_suggests_resize_inc = FALSE;
	}


	if (flags & XValue) {
		if (flags & XNegative) {
			x = DisplayWidth(wn__Dpy, W_SCREEN(w)) + x -
					sh.width - 2 * border_width;
		}
		sh.x = x;
		sh.flags |= USPosition;
	}
	if (flags & YValue) {
		if (flags & YNegative) {
			y = DisplayHeight(wn__Dpy, W_SCREEN(w)) + y -
						sh.height - 2 * border_width;
		}
		sh.y = y;
		sh.flags |= USPosition;
	}

	if (flags & XNegative) {
		sh.win_gravity = (flags & YNegative) ? SouthEastGravity
						     : NorthEastGravity;
	}
	else {
		sh.win_gravity = (flags & YNegative) ? SouthWestGravity
						     : NorthWestGravity;
	}
	sh.flags |= PWinGravity;

	if (_Xdebug) printf ("ups: w             = %p\n", w);
	if (_Xdebug) printf ("ups: W_SCREEN(w)   = %d\n", W_SCREEN(w));
	if (_Xdebug) printf ("ups: RootWindow()  = %ld\n", RootWindow(wn__Dpy, W_SCREEN(w)));
	if (_Xdebug) printf ("ups: W_FG_PIXEL(w) = %6ld, W_BG_PIXEL(w) = %6ld\n", W_FG_PIXEL(w), W_BG_PIXEL(w));

	win = XCreateSimpleWindow(wn__Dpy,
				  RootWindow(wn__Dpy, W_SCREEN(w)),
				  sh.x, sh.y,
				  (unsigned)sh.width, (unsigned)sh.height,
				  border_width,
				  W_FG_PIXEL(w), W_BG_PIXEL(w));
	if (_wn__Use_install)
	  XSetWindowColormap(wn__Dpy, win, _wn_first_screen()->s_cmap);

	xwmh.initial_state = Want_to_start_as_icon ? IconicState : NormalState;
	xwmh.input = True;
	xwmh.flags = InputHint | StateHint;

	if (Appclass == Default_appclass && Appname != Default_appname) {
		char *class;

		class = wn__e_malloc(strlen(Appname) + 1);
		strcpy(class, Appname);
		if (islower(*class))
			*class = toupper(*class);
		Appclass = class;
	}

	class_hint.res_name = (char *)Appname;
	class_hint.res_class = (char *)Appclass;

	if (XStringListToTextProperty((char **)&name, 1, &nameprop) == 0)
		wn__panic("can't make text property for window name");

	/*  Set the standard properties for the window managers.
	 */

	XSetWMProperties(wn__Dpy, win, &nameprop, &nameprop,
				(char **)NULL, 0, &sh, &xwmh, &class_hint);

	XSelectInput(wn__Dpy, win, ExposureMask);

	w->w_win = win;
	initialize_atoms(win);
	return 0;
}

void
_wn_map_X_window(w, iw)
swin_t *w, *iw;
{
	XEvent x_event;
	XWMHints xwmh;

	/*  BUG: The wn window creation stuff for X11 needs rewriting.
	 *       The whole thing is a mess.
	 */

	if (_wn_Icon_geometry != NULL) {
		int res;
		unsigned junk_width, junk_height;

		res = XParseGeometry(_wn_Icon_geometry,
						&xwmh.icon_x, &xwmh.icon_y,
					        &junk_width, &junk_height);
		if ((res & (XValue | YValue)) == (XValue | YValue))
			xwmh.flags |= IconPositionHint;
	}

	if (iw != NULL) {
		xwmh.icon_window = iw->w_win;
		xwmh.flags |= IconWindowHint;

		XSetWMHints(wn__Dpy, w->w_win, &xwmh);
	}


	if (Map_windows_on_create) {
		XMapWindow(wn__Dpy, w->w_win);
		if (!Want_to_start_as_icon) {
			do {
				XNextEvent(wn__Dpy, &x_event);
			} while (x_event.type != Expose);
		}
	}

	wn__update_size(w);
}

void
wn_set_window_hints(wn, title, icontitle, icon)
int wn;
const char* title;
const char* icontitle;
bitmap_t* icon;
{
	swin_t *w;
	Pixmap icon_pixmap = None;
	char title_buff[150];
	char icontitle_buff[150];
	W_CHECK(wn);
	w = WN_TO_W(wn);
	if ( title && !strncmp(title, "%a",2))
	{
	     sprintf(title_buff, "%s%s", Appname,title+2);
	     title = title_buff;
	}
	if ( icontitle && !strncmp(icontitle, "%a",2))
	{
	     sprintf(icontitle_buff, "%s%s", Appname,icontitle+2);
	     icontitle = icontitle_buff;
	}
        if ( icon)
	{
	    icon_pixmap = bm_to_pixmap(icon);
	}
	XSetStandardProperties(wn__Dpy, w->w_win,title, icontitle,icon_pixmap, NULL, 0, NULL);
}
#endif /* X11 */

void
wn_suggest_window_position(x, y)
int x, y;
{
	App_suggests_xpos = x != -1;
	App_xpos = x;
	App_suggests_ypos = y != -1;
	App_ypos = y;
}

void
wn_suggest_resize_inc(base_width, base_height, width_inc, height_inc)
int base_width, base_height, width_inc, height_inc;
{
	App_base_width = base_width;
	App_base_height = base_height;
	App_width_inc = width_inc;
	App_height_inc = height_inc;
	App_suggests_resize_inc = TRUE;
}

void
wn_suggest_window_size(width, height)
int width, height;
{
	App_suggests_width = width > 0;
	App_width = width;
	App_suggests_height = height > 0;
	App_height = height;
}

void
_wn_dont_map_windows_on_create()
{
#ifdef X11
	Map_windows_on_create = FALSE;
#endif
}

void
_wn_map_iclguide_window(wn)
int wn;
{
#ifdef X11
	swin_t *w;
	Window win;
	XSetWindowAttributes xwa;

	W_CHECK(wn);
	w = WN_TO_W(wn);

	xwa.override_redirect = TRUE;
	XChangeWindowAttributes(wn__Dpy, w->w_win,
					(unsigned)CWOverrideRedirect, &xwa);

	win = Want_to_start_as_icon ? w->w_bw->bw_assocw->w_win : w->w_win;
	w->w_bw->bw_is_mapped = !Want_to_start_as_icon;
	w->w_bw->bw_assocw->w_bw->bw_is_mapped = Want_to_start_as_icon;

	XMapWindow(wn__Dpy, win);
	wn__update_size(w);
#endif
}

#if WANT_MULTI_SCREEN
/*
 *  Get pixel value.
 */
xpixel_t
_wn_get_x_pixel (scr, pixel)
screen_t *scr;
xpixel_t pixel;
{
	if (pixel & WN_MY_PIXEL)
		return (scr->s_pixels[pixel & (~WN_MY_PIXEL)]);
	if (_Xdebug) printf("ups: pixel 0x%8lx not mine.\n", pixel);
	return pixel;
}

/*
 *  Return index of free entry in 's_pixels' array, mark "in use".
 *  Grow array if no space.
 */
static int
_wn_find_pix (scr)
screen_t *scr;
{
	int i, rv;
	size_t n;

	for (i = 0; i < scr->s_npixel; i++) {
		if (scr->s_pixels[i] == WN_MY_PIXEL) {
		    scr->s_pixels[i] = 0;
		    return i;
		}
	}

	n = scr->s_npixel * 2;
	scr->s_pixels = (xpixel_t *) realloc((char *)scr->s_pixels, n * sizeof(xpixel_t));
	if (scr->s_pixels == NULL)
		wn__panic("realloc failed in _wn_find_pix");
	for (i = scr->s_npixel; i < n; i++)
		scr->s_pixels[i] = WN_MY_PIXEL;
	rv = scr->s_npixel;
	scr->s_npixel = n;
	scr->s_pixels[rv] = 0;
	return rv;
}
#endif

/* Function to determine if a TrueColor visual is being used */
/* RGA: from Craig Amey <craiga@MetaSW.COM> */

int
wn_is_truecolor(s_screen)
int s_screen;
{
	Visual *v;

	v = DefaultVisual(wn__Dpy, s_screen);
	return (v->class == TrueColor);
}

/* Function used to allocate shared colors. */
/* RGA: from Craig Amey <craiga@MetaSW.COM> */
/* (IDE) If we want multi-screen support we put the color into all of the
 * colormaps.  This is because UPS code caches its colours without much
 * regard to the window, and doing this gets better results.
 */

int
_wn_alloc_x_color(wn, color, colorname)
int wn;
color_t *color;
const char *colorname;
{
    XColor xcolor;
    screen_t *scr;
    swin_t *w;
    int rv = 0, pix;

    W_CHECK(wn);
    w = WN_TO_W(wn);

    xcolor.red = color->co_red;
    xcolor.green = color->co_green;
    xcolor.blue = color->co_blue;

    if (_Xdebug) printf("ups: [%2d] want %04x,%04x,%04x  ", wn, xcolor.red, xcolor.green, xcolor.blue);
#if WANT_MULTI_SCREEN
    pix = _wn_find_pix (_wn_first_screen());
    for (scr = _wn_first_screen(); scr != NULL; scr = scr->s_next) {
    if (XAllocColor(wn__Dpy, scr->s_cmap, &xcolor) == 0) {
#else
    if (XAllocColor(wn__Dpy, W_CMAP(w), &xcolor) == 0) {
#endif
	/* No R/W color cells left in colormap. */
	fprintf(stderr, "ups: Failed to allocate color `%s'\n", colorname);
	if (_Xdebug) printf("got [   ]     ,    ,      ");
#if WANT_MULTI_SCREEN
	scr->s_pixels[pix] = W_FG_PIXEL(w);
	color->co_pixel = ((xpixel_t)pix | WN_MY_PIXEL);
#else
	color->co_pixel = W_FG_PIXEL(w);
#endif

	wn__Use_alloc_color = FALSE;
	wn__Use_alloc_color_for_highlight = TRUE; /* note */
	wn__Use_extended_text_colors = FALSE;
	wn__Use_menu_colors = FALSE;
	rv = 0;
    } else {
	if (_Xdebug) printf("got [%3ld] %04x,%04x,%04x  ", xcolor.pixel, xcolor.red, xcolor.green, xcolor.blue);
#if WANT_MULTI_SCREEN
	scr->s_pixels[pix] = xcolor.pixel;
	color->co_pixel = ((xpixel_t)pix | WN_MY_PIXEL);
#else
	color->co_pixel = xcolor.pixel;
#endif

	wn_use_alloc_color(1);	/* set */
	wn_use_alloc_color_for_highlight(1);	/* set */
	rv = 1;
    }
#if WANT_MULTI_SCREEN
    }
#endif
    if (_Xdebug) printf ("\n");
    return rv;
}

int
wn_alloc_color(wn, color, colorname)
int wn;
color_t *color;
const char *colorname;
{
#ifdef X11
    return (_wn_alloc_x_color(wn, color, colorname));
#else
    color->co_pixel = W_FG_PIXEL(w);
    return 1;
#endif
}

void
wn_set_close_func(func)
wn_close_func_t func;
{
	Close_func = func;
}

