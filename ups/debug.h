/* debug.h - bit values for the -debug flag */

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


/* $Id$ */

/*  These flags are set by the -dbflags command line switch.
 *  They mostly enable debugging output (useful in the first
 *  stages of porting ups to a machine with no existing useful
 *  debugger).
 */

/*  This flag has no global purpose - it is used (usually temporarily)
 *  to alter the behaviour of bits of code for testing.
 */
#define DBFLAG_MISC			0x00
 
/*  If stack.c is compiled with -DSTACK_DEBUG and this flag is set,
 *  stack frame information is printed to stderr by build_stack_trace().
 */
#define DBFLAG_STACK			0x00

/*  This flag makes some of the as_*.c disassemblers try to produce
 *  output that looks more like the output of dbx (even if this means
 *  making the output worse).
 */
#define DBFLAG_DBXASM			0x0001

/*  This flag makes the $debug commands asm and asmsrc label instrunctions
 *  with addresses of the form "main+0x44" rather than absolute hex.
 */
#define DBFLAG_ASM_OFFSET_ADDRS		0x0002

/*  This flag makes the output of asm and asmsrc line buffered.
 */
#define DBFLAG_ASM_LINEBUF		0x0004

/*  This flag makes ups print out C interpreter pseudo-assembler
 *  when breakpoint code is edited.
 */
#define DBFLAG_SHOW_CI_CODE		0x0008

/*  If proc.c is compiled with -DPTRACE_DEBUG and this flag is set,
 *  the arguments and results of ptrace calls are logged to stderr.
 */
#define DBFLAG_PTRACE			0x0010

/*  This flag enables the boundary moving code in ups, which is now
 *  switched off by default.
 */
#define DBFLAG_MVLINES			0x0020

/*  This flag turns off the stop button in ups (i.e. you can't select
 *  stop to stop the target).  I think this is causing spurious signals.
 */
#define DBFLAG_NO_STOP			0x0040

/*  Switch on editing assertion checks.
 */
#define DBFLAG_EDITCHECKS		0x0080

/*  Dump memory map info on target startup.
 */
#define DBFLAG_DUMP_MMAP_INFO		0x0100

/*  If UPS built with -DWANT_DEBUG dump symbol table information.
 */
#define DBFLAG_DUMP_SYMTAB		0x0200

/*  If UPS built with -DWANT_DEBUG dump source line number info.
 */
#define DBFLAG_DUMP_LNOS		0x0400

extern unsigned long Debug_flags;
