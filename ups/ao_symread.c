/* ao_symread.c - buffered reading of symbols from an a.out file */

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


/* @(#)ao_symread.c	1.4 04 Jun 1995 (UKC) */
char ups_ao_symread_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#ifdef AO_TARGET

#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef AO_ELF
#include <a.out.h>
#endif
#include <errno.h>

#ifdef __STDC__
#include <unistd.h>
#endif

#ifndef L_SET
#define L_SET 0
#endif

#include <local/ukcprog.h>
#include <mtrprog/strcache.h>
#include <mtrprog/utils.h>

#include "ups.h"
#include "symtab.h"
#ifdef ARCH_MIPS
#include "mips_frame.h"
#endif
#ifdef AO_ELF
#include "elfstab.h"
#endif
#include "target.h"
#include "st.h"
#include "ao_syms.h"
#include "ao_symread.h"

#ifdef ARCH_VAX
#define N_TXTADDR(x)	0
#endif

#ifdef ARCH_CLIPPER
#ifndef N_TXTADDR
#define N_TXTADDR(x)	0x8000
#endif
#endif


#define STRPAGESIZE	256

/*  This used to be `typedef const char *strpage_t[STRPAGESIZE]', but this
 *  triggers a bug in SC 2.0.1.
 */
typedef struct {
	const char *str[STRPAGESIZE];
} strpage_t;

#ifdef ST_TE
#ifdef MIPSEB
#define ST_COFF_MAGIC	MIPSEBMAGIC
#else
#define ST_COFF_MAGIC	MIPSELMAGIC
#endif
#endif

typedef struct Extra_offset Extra_offset;

struct Extra_offset {
	int symno;		/* Symno from which offset applies */
	size_t offset;		/* Additional offset into strings */
	Extra_offset *next;
};

typedef unsigned Lru_stamp;

typedef unsigned long Bitvec_slot;
enum { BITS_PER_SLOT = sizeof(Bitvec_slot) * 8 };
	
/*  Fields needed for reading symbols from the text file.
 */
struct symio_s {
	alloc_pool_t *apool;
	const char *path;
	Lru_stamp last_used;
	bool own_apool;
	int fd;		/* File descriptor of the a.out file */
	strcache_id_t strcache_id;	/* Symbol strings cache handle */
	strpage_t **strpage_tab;	/* Table of cached sym string values */
	off_t syms_offset;	/* File offset of start of symbols */
	off_t strings_offset;/* File offset of symtab strings */
	int nsyms;		/* Number of symbols in this a.out file */
	Bitvec_slot *bitvec;	/* For mark_sym() and next_unmarked_sym() */
	int num_bitvec_slots;	/* # entries in bitvec */
	int symbuf_nsyms;	/* Size of symbol buffer in symbols */
	nlist_t *symbuf;	/* Symbol buffer */
	int symbase;		/* Symno of first symbol in buffer */
	int symlim;		/* Symno of one after the last sym in buffer */
	Extra_offset *extra_offsets;
	Extra_offset *last_extra_offset;
};

#define MAX_SYMIOS	4
static symio_t *Sitab[MAX_SYMIOS];

static void fill_symbuf PROTO((symio_t *si, int symno));
static size_t get_extra_offset PROTO((symio_t *si, int symno));
static symio_t **get_free_symio_slot PROTO((void));
static bool prepare_to_use PROTO((symio_t *si));
static void about_to_use PROTO((symio_t *si));

/*  Size of the buffer used by getsym, in units of SYMSIZE.
 *
 *  There are two buffer sizes - a large one for sequential
 *  reading and a small one for random reading.  The sequential scan
 *  routine (findsym) reduces the buffer size when it reads the last
 *  symbol.
 *
 *  Unfortunately SYMSIZE (12) is not an even divisor of 512 or 1024, so
 *  the read() buffer size is almost but not quite entirely unlike 1024.
 */
#define S_SYMBUF_NSYMS	256
#define R_SYMBUF_NSYMS	42


/*  Given an executable file name and some other information, make a
 *  symio_t structure which can be use to read symbols and their strings
 *  from the text file.  Return an opaque handle on the structure.
 */
