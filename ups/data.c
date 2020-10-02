/* data.c - higher level routines to read and write target data */

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


/* @(#)data.c	1.17 04 Jun 1995 (UKC) */
char ups_data_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>

#include <sys/types.h>
#include <signal.h>
#include <setjmp.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "data.h"
#include "obj_stack.h"
#include "obj_target.h"
#include "state.h"
#include "util.h"


/*  BUG: we assume here that a register is the same size as
 *       a pointer, and that address and data registers are
 *       the same.
 */
#define REGBYTES	sizeof(taddr_t)

static volatile int Got_sigill;

static jmp_buf Sigill_env;

static void catch_sigill PROTO((int sig));
static int charval PROTO((int n));

static void
catch_sigill(unused_sig)
int unused_sig;
{
	Got_sigill = 1;
	longjmp(Sigill_env, 1);
}

/*  Extract a floating point value from the value_t union.
 *
 *  On the VAX (and possibly other machines) certain bit patterns cause
 *  exceptions when treated as a floating point value, so we bracket
 *  the extraction with a handler for these exceptions.
 */
const char *
get_real(words_big_endian, vl, want_hex, num_bytes)
bool words_big_endian;
value_t vl;
bool want_hex;
int num_bytes;
{
#if HAVE_LONG_DOUBLE && (SIZEOF_LONG_DOUBLE != SIZEOF_DOUBLE)
#define LG_FMT "Lg"
	static char buf[120];
	long double d;
#else
#define LG_FMT "g"
	static char buf[60];
	double d;
#endif
	void (*old_sigill_func)PROTO((int sig));
	void (*old_sigfpe_func)PROTO((int sig));

	if (want_hex) {
	    switch (num_bytes) {
		    case sizeof(float):
			sprintf(buf, "<0x%08x>", vl.vl_int);
			break;
		    case sizeof(double):
			{
			unsigned word[2];
 
			if (words_big_endian) {
				word[0] = vl.vl_ints[0];
				word[1] = vl.vl_ints[1];
			}
			else {
				word[0] = vl.vl_ints[1];
				word[1] = vl.vl_ints[0];
			}
			sprintf(buf, "<0x%08x%08x>", word[0], word[1]);
			}
			break;
#if HAVE_LONG_DOUBLE
		    case sizeof(int)*3:
			{
			unsigned word[3];

			if (words_big_endian) {
				word[0] = vl.vl_ints[0];
				word[1] = vl.vl_ints[1];
				word[2] = vl.vl_ints[2];
			}
			else {
				word[0] = vl.vl_ints[2];
				word[1] = vl.vl_ints[1];
				word[2] = vl.vl_ints[0];
			}
			sprintf(buf, "<0x%08x%08x%08x>", word[0], word[1], word[2]);
			}
			break;
		    case sizeof(int)*4:
			{
			unsigned word[4];

			if (words_big_endian) {
				word[0] = vl.vl_ints[0];
				word[1] = vl.vl_ints[1];
				word[2] = vl.vl_ints[2];
				word[3] = vl.vl_ints[3];
			}
			else {
				word[0] = vl.vl_ints[3];
				word[1] = vl.vl_ints[2];
				word[2] = vl.vl_ints[1];
				word[3] = vl.vl_ints[0];
			}
			sprintf(buf, "<0x%08x%08x%08x%08x>", word[0], word[1], word[2], word[3]);
			}
			break;
#endif
		}
		return buf;
	}

	old_sigill_func = signal(SIGILL, catch_sigill);
	old_sigfpe_func = signal(SIGFPE, catch_sigill);
	Got_sigill = FALSE;
	
	if (setjmp(Sigill_env) == 0) {
	    switch (num_bytes) {
		case sizeof(float):
		    d = vl.vl_float;
		    break;
		case sizeof(double):
		    d = vl.vl_double;
		    break;
#if HAVE_LONG_DOUBLE && (SIZEOF_LONG_DOUBLE != SIZEOF_DOUBLE)
		case sizeof(long double):
		    d = vl.vl_longdouble;
		    break;
#endif
		default:
		    d = 0; /* to satisfy gcc */
		    break;
	    }
	}
	else
		d = 0;

	(void) signal(SIGILL, old_sigill_func);
	(void) signal(SIGILL, old_sigfpe_func);

	if (Got_sigill) {
		/*  Note: get_real checks for this output when reading
		 *        a value so the exact format of the output
		 *	  is important.
		 */
	    switch (num_bytes) {
		case sizeof(float):
			sprintf(buf, "<illegal float 0x%08x>", vl.vl_int);
			break;
		case sizeof(double):
		    {
			unsigned word[2];

			if (words_big_endian) {
				word[0] = vl.vl_ints[0];
				word[1] = vl.vl_ints[1];
			}
			else {
				word[0] = vl.vl_ints[1];
				word[1] = vl.vl_ints[0];
			}
			
			sprintf(buf, "<illegal double 0x%x%08x>",
				word[0], word[1]);
		    }
		    break;
#if HAVE_LONG_DOUBLE
		case sizeof(int)*3:
		    sprintf(buf, "<illegal long double %lu bytes %s [0]0x%08x [1]0x%08x [2]0x%08x>",
			    (unsigned long)sizeof(long double),
			    (words_big_endian ? "1st word on left" : "1st word on right"),
			    vl.vl_ints[0], vl.vl_ints[1], vl.vl_ints[2]);
		    break;
		case sizeof(int)*4:
		    sprintf(buf, "<illegal long double %lu bytes %s [0]0x%08x [1]0x%08x [2]0x%08x [3]0x%08x>",
			    (unsigned long)sizeof(long double),
			    (words_big_endian ? "1st word on left" : "1st word on right"),
			    vl.vl_ints[0], vl.vl_ints[1], vl.vl_ints[2], vl.vl_ints[3]);
		    break;
#endif
		}
	}
	else {
		char *s;

		switch (num_bytes) {
		    case sizeof(float):
			sprintf(buf, "%.6" LG_FMT, d);
			break;
		    case sizeof(double):
			sprintf(buf, "%.12" LG_FMT, d);
			break;
#if HAVE_LONG_DOUBLE && (SIZEOF_LONG_DOUBLE != SIZEOF_DOUBLE)
		    case sizeof(long double):
			sprintf(buf, "%.18" LG_FMT, d);
			break;
#endif
		}

		/*  We always want a decimal point even if the value
		 *  is integral.
		 */
		s = buf;
		if (*s == '-')
			++s;
		while (isdigit(*s))
			++s;
		if (*s == '\0')
			strcpy(s, ".0");
	}

	return buf;
}

