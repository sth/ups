/* ao_shlib.c - SunOS 4.X shared library routines */

/*  Copyright 1993 Mark Russell, University of Kent at Canterbury.
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


/* @(#)ao_shlib.c	1.5 24 May 1995 (UKC) */
char ups_st_shlib_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#ifdef AO_TARGET

#if defined(AO_AOUT_SHLIBS)
#include <stdlib.h>
#include <fcntl.h>
#if HAVE_A_OUT_H
#include <a.out.h>
#endif
#if HAVE_LINK_H
#define FREEBSD_AOUT 1
#include <link.h>
#undef FREEBSD_AOUT
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <mtrprog/io.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "st.h"
#include "state.h"
#include "ao_syms.h"
#include "ao_shlib.h"
#include "ao_symload.h"
#include "ao_symread.h"
#include "ao_symscan.h"
#include "ao_target.h"
#include "ao_text.h"
#include "ao_aout.h"
#include "data.h"
#include "obj_bpt.h"        /* BUG: need this for recalculate_bpt_addrs */

static int get_preload_shlib_list PROTO((alloc_pool_t *ap,
					 const char *textpath,
					 shlib_t **p_shlibs,
					 int verbose));
static int get_shlibs_and_global_addrs PROTO((target_t *xp, alloc_pool_t *ap,
					      alloc_pool_t *shlib_ap,
				              symtab_t *st,
					      taddr_t addr_dynamic,
				              shlib_t **p_shlibs));
static const char **add_to_env PROTO((const char *s));
static int load_shlib_symtabs PROTO((shlib_t *shlibs, symtab_t **p_stlist,
				     target_t *xp));
static void free_symtab_cache PROTO((target_t *xp));
static symtab_t *get_symtab_from_cache PROTO((target_t *xp, const char *name));
static void mark_duplicate_shlib_files PROTO((symtab_t *stlist));
static int save_cached_symtab_breakpoints PROTO((target_t *xp));
static void collapse_srcfile_list PROTO((target_t *xp));

#define NEXT_ST(st)	AO_STDATA(st)->st_next

static int Verbose;

int
make_symtab_cache(textpath, p_stcache)
const char *textpath;
symtab_t **p_stcache;
{
	shlib_t *shlibs;
	symtab_t *stlist;
	alloc_pool_t *ap;

	Verbose = getenv("VERBOSE") != NULL;
	if (Verbose && !strcmp(getenv("VERBOSE"), "NOLOAD"))
	  Verbose = 2;

	if (Verbose == 1)
	  fputs("Get preload list...", stderr);
	ap = alloc_create_pool();
	get_preload_shlib_list(ap, textpath, &shlibs, Verbose);

	if (Verbose == 1)
	  fputs("done\n", stderr);
	if (load_shlib_symtabs(shlibs, &stlist, (target_t *)NULL) != 0) {
		alloc_free_pool(ap);
		return -1;
	}

	alloc_free_pool(ap);

	*p_stcache = stlist;
	return 0;
}

bool
aout_next_symtab(xp, st, load_new, p_next_st)
target_t *xp;
symtab_t *st;
bool load_new;
symtab_t **p_next_st;
{
	iproc_t *ip;
	symtab_t *next_st;
	
	ip = GET_IPROC(xp);

	if (st == NULL) {
		next_st = ip->ip_main_symtab;
	}
	else if (st == ip->ip_main_symtab) {
		if (ip->ip_shlib_symtabs != NULL)
			next_st = ip->ip_shlib_symtabs;
		else
			next_st = ip->ip_symtab_cache;
	}
	else {
		next_st = NEXT_ST(st);
	}

	if (next_st == NULL)
		return FALSE;
	
	*p_next_st = next_st;
	return TRUE;
}


/*  Dig out the list of loaded shared libraries and run time linked
 *  global addresses.  This basically involves trundling down linked	
 *  lists in the target via dread().  This must be called after the
 *  target has started and the mapping been done.
 */
