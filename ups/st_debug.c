/* st_debug.c - symbol table debugging routines */

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


/* @(#)st_debug.c	1.1 04 Jun 1995 (UKC) */
char ups_st_debug_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <mtrprog/hash.h>

#include "ups.h"
#include "symtab.h"
#include "ao_syms.h"
#include "target.h"
#include "st.h"
#include "st_debug.h"
#include "va.h"
#include "util.h"
#include "ci.h"

static int load_func_info PROTO((func_t *f));
static bool get_file_or_func PROTO((const char *name, fil_t **p_fil, 
                                    func_t **p_f));
static var_t *reverse_varlist PROTO((var_t *varlist));
static enum_member_t *reverse_enumlist PROTO((enum_member_t *enumlist));
static aggr_or_enum_def_t *reverse_aelist PROTO((aggr_or_enum_def_t *aelist));
static typedef_t *reverse_tdlist PROTO((typedef_t *tdlist));
static void dump_varlist PROTO((FILE *fp, int level, var_t *varlist));
static bool dump_aelist PROTO((FILE *fp, int level, language_t language, 
                               aggr_or_enum_def_t *aelist));
static void dump_tdlist PROTO((FILE *fp, int level, language_t language, 
                               typedef_t *tdlist));
static bool dump_aggr PROTO((FILE *fp, int level, language_t language, 
                             aggr_or_enum_def_t *ae));
static bool dump_file_syms PROTO((FILE *fp, fil_t *fil));
static bool dump_block PROTO((FILE *fp, int level, language_t language, 
                              block_t *parbl, funclist_t *funclist));
static bool dump_func_syms PROTO((FILE *fp, int level, func_t *f));
static bool dump_all_syms PROTO((target_t *xp, FILE *fp));
static const char *get_fil_name PROTO((fil_t *fil));
#if WANT_DEBUG
static const char *st_dt_is_str PROTO((dt_is_t dt_is));
static const char *st_class_str PROTO((class_t class));
#endif

enum { ISPACES = 4 };

static int
load_func_info(f)
func_t *f;
{
	taddr_t junk_addr;
	
	if (get_min_bpt_addr(f, &junk_addr, FALSE) != 0)
		return -1;
	
	st_get_fu_lnos(f);
	st_get_fu_blocks(f);

	return 0;
}

static bool
get_file_or_func(name, p_fil, p_f)
const char *name;
fil_t **p_fil;
func_t **p_f;
{
	if (*name == '\0') {
		*p_fil = NULL;
		*p_f = NULL;
	}
	else if (strchr(name, '.') != NULL && strchr(name, ':') == NULL) {
		fil_t *fil;
		
		if ((fil = name_to_fil(name)) == NULL) {
			errf("Unknown source file name %s", name);
			return FALSE;
		}

		*p_fil = fil;
		*p_f = NULL;
	}
	else {
		func_t *f, *f1;

		if (find_func_by_name(name, &f, &f1, TRUE) != 0)
			return FALSE;
		
		*p_fil = NULL;
		*p_f = f;
	}

	return TRUE;
}

static var_t *
reverse_varlist(varlist)
var_t *varlist;
{
	var_t *v, *next, *newlist;

	newlist = NULL;
	
	for (v = varlist; v != NULL; v = next) {
		next = v->va_next;
		v->va_next = newlist;
		newlist = v;
	}
	
	return newlist;
}

static enum_member_t *
reverse_enumlist(enumlist)
enum_member_t *enumlist;
{
	enum_member_t *em, *next, *newlist;

	newlist = NULL;
	
	for (em = enumlist; em != NULL; em = next) {
		next = em->em_next;
		em->em_next = newlist;
		newlist = em;
	}
	
	return newlist;
}

static aggr_or_enum_def_t *
reverse_aelist(aelist)
aggr_or_enum_def_t *aelist;
{
	aggr_or_enum_def_t *ae, *next, *newlist;

	newlist = NULL;
	
	for (ae = aelist; ae != NULL; ae = next) {
		next = ae->ae_next;
		ae->ae_next = newlist;
		newlist = ae;
	}
	
	return newlist;
}

