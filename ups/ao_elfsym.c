/* ao_elfsym.c - handle Solaris 2 ELF symbol tables  */

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


/* @(#)ao_elfsym.c	1.3 20 Jun 1995 (UKC) [Patched] */
char ups_ao_elfsym_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#ifdef AO_ELF

#if HAVE_ELF_H
#include <elf.h>
#elif HAVE_LINUX_ELF_H
#include <linux/elf.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <mtrprog/io.h>
#include <mtrprog/hash.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "st.h"
#include "srcwin.h"
#include "ui.h"			/* for get_or_create_outwin() */
#include "ao_core.h"
#include "ao_execinfo.h"
#include "ao_dwarf.h"
#include "ao_syms.h"
#include "ao_symload.h"
#include "ao_text.h"
#include "ao_symread.h"
#include "ao_symscan.h"
#include "ao_aflist.h"
#include "ao_elfpriv.h"
#include "ao_elfread.h"
#include "ao_elfsym.h"
#include "ao_elfread.h"
#include "ao_elflib.h"
#include "elfstab.h"
#include "srcpath.h"
#include "util.h"
#include "va.h"

static bool Had_error;
	
static void
check_optsym_matches(alloc_pool_t *ap, stf_t *stf,
		     const char *optstr, time_t obj_mtime,
		     bool *p_has_debug_syms)
{
	const char *global_prefix;
	Compiler_type compiler_type = CT_UNKNOWN;
	
	elf_handle_optsym(ap, optstr, &stf->stf_language,
			  &global_prefix, &compiler_type, p_has_debug_syms);

        /* RCB: See if the compiler was found */
        if ( compiler_type != CT_UNKNOWN)
        {
            stf->stf_fil->fi_flags |= FI_FOUND_COMPILER;
            stf->stf_compiler_type = compiler_type;
        }

	/*  Can't be bothered to check for compiler type mismatch.
	 */

	if (global_prefix != NULL) {
		if (stf->stf_global_prefix == NULL) {
			stf->stf_global_prefix = global_prefix;
		}
		else if (strcmp(stf->stf_global_prefix, global_prefix) != 0) {
			errf("Global prefix mismatch for %s: %s != %s",
			     stf->stf_name,
			     stf->stf_global_prefix, global_prefix);
		}
	}

	if (obj_mtime != 0) {
		if (stf->stf_obj_mtime == 0) {
			stf->stf_obj_mtime = obj_mtime;
		}
		else if (stf->stf_obj_mtime != obj_mtime) {
			errf("Obj file mtime mismatch for %s: %ld != %ld",
			     stf->stf_name,
			     stf->stf_obj_mtime, obj_mtime);
		}
	}
}

static bool
lookup_fil(fil_t *sfiles, const char *path_hint, const char *name,
	   fil_t **p_fil)
{
	fil_t *fil;

	for (fil = sfiles; fil != NULL; fil = fil->fi_next) {
		if (same_string(path_hint, fil->fi_path_hint) &&
		    strcmp(name, fil->fi_name) == 0) {
			*p_fil = fil;
			return TRUE;
		}
	}
 
	return FALSE;
}

const char *
elf_name_from_symtab_name(const char *prefix, const char *symtab_name)
{
	size_t pflen;

	if (prefix == NULL)
		return symtab_name;

	pflen = strlen(prefix);
	
	if (strlen(symtab_name) <= pflen)
		return symtab_name;
	
	if (memcmp(symtab_name, prefix, pflen) == 0)
		return &symtab_name[pflen];

	if (pflen > 3 &&
	    memcmp(symtab_name, prefix, 2) == 0 &&
	    symtab_name[2] == prefix[2] + 1 &&
	    memcmp(&symtab_name[3], &prefix[3], pflen - 3) == 0)  {
		const char *dot;

		dot = strchr(symtab_name + pflen, '.');

		if (dot != NULL && dot > symtab_name + pflen && dot[1] != '\0')
			return dot + 1;
	}

	return symtab_name;
}

static void
update_or_add_sn(Symrec *sr, alloc_pool_t *ap, stf_t *stf,
		 const char *symtab_name)
{
	snlist_t *sn;

	if (!symtab_sym_name_to_sn(stf->stf_snlist, symtab_name, &sn, FALSE))
		push_symname(sr,ap,  stf, symtab_name, 0);
}


