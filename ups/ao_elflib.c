/* ao_elflib.c - ELF shared library support */

/*  Copyright 1995 Mark Russell, University of Kent at Canterbury.
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


/* @(#)ao_elflib.c	1.1 24/5/95 (UKC) */
char ups_ao_elflib_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"
	
#ifdef AO_ELF

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>                /* for MAXPATHLEN */
#include <elf.h>
#define FREEBSD_ELF 1
#include <link.h>
#undef FREEBSD_ELF
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <mtrprog/hash.h>
#include <local/wn.h>
#include <mtrprog/io.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "st.h"
#include "ao_dwarf.h"
#include "ao_syms.h"
#include "ao_core.h"
#include "ao_target.h"
#include "ao_elfpriv.h"
#include "ao_elfread.h"
#include "ao_elflib.h"
#include "ao_elfsym.h"
#include "ao_symscan.h"
#include "breakpoint.h"
#include "data.h"		/* RGA for dread & dgets */
#include "srcwin.h"
#include "state.h"
#include "obj_bpt.h"
#include "va.h"

struct Solib_addr {
	dev_t dev;
	ino_t ino;
        off_t size;		/* RGA added for Clearcase */
#ifdef OS_SUNOS
        long mtv_sec;		/* RGA added for Clearcase */
        long mtv_nsec;		/* RGA added for Clearcase */
#endif
	off_t offset;
	taddr_t vaddr;
	size_t pagesize;
  	char *path;		/* RGA for unloaded libs */
	Solib_addr *next;
};

struct Solib {
	symtab_t *symtab;
	
	const char *soname;
	const char *rpath;
	const char *runpath;
	
	taddr_t min_file_vaddr;
	taddr_t debug_vaddr;
	taddr_t dyn_symtab_vaddr;
	taddr_t dyn_strtab_vaddr;
	taddr_t plt_rel_vaddr;
	int plt_rel_type;
	
	dev_t dev;
	ino_t ino;
        off_t size;		/* RGA added for Clearcase */
#ifdef OS_SUNOS
        long mtv_sec;		/* RGA added for Clearcase */
        long mtv_nsec;		/* RGA added for Clearcase */
        bool mvfs_file;		/* RGA added for Clearcase */
#endif
	
	Libdep *latest_libdep;

	bool is_last_symtab;
	Solib *next;
};

struct Libdep {
	const char *name;
	const char *name_only;
	Solib *so;
	Libdep *par;
	Libdep *deps;
	Libdep *next;
	long mtime;		/* RGA */
	bool emitted_noload_msg; /* RGA so verbose messages not repeated*/
	bool no_reload; /* RGA set when scan_elf_symtab() fails */
	bool wants_load; /* RGA set by user_wants_library_loaded() */
};

static Libdep *next_ld PROTO((Libdep *cur));
static bool
elf_get_dynamic_shlib_info PROTO((alloc_pool_t *ap, Libdep *ld, const char *textpath,
			   taddr_t debug_vaddr,
			   Libdep **last_child));

#ifdef AO_USE_PTRACE /* interp */
static bool
get_preload_shlib_list PROTO((alloc_pool_t *ap, const char *textpath,
			Solib **p_solibs, Solib_addr **p_solib_addrs));
static const char **
add_to_env PROTO((const char *s));
#endif

static unsigned long Main_debug_vaddr = 0;

static void
dump_libdep(Libdep *par, int level)
{
	Libdep *ld;
	
	printf("%*s%p: %s", level * 8, "", par, par->name);

	if (par->so != NULL)
		printf(" so=%s", par->so->symtab->st_path);

	fputc('\n', stdout);

	for (ld = par->deps; ld != NULL; ld = ld->next)
		dump_libdep(ld, level + 1);
}

void
dump_elf_libs(target_t *xp)
{
	Solib *so;
	iproc_t *ip;

	ip = GET_IPROC(xp);

	for (so = ip->ip_solibs; so != NULL; so = so->next) {
		printf("%s: soname=%s latest_libdep=%p\n",
		       so->symtab->st_path,
		       (so->soname != NULL) ? so->soname : "NULL",
		       so->latest_libdep);
	}

	fputc('\n', stdout);
	
	dump_libdep(ip->ip_solibs->latest_libdep, 0);
}

static Libdep *
make_libdep(alloc_pool_t *ap, const char *name, Libdep *par, long mtime)
{
	Libdep *ld;
	const char *c;

	ld = (Libdep *)alloc(ap, sizeof(Libdep));
	ld->name = name;
	if (name)
	{
	  if ((c = strrchr (name, '/')) != NULL)
	    c++;
	  else
	    c = name;
	  ld->name_only = c;
	}
	else
	  ld->name_only = name;
	ld->so = NULL;
	ld->par = par;
	ld->deps = NULL;
	ld->next = NULL;
	ld->mtime = mtime;
	ld->emitted_noload_msg = FALSE;
	ld->no_reload = FALSE;
	ld->wants_load = user_wants_library_loaded((char *)name);
	return ld;
}

static bool
search_lib_path(const char *dirs, const char *seps, const char *name,
		char **p_path)
{
	char **dirvec, **dp;

	if (dirs == NULL)
		return FALSE;
	
	dirvec = ssplit(dirs, seps);

	for (dp = dirvec; *dp != NULL; ++dp) {
		char *path;
		
		path = strf("%s/%s", *dp, name);

		if (access(path, F_OK) == 0) {
			*p_path = path;
			free(dirvec);
			return TRUE;
		}

		free(path);
	}

	free(dirvec);
	return FALSE;
}

#ifdef OS_LINUX

typedef struct Solib_path Solib_path;

struct Solib_path {
	char *name;
	char *path;
	Solib_path *next;
};

static Solib_path *Solib_cache = NULL;

