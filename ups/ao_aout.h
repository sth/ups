/* ao_aout.h - header file for ao_aout.c */

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


/* @(#)ao_aout.h	1.1 24/5/95 (UKC) */

#ifdef AO_EXECINFO_H_INCLUDED
bool aout_get_exec_info PROTO((const char *textpath, int fd,
			       taddr_t shlib_load_addr, Execinfo *ei));
#endif

int aout_check_exec_header PROTO((int fd, const char *textpath,
				  taddr_t *p_data_addr));

bool aout_scan_main_symtab PROTO((const char *textpath, int fd,
				  struct symtab_s **p_symtabs,
				  struct symtab_s **p_symtab_cache,
				  taddr_t *p_entryaddr,
				  struct func_s **p_mainfunc));

bool aout_scan_symtab PROTO((const char *textpath, int fd,
			     taddr_t text_addr_offset,
			     struct symtab_s **p_symtab,
			     taddr_t *p_entryaddr,
			     struct func_s **p_mainfunc));

#ifdef AO_CORE_H_INCLUDED
bool aout_get_core_info PROTO((alloc_pool_t *ap, const char *corepath, int fd,
			       bool want_messages, int *p_signo,
			       char **p_cmdname, const char **p_cmdline,
			       Core_segment **p_segments, int *p_nsegments,
			       char **p_regs));

int aout_get_core_reg PROTO((Coredesc *co, int regno, taddr_t *p_val));

const char *get_command_line_from_stack PROTO((alloc_pool_t *ap, Coredesc *co));
#endif