static int
get_shlibs_and_global_addrs(xp, ap, shlib_ap, st, addr_dynamic, p_shlibs)
target_t *xp;
alloc_pool_t *ap;
alloc_pool_t *shlib_ap;
symtab_t *st;
taddr_t addr_dynamic;
shlib_t **p_shlibs;
{
#ifdef OS_SUNOS
	struct link_dynamic ldbuf;
	struct link_dynamic_1 ld1buf;
	struct link_map lmbuf, *lm;
	struct ld_debug lddbuf;
	struct rtc_symb rtcbuf, *rtc;
#define LDDBUF_ADDR(ldbuf)	(taddr_t)ldbuf.ldd
#define SDT_ADDR(ldbuf)		(taddr_t)ldbuf.ld_un.ld_1
#define RT_SYM_ADDR(lddbuf)	(lddbuf.ldd_cp)
#define NEXT_RT_SYM(rtcbuf)	(rtcbuf.rtc_next)
#define SYM_PTR(rtcbuf)		(taddr_t)rtcbuf.rtc_sp
#define LD_MAP_ADDR(ld1buf)	(ld1buf.ld_loaded)
#define NEXT_LD_MAP(lmbuf)	(lmbuf.lm_next)
#define SHLIB_NAME(lmbuf)	(taddr_t)lmbuf.lm_name
#define SHLIB_ADDR(lmbuf)	(taddr_t)lmbuf.lm_addr
#else
	struct _dynamic ldbuf;
	struct so_debug lddbuf;
	struct section_dispatch_table ld1buf;
	struct rt_symbol rtcbuf, *rtc;
	struct so_map lmbuf, *lm;
#define LDDBUF_ADDR(ldbuf)	(taddr_t)ldbuf.d_debug
#define SDT_ADDR(ldbuf)		(taddr_t)ldbuf.d_un.d_sdt
#define RT_SYM_ADDR(lddbuf)	(lddbuf.dd_cc)
#define NEXT_RT_SYM(rtcbuf)	(rtcbuf.rt_next)
#define SYM_PTR(rtcbuf)		(taddr_t)rtcbuf.rt_sp
#define LD_MAP_ADDR(ld1buf)	(ld1buf.sdt_loaded)
#define NEXT_LD_MAP(lmbuf)	(lmbuf.som_next)
#define SHLIB_NAME(lmbuf)	(taddr_t)lmbuf.som_path
#define SHLIB_ADDR(lmbuf)	(taddr_t)lmbuf.som_addr
#endif

	struct nlist nm;
#define n_corename	n_un.n_name

	shlib_t *shlist, *sh;
	char buf[256];
	iproc_t *ip;

	if (dread(xp, addr_dynamic, (char *)&ldbuf, sizeof(ldbuf)) != 0)
		return -1;
	
	/*  Get the list of addresses of shared data objects.
	 */
	if (dread(xp, LDDBUF_ADDR(ldbuf), (char *)&lddbuf, sizeof(lddbuf)) != 0)
		return -1;
	for (rtc = RT_SYM_ADDR(lddbuf); rtc != NULL; rtc = NEXT_RT_SYM(rtcbuf)) {
		const char *modname, *fname;
		
		if (dread(xp, (taddr_t)rtc,
					(char *)&rtcbuf, sizeof(rtcbuf)) != 0)
			return -1;
		if (dread(xp, SYM_PTR(rtcbuf),
						(char *)&nm, sizeof(nm)) != 0)
			return -1;
		if (dgets(xp, (taddr_t)nm.n_corename, buf, sizeof(buf)) != 0)
			return -1;

		parse_fname(st->st_apool, buf, '\0', TRUE, &modname, &fname);
		insert_global_addr(st->st_apool, &st->st_addrlist,
				   fname, nm.n_value);
	}

	/*  Now dig out the list of loaded shared libraries.
	 */
	if (dread(xp, SDT_ADDR(ldbuf),
					(char *)&ld1buf, sizeof(ld1buf)) != 0)
		return -1;

	shlist = NULL;
	for (lm = LD_MAP_ADDR(ld1buf); lm != NULL; lm = NEXT_LD_MAP(lmbuf)) {
		if (dread(xp, (taddr_t)lm, (char *)&lmbuf, sizeof(lmbuf)) != 0)
			return -1;
		if (dgets(xp, SHLIB_NAME(lmbuf), buf, sizeof(buf)) != 0)
			return -1;
		/*		if (user_wants_library_loaded(buf))*/
		{
		    if (buf[0] == '.')
		    {
		      symtab_t *st;
		      int i;
		      char name[256];

		      ip = GET_IPROC(xp);
		      st = ip->ip_main_symtab;
		      for(i = strlen(st->st_path) - 1;
			  i >= 0 && st->st_path[i] != '/'; i--);
		      i++;
		      if (i > 0)
		      {
			strcpy(name, buf);
			strncpy(buf, st->st_path, i);
			buf[i] = 0;
			strcat(buf, name);
		      }
		    } /* RGA need list latter in get_unloaded_lib */
		    sh = (shlib_t *)alloc(shlib_ap, sizeof(shlib_t));
		    sh->sh_name = alloc_strdup(shlib_ap, buf);
		    sh->sh_addr = SHLIB_ADDR(lmbuf);
		    sh->sh_next = shlist;
		    shlist = sh;
		  }
	}

	*p_shlibs = shlist;
	return 0;
}