static bool
load_ld_so_cache(void)
{
	int fds[2];
	FILE *fp;
	pid_t pid;
	pid_t wpid;
	char buf[MAXPATHLEN+256];
	Solib_path *cache_tail = NULL;

	if (pipe(fds) != 0) {
		errf("Pipe failed: %s", get_errno_str());
		return FALSE;
	}

	if ((fp = fdopen(fds[0], "r")) == NULL) {
		errf("Fdopen failed: %s", get_errno_str());
		return FALSE;
	}

	if ((pid = vfork()) == -1) {
		errf("Vfork failed: %s", get_errno_str());
		fclose(fp);
		close(fds[1]);
		return FALSE;
	}

	if (pid == 0) {
		close(fds[0]);
		if (fds[1] != 1)
			dup2(fds[1], 1);
		execlp("ldconfig", "ldconfig", "-p", NULL);
            if (errno == ENOENT)
                   execl("/sbin/ldconfig", "ldconfig", "-p", NULL);
		failmesg("Can't exec", "", "ldconfig");
		_exit(1);
	}

	close(fds[1]);

	while (fgets(buf, sizeof(buf), fp) != NULL) {
		char name[256];
		char path[MAXPATHLEN];

		if (sscanf(buf+1, " %s (%*[^)]) => %s ", name, path) == 2) {
			Solib_path *sp = e_malloc(sizeof(Solib_path));

			sp->name = strsave(name);
			sp->path = strsave(path);
			sp->next = NULL;

			if (cache_tail)
				cache_tail->next = sp;
			else
				Solib_cache = sp;

			cache_tail = sp;
		}
	}

	fclose(fp);

	while ((wpid = wait((int *)NULL)) != pid && wpid != -1);
	
	return TRUE;

}

static bool
search_ld_so_cache(const char *name, char **p_path)
{
	Solib_path *sp;

	if (!Solib_cache && !load_ld_so_cache()) {
		errf("Load of ld.so.cache failed");
		return FALSE;
	}

	for (sp = Solib_cache; sp; sp = sp->next) {
		if (strcmp(sp->name, name) == 0) {
			*p_path = strsave(sp->path);
			return TRUE;
		}
	}

	return FALSE;
}

#endif

/*  Apply the rules on page 5-20 of the System V ABI book to find the
 *  path (relative or absolute) of the shared library.
 *
 *  Latest ld.so man page (Linux) says
 *  1) $LD_LIBRARY_PATH
 *  2) /etc/ld.so.cache  (this is binary)
 *  3) /usr/lib:/lib
 */
static char *
get_ld_path(Libdep *ld)
{
	char *path, *ptr;
	
	if (strchr(ld->name, '/') != NULL)
		return strsave(ld->name);

	if (ld->par->so->rpath && !ld->par->so->runpath &&
	    search_lib_path(ld->par->so->rpath, ":", ld->name, &path))
		return path;
	if ((ptr = getenv ("LD_LIBRARY_PATH")) != NULL) {
		if (search_lib_path (ptr, ":;", ld->name, &path))
			return path;
	}
	if (ld->par->so->runpath &&
	    search_lib_path(ld->par->so->runpath, ":", ld->name, &path))
		return path;
#ifdef OS_LINUX
	if (search_ld_so_cache (ld->name, &path))
		return path;
#endif
	if (search_lib_path ("/usr/lib:/lib", ":", ld->name, &path))
		return path;

	return strf("/usr/lib/%s", ld->name);
}

static void
set_base_address(Solib *so, off_t offset, taddr_t vaddr, size_t pagesize)
{
	taddr_t base_address;

	if (pagesize == 0) {
		base_address = vaddr;
	}
	else {
		taddr_t min_mem_vaddr, pagemask;
		
		pagemask = ~(pagesize - 1);
		
		min_mem_vaddr = (vaddr - offset) & pagemask;
		base_address = (min_mem_vaddr & pagemask) -
			(so->min_file_vaddr & pagemask);
	}
		
	change_base_address(so->symtab, base_address);
}

static bool
load_elf_shlib(target_t *xp, Libdep *ld)
{
	iproc_t *ip;
	char *path;
	int fd = 0, reload = 0;
	bool ok;
	Solib *so;
	long mtime = 0;
	struct stat stbuf;
	static int verbose = -1;
	Solib_addr *sa = NULL;

	ip = GET_IPROC(xp);

	if (ld->no_reload == TRUE)
	  return FALSE;		/* RGA change from TRUE to force scan of next lib */
  
	if (verbose == -1)
	{
	  verbose = getenv("VERBOSE") != NULL;
	  if (verbose && !strcmp(getenv("VERBOSE"), "NOLOAD"))
	    verbose = 2;
	}
	path = get_ld_path(ld);

	for (so = ip->ip_solibs; so != NULL; so = so->next) {
	  if (!so->symtab->st_eclipsed &&
	      strcmp(so->symtab->st_path, path) == 0) {
	    ld->so = so;
	    so->latest_libdep = ld;
	    
	    /*  We leave ld->deps as NULL, as they will
	     *  always be visited first under the earlier
	     *  instance of the symbol table.
	     */
	    if (ld->wants_load == TRUE)
	    {
	      ok = open_for_reading(path, "shared library", &fd);
	      if (ok)
	      {
		if (fstat(fd, &stbuf) == 0)
		  mtime = stbuf.st_mtime;
		close(fd);
		if (mtime == so->symtab->st_modtime)
		{
		  free(path);
		  return TRUE;
		}
		else
		{
		  so->symtab->st_eclipsed = 1;
		  reload = 1;
		  break;
		}
	      }
	    }
	    free(path);
	    return TRUE;
	  }
	}

	if (ld->wants_load == FALSE)
	{
	  add_to_noload_list(path, 0);
	  if (verbose && get_message_wn() == -1)
	    if (ld->emitted_noload_msg == FALSE)
	    {
	      ld->emitted_noload_msg = TRUE;
	      fprintf(stderr, "NOT loading symtab %s\n", path);
	    }
	}

	ok = (open_for_reading(path, "shared library", &fd) &&
	      check_elf_file_type(fd, path, ET_DYN, "load symbols from"));
	if (ok)
	{
	  if (fstat(fd, &stbuf) == 0)
	    mtime = stbuf.st_mtime;
	  if (ld->wants_load == FALSE)
	  {
	    /*	    ok = FALSE;*/
	    for (sa = ip->ip_solib_addrs; sa != NULL; sa = sa->next) {
	      if (sa->dev == stbuf.st_dev && sa->ino == stbuf.st_ino)
		break;
	    }

#ifdef OS_SUNOS
	    /* RGA if solib was a mvfs file (e.g. Rational Clearcase), */
	    /* procfs file may point to a nfs equivalent, so use the */
	    /* st_size, mtv_sec and mtv_nsec fields to match */

	    if (sa == NULL)
	      for (sa = ip->ip_solib_addrs; sa != NULL; sa = sa->next) {
		if (stbuf.st_fstype && !strcmp(stbuf.st_fstype, "mvfs") && 
		    sa->size == stbuf.st_size &&
		    sa->mtv_sec == stbuf.st_mtim.tv_sec &&
		    sa->mtv_nsec == stbuf.st_mtim.tv_nsec)
		  break;
	      }
#endif	    
	    if (sa != NULL)
	    {
	      if (sa->path)
		  free(sa->path);
	      sa->path = path;
	    }
	  }
	  else
	  {
	    int wn = get_message_wn();

	    add_to_noload_list(path, 1); /* mark as loaded */
	    if (wn != -1)
	    {
	      errf("\b%s symtab %s...", reload ? "Reloading" :
		   "Loading", path);
	      wn_do_flush();
	    }
	    if (verbose == 1)
	      fprintf(stderr, "%s symtab %s...", reload ? "Reloading" :
		      "Loading", path);
	    ok = scan_elf_symtab(xp->xp_apool, path, fd, ld, (taddr_t *)NULL,
				 (func_t **)NULL, (func_t **)NULL,
				 &ip->ip_solibs, xp->xp_target_updated);
	    if (xp->xp_hit_solib_event == TRUE)
	      xp->xp_new_dynamic_libs_loaded = TRUE;
	    if (wn != -1)
	    {
	      errf("\b%s symtab %s...done", reload ? "Reloading" :
		   "Loading", path);
	      wn_do_flush();
	    }
	    if (verbose == 1)
	      fputs("done\n", stderr);
	  }
	}
	
	if (ld->wants_load == TRUE || sa == NULL)
	  free(path);
	
	if (ok && ld->wants_load) {
	  	ld->no_reload = FALSE;
		so = ld->so;
		so->symtab->st_modtime = stbuf.st_mtime;

		/*  TODO: should pass load address to scan_elf_symtab().
		 */
		for (sa = ip->ip_solib_addrs; sa != NULL; sa = sa->next) {
		  if (sa->dev == so->dev && sa->ino == so->ino)
		    break;
		  }

#ifdef OS_SUNOS
		/* RGA if solib was a mvfs file (e.g. Rational Clearcase), */
		/* procfs file may point to a nfs equivalent, so use the */
		/* st_size, mtv_sec and mtv_nsec fields to match */
		
		if (sa == NULL)
		  for (sa = ip->ip_solib_addrs; sa != NULL; sa = sa->next) {
		    if (sa->size == so->size && sa->mtv_sec == so->mtv_sec &&
			sa->mtv_nsec == so->mtv_nsec)
		      break;
		  }
#endif	    
		if (sa != NULL) {
		  set_base_address(so,
				   sa->offset, sa->vaddr, sa->pagesize);
		}
				
	}
	else
	{
	  ld->no_reload = TRUE;
	  if (fd)
	    close(fd);
	}

	return ok;
}

