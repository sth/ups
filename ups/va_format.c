/* va_format.c - save and restore the formats of variables */

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


/* @(#)va_format.c	1.3 04 Jun 1995 (UKC) */
char ups_va_format_c_rcsid[] = "$Id$";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <local/ukcprog.h>
#include <local/wn.h>
#include <local/obj/obj.h>

#include "ups.h"
#include "symtab.h"
#include "va.h"
#include "va_priv.h"
#include "st.h"
#include "ui.h"
#include "objtypes.h"
#include "obj_stack.h"

#define DEFAULT_ILEVEL	(-1)

typedef struct Format {
	fil_t *fil;
	func_t *func;
	bool is_struct_member;
	const char *tag;
	const char *name;
	vformat_t format;
	int ilevel;
	bool is_hint;
	struct Format *next;
} Format;

enum {
	SC_ANY_MATCH,		/* match on null pointer or `*' wildcard */
	SC_EXACT_MATCH		/* non-glob match */
};

static void push_format_entry PROTO((fil_t *fil, func_t *f,
				     bool is_struct_member,
				     const char *tag, const char *name, 
                                     vformat_t format, int ilevel,
				     bool is_hint));
static void dump_format PROTO((FILE *fp, Format *f));
static void get_var_info PROTO((dvar_t *dv, fil_t **p_fil, func_t **p_func, 
                                const char **p_tag, bool *p_is_struct_member));
static bool matchstr PROTO((const char *pat, const char *str, bool exact,
			    int *p_score));
static bool matchptr PROTO((bool is_null, bool same, bool exact, int *p_score));
static Format *reverse_list PROTO((Format *list));
static bool lookup_format PROTO((fil_t *fil, func_t *func, const char *tag, 
                                 bool is_struct_member, const char *name, 
                                 bool exact, Format **p_ft));

static Format *Format_list = NULL;


int
handle_format_command(cmd, args, nargs, is_hint)
const char *cmd;
char **args;
int nargs;
bool is_hint;
{
	const char *format_str;
	char *varstr, *tag, *varname, *filestr, *funcstr, *ilevel_str;
	int ilevel;
	vformat_t format;
	bool is_struct_member;
	fil_t *fil;
	func_t *f, *f1;
	
	if (nargs != 3 && nargs != 4) {
		errf("Usage: %s file[:func] [[tag].]varname format [indir-level]", cmd);
		return -1;
	}
	filestr = args[0];
	varstr = args[1];
	format_str = args[2];
	ilevel_str = args[3];

	funcstr = strchr(filestr, ':');

	if (strcmp(filestr, "*") == 0) {
		funcstr = filestr = NULL;
	}
	else if (funcstr != NULL) {
		*funcstr++ = '\0';
	}
	else if (strchr(filestr, '.') == NULL) {
		funcstr = filestr;
		filestr = NULL;
	}

	if (filestr == NULL) {
		fil = NULL;
	}
	else {
		if (*filestr == '\0') {
			errf("Zero length file name");
			return -1;
		}

		if ((fil = name_to_fil(filestr)) == NULL) {
			errf("Unknown source file name `%s'", filestr);
			return -1;
		}
	}

	if (funcstr == NULL) {
		f = NULL;
	}
	else {
		if (*funcstr == '\0') {
			errf("Zero length function name");
			return -1;
		}

		if (fil != NULL) {
			if ((f = name_and_fil_to_func(funcstr, fil, FALSE)) == NULL) {
				if (find_func_by_name(funcstr, &f, &f1, FALSE) != 0)
					return -1;;
			}
		}
		else {
			if (find_func_by_name(funcstr, &f, &f1, FALSE) != 0)
				return -1;
		}
	}
		
	if (!string_to_format(format_str, &format))
		return -1;

	if (ilevel_str == NULL) {
		ilevel = DEFAULT_ILEVEL;
	}
	else if (!get_num(&ilevel_str, "indirection level", &ilevel)) {
		return -1;
	}

	if ((varname = strchr(varstr, '.')) == NULL) {
		tag = NULL;
		varname = varstr;
		is_struct_member = FALSE;
	}
	else {
		*varname++ = '\0';
	
		if (*varname == '\0') {
			errf("Zero length variable name");
			return -1;
		}

		tag = (*varstr != '\0') ? strsave(varstr) : NULL;

		is_struct_member = TRUE;
	}
	
	push_format_entry(fil, f, is_struct_member, tag, strsave(varname),
			  format, ilevel, is_hint);
	return 0;
}