/*  Read a string from the data area.
 */
int
dgets(xp, addr, optr, max_nbytes)
target_t *xp;
taddr_t addr;
register char *optr;
int max_nbytes;
{
	char ibuf[4], *olim;
	register char *iptr;

	olim = optr + max_nbytes - 1;
	iptr = ibuf + sizeof(ibuf);
	
	while (optr < olim) {
		if (iptr == ibuf + sizeof(ibuf)) {
			if (dread(xp, addr, ibuf, sizeof(ibuf)) != 0)
				return -1;
			iptr = ibuf;
			addr += sizeof(ibuf);
		}
		if (*iptr == '\0')
			break;
		*optr++ = *iptr++;
	}
	
	*optr++ = '\0';
	
	return 0;
}

/*  Nominal address of the registers. Recognised by dread()
 *
 *  Just pick a value that can't be confused with a stack or data address.
 *
 *  BUG: this is unsafe now that we have mmap() and shared libraries -
 *  something could actually be mapped at this address.
 */
#define REG_ADDR	((taddr_t)(0xf0000000 - -REG_MIN_REGNO))

/*  Maximum number of registers.  Used only for limiting the range of
 *  pseudo addresses that will be interpreted as register numbers.
 */
#define MAX_REGS	256

/*  Is addr an encoded register address?
 */
#define IS_REG_ADDR(addr)  (addr >= (REG_ADDR - -REG_MIN_REGNO) && \
			    addr < REG_ADDR + MAX_REGS)

/*  Convert a register number to an "address" that will be recognised by
 *  dread().
 */
taddr_t
regno_to_addr(regno)
int regno;
{
	if (regno > MAX_REGS)
		panic("bad reg addr in rta");
	return REG_ADDR + regno;
}

int
dread_fpval(xp, addr, is_reg, num_bytes, buf)
target_t *xp;
taddr_t addr;
bool is_reg;
int num_bytes;
char *buf;
{
	int res;
	fpval_t fpval;

	if (IS_REG_ADDR(addr)) {
		res = xp_read_fpreg(xp, (int)(addr - REG_ADDR),
							num_bytes, &fpval);
	}
	else if (is_reg) {
		/*  A register that's been saved on the stack.
		 */
		res = xp_read_fpval(xp, addr, num_bytes, &fpval);
	}
	else
		return dread(xp, addr, buf, num_bytes);

	if (res != 0)
		return -1;

	switch (num_bytes) {
	    case sizeof(float):
		*(float *)buf = fpval.f;
		break;
	    case sizeof(double):
		*(double *)buf = fpval.d;
		break;
#if HAVE_LONG_DOUBLE && (SIZEOF_LONG_DOUBLE != SIZEOF_DOUBLE)
	    case sizeof(long double):
		*(long double *)buf = fpval.ld;
		break;
#endif
	}

	return 0;
}