void
elf_zap_solib_addrs(xp)
target_t *xp;
{
	Solib_addr *sa, *next;
	iproc_t *ip;

	ip = GET_IPROC(xp);
	
	for (sa = ip->ip_solib_addrs; sa != NULL; sa = next) {
		next = sa->next;
		if (sa->path)
		  free(sa->path);
		/* RGA Purify gives a FUM here */
		/* free(sa);*/
	}

	ip->ip_solib_addrs = NULL;
}

static Libdep *
libdep_find_name(new_libdep, name)
     Libdep *new_libdep;
     const char *name;
{
  Libdep *ld;

  for (ld = new_libdep; ld; ld = ld->next)
    if (!strcmp(ld->name_only, name))
      return ld;		/* found */
  return NULL;
}

bool
elf_get_dynamic_solibs(alloc_pool_t *target_ap, const char *path,
		       Solib **p_solibs, bool check_only, bool save_state)
{
    Libdep *ld, first_child, *last_child, *ld1, *ld2;
    int same, same_name, times_differ = 0;

    if (Main_debug_vaddr != 0)
    {
	ld = make_libdep(target_ap, path, (Libdep *)NULL, 0);
	last_child = &first_child;
	elf_get_dynamic_shlib_info (target_ap, ld, path, Main_debug_vaddr,
				    &last_child);
	last_child->next = NULL;
	ld->deps = first_child.next;

	for (same = 1, ld1 = (*p_solibs)->latest_libdep->deps, ld2 = ld->deps;
		 ld1 && ld2; ld1 = ld1->next, ld2 = ld2->next)
	{
	  same_name = !strcmp(ld1->name_only, ld2->name_only);
	  times_differ = same_name && ld1->mtime != ld2->mtime;
	  if (times_differ ||
	      (!same_name  && ld2->wants_load == TRUE) ||
	      (ld1->wants_load == FALSE && ld2->wants_load == TRUE))
	  {
	    /* RGA was noload before, now is not, after init file rescan */
	    /* RGA replaced by a newer lib of diff name, so deactivate old */
	    /* or library has been rebuilt with no name change. */
	    same = 0;
	    break;
	  }
	}
	if (same == 0 || ld2) /* New list is longer */
	{
	  target_t *xp;

	  /* RGA if old libdep is not in the new list, mark the st as eclipsed */
	  for (ld1 = (*p_solibs)->latest_libdep->deps; ld1; ld1 = ld1->next)
	  {
	    ld2 = libdep_find_name(ld->deps, ld1->name_only);
	     
	    if (!ld2 || ld1->mtime != ld2->mtime)
	    {
	      ld1->no_reload = 1;
	      if ( ld1->so )
		ld1->so->symtab->st_eclipsed = 1;
	    }
	  }
	  xp = get_current_target();
	  if (save_state == TRUE)
	    save_target_state(xp, TRUE, TRUE, FALSE);
	  
	  if (check_only == FALSE)
	  {
	    (*p_solibs)->latest_libdep->deps = ld->deps; /* RGA is this right? */
	    load_from_solibs(xp, 0); /* load all libs that user wants */
	  }
	  
	  if (save_state == TRUE)
	    restore_target_state(FALSE, TRUE);
	  return times_differ ? TRUE : FALSE;
	}
    }
    return FALSE;
}

void
elf_update_dynamic_solibs(alloc_pool_t *target_ap, const char *path,
		       Solib **p_solibs, bool load_libs)
{
    Libdep *ld, first_child, *last_child;

    if (Main_debug_vaddr != 0)
    {
	ld = make_libdep(target_ap, path, (Libdep *)NULL, 0);
	last_child = &first_child;
	elf_get_dynamic_shlib_info (target_ap, ld, path, Main_debug_vaddr,
				    &last_child);
	last_child->next = NULL;
	ld->deps = first_child.next;
	(*p_solibs)->latest_libdep->deps = ld->deps; /* RGA is this right? */
	if (load_libs)
	{
	  target_t *xp;

	  xp = get_current_target();
	  load_from_solibs(xp, 0); /* load all libs that user wants */
	}
    }
    return;
}