/*  Return a pointer to an array of strings which consists of the
 *  environment plus string s, which should be of the form
 *  "name=value".
 */
static const char **
add_to_env(s)
const char *s;
{
	extern const char **environ;
	const char **sptr, **envp, **src, **dst;

	for (sptr = environ; *sptr != NULL; ++sptr)
		;
	envp = dst = (const char **)e_malloc(sizeof(char *) * (sptr - environ + 2));
	src = environ;
	while (*src != NULL)
		*dst++ = *src++;
	*dst++ = s;
	*dst = NULL;
	return envp;
}

/*  Get the list of shared libraries that the textpath will load
 *  when it is run.  We do this in the same way as ldd(1), by
 *  running the target with LD_TRACE_LOADED_OBJECTS set in its
 *  environment and parsing the output.
 */
static int
get_preload_shlib_list(ap, textpath, p_shlibs, verbose)
alloc_pool_t *ap;
const char *textpath;
shlib_t **p_shlibs;
int verbose;
{
	int pid, wpid;
	int fds[2];
	char buf[256], name[256];
	char *pos;
	const char **envp;
	FILE *fp;
	shlib_t *sh, *shlist;
	taddr_t addr;

	envp = add_to_env("LD_TRACE_LOADED_OBJECTS=1");
	if (pipe(fds) != 0) {
		errf("Pipe failed: %s", get_errno_str());
		free((char *)envp);
		return -1;
	}
	if ((fp = fdopen(fds[0], "r")) == NULL) {
		errf("Fdopen failed: %s", get_errno_str());
		free((char *)envp);
		return -1;
	}
	if ((pid = vfork()) == -1) {
		errf("Vfork failed: %s", get_errno_str());
		fclose(fp);
		close(fds[1]);
		free((char *)envp);
		return -1;
	}
	if (pid == 0) {
		close(fds[0]);
		if (fds[1] != 1)
			dup2(fds[1], 1);
		execle(textpath, textpath, (char *)NULL, envp);
		failmesg("Can't exec", "", textpath);
		_exit(1);
	}

	free((char *)envp);
	close(fds[1]);

	shlist = NULL;
	while (fgets(buf, sizeof(buf), fp) != NULL) {
		/*  We seem to get carriage returns as well as newlines
		 */
		if ((pos = strchr(buf, '\r')) != NULL)
			*pos = '\0';
		if ((pos = strchr(buf, '\n')) != NULL)
			*pos = '\0';
		addr = 0;
#if defined(OS_BSD44)
		if ((sscanf(buf, "%*s => %s (%x)", name, &addr) == 2)
			|| (sscanf(buf, "%*s => %s", name) == 1))
#else
		if (sscanf(buf, "%*s => %s", name) == 1)
#endif
		{
		  if (user_wants_library_loaded(name))
		  {
		    sh = (shlib_t *)alloc(ap, sizeof(shlib_t));
		    /* RGA put out a better message when a library is */
		    /* found  */
		    if (!strcmp(name, "not") && strstr(buf, "not found"))
		    {
		      errf("Warning: %s", buf);
		      errf("\b\t - check shared library paths");
		    }
		    if (name[0] == '.')
		    {
		      int i;

		      for(i = strlen(textpath) - 1;
			  i >= 0 && textpath[i] != '/'; i--);
		      i++;
		      if (i > 0)
		      {
			strcpy(buf, name);
			strncpy(name, textpath, i);
			name[i] = 0;
			strcat(name, buf);
		      }
		    }
		    sh->sh_name = alloc_strdup(ap, name);
		    sh->sh_addr = addr;
		    sh->sh_next = shlist;
		    shlist = sh;
		}
		  else
		  {
		    add_to_noload_list(name, 0);
		    if (verbose)
		      errf("\bNOT loading %s", name);
		  }
		}
	      }
	fclose(fp);
	while ((wpid = wait((int *)NULL)) != pid && wpid != -1)
		;
	*p_shlibs = shlist;
	return 0;
}


