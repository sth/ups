/* xc_symparse.h - header file for xc_symparse.c  */

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


/* @(#)xc_symparse.h	1.1 22/12/93 (UKC) */

typedef struct symdesc_s symdesc_t;

symdesc_t *make_symdesc PROTO((alloc_pool_t *ap, const char *typeptr,
			       const char *strings, size_t strings_size,
			       size_t ntypes,
			       char **varaddrs, taddr_t data_addr,
			       const char *iptr));

void free_symdesc PROTO((symdesc_t *sd));

block_t *unpack_one_block PROTO((symdesc_t *sd));
