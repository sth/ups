/* hash.c - chained hash table */

/* @(#)hash.c	1.3 24/5/95 (UKC) */
char mtrprog_hash_rcsid[] = "$Id$";

/* Copyright 1992 Mark Russell, University of Kent at Canterbury
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


#include <stdlib.h>
#include <string.h>
#include <local/ukcprog.h>

#include "hash.h"

typedef struct vlist_s {
	hash_value_t vl_value;
	struct vlist_s *vl_next;
} vlist_t;

typedef struct hash_s {
	hash_key_t ha_key;
	size_t ha_keylen;
	int ha_nvalues;
	union {
		vlist_t *hau_vlist;
		hash_value_t hau_value;
	} ha_u;
	struct hash_s *ha_next;
} hash_t;

#define ha_vlist	ha_u.hau_vlist
#define ha_value	ha_u.hau_value

typedef struct qhash_s {
	hash_t *qh_hash;
	hash_t *qh_prev;
	int qh_pos;
} qhash_t;

struct hashtab_s {
	int ht_size;
	hash_t **ht_tab;
	hash_key_t ht_qkey;
	qhash_t *ht_qhash;
	alloc_pool_t *ht_apool;
};

static vlist_t *Vlist_freelist = NULL;
static hash_t *Hash_freelist = NULL;

static void add_value PROTO((alloc_pool_t *ap, hash_t *ha, hash_value_t value));
static hash_t *add_hash PROTO((hashtab_t *ht, int pos, hash_key_t key, size_t keylen));
static void delete_hash PROTO((hashtab_t *ht, int pos, hash_t *prev, hash_t *ha));
static hash_t *find_hash PROTO((hashtab_t *ht, hash_key_t key, size_t keylen,
						int *p_pos, hash_t **p_prev));
static void copy_values PROTO((hash_t *ha, hashvalues_t *hv));
static void push_vlist PROTO((alloc_pool_t *ap, hash_t *ha, hash_value_t value));
static void delete_value PROTO((hash_t *ha, hash_value_t value));
static void delete_all_values PROTO((hash_t *ha));

#define DESIRED_CHAIN_LENGTH	4

static void
push_vlist(ap, ha, value)
alloc_pool_t *ap;
hash_t *ha;
hash_value_t value;
{
	vlist_t *vl;

	if (Vlist_freelist == NULL) {
		vl = (vlist_t *)alloc(ap, sizeof(vlist_t));
	}
	else {
		vl = Vlist_freelist;
		Vlist_freelist = vl->vl_next;
	}

	vl->vl_value = value;
	vl->vl_next = ha->ha_vlist;
	ha->ha_vlist = vl;
}

static void
add_value(ap, ha, value)
alloc_pool_t *ap;
hash_t *ha;
hash_value_t value;
{
	if (ha->ha_nvalues == 0) {
		ha->ha_value = value;
	}
	else {
		if (ha->ha_nvalues == 1) {
			hash_value_t first_value;

			first_value = ha->ha_value;
			ha->ha_vlist = NULL;

			push_vlist(ap, ha, first_value);
		}

		push_vlist(ap, ha, value);
	}

	++ha->ha_nvalues;
}

static void
delete_value(ha, value)
hash_t *ha;
hash_value_t value;
{
	if (ha->ha_nvalues == 0)
		panic("delete_value: zero values");

	if (ha->ha_nvalues == 1) {
		if (ha->ha_value != value)
			panic("delete_value: value mismatch");
		ha->ha_vlist = NULL;
	}
	else {
		vlist_t *vl, *prev;

		prev = NULL;		/* to satisfy gcc */

		for (vl = ha->ha_vlist; vl != NULL; vl = vl->vl_next) {
			if (vl->vl_value == value)
				break;
			prev = vl;
		}

		if (vl == NULL)
			panic("delete_value: no values match");
		
		if (prev == NULL)
			ha->ha_vlist = vl->vl_next;
		else
			prev->vl_next = vl->vl_next;
		
		vl->vl_next = Vlist_freelist;
		Vlist_freelist = vl;

		if (ha->ha_nvalues == 2) {
			hash_value_t last_value;

			last_value = ha->ha_vlist->vl_value;

			ha->ha_vlist->vl_next = Vlist_freelist;
			Vlist_freelist = ha->ha_vlist;

			ha->ha_value = last_value;
		}
	}

	--ha->ha_nvalues;
}

static void
delete_all_values(ha)
hash_t *ha;
{
	if (ha->ha_nvalues > 1) {
		vlist_t *vl, *prev;

		prev = NULL;	/* to satisfy gcc */

		for (vl = ha->ha_vlist; vl != NULL; vl = vl->vl_next)
			prev = vl;

		prev->vl_next = Vlist_freelist;
		Vlist_freelist = ha->ha_vlist;

		ha->ha_vlist = NULL;	/* for safety */
	}

	ha->ha_nvalues = 0;
}

