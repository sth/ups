/* ao_target.h - common interface to the ao_ptrace.c and ao_proc.c */

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


/* @(#)ao_target.h	1.3 24 May 1995 (UKC) */

#define AO_TARGET_H_INCLUDED

#if AO_USE_PROCFS
typedef struct Procfs_info Procfs_info;
#else
typedef struct Ptrace_info Ptrace_info;
#endif

#if defined(AO_AOUT_SHLIBS)
/*  Shared library header on Suns.
 */
typedef struct shlibst {
	const char *sh_name;
	taddr_t sh_addr;
	struct shlibst *sh_next;
} shlib_t;
#endif

typedef struct iproc {
	int ip_pid;
	struct Coredesc *ip_core;
	taddr_t ip_base_sp;
	taddr_t ip_restart_pc;
	bool ip_lastsig;
	bool ip_attached;
	stopres_t ip_stopres;
#if AO_USE_PROCFS
	Procfs_info *ip_procfs_info;
#else
	Ptrace_info *ip_ptrace_info;
#endif
#ifdef AO_ELF
	struct Solib *ip_solibs;
	struct Solib_addr *ip_solib_addrs;
#else
	symtab_t *ip_main_symtab;
	symtab_t *ip_shlib_symtabs;
	symtab_t *ip_symtab_cache;
#endif
#if defined(AO_AOUT_SHLIBS)
	alloc_pool_t *ip_shlibs_ap;
  	shlib_t *ip_shlibs;
#endif
} iproc_t;

#define GET_IPROC(xp)	((iproc_t *)(xp)->xp_data)