static typedef_t *
reverse_tdlist(tdlist)
typedef_t *tdlist;
{
	typedef_t *td, *next, *newlist;

	newlist = NULL;
	
	for (td = tdlist; td != NULL; td = next) {
		next = td->td_next;
		td->td_next = newlist;
		newlist = td;
	}
	
	return newlist;
}

static void
dump_varlist(fp, level, varlist)
FILE *fp;
int level;
var_t *varlist;
{
	var_t *v;

	varlist = reverse_varlist(varlist);
	
	for (v = varlist; v != NULL; v = v->va_next) {
		fprintf(fp, "%*s%s;\n", level * ISPACES, "",
			type_to_decl(v->va_name, v->va_type, v->va_class,
				     v->va_language, TRUE));
	}

	reverse_varlist(varlist);
}

static bool
dump_aelist(fp, level, language, aelist)
FILE *fp;
int level;
language_t language;
aggr_or_enum_def_t *aelist;
{
	aggr_or_enum_def_t *ae;
	bool ok;

	ok = TRUE;

	aelist = reverse_aelist(aelist);
	
	for (ae = aelist; ae != NULL && ok; ae = ae->ae_next) {
		if (ae->ae_type != NULL)
			ok = dump_aggr(fp, level, language, ae);
		else
			ok = dump_aelist(fp, level, language, ae->ae_sublist);
	}

	reverse_aelist(aelist);

	return ok;
}

static void
dump_tdlist(fp, level, language, tdlist)
FILE *fp;
int level;
language_t language;
typedef_t *tdlist;
{
	typedef_t *td;

	tdlist = reverse_tdlist(tdlist);
	
	for (td = tdlist; td != NULL; td = td->td_next) {
		if (td->td_type != NULL) {
			fprintf(fp, "%*stypedef %s;\n", level * ISPACES, "",
				type_to_decl(td->td_name, td->td_type,
					     CL_TYPEDEF, language, FALSE));
		}
		else {
			dump_tdlist(fp, level, language, td->td_sublist);
		}
	}

	reverse_tdlist(tdlist);
}

static bool
dump_aggr(fp, level, language, ae)
FILE *fp;
int level;
language_t language;
aggr_or_enum_def_t *ae;
{
	fprintf(fp, "%*s%s", level * ISPACES, "",
		type_to_decl("", ae->ae_type, CL_AUTO, language, FALSE));

	if (ae->ae_is_complete == AE_COMPLETE) {
		typecode_t typecode;

		fputs("{\n", fp);
		++level;

		typecode = ae->ae_type->ty_code;
		if (typecode == TY_ENUM || typecode == TY_U_ENUM) {
			enum_member_t *em, *members;

			members = reverse_enumlist(ae->ae_enum_members);
		
			for (em = members; em != NULL; em = em->em_next) {
				fprintf(fp, "%*s %s = %ld,\n",
					level * ISPACES, "",
					em->em_name, em->em_val);
			}

			reverse_enumlist(members);
		}
		else {
			dump_varlist(fp, level, ae->ae_aggr_members);
		}

		--level;
		fprintf(fp, "%*s}", level * ISPACES, "");
	}

	fputs(";\n\n", fp);
	
	return !ferror(fp);
}

static const char *
get_fil_name(fil)
fil_t *fil;
{
	static char *last = NULL;
	const char *name;
	
	if (fil == NULL)
		return "";
	name = fil->fi_name;

	if (*name == '/' || fil->fi_path_hint == NULL)
		return name;

	if (last != NULL)
		free(last);

	last = strf("%s%s", fil->fi_path_hint, name);

	return last;
}

