/* cx_symio.c - write out debugging symbol table information */

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


/* @(#)cx_symio.c	1.3 16 Sep 1994 (UKC) */
char ups_cx_symio_c_rcsid[] = "$Id$";

#include <sys/types.h>

#include <stdio.h>
#include <string.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>

typedef struct block_s block_t;		/* because we don't have symtab.h */

#include "ups.h"
#include "ci.h"
#include "xc_load.h"
#include "cx_link.h"
#include "cx_symio.h"
#include "cx_wsym.h"

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#endif

static void unpack_letab PROTO((symfunc_t *symfuncs, size_t nsymfuncs,
					const char *buf, size_t nbytes));
static int write_letab PROTO((FILE *fp, lnum_entry_t *letab, int count,
							    int *p_nbytes));

int
readfp(fp, path, buf, nbytes)
FILE *fp;
const char *path;
char *buf;
size_t nbytes;
{
	if (nbytes == 0 || fread(buf, nbytes, 1, fp) == 1)
		return 0;

	if (feof(fp))
		errf("Unexpected EOF reading from %s", path);
	else
		failmesg("Error reading from", "", path);
	
	return -1;
}

int
fp_write_val(fp, val)
FILE *fp;
size_t val;
{
	return fp_write(fp, (char *)&val, sizeof(val));
}

int
fp_write(fp, buf, count)
FILE *fp;
const char *buf;
size_t count;
{
	if (count != 0 && fwrite(buf, count, 1, fp) != 1)
		return -1;
	
	return 0;
}

int
save_string(eb, s, fp)
ebuf_t *eb;
const char *s;
FILE *fp;
{
	return fp_write_val(fp, note_string(eb, s));
}

size_t
note_string(eb, s)
ebuf_t *eb;
const char *s;
{
	int pos;

	if (s == NULL)
		return (size_t)-1; /* BUG: this func should not return size_t */

	ebuf_get(eb, &pos);
	ebuf_add(eb, s, strlen(s) + 1);
	
	return pos;
}

int
write_strings(eb, fp, p_size)
ebuf_t *eb;
FILE *fp;
size_t *p_size;
{
	const char *buf;
	int nbytes;

	buf = ebuf_get(eb, &nbytes);

	if (fp_write(fp, buf, (size_t)nbytes) != 0)
		return -1;

	*p_size = nbytes;
	return 0;
}

static int
write_letab(fp, letab, count, p_nbytes)
FILE *fp;
lnum_entry_t *letab;
int count, *p_nbytes;
{
	lnum_entry_t *le, *lim;
	int lnum, addr, nbytes;

	/*  We need a non-relative record at the start of each function
	 *  for the benifit of the unpacking code in load_symtab().
	 *  We force this by setting lnum to cause a large delta.
	 */
	lnum = -1000;
	addr = 0;	/* to satisfy gcc */
	nbytes = 0;

	for (le = letab, lim = &letab[count]; le < lim; ++le) {
		unsigned short sval;
		int lnum_delta, addr_delta;

		lnum_delta = le->le_lnum - lnum;
		addr_delta = (int)le->le_addr - addr;

		if (lnum_delta < 0)
			panic("sort botch in write_letab");

		if (lnum_delta < 128 &&
		    -256 < addr_delta && addr_delta <= 255) {
			int sign;

		 	sign = (addr_delta < 0) ? (1 << 8) : 0;
			sval = (lnum_delta << 9) | sign | (addr_delta & 0xff);

			if (fp_write(fp, (char *)&sval, sizeof(sval)) != 0)
				return -1;
		}
		else {
			sval = 1 << 8;		/* -0 */
			if (fp_write(fp, (char *)&sval, sizeof(sval)) != 0 ||
			    fp_write_val(fp, (size_t)le->le_lnum) != 0 ||
			    fp_write_val(fp, (size_t)le->le_addr) != 0) {
				return -1;
			}

			nbytes += 2 * sizeof(size_t);
		}

		lnum = le->le_lnum;
		addr = le->le_addr;

		nbytes += sizeof(sval);
	}

	*p_nbytes = nbytes;
	return 0;
}