static bool
scan_index_syms(symtab_t *st, Elfinfo *el, fil_t **p_sfiles, func_t **p_flist,
		symio_t *symio, int nsyms, const char **p_mainfunc_name)
{
	int symno, opt_with_no_optsym;
	off_t file_offset, next_file_offset;
	const char *objdir, *path_hint, *prefix;
	size_t pflen;
	stf_t *stf;
	fil_t *new_sfiles;
	block_t *rootblock;
	const char *mainfunc_name;
	alloc_pool_t *ap;
	bool seen_sosym_but_no_optsym;
	Symrec symrec;
	
	file_offset = next_file_offset = 0;
	objdir = path_hint = NULL;
	stf = NULL;
	new_sfiles = *p_sfiles;
	rootblock = get_rootblock();
	ap = st->st_apool;
	prefix = NULL;
	pflen = 0;
	mainfunc_name = NULL;
	seen_sosym_but_no_optsym = FALSE;
	opt_with_no_optsym = 0;
	
	for (symno = 0; symno < nsyms; ++symno) {
		nlist_t nm;
		
		getsym(symio, symno, &nm);
		
		switch (nm.n_type) {
			const char *path;
			fil_t *fil;
			bool has_debug_syms;
			
		case N_ILDPAD:
			next_file_offset += nm.n_value;
			break;
		case N_UNDF:
			file_offset = next_file_offset;
			next_file_offset = file_offset + nm.n_value;
			add_extra_string_offset(symio, symno, file_offset);
			break;

		case N_MAIN:
			if (p_mainfunc_name != NULL) {
				const char *name;
				
				name = symstring(symio, symno);
				*p_mainfunc_name = alloc_strdup(ap, name);
			}
			break;
			
		case N_SO:
			path = symstring(symio, symno);

			if (path_hint == NULL &&
			    path[strlen(path) - 1] == '/') {
				path_hint = path;
				break;
			}
			
			/* RGA */
			/*if (!path_hint || opt_with_no_optsym > 1)*/ 
			/*			  break;*/

			if (lookup_fil(*p_sfiles, path_hint, path, &fil)) {
				stf = AO_FIDATA(fil);
			}
			else {

				stf = make_stf(ap, alloc_strdup(ap, path),
					       st, 0, srctype(path), 0);
				
				new_sfiles = ao_make_fil(stf, rootblock,
						      alloc_strdup(ap,
								   path_hint),
						      new_sfiles);
				stf->stf_fil = new_sfiles;
				
				stf->stf_flags |= STF_NEED_SYMSCAN;
			}

			path_hint = NULL;

			prefix = stf->stf_global_prefix;
			pflen = (prefix != NULL) ? strlen(prefix) : 0;

			seen_sosym_but_no_optsym = TRUE;

			break;

		case N_OPT:
			/*  The Sun C compiler emits an N_OPT before any N_SO,
			 *  giving the compiler and symtab version numbers.
			 *  I have also seen compilers emit an N_OPT with
			 *  no matching preceding N_SO.
			 *
			 *  Thus we ignore an N_OPT unless we have previously
			 *  seen an N_SO, and this is the first N_OPT we
			 *  have seen since.
			 */
		        if (!seen_sosym_but_no_optsym)
			{
			  /*			  opt_with_no_optsym++;*/
			  break;
			}

			seen_sosym_but_no_optsym = FALSE;
			
			check_optsym_matches(st->st_apool, stf,
					     symstring(symio, symno),
					     (time_t)nm.n_value,
					     &has_debug_syms);

			/*  There's no point loading symbols from object
			 *  files that weren't compiled with -g.  Also,
			 *  skipping these avoids error messages about
			 *  nonexistent .o files pointed at by shared libraries.
			 */
			if (!has_debug_syms &&
			    (stf->stf_flags & STF_NEED_SYMSCAN) != 0) {
				prefix = NULL;
				pflen = 0;
				stf = NULL;
				new_sfiles = new_sfiles->fi_next;
			}
			else {
				prefix = stf->stf_global_prefix;
				pflen = (prefix != NULL) ? strlen(prefix) : 0;
			}
			
			break;

		case N_OBJ:
			path = symstring(symio, symno);
			
			/* RGA */
			/*			if (opt_with_no_optsym > 1)
			  break;*/
#if 0
/* RGA temp */
			if (!strncmp(path, "Templates.DB", 12)) 
			{
			  if (lookup_fil(*p_sfiles, objdir, path, &fil)) {
			    stf = AO_FIDATA(fil);
			  }
			  else {
			    stf = make_stf(ap, alloc_strdup(ap, path),
					   st, 0, srctype(path), 0);
				
			    new_sfiles = ao_make_fil(stf, rootblock,
						  alloc_strdup(ap,
							       objdir),
						  new_sfiles);
			    stf->stf_fil = new_sfiles;
				
			    stf->stf_flags |= STF_NEED_SYMSCAN;
			  }

			}
#endif
			if (objdir == NULL) {
			  objdir = path;
			  break;
			}
			
			if (stf == NULL) {
			  objdir = NULL;
			  break;
			}
			
			stf->stf_objpath_hint = alloc_strdup(ap, objdir);
			stf->stf_objname = alloc_strdup(ap, path);
			break;

		case N_CMDLINE:
		case N_PATCH:
			break;
			
		case N_FUN:
			if (stf == NULL ||
			    (stf->stf_flags & STF_NEED_SYMSCAN) == 0) {
				break;
			}

			add_function_to_symtab(st, p_flist,
					       symstring(symio, symno),
					       stf->stf_fil, (fil_t *)NULL,
					       FALSE, FALSE, 0, 0);
			break;

		case N_GSYM:
		case N_ISYM:	/* RGA added for Forte compiler */
		case N_ESYM:	/* RGA added for Forte compiler */
			if (stf != NULL) {
			  symrec.symio = symio;
			  symrec.symno = symno;
			  update_or_add_sn(&symrec, ap, stf,
					   symstring(symio, symno));
			}
			break;

#ifdef N_XLINE
		case N_XLINE:
				/* Purelink emits these benignly */
			break;
#endif
		
		default:
			errf("\b%s stab.index symbol #%d: unknown type %d",
			     el->path, symno, nm.n_type);
			break;
		}
	}

	*p_sfiles = new_sfiles;

	return TRUE;
}