static bool
dump_file_syms(fp, fil)
FILE *fp;
fil_t *fil;
{
	block_t *bl;
	
	/*  Ensure types and vars are loaded.
	 */
	st_get_fi_vars(fil);
	bl = fil->fi_block;
	
	fprintf(fp, "%s ", get_fil_name(fil));

	if (bl->bl_next != NULL)
		panic("fil next bl botch");
	
	if (!dump_block(fp, 0, fil->fi_language, bl, fil->fi_funclist))
		return FALSE;

	return fflush(fp) != EOF;
}

static bool
dump_block(fp, level, language, parbl, funclist)
FILE *fp;
int level;
language_t language;
block_t *parbl;
funclist_t *funclist;
{
	block_t *bl;
	funclist_t *fl;
	
	fprintf(fp, "%*s{\n", level * ISPACES, "");
	++level;

	if (parbl->bl_aggr_or_enum_defs != NULL)
		fputc('\n', fp);
	if (!dump_aelist(fp, level, language, parbl->bl_aggr_or_enum_defs))
		return FALSE;

	dump_tdlist(fp, level, language, parbl->bl_typedefs);
	if (parbl->bl_typedefs != NULL)
		fputc('\n', fp);
	
	dump_varlist(fp, level, parbl->bl_vars);
	if (parbl->bl_vars != NULL)
		fputc('\n', fp);
	
	for (bl = parbl->bl_blocks; bl != NULL; bl = bl->bl_next) {
		if (bl->bl_aggr_or_enum_defs != NULL || bl->bl_vars != NULL ||
		    bl->bl_typedefs != NULL || bl->bl_blocks != NULL) {
			if (!dump_block(fp, level, language, bl,
					(funclist_t *)NULL)) {
				return FALSE;
			}
		}
	}

	for (fl = funclist; fl != NULL; fl = fl->fl_next) {
		if (!dump_func_syms(fp, level, fl->fl_func))
			return FALSE;
	}
	
	--level;
	fprintf(fp, "%*s}\n\n", level * ISPACES, "");
	
	return !ferror(fp);
}

static bool
dump_func_syms(fp, level, f)
FILE *fp;
int level;
func_t *f;
{
	block_t *bl;
	
	bl = FU_BLOCKS(f);

	if (bl != NULL && bl->bl_next != NULL)
		panic("func next bl botch");

	fprintf(fp, "%*s", level * ISPACES, "");
	
	if (level == 0)
		fprintf(fp, "%s:", get_fil_name(f->fu_fil));
	
	fprintf(fp, "%s%s()",
		(f->fu_flags & FU_STATIC) ? "static " : "",
		f->fu_demangled_name);

	if (bl == NULL) {
		fputs("\n\n", fp);
		return TRUE;
	}

	fputc(' ', fp);
	return dump_block(fp, level, f->fu_language, bl, (funclist_t *)NULL);
}

static bool
dump_all_syms(xp, fp)
target_t *xp;
FILE *fp;
{
	int symtab_num, nsymtabs;
	symtab_t *st;
	bool ok;

	st = NULL;
	nsymtabs = 0;
	while (xp_next_symtab(xp, st, TRUE, &st))
		++nsymtabs;

	ok = TRUE;
	symtab_num = 1;
	
	st = NULL;
	while (ok && xp_next_symtab(xp, st, TRUE, &st)) {
		int counter, nfiles;
		fil_t *fil;
		
		nfiles = 0;
		for (fil = st->st_sfiles; fil != NULL; fil = fil->fi_next)
			++nfiles;

		counter = 1;
		for (fil = st->st_sfiles; fil != NULL; fil = fil->fi_next) {
			errf("\bSymtab %d of %d: File %d of %d: %s",
			     symtab_num, nsymtabs,
			     counter++, nfiles, fil->fi_name);

			if (!dump_file_syms(fp, fil)) {
				ok = FALSE;
				break;
			}
		}

		++symtab_num;
	}

	return ok;
}