static hash_t *
add_hash(ht, pos, key, keylen)
hashtab_t *ht;
int pos;
hash_key_t key;
size_t keylen;
{
	hash_t *ha;

	if (Hash_freelist == NULL) {
		ha = (hash_t *)alloc(ht->ht_apool, sizeof(hash_t));
	}
	else {
		ha = Hash_freelist;
		Hash_freelist = ha->ha_next;
	}

	ha->ha_key = key;
	ha->ha_keylen = keylen;
	ha->ha_nvalues = 0;
	ha->ha_vlist = NULL;

	ha->ha_next = ht->ht_tab[pos];
	ht->ht_tab[pos] = ha;

	return ha;
}

static void
delete_hash(ht, pos, prev, ha)
hashtab_t *ht;
int pos;
hash_t *prev, *ha;
{
	if (ha->ha_nvalues != 0)
		panic("delete_hash: nvalues != 0");

	if (prev != NULL)
		prev->ha_next = ha->ha_next;
	else
		ht->ht_tab[pos] = ha->ha_next;

	ha->ha_next = Hash_freelist;
	Hash_freelist = ha;
}

static hash_t *
find_hash(ht, key, keylen, p_pos, p_prev)
hashtab_t *ht;
hash_key_t key;
size_t keylen;
int *p_pos;
hash_t **p_prev;
{
	hash_t *ha, *prev;
	int pos, i, hlen;
	unsigned hash;

	if (key == NULL)
		panic("find_hash: NULL key");
	
	if (key == ht->ht_qkey) {
		qhash_t *qh;

		qh = ht->ht_qhash;
		*p_pos = qh->qh_pos;
		*p_prev = qh->qh_prev;
		return qh->qh_hash;
	}

	/*  Don't use all of a long key for the hash.
	 */
	hlen = (keylen <= 16) ? keylen : 16;
	
	/*  This hash function is from an article <15027@ulysses.att.com> 
	 *  posted by Phong Vo from AT&T.  He said it was good ...
	 */
	hash = 0;
	for (i = 0; i < hlen; ++i)
		hash = (hash << 8) + (hash << 2) + key[i];
	pos = hash % ht->ht_size;

	prev = NULL;
	for (ha = ht->ht_tab[pos]; ha != NULL; ha = ha->ha_next) {
		if (ha->ha_keylen == keylen &&
				memcmp(ha->ha_key, key, (size_t)keylen) == 0)
			break;
		prev = ha;
	}

	*p_pos = pos;
	*p_prev = prev;
	return ha;
}

static void
copy_values(ha, hv)
hash_t *ha;
hashvalues_t *hv;
{
	if (ha->ha_nvalues > hv->hv__max_nvalues) {
	  /* RGA: do loop contributed by Jody Goldberg <jodygold@sanwafp.COM>
	   * on  Mon Jul 27 14:36:55 1998. Replaces simple if statement
	   */
            do
                     hv->hv__max_nvalues *= 2;
              while (ha->ha_nvalues > hv->hv__max_nvalues);

		if (hv->hv_values != hv->hv__static_values)
			free((char *)hv->hv_values);

		hv->hv_values = (hash_value_t *)e_malloc(hv->hv__max_nvalues *
							sizeof(hash_value_t ));
	}

	if (ha->ha_nvalues == 1) {
		hv->hv_values[0] = ha->ha_value;
	}
	else {
		hash_value_t *vp;
		vlist_t *vl;

		vp = hv->hv_values;
		for (vl = ha->ha_vlist; vl != NULL; vl = vl->vl_next)
			*vp++ = vl->vl_value;
	}

	hv->hv_nvalues = ha->ha_nvalues;
}

hashtab_t *
hash_create_tab(ap, probable_size)
alloc_pool_t *ap;
int probable_size;
{
	hashtab_t *ht;
	int i, size;
	hash_t **tab;

	size = probable_size / DESIRED_CHAIN_LENGTH;
	if (size < 20)
		size = 20;

	tab = (hash_t **)alloc(ap, sizeof(hash_t *) * size);

	for (i = 0; i < size; ++i)
		tab[i] = NULL;

	ht = (hashtab_t *)alloc(ap, sizeof(hashtab_t));

	ht->ht_size = size;
	ht->ht_tab = tab;
	ht->ht_apool = ap;

	return ht;
}

void
hash_enter_key(ht, key, keylen)
hashtab_t *ht;
hash_key_t key;
size_t keylen;
{
	hash_t *ha, *prev;
	int pos;

	ha = find_hash(ht, key, keylen, &pos, &prev);
	
	if (ha == NULL)
		add_hash(ht, pos, key, keylen);
}