static bool
setup_ofile_symio(Elfinfo *el, stf_t *stf, off_t ar_offset)
{
	Elf32_Shdr *symsh, *strsh;
	const char *path;

	if (stf->stf_symio != NULL)
		panic("dup symio botch in setup_ofile_symio");
	
	if (!elf_find_section(el, ".stab.excl", "debugging symbols", &symsh) ||
	    !elf_find_section(el, ".stab.exclstr", "debugging symbol strings",
			      &strsh))
		return FALSE;
	
#if 0
/* RGA temp */
	if (!elf_find_section(el, ".stab", "debugging symbols", &symsh) ||
	    !elf_find_section(el, ".stabstr", "debugging symbol strings",
			  &strsh))
		return FALSE;
#endif
	path = alloc_strdup(stf->stf_symtab->st_apool, el->path);
	stf->stf_symio = make_symio((alloc_pool_t *)NULL, path, el->fd,
				    (off_t)symsh->sh_offset + ar_offset,
				    (int)(symsh->sh_size / symsh->sh_entsize),
				    (off_t)strsh->sh_offset + ar_offset);
	return TRUE;
}

static void
to_output_window(const char *mesg)
{
	Outwin *ow;
	
	ow = get_or_create_outwin();
	if (*mesg == '\b')
	  ++mesg;		/* skip bell */
	outwin_insert(ow, mesg, strlen(mesg));
	outwin_insert(ow, "\n", 1);
	Had_error = TRUE;
}