void
debug_dump_symbols(xp, name)
target_t *xp;
const char *name;
{
	static const char what[] = "output file";
	fil_t *fil;
	func_t *f;
	FILE *fp;
	const char *opath;
	bool ok, overwrite;
	
	if (!get_debug_output_path(name, (const char *)NULL,
				   &name, &opath, &overwrite)) {
		return;
	}
 
	if (!get_file_or_func(name, &fil, &f))
		return;

	if (!fopen_new_file(what, opath, overwrite, &fp))
		return;
	
	if (fil != NULL) {
		ok = dump_file_syms(fp, fil);
	}
	else if (f != NULL) {
		ok = dump_func_syms(fp, 0, f);
	}
	else {
		ok = dump_all_syms(xp, fp);
	}

	fclose_new_file(what, name, ok, fp);
}

/*  Load all symbol table information.  This is used for debugging.
 */
void
debug_load_symbols(xp, name)
target_t *xp;
const char *name;
{
	symtab_t *st;
	fil_t *fil;
	func_t *f;
	int symtab_num, nsymtabs;

	if (!get_file_or_func(name, &fil, &f))
		return;

	if (fil != NULL) {
		errf("\bLoading symbols of file %s", fil->fi_name);
		st_get_fi_vars(fil);
		errf("\b");
		return;
	}

	if (f != NULL) {
		errf("\bLoading symbols of function %s", f->fu_demangled_name);
		load_func_info(f);
		errf("\b");
		return;
	}

	st = NULL;
	nsymtabs = 0;
	while (xp_next_symtab(xp, st, TRUE, &st))
		++nsymtabs;
	
	st = NULL;
	symtab_num = 1;
	while (xp_next_symtab(xp, st, TRUE, &st)) {
		int counter, nfuncs, nfiles;
		
		nfuncs = 0;
		for (f = st->st_funclist; f != NULL; f = f->fu_next)
			++nfuncs;

		counter = 1;
		for (f = st->st_funclist; f != NULL; f = f->fu_next) {
			errf("\bSymtab %d of %d: Func %d of %d: %s",
			     symtab_num, nsymtabs,
			     counter++, nfuncs, f->fu_demangled_name);
			
			load_func_info(f);
		}
		
		nfiles = 0;
		for (fil = st->st_sfiles; fil != NULL; fil = fil->fi_next)
			++nfiles;

		counter = 1;
		for (fil = st->st_sfiles; fil != NULL; fil = fil->fi_next) {
			errf("\bSymtab %d of %d: File %d of %d: %s",
			     symtab_num, nsymtabs,
			     counter++, nfiles, fil->fi_name);
			
			st_get_fi_vars(fil);
		}

		++symtab_num;
	}

	errf("\bEntire symbol table loaded");
}

#if WANT_DEBUG
/*
 * (IDE)
 */

void
dump_header(const char *s)
{
    if (s)
	fprintf(stderr, "----- %s -----\n", s);
    else
	fprintf(stderr, "----- %s -----\n", "<NULL>");
}

void
dump_trailer()
{
    fprintf(stderr, "--------------------------------------------------\n");
}

void
dump_enum_member_t(em, level)
enum_member_t *em;
int level;
{
    if (em == NULL) {
	fprintf(stderr, "%*.*senum  \t[NULL]\n",
		level*4, level*4, "....");
	return;
    }
    while (em != NULL) {
	fprintf(stderr, "%*.*senum  \t[%p]:  %ld  '%s'\n",
		level*4, level*4, "....",
		em, em->em_val, em->em_name);
	em = em->em_next;
    }
}