static Format *
reverse_list(list)
Format *list;
{
	Format *ft, *next, *newlist;

	newlist = NULL;

	for (ft = list; ft != NULL; ft = next) {
		next = ft->next;
		ft->next = newlist;
		newlist = ft;
	}

	return newlist;
}

int
write_format_state_to_file(path, fp)
const char *path;
FILE *fp;
{
	Format *ft;
	
	if (Format_list != NULL)
		fputc('\n', fp);

	Format_list = reverse_list(Format_list);
	
	for (ft = Format_list; ft != NULL; ft = ft->next) {
		dump_format(fp, ft);
		if (ferror(fp))
			return -1;
	}

	Format_list = reverse_list(Format_list);
	
	return 0;
}

static void
dump_format(fp, ft)
FILE *fp;
Format *ft;
{
	if (!ft->is_hint)
		return;
	
	fputs("format-hint ", fp);

	/* RGA enclose demangled name in double quotes, to allow for spaces
	   in C++ names.
	   3.37: use unquoted mangled name so overloaded methods are unique,
	   followed by demangled name in quotes for readability
	   */
	if (ft->fil != NULL) {
		fputs(ft->fil->fi_name, fp);

		if (ft->func != NULL)
			fprintf(fp, ":%s \"%s\"", ft->func->fu_name,
				ft->func->fu_demangled_name);
	}
	else {
	  if (ft->func != NULL)
	    fprintf(fp, "%s \"%s\"", ft->func->fu_name,
		    ft->func->fu_demangled_name);
	  else
	    fprintf(fp, "*");
	}
	fputc(' ', fp);

	if (ft->is_struct_member) {
		if (ft->tag != NULL)
			fputs(ft->tag, fp);
		fputc('.', fp);
	}

	fputs(ft->name, fp);
	fputc(' ', fp);

	fputs(format_to_string(ft->format), fp);

	if (ft->ilevel != DEFAULT_ILEVEL)
		fprintf(fp, " %d", ft->ilevel);

	fputc('\n', fp);
}

static void
push_format_entry(fil, f, is_struct_member, tag, name, format, ilevel, is_hint)
fil_t *fil;
func_t *f;
bool is_struct_member;
const char *tag, *name;
vformat_t format;
int ilevel;
bool is_hint;
{
	Format *ft;

	ft = (Format *)e_malloc(sizeof(Format));
	ft->fil = fil;
	ft->func = f;
	ft->is_struct_member = is_struct_member;
	ft->tag = tag;
	ft->name = name;
	ft->format = format;
	ft->ilevel = ilevel;
	ft->is_hint = is_hint;
	ft->next = Format_list;
	Format_list = ft;
}

static bool
matchstr(pat, str, exact, p_score)
const char *pat, *str;
bool exact;
int *p_score;
{
	int score;
	
	if (pat == NULL) {
		if (exact && str != NULL)
			return FALSE;
			
		score = SC_ANY_MATCH;
	}
	else if (str != NULL && strcmp(pat, str) == 0) {
		score = SC_EXACT_MATCH;
	}
	else {
		return FALSE;
	}

	*p_score = score;
	return TRUE;
}

static bool
matchptr(is_null, same, exact, p_score)
bool is_null, same, exact;
int *p_score;
{
	int score;

	if (same) {
		score = SC_EXACT_MATCH;
	}
	else if (is_null) {
		if (exact)
		       return FALSE;
		
		score = SC_ANY_MATCH;
	}
	else {
		return FALSE;
	}

	*p_score = score;
	return TRUE;
}

static bool
lookup_format(fil, func, tag, is_struct_member, name, exact, p_ft)
fil_t *fil;
func_t *func;
const char *tag;
bool is_struct_member;
const char *name;
bool exact;
Format **p_ft;
{
	Format *ft;
	int bestscore;

	*p_ft = NULL;
	bestscore = 0;

	for (ft = Format_list; ft != NULL; ft = ft->next) {
		int tagscore, filscore, funcscore, namescore;
		
		if (is_struct_member == ft->is_struct_member &&
		    matchstr(ft->tag, tag, exact, &tagscore) &&
		    matchptr(ft->func == NULL, ft->func == func, exact,
			     &funcscore) &&
		    matchptr(ft->fil == NULL, ft->fil == fil, exact,
			     &filscore) &&
		    matchstr(ft->name, name, exact, &namescore)) {
			int score;
			
			score = tagscore + funcscore + filscore + namescore;

			if (score > bestscore) {
				*p_ft = ft;
				bestscore = score;
			}
		}
	}

	return *p_ft != NULL;
}
	
