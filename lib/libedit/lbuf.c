/* lbuf.c - buffer implementation using a list of blocks */

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


/* @(#)lbuf.c	1.5 09 Apr 1995 (UKC) */
char edit_lbuf_c_rcsid[] = "$Id$";

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include <local/ukcprog.h>

#include "textbuf.h"
#include "lbuf.h"

static void lbuf_destroy PROTO((Text_bufdata *b));

static void lbuf_insert PROTO((Text_bufdata *b,
			     size_t pos, const char *text, size_t len));
static void lbuf_delete PROTO((Text_bufdata *b, size_t pos, size_t len));

static bool lbuf_get_bytes PROTO((Text_bufdata *b, size_t pos,
				const char **p_start, const char **p_lim));
static bool lbuf_get_bytes_before PROTO((Text_bufdata *b, size_t pos,
				       const char **p_start,
				       const char **p_lim));

static size_t lbuf_get_length PROTO((Text_bufdata *b));

static void lbuf_set_debug_flag PROTO((Text_bufdata *b));

typedef struct Block {
	char *text;		/* NULL if still to read from file */
	size_t blocklen;	/* number of bytes of text */
	off_t file_offset;
	struct Block *next;
} Block;

struct Text_bufdata {
	alloc_pool_t *apool;
	char *handle;
	Block *blocks;

	Block *lastprev;
	size_t lastpos;

	bool done_all_file_offsets;
	Text_info_func info_func;
	Text_read_func read_func;
	Text_close_func close_func;
	size_t buflen;
	size_t blocksize;

	bool debug;
	bool apool_is_scratch;
};

static bool get_block PROTO((Text_bufdata *b, size_t pos, Block **p_prev,
			     Block **p_bl, size_t *p_offset_within_block));
static bool read_text PROTO((Text_bufdata *b, Block *bl));
static bool set_all_file_offsets PROTO(( Text_bufdata *b));
static void dump_blocks PROTO((Text_bufdata *b, const char *label));
static Block *append_block PROTO((Text_bufdata *b, Block *prev,
				  bool allocate_text));
static void transfer PROTO((Text_bufdata *b, Block *bl,
			    size_t offset_within_block,
			    const char **p_instext, size_t *p_inslen));

static void
lbuf_destroy(b)
Text_bufdata *b;
{
	(*b->close_func)(b->handle);
/* RGA when debugging a SunOS target without a PID, and having
   opened shared library files (for viewing or breakpoints), this is 
   called on the first attach from free_symtab_cache(). Cannot free
   the apool at this point as it is the pool for the whole symtab.
   It is freed later at the end of close_symtab().

   16 May 01 RGA to fix leaks when changing targets, free the apool
   only when apool_is_scratch is set. This is for buffers created
   from text_create_empty_lbuf_buffer only.
*/
	if (b->apool_is_scratch)
	    alloc_free_pool(b->apool);
}

static size_t
lbuf_get_length(b)
Text_bufdata *b;
{
	return b->buflen;
}

static void
lbuf_set_debug_flag(b)
Text_bufdata *b;
{
	b->debug = TRUE;
}

Text_buffer *
text_create_empty_lbuf_buffer(ap, maxblocks, blocksize)
alloc_pool_t *ap;
size_t maxblocks, blocksize;
{
	return text_create_lbuf_buffer(ap, maxblocks, blocksize,
				       (voidptr)NULL,
				       (Text_info_func)NULL,
				       (Text_read_func)NULL,
				       (Text_close_func)NULL,
				       TRUE);
}

Text_buffer *
text_create_lbuf_buffer(ap, maxblocks, blocksize,
			handle, info_func, read_func, close_func, apool_is_scratch)
