/* data.h - header file for data.c */

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


/* @(#)data.h	1.6 09 Apr 1995 (UKC) */

typedef union {
	char vl_char;
	unsigned char vl_uchar;
	short vl_short;
	unsigned short vl_ushort;
	int vl_int;
	unsigned int vl_uint;
	long vl_long;
	unsigned long vl_ulong;
#if HAVE_LONG_LONG
	long long vl_longlong;
	unsigned long long vl_ulonglong;
#endif
	float vl_float;
	double vl_double;
#if HAVE_LONG_DOUBLE
	long double vl_longdouble;
	int vl_ints[sizeof(long double)/sizeof(int)];	/* for illegal double values */
#else
	int vl_ints[sizeof(double)/sizeof(int)];	/* for illegal double values */
#endif
	int vl_logical;
	taddr_t vl_addr;
} value_t;

void dump_uarea_to_file PROTO((target_t *xp, const char *name));
void dump_stack_to_file PROTO((target_t *xp, const char *name));

int dgets PROTO((target_t *xp, taddr_t addr, char *optr, int max_nbytes));
int dread PROTO((target_t *xp, taddr_t addr, void *buf, size_t nbytes));
int dread_fpval PROTO((target_t *xp, taddr_t addr,
				bool is_reg, int num_bytes, char *buf));
int dwrite PROTO((target_t *xp, taddr_t addr, const void *buf, size_t nbytes));
taddr_t regno_to_addr PROTO((int regno));
const char *get_real PROTO((bool words_big_endian,
			    value_t vl, bool want_hex, int num_bytes));

taddr_t adjust_saved_reg_addr PROTO((target_t *xp, taddr_t addr, size_t size));
int dump_to_buffer PROTO((target_t *xp, taddr_t addr, int len, int grp,
			  char *buffer));