int
write_symtab(fp, path, str_eb, li, p_symtab_size)
FILE *fp;
const char *path;
ebuf_t *str_eb;
linkinfo_t *li;
long *p_symtab_size;
{
	o_syminfo_t oshdr;
	symfunc_t *sf, *sflim;
	const char **sv, **svlim;
	long symtab_startpos, symdata_startpos;
	size_t symdata_nbytes, strings_size;
	size_t i, letab_nbytes, letab_size, ntypes, types_offset;
	alloc_pool_t *ap;
	syminfo_t *si;

	symtab_startpos = ftell(fp);

	if (fseek(fp, (long)sizeof(o_syminfo_t), SEEK_CUR) != 0) {
		failmesg("Error seeking in", "", path);
		return -1;
	}

	letab_size = 0;

	sflim = li->li_symfuncs + li->li_nsymfuncs;
	for (sf = li->li_symfuncs; sf < sflim; ++sf) {
		if (save_string(str_eb, sf->sf_name, fp) != 0 ||
		    fp_write_val(fp, (size_t)sf->sf_addr) != 0 ||
		    fp_write_val(fp, (size_t)sf->sf_letab_size) != 0)
			return -1;

		letab_size += sf->sf_letab_size;
	}

	letab_nbytes = 0;
	for (sf = li->li_symfuncs; sf < sflim; ++sf) {
		int nbytes;

		if (write_letab(fp, sf->sf_letab, sf->sf_letab_size,
								&nbytes) != 0)
			return -1;

		letab_nbytes += nbytes;
	}

	svlim = li->li_symvars + li->li_nsymvars;
	for (sv = li->li_symvars; sv < svlim; ++sv) {
		if (save_string(str_eb, *sv, fp) != 0)
			return -1;
	}

	ap = alloc_create_pool();
	si = make_syminfo(ap, fp, str_eb);

	symdata_startpos = ftell(fp);

	if (write_block(si, li->li_filblock) != 0) {
		alloc_free_pool(ap);
		return -1;
	}

	for (i = 0; i < li->li_nsymfuncs; ++i) {
		if (write_block(si, li->li_funcblocks[i]) != 0) {
			alloc_free_pool(ap);
			return -1;
		}
	}

	types_offset = ftell(fp) - symtab_startpos;

	if (write_types(si, &ntypes) != 0) {
		alloc_free_pool(ap);
		return -1;
	}

	symdata_nbytes = ftell(fp) - symdata_startpos;

	alloc_free_pool(ap);
		
	if (write_strings(str_eb, fp, &strings_size) != 0)
		return -1;

	oshdr.os_nfuncs = li->li_nsymfuncs;
	oshdr.os_nvars = li->li_nsymvars;
	oshdr.os_letab_count = letab_size;
	oshdr.os_letab_nbytes = letab_nbytes;
	oshdr.os_types_offset = types_offset;
	oshdr.os_ntypes = ntypes;
	oshdr.os_symdata_nbytes = symdata_nbytes;
	oshdr.os_strings_size = strings_size;

	if (fseek(fp, symtab_startpos, SEEK_SET) != 0) {
		failmesg("Error seeking in", "", path);
		return -1;
	}

	if (fp_write(fp, (char *)&oshdr, sizeof(oshdr)) != 0)
		return -1;

	*p_symtab_size = get_symtab_size(&oshdr);
	return 0;
}

int
get_symtab_size(os)
o_syminfo_t *os;
{
	return sizeof(*os) +
	       os->os_nfuncs * sizeof(size_t) * 3 +
	       os->os_nvars * sizeof(size_t) +
	       os->os_symdata_nbytes +
	       os->os_letab_nbytes + 
	       os->os_strings_size;
}