alloc_pool_t *ap;
size_t maxblocks, blocksize;
voidptr handle;
Text_info_func info_func;
Text_read_func read_func;
Text_close_func close_func;
bool apool_is_scratch;
{
	static Text_buffer_ops lbuf_ops = {
		lbuf_destroy,
		lbuf_insert,
		lbuf_delete,
		lbuf_get_bytes,
		lbuf_get_bytes_before,
		lbuf_get_length,
		lbuf_set_debug_flag
	};
	Text_bufdata *b;
	Text_buffer *tb;
	size_t size;
	time_t mtime;

	if (info_func == NULL) {
		size = 0;
		mtime = 0;
	}
	else {
		if (!(*info_func)(handle, &size, &mtime))
			return NULL;
	}
	
    	b = (Text_bufdata *)alloc(ap, sizeof(Text_bufdata));
	b->apool = ap;
	b->blocks = NULL;
	b->lastprev = NULL;
	b->lastpos = 0;
	b->done_all_file_offsets = FALSE;
	b->handle = handle;
	b->info_func = info_func;
	b->read_func = read_func;
	b->close_func = close_func;
	b->buflen = size;
	b->blocksize = blocksize;
	b->debug = FALSE;
	b->apool_is_scratch = apool_is_scratch;

	tb = (Text_buffer *)alloc(ap, sizeof(Text_buffer));
	tb->bufdata = b;
	tb->ops = &lbuf_ops;
	
	return tb;
}

void
text_get_buffer_modtime(tb, mtime)
     Text_buffer *tb;
     long *mtime;
{
  Text_bufdata *b;
  size_t size;
  time_t mt;


  b = tb->bufdata;
  (*b->info_func)(b->handle, &size, &mt);
  *mtime = (long)mt;
}

static bool
get_block(b, wanted_pos, p_prev, p_bl, p_offset_within_block)
Text_bufdata *b;
size_t wanted_pos;
Block **p_prev, **p_bl;
size_t *p_offset_within_block;
{
	size_t pos;
	Block *prev;
	Block *bl;

	if (b->lastprev != NULL && b->lastpos <= wanted_pos) {
		pos = b->lastpos;
		prev = b->lastprev;
		bl = prev->next;
	}
	else {
		pos = 0;
		prev = NULL;
		bl = b->blocks;
	}
	
	for (;;) {
		if (bl == NULL) {
			size_t blocklen;
			
			if (pos == b->buflen && wanted_pos == pos)
				break;
			
			if (pos >= b->buflen)
				panic("pos botch in gb");

			blocklen = b->buflen - pos;

			if (blocklen > b->blocksize)
				blocklen = b->blocksize;

			bl = append_block(b, prev, FALSE);
			
			bl->blocklen = blocklen;
			bl->file_offset = pos;
		}
		
		if (wanted_pos >= pos && wanted_pos < pos + bl->blocklen)
			break;

		pos += bl->blocklen;
		
		prev = bl;
		bl = bl->next;
	}

	if (bl != NULL && bl->text == NULL && !read_text(b, bl))
		return FALSE;

	b->lastpos = pos;
	b->lastprev = prev;
	
	if (p_prev != NULL)
		*p_prev = prev;

	*p_bl = bl;
	*p_offset_within_block = wanted_pos - pos;
	return TRUE;
}

static Block *
append_block(b, prev, allocate_text)
Text_bufdata *b;
Block *prev;
bool allocate_text;
{
	Block *bl;
	
	bl = (Block *)alloc(b->apool, sizeof(Block));
	bl->text = (allocate_text) ? alloc(b->apool, b->blocksize) : NULL;
	bl->blocklen = 0;
	bl->file_offset = 0;
	
	if (prev != NULL) {
		bl->next = prev->next;
		prev->next = bl;
	}
	else {
		bl->next = b->blocks;
		b->blocks = bl;
	}
	
	return bl;
}
		
static bool
read_text(b, bl)
Text_bufdata *b;
Block *bl;
{
	bl->text = allocstr(b->apool, b->blocksize);
		
	return (*b->read_func)(b->handle, bl->text,
			       bl->file_offset, bl->blocklen);
}

static bool
lbuf_get_bytes(b, pos, p_start, p_lim)
Text_bufdata *b;
size_t pos;
const char **p_start, **p_lim;
{
	Block *bl;
	size_t offset_within_block;
	
	if (pos >= b->buflen) {
		*p_start = *p_lim = NULL;
		return FALSE;
	}
	
	if (!get_block(b, pos, (Block **)NULL, &bl, &offset_within_block)) {
		static char c;
		
		*p_start = *p_lim = &c;
		return FALSE;
	}
	
	*p_start = bl->text + offset_within_block;
	*p_lim = *p_start + (bl->blocklen - offset_within_block);
	
	return TRUE;
}