static bool
scan_dot_o_file_symtab(stf_t *stf)
{
	Elf32_Ehdr hdr;
	Elfinfo *el;
	char *name, *member;
	const char *path;
	int fd;
	off_t ar_offset;
	bool ok;
	errf_ofunc_t old_ofunc;

	old_ofunc = errf_set_ofunc(to_output_window);
	fd = -1;
	path = member = NULL;
	Had_error = FALSE;
	el = NULL;
	
	ar_get_path_and_member(stf->stf_objname, &name, &member);
		
	ok = (srcpath_resolve_path("object file", stf->stf_objpath_hint,
				   name, &path) &&
	      open_for_reading(path, ".o file", &fd) &&
	      ar_get_member_offset(path, fd, member, &ar_offset) &&
	      read_chunk(path, "", fd, "ELF header", ar_offset,
			&hdr, sizeof(hdr)) &&
	      check_elf_header_type(&hdr, path, ET_REL, "read symbols from") &&
	      prepare_to_parse_elf_file(path, fd, &hdr, &el, ar_offset) &&
	      setup_ofile_symio(el, stf, ar_offset));

	if (ok) {
	   	scan_symtab(stf->stf_symtab, path, stf,
			    (func_t **)NULL, (const char **)NULL);
		stf->stf_flags &= ~STF_NEED_SYMSCAN;
	}

	if (el != NULL)
		free_elfinfo(el);
	
	free(member);
	
	if (!ok) {
		if (fd != -1)
			close(fd);
/*		free(name);   see below */
	}
	
	errf_set_ofunc(old_ofunc);

	if (Had_error) {
		const char *namestr;
		
		namestr = strrchr(path, '/');
		if (name == NULL || name[1] == '\0')
			namestr = path;
		else
			++namestr;
			
		errf("\bError loading symbols from %s - see output window",
		     namestr);
	}
	if (!ok)		/* RGA for purify */
		free(name);

	return ok;
}

bool
elf_scan_then_setup_symio(fil, p_symio)
fil_t *fil;
symio_t **p_symio;
{
	stf_t *stf;
	
	if (fil == NULL) {
		*p_symio = AO_STDATA(fil->fi_symtab)->st_text_symio;
		return TRUE;
	}

	stf = AO_FIDATA(fil);
	if (stf == NULL)	/* RGA */
		return FALSE;

	/*  We always want the source file stf (i.e. we don't set up
	 *  the symio fields for header files).  We check for stf_fmap
	 *  non NULL because we may not have done the scan_symtab() for this
	 *  file yet.
	 *
	 *  I think the only time we are called on a header file is with
	 *  an early version of the SC 3.0 compiler, which emits N_BINCL
	 *  syms with a non-zero n_value (see the code in get_fi_types()).
	 *
	 *  BUG: The header file stuff is long overdue for a rewrite.
	 */
	if (stf->stf_fmap != NULL)
		stf = stf->stf_fmap[0]->hf_stf;

	if ((stf->stf_flags & STF_NEED_SYMSCAN) != 0 &&
	    !scan_dot_o_file_symtab(stf)) {
		return FALSE;
	}

	*p_symio = stf->stf_symio;
	return TRUE;
}


void
elf_handle_optsym(alloc_pool_t *ap, const char *opts, language_t* language,
		  const char **p_global_prefix, Compiler_type *p_compiler_type,
		  bool *p_has_debug_syms)
{
	const char *global_prefix;
	bool has_debug_syms;
	Compiler_type compiler_type = CT_CC;

	global_prefix = NULL;
	has_debug_syms = FALSE;
	
	if (strcmp(opts, "gcc2_compiled.") == 0) {
		compiler_type = CT_GNU_CC;
	}
	else {
		const char *s;

		switch (language?*language:LANG_UNKNOWN) {
		case LANG_F77:
			compiler_type = CT_F77;
			break;
		case LANG_F90:
			compiler_type = CT_EPCF90;
			break;
		case LANG_C:
		case LANG_CC:
			if (*p_compiler_type == CT_UNKNOWN ||
			    *p_compiler_type == CT_CC)
			{	/* RGA don't reset to CT_CC on getting */
				/* compiler command options*/
			   if (strncmp(opts, "clcc_", 5) == 0) 
			     compiler_type = CT_CLCC;
			   else
			     compiler_type = CT_CC;
			 }
			break;
		default:
			compiler_type = CT_UNKNOWN;
			break;
		}
		
		for (s = opts; *s != '\0'; ) {
			const char *opt;
		
			while (isspace(*s))
				++s;

			opt = s;

			while (!isspace(*s) && *s != ';' && *s != '\0')
				++s;
		
			if (*opt == 'G' && opt[1] == '=') {
				char *optval;
				size_t len;
			
				opt += 2;
				len = s - opt;
			
				optval = allocstr(ap, len + 1);
				memcpy(optval, opt, len);
				optval[len] = '\0';

				global_prefix = optval;
			}
			else if (*opt == 'g' && s == &opt[1]) {
				has_debug_syms = TRUE;
			}
			else if (*opt == 'V' && opt[1] == '=') {
			     double ver;
			     if ( sscanf(opt+2,"%lf",&ver))
				 set_compiler_version(ver);
			}
			else if (*opt == 'R' && compiler_type == CT_UNKNOWN)
			{
			     /* Attempt to identify both compiler and
			     ** language from opt string
			     */
			     const char* nextopt = strchr(opt,';');
			     const char* langString = strstr(opt,"C++");
			     if ( langString )
			     {
#if 1
     if (langString < nextopt || nextopt==NULL)
     {
         /* Treat C put through a C++ compiler as C++ */
         compiler_type = CT_CC;
         if (language &&
                 ((*language == LANG_UNKNOWN) || (*language == LANG_C)))
             *language = LANG_CC;
     }
#else
				 if (langString < nextopt || nextopt==NULL)
			         {
				     compiler_type = CT_CC;
				     if (language && *language == LANG_UNKNOWN)
				         *language = LANG_CC;
			         }
#endif
			     } else
			     {
				 langString = strstr(opt," C ");
			         if ( langString
				     && (langString < nextopt || nextopt==NULL))
			         {
				     compiler_type = CT_CC;
				     if (language && *language == LANG_UNKNOWN)
				         *language = LANG_C;
			         }
			     }
			}

			while (isspace(*s))
				++s;
			if (*s == ';')
				++s;
		}
	}

	
	ao_compiler(NULL, TRUE, compiler_type);
	*p_compiler_type = compiler_type;
	*p_global_prefix = global_prefix;
	*p_has_debug_syms = has_debug_syms;
}