/*  Free any symbol tables in the cache.  This is called just
 *  after startup of the target to flush any cached symbol
 *  tables that weren't in fact used.
 */
static void
free_symtab_cache(xp)
target_t *xp;
{
	symtab_t *st, *next;
	
	for (st = GET_IPROC(xp)->ip_symtab_cache; st != NULL; st = next) {
		next = NEXT_ST(st);
		close_symtab(st);
	}
}

static void
collapse_srcfile_list(xp)
target_t *xp;
{
	if (GET_IPROC(xp)->ip_symtab_cache)
	  collapse_srcfiles();
}

static int
save_cached_symtab_breakpoints(xp)
target_t *xp;
{
	symtab_t *st, *next;
	FILE *fp;
	
	if (!GET_IPROC(xp)->ip_symtab_cache)
	  return 0;

	if ((fp = fopen(get_temp_state_filename(), "w")) == NULL)
	{
	  errf("Can't create temp saved breakpoints file `%s'",
	       get_temp_state_filename());
	  return 0;
	}
	for (st = GET_IPROC(xp)->ip_symtab_cache; st != NULL; st = next) {
	  next = NEXT_ST(st);
	  save_matching_breakpoints(st, NULL, (char *)fp);
	}
	return 1;
}

/*  Look up the symbol table for name in the cache.  Remove
 *  it from the cache and return it if found, otherwise
 *  return NULL.
 *
 *  Name is the name of the executable or shared library file
 *  that the symbol table came from.
 */
static symtab_t *
get_symtab_from_cache(xp, name)
target_t *xp;
const char *name;
{
	iproc_t *ip;
	symtab_t *st, *prev;

	ip = GET_IPROC(xp);
	
	prev = NULL;
	for (st = ip->ip_symtab_cache; st != NULL; st = NEXT_ST(st)) {
		if (strcmp(name, st->st_path) == 0) {
			if (prev != NULL)
				NEXT_ST(prev) = NEXT_ST(st);
			else
				ip->ip_symtab_cache = NEXT_ST(st);
			NEXT_ST(st) = NULL;		/* for safety */
			return st;
		}
		prev = st;
	}
	return NULL;
}

/* RGA more stuff to cope with dynamic relinking of Schlumberger ASAP code */
static void  mark_duplicate_shlib_files(stlist)
     symtab_t *stlist;
{
  symtab_t *st_last, *st, *next;
  fil_t *fil, *fil2;

  if (!stlist)
    return;

  for (st_last = st = stlist; st; st = next)
  {
    st_last = st;
    next = NEXT_ST(st);
  }
  for (; stlist; stlist = next)
  {
    for (fil = stlist->st_sfiles; fil; fil = fil->fi_next)
      if (fil->fi_name && fil->fi_name[0] != '/')
	for (fil2 = st_last->st_sfiles; fil2; fil2 = fil2->fi_next)
	{
	  if (fil2->fi_name && fil2->fi_name[0] != '/' &&
	      !strcmp(fil->fi_name, fil2->fi_name))
	  {
	    fil->fi_flags |= FI_DUPLICATE;
	    fil2->fi_flags |= FI_DUPLICATE;
	  }
	}
    next = NEXT_ST(stlist);
  }
}