void
dump_aggr_or_enum_def_t(ae, ty_code, level, follow)
aggr_or_enum_def_t *ae;
int ty_code;
int level;
bool follow;
{
    if (ae == NULL) {
	fprintf(stderr, "%*.*saggr  \t[NULL]\n",
		level*4, level*4, "....");
	return;
    }
    fprintf(stderr, "%*.*saggr  \t[0x%08lx]:  size %3d align %2d  %s  %s\n",
	    level*4, level*4, "....",
	    (long)ae, ae->ae_size, ae->ae_alignment,
	    ae->ae_tag ? ae->ae_tag : "(no tag)",
	    (ae->ae_is_complete == AE_COMPLETE) ? "complete" : "incomplete");
    if ((ty_code == TY_ENUM) ||(ty_code == TY_U_ENUM))
	dump_enum_member_t(ae->ae_enum_members, level+1);
    else if ((ty_code == TY_STRUCT) || (ty_code == TY_U_STRUCT))
	dump_var_t(ae->ae_aggr_members, level+1, TRUE);
    else if ((ty_code == TY_UNION) || (ty_code == TY_U_UNION))
	dump_var_t(ae->ae_aggr_members, level+1, TRUE);
}

void
dump_type_t(t, level, follow)
type_t *t;
int level;
bool follow;
{
    char *s = NULL;
    if (t == NULL) {
	fprintf(stderr, "%*.*stype_t\t[NULL]\n",
		level*4, level*4, "....");
	return;
    }
    fprintf(stderr, "%*.*stype_t\t[%p]:  [%p]  code %2d size %3ld",
	    level*4, level*4, "....",
	    t, t->ty_base, t->ty_code, t->ty_size);
    if (t->ty_name)
	fprintf(stderr, " name '%s'", t->ty_name);
    if (t->ty_base == NULL) {
	if (ISDERIV(t->ty_code)) {
	    fprintf(stderr, " base '%s'\n", "*** NOT BASE TYPE ***");
	} else {
	    s = ci_qualified_basetype_name(t);
	    if (s)
		fprintf(stderr, " base '%s'", s);
	    else
		fprintf(stderr, " base '%s'", "*** ??? ***");
	    fprintf(stderr, "\n");
	    if ((t->ty_code == TY_ENUM) || (t->ty_code == TY_U_ENUM)
		    || (t->ty_code == TY_STRUCT) || (t->ty_code == TY_U_STRUCT)
		    || (t->ty_code == TY_UNION) || (t->ty_code == TY_U_UNION))
		dump_aggr_or_enum_def_t(t->ty_aggr_or_enum, t->ty_code, level+1, follow);
	}
    } else {
	s = ci_type_to_decl(t, FALSE);
	if (s)
	    fprintf(stderr, " decl '%s'", s);
	else
	    fprintf(stderr, " decl '%s'", "*** ??? ***");
	fprintf(stderr, "\n");
	if (follow)
	    dump_type_t(t->ty_base, level+1, follow);
    }
    if (s)
	free(s);
}

void
dump_dim_t(dim, level)
dim_t *dim;
int level;
{
    if (dim == NULL) {
	fprintf(stderr, "%*.*sdim_t  \t[NULL]\n",
		level*4, level*4, "....");
	return;
    }
    fprintf(stderr, "%*.*sdim_t  \t[%p]:  %s %ld %s %ld\n",
	    level*4, level*4, "....",
	    dim,
	    dim->di_ltype == DT_CONSTANT ? "" : "variable",
	    dim->di_ltype == DT_CONSTANT ? dim->di_low : 0,
	    dim->di_htype == DT_CONSTANT ? "" : "variable",
	    dim->di_htype == DT_CONSTANT ? dim->di_high : 0);
    dump_type_t(dim->di_type, level+1, FALSE);
}

void
dump_typedef_t(td, level, follow)
typedef_t *td;
int level;
bool follow;
{
    if (td == NULL) {
	fprintf(stderr, "%*.*stypedef\t[NULL]\n",
		level*4, level*4, "....");
	return;
    }
    while (td) {
	fprintf(stderr, "%*.*stypedef\t[%p]:  '%s'\n",
		level*4, level*4, "....",
		td, td->td_name);
	dump_type_t(td->td_type, level+1, follow);
	if (!follow)
	    break;
	td = td->td_next;
    }
}