static bool
scan_stab_index(symtab_t *st, Elfinfo *el, fil_t **p_sfiles,
		    func_t **p_flist, const char **p_mainfunc_name)
{
	symio_t *symio;
	bool ok;
	int nsyms, fd;
	
	if (el->indexsh == NULL)
		return TRUE;

	if ((fd = dup(el->fd)) == -1) {
		failmesg("Can't dup fd for", el->what, el->path);
		return FALSE;
	}

	nsyms = el->indexsh->sh_size / el->indexsh->sh_entsize;
	symio = make_symio((alloc_pool_t *)NULL, el->path, fd,
			   (off_t)el->indexsh->sh_offset, nsyms,
			   (off_t)el->indexstrsh->sh_offset);

	ok = scan_index_syms(st, el, p_sfiles, p_flist, symio, nsyms,
			     p_mainfunc_name);

	close_symio(symio);

	return ok;
}

static bool
resolve_func_addr(const char *name, int bind, fil_t *fil, taddr_t addr,
		  hashtab_t *ht, hashvalues_t *hv)
{
	if (bind == STB_LOCAL) {
		func_t *f;

		if (fil == NULL)
			return FALSE;

		if ((f = name_and_fil_to_func(name, fil, FALSE)) == NULL)
			return FALSE;

		f->fu_addr = addr;
	}
	else {
		int i;
		size_t len;
		bool need_fortran;

		len = strlen(name);
		
		/*  The following little dance is because SC f77 adds a
		 *  trailing underscore to Fortran function names in the
		 *  .symtab section (there is no trailing underscore in
		 *  the .stab section).
		 */
		if (hash_lookup_key(ht, name, len, hv)) {
			need_fortran = FALSE;
		}
		else {
			if (len < 1 || name[len - 1] != '_' ||
			    !hash_lookup_key(ht, name, len - 1, hv)) {
				return FALSE;
			}
			need_fortran = TRUE;
		}
		
		for (i = 0; i < hv->hv_nvalues; ++i) {
			func_t *f;
			
			f = (func_t *)hv->hv_values[i];

			if (!need_fortran || IS_FORTRAN(f->fu_language))
			{
			    if (f->fu_language == LANG_CC)
			    {
				if (!(f->fu_flags & FU_DONE_ADDR))
				{
				  f->fu_addr = addr;
				  f->fu_flags |= FU_DONE_ADDR;
				  break;
				}
			    }
			    else
				f->fu_addr = addr;
			}
		}
	}

	return TRUE;
}