int
find_duplicate_file(fil, fil1)
     fil_t *fil;
     fil_t **fil1;
{
  symtab_t *st, *next;
  iproc_t *ip;

  ip = GET_IPROC( get_current_target());
  if (fil)
  {
/*    for (st = ip->ip_main_symtab; st; st = next)*/
    for (st = ip->ip_shlib_symtabs; st; st = next)
    {
      for (*fil1 = st->st_sfiles; *fil1; *fil1 = (*fil1)->fi_next)
      {
	if (fil != *fil1 && ((*fil1)->fi_flags & FI_DUPLICATE) &&
	    !strcmp(fil->fi_name, (*fil1)->fi_name))
	  return 1;
      }
      next = NEXT_ST(st);
    }
  }
  return 0;
}

/*  Load the symbol tables for the shared libraries of an object.
 *  This is called just after the target starts, because only
 *  then have the shared libraries been loaded and mapped.
 */
int
load_shared_library_symtabs(xp, adding_libs_only)
target_t *xp;
int adding_libs_only;
{
	static char dynamic[] = "__DYNAMIC";
	const char *sym0_name;
	nlist_t nm;
	taddr_t addr;
	shlib_t *shlibs;
	symtab_t *main_st, *stlist;
	ao_stdata_t *main_ast;
	alloc_pool_t *ap;
	iproc_t *ip;
	int saved = 0;

	ip = GET_IPROC(xp);
	
	main_st = ip->ip_main_symtab;
	main_ast = AO_STDATA(main_st);

	if (ip->ip_main_symtab == NULL || ip->ip_shlib_symtabs != NULL)
		panic("shared lib botch");

	
	if (main_ast->st_dynamic) {
		sym0_name = symstring(main_ast->st_text_symio, 0);
		if (strcmp(sym0_name, dynamic) != 0) {
			errf("First symbol in %s is %s (expected %s)",
					main_st->st_path, sym0_name, dynamic);
			return -1;
		}
		getsym(main_ast->st_text_symio, 0, &nm);
		addr = nm.n_value;

		if (Verbose == 1)
			fputs("Get shlib names and global addresses...", stderr);
		ap = alloc_create_pool();

		ip->ip_shlibs_ap = alloc_create_pool();
		if (get_shlibs_and_global_addrs(xp, ap, ip->ip_shlibs_ap, main_st,
						addr, &shlibs) != 0) {
			alloc_free_pool(ap);
			return -1;
		}
		ip->ip_shlibs = shlibs;
		if (Verbose == 1)
			fputs("done\n", stderr);
		if (load_shlib_symtabs(shlibs, &stlist, xp) != 0) {
			alloc_free_pool(ap);
			return -1;
		}

		alloc_free_pool(ap);
		if (!adding_libs_only)
		{
		  saved = save_cached_symtab_breakpoints(xp);
		  collapse_srcfile_list(xp);
		}
		free_symtab_cache(xp);
		ip->ip_symtab_cache = NULL;
		ip->ip_shlib_symtabs = stlist;
		mark_duplicate_shlib_files(stlist);
		if (!adding_libs_only)
		{
		  if (saved)
		    restore_cached_symtab_breakpoints();
		  recalculate_bpt_addrs(xp);
		}
	}
	return 0;
}

/*  Load the shared libraries desribed in shlibs, and point *p_stlist
 *  at the loaded list.  Return 0 for success, -1 for failure.
 *  In the normal case all the shared libraries will be in the
 *  cache, either from the preload or from a previous run of the
 *  target.
 */