bool
elf_note_shlib_addr(alloc_pool_t *ap, iproc_t *ip, dev_t dev, ino_t ino, 
		    off_t size, long mtv_sec,  long mtv_nsec, 
		    off_t offset, taddr_t vaddr, size_t pagesize)
{
	Solib *so;
	
	for (so = ip->ip_solibs; so != NULL; so = so->next) {
		if (so->dev == dev && so->ino == ino)
			break;
	}

#ifdef OS_SUNOS
	/* RGA if solib was a mvfs file (e.g. Rational Clearcase), */
	/* procfs file may point to a nfs equivalent, so use the */
	/* st_size, mtv_sec and mtv_nsec fields to match */

	if (so == NULL)
	  for (so = ip->ip_solibs; so != NULL; so = so->next) {
	    if (so->mvfs_file == TRUE && so->size == size &&
		so->mtv_sec == mtv_sec && so->mtv_nsec == mtv_nsec)
	      break;
	}
#endif	    
	if (so != NULL) {
		set_base_address(so, offset, vaddr, pagesize);
	}
	else {
		Solib_addr *sa;
		
		sa = alloc(ap, sizeof(Solib_addr));
		sa->dev = dev;
		sa->ino = ino;
		sa->size = size;
#ifdef OS_SUNOS
		sa->mtv_sec = mtv_sec;
		sa->mtv_nsec = mtv_nsec;
#endif
		sa->offset = offset;
		sa->vaddr = vaddr;
		sa->pagesize = pagesize;
		sa->path = NULL;
		sa->next = ip->ip_solib_addrs;
		ip->ip_solib_addrs = sa;
	}
	
	return TRUE;
}

bool
elf_get_core_shlib_info(alloc_pool_t *ap, iproc_t *ip)
{
	struct r_debug debug;
	Solib *so;
	const char *path;
	taddr_t dbaddr, lmaddr;
	Coredesc *co;
	struct link_map lmap;
	
	so = ip->ip_solibs;
	co = ip->ip_core;
	path = so->symtab->st_path;

	if (so->debug_vaddr == 0) {
		errf("No DT_DEBUG entry in .dynamic section of %s", path);
		return FALSE;
	}

	if (!core_dread(co, so->debug_vaddr, &dbaddr, sizeof(dbaddr)) ||
	    !core_dread(co, dbaddr, (char *)&debug, sizeof(debug))) {
		errf("Can't read r_debug structure from %s", path);
		return FALSE;
	}

	for (lmaddr = (taddr_t)debug.r_map;
	     lmaddr != TADDR_NULL;
	     lmaddr = (taddr_t)lmap.l_next) {
		char libpath[1024]; /* TODO: use MAXPATHLEN or similar */
		struct stat stbuf;

		if (!core_dread(co, lmaddr, &lmap, sizeof(lmap))) {
			errf("Error reading lmap at address %lx in %s",
			     lmaddr, path);
			return FALSE;
		}
		/*  Skip the first entry - it refers to the executable file
		 *  rather than a shared library.
		 */
		if (lmap.l_prev == NULL)
			continue;
#if HAVE_L_PHNUM_F
		if (lmap.l_phnum == 0) {
			errf("Ignoring lmap at address %lx in %s",
			     lmaddr, path);
			continue;
		}
#endif
		if (!core_readstr(co, (taddr_t)lmap.l_name,
				  libpath, sizeof(libpath))) {
			errf("Error reading lmap.l_name at address %lx in %s",
			     lmap.l_name, path);
			return FALSE;
		}

		if (stat(libpath, &stbuf) != 0) {
			errf("Can't stat shared library %s: %s (ignored)",
			     libpath, get_errno_str());
			continue;
		}

		elf_note_shlib_addr(ap, ip, stbuf.st_dev, stbuf.st_ino,
				    0, 0, 0,
				    0, (taddr_t)lmap.l_addr, 0);
		/* Here libpath is a full path, when we first got the
		   shared library details we just got a name. */
		/* Possibly not necessary as library should be findable
		   by 'get_ld_path()' anyway. */
		if (*libpath == '/') {
			char *ptr;
			Libdep *ld;
			ptr = strrchr (libpath, '/') + 1;
			for (ld = next_ld(so->latest_libdep);
			     ld != NULL;
			     ld = next_ld(ld)) {
				if (strcmp (ld->name, ptr) == 0) {
					ld->name = strsave(libpath);
					break;
				}
			}
		}
	}

	return TRUE;
}

static bool
elf_get_dynamic_shlib_info(alloc_pool_t *ap, Libdep *ld, const char *textpath,
			   taddr_t debug_vaddr,
			   Libdep **last_child)
{
	struct r_debug debug;
	const char *path = "DT_DEBUG";
	const char *libpath_copy;
	taddr_t dbaddr, lmaddr;
	struct link_map lmap;
	target_t *xp;
	iproc_t *ip;
	breakpoint_t *bp;
	static bool dl_bp_installed = FALSE;

	xp = get_current_target();

	ip = GET_IPROC(xp);
	if (debug_vaddr == 0) {
		errf("No DT_DEBUG entry in .dynamic section of %s", path);
		return FALSE;
	}

	if (dread(xp, debug_vaddr, &dbaddr, sizeof(taddr_t)) == -1 ||
	    dread(xp, dbaddr, (char *)&debug, sizeof(struct r_debug)) == -1) {
	  errf("Can't read r_debug structure from %s", path);
	  return FALSE;
	}

	if (debug.r_brk && dl_bp_installed == FALSE)
	{
	  bp = xp_add_breakpoint(xp, (taddr_t)debug.r_brk);
	  if (install_breakpoint(bp, xp) != 0)
	    errf("can't install breakpoint in dynamic linker");
	  else
	  {
	    dl_bp_installed = TRUE;
	    set_breakpoint_as_solib_event(bp);
	  }
	}
	  
	for (lmaddr = (taddr_t)debug.r_map;
	     lmaddr != TADDR_NULL;
	     lmaddr = (taddr_t)lmap.l_next) {
		char libpath[MAXPATHLEN], buf[MAXPATHLEN], *path_ptr; 
		struct stat stbuf;

		if (dread(xp, lmaddr, &lmap, sizeof(struct link_map)) == -1)
		{
		  errf("Error reading lmap at address %lx in %s",
		       lmaddr, path);
		  return FALSE;
		}
		if (lmaddr == (taddr_t)lmap.l_next) /* break possible */
						   /* infinite loop */
		  break;

		/*  Skip the first entry - it refers to the executable file
		 *  rather than a shared library.
		 */
		if (lmap.l_prev == NULL || !lmap.l_name)
		  continue;
		
		if (dgets(xp, (taddr_t)lmap.l_name,
			  libpath, MAXPATHLEN-1) == -1)
		{
		  errf("Error reading lmap name %lx in %s",
		       lmap.l_name, path);
		  return FALSE;
		}
		
		if (libpath[0] == '.')
		{
		  int i;
		  char name[MAXPATHLEN];

		  strcpy(buf, libpath);
		  for(i = strlen(textpath) - 1;
		      i >= 0 && textpath[i] != '/'; i--);
		  i++;
		  if (i > 0)
		  {
		    strcpy(name, buf);
		    strncpy(buf, textpath, i);
		    buf[i] = 0;
		    strcat(buf, name);
		  }
		  path_ptr = buf;
		}
		else
		  path_ptr = libpath;

		if (stat(path_ptr, &stbuf) != 0) {
		  errf("Can't stat shared library %s: %s (ignored)",
		       path_ptr, get_errno_str());
		  continue;
		}

		libpath_copy = alloc_strdup(ap, path_ptr);
		(*last_child)->next = make_libdep(ap, libpath_copy, ld,
						  stbuf.st_mtime);
		(*last_child) = (*last_child)->next;

#ifdef AO_USE_PTRACE
		elf_note_shlib_addr(ap, ip,
				    stbuf.st_dev, stbuf.st_ino,
				    stbuf.st_size,
				    0, 0,
				    0, (taddr_t)lmap.l_addr, 0);
#endif
	      }
	return TRUE;
}