int
dread_addrval(xp, addr, buf)
target_t *xp;
taddr_t addr;
taddr_t *buf;
{
	int addrsize = xp_get_addrsize(xp);
   
	if (addrsize == sizeof(taddr_t) * 8) {
		return dread(xp, addr, buf, sizeof(taddr_t));
	}
	else if (addrsize == 32) {
		unsigned int val;

		if (dread(xp, addr, &val, sizeof(val)) != 0)
			return -1;

		*buf = val;
	}
	else {
		panic("unsupported target address size in dread_addrval");
	}

	return 0;
}

/*  Read n bytes into buf from the data or stack area of the target process.
 *  We deduce from addr whether we are supposed to read the stack or
 *  the data area.
 *
 *  Return 0 for success, -1 for failure.
 */
int
dread(xp, addr, buf, nbytes)
target_t *xp;
taddr_t addr;
voidptr buf;
size_t nbytes;
{
	if (IS_REG_ADDR(addr)) {
		taddr_t regval;
		int regno;
		char *rptr;

		rptr = (char *)&regval;
		regno = addr - REG_ADDR;

		if (xp_readreg(xp, regno, &regval) != 0)
			return -1;
		
		if (nbytes == REGBYTES) {
			memcpy(buf, rptr, REGBYTES);
		}
		else if (nbytes > 0 && nbytes < REGBYTES) {
			if (xp->xp_words_big_endian)
				rptr += REGBYTES - nbytes;

			memcpy(buf, rptr, nbytes);
		}
		else if (nbytes == 2 * REGBYTES) {
			int regno2;

			/*  We assume this is a pair of registers.
			 *  BUG: why?
			 */
			memcpy(buf, rptr, REGBYTES);
			
			regno2 = regno + 1;
#if (defined ARCH_BSDI386) || (defined ARCH_LINUX386)
			if (regno == 3)
				regno2 = 6;
#endif
			if (xp_readreg(xp, regno2, &regval) != 0)
				return -1;
			
			memcpy((char *)buf + REGBYTES, rptr, REGBYTES);
		}
		else {
			/*  BUG: we can get here if we got an address from
			 *       the target in the range we use for register
			 *       addresses, so don't panic.
			 *
			 *  We need to dump the magic address scheme for
			 *  registers.
			 */
			return -1;
		}
		return 0;
	}

	return xp_read_data(xp, addr, buf, nbytes);
}

int
dwrite_addrval(xp, addr, buf)
target_t *xp;
taddr_t addr;
const taddr_t *buf;
{
	int addrsize = xp_get_addrsize(xp);
   
	if (addrsize == sizeof(taddr_t) * 8) {
		return dwrite(xp, addr, buf, sizeof(taddr_t));
	}
	else if (addrsize == 32) {
		unsigned int val = *buf;

		return dwrite(xp, addr, &val, sizeof(val));
	}
	else {
		panic("unsupported target address size in dwrite_addrval");
		return -1;
	}
}

int
dwrite(xp, addr, buf, nbytes)
target_t *xp;
taddr_t addr;
constvoidptr buf;
size_t nbytes;
{
	if (!target_process_exists(xp))
		panic("dwrite called with no target process");

	if (IS_REG_ADDR(addr)) {
		taddr_t regval;
		int regno;
		char *rptr;

		rptr = (char *)&regval;
		regno = addr - REG_ADDR;

		if (nbytes == REGBYTES) {
			memcpy(rptr, buf, REGBYTES);
		}
		else if (nbytes > 0 && nbytes < REGBYTES) {
			if (xp->xp_words_big_endian)
				rptr += REGBYTES - nbytes;

			memcpy(rptr, buf, nbytes);
		}
		else if (nbytes == 2 * REGBYTES) {
			/*  We assume this is a pair of registers.
			 *  BUG: why?
			 */
			memcpy(rptr, (char *)buf + REGBYTES, REGBYTES);
			
			if (xp_setreg(xp, regno + 1, regval) != 0)
				return -1;
			
			memcpy(rptr, buf, REGBYTES);
		}
		else {
			panic("nbytes botch in dwrite");
		}
		
		return xp_setreg(xp, regno, regval);
	}

	return xp_write_data(xp, addr, buf, nbytes);
}

taddr_t
adjust_saved_reg_addr(xp, addr, size)
target_t *xp;
taddr_t addr;
size_t size;
{
	if (size < REGBYTES && xp->xp_words_big_endian)
		addr += REGBYTES - size;

	return addr;
}