static int
load_shlib_symtabs(shlibs, p_stlist, xp)
shlib_t *shlibs;
symtab_t **p_stlist;
target_t *xp;
{
	symtab_t *stlist, *st;
	shlib_t *sh;
	int shlib_fd;
	bool open_ok;
	taddr_t junk;
	long mtime = 0;
	struct stat stbuf;
	taddr_t entryaddr;

	stlist = NULL;
	for (sh = shlibs; sh != NULL; sh = sh->sh_next) {
	  if (!user_wants_library_loaded((char *)sh->sh_name))
	    continue;
	  open_ok = 0;
	  st = NULL;
	  if (xp != NULL)
	  {
	    st = get_symtab_from_cache(xp, sh->sh_name);
	    if (st != NULL)
	    {
	      open_ok = open_for_reading(sh->sh_name, "shared_library",
					 &shlib_fd);
	      if (open_ok)
		if (fstat(shlib_fd, &stbuf) == 0)
		  mtime = stbuf.st_mtime;
	    }
	    if (st != NULL && st->st_modtime == mtime)
	    {
	      change_base_address(st, sh->sh_addr);
	      close(shlib_fd);
	    }
	  }

	  if (st == NULL ||
	      (st != NULL && st->st_modtime != mtime))
	  {
	    if (get_message_wn() != -1)
	      if (st == NULL)
		errf("\bLoading symtab %s...", sh->sh_name);
	      else
		errf("\bReloading symtab %s...", sh->sh_name);
	    add_to_noload_list(sh->sh_name, 1); /* mark as loaded */
	    if (!open_ok)
	    {
	      open_ok = open_for_reading(sh->sh_name, "shared_library",
					 &shlib_fd);
	      if (open_ok)
		if (fstat(shlib_fd, &stbuf) == 0)
		  mtime = stbuf.st_mtime;
	    }
	    if (get_message_wn() != -1)
	      if (st == NULL)
		errf("\bLoading symtab %s...done", sh->sh_name);
	      else
		errf("\bReloading symtab %s...done", sh->sh_name);
	    if (!open_ok)
	      continue;
	    if (Verbose == 1)
	      fprintf(stderr, "Loading symtab %s...", sh->sh_name);
	    if (!open_textfile(shlib_fd, sh->sh_name, &junk) ||
		!aout_scan_symtab(sh->sh_name, shlib_fd,
				  sh->sh_addr, &st,
				  &entryaddr,
				  (func_t **)NULL))
	    {
	      /*	RGA skip bad libraries */
	      /*	return -1; */
	      if (Verbose == 1)
	        fputs("failed\n", stderr);
	      continue;
	    }
	    if (Verbose == 1)
	      fputs("done\n", stderr);
	    st->st_modtime = mtime;
	  }
	  NEXT_ST(st) = stlist;
	  stlist = st;
	}
	*p_stlist = stlist;
	return 0;
}

/*  Unload shared library symbol tables.  This is called when the
 *  target dies.  We don't free the symbol tables at this point - we
 *  just put them in the cache in anticipation of using them again
 *  when the target is re-run.
 */
void
unload_shared_library_symtabs(xp)
target_t *xp;
{
	iproc_t *ip;

	ip = GET_IPROC(xp);
	
	/* IDE 02/Jul/98 - multiple calls don't loose cache. */
	if (ip->ip_shlib_symtabs == NULL)
	  return;
 
	ip->ip_symtab_cache = ip->ip_shlib_symtabs;
	ip->ip_shlib_symtabs = NULL;
}

void
free_shlib_list(xp)
target_t *xp;
{
	iproc_t *ip;

	ip = GET_IPROC(xp);
	
	alloc_free_pool(ip->ip_shlibs_ap);
}

char *
get_unloaded_lib(addr)
taddr_t addr;
{
  iproc_t *ip;
  shlib_t *sh, *last_sh = NULL;
  
  ip = GET_IPROC(get_current_target());
  for (sh = ip->ip_shlibs; sh != NULL; sh = sh->sh_next)
  {
    if (addr > sh->sh_addr)
      last_sh = sh;
  }
  if (last_sh)
  {
    return (char *)last_sh->sh_name;
  }
  return 0;
}

#endif /* AO_AOUT_SHLIBS */

#endif /* AO_TARGET */
