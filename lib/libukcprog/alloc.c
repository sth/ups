/* alloc_pool.c - routines for allocating memory from tagged pools */

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


/* @(#)alloc.c	1.10 25/10/93 UKC */
char ukcprog_alloc_rcsid[] = "$Id$";

#include <stdio.h>	/* for NULL - grrr */
#include <stdlib.h>
#include <string.h>
#ifndef __STDC__
#include <memory.h>
#endif

#include "ukcprog.h"

/*  This is a conservative guess at the per-request malloc overhead in
 *  bytes.  Nothing breaks if this is wrong.
 */
#define MALLOC_OVERHEAD	24

/*  When we run out a space in an alloc pool we add another block.
 *  We add small blocks (SBLOCKSIZE bytes each) for the first NSMALLBLOCKS
 *  requests, then switch switch to large (BLOCKSIZE) ones.
 *
 *  The intention is that we don't gobble large amounts of memory for
 *  a small alloc pool, but that we are reasonablty efficient for
 *  one that's continually growing.
 *
 *  Currently, we go slowly (256 bytes a go) for the first 8K, then
 *  fast (4K a go).
 */
#define NSMALLBLOCKS	32

/*  Size of the first block for an alloc pool (requested when the alloc
 *  pool is created) and for the subsequent NSMALLBLOCKS blocks.
 */
#define SBLOCKSIZE	(256 - sizeof(alloc_pool_t) - MALLOC_OVERHEAD)

/*  Size of the requested for an alloc pool after the first NSMALLBLOCKS
 *  block additions.
 *
 *  Try to make the malloc request size a bit less than a power of two
 *  to compensate for brain-damaged mallocs that add overhead then round
 *  up to a power of two.
 */
#define BLOCKSIZE	(4096 - sizeof(ablock_t) - MALLOC_OVERHEAD)

/*  Maximum alignment requirements for all types *including* float and double.
 */
#define ALIGN		sizeof(double)

typedef struct ablockst {
	union {
		double align;
		struct ablock {
			char *abu_buf;
			char *abu_pos;
			char *abu_end;
			size_t abu_size;
			struct ablockst *abu_next;
		} a;
	} u;
} ablock_t;

#define ab_buf	u.a.abu_buf
#define ab_pos	u.a.abu_pos
#define ab_end	u.a.abu_end
#define ab_size	u.a.abu_size
#define ab_next	u.a.abu_next

struct alloc_pool_s {
	ablock_t *ap_ablock;
	ablock_t *ap_freelist;
	int ap_nblocks;
	bool ap_debug;
	ablock_t ap_first_ablock;
};

struct alloc_mark_s {
	alloc_pool_t *am_apool;
	ablock_t *am_ablock;
	char *am_pos;
	char *am_end;
};

static ablock_t *push_ablock PROTO((alloc_pool_t *ap, ablock_t *ab, unsigned size));
static ablock_t *find_ab PROTO((alloc_pool_t *ap, unsigned size));
static void reset_ablocks PROTO((alloc_pool_t *ap, ablock_t *limab));

/*  The default debug flag for a new alloc_pool.  When the debug flag
 *  is TRUE, we initialise memory to garbage, and set it to (different)
 *  garbage when free_alloc_pool is called.
 */
static bool Default_debug_flag = TRUE;

bool
alloc_set_default_debug_flag(val)
bool val;
{
	bool oldval;

	oldval = Default_debug_flag;
	Default_debug_flag = val;
	return oldval;
}

bool
alloc_set_debug_flag(ap, val)
alloc_pool_t *ap;
bool val;
{
	bool oldval;

	oldval = ap->ap_debug;
	ap->ap_debug = val;
	return oldval;
}

/*  Make a new alloc_pool().  We make an initial allocation of a small
 *  amount of memory, to make small alloc pool creation cheap (one malloc).
 */