static hashtab_t *
make_func_hash(alloc_pool_t *ap, func_t *flist)
{
	int num_global_funcs;
	hashtab_t *ht;
	func_t *f;

	num_global_funcs = 0;
	for (f = flist; f != NULL; f = f->fu_next) {
		if ((f->fu_flags & FU_STATIC) == 0)
			++num_global_funcs;
	}
	
	ht = hash_create_tab(ap, num_global_funcs);
	
	for (f = flist; f != NULL; f = f->fu_next) {
		if ((f->fu_flags & FU_STATIC) == 0) {
			hash_enter(ht, f->fu_name, strlen(f->fu_name),
				   (hash_value_t)f);
		}
	}

	return ht;
}

static hashtab_t *
make_fil_hash(alloc_pool_t *ap, fil_t *sfiles)
{
	fil_t *fil;
	hashtab_t *ht;
	int nfiles;

	nfiles = 0;
	for (fil = sfiles; fil != NULL; fil = fil->fi_next)
		++nfiles;

	ht = hash_create_tab(ap, nfiles);

	for (fil = sfiles; fil != NULL; fil = fil->fi_next) {
		const char *basename;

		basename = strrchr(fil->fi_name, '/');

		if (basename == NULL || basename[1] == '\0')
			basename = fil->fi_name;
		else
			++basename;

		hash_enter(ht, basename, strlen(basename), (hash_value_t)fil);
	}

	return ht;
}

static bool
set_function_addresses(Elfinfo *el, symtab_t *st, func_t **p_flist)
{
	Elf32_Shdr *strings_sh;
	Elf32_Sym *syms;
	size_t i, nsyms, strings_nbytes;
	char *strings;
	func_t *funclist;
	fil_t *fil, *lastfil;
	alloc_pool_t *hash_ap;
	hashtab_t *fil_ht, *func_ht;
	hashvalues_t *hv;
	Ambig_fil *aflist;

	strings_sh = &el->sections[el->symtab_sh->sh_link];

	syms = e_malloc(el->symtab_sh->sh_size);
	strings = e_malloc(strings_sh->sh_size);

	if (!read_elf_section(el, "symbol table", el->symtab_sh, syms) ||
	    !read_elf_section(el, "symtab strings", strings_sh, strings)) {
		free(syms);
		free(strings);
		return FALSE;
	}

	nsyms = el->symtab_sh->sh_size / el->symtab_sh->sh_entsize;
	strings_nbytes = strings_sh->sh_size;

	hash_ap = alloc_create_pool();
	fil = lastfil = NULL;
	funclist = *p_flist;
	aflist = NULL;

	func_ht = make_func_hash(hash_ap, funclist);
	fil_ht = make_fil_hash(hash_ap, st->st_sfiles);

	hv = hash_make_hashvalues();
	st->st_func_hv = (char *)hv;

	for (i = 0; i < nsyms; ++i) {
		Elf32_Sym *sym;
		const char *name;
		taddr_t value;
		int type, bind, ar;

		sym = &syms[i];

		value = sym->st_value;
		type = ELF32_ST_TYPE(sym->st_info);
		bind = ELF32_ST_BIND(sym->st_info);

		if (sym->st_name > strings_nbytes) {
			errf("\bString offset out of range for "
			     ".stab symbol #%ld - ignored", (long)i);
			continue;
		}
		name = &strings[sym->st_name];

		switch (type) {
		case STT_FUNC:
			if (sym->st_shndx == SHN_UNDEF)
				break;
			if (!resolve_func_addr(name, bind, fil, value,
					       func_ht, hv)) {
				add_function_to_symtab(st, p_flist, name,
						       fil, (fil_t *)NULL,
						       bind == STB_LOCAL, FALSE,
						       0, value);

				if (bind == STB_WEAK)
					(*p_flist)->fu_flags |= FU_WEAK;
			}
			break;
			
		case STT_OBJECT:
			insert_global_addr(st->st_apool, &st->st_addrlist,
					   alloc_strdup(st->st_apool, name),
					   value);
			break;

		case STT_FILE:
			lastfil = fil;

			if (strcmp(name, "crtn.s") == 0) {
				fil = NULL;
			}
			else {
				fil = basename_to_maybe_ambig_fil(st, name,
								  fil_ht, hv,
								  &aflist);
			}
			break;

		case STT_NOTYPE:
			if (strcmp(name, "Bbss.bss") == 0)
				ar = AR_BSS;
			else if (strcmp(name, "Ddata.data") == 0)
				ar = AR_DATA;
			else if (strcmp(name, "Drodata.rodata") == 0)
				ar = AR_RODATA;
			else
				break;

			if (lastfil != NULL)
				AO_FIDATA(lastfil)->stf_range[ar].lim = value;

			if (fil != NULL)
				AO_FIDATA(fil)->stf_range[ar].base = value;
			
			break;

		default:
			break;
		}
	}

	resolve_aflist(*p_flist, aflist);

	alloc_free_pool(hash_ap);
	free(syms);
	free(strings);

	return TRUE;
}