bool
scan_main_elf_symtab(alloc_pool_t *target_ap, const char *path, int fd,
		     long modtime, Solib **p_solibs, Solib_addr **p_solib_addrs,
		     taddr_t *p_entryaddr, struct func_s **p_mainfunc,
		     struct func_s **p_initfunc, bool target_updated)
{
	Libdep *ld;
	bool ok;

	Main_debug_vaddr = 0;	/* RGA for reattach with rescan */
	*p_solibs = NULL;
	ld = make_libdep(target_ap, path, (Libdep *)NULL, 0);
	if (target_updated == TRUE)
	{
	  errf("\bScanning symbols of `%s'...", path);
	  wn_do_flush();
	}
	ok = scan_elf_symtab(target_ap, path, fd, ld, p_entryaddr,
			     p_mainfunc, p_initfunc, p_solibs,
			     target_updated);
	if (target_updated == TRUE)
	{
	  errf("\bScanning symbols of `%s'... done", path);
	  wn_do_flush();
	}

	if (ld->so)
	{
	  ld->so->symtab->st_modtime = modtime;
#ifdef AO_USE_PTRACE /* interp */
	  if (!get_preload_shlib_list (target_ap, path, p_solibs, p_solib_addrs))
	    return FALSE;
#endif
	  Main_debug_vaddr = ld->so->debug_vaddr;
	}
	return ok;
}

static Libdep *
find_first_descendent(Libdep *par, int level)
{
	Libdep *ld;

	if (par == NULL)
		return NULL;
	
	if (level == 0)
		return par->deps;

	for (ld = par->deps; ld != NULL; ld = ld->next) {
		Libdep *res;

		res = find_first_descendent(ld, level - 1);

		if (res != NULL)
			return res;
	}

	return NULL;
}

static Libdep *
next_ld(Libdep *cur)
{
	Libdep *ld;
	int level;

	if (cur->next != NULL)
		return cur->next;

	if (cur->par == NULL)
		return cur->deps;

	level = 0;
	for (ld = cur->par; ld != NULL; ld = ld->par) {
		Libdep *res;
		
		res = find_first_descendent(ld->next, level);

		if (res != NULL)
			return res;
		
		++level;
	}

	return find_first_descendent(cur->par, 1);
}

bool
elf_next_symtab(target_t *xp, symtab_t *st, bool load_new, symtab_t **p_next_st)
{
	Solib *so, *so1;
	Libdep *ld;
	
	if (st == NULL) 
	  for (so = GET_IPROC(xp)->ip_solibs; so != NULL; so = so->next) {
	    if (!so->symtab->st_eclipsed) {	 
	      *p_next_st = so->symtab;
	      return TRUE;
	    }
	  }
	
	so = AO_STDATA(st)->st_solib;

	/* RGA 07/18/00 add a check so the extensive lookup is done
	   only when the dynamic linker breakpoint has been set.

	   RGA 01/30/01 Older library versions are marked
	   with st_eclipsed, so skip those. */

	if (xp->xp_hit_solib_event == FALSE)
	{
	  for (so1 = so->next; so1 != NULL; so1 = so1->next) {
	    if (!so1->symtab->st_eclipsed) {	 
	      *p_next_st = so1->symtab;
	      return TRUE;
	    }
	  }
	  
	  if (!load_new || so->is_last_symtab) 
	    return FALSE;
	}
	for (ld = next_ld(so->latest_libdep); ld != NULL; ld = next_ld(ld)) {
		if (!load_elf_shlib(xp, ld))
			continue;

		if (so->next != NULL) {
			/*  Loaded a new symbol table.
			 */
			*p_next_st = so->next->symtab;
			if (!(*p_next_st)->st_eclipsed)
			  return TRUE;
		}
	}
	while (so->next != NULL) {
	  if (!so->next->symtab->st_eclipsed) {
	    *p_next_st = so->next->symtab;
	    return TRUE;
	  }
	  else
	    so = so->next;
	}
	
	so->is_last_symtab = TRUE;
	return FALSE;
}