static const char *
st_dt_is_str(dt_is)
dt_is_t dt_is;
{
    switch(dt_is) {
    case DT_IS_TYPE:	return "DT_IS_TYPE    ";
    case DT_IS_TYPEDEF:	return "DT_IS_TYPEDEF ";
    case DT_IS_VAR:	return "DT_IS_VAR     ";
    case DT_IS_RANGE:	return "DT_IS_RANGE   ";
    case DT_IS_BITFIELD:return "DT_IS_BITFIELD";
    }
    return "DT_IS_??";
}

void
dump_dtype_t(dts, level, follow)
dtypes_t *dts;
int level;
bool follow;
{
    dtype_t *dt = dts->dts_first_dt;
    if (dt == NULL) {
	fprintf(stderr, "%*.*sdtype_t\t[NULL]\n",
		level*4, level*4, "....");
	return;
    }
    while (dt) {
	fprintf(stderr, "%*.*sdtype_t\t[%p]:  <%ld>  %s",
		level*4, level*4, "....",
		dt, (long)dt->dt_offset, st_dt_is_str(dt->dt_is));
	if (dt->dt_base_offset != (off_t)0)
	    fprintf(stderr, "  *** INCOMPLETE *** <%ld>\n", (long)dt->dt_base_offset);
	else
	    fprintf(stderr, "\n");
#if 0
	switch (dt->dt_is) {
	case DT_IS_TYPE:    dump_type_t(dt->dt_type, 0, TRUE); break;
	case DT_IS_TYPEDEF: dump_typedef_t(dt->dt_typedef, 0, FALSE); break;
	case DT_IS_VAR:     dump_var_t(dt->dt_var, 0, FALSE); break;
	case DT_IS_RANGE:   dump_dim_t(dt->dt_dim, 0); break;
	}
#else
	dump_type_t(dt->dt_type, level+1, TRUE);
	if (dt->dt_p_type)
	    dump_type_t(*(dt->dt_p_type), level+1, TRUE);
#endif
	if (!follow)
	    break;
	dt = dt->dt_next;
    }
}

static const char *
st_class_str(class)
class_t class;
{
    switch(class) {
    case CL_NOCLASS:	return "CL_NOCLASS";
    case CL_EXTREF:	return "CL_EXTREF ";
    case CL_DECL:	return "CL_DECL   ";
    case CL_AUTO:	return "CL_AUTO   ";
    case CL_EXT:	return "CL_EXT    ";
    case CL_STAT:	return "CL_STAT   ";
    case CL_REG:	return "CL_REG    ";
    case CL_MOS:	return "CL_MOS    ";
    case CL_ARG:	return "CL_ARG    ";
    case CL_REF:	return "CL_REF    ";
    case CL_MOU:	return "CL_MOU    ";
    case CL_MOE:	return "CL_MOE    ";
    case CL_LSTAT:	return "CL_LSTAT  ";
    case CL_FUNC:	return "CL_FUNC   ";
    case CL_LFUNC:	return "CL_LFUNC  ";
    case CL_TYPEDEF:	return "CL_TYPEDEF";
    case CL_TAGNAME:	return "CL_TAGNAME";
    }
    return "CL_?????";
}

void
dump_var_t(v, level, follow)
var_t *v;
int level;
bool follow;
{
    if (v == NULL) {
	fprintf(stderr, "%*.*svar_t\t[NULL]\n",
		level*4, level*4, "....");
	return;
    }
    while (v) {
	fprintf(stderr, "%*.*svar_t\t[%p]:  0x%08lx  %04x  %s\t%s\n",
		level*4, level*4, "....",
		v, v->va_addr, v->va_flags,
		st_class_str(v->va_class), v->va_name);
	dump_type_t(v->va_type, level+1, FALSE);
	if (!follow)
	    break;
	v = v->va_next;
    }
}