static bool
lbuf_get_bytes_before(b, pos, p_start, p_lim)
Text_bufdata *b;
size_t pos;
const char **p_start, **p_lim;
{
	Block *bl;
	size_t offset_within_block;
	
	if (pos == 0 || pos > b->buflen) {
		*p_start = *p_lim = NULL;
		return FALSE;
	}

	if (!get_block(b, pos - 1, (Block **)NULL, &bl, &offset_within_block)) {
		static char c;
		
		*p_start = *p_lim = &c;
		return FALSE;
	}

	*p_start = bl->text;
	*p_lim = bl->text + offset_within_block + 1;
	return TRUE;
}

static bool
set_all_file_offsets(b)
Text_bufdata *b;
{
	size_t junk;
	Block *bl;
	
	if (!get_block(b, b->buflen, (Block **)NULL, &bl, &junk))
		return FALSE;

	b->done_all_file_offsets = TRUE;
	return TRUE;
}

static void
lbuf_delete(b, pos, zapcount)
Text_bufdata *b;
size_t pos, zapcount;
{
	size_t expected_new_buflen, offset_within_block;
	Block *prev, *bl, *next;
		
	if (!b->done_all_file_offsets && !set_all_file_offsets(b))
		return;		/* FIXME: failure return */

	if (pos + zapcount > b->buflen)
		panic("pos out of range in lbuf_delete");

	expected_new_buflen = b->buflen - zapcount;
	
	if (!get_block(b, pos, &prev, &bl, &offset_within_block))
		return;		/* FIXME: failure return */

	if (b->lastpos >= pos)
		b->lastprev = NULL;

	/*  First slice out the part within a block (if any).
	 */
	if (offset_within_block != 0) {
		size_t block_zapcount;

		block_zapcount = zapcount;
		
		if (offset_within_block + block_zapcount >= bl->blocklen) {
			block_zapcount = bl->blocklen - offset_within_block;
		}
		else {
			memmove(bl->text + offset_within_block,
				bl->text + offset_within_block + zapcount,
				bl->blocklen - (offset_within_block+zapcount));
		}
		
		bl->blocklen -= block_zapcount;
		b->buflen -= block_zapcount;

		zapcount -= block_zapcount;
		prev = bl;
		bl = bl->next;
	}

	/*  Now zap any whole blocks.
	 */
	for (; bl != NULL && zapcount >= bl->blocklen; bl = next) {
		next = bl->next;

		if (prev == NULL)
			b->blocks = bl->next;
		else
			prev->next = bl->next;
		
		/* FIXME: push block on free list */

		zapcount -= bl->blocklen;
		b->buflen -= bl->blocklen;
	}
	
	/*  Finally zap the partial block at the end of the deletion (if any).
	 */
	if (zapcount != 0) {
		if (bl->text == NULL && !read_text(b, bl))
			return;	/* FIXME: failure return */
		
		memmove(bl->text,
			bl->text + zapcount,
			bl->blocklen - zapcount);

		bl->blocklen -= zapcount;
		b->buflen -= zapcount;
	}

	if (b->debug)
		dump_blocks(b, "Delete");
	
	if (b->buflen != expected_new_buflen)
		panic("lbuf_delete botch");
}