alloc_pool_t *
alloc_create_pool()
{
	alloc_pool_t *ap;

	ap = (alloc_pool_t *)e_malloc(sizeof(alloc_pool_t) + SBLOCKSIZE);
	ap->ap_ablock = NULL;
	ap->ap_freelist = NULL;
	ap->ap_nblocks = 0;
	ap->ap_debug = Default_debug_flag;
	push_ablock(ap, &ap->ap_first_ablock, SBLOCKSIZE);

	return ap;
}

static void
reset_ablocks(ap, limab)
alloc_pool_t *ap;
ablock_t *limab;
{
	ablock_t *ab, *next;
	bool debug;

	debug = ap->ap_debug;
	for (ab = ap->ap_ablock; ab != limab; ab = next) {
		next = ab->ab_next;
		if (debug)
			memset(ab->ab_buf, 0x42, ab->ab_size);
		ab->ab_pos = ab->ab_buf;
		ab->ab_end = ab->ab_pos + ab->ab_size;
		ab->ab_next = ap->ap_freelist;
		ap->ap_freelist = ab;
	}
}

void
alloc_reset_pool(ap)
alloc_pool_t *ap;
{
	ablock_t *ab;

	ab = &ap->ap_first_ablock;

	reset_ablocks(ap, ab);

	if (ap->ap_debug)
		memset(ab->ab_buf, 0x42, ab->ab_size);
	ab->ab_pos = ab->ab_buf;
	ab->ab_end = ab->ab_pos + ab->ab_size;

	ap->ap_ablock = ab;
}

void
alloc_free_pool(ap)
alloc_pool_t *ap;
{
	ablock_t *ab, *next;
	bool debug;

	debug = ap->ap_debug;

	/*  The odd form of the loop here is because we want to overwrite
	 *  all blocks with garbage (if debug is set), but we don't want
	 *  to free the last block in the chain, which is allocated as part
	 *  of the header block.
	 */
	ab = ap->ap_ablock;
	for (;;) {
		next = ab->ab_next;
		if (debug)
			memset(ab->ab_buf, 0x42, ab->ab_size);
		if (next == NULL)
			break;
		free((char *)ab);
		ab = next;
	}

	free((char *)ap);
}

static ablock_t *
push_ablock(ap, ab, size)
alloc_pool_t *ap;
ablock_t *ab;
unsigned size;
{
	ab->ab_buf = ab->ab_pos = (char *)&ab[1];
	ab->ab_end = ab->ab_buf + size;
	ab->ab_size = size;
	ab->ab_next = ap->ap_ablock;
	ap->ap_ablock = ab;

	if (ap->ap_debug)
		memset(ab->ab_buf, 0x53, (size_t)size);
	
	return ab;
}

/*  Find an ablock with at least nbytes free.  If the block at the
 *  head of the free list is big enough, use that.  Otherwise malloc
 *  a new ablock and push it on the chain.
 */
static ablock_t *
find_ab(ap, size)
alloc_pool_t *ap;
unsigned size;
{
	ablock_t *ab;

	if (ap->ap_freelist != NULL && ap->ap_freelist->ab_size >= size) {
		ab = ap->ap_freelist;
		ap->ap_freelist = ap->ap_freelist->ab_next;
		ab->ab_next = ap->ap_ablock;
		ap->ap_ablock = ab;
	}
	else {
		voidptr buf;
		unsigned blocksize;

		blocksize = (ap->ap_nblocks < NSMALLBLOCKS) ? SBLOCKSIZE : BLOCKSIZE;
		if (size < blocksize)
			size = blocksize;
		if ((buf = malloc((size_t)(sizeof(ablock_t) + size))) == NULL)
			return NULL;
		ab = push_ablock(ap, (ablock_t *)buf, size);
		++ap->ap_nblocks;
	}
	return ab;
}

/*  Allocate nbytes from alloc pool ap.  This interface never
 *  returns NULL - if memory runs out we panic.
 */