bool
maybe_restore_format(dv)
dvar_t *dv;
{
	Format *ft;
	fil_t *fil;
	func_t *func;
	const char *tag;
	bool is_struct_member;
	short orig_ilevel;
	vformat_t orig_format;
	ilist_t *orig_ilist;
	
	get_var_info(dv, &fil, &func, &tag, &is_struct_member);
	
	if (!lookup_format(fil, func, tag, is_struct_member,
			   dv->dv_var->va_name, FALSE, &ft))
		return FALSE;

	orig_ilevel = dv->dv_ilevel;
	orig_format = dv->dv_format;
	orig_ilist = dv->dv_ilist;
	if (ft->ilevel != DEFAULT_ILEVEL) {
		int i;
		
		for (i = dv->dv_ilevel; i < ft->ilevel; ++i)
			change_dv_level(dv, CL_UP);

		for (i = ft->ilevel; i < dv->dv_ilevel; ++i)
			change_dv_level(dv, CL_DOWN);
	}

	if (ft->format != DF_NONE) {
		dv->dv_format = ft->format;
	}
	else {
		type_t *type;

		type = get_type_at_level(dv->dv_var, dv->dv_ilevel);
		dv->dv_format = default_format(dv->dv_var->va_type, type);
	}

	redo_decl(dv);
	if (dv->dv_flags & DVF_BAD_FORMAT_HINT)
	{
	  dv->dv_flags &= ~DVF_BAD_FORMAT_HINT;
	  dv->dv_ilevel = orig_ilevel;
	  dv->dv_format = orig_format;
	  dv->dv_ilist = orig_ilist;

	  /* format hint was bad, so remove the hint */
	  ft->format = orig_format;
	  ft->ilevel = orig_ilevel;
	  ft->is_hint = FALSE;
	  redo_decl(dv);
	}

	return TRUE;
}

static void
get_var_info(dv, p_fil, p_func, p_tag, p_is_struct_member)
dvar_t *dv;
fil_t **p_fil;
func_t **p_func;
const char **p_tag;
bool *p_is_struct_member;
{
	bool is_struct_member;
	const char *tag;
	fil_t *fil;
	func_t *func;
	objid_t par;
	int partype;
	taddr_t junk;
			
	is_struct_member = (dv->dv_var->va_class == CL_MOU ||
			    dv->dv_var->va_class == CL_MOS);

	/*  Structure member formats probably want to be applied everywhere.
	 */
	if (is_struct_member) {
		type_t *type;
		
		fil = NULL;
		func = NULL;
		
		par = get_code((objid_t)dv, OBJ_PARENT);
		
		switch (ups_get_object_type(par)) {
		case OT_VAR:
			type = get_basetype(((dvar_t *)par)->dv_var->va_type);
			break;
		case OT_EXPR:
			type = get_basetype(get_expr_type(par));
			break;
		default:
			panic("bad partype in nfc");
			type = NULL; /* to satisfy gcc */
		}

		switch (type->ty_code) {
		case TY_STRUCT:
		case TY_UNION:
		case TY_U_STRUCT:
		case TY_U_UNION:
			break;
		default:
			panic("expected aggr basetype in nfc");
		}

		if (type->ty_typedef != NULL)
			tag = type->ty_typedef->td_name;
		else
			tag = type->ty_aggr_or_enum->ae_tag;
	}
	else {
		par = (objid_t)dv;
		tag = NULL;
	}
	
	do {
		par = get_code(par, OBJ_PARENT);
		partype = ups_get_object_type(par);
	} while (partype == OT_VAR || partype == OT_EXPR);
	
	switch (partype) {
	case OT_FUNC:
	case OT_BLOCK:
		func = get_stack_func(par, &junk, &junk);
		fil = func->fu_fil;
		break;
	case OT_SFILE:
		fil = (fil_t *)par;
		func = NULL;
		break;
	case OT_GLOBALS:
		fil = NULL;
		func = NULL;
		break;
	case OT_CBLOCK:
		func = NULL;
		fil = get_cblock_fil((common_block_t *)par);
		break;
	default:
		panic("unknown par type in nfc");
		fil = NULL;	/* to satisfy gcc */
		func = NULL;	/* to satisfy gcc */
	}

	*p_fil = fil;
	*p_func = func;
	*p_tag = tag;
	*p_is_struct_member = is_struct_member;
}