static void
lbuf_insert(b, pos, instext, inslen)
Text_bufdata *b;
size_t pos;
const char *instext;
size_t inslen;
{
	Block *prev, *bl;
	size_t residue_len, spare_space, expected_new_buflen;
	size_t offset_within_block;
	const char *residue;
		
	if (!b->done_all_file_offsets && !set_all_file_offsets(b))
		return;		/* FIXME: failure return */

	if (pos > b->buflen)
		pos = b->buflen;
	
	if (!get_block(b, pos, &prev, &bl, &offset_within_block))
		return;		/* FIXME: error return */

	if (b->lastpos >= pos)
		b->lastprev = NULL;

	if (b->debug) {
		printf("\nInsert \"%.*s\"\n", (int)inslen, instext);
		dump_blocks(b, "Ins start");
	}

	/*  If we are at the start of a block, back up to the end of the
	 *  previous block.
	 */
	if (offset_within_block == 0 && prev != NULL) {
		bl = prev;
		offset_within_block = bl->blocklen;
	}
	
	/*  If there is space in block for the whole insertion,
	 *  just shuffle bytes around within the block and we're done.
	 */
	if (bl != NULL && bl->blocklen + inslen <= b->blocksize) {
		if (bl->text == NULL && !read_text(b, bl))
			return;	/* FIXME: error return */
		
		transfer(b, bl, offset_within_block, &instext, &inslen);

		if (b->debug)
			dump_blocks(b, "Ins onebl");
		
		return;
	}

	expected_new_buflen = b->buflen + inslen;

	/*  The insertion won't fit in a single block.  First stash a copy
	 *  of any bytes in the current block beyond the insertion point.
	 */
	
	residue_len = (bl != NULL) ? bl->blocklen - offset_within_block : 0;

	if (residue_len == 0) {
		residue = NULL;	/* to satisfy gcc */
	}
	else {
		static char *buf;
		static size_t buflen = 0;
		
		if (residue_len > buflen) {
			if (buf != NULL)
				free(buf);
			buflen = residue_len + 100;
			buf = e_malloc(buflen);
		}

		memcpy(buf, bl->text + offset_within_block, residue_len);
		residue = buf;

		bl->blocklen -= residue_len;
		b->buflen -= residue_len;
	}

	if (b->debug)
		dump_blocks(b, "Pre fill");

	/*  Pour as many bytes as will fit from the front of instext
	 *  into any space in the first block.
	 */
	if (bl != NULL) {
		if (bl->text == NULL && !read_text(b, bl))
			return;	/* FIXME: error return */
	       
		transfer(b, bl, bl->blocklen, &instext, &inslen);
		transfer(b, bl, bl->blocklen, &residue, &residue_len);
		prev = bl;
	}

	if (b->debug)
		dump_blocks(b, "Pre mul ");
	
	/*  The first block (if any) is now full.
	 *  Now insert full blocks until we have less to insert than will
	 *  fit in the spare space in the following block.
	 */

	if (bl != NULL && bl->next != NULL)
		spare_space = b->blocksize - bl->next->blocklen;
	else
		spare_space = 0;

	while (inslen + residue_len > spare_space) {
		bl = append_block(b, prev, TRUE);

		transfer(b, bl, bl->blocklen, &instext, &inslen);
		transfer(b, bl, bl->blocklen, &residue, &residue_len);
		prev = bl;
	}

	if (b->debug)
		dump_blocks(b, "Pre final");
	
	if (inslen + residue_len != 0) {
		size_t offset_for_residue;
		
		if (bl == NULL || bl->next == NULL)
			panic("bl botch in li");
		bl = bl->next;

		offset_for_residue = inslen;
		
		if (bl->text == NULL && !read_text(b, bl))
			return;	/* FIXME: error return */
		
		transfer(b, bl, 0, &instext, &inslen);
		transfer(b, bl, offset_for_residue, &residue, &residue_len);
	}

	if (b->debug)
		dump_blocks(b, "Done final");
	
	if (inslen + residue_len != 0 || b->buflen != expected_new_buflen)
		panic("buflen botch in li");
}

static void
transfer(b, bl, offset_within_block, p_instext, p_inslen)
Text_bufdata *b;
Block *bl;
size_t offset_within_block;
const char **p_instext;
size_t *p_inslen;
{
	size_t to_copy;

	to_copy = b->blocksize - bl->blocklen;

	if (to_copy == 0)
		return;

	if (to_copy > *p_inslen)
		to_copy = *p_inslen;
		
	if (offset_within_block != bl->blocklen) {
		if (offset_within_block > bl->blocklen)
			panic("offset botch in transfer");

		memmove(bl->text + offset_within_block + to_copy,
			bl->text + offset_within_block,
			bl->blocklen - offset_within_block);
	}

	memcpy(bl->text + offset_within_block, *p_instext, to_copy);

	bl->blocklen += to_copy;
	b->buflen += to_copy;
	
	*p_instext += to_copy;
	*p_inslen -= to_copy;
}

static void
dump_blocks(b, label)
Text_bufdata *b;
const char *label;
{
	Block *bl;
	int count;
	
	printf("%-10s [%ld]: |", label, (long)b->buflen);

	count = 0;

	for (bl = b->blocks; bl != NULL; bl = bl->next) {
		int i;

		for (i = 0; i < bl->blocklen; ++i) {
			int ch;

			ch = (bl->text != NULL) ? bl->text[i] : '\0';
				
			if (isalnum(ch) || ch == ' ' || ch == '\t')
				putchar(ch);
			else if (ch == '\n')
				putchar('.');
			else if (bl->text == NULL)
				putchar('@');
			else
				putchar('?');
		}

		for (; i < b->blocksize; ++i)
			putchar('#');

		putchar('|');
	}

	putchar('\n');
}