static bool
get_elf_shlib_info(alloc_pool_t *ap, Elfinfo *el,
		   const char **p_soname, const char **p_rpath,
		   const char **p_runpath, taddr_t *p_debug_vaddr, 
		   taddr_t *p_dyn_symtab_vaddr, taddr_t *p_dyn_strtab_vaddr, 
		   taddr_t *p_plt_rel_vaddr, int *p_plt_rel_type)
{
	Elf32_Shdr *dynsh, *strsh;
	Elf32_Dyn *dyntab;
	int i, count;
	char *strings;
	Libdep first_child, *last_child;

	if (!elf_find_section(el, ".dynamic", "shared library information",
			      &dynsh))
		return FALSE;
	
	if (!check_elf_entsize(dynsh, ".dynamic", sizeof(Elf32_Dyn)))
		return FALSE;
	count = dynsh->sh_size / sizeof(Elf32_Dyn);
	
	if (dynsh->sh_link >= el->num_sections) {
		errf("String table index (%ld) for .dynamic section "
		     "not in range 0..%d in %s %s",
		     dynsh->sh_link, el->num_sections - 1, el->what, el->path);
		return FALSE;
	}
	
	strsh = &el->sections[dynsh->sh_link];
	
	dyntab = e_malloc(dynsh->sh_size);
	strings = e_malloc(strsh->sh_size);

	if (!read_elf_section(el, ".dynamic section", dynsh, dyntab) ||
	    !read_elf_section(el, ".dynamic section strings", strsh, strings)) {
		free(dyntab);
		free(strings);
		return FALSE;
	}

	last_child = &first_child;

	*p_soname = *p_rpath = *p_runpath = NULL;
	*p_debug_vaddr = 0;
	*p_dyn_symtab_vaddr = *p_dyn_strtab_vaddr = 0;
	*p_plt_rel_vaddr = *p_plt_rel_type = 0;
	
	for (i = 0; i < count; ++i) {
		Elf32_Dyn *d;
		const char **p_str, *depname;
		off_t offset;

		d = &dyntab[i];
		
		switch (d->d_tag) {
		case DT_SONAME:
			p_str = p_soname;
			break;
		case DT_RPATH:
			p_str = p_rpath;
			break;
#ifdef DT_RUNPATH
		case DT_RUNPATH:
			p_str = p_runpath;
			break;
#endif
		case DT_NEEDED:
			p_str = &depname;
			break;
		case DT_DEBUG:
			*p_debug_vaddr = dynsh->sh_addr +
				((char *)&d->d_un.d_ptr - (char *)dyntab);
			continue;
		case DT_SYMTAB:
			*p_dyn_symtab_vaddr = d->d_un.d_ptr;
			continue;
		case DT_STRTAB:
			*p_dyn_strtab_vaddr = d->d_un.d_ptr;
			continue;
		case DT_JMPREL:
			*p_plt_rel_vaddr = d->d_un.d_ptr;
			continue;
		case DT_PLTREL:
			*p_plt_rel_type = d->d_un.d_val;
			continue;
		default:
			continue;
		}

		offset = d->d_un.d_val;

		if (offset >= strsh->sh_size) {
			errf("\bString offset for .dynamic entry %d in %s %s "
			     "not in range 0..%ld - ignored",
			     i, el->what, el->path, strsh->sh_size - 1);
			continue;
		}
		
		*p_str = alloc_strdup(ap, &strings[offset]);

		if (d->d_tag == DT_NEEDED) {
			last_child->next = make_libdep(ap, depname, el->libdep,
						       0);
			last_child = last_child->next;
		}
	}

	free(dyntab);
	free(strings);

	last_child->next = NULL;
	el->libdep->deps = first_child.next;
	
	return i == count;
}

bool
add_solib_entry(alloc_pool_t *ap, symtab_t *st, func_t *flist, Elfinfo *el, Solib **p_solibs)
{
	Solib *so;
	const char *soname, *rpath, *runpath;
	ao_stdata_t *ast;
	taddr_t debug_vaddr;
	taddr_t dyn_symtab_vaddr;
	taddr_t dyn_strtab_vaddr;
	taddr_t plt_rel_vaddr;
	int plt_rel_type;
	struct stat stbuf;
	symtab_t *oldst;

	ast = AO_STDATA(st);

	if (fstat(ast->st_textfd, &stbuf) != 0) {
		errf("Can't fstat %s (fd=%d): %s",
		     st->st_path, ast->st_textfd, get_errno_str());
		return FALSE;
	}

	st->st_modtime = stbuf.st_mtime;

	if (!get_elf_shlib_info(ap, el, &soname, &rpath, &runpath, &debug_vaddr, 
				&dyn_symtab_vaddr, &dyn_strtab_vaddr,
				&plt_rel_vaddr, &plt_rel_type))
	{
	  /* RGA completely statically linked targets used to return
	     FALSE at this point, terminiating the debug seesion.
	     Changed to continue with null data.
	     */
	  soname = 0;
	  rpath = "";
	  debug_vaddr = 0;
	}

	for (so = *p_solibs; so; so = so->next)
		if (strcmp(so->symtab->st_path, st->st_path) == 0)
			break;

	if (so) {
		oldst = so->symtab;
	}
	else {
		Solib *so2, *last;
		
	so = alloc(ap, sizeof(Solib));

		so->next = NULL;
		
		last = NULL;
		for (so2 = *p_solibs; so2 != NULL; so2 = so2->next)
			last = so2;

		if (last != NULL)
			last->next = so;
		else
			*p_solibs = so;

		oldst = NULL;
	}
	
	so->symtab = st;
	so->soname = soname;
	so->rpath = rpath;
	so->runpath = runpath;
	so->min_file_vaddr = el->min_file_vaddr;
	so->debug_vaddr = debug_vaddr;
	so->dyn_symtab_vaddr = dyn_symtab_vaddr;
	so->dyn_strtab_vaddr = dyn_strtab_vaddr;
	so->plt_rel_vaddr = plt_rel_vaddr;
	so->plt_rel_type = plt_rel_type;
	so->dev = stbuf.st_dev;
	so->ino = stbuf.st_ino;
	so->size = stbuf.st_size;
#ifdef OS_SUNOS
	so->mtv_sec = stbuf.st_mtim.tv_sec;
	so->mtv_nsec = stbuf.st_mtim.tv_nsec;
	if (stbuf.st_fstype && !strcmp(stbuf.st_fstype, "mvfs"))
	  so->mvfs_file = TRUE;
	else
	  so->mvfs_file = FALSE;
#endif
	so->latest_libdep = el->libdep;
	so->is_last_symtab = FALSE;

	ast->st_solib = so;

	el->libdep->so = so;
	el->libdep->mtime = stbuf.st_mtime;

	if (oldst) {
		target_t *xp;
		Srcwin *sw;

		update_matching_bpts(oldst, st);
		
		update_var_state(oldst, st);

		if ((xp = get_current_target()) != NULL) {
			func_t *oldf = xp_get_mainfunc(xp);

			if (oldf && oldf->fu_symtab == oldst) {
				func_t *newf;

				for (newf = flist; newf; newf = newf->fu_next)
					if (strcmp(oldf->fu_name, newf->fu_name) == 0)
						break;
			
				xp_set_mainfunc(xp, newf);
			}
		}

		srcwin_set_stopline(NULL, 0);

		if ((sw = get_current_srcwin()) != NULL) {
			Srcinfo si;
		   
			srcwin_get_info(sw, 0, srcwin_get_window_height(sw) / 2, &si);

			if (si.fil && si.fil->fi_symtab == oldst) {
				fil_t *fil;
			   
				if ((fil = name_and_st_to_fil(si.fil->fi_name, st)) != NULL)
					srcwin_show(sw, fil, si.fil_lnum - 1);
			}
		}
	   
		close_symtab(oldst);
	}

	return TRUE;
}

