/* st_fmap.c - functions for mapping names and addresses to func_t pointers */

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


/* @(#)st_fmap.c	1.22 04 Jun 1995 (UKC) */
char ups_st_fmap_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <sys/types.h>
#include <string.h>

#include <local/ukcprog.h>
#include <mtrprog/genmergesort.h>
#include <mtrprog/alloc.h>
#include <mtrprog/strcache.h>

#include "ups.h"
#include "symtab.h"
#include "st.h"

/*  The sorted array of func_t pointers.
 *  Used for binary chop search for function by address.
 */
struct functab_s {
	taddr_t addr_base;	/* First valid address of segment */
	taddr_t addr_lim;	/* One past last address of segment */

	func_t **tab;		/* Sorted table of functions */
	int nfuncs;		/* #Funcs in this table */
};

static void adjust_lstat_addr PROTO((var_t *v, char *c_delta));
static func_t *sort_functab PROTO((alloc_pool_t *ap, functab_t *functab,
						func_t *flist, int flist_len));

GENERIC_MERGE_SORT(static,sortflist,func_t,fu_next)

ALLOC_NEW_FREELIST(extern,funclist_t,flist,fl_next)


/*  Comparison function for the sort by address.  The list may contain
 *  duplicate entries for a given function (this comes from the dbx
 *  type entry in the symbol table and the linker N_TEXT entry).
 *  These entries are deleted after the sort.
 *
 *  Do the comparison so that entries with symbol table information
 *  appear before entries without).
 *
 */
int
addrcmp(f1, f2)
func_t *f1, *f2;
{
	if (f1->fu_addr != f2->fu_addr)
		return (f1->fu_addr < f2->fu_addr) ? -1 : 1;
	
	if ((f1->fu_flags & FU_NOSYM) != (f2->fu_flags & FU_NOSYM))
		return ((f1->fu_flags & FU_NOSYM) == 0) ? -1 : 1;

	/*  Put weak symbols first, as these have the normal names (e.g.
	 *  `write' rather than `_write').
	 */
	if ((f1->fu_flags & FU_WEAK) != (f2->fu_flags & FU_WEAK))
		return ((f1->fu_flags & FU_WEAK) != 0) ? -1 : 1;

	/*  If two functions have the same address and FU_NOSYM flag, put global
	 *  functions ahead of static ones.  This ordering is used because the
	 *  gdb code on IRIX emits duplicate entries for functions with one
	 *  global and the other static, and the global one has the line number
	 *  information.  This kludgery can go away once we are using
	 *  lookup_symbol() like we ought to.
	 */
	if ((f1->fu_flags & FU_STATIC) != (f2->fu_flags & FU_STATIC))
		return ((f1->fu_flags & FU_STATIC) == 0) ? -1 : 1;
	
	return 0;
}

/*  Create a functab structure representing the list of functions flist.
 *  Flist is unsorted (actually in symbol table order).
 */
void
make_funcinfo(ap, flist, flist_len, addr_base, addr_lim, addr_offset,
	      p_funclist, p_functab)
alloc_pool_t *ap;
func_t *flist;
int flist_len;
taddr_t addr_base, addr_lim;
long addr_offset;
func_t **p_funclist;
functab_t **p_functab;
{
	functab_t *functab;
	func_t *f;

	for (f = flist; f != NULL; f = f->fu_next)
		f->fu_addr += addr_offset;
	
	functab = (functab_t *) alloc(ap, sizeof(functab_t));

	functab->addr_base = addr_base;
	functab->addr_lim = addr_lim;

	*p_funclist = sort_functab(ap, functab, flist, flist_len);
	*p_functab = functab;
}

void
delete_duplicate_funcs(fil)
fil_t *fil;
{
	funclist_t *fl, first, *last;

	last = &first;
	for (fl = fil->fi_funclist; fl != NULL; fl = fl->fl_next) {
		if ((fl->fl_func->fu_flags & FU_DUPLICATE) == 0) {
			last->fl_next = fl;
			last = fl;
		}
	}
	last->fl_next = NULL;

	fil->fi_funclist = first.fl_next;
}

/*  Create a table of functions sorted by address in functab->tab.
 *  We eliminate duplicate function entries.
 */