symio_t *
make_symio(ap, textpath, fd, syms_offset, nsyms, strings_offset)
alloc_pool_t *ap;
const char *textpath;
int fd;
off_t syms_offset;
int nsyms;
off_t strings_offset;
{
	symio_t *si;
	strcache_id_t strcache_id;
	strpage_t **pagetab;
	int npages, pagenum, num_bitvec_slots;
	bool own_apool;
	Bitvec_slot *bitvec;

	own_apool = ap == NULL;
	if (own_apool)
		ap = alloc_create_pool();
	
	strcache_id = sc_make_fd_strcache(fd);

	npages = nsyms / STRPAGESIZE + 1;
	pagetab = (strpage_t **)alloc(ap, sizeof(strpage_t *) * npages);
	for (pagenum = 0; pagenum < npages; ++pagenum)
		pagetab[pagenum] = NULL;

	/*  We allocate one extra slot as an end stop marker.
	 */
	num_bitvec_slots = ((nsyms + BITS_PER_SLOT - 1) / BITS_PER_SLOT) + 2;
	
	bitvec = (Bitvec_slot *)alloc(ap,
				      num_bitvec_slots * sizeof(Bitvec_slot));
	memset(bitvec, '\0', num_bitvec_slots * sizeof(Bitvec_slot));

	si = (symio_t *)alloc(ap, sizeof(symio_t));

	si->apool = ap;
	si->own_apool = own_apool;
/*	si->path = textpath;*/ 
/*RGA Insight says this is freed at end of make_symtab_cache, so save it */
	si->path =  alloc_strdup(ap, textpath);
	
	si->fd = fd;
	si->strcache_id = strcache_id;
	si->strings_offset = strings_offset;
	si->strpage_tab = pagetab;

	si->num_bitvec_slots = num_bitvec_slots;
	si->bitvec = bitvec;
	
	si->syms_offset = syms_offset;
	si->nsyms = nsyms;
	si->symbuf_nsyms = S_SYMBUF_NSYMS;
	si->symbuf = (nlist_t *)alloc(ap, si->symbuf_nsyms * SYMSIZE);
	si->symbase = si->nsyms + 1;
	si->symlim = 0;

	si->extra_offsets = NULL;
	si->last_extra_offset = NULL;

	*get_free_symio_slot() = si;
	
	return si;
}

void
mark_sym(si, symno)
symio_t *si;
int symno;
{
	if (symno < 0 || symno >= si->nsyms)
		panic("symno out of range in ms");

	si->bitvec[symno / BITS_PER_SLOT] |= 1 << (symno % BITS_PER_SLOT);
}

int
next_unmarked_sym(si, symno)
symio_t *si;
int symno;
{
	Bitvec_slot *slot;
	int offset;

	if (symno < 0 || symno > si->nsyms)
		panic("symno out of range in nus");
	
	slot = &si->bitvec[symno / BITS_PER_SLOT];
	
	offset = symno % BITS_PER_SLOT;

	/*  We are guaranteed to find a zero bit eventually, because we
	 *  have an extra slot with zero bits that mark_sym() can never set.
	 */
	for (;;) {
		if (*slot != ~0) {
			Bitvec_slot bits, mask;

			bits = *slot;
			mask = 1 << offset;
		
			for (; offset < BITS_PER_SLOT; ++offset) {
				if ((bits & mask) == 0) {
					return (slot - si->bitvec) *
						             BITS_PER_SLOT +
						offset;
				}

				mask <<= 1;
			}
		}

		offset = 0;
		++slot;
	}
}

/*  Return the number of symbols in the text file referred to by si.
 *  Used by scan_symtab() to detect the end of the symbol table.
 */
int
get_symio_nsyms(si)
symio_t *si;
{
	return si->nsyms;
}

static symio_t **
get_free_symio_slot()
{
	int i;
	symio_t **p_si;

	p_si = NULL;
	
	for (i = 0; i < MAX_SYMIOS; ++i) {
		if (Sitab[i] == NULL)
			return &Sitab[i];
		
		if (p_si == NULL ||
		    (Sitab[i] != NULL &&
		     Sitab[i]->last_used < (*p_si)->last_used)) {
			p_si = &Sitab[i];
		}
	}
			
	if (p_si == NULL)
		panic("lru botch in cls");

	sc_close_strcache((*p_si)->strcache_id);
	close((*p_si)->fd);
	(*p_si)->fd = -1;

	*p_si = NULL;
	return p_si;
}