voidptr
alloc(ap, nbytes)
alloc_pool_t *ap;
size_t nbytes;
{
	ablock_t *ab;
	int over;
	char *ptr;

	over = nbytes % ALIGN;
	if (over != 0)
		nbytes += ALIGN - over;

	ab = ap->ap_ablock;

	if (nbytes > ab->ab_end - ab->ab_pos) {
		ab = find_ab(ap, (unsigned)nbytes);
		if (ab == NULL)
			panic("out of memory in alloc");
	}

	ptr = ab->ab_pos;
	ab->ab_pos += nbytes;

	return ptr;
}

/*  Like alloc, but return NULL if we can't satisfy the request.
 */
voidptr
alloc_ck(ap, nbytes)
alloc_pool_t *ap;
size_t nbytes;
{
	ablock_t *ab;
	int over;
	char *ptr;

	over = nbytes % ALIGN;
	if (over != 0)
		nbytes += ALIGN - over;

	ab = ap->ap_ablock;

	if (nbytes > ab->ab_end - ab->ab_pos) {
		ab = find_ab(ap, nbytes);
		if (ab == NULL)
			return NULL;
	}

	ptr = ab->ab_pos;
	ab->ab_pos += nbytes;

	return ptr;
}

alloc_mark_t *
alloc_mark(ap)
alloc_pool_t *ap;
{
	alloc_mark_t *am;
	ablock_t *save_ab;
	char *save_pos, *save_end;

	save_ab = ap->ap_ablock;
	save_pos = save_ab->ab_pos;
	save_end = save_ab->ab_end;

	am = (alloc_mark_t *)alloc(ap, sizeof(alloc_mark_t));
	am->am_apool = ap;
	am->am_ablock = save_ab;
	am->am_pos = save_pos;
	am->am_end = save_end;

	return am;
}

void
alloc_release(ap, am)
alloc_pool_t *ap;
alloc_mark_t *am;
{
	ablock_t *ab;
	alloc_mark_t mark;

	if (am->am_apool != ap)
		panic("id botch in ar");

	/*  If debug is set, we are about to step on the store that
	 *  the mark was allocated from, so save it.
	 */
	mark = *am;
	ab = mark.am_ablock;

	reset_ablocks(ap, ab);

	if (ap->ap_debug) {
		memset(mark.am_pos, 0x42, (size_t)(ab->ab_pos - mark.am_pos));
		memset(ab->ab_end, 0x42, (size_t)(mark.am_end - ab->ab_end));
	}
	else {
		/*  Make sure the application can't use this mark again.
		 */
		am->am_apool = NULL;
	}

	ab->ab_pos = mark.am_pos;
	ab->ab_end = mark.am_end;
	ap->ap_ablock = ab;
}

/*  Like alloc(), except that the result is assumed not to need alignment.
 *  We work from the other end of the pool than alloc so hopefully all the
 *  string requests will be packed together with no alignment holes.
 *
 *  We never return NULL - if we can't fulfill the request we panic.
 */
char *
allocstr(ap, nbytes)
alloc_pool_t *ap;
size_t nbytes;
{
	ablock_t *ab;

	ab = ap->ap_ablock;

	if (nbytes > ab->ab_end - ab->ab_pos) {
		ab = find_ab(ap, (unsigned)nbytes);
		if (ab == NULL)
			panic("out of memory in allocstr");
	}

	return ab->ab_end -= nbytes;
}

char *
allocstr_ck(ap, nbytes)
alloc_pool_t *ap;
size_t nbytes;
{
	ablock_t *ab;

	ab = ap->ap_ablock;

	/*  We cast nbytes to unsigned to catch negative values: they
	 *  turn into huge positive values which get caught by e_malloc().
	 */
	if ((unsigned)nbytes > ab->ab_end - ab->ab_pos) {
		ab = find_ab(ap, (unsigned)nbytes);
		if (ab == NULL)
			return NULL;
	}

	return ab->ab_end -= nbytes;
}

char *
alloc_strdup(ap, s)
alloc_pool_t *ap;
const char *s;
{
	size_t nbytes;

	nbytes = strlen(s) + 1;
	return memcpy(allocstr(ap, nbytes), s, nbytes);
}