static func_t *
sort_functab(ap, functab, flist, flist_len)
alloc_pool_t *ap;
functab_t *functab;
func_t *flist;
int flist_len;
{
	func_t *f, *next, *high_func;
	func_t **ft, **ftab;
	int nfuncs;

	flist = sortflist(flist, flist_len, addrcmp);

	nfuncs = 0;

	for (f = flist; f != NULL; f = next) {
		next = f->fu_next;

		while (next != NULL && f->fu_addr == next->fu_addr) {
			if ((next->fu_flags & FU_STATIC) == 0)
				f->fu_flags &= ~FU_STATIC;

			next->fu_flags |= FU_DUPLICATE;
			
			next = next->fu_next;
			f->fu_next = next;
		}

		++nfuncs;
	}

	ft = ftab = (func_t **)alloc(ap, (nfuncs + 1) * sizeof(func_t *));

	for (f = flist; f != NULL; f = f->fu_next)
		*ft++ = f;

	high_func = (func_t *)alloc(ap, sizeof(func_t));
	high_func->fu_addr = functab->addr_lim;
	high_func->fu_name = "[end text marker]";
	high_func->fu_demangled_name = high_func->fu_name;
	ftab[nfuncs] = high_func;

	functab->tab = ftab;
	functab->nfuncs = nfuncs;

	return flist;
}

static void
adjust_lstat_addr(v, c_delta)
var_t *v;
char *c_delta;
{
	if (v->va_class == CL_LSTAT)
		v->va_addr += (long)c_delta;
}

/*  Adjust the segment base functab by delta.  This involves changing the
 *  function and line number addresses for all functions in the table.
 *  We also have to change the offsets of any static local variables.
 *
 *  This is called when a shared library's base address changes -
 *  see change_base_address() in st_shlib.c.
 */
void
adjust_functab_text_addr_base(functab, funclist, delta)
functab_t *functab;
func_t *funclist;
long delta;
{
	func_t *f;
	lno_t *lno;

	for (f = funclist; f != NULL; f = f->fu_next) {
		f->fu_addr += delta;

		if (f->fu_flags & FU_DONE_LNOS) {
			for (lno = FU_LNOS(f); lno != NULL; lno = lno->ln_next)
				lno->ln_addr += delta;
		}

		if (f->fu_flags & FU_DONE_BLOCKS) {
			iterate_over_vars_of_block(FU_BLOCKS(f),
						   adjust_lstat_addr,
						   (char *)delta);
		}
	}

	functab->addr_base += delta;
	functab->addr_lim += delta;

	/* from Daniel Quinlan (danq@jspc.colorado.edu) */
        functab->tab[functab->nfuncs]->fu_addr += delta ; 
}

/*  Return the highest text address that is still within function f.
 *  Used for getting the last text line of a function. See trun_j.c.
 *
 *  The so_get_addr_lim function pointer points to this function for some
 *  but not all target types.  Currently a.out targets use this and C
 *  interpreter targets do not.
 *
 *  BUG: this returns a way too high value for the last function in
 *       a text segment, where all the padding to the next 8k or whatever
 *       boundary is included.  Possibly peek at the text for an rts?
 */
taddr_t
get_addr_lim_via_functab(f)
func_t *f;
{
	functab_t *functab;
	func_t **ft;

	functab = f->fu_symtab->st_functab;

	for (ft = functab->tab; *ft != f; ++ft)
		;
	
	return ft[1]->fu_addr;
}

/*  Call (*func)(f, addrlim(f), arg1, arg2) for all functions in
 *  the function table of symtab.
 */
int
iterate_over_functions(st, func, arg1, arg2)
symtab_t *st;
iof_func_t func;
char *arg1, *arg2;
{
	functab_t *functab;
	func_t **ft, **lim;
	int res;

	functab = st->st_functab;

	ft = functab->tab;
	lim = ft + functab->nfuncs;
	for (; ft < lim; ++ft)
		if ((res = (*func)(*ft, ft[1]->fu_addr, arg1, arg2)) != 0)
			return res;
	return 0;
}

/*  Look up the function by address and return the function with address
 *  greatest but <= addr.
 */
bool
addr_and_functab_to_func(functab, addr, p_func)
functab_t *functab;
taddr_t addr;
func_t **p_func;
{
	func_t **mid;
	func_t **vec, **low, **max, **high;
	int vec_len;
	taddr_t maddr;

	/*  Don't bother with the search if the address is out of range.
	 */
	if (addr < functab->addr_base || addr >= functab->addr_lim)
		return FALSE;

	vec = functab->tab;
	vec_len = functab->nfuncs;

	low = vec;
	high = max = vec + vec_len;
	while (low <= high) {
		mid = low + (high - low) / 2;
		maddr = (*mid)->fu_addr;
		if (addr >= maddr && (mid == max || addr < mid[1]->fu_addr)) {
			if (mid == vec + vec_len)
				return FALSE;

			*p_func = *mid;
			return TRUE;
		}
		else if (maddr > addr) {
			high = mid - 1;
		}
		else {
			low = mid + 1;
		}
	}
	
	return FALSE;
}