void
dump_func_t(f, follow)
func_t *f;
bool follow;
{
    if (f == NULL) {
	fprintf(stderr, "func_t\t[NULL]\n");
	return;
    }
    while (f) {
	fprintf(stderr, "func_t\t[%p]:  fil [%p] addr 0x%08lx %s '%s'\n",
		f, f->fu_fil, f->fu_addr,
		(strcmp(f->fu_name,f->fu_demangled_name) ? "*" : " "),
		(f->fu_demangled_name ? f->fu_demangled_name : f->fu_name));
	if (!follow)
	    break;
	f = f->fu_next;
    }
}

void
dump_lno_t(lno, follow)
lno_t *lno;
bool follow;
{
    if (lno == NULL) {
	fprintf(stderr, "lno_t\t[NULL]\n");
	return;
    }
    while (lno) {
	fprintf(stderr, "lno_t\t[%p]:  fil [%p] addr 0x%08lx  %d\n",
		lno, lno->ln_fil, lno->ln_addr, lno->ln_num);
	if (!follow)
	    break;
	lno = lno->ln_next;
    }
}

void
dump_block_t(bl, level, follow)
block_t *bl;
int level;
bool follow;
{
    if (bl == NULL) {
	fprintf(stderr, "%*.*sblock_t\t[NULL]\n",
		level*4, level*4, "....");
	return;
    }
    while (bl) {
	fprintf(stderr, "%*.*sblock_t [%p]:  [%p]%*.*s start %d\n",
		level*4, level*4, "....",
		bl, bl->bl_parent,
		level*4, level*4, "    ",
		bl->bl_start_lnum);
	if (1)
	    dump_var_t(bl->bl_vars, level+1, TRUE);
	if (follow && bl->bl_blocks)
	    dump_block_t(bl->bl_blocks, level+1, follow);
	fprintf(stderr, "%*.*sblock_t [%p]:  [%p]%*.*s   end %d\n",
		level*4, level*4, "....",
		bl, bl->bl_parent,
		level*4, level*4, "    ",
		bl->bl_end_lnum);
	if (!follow)
	    break;
	bl = bl->bl_next;
    }
}

void
dump_funclist_t(fl, follow)
funclist_t *fl;
bool follow;
{
    if (fl == NULL) {
	fprintf(stderr, "funclist\t[NULL]\n");
	return;
    }
/*
    fprintf(stderr, "funclist_t\t[0x%08lx]\n",
	    (long)funclist);
*/
    while (fl) {
	dump_func_t(fl->fl_func, FALSE);
	if (!follow)
	    break;
	fl = fl->fl_next;
    }
}

void
dump_stf_t(stf, follow)
stf_t *stf;
bool follow;
{
    if (stf == NULL) {
	fprintf(stderr, "stf_t\t[NULL]\n");
	return;
    }
    fprintf(stderr, "stf_t\t[0x%08lx]:  %d\n",
	    (long)stf, stf->stf_mapsize);
}

void
dump_fil_t(fil, details, follow)
fil_t *fil;
bool details;
bool follow;
{
    if (fil == NULL) {
	fprintf(stderr, "fil_t\t[NULL]\n");
	return;
    }
    fprintf(stderr, "fil_t\t[%p]:  symtab [%p] block [%p] '%s'\n",
	    fil, fil->fi_symtab, fil->fi_block,
	    fil->fi_name);
    if (details)
	dump_funclist_t(fil->fi_funclist, TRUE);
    if (follow) {
	if (fil->fi_next)
	    dump_fil_t(fil->fi_next, details, follow);
	else
	    fprintf(stderr, "fil_t\t[%p]: end of list\n", fil);
    } else {
	fprintf(stderr, "fil_t\t[%p]: no follow\n", fil);
    }
}

void
dump_symtab_t(st, follow)
symtab_t *st;
bool follow;
{
    if (st == NULL) {
	fprintf(stderr, "symtab_t\t[NULL]\n");
	return;
    }
    fprintf(stderr, "symtab_t\t[0x%08lx]:  path '%s'\n",
	    (long)st, st->st_path);
    dump_fil_t(st->st_sfiles, TRUE, TRUE);
}

#endif