static int
charval(n)
int n;
{
	 n = (unsigned char)n;
	 return isprint(n) ? n : '?';
}

void
dump_stack_to_file(xp, name)
target_t *xp;
const char *name;
{
	FILE *ofp;
	taddr_t sp, fp, ap, pc, addr;
	int val, nzeroes;
	const char *arg, *label;
	bool overwrite;
	bool is_labelled;

	if (xp_get_state(xp) == TS_NOTR) {
		errf("Target not running");
		return;
	}

	pc = xp_getreg(xp, UPSREG_PC);
	sp = xp_getreg(xp, UPSREG_SP);
	fp = xp_getreg(xp, UPSREG_FP);
	ap = xp_getreg(xp, UPSREG_AP);

	if (sp % sizeof(val) != 0) {
		errf("SP (0x%lx) not a multiple of %ld", sp, (long)sizeof(val));
		return;
	}

	if (!get_debug_output_path(name, "ups-stack", &arg, &name, &overwrite))
		return;

	if (*arg != '\0') {
		errf("Usage: dumpstack [> file]");
		return;
	}
		     
	if (!fopen_new_file("output file", name, overwrite, &ofp))
		return;
	
	errf("Dumping stack to file %s", name);

	fprintf(ofp, "sp:%08lx fp:%08lx ap:%08lx pc:%08lx\n", sp, fp, ap, pc);

	nzeroes = 0;
	for (addr = sp; xp_read_data(xp, addr, (char *)&val, 4)==0; addr += 4) {

		/*  On the BSDi box, dread() goes on and on succeeding,
		 *  generating huge stack dumps, hence this boredom theshold.
		 */
		if (val != 0) {
			nzeroes = 0;
		}
		else {
			if (++nzeroes == 1000) {
				fputs(
				   "Giving up after 1000 consecutive zeroes\n", 
									ofp);
				break;
			}
		}

		label = stack_addr_label(addr, &is_labelled);
		(void) fprintf(ofp, is_labelled ? "%32s " : "%-32s ", label);
		(void) fprintf(ofp, "%8lx |%8x|  |%12d|  |%c%c%c%c|\n",
				addr, val, val,
				charval(val),
				charval(val >> 8),
				charval(val >> 16),
				charval(val >> 24));
	}

	if (fclose_new_file("output file", name, TRUE, ofp))
		errf("Stack dump complete");
}

/*
**   Read target memory at addr and format it for display in the buffer.
*/
int
dump_to_buffer (xp, addr, length, grp, buffer)
target_t *xp;
taddr_t addr;    /* Address to dump */
int length;	 /* length to dump */
int grp;	 /* group as bytes, shorts, longs */
char *buffer;    /* Place output here */
{
    char* ascii_text;
    char* hex_text;
    const char* fmt;
    int   fmt_len = grp*2+1;
    int   len = length;
    unsigned char target_memory[16];
    unsigned char* target_mem = target_memory;

    if ( len > 16)
       len = 16;


    sprintf(buffer, "%08lx: ", addr);
    hex_text = buffer+10;
    /* Get the memory */
    if ( dread(xp, addr, target_memory, len) )
    {
	strcat(hex_text, "Target memory read error\n");
	return -1;
    }

    /* decide where the ascii output will go */
    ascii_text = hex_text+ (16/grp)*fmt_len+1;

    while (len)
    {
	unsigned long val;
	if ( len < grp)
	   grp = len;
	switch (grp)
	{
	case 1:
	    val = *target_mem;
	    fmt = "%02lx ";
	    fmt_len = 3;
	    break;
	case 3:
	    grp = 2;
	    /* fall into next case */
	    /* fall through */
	case 2:
	    val = *(unsigned short*)target_mem;
	    fmt = "%04lx ";
	    fmt_len = 5;
	    break;
	case 4:
	    val = *(unsigned long*)target_mem;
	    fmt = "%08lx ";
	    fmt_len = 9;
	    break;
	default:
	    strcat(buffer,"Invalid size!\n");
	    return -1;
	}
	sprintf(hex_text,fmt,val);
	target_mem+=grp;
	len -= grp;
	hex_text += fmt_len;
    }
    while ( hex_text < ascii_text)
       *hex_text++ = ' ';

    target_mem = target_memory;
    len = length;
    while(len--)
    {
	unsigned char uc = *target_mem++;
	if ( uc < 0x80 && isprint(uc))
	   *ascii_text++ = (char)uc;
	else
	   *ascii_text++ = '.';
    }
    *ascii_text++ = '\n';
    *ascii_text++ = 0;
    return 0;
}