static bool
prepare_to_use(si)
symio_t *si;
{
	static Lru_stamp stamp;
	symio_t **p_si;
	
	si->last_used = stamp++;
	
	if (si->fd != -1)
		return TRUE;

	p_si = get_free_symio_slot();

	if ((si->fd = open(si->path, O_RDONLY)) == -1) {
		failmesg("Can't reopen", "", si->path);
		return FALSE;
	}

	si->strcache_id = sc_make_fd_strcache(si->fd);

	*p_si = si;
	return TRUE;
}

/*  TODO: error returns from getsym() et al, so we can handle failure
 *        to reopen a symio.
 */
static void
about_to_use(si)
symio_t *si;
{
	if (!prepare_to_use(si))
		panic("failed to reopen si");
}

/*  Close (and conceptually destroy) si.  We don't actually free
 *  the structure here because space for it was allocated via alloc().
 */
void
close_symio(si)
symio_t *si;
{
  	if (!si)
	  return;		/* RGA */

	if (si->fd != -1) {
		int i;
		
		sc_close_strcache(si->strcache_id);

		close(si->fd);

		for (i = 0; i < MAX_SYMIOS; ++i) {
			if (Sitab[i] == si) {
				Sitab[i] = NULL;
				break;
			}
		}

		if (i == MAX_SYMIOS)
			panic("missing sitab entry in cs");
	}

	if (si->own_apool)
		alloc_free_pool(si->apool);
}

/*  Fill the symbols buffer of si so that symbol symno is at the start of
 *  the buffer.
 */
static void
fill_symbuf(si, symno)
symio_t *si;
int symno;
{
	int n_read, nsyms_to_read;

	if (symno < 0 || symno >= si->nsyms)
		panic("symno botch in fs");

	if (lseek(si->fd, si->syms_offset + symno * (int)SYMSIZE,
								L_SET) == -1)
		panic("lseek failed in fs");

	nsyms_to_read = si->symbuf_nsyms;
	if (si->nsyms - symno < nsyms_to_read)
		nsyms_to_read = si->nsyms - symno;

	n_read = read(si->fd, (char *)si->symbuf,
						nsyms_to_read * SYMSIZE);

	if (n_read == -1)
		panic("read error in symbol table");
	if (n_read == 0)
		panic("unexpected EOF in symbol table");
	if (n_read % SYMSIZE != 0)
		panic("n_read botch in fs");

	si->symbase = symno;
	si->symlim = si->symbase + n_read / SYMSIZE;
}

/*  Get symbol symnum from the symbol table, and copy to *p_nm.
 */
void
getsym(si, symno, p_nm)
symio_t *si;
int symno;
nlist_t *p_nm;
{
	char *src;

	about_to_use(si);
	
	if (symno < si->symbase || symno >= si->symlim)
		fill_symbuf(si, symno);

	/*  On the Sun 386, sizeof(nlist_t) is not the same as the
	 *  size of an nlist entry in the file.  Thus we have to
	 *  play silly games to get to the right entry.
	 */
	src = (char *)si->symbuf + (symno - si->symbase) * SYMSIZE;
	*p_nm = *(nlist_t *)src;
}

#ifdef ST_TE
/*  Return a copy of the NUL terminated string at the given offset
 *  from the start of the strings for si.
 */
const char *
si_get_string(si, offset)
symio_t *si;
off_t offset;
{
	size_t len;
	const char *s;

	about_to_use(si);
	
	s = sc_get_string(si->strcache_id, si->strings_offset + offset,
								  '\0', &len);
	if (s == NULL)
		panic("str botch in sgs");
	
	return strcpy(alloc(si->apool, len + 1), s);
}
#endif /* ST_TE */


#ifndef ST_TE
/*  Search the symbol table starting from symbol number symno for a
 *  symbol whose type is in symset.  A symbol is wanted if symset[nm.n_type]
 *  is non zero.  If we find a wanted symbol, copy it to *p_nm and return
 *  its symbol number.  Otherwise return a symbol number one larger than
 *  that of the last symbol in the table.
 *
 *  This is used by the sequential prescan in scan_symtab().
 *
 *  See the comment in getsym for why p_nm below is "char *" rather
 *  than "nlist_t *".
 *
 *  Not used on the DECstation 3100.
 */