void
unload_shared_library_symtabs(xp)
target_t *xp;
{
    /* Just to satisfy a link reference. */
}

void
elf_rescan_dynamic_solibs(xp, check_only)
target_t *xp;
bool check_only;
{
#ifdef AO_ELF
  iproc_t *ip;
  
  ip = GET_IPROC(xp);
  if (ip->ip_pid)
    xp->xp_new_dynamic_libs = 
      elf_get_dynamic_solibs(xp->xp_apool, xp->xp_textpath,
			     &ip->ip_solibs, check_only, TRUE); /* save state */
#endif
}

#ifdef AO_USE_PTRACE /* interp */
/*  Get the list of shared libraries that the textpath will load
 *  when it is run.  We do this in the same way as ldd(1), by
 *  running the target with LD_TRACE_LOADED_OBJECTS set in its
 *  environment and parsing the output.
 *
 *  (IDE) taken out of ao_shlib.c - FreeBSD/Linux sometimes need the
 *  correct addresses of functions *before* the target has been started,
 *  e.g. interpreted code at breakpoints calling library functions.
 *  All we do here is create the 'Solib_addr' structures for later code
 *  to pick up.
 */
static bool
get_preload_shlib_list(ap, textpath, p_solibs, p_solib_addrs)
alloc_pool_t *ap;
const char *textpath;
Solib **p_solibs;
Solib_addr **p_solib_addrs;
{
    int pid, wpid;
    int fds[2];
    char buf[MAXPATHLEN+256], name[MAXPATHLEN];
    char *pos;
    const char **envp;
    FILE *fp;
    taddr_t addr;
    struct stat stbuf;
    Solib_addr *sa;
/*
    char *libpath_copy;
    Libdep first_child, *last_child;
    last_child = &first_child;
*/

    envp = add_to_env("LD_TRACE_LOADED_OBJECTS=1");
    if (pipe(fds) != 0) {
	    errf("Pipe failed: %s", get_errno_str());
	    free((char *)envp);
	    return FALSE;
    }
    if ((fp = fdopen(fds[0], "r")) == NULL) {
	    errf("Fdopen failed: %s", get_errno_str());
	    free((char *)envp);
	    return FALSE;
    }
    if ((pid = vfork()) == -1) {
	    errf("Vfork failed: %s", get_errno_str());
	    fclose(fp);
	    close(fds[1]);
	    free((char *)envp);
	    return FALSE;
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

    while (fgets(buf, sizeof(buf), fp) != NULL)
    {
	/*  We seem to get carriage returns as well as newlines
	 */
	if ((pos = strchr(buf, '\r')) != NULL)
		*pos = '\0';
	if ((pos = strchr(buf, '\n')) != NULL)
		*pos = '\0';
	pos = buf;
	addr = 0;
	if ((sscanf(buf, "%*s => %s (%x)", name, &addr) == 2)
		|| (sscanf(buf, "%*s => %s", name) == 1))
	{
	    if (stat(name, &stbuf) != 0)
	    {
		errf("Can't stat shared library %s: %s (ignored)",
		     name, get_errno_str());
		continue;
	    }
/*
	    libpath_copy = alloc_strdup(ap, name);
	    last_child->next = make_libdep(ap, libpath_copy, (Libdep *)NULL,
					      stbuf.st_mtime);
	    last_child = last_child->next;
*/
	    sa = e_malloc(sizeof(Solib_addr));
	    sa->dev = stbuf.st_dev;
	    sa->ino = stbuf.st_ino;
	    sa->size = stbuf.st_size;
#ifdef OS_SUNOS
	    sa->mtv_sec = stbuf.mtv_sec;
	    sa->mtv_nsec = stbuf.mtv_nsec;
#endif
	    sa->offset = 0;
	    sa->vaddr = addr;
	    sa->pagesize = 0;
	    sa->path = NULL;
	    sa->next = *p_solib_addrs;
	    *p_solib_addrs = sa;
	}
    }
    fclose(fp);
    while ((wpid = wait((int *)NULL)) != pid && wpid != -1)
	    ;
/*
    last_child->next = NULL;
*/
    return TRUE;
}

/*  Return a pointer to an array of strings which consists of the
 *  environment plus string s, which should be of the form
 *  "name=value".
 */
static const char **
add_to_env(s)
const char *s;
{
#if !HAVE_DEFINED_ENVIRON
	extern const char **environ;
#endif
	const char **sptr, **envp, **src, **dst;

	for (sptr = (const char **)environ; *sptr != NULL; ++sptr)
		;
	envp = dst = (const char **)e_malloc(sizeof(char *) *
					     (sptr - (const char **)environ + 2));
	src = (const char **)environ;
	while (*src != NULL)
		*dst++ = *src++;
	*dst++ = s;
	*dst = NULL;
	return envp;
}
#endif


typedef struct addr_s{
  taddr_t addr;
  const char *path;
} addr_t;

static int
addr_cmp(addr1, addr2)
addr_t *addr1, *addr2;
{
  if (!addr1 || !addr2)
    return 1;
  return addr1->addr < addr2->addr;
}

char *
get_unloaded_lib(addr)
taddr_t addr;
{
  iproc_t *ip;
  Solib_addr *sa;
  int i;
  addr_t *n, *nroot;
  char *ret;
  
  ip = GET_IPROC(get_current_target());
  for (i = 0, sa = ip->ip_solib_addrs; sa != NULL; sa = sa->next)
    if (sa->vaddr)
      i++;
  if (i == 0)
    return 0;
  nroot = n = malloc(sizeof(addr_t)*(i+1));
  for (sa = ip->ip_solib_addrs; sa != NULL; sa = sa->next, n++)
    if (sa->vaddr)
    {
      n->addr = sa->vaddr;
      n->path = sa->path;
    }
  n->addr = 0;
  n->path = 0;
  qsort((char *)nroot, i, sizeof(addr_t), addr_cmp);

  for (n = nroot; n->addr; n++)
    if (addr > n->addr)
    {
      ret = (char *)n->path;
      free(nroot);
      return ret;
    }
  free(nroot);
  return 0;
}

void
reset_main_debug_address()
{
	Main_debug_vaddr = 0;	/* RGA for reattach with rescan */
}

static void
load_from_libdep(xp, par, level, name)
     target_t *xp;
     Libdep *par; 
     int level;
     char *name;
{
  Libdep *ld;
	
  if (!name && par->par)
    load_elf_shlib(xp, par);
  else 
    if (par->par && !strcmp(par->name, name))
    {
      load_elf_shlib(xp, par);
      return;
    }
  
  for (ld = par->deps; ld != NULL; ld = ld->next)
    load_from_libdep(xp, ld, level + 1, name);
}

void
load_from_solibs(xp, name)
     target_t *xp;
     char *name;
{
  Solib *so;
  iproc_t *ip;

  ip = GET_IPROC(xp);

  for (so = ip->ip_solibs; so != NULL; so = so->next)
  {
    if (!name && so->latest_libdep->par)
      load_elf_shlib(xp, so->latest_libdep);
    else 
      if (so->latest_libdep->par && !strcmp(so->symtab->st_path, name))
      {
	load_elf_shlib(xp, so->latest_libdep);
	return;
      }
  }
  load_from_libdep(xp, ip->ip_solibs->latest_libdep, 0, name);
}

/* RGA a hack to attempt to reuse previously read libraries when attaching or
   starting a target which is newer or of a different name. Need to change
   procfs_attach_to_process() too for this to work */
void
elf_make_main_symtab(xp, target_updated, pid)
target_t *xp;
bool target_updated;
int pid;
{
  Libdep *ld;
  int fd, i;
  long mtime;
  iproc_t *ip;

  ip = GET_IPROC(xp);
  
#if AO_USE_PROCFS
  procfs_init(ip);
#else
	ip->ip_ptrace_info = NULL;
#if defined(AO_AOUT_SHLIBS)
	ip->ip_shlibs_ap = NULL;
	ip->ip_shlibs = NULL;
#endif
#endif
  ld = make_libdep(xp->xp_apool, xp->xp_textpath, (Libdep *)NULL, 0);
  if ((check_target(xp->xp_textpath, &fd, &i, &mtime, FALSE)) == -1)
    return;
  if (target_updated == TRUE)
  {
    errf("\bScanning symbols of `%s'...", xp->xp_textpath);
    wn_do_flush();
  }
  scan_elf_symtab(xp->xp_apool, xp->xp_textpath, fd, ld, &xp->xp_entryaddr,
		  &xp->xp_mainfunc, &xp->xp_initfunc, &ip->ip_solibs,
		  target_updated);
  if (target_updated == TRUE)
  {
    errf("\bScanning symbols of `%s'...done", xp->xp_textpath);
    wn_do_flush();
  }
  if (ld->so)
  {
    ld->so->symtab->st_modtime = mtime;
#ifdef AO_USE_PTRACE /* interp */
    if (!get_preload_shlib_list (xp->xp_apool, xp->xp_textpath, &ip->ip_solibs,
				 &ip->ip_solib_addrs))
      return;
#endif
    Main_debug_vaddr = ld->so->debug_vaddr;
  }
}

func_t *
elf_decode_plt(xp, pc)
target_t *xp;
taddr_t pc;
{
   func_t *f = NULL;

#ifdef ARCH_386
   taddr_t ebx;
   unsigned char jmp[6];
   taddr_t newpc;

   ebx = xp_getreg(xp, 3);
   
   xp_read_data(xp, pc, (char *)jmp, 6);

   if (jmp[0] == 0xff && jmp[1] == 0x25) /* jmp *ADDR */
      xp_read_data(xp, *((taddr_t *)(jmp + 2)), (char *)&newpc, 4);
   else if (jmp[0] == 0xff && jmp[1] == 0xa3) /* jmp *OFFSET(%ebx) */
      xp_read_data(xp, *((taddr_t *)(jmp + 2)) + ebx, (char *)&newpc, 4);
   else
      panic("Unexpected opcode in procedure linkage table");

   if (newpc == pc + 6) {
      ao_stdata_t *ast = AO_STDATA(addr_to_func(pc)->fu_symtab);
      Solib *solib = ast->st_solib;
      unsigned char push[5];
      Elf32_Sym sym;
      taddr_t off;
      char func_name[256];
      func_t *f1;
      
      xp_read_data(xp, newpc, (char *)push, 5);
      if (push[0] != 0x68) /* push NNN */
         panic("Unexpected opcode in procedure linkage table");

      off = *((taddr_t *)(push + 1)) + solib->plt_rel_vaddr;

      if (solib->plt_rel_type == DT_REL) {
	 Elf32_Rel rel;
	 
	 xp_read_data(xp, ast->st_base_address + off, (char *)&rel, sizeof(rel));
	 off = ELF32_R_SYM(rel.r_info) * sizeof(sym) + solib->dyn_symtab_vaddr;
      }
      else if (solib->plt_rel_type == DT_RELA) {
	 Elf32_Rela rela;
	 
	 xp_read_data(xp, ast->st_base_address + off, (char *)&rela, sizeof(rela));
	 off = ELF32_R_SYM(rela.r_info) * sizeof(sym) + solib->dyn_symtab_vaddr;
      }
      else {
	 panic("Unexpected relocation type in procedure linkage table");
      }

      xp_read_data(xp, ast->st_base_address + off, (char *)&sym, sizeof(sym));

      off = sym.st_name + solib->dyn_strtab_vaddr;
      xp_read_data(xp, ast->st_base_address + off, func_name, 256);

      (void)find_func_by_name(func_name, &f, &f1, FALSE);

      if (f1 != NULL) {
         errf("Can't resolve function %s in procedure linkage table - multiple/overloaded instances exist", func_name);
         f = NULL;
      }
      else if (f == NULL) {
         errf("Can't resolve function %s in procedure linkage table - no such function", func_name);
      }
   }
   else {
      if ((f = addr_to_func(newpc)) == NULL) {
         errf("Can't resolve function for address %p in procedure linkage table", newpc);
      }
   }
#endif

   return f;
}

#endif /* AO_ELF */