void
hash_enter(ht, key, keylen, value)
hashtab_t *ht;
hash_key_t key;
size_t keylen;
hash_value_t value;
{
	hash_t *ha, *prev;
	int pos;

	ha = find_hash(ht, key, keylen, &pos, &prev);

	if (ha == NULL)
		ha = add_hash(ht, pos, key, keylen);

	add_value(ht->ht_apool, ha, value);
}

hashvalues_t *
hash_make_hashvalues()
{
	hashvalues_t *hv;

	hv = (hashvalues_t *)e_malloc(sizeof(hashvalues_t));

	hv->hv_nvalues = 0;
	hv->hv__max_nvalues = sizeof(hv->hv__static_values) / sizeof(char *);
	hv->hv_values = hv->hv__static_values;

	return hv;
}

void
hash_free_hashvalues(hv)
hashvalues_t *hv;
{
	if (hv->hv_values != hv->hv__static_values)
		free((char *)hv->hv_values);
	
	free((char *)hv);
}

bool
hash_lookup_key(ht, key, keylen, hv)
hashtab_t *ht;
hash_key_t key;
size_t keylen;
hashvalues_t *hv;
{
	hash_t *ha, *prev;
	int pos;

	ha = find_hash(ht, key, keylen, &pos, &prev);
	
	if (ha != NULL) {
		copy_values(ha, hv);
		return TRUE;
	}
	
	return FALSE;
}
	
hash_value_t 
hash_lookup(ht, key, keylen)
hashtab_t *ht;
hash_key_t key;
size_t keylen;
{
	hash_t *ha, *prev;
	int pos;

	ha = find_hash(ht, key, keylen, &pos, &prev);
	
	if (ha == NULL || ha->ha_nvalues == 0)
		return NULL;

	return (ha->ha_nvalues == 1) ? ha->ha_value : ha->ha_vlist->vl_value;
}

void
hash_delete_key(ht, key, keylen)
hashtab_t *ht;
hash_key_t key;
size_t keylen;
{
	hash_t *ha, *prev;
	int pos;

	ha = find_hash(ht, key, keylen, &pos, &prev);
	if (ha == NULL)
		panic("hash_delete_key: no matching entry");
	
	delete_all_values(ha);
	delete_hash(ht, pos, prev, ha);
}

void
hash_delete(ht, key, keylen, value)
hashtab_t *ht;
hash_key_t key;
size_t keylen;
hash_value_t value;
{
	hash_t *ha, *prev;
	int pos;

	ha = find_hash(ht, key, keylen, &pos, &prev);
	if (ha == NULL)
		panic("hash_delete: no matching entry");
	
	delete_value(ha, value);

	if (ha->ha_nvalues == 0)
		delete_hash(ht, pos, prev, ha);
}

char *
hash_stats(ht)
hashtab_t *ht;
{
	hash_t **hp, **lim, *ha;
	long nkeys, nvals, nchains, maxchn;

	hp = ht->ht_tab;
	lim = hp + ht->ht_size;

	nkeys = nvals = nchains = maxchn = 0;

	for (; hp < lim; ++hp) {
		long last_nkeys;

		if (*hp == NULL)
			continue;

		++nchains;

		last_nkeys = nkeys;

		for (ha = *hp; ha != NULL; ha = ha->ha_next) {
			++nkeys;
			nvals += ha->ha_nvalues;
		}

		if (nkeys - last_nkeys > maxchn)
			maxchn = nkeys - last_nkeys;
	}

	return strf(
	    "keys:%ld vals:%ld chains:%ld/%d %ld%% maxchn:%ld avgchn:%ld",
				nkeys, nvals,
				nchains, ht->ht_size,
				(nchains * 100) / ht->ht_size,
				maxchn,
				(nchains > 0) ? nkeys / nchains : 0);
}

const char *
hash_apply(ht, func, arg, hv)
hashtab_t *ht;
hash_apply_func_t func;
const char *arg;
hashvalues_t *hv;
{
	qhash_t qhash;
	int pos, size;
	const char *res;
	hash_t **tab, *ha, *next;

	tab = ht->ht_tab;
	size = ht->ht_size;
	ht->ht_qhash = &qhash;
	res = NULL;

	for (pos = 0; pos < size; ++pos) {

		qhash.qh_pos = pos;
		qhash.qh_prev = NULL;

		for (ha = tab[qhash.qh_pos]; ha != NULL; ha = next) {
			ht->ht_qkey = ha->ha_key;
			qhash.qh_hash = ha;

			next = ha->ha_next;
			copy_values(ha, hv);
			res = (*func)(ht, arg, ha->ha_key, ha->ha_keylen, hv);
			
			if (res != NULL)
				goto endloop;
			
			/*  Only update qh_prev if (*func)() didn't zap ha.
			 */
			if (ha->ha_next == next)
				qhash.qh_prev = ha;
		}
	}

endloop:
	ht->ht_qkey = NULL;
	ht->ht_qhash = NULL;	/* not necessary, but tidy */

	return res;
}