void
note_format_change(dv)
dvar_t *dv;
{
	Format *ft;
	bool is_struct_member;
	const char *tag;
	fil_t *fil;
	func_t *func;

	get_var_info(dv, &fil, &func, &tag, &is_struct_member);
	
	/*  Don't record the source file for a local, as the function is
	 *  probably sufficient context.  This also means the user can
	 *  move the function to a different source file without problems.
	 */
	if (func != NULL)
		fil = NULL;

	/*  Don't record the source file or function for a struct member.
	 */
	if (is_struct_member) {
		func = NULL;
		fil = NULL;
	}
	
	if (lookup_format(fil, func, tag, is_struct_member, dv->dv_var->va_name,
			  TRUE, &ft)) {
		if (ft->is_hint) {
			ft->format = dv->dv_format;
			ft->ilevel = dv->dv_ilevel;
		}
		return;
	}

	push_format_entry(fil, func, is_struct_member, tag,
			  dv->dv_var->va_name,
			  dv->dv_format, dv->dv_ilevel, TRUE);
}


bool
string_to_format(str, p_format)
const char *str;
vformat_t *p_format;
{
	static struct {
		const char *str;
		vformat_t format;
	} tab[] = {
		{ "default",		DF_NONE		},
		{ "signed_decimal",	DF_SDEC		},
		{ "unsigned_decimal",	DF_UDEC		},
		{ "signed_octal",	DF_SOCT		},
		{ "unsigned_octal",	DF_UOCT		},
		{ "signed_hex",		DF_SHEX		},
		{ "unsigned_hex",	DF_UHEX		},
		{ "ascii",		DF_ASCII	},
		{ "string",		DF_STRING	},
		{ "unsigned_binary",	DF_UBIN		}
	};
	int i;

	for (i = 0; i < sizeof tab / sizeof *tab; ++i) {
		if (strcmp(tab[i].str, str) == 0) {
			*p_format = tab[i].format;
			return TRUE;
		}
	}

	errf("Unknown format `%s'", str);
	return FALSE;
}

const char *
format_to_string(format)
vformat_t format;
{
	switch (format) {
	case DF_NONE:	return "default";
	case DF_SDEC:	return "signed_decimal";
	case DF_UDEC:	return "unsigned_decimal";
	case DF_SOCT:	return "signed_octal";
	case DF_UOCT:	return "unsigned_octal";
	case DF_SHEX:	return "signed_hex";
	case DF_UHEX:	return "unsigned_hex";
	case DF_ASCII:	return "ascii";
	case DF_STRING:	return "string";
	case DF_UBIN:	return "unsigned_binary";
	}

	panic("bad format in fts");
	return NULL;		/* to satisfy gcc */
}
void
free_ftlist()
{
  Format *ft, *ft_next;
  
  for (ft = Format_list; ft != NULL;)
  {
    ft_next = ft->next;
    free(ft);
    ft = ft_next;
  }
  Format_list = NULL;
}

void
update_format_state(oldst,newst)
symtab_t *oldst;
symtab_t *newst;
{
   Format **ftp = &Format_list;

   while (*ftp) {
      Format *ft = *ftp;

      if (ft->func && ft->func->fu_symtab == oldst) {
	 func_t *f, *f1;
	 fil_t *fil;
	 
	 if ((fil = name_and_st_to_fil(ft->func->fu_fil->fi_name, newst)) == NULL ||
	     (f = name_and_fil_to_func(ft->func->fu_name, fil, FALSE)) == NULL) {
	    if (find_func_by_name(ft->func->fu_name, &f, &f1, FALSE) != 0) {
	       errf("Function %s no longer appears to exist",
		    ft->func->fu_demangled_name);
	       free(ft);
	       *ftp = ft->next;
	       continue;
	    }
	 }

	 ft->func = f;
      }

      if (ft->fil && ft->fil->fi_symtab == oldst) {
	 fil_t *fil;
	 
	 if ((fil = name_and_st_to_fil(ft->fil->fi_name, newst)) == NULL) {
            errf("File %s no longer appears to exist", ft->fil->fi_name);
            free(ft);
            *ftp = ft->next;
            continue;
	 }

	 ft->fil = fil;
      }

      ftp = &(*ftp)->next;
   }

   return;
}