static void
unpack_letab(symfuncs, nsymfuncs, buf, nbytes)
symfunc_t *symfuncs;
size_t nsymfuncs;
const char *buf;
size_t nbytes;
{
	symfunc_t *sf, *sflim;
	const char *iptr, *ilim;
	int lnum, addr, le_index;

	sf = symfuncs;
	sflim = symfuncs + nsymfuncs;
	iptr = buf;
	ilim = buf + nbytes;

	lnum = addr = le_index = 0;

	for (;;) {
		unsigned short sval;
		int addr_delta, lnum_delta;
		bool negative;

		if (le_index == sf->sf_letab_size) {
			lnum = addr = le_index = 0;

			while (++sf < sflim && le_index == sf->sf_letab_size)
				;

			if (sf >= sflim || iptr >= ilim)
				break;
		}

		memcpy((char *)&sval, iptr, sizeof(sval));
		iptr += sizeof(sval);

		lnum_delta = sval >> 9;
		negative = (sval & (1 << 8)) != 0;
		addr_delta = sval & 0xff;

		if (negative && addr_delta == 0) {
			size_t words[2];

			memcpy((char *)words, iptr, sizeof(words));
			iptr += sizeof(words);

			lnum = words[0];
			addr = words[1];
		}
		else {
			lnum += lnum_delta;
			addr += negative ? -addr_delta : addr_delta;
		}

		sf->sf_letab[le_index].le_lnum = lnum;
		sf->sf_letab[le_index].le_addr = addr;
		++le_index;
	}

	if (sf != sflim || le_index != 0 || iptr != ilim)
		panic("letab botch in ult");
}

int
read_symtab(fp, path, symtab_size, li)
FILE *fp;
const char *path;
size_t symtab_size;
linkinfo_t *li;
{
	o_syminfo_t oshdr;
	char *strings;

	if (readfp(fp, path, (char *)&oshdr, sizeof(oshdr)) != 0)
		return -1;

	if (get_symtab_size(&oshdr) != symtab_size) {
		errf("Symtab size mismatch (%d != %d) in %s",
				get_symtab_size(&oshdr), symtab_size, path);
		return -1;
	}

	strings = alloc(li->li_apool, oshdr.os_strings_size);

	li->li_nsymfuncs = oshdr.os_nfuncs;

	if (oshdr.os_nfuncs == 0) {
		li->li_symfuncs = NULL;
	}
	else {
		symfunc_t *sf, *lim;
		char *buf;

		li->li_symfuncs = (symfunc_t *)alloc(li->li_apool,
					   oshdr.os_nfuncs * sizeof(symfunc_t));

		lim = li->li_symfuncs + oshdr.os_nfuncs;
		for (sf = li->li_symfuncs; sf < lim; ++sf) {
			size_t words[3];

			if (readfp(fp, path, (char *)words, sizeof(words)) != 0)
				return -1;

			sf->sf_name = strings + words[0];
			sf->sf_addr = words[1];
			sf->sf_letab_size = words[2];
			sf->sf_letab = (lnum_entry_t *)alloc(li->li_apool,
					       words[2] * sizeof(lnum_entry_t));
		}

		buf = alloc(li->li_apool, oshdr.os_letab_nbytes);
		if (readfp(fp, path, buf, oshdr.os_letab_nbytes) != 0)
			return -1;

		unpack_letab(li->li_symfuncs, oshdr.os_nfuncs,
						buf, oshdr.os_letab_nbytes);
	}

	li->li_nsymvars = oshdr.os_nvars;

	if (oshdr.os_nvars == 0) {
		li->li_symvars = NULL;
	}
	else {
		const char **sv, **lim;

		li->li_symvars = (const char **)alloc(li->li_apool,
				         oshdr.os_nvars * sizeof(const char *));

		lim = li->li_symvars + oshdr.os_nvars;
		for (sv = li->li_symvars; sv < lim; ++sv) {
			size_t word;

			if (readfp(fp, path, (char *)&word, sizeof(word)) != 0)
				return -1;

			*sv = strings + word;
		}
	}

	/*  We skip the pure debugging information - we don't need it
	 *  for linking or disassembling.
	 */
	if (fseek(fp, (long)oshdr.os_symdata_nbytes, SEEK_CUR) == -1) {
		errf("Can't seek forwards %d bytes in %s: %s",
		     oshdr.os_symdata_nbytes, path, get_errno_str());
		return -1;
	}

	return readfp(fp, path, strings, oshdr.os_strings_size);
}