int
findsym(si, symno, p_res, symset)
symio_t *si;
int symno;
nlist_t *p_res;
register const char *symset;
{
	register char *p_nm;

	about_to_use(si);
	
	if (symno < si->symbase)
		fill_symbuf(si, symno);
	p_nm = (char *)si->symbuf + (symno - si->symbase) * SYMSIZE;
	for (; symno < si->nsyms; p_nm += SYMSIZE, ++symno) {
		if (symno >= si->symlim) {
			fill_symbuf(si, symno);
			p_nm = (char *)si->symbuf;
		}
		if (symset[((nlist_t *)p_nm)->n_type] != 0) {
			*p_res = *(nlist_t *)p_nm;
			return symno;
		}
	}

	/*  End of sequential scan, so shrink the buffer.
	 *
	 *  The buffer was allocated via alloc(), so we don't actually
	 *  change the size - we just drop symbuf_nsyms.  The next
	 *  fill of the buffer will use the reduced size.
	 */
	si->symbuf_nsyms = R_SYMBUF_NSYMS;

	return symno;
}

/*  Symbol table perversity #473: gcc on BSD/386 puts N_DATA symbols
 *  in the middle of a symbol whose definition is split over multiple
 *  lines.  So when we see a backslash continuation, we have to keep
 *  reading until we see a symbol of the same type as the first.
 */
const char *
get_cont_symstring(sr)
Symrec *sr;
{
#ifndef ARCH_BSDI386
	return symstring(sr->symio, ++sr->symno);
#else
	int symtype;
	nlist_t nm;

	getsym(sr->symio, sr->symno, &nm);
	symtype = nm.n_type;

	do {
		getsym(sr->symio, ++sr->symno, &nm);
	} while (nm.n_type != symtype);

	return symstring(sr->symio, sr->symno);
#endif
}

/*  Return the string for symbol symno.
 *
 *  Not used on the DECstation 3100.
 */
const char *
symstring(si, symno)
symio_t *si;
int symno;
{
	nlist_t nm;
	int pagenum, slot;
	strpage_t *page;
	
	about_to_use(si);

	if (symno < 0 || symno >= si->nsyms)
		panic("symno botch in ss");
	
	pagenum = symno / STRPAGESIZE;
	page = si->strpage_tab[pagenum];
	
	if (page == NULL) {
		int i;

		page = (strpage_t *)alloc(si->apool, sizeof(strpage_t));
		for (i = 0; i < STRPAGESIZE; ++i)
			page->str[i] = NULL;
		si->strpage_tab[pagenum] = page;
	}
	
	slot = symno % STRPAGESIZE;
	if (page->str[slot] == NULL) {
		off_t offset;
		size_t len;
		const char *line;

		getsym(si, symno, &nm);

		offset = nm.n_offset;
		if (si->extra_offsets != NULL)
			offset += get_extra_offset(si, symno);
			
		line = sc_get_string(si->strcache_id,
				     si->strings_offset + offset,
				     '\0', &len);
		line = strcpy(alloc(si->apool, len + 1), line);
		page->str[slot] = line;
	}
	
	return page->str[slot];
}
#endif /* !ST_TE */

static size_t
get_extra_offset(si, symno)
symio_t *si;
int symno;
{
	Extra_offset *eo, *prev;
	
	prev = NULL;

	eo = si->last_extra_offset;
	
	if (eo == NULL || eo->symno > symno)
		eo = si->extra_offsets;

	prev = NULL;
	for (; eo != NULL; eo = eo->next) {
		if (eo->symno > symno)
			break;
		prev = eo;
	}

	si->last_extra_offset = prev;

	return (prev != NULL) ? prev->offset : 0;
}
	
void
add_extra_string_offset(si, symno, offset)
symio_t *si;
int symno;
off_t offset;
{
	Extra_offset *eo, *last;

	if (offset == 0)
		return;
	
	last = NULL;
	for (eo = si->extra_offsets; eo != NULL; eo = eo->next)
		last = eo;

	eo = (Extra_offset *)alloc(si->apool, sizeof(Extra_offset));
	eo->symno = symno;
	eo->offset = offset;
	eo->next = NULL;
	
	if (last == NULL)
		si->extra_offsets = eo;
	else
		last->next = eo;
}

/* RGA use fd of symio instead of symtab, because getsym(), findsym(), etc
   call get_free_symio_slot() which reuses fd numbers, so symtab_t fd 
   can be invalid */
int
get_symio_fd(si)
symio_t *si;
{
  about_to_use(si);
  return si->fd;
}
		
#endif /* AO_TARGET */