/*  Do a prescan of the symbol table of textpath.
 */
bool
scan_elf_symtab(target_ap, textpath, fd, libdep, p_entryaddr, p_mainfunc,
		p_initfunc, p_solibs, reattach_with_rescan)
alloc_pool_t *target_ap;
const char *textpath;
int fd;
Libdep *libdep;
taddr_t *p_entryaddr;
func_t **p_mainfunc;
func_t **p_initfunc;
Solib **p_solibs;
bool reattach_with_rescan;
{
	symtab_t *st;
	Execinfo eibuf;
	const char *mainfunc_name, **p_mainfunc_name, *initfunc_name;
	func_t *flist;
	Elfinfo *el;
	symtab_type_t st_is;
#if WANT_DWARF
	Dwarf_Debug dw_dbg;
	Dwarf_Error dw_err;
	int rv;
#endif

	if (!elf_get_exec_info(textpath, fd, libdep, &eibuf, &el, &st_is))
		return FALSE;

	if (st_is == ST_DWARF) {
		if ((rv = dwarf_init(fd, DW_DLC_READ, NULL, NULL,
				     &dw_dbg, &dw_err) == DW_DLV_ERROR)) {
			dwf_fatal_error("dwarf_init", rv, NULL, dw_err);
			free_elfinfo(el);
			alloc_free_pool(st->st_apool);
			return FALSE;
		}
		if (rv == DW_DLV_NO_ENTRY) {
			errf("Can't debug %s: no DWARF debugging information",
			      textpath);
			free_elfinfo(el);
			alloc_free_pool(st->st_apool);
			return FALSE;
		}
	}

	if (p_entryaddr)
		*p_entryaddr = eibuf.entry_addr;

	mainfunc_name = NULL;
	p_mainfunc_name = (p_mainfunc != NULL) ? &mainfunc_name : NULL;
	if (!scan_ao_symtab(textpath, fd, &eibuf, dw_dbg, 0,
			    st_is, &st, &flist, p_mainfunc_name)) {
		free_elfinfo(el);
		alloc_free_pool(st->st_apool);
		return FALSE;
	}
  
	if (st_is == ST_DWARF) {
		/*
		 * dwarfTODO: probably need equivalent of the routines below
		 * but not sure what they are trying to do
		 */
	} else {
		scan_stab_index(st, el, &st->st_sfiles, &flist, p_mainfunc_name);
		set_function_addresses(el, st, &flist);
	}

	if (el->pltsh) {
		add_function_to_symtab(st, &flist, "[_plt_]", NULL, NULL, 
					FALSE, FALSE, 0, el->pltsh->sh_addr );
		flist->fu_flags |= FU_PLT;
	}

#if WANT_DEBUG
if (strncmp(textpath, "/usr/lib", 6) != 0) {
dump_header(textpath);
dump_symtab_t(st, FALSE);
dump_trailer();
}
#endif

	if (!add_solib_entry(target_ap, st, flist, el, p_solibs)) {
		free_elfinfo(el);
		alloc_free_pool(st->st_apool);
		return FALSE;
	}
	
	free_elfinfo(el);

	initfunc_name = "_init"; /* RGA temp */
	do_ao_postscan_stuff(st, flist, &eibuf, p_mainfunc, mainfunc_name,
			     p_initfunc, initfunc_name);
	return TRUE;
}

bool
elf_scan_dot_o_file_symtab(fil)
fil_t *fil;
{
  stf_t *stf;

  stf = AO_FIDATA(fil);
  if (stf && (stf->stf_flags & STF_NEED_SYMSCAN) != 0)
    return scan_dot_o_file_symtab(stf);
  else
    return FALSE;
}
#endif /* !AO_ELF */
