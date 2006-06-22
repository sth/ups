/* xc_machine.c - execute interpreter code */

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


/* @(#)xc_machine.c	1.5 25 May 1995 (UKC) */
char ups_xc_machine_c_rcsid[] = "$Id$";

#ifndef WANT_JUMPTAB
#if defined(__GNUC__) && !defined(__CXC__)
#define WANT_JUMPTAB	1
#else
#define WANT_JUMPTAB	0
#endif
#endif

#include <mtrprog/ifdefs.h>

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <local/ukcprog.h>
#include <mtrprog/utils.h>

#include "ups.h"
#include "symtab.h"
#include "ci.h"
#include "xc_opcodes.h"
#include "xc_machine.h"
#include "xc_builtins.h"

/* We would like longjmp()/setjmp() which do not change sigmask. */
/* Prefer BSD versions if available. */
#if HAVE__LONGJMP && HAVE__SETJMP
#define ups_longjmp(env,val)	_longjmp(env,val)
#define ups_setjmp(env)		_setjmp(env)
#elif HAVE_SIGLONGJMP && HAVE_SIGSETJMP
#define ups_longjmp(env,val)	siglongjmp(env,val)
#define ups_setjmp(env)		sigsetjmp(env,0)
#else
#define ups_longjmp(env,val)	longjmp(env,val)
#define ups_setjmp(env)		setjmp(env)
#endif

static void spfailed PROTO((long expected, long got, long location));
static int callfunc PROTO((stackword_t a0, ...));
static void catchsig PROTO((int signo));
static void call_machine_func PROTO((machine_t *ma, stackword_t *sp,
						stackword_t func, int nargs));
static void link_mem PROTO((machine_t *ma, mem_t *m));
static void unlink_mem PROTO((mem_t *m));
static void set_size PROTO((mem_t *m, size_t size));
static mem_t *get_mem PROTO((machine_t *ma, voidptr p));
static int copy_stringtab PROTO((machine_t *ma, char **itab, char ***p_copy));
static void catch_arithmetic_signals PROTO((int signo));
static void write_long PROTO((machine_t *ma, textword_t *pos, stackword_t val));

static jmp_buf ci_Arithmetic_signal_env;
static bool Want_longjmp_on_arithmetic_signal;
static void (*Uncaught_arithmetic_signal_handler)PROTO((int signo));

static machine_t *Current_machine;

#ifndef NSIG
#define NSIG	100
#endif

/*  We rely on this being initialised to zero.
 */
static stackword_t Sigfuncs[NSIG];

#define M_STARTVAL	0x4853acbd
#define M_ENDVAL	0xd31c8bee

#define M_FREED_BYTE	((unsigned)0x91)

typedef double (*stackword_funcptr_t)PROTO((stackword_t a1, stackword_t a2,
					    stackword_t a3, stackword_t a4,
					    stackword_t a5, stackword_t a6,
					    stackword_t a7, stackword_t a8,
					    stackword_t a9, stackword_t a10,
					    stackword_t a11, stackword_t a12,
					    stackword_t a13, stackword_t a14,
					    stackword_t a15, stackword_t a16,
					    stackword_t a17, stackword_t a18,
					    stackword_t a19, stackword_t a20));
typedef double (*double_funcptr_t)PROTO((stackword_t a1, stackword_t a2,
					 stackword_t a3, stackword_t a4,
					 stackword_t a5, stackword_t a6,
					 stackword_t a7, stackword_t a8,
					 stackword_t a9, stackword_t a10,
					 stackword_t a11, stackword_t a12,
					 stackword_t a13, stackword_t a14,
					 stackword_t a15, stackword_t a16,
					 stackword_t a17, stackword_t a18,
					 stackword_t a19, stackword_t a20));

/*  This should be called once at startup if we expect to handle arithemetic
 *  signals.
 */
void
ci_catch_arithmetic_signals(handler)
void (*handler)PROTO((int signo));
{
	Want_longjmp_on_arithmetic_signal = FALSE;

	Uncaught_arithmetic_signal_handler = handler;
	
	/*  Catch divisions by zero etc in the target code.
	 */
	signal(SIGFPE, catch_arithmetic_signals);

#ifdef ARCH_VAX
	/*  The VAX sends a SIGILL signal for certain illegal floating
	 *  point values.
	 */
	signal(SIGILL, catch_arithmetic_signals);
#endif
}

void
ci_set_expecting_arithmetic_signals(expecting_signals)
bool expecting_signals;
{
	Want_longjmp_on_arithmetic_signal = expecting_signals;
}

static void
catch_arithmetic_signals(signo)
int signo;
{
	if (Want_longjmp_on_arithmetic_signal) {
		sigset_t mask;
		
		Want_longjmp_on_arithmetic_signal = FALSE;

		sigemptyset(&mask);
		sigaddset(&mask, signo);
		sigprocmask(SIG_UNBLOCK, &mask, (sigset_t *)NULL);
		
		ups_longjmp(ci_Arithmetic_signal_env, 1);
	}

	(*Uncaught_arithmetic_signal_handler)(signo);

	/*  Should not get here.
	 */
	panic("Unexpected arithmetic signal");
}

const char *
ci_exec_result_to_string(res)
ci_exec_result_t res;
{
	switch (res) {
		case CI_ER_EXITED:		return "exited";
		case CI_ER_CONTINUE:		return "continue";
		case CI_ER_TRAP:		return "trap";
		case CI_ER_LOADFAULT:		return "text load error";
		case CI_ER_READDATA_FAILED:	return "data read failed";
		case CI_ER_WRITEDATA_FAILED:	return "data write failed";
		case CI_ER_INDIRECT_CALL_FAILED:return "indirect call failed";
		case CI_ER_STACK_OVERFLOW:	return "stack overflow";
		case CI_ER_DIVISION_BY_ZERO:	return "division by zero";
		case CI_ER_ARITHMETIC_EXCEPTION:return "arithmetic exception";
		case CI_ER_BAD_MA_COUNT:	return "bad ->[] index";
		case CI_ER_MEM_BAD:		return "malloc arena corrupted";
		case CI_ER_MEM_OVERRUN:		return "malloc block overrun";
		case CI_ER_MEM_FREED:		return "block freed twice";
		case CI_ER_USER1:		return "user1";
		case CI_ER_USER2:		return "user2";
		case CI_ER_USER3:		return "user3";
	}
	
	return "unknown result";
}

/*  BUG: we take no precautions to ensure that the C interpreter address
 *       range doesn't overlap other address ranges.  This means that this
 *       routine may give the wrong answer.
 */
bool
ci_is_ci_addr(ma, addr, len)
machine_t *ma;
unsigned long addr;
size_t len;
{
	typedef unsigned long address_t;
	codefile_t *cf, *cflim;
	mem_t *m, *mh;
	address_t lim;

	if (addr == 0)
		return FALSE;
	lim = addr + len;

	if (addr >= (address_t)ma->ma_minsp && lim <= (address_t)ma->ma_maxsp)
		return TRUE;

	if (addr >= (address_t)ma->ma_data &&
	    lim <= (address_t)ma->ma_data + ma->ma_data_size + ma->ma_bss_size)
		return TRUE;

	cflim = ma->ma_cftab + ma->ma_cftab_size;
	for (cf = ma->ma_cftab; cf < cflim; ++cf) {
		if (!cf->cf_loaded)
			continue;

		if (addr >= (address_t)cf->cf_text_and_data + cf->cf_text_size &&
		    lim <= (address_t)cf->cf_text_and_data + cf->cf_text_size +
					cf->cf_data_size + cf->cf_bss_size)
	    		return TRUE;
	}

	mh = &ma->ma_mhead;
	for (m = mh->m_next; m != mh; m = m->m_next) {
		if (addr >= (address_t)&m[1] &&
		    lim <= (address_t)&m[1] + m->m_size)
			return TRUE;
	}

	return FALSE;
}

/*  Do what ci_code_long() does.
 */
void
ci_write_text_long(text, val)
textword_t *text;
unsigned long val;
{
	text[0] = val & 0xff;
	text[1] = (val >> 8) & 0xff;
	text[2] = (val >> 16) & 0xff;
	text[3] = (val >> 24) & 0xff;
}

void
ci_make_crt0(ap, main_index, exit_index, su)
alloc_pool_t *ap;
int main_index, exit_index;
startup_info_t *su;
{
	textword_t *text;
	textaddr_t pc;
	size_t text_size;
	int start_addr, setjmp_addr, longjmp_addr;
	int argc_pos, argv_pos, envp_pos;

	/*	    envp argv argc main retval exit trap sj/lj */
	text_size = 5  + 5  + 5  + 6  + 1    + 6  + 1  + 2;
#if WANT_TYPE_PUSHED
	/*	    envp argv argc main type retval exit type trap sj/lj */
	text_size = 5  + 5  + 5  + 6  + 1  + 1    + 6  + 1  + 1  + 2;
#endif

	text = (textword_t *)alloc(ap, text_size);

	pc = 0;
	start_addr = pc;

	text[pc++] = (int)OC_CONSTPUSH_L;
	envp_pos = pc;
	pc += sizeof(long);

	text[pc++] = (int)OC_CONSTPUSH_L;
	argv_pos = pc;
	pc += sizeof(long);

	text[pc++] = (int)OC_CONSTPUSH_L;
	argc_pos = pc;
	pc += sizeof(long);

	text[pc++] = (int)OC_CALL_L;
	ci_write_text_long(&text[pc], (unsigned long)main_index);
	pc += sizeof(long);
	text[pc++] = 3;		/* three arguments to main */
#if WANT_TYPE_PUSHED
	text[pc++] = (int)TY_INT;
#endif

	text[pc++] = (int)OC_PUSH_WORD_RETVAL;

	text[pc++] = (int)OC_CALL_L;
	ci_write_text_long(&text[pc], (unsigned long)exit_index);
	pc += sizeof(long);
	text[pc++] = 1;		/* one argument to exit */
#if WANT_TYPE_PUSHED
	text[pc++] = (int)TY_INT;
#endif

	text[pc++] = (int)OC_TRAP;

	setjmp_addr = pc;
	text[pc++] = (int)OC_SETJMP;

	longjmp_addr = pc;
	text[pc++] = (int)OC_LONGJMP;

	if (pc != text_size)
		panic("textsize botch in mc0");
	
	su->su_text = text;
	su->su_text_size = text_size;
	su->su_start_addr = start_addr;
	su->su_argv_pos = argv_pos;
	su->su_argc_pos = argc_pos;
	su->su_envp_pos = envp_pos;
	su->su_setjmp_addr = setjmp_addr;
	su->su_longjmp_addr = longjmp_addr;
}

void
ci_register_fd(ma, fd)
machine_t *ma;
int fd;
{
	if (ma == NULL)
		ma = Current_machine;

	if (fd < 0)
		panic("fd<0 in rf");

	if (fd >= ma->ma_fdtab_size) {	
		ma->ma_fdtab_size = fd + 32;	/* add a little slop */
		ma->ma_fdtab = e_realloc(ma->ma_fdtab, ma->ma_fdtab_size);
	}

	if (ma->ma_fdtab[fd] != 0)
		panic("dup fd in rf");

	ma->ma_fdtab[fd] = 1;
}

void
ci_unregister_fd(ma, fd)
machine_t *ma;
int fd;
{
	if (ma == NULL)
		ma = Current_machine;

	if (fd < 0 || fd >= ma->ma_fdtab_size)
		panic("bad fd in urf");

	if (ma->ma_fdtab[fd] != 1)
		panic("missing fd in rf");

	ma->ma_fdtab[fd] = 0;
}

bool
ci_fd_is_registered(ma, fd)
machine_t *ma;
int fd;
{
	if (ma == NULL)
		ma = Current_machine;

	return fd >= 0 && fd < ma->ma_fdtab_size && ma->ma_fdtab[fd] != 0;
}

voidptr
ci_mem_malloc(ma, size)
machine_t *ma;
size_t size;
{
	mem_t *m;

	if (ma == NULL)
		ma = Current_machine;

	if ((m = (mem_t *)malloc(sizeof(mem_t) + size + sizeof(int))) == NULL)
		return NULL;

	link_mem(ma, m);

	m->m_start_marker = M_STARTVAL;
	set_size(m, size);

	return (voidptr)(m + 1);
}

static void
link_mem(ma, m)
machine_t *ma;
mem_t *m;
{
	mem_t *mh;

	mh = &ma->ma_mhead;

	m->m_prev = mh;
	m->m_next = mh->m_next;
	mh->m_next->m_prev = m;
	mh->m_next = m;
}

static void
unlink_mem(m)
mem_t *m;
{
	m->m_prev->m_next = m->m_next;
	m->m_next->m_prev = m->m_prev;
}

void
ci_mem_free(ma, p)
machine_t *ma;
voidptr p;
{
	mem_t *m;

	if (ma == NULL)
		ma = Current_machine;
	
	if (p == NULL || (m = get_mem(ma, p)) == NULL)
		return;

	unlink_mem(m);

	/*  Immediately zap the freed memory.
	 */
	memset((char *)m, M_FREED_BYTE,
				sizeof(mem_t) + m->m_size + sizeof(int));
	
	free((char *)m);
}

voidptr
ci_mem_realloc(ma, p, size)
machine_t *ma;
voidptr p;
size_t size;
{
	mem_t *m;

	if (p == NULL)
		return ci_mem_malloc(ma, size);

	if (size == 0) {
		ci_mem_free(ma, p);
		return NULL;
	}

	if (ma == NULL)
		ma = Current_machine;

	if ((m = get_mem(ma, p)) == NULL)
		return NULL;

	unlink_mem(m);

	m = (mem_t *)realloc((voidptr)m, sizeof(mem_t) + size + sizeof(int));

	link_mem(ma, m);

	m->m_start_marker = M_STARTVAL;
	set_size(m, size);

	return (voidptr)(m + 1);
}

static void
set_size(m, size)
mem_t *m;
size_t size;
{
	int end_marker;

	m->m_size = size;

	end_marker = M_ENDVAL;
	memcpy((char *)m + sizeof(mem_t) + size,
					(char *)&end_marker, sizeof(int));
}

static mem_t *
get_mem(ma, p)
machine_t *ma;
voidptr p;
{
	mem_t *m;
	int end_marker;

	m = ((mem_t *)p) - 1;

	if (m->m_start_marker == (M_FREED_BYTE * 0x01010101)) {
		ma->ma_stopres = CI_ER_MEM_FREED;
		return NULL;
	}

	if (m->m_start_marker != M_STARTVAL ||
	    m->m_next->m_prev != m || m->m_prev->m_next != m) {
	    	ma->ma_stopres = CI_ER_MEM_BAD;
		return NULL;
	}

	memcpy((char *)&end_marker, (char *)p + m->m_size, sizeof(int));
	if (end_marker != M_ENDVAL) {
		ma->ma_stopres = CI_ER_MEM_OVERRUN;
		return NULL;
	}

	return m;
}

static int
copy_stringtab(ma, itab, p_copy)
machine_t *ma;
char **itab, ***p_copy;
{
	int count, i;
	size_t nbytes;
	char *optr, **otab;

	nbytes = 0;
	for (count = 0; itab[count] != NULL; ++count)
		nbytes += strlen(itab[count]) + 1;
	nbytes += (count + 1) * sizeof(char *);

	if ((otab = (char **)ci_mem_malloc(ma, nbytes)) == NULL)
		panic("no mem for argv/envp");

	optr = (char *)otab + nbytes;
	for (i = 0; i < count; ++i) {
		optr -= strlen(itab[i]) + 1;
		otab[i] = strcpy(optr, itab[i]);
	}
	otab[i] = NULL;

	*p_copy = otab;
	return count;
}

machine_t *
ci_create_machine(ap, stack_nwords, cftab, cftab_size, entry_cf, entry_point)
alloc_pool_t *ap;
size_t stack_nwords;
codefile_t *cftab;
size_t cftab_size;
codefile_t *entry_cf;
size_t entry_point;
{
	machine_t *ma;
	
	ma = (machine_t *)alloc(ap, sizeof(machine_t));
	ma->ma_apool = ap;

	ma->ma_data = NULL;
	ma->ma_data_copy = NULL;

	ma->ma_data_size = 0;
	ma->ma_bss_size = 0;

	ma->ma_libfuncs = NULL;

	ma->ma_get_regaddr_proc = NULL;
	ma->ma_get_regaddr_proc_arg = NULL;
	ma->ma_reg_relocs = NULL;

	ma->ma_loadfunc = NULL;
	ma->ma_cwd = NULL;
	
	ma->ma_cftab = cftab;
	ma->ma_cftab_size = cftab_size;

	ma->ma_entry_codefile = entry_cf;
	ma->ma_entry_point = entry_point;

	ma->ma_callbacks = NULL;
	ma->ma_free_callbacks = NULL;

	ma->ma_stack_size = stack_nwords * sizeof(stackword_t);
	ma->ma_minsp = (stackword_t *)alloc(ap,
					    stack_nwords * sizeof(stackword_t));
	ma->ma_maxsp = ma->ma_minsp + stack_nwords;
	ma->ma_initial_sp = ma->ma_maxsp;

	ma->ma_argc_pos = NULL;
	ma->ma_argv_pos = NULL;
	ma->ma_envp_pos = NULL;

	ma->ma_fdtab_size = 64;
	ma->ma_fdtab = e_malloc(ma->ma_fdtab_size);
	memset(ma->ma_fdtab, '\0', ma->ma_fdtab_size);
	ma->ma_mhead.m_next = ma->ma_mhead.m_prev = &ma->ma_mhead;

	ma->ma_funcmap = NULL;
	ma->ma_funcmap_size = 0;

	return ma;
}

void
ci_push_trap_frame(ma)
machine_t *ma;
{
	codefile_t *cf;

	cf = ma->ma_entry_codefile;

	if (GETOPCODE(cf->cf_text_and_data) != OC_TRAP)
		panic("missing OC_TRAP");
	
	*--ma->ma_initial_sp = (stackword_t)cf;
	*--ma->ma_initial_sp = (stackword_t)cf->cf_text_and_data;

	if (ma->ma_initial_sp < ma->ma_minsp)
		panic("stack overflow botch in ptf");
}

void
ci_initialise_machine(ma, reset_data, want_opcounts, argv, envp)
machine_t *ma;
bool reset_data, want_opcounts;
char **argv, **envp;
{
	reg_reloc_t *rr;
	int fd;

	for (rr = ma->ma_reg_relocs; rr != NULL; rr = rr->rr_next) {
		unsigned long val;

		val = (*ma->ma_get_regaddr_proc)(ma->ma_get_regaddr_proc_arg,
							         rr->rr_regno);
		ci_write_text_long(ma->ma_entry_codefile->cf_text_and_data + rr->rr_addr,
									   val);
	}

	if (want_opcounts) {
		size_t nbytes;

		nbytes = NUM_OPCODES * sizeof(unsigned long);
		ma->ma_opcounts = (unsigned long *)alloc(ma->ma_apool, nbytes);
		memset((char *)ma->ma_opcounts, '\0', nbytes);
	}
	else {
		ma->ma_opcounts = 0;
	}

	ma->ma_codefile = ma->ma_entry_codefile;
	ma->ma_sp = ma->ma_initial_sp;
	ma->ma_pc = ma->ma_codefile->cf_text_and_data + ma->ma_entry_point;
	ma->ma_fp = NULL;

	if (argv != NULL) {
		char **argv_copy, **envp_copy;
		int argc;

		argc = copy_stringtab(ma, argv, &argv_copy);
		copy_stringtab(ma, envp, &envp_copy);

		write_long(ma, ma->ma_argc_pos, (unsigned long)argc);
		write_long(ma, ma->ma_argv_pos, (unsigned long)argv_copy);
		write_long(ma, ma->ma_envp_pos, (unsigned long)envp_copy);

		cx_set_environ(envp_copy);
	}

	if (reset_data) {
		codefile_t *cf, *cflim;

		memcpy(ma->ma_data, ma->ma_data_copy, ma->ma_data_size);
	 	memset(ma->ma_data + ma->ma_data_size, 0, ma->ma_bss_size);

		cflim = ma->ma_cftab + ma->ma_cftab_size;
		for (cf = ma->ma_cftab; cf < cflim; ++cf) {
			char *data;

			if (!cf->cf_loaded)
				continue;

			data = (char *)cf->cf_text_and_data + cf->cf_text_size;
			memcpy(data, cf->cf_data_copy, cf->cf_data_size);
			memset(data + cf->cf_data_size, '\0', cf->cf_bss_size);
		}
	}

	/*  DUBIOUS: we may not want all of these.
	 */
	for (fd = 0; fd < 3; ++fd) {
		if (!ci_fd_is_registered(ma, fd))
			ci_register_fd(ma, fd);
	}
}

static void
write_long(ma, pos, val)
machine_t *ma;
textword_t *pos;
stackword_t val;
{
	if (pos > (textword_t *)ma->ma_minsp &&
	    pos <= (textword_t *)ma->ma_maxsp) {
		*(stackword_t *)pos = val;
	}
	else {
		ci_write_text_long(pos, val);
	}
}

void
ci_reset_machine(ma)
machine_t *ma;
{
	int fd;
	mem_t *m, *next, *mh;
	ci_exec_result_t old_stopres;

	for (fd = 3; fd < ma->ma_fdtab_size; ++fd) {
		if (ma->ma_fdtab[fd] != 0) {
			if (close(fd) != 0)
				errf("Failed to close fd %d: %s",
				     fd, get_errno_str());
			ma->ma_fdtab[fd] = 0;
		}
	}

	old_stopres = ma->ma_stopres;
	ma->ma_stopres = CI_ER_CONTINUE;

	mh = &ma->ma_mhead;
	for (m = mh->m_next; m != mh; m = next) {
		next = m->m_next;

		ci_mem_free(ma, (voidptr)(m + 1));
		if (ma->ma_stopres != CI_ER_CONTINUE)
			break;
	}

	if (m == mh)
		ma->ma_stopres = old_stopres;
}

callback_t
ci_make_callback(func, nargs)
stackword_t func;
int nargs;
{
	machine_t *ma;
	callback_list_t *cl;

	ma = Current_machine;

	if (ma->ma_free_callbacks != NULL) {
		cl = ma->ma_free_callbacks;
		ma->ma_free_callbacks = cl->cl_next;
	}
	else {
		cl = (callback_list_t *)alloc(ma->ma_apool,
					      sizeof(callback_list_t));
	}

	cl->cl_func = func;
	cl->cl_nargs = nargs;
	cl->cl_next = ma->ma_callbacks;
	ma->ma_callbacks = cl;

	return callfunc;
}

textaddr_t
ci_set_sig_handler(signo, func)
int signo;
stackword_t func;
{
	typedef void (*sigfunc_t)PROTO((int fsig));
	sigfunc_t oldfunc;
	stackword_t old_sigfunc;

	if (signo < 0 || signo >= NSIG)
		return (textaddr_t)-1;
	
	old_sigfunc = Sigfuncs[signo];
	
	if (func == (stackword_t)SIG_IGN || func == (stackword_t)SIG_DFL) {
		oldfunc = signal(signo, (sigfunc_t)func);
		Sigfuncs[signo] = 0;
	}
	else {
		oldfunc = signal(signo, catchsig);
		Sigfuncs[signo] = func;
	}

	return (oldfunc == catchsig) ? old_sigfunc : (stackword_t)oldfunc;
}

static void
catchsig(signo)
int signo;
{
	machine_t *ma;

	if (signo < 0 || signo >= NSIG || Sigfuncs[signo] == 0)
		panic("signo botch");

	ma = Current_machine;
	ma->ma_sp[-1] = signo;

	call_machine_func(ma, ma->ma_sp - 1, Sigfuncs[signo], 1);
}

#ifdef __STDC__
static int
callfunc(stackword_t a0, ...)
#else
static int
callfunc(a0, va_alist)
  stackword_t a0;
  va_dcl
#endif
{
	machine_t *ma;
	va_list ap;
	stackword_t *sp;
	int nargs;
	callback_list_t *cl;

	ma = Current_machine;

	cl = ma->ma_callbacks;
	nargs = cl->cl_nargs;
	sp = ma->ma_sp - nargs;

	if (nargs != 0) {
		int i;

		sp[0] = a0;

#ifdef __STDC__
		va_start(ap, a0);
#else
		va_start(ap);
#endif
		for (i = 1; i < nargs; ++i)
			sp[i] = va_arg(ap, stackword_t);
		va_end(ap);
	}

	call_machine_func(ma, sp, cl->cl_func, nargs);
	
	return *ma->ma_sp;
}

static void
call_machine_func(ma, sp, func, nargs)
machine_t *ma;
stackword_t *sp, func;
int nargs;
{
	static textword_t text[8];
	ci_exec_result_t res;
	textword_t *textptr = text;
	textword_t *trapptr;

	WRITEOPCODE(textptr, OC_CALL_INDIRECT);
	*textptr++ = nargs;
#if WANT_TYPE_PUSHED
	*textptr++ = TY_INT_ASSUMED;
#endif
	WRITEOPCODE(textptr, OC_PUSH_WORD_RETVAL);
	trapptr = textptr;
	WRITEOPCODE(textptr, OC_TRAP);

	*--sp = func;

	ma->ma_sp = sp;
	ma->ma_pc = text;
	res = ci_execute_machine(ma, (textaddr_t)0, (textaddr_t)0, (textaddr_t)0,
			      (ci_readproc_t)NULL, (ci_writeproc_t)NULL,
			      (ci_indirect_call_proc_t)NULL);
	
	if (res != CI_ER_TRAP || ma->ma_pc != trapptr)
		panic("bad callback");
}

#define PUSH(val, sp)		(*--(sp) = (val))
#define POP(sp)			(*(sp)++)

/* FIX: */

#if WANT_LDBL
#define PUSH_LDOUBLE(d, sp)	\
{ int i; for (i=LDOUBLE_NSLOTS-1; i>=0; i--) PUSH(d.d_words[i], sp);}
#define POP_LDOUBLE(d, sp)	\
{ int i; for (i=0; i<LDOUBLE_NSLOTS; i++) d.d_words[i] = POP(sp);}
#define PUT_LDOUBLE(d, p)	\
{ int i; for (i=LDOUBLE_NSLOTS-1; i>=0; i--) (p)[i] = d.d_words[i];}
#define GET_LDOUBLE(d, p)	\
{ int i; for (i=0; i<LDOUBLE_NSLOTS; i++) d.d_words[i] = (p)[i];}
#endif

#if WANT_LL
#define PUSH_LONGLONG(d, sp)	\
{ int i; for (i=LONGLONG_NSLOTS-1; i>=0; i--) PUSH(d.d_words[i], sp);}
#define POP_LONGLONG(d, sp)	\
{ int i; for (i=0; i<LONGLONG_NSLOTS; i++) d.d_words[i] = POP(sp);}
#define PUT_LONGLONG(d, p)	\
{ int i; for (i=LONGLONG_NSLOTS-1; i>=0; i--) (p)[i] = d.d_words[i];}
#define GET_LONGLONG(d, p)	\
{ int i; for (i=0; i<LONGLONG_NSLOTS; i++) d.d_words[i] = (p)[i];}
#endif

#define PUSH_DOUBLE(d, sp)	\
{ int i; for (i=DOUBLE_NSLOTS-1; i>=0; i--) PUSH(d.d_words[i], sp);}
#define POP_DOUBLE(d, sp)	\
{ int i; for (i=0; i<DOUBLE_NSLOTS; i++) d.d_words[i] = POP(sp);}
#define PUT_DOUBLE(d, p)	\
{ int i; for (i=DOUBLE_NSLOTS-1; i>=0; i--) (p)[i] = d.d_words[i];}
#define GET_DOUBLE(d, p)	\
{ int i; for (i=0; i<DOUBLE_NSLOTS; i++) d.d_words[i] = (p)[i];}

#define PUSH_FLOAT(d, sp)	PUSH(d.d_word, sp)
#define POP_FLOAT(d, sp)	(d.d_word = POP(sp))
#define PUT_FLOAT(d, p)		(*(p) = d.d_word)
#define GET_FLOAT(d, p)		(d.d_word = *(p))

ci_exec_result_t
ci_execute_machine(ma, procfp, procap, proccfa, readproc, writeproc, indirect_call_proc)
machine_t *ma;
textaddr_t procfp, procap, proccfa;
ci_readproc_t readproc;
ci_writeproc_t writeproc;
ci_indirect_call_proc_t indirect_call_proc;
{
#if WANT_JUMPTAB
	static void *jumptab[] = {
#define WANT_LABADDRS
#include "xc_opcodes.h"
	};
	register void **jtab;
#define SWITCH(n)	goto *jtab[n];
#define BREAK		goto *jtab[READOPCODE(pc)]
#define CASE		
#else
#define SWITCH(n)	switch((opcode_t)n)
#define BREAK		break
#define CASE		case
#endif	/* !WANT_JUMPTAB */
	func_retval_t d0, d1;

	machine_state_t *ms;		/* for setjmp and longjmp */

	/*  These variables are copied from ma at execution start and written
	 *  back when execution stops (for example when we hit a trap).
	 */
	textword_t *pc;
	stackword_t *sp;
	codefile_t *cf;
	stackword_t *fp;
	func_retval_t retval;

	stackword_t rvword;
	double rvdouble;
	ci_exec_result_t res = CI_ER_CONTINUE; /* RGA for purify */
	stackword_t *const minsp = ma->ma_minsp;
	stackword_t *const maxsp = ma->ma_maxsp;
	libfunc_addr_t *const libfuncs = ma->ma_libfuncs;

	char *mindata, *maxdata;

	
	Current_machine = ma;

	pc = ma->ma_pc;
	sp = ma->ma_sp;
	fp = ma->ma_fp;
	cf = ma->ma_codefile;
	retval = ma->ma_retval;

	mindata = (char *)cf->cf_text_and_data + cf->cf_text_size;
	maxdata = mindata + cf->cf_data_size + cf->cf_bss_size;
	
	/*  This macro only looks at the current cf, but that's OK because
	 *  with breakpoint and expression code there is only a single
	 *  codefile_t.
	 */
#define IS_CI_ADDR(addr) \
	(((char *)(addr) >= mindata && (char *)(addr) < (char *)maxdata) || \
	 ((char *)(addr) >= (char *)minsp && (char *)(addr) < (char *)maxsp))


#define READDATA(addr, w)	(*readproc)(addr, (char *)&w, sizeof(w))
#define WRITEDATA(addr, w)	(*writeproc)(addr, (const char *)&w, sizeof(w))

	/*  We don't use sigsetjmp() here because that is too expensive in
	 *  system calls when the machine is being started and stopped
	 *  frequently (for example in ups when there is a conditional
	 *  breakpoint which triggers infrequently).  Instead we reset the
	 *  mask in the catch_arithmetic_signals().
	 */
	if (ups_setjmp(ci_Arithmetic_signal_env) != 0) {
		res = CI_ER_ARITHMETIC_EXCEPTION;
		goto quit;
	}
	ci_set_expecting_arithmetic_signals(TRUE);

#if WANT_JUMPTAB
	if (ma->ma_opcounts == NULL) {
		jtab = jumptab;
	}
	else {
		static void *count_jtab[NUM_OPCODES];
		int i;

		for (i = 0; i < NUM_OPCODES; ++i)
			count_jtab[i] = &&update_count;

		jtab = count_jtab;
	}
#endif
		
	ma->ma_stopres = CI_ER_CONTINUE;

	for (;;) {
		SWITCH (READOPCODE(pc)) {
#if WANT_JUMPTAB
update_count:	++ma->ma_opcounts[PREVOPCODE(pc)];
		goto *jumptab[PREVOPCODE(pc)];
#endif

		CASE OC_PUSH_WORD_RETVAL:
			PUSH(retval.d_word, sp);
			BREAK;
#if WANT_LL
		CASE OC_PUSH_LONG_LONG_RETVAL:
			PUSH_LONGLONG(retval, sp);
			BREAK;
#endif
		CASE OC_PUSH_FLOAT_RETVAL:
			PUSH_FLOAT(retval, sp);
			BREAK;
		CASE OC_PUSH_DOUBLE_RETVAL:
			PUSH_DOUBLE(retval, sp);
			BREAK;
#if WANT_LDBL
		CASE OC_PUSH_LONG_DOUBLE_RETVAL:
			PUSH_LDOUBLE(retval, sp);
			BREAK;
#endif
			
		CASE OC_RET_FLOAT:
			POP_FLOAT(retval, sp);
			goto unlink;

		CASE OC_RET_DOUBLE:
			POP_DOUBLE(retval, sp);
			goto unlink;

#if WANT_LDBL
		CASE OC_RET_LONG_DOUBLE:
			POP_LDOUBLE(retval, sp);
			goto unlink;
#endif
			
#if WANT_LL
		CASE OC_RET_LONG_LONG:
			POP_LONGLONG(retval, sp);
			goto unlink;
#endif
			
		CASE OC_RET_WORD:
			retval.d_word = *sp;
		CASE OC_RET:
unlink:			sp = fp;
			fp = (stackword_t *)POP(sp);
			pc = (textword_t *)POP(sp);
			cf = (codefile_t *)POP(sp);
			BREAK;

		CASE OC_PUSH_STRUCTRET_ADDR: {
			int offset;
			stackword_t addr;

			offset = fp[3];
			addr = fp[offset + 3];
			PUSH(addr, sp);
			BREAK; }

		CASE OC_RET_STRUCT: {
			stackword_t addr;
			size_t nbytes;

			nbytes = GETLONG(pc);
			addr = *sp;
			sp = fp;
			fp = (stackword_t *)POP(sp);
			pc = (textword_t *)POP(sp);
			cf = (codefile_t *)POP(sp);

			if (*sp != 0)
				memcpy((char *)sp[*sp], (char *)addr, nbytes);
			BREAK; }

		CASE OC_SETJMP:
			retval.d_word = 0;
			pc = (textword_t *)POP(sp);
			cf = (codefile_t *)POP(sp);

			ms = (machine_state_t *)*sp;
			ms->ms_cf = cf;
			ms->ms_pc = pc;
			ms->ms_fp = fp;
			ms->ms_sp = sp;
			memcpy((char *)ms->ms_stackwords, (char *)&sp[1],
					SETJMP_NWORDS * sizeof(stackword_t));
			BREAK;
		CASE OC_LONGJMP:
			retval.d_word = sp[3];
			ms = (machine_state_t *)sp[2];
			cf = ms->ms_cf;
			pc = ms->ms_pc;
			fp = ms->ms_fp;
			sp = ms->ms_sp;
			memcpy((char *)&sp[1], (char *)ms->ms_stackwords,
					  SETJMP_NWORDS * sizeof(stackword_t));
			BREAK;

		{
			codefile_t *fcf;
			int func_index;

		CASE OC_CALL_INDIRECT: {
			stackword_t addr;

			addr = POP(sp);
			fcf = &ma->ma_cftab[addr >> 16];
			func_index = (short)addr;   /* 16->32 sign extension */

			if (!fcf->cf_loaded && XLOAD(ma, fcf) != 0) {
				res = CI_ER_LOADFAULT;
				goto quit;
			}

			goto func_call; }

		CASE OC_CALL_W:
			fcf = cf;
			func_index = GETWORD(pc);
			pc += 2;
			goto func_call;

		CASE OC_CALL_L:
			fcf = cf;
			func_index = GETLONG(pc);
			pc += 4;
			goto func_call;

		CASE OC_CALL_Q:
			fcf = cf;
			func_index = GETQUAD(pc);
			pc += 8;
			goto func_call;

		CASE OC_CALL_B: {
			extref_t *er;
			libfunc_addr_t lf;
			int nargs;
			typecode_t typecode;

			fcf = cf;
			func_index = *pc++;
func_call:
			nargs = *pc++;
#if WANT_TYPE_PUSHED
			typecode = (typecode_t)*pc++;
#endif
			if (func_index > 0) {
				PUSH((stackword_t)cf, sp);
				PUSH((stackword_t)pc, sp);

				cf = fcf;
				pc = cf->cf_text_and_data + cf->cf_funcaddrs[func_index];
				BREAK;
			}

			er = fcf->cf_funcrefs[func_index];
			
			if (er->er_file != NULL) {
				PUSH((stackword_t)cf, sp);
				cf = er->er_file;
				if (!cf->cf_loaded && XLOAD(ma, cf) != 0) {
					res = CI_ER_LOADFAULT;
					goto quit;
				}

				PUSH((stackword_t)pc, sp);
				pc = cf->cf_text_and_data + er->er_addr;
				BREAK;
			}

			/* (IDE) When the interpreter is built into UPS then
			 * calls in code entered interactively are 'indirect'
			 * - see 'getaddr()' in ui_misc.c.  The typecode is
			 * pushed in 'compile_func_call_expr()'.
			 */

			if (er->er_indirect) {
				res = (*indirect_call_proc)(ma,
					      er->er_addr,
					      sp, nargs, &retval.d_word,
					      typecode);
				if (res != CI_ER_CONTINUE)
					goto quit;
				BREAK;
			}

			lf = libfuncs[er->er_addr];
			ma->ma_sp = sp;

#if WANT_TYPE_PUSHED
			{
			    stackword_t params[20];

			    if (nargs > (sizeof(params)/sizeof(params[0])))
				    panic("too many args");
			    memset (params, 0, sizeof(params));
			    memcpy (params, sp, nargs * sizeof(params[0]));

			    switch (typecode) {
			    case TY_LONGDOUBLE: /* FIX: */
			    case TY_DOUBLE:
				if (nargs == 0)
				    rvdouble = (*(double (*)PROTO((void)))lf) ();
				else
				    rvdouble = (*(double_funcptr_t)lf) (
		    params[0],  params[1],  params[2],  params[3],  params[4],
		    params[5],  params[6],  params[7],  params[8],  params[9],
		    params[10], params[11], params[12], params[13], params[14],
		    params[15], params[16], params[17], params[18], params[19]);
				retval.d_double = rvdouble;
				break;
			    default:
				if (nargs == 0)
				    rvword = (*(stackword_t (*)PROTO((void)))lf) ();
				else
				    rvword = (*(stackword_funcptr_t)lf) (
		    params[0],  params[1],  params[2],  params[3],  params[4],
		    params[5],  params[6],  params[7],  params[8],  params[9],
		    params[10], params[11], params[12], params[13], params[14],
		    params[15], params[16], params[17], params[18], params[19]);
				retval.d_word = rvword;
				break;
			    } /* switch */

			}
#else
			switch (nargs) {
			case 0:
				rvword = (*(stackword_t (*)PROTO((void)))lf)();
				break;
			case 1:
				rvword = (*lf)(sp[0]);
				break;
			case 2:
				rvword = (*lf)(sp[0], sp[1]);
				break;
			case 3:
				rvword = (*lf)(sp[0], sp[1], sp[2]);
				break;
			case 4:
				rvword = (*lf)(sp[0], sp[1], sp[2], sp[3]);
				break;
			case 5:
				rvword = (*lf)(sp[0], sp[1], sp[2], sp[3],
					       sp[4]);
				break;
			case 6:
				rvword = (*lf)(sp[0], sp[1], sp[2], sp[3],
					       sp[4], sp[5]);
				break;
			case 7:
				rvword = (*lf)(sp[0], sp[1], sp[2], sp[3],
					       sp[4], sp[5], sp[6]);
				break;
			case 8:
				rvword = (*lf)(sp[0], sp[1], sp[2], sp[3],
					       sp[4], sp[5], sp[6], sp[7]);
				break;
			case 9:
				rvword = (*lf)(sp[0], sp[1], sp[2], sp[3],
					       sp[4], sp[5], sp[6], sp[7],
					       sp[8]);
				break;
			case 10:
				rvword = (*lf)(sp[0], sp[1], sp[2], sp[3],
					       sp[4], sp[5], sp[6], sp[7],
					       sp[8], sp[9]);
				break;
			default:
				if (nargs > 30)
					panic("too many args");
				rvword = (*lf)(sp[0],  sp[1],  sp[2], sp[3],
					       sp[4],  sp[5], sp[6],  sp[7],
					       sp[8], sp[9],  sp[10], sp[11],
					       sp[12], sp[13], sp[14], sp[15],
					       sp[16], sp[17], sp[18], sp[19],
					       sp[20], sp[21], sp[22], sp[23],
					       sp[24], sp[25], sp[26], sp[27],
					       sp[28], sp[29]);
				break;
			}
			/* BUG: what if the function returned double? */
			retval.d_word = rvword;
#endif
			if (ma->ma_stopres != CI_ER_CONTINUE) {
				res = ma->ma_stopres;
				goto quit;
			}

			BREAK; }
		}

		CASE OC_LINK_B:
			PUSH((stackword_t)fp, sp);
			fp = sp;
			sp = (stackword_t *)((char *)sp - *pc++);
#if WANT_OLD_LINK
			if ((char *)sp - *pc++ >= (char *)minsp)
				BREAK;
#else
			/* Fix LINK ARG (CHAR -> LONG) : Dibyendu */
			if (((char *)sp - GETLONG(pc)) >= (char *)minsp) {
				pc += 4;
				BREAK;
			}
#endif
			goto stack_overflow;

		CASE OC_LINK_W:
			PUSH((stackword_t)fp, sp);
			fp = sp;
			sp = (stackword_t *)((char *)sp - GETWORD(pc));
			pc += 2;
#if WANT_OLD_LINK
			if ((char *)sp - *pc++ >= (char *)minsp)
				BREAK;
#else
			/* Fix LINK ARG (CHAR -> LONG) : Dibyendu */
			if (((char *)sp - GETLONG(pc)) >= (char *)minsp) {
				pc += 4;
				BREAK;
			}
#endif
			goto stack_overflow;

		CASE OC_LINK_L:
			PUSH((stackword_t)fp, sp);
			fp = sp;
			sp = (stackword_t *)((char *)sp - GETLONG(pc));
			pc += 4;
#if WANT_OLD_LINK
			if ((char *)sp - *pc++ >= (char *)minsp)
				BREAK;
#else
			/* Fix LINK ARG (CHAR -> LONG) : Dibyendu */
			if (((char *)sp - GETLONG(pc)) >= (char *)minsp) {
				pc += 4;
				BREAK;
			}
#endif
			goto stack_overflow;

		CASE OC_LINK_Q:
			PUSH((stackword_t)fp, sp);
			fp = sp;
			sp = (stackword_t *)((char *)sp - GETQUAD(pc));
			pc += 8;
#if WANT_OLD_LINK
			if ((char *)sp - *pc++ >= (char *)minsp)
				BREAK;
#else
			/* Fix LINK ARG (CHAR -> LONG) : Dibyendu */
			if (((char *)sp - GETLONG(pc)) >= (char *)minsp) {
				pc += 4;
				BREAK;
			}
#endif

stack_overflow:		res = CI_ER_STACK_OVERFLOW;
#if WANT_OLD_LINK
			--pc;
#endif
			goto quit;

		CASE OC_POPMANY_B:
			sp = (stackword_t *)((char *)sp + *pc++);
			BREAK;
		CASE OC_POPMANY_W:
			sp = (stackword_t *)((char *)sp + GETWORD(pc));
			pc += 2;
			BREAK;
		CASE OC_POPMANY_L:
			sp = (stackword_t *)((char *)sp + GETLONG(pc));
			pc += 4;
			BREAK;
		CASE OC_POPMANY_Q:
			sp = (stackword_t *)((char *)sp + GETQUAD(pc));
			pc += 8;
			BREAK;

		{
			size_t nbytes;

		CASE OC_PROC_MEMCPY_B:
			nbytes = *pc++;
			goto proc_memcpy;
		CASE OC_PROC_MEMCPY_W:
			nbytes = GETWORD(pc);
			pc += 2;
			goto proc_memcpy;
		CASE OC_PROC_MEMCPY_L:
			nbytes = GETLONG(pc);
			pc += 4;
			goto proc_memcpy;
		CASE OC_PROC_MEMCPY_Q:
			nbytes = GETQUAD(pc);
			pc += 8;
proc_memcpy:
			if (IS_CI_ADDR(sp[1])) {
				if (IS_CI_ADDR(*sp))
					memcpy((char *)sp[1], (char *)*sp, nbytes);
				else {
					if ((*readproc)(*sp, (char *)sp[1],
								 nbytes) != 0) {
						res = CI_ER_READDATA_FAILED;
						goto quit;
					}
				}
			}
			else {
				if (IS_CI_ADDR(*sp)) {
					if ((*writeproc)(sp[1], (char *)*sp,
							         nbytes) != 0) {
						res = CI_ER_WRITEDATA_FAILED;
						goto quit;
					}
				}
				else {
					char *buf;

					buf = e_malloc(nbytes);
					if ((*readproc)(*sp, buf, nbytes) != 0){
						res = CI_ER_READDATA_FAILED;
						goto quit;
					}
					if ((*writeproc)(sp[1], buf, nbytes)
									!= 0) {
						res = CI_ER_WRITEDATA_FAILED;
						goto quit;
					}
					free(buf);
				}
			}

			sp += 2;
			BREAK;
		}

		CASE OC_MEMCPY_B:
			memcpy((char *)sp[1], (char *)*sp, (size_t)*pc++);
			sp += 2;
			BREAK;
		CASE OC_MEMCPY_W:
			memcpy((char *)sp[1], (char *)*sp, (size_t)GETWORD(pc));
			pc += 2;
			sp += 2;
			BREAK;
		CASE OC_MEMCPY_L:
			memcpy((char *)sp[1], (char *)*sp, (size_t)GETLONG(pc));
			pc += 4;
			sp += 2;
			BREAK;
		CASE OC_MEMCPY_Q:
			memcpy((char *)sp[1], (char *)*sp, (size_t)GETQUAD(pc));
			pc += 8;
			sp += 2;
			BREAK;
		
		CASE OC_RESERVE_BYTES: {
			size_t nbytes;

			nbytes = GETLONG(pc);
			pc += 4;
			sp -= nbytes / 4;
			sp[-1] = (long)sp;
			--sp;
			BREAK; }

		CASE OC_PROC_PUSH_BYTES: {
			stackword_t addr;
			size_t nbytes;

			nbytes = GETLONG(pc);
			addr = *sp;
			sp -= nbytes / 4 - 1;

			if (IS_CI_ADDR(*sp)) {
				memcpy((char *)sp, (char *)addr, nbytes);
			}
			else {
				if ((*readproc)((unsigned)addr, (char *)sp, nbytes) != 0){
					res = CI_ER_READDATA_FAILED;
					goto quit;
				}
			}

			pc += 4; 
			BREAK; }
		
		CASE OC_PUSH_BYTES: {
			stackword_t addr;
			size_t nbytes;

			nbytes = GETLONG(pc);
			addr = *sp;
			sp -= nbytes / 4 - 1;
			memcpy((char *)sp, (char *)addr, nbytes);
			pc += 4;
			BREAK; }
		
		CASE OC_EXTRACT_UNSIGNED_BITFIELD: {
			int offset, width;

			offset = *pc++;
			width = *pc++;
			*sp = (*sp >> offset) & ((1 << width) - 1);
			BREAK; }
		CASE OC_EXTRACT_SIGNED_BITFIELD: {
			int offset, width;

			offset = *pc++;
			width = *pc++;
			*sp = (*sp >> offset) & ((1 << width) - 1);
			if (*sp & (1 << (width - 1)))
				*sp |= ~0 << width;
			BREAK; }
		CASE OC_INSERT_SIGNED_BITFIELD:
		CASE OC_INSERT_UNSIGNED_BITFIELD: {
			int offset, width;
			unsigned long mask;

			offset = *pc++;
			width = *pc++;
			mask = ((1 << width) - 1) << offset;
			sp[1] = (sp[1] & ~mask) | ((*sp << offset) & mask);
			++sp;
			BREAK; }

#if WANT_LL
		CASE OC_PROC_ASSIGN_LONG_LONG: {
			stackword_t addr;

			POP_LONGLONG(d0, sp);
			addr = POP(sp);
			if (IS_CI_ADDR(addr)) {
				PUT_LONGLONG(d0, (stackword_t *)addr);
			}
			else {
				if (WRITEDATA(addr, d0.d_longlong) != 0) {
					res = CI_ER_WRITEDATA_FAILED;
					goto quit;
				}
			}
			BREAK; }
#endif
		CASE OC_PROC_ASSIGN_BYTE:
			d0.d_char = *sp;
			if (IS_CI_ADDR(sp[1]))
				*(char *)sp[1] = d0.d_char;
			else {
				if (WRITEDATA(sp[1], d0.d_char) != 0) {
					res = CI_ER_WRITEDATA_FAILED;
					goto quit;
				}
			}
			sp += 2;
			BREAK;
		CASE OC_PROC_ASSIGN_WORD:
			d0.d_short = *sp;
			if (IS_CI_ADDR(sp[1]))
				*(short *)sp[1] = d0.d_short;
			else {
				if (WRITEDATA(sp[1], d0.d_short) != 0) {
					res = CI_ER_WRITEDATA_FAILED;
					goto quit;
				}
			}
			sp += 2;
			BREAK;
		CASE OC_PROC_ASSIGN_LONG:
			d0.d_long = *sp;
			if (IS_CI_ADDR(sp[1]))
				*(long *)sp[1] = d0.d_long;
			else {
				if (WRITEDATA(sp[1], d0.d_long) != 0) {
					res = CI_ER_WRITEDATA_FAILED;
					goto quit;
				}
			}
			sp += 2;
			BREAK;
		CASE OC_PROC_ASSIGN_FLOAT: {
			stackword_t addr;

			POP_FLOAT(d0, sp);
			addr = POP(sp);
			if (IS_CI_ADDR(addr)) {
				PUT_FLOAT(d0, (stackword_t *)addr);
			}
			else {
				if (WRITEDATA(addr, d0.d_float) != 0) {
					res = CI_ER_WRITEDATA_FAILED;
					goto quit;
				}
			}
			BREAK; }
		CASE OC_PROC_ASSIGN_DOUBLE: {
			stackword_t addr;

			POP_DOUBLE(d0, sp);
			addr = POP(sp);
			if (IS_CI_ADDR(addr)) {
				PUT_DOUBLE(d0, (stackword_t *)addr);
			}
			else {
				if (WRITEDATA(addr, d0.d_double) != 0) {
					res = CI_ER_WRITEDATA_FAILED;
					goto quit;
				}
			}
			BREAK; }
#if WANT_LDBL
		CASE OC_PROC_ASSIGN_LONG_DOUBLE: {
			stackword_t addr;

			POP_LDOUBLE(d0, sp);
			addr = POP(sp);
			if (IS_CI_ADDR(addr)) {
				PUT_LDOUBLE(d0, (stackword_t *)addr);
			}
			else {
				if (WRITEDATA(addr, d0.d_longdouble) != 0) {
					res = CI_ER_WRITEDATA_FAILED;
					goto quit;
				}
			}
			BREAK; }
#endif

#if WANT_LL
		CASE OC_ASSIGN_LONG_LONG: {
			stackword_t addr;

			POP_LONGLONG(d0, sp);
			addr = POP(sp);
			PUT_LONGLONG(d0, (stackword_t *)addr);
			BREAK; }
#endif
		CASE OC_ASSIGN_BYTE:
			*(char *)sp[1] = *sp;
			sp += 2;
			BREAK;
		CASE OC_ASSIGN_WORD:
			*(short *)sp[1] = *sp;
			sp += 2;
			BREAK;
		CASE OC_ASSIGN_LONG:
			*(long *)sp[1] = *sp;
			sp += 2;
			BREAK;
		CASE OC_ASSIGN_FLOAT: {
			stackword_t addr;

			POP_FLOAT(d0, sp);
			addr = POP(sp);
			PUT_FLOAT(d0, (stackword_t *)addr);
			BREAK; }
		CASE OC_ASSIGN_DOUBLE: {
			stackword_t addr;

			POP_DOUBLE(d0, sp);
			addr = POP(sp);
			PUT_DOUBLE(d0, (stackword_t *)addr);
			BREAK; }
#if WANT_LDBL
		CASE OC_ASSIGN_LONG_DOUBLE: {
			stackword_t addr;

			POP_LDOUBLE(d0, sp);
			addr = POP(sp);
			PUT_LDOUBLE(d0, (stackword_t *)addr);
			BREAK; }
#endif

#if WANT_LL
		CASE OC_PROC_ASSIGN_AND_PUSH_LONG_LONG: {
			stackword_t addr;

			POP_LONGLONG(d0, sp);
			addr = POP(sp);
			if (IS_CI_ADDR(addr)) {
				PUT_LONGLONG(d0, (stackword_t *)addr);
			}
			else {
				if (WRITEDATA(addr, d0.d_longlong) != 0) {
					res = CI_ER_WRITEDATA_FAILED;
					goto quit;
				}
			}
			PUSH_LONGLONG(d0, sp);
			BREAK; }
#endif
		CASE OC_PROC_ASSIGN_AND_PUSH_BYTE:
			d0.d_char = *sp;
			if (IS_CI_ADDR(sp[1]))
				*(char *)sp[1] = d0.d_char;
			else {
				if (WRITEDATA(sp[1], d0.d_char) != 0) {
					res = CI_ER_WRITEDATA_FAILED;
					goto quit;
				}
			}
			sp[1] = d0.d_char;
			++sp;
			BREAK;
		CASE OC_PROC_ASSIGN_AND_PUSH_WORD:
			d0.d_short = *sp;
			if (IS_CI_ADDR(sp[1]))
				*(short *)sp[1] = d0.d_short;
			else {
				if (WRITEDATA(sp[1], d0.d_short) != 0) {
					res = CI_ER_WRITEDATA_FAILED;
					goto quit;
				}
			}
			sp[1] = d0.d_short;
			++sp;
			BREAK;
		CASE OC_PROC_ASSIGN_AND_PUSH_LONG:
			d0.d_long = *sp;
			if (IS_CI_ADDR(sp[1]))
				*(long *)sp[1] = d0.d_long;
			else {
				if (WRITEDATA(sp[1], d0.d_long) != 0) {
					res = CI_ER_WRITEDATA_FAILED;
					goto quit;
				}
			}
			sp[1] = d0.d_long;
			++sp;
			BREAK;
		CASE OC_PROC_ASSIGN_AND_PUSH_FLOAT: {
			stackword_t addr;

			POP_FLOAT(d0, sp);
			addr = POP(sp);
			if (IS_CI_ADDR(addr)) {
				PUT_FLOAT(d0, (stackword_t *)addr);
			}
			else {
				if (WRITEDATA(addr, d0.d_float) != 0) {
					res = CI_ER_WRITEDATA_FAILED;
					goto quit;
				}
			}
			PUSH_FLOAT(d0, sp);
			BREAK; }
		CASE OC_PROC_ASSIGN_AND_PUSH_DOUBLE: {
			stackword_t addr;

			POP_DOUBLE(d0, sp);
			addr = POP(sp);
			if (IS_CI_ADDR(addr)) {
				PUT_DOUBLE(d0, (stackword_t *)addr);
			}
			else {
				if (WRITEDATA(addr, d0.d_double) != 0) {
					res = CI_ER_WRITEDATA_FAILED;
					goto quit;
				}
			}
			PUSH_DOUBLE(d0, sp);
			BREAK; }
#if WANT_LDBL
		CASE OC_PROC_ASSIGN_AND_PUSH_LONG_DOUBLE: {
			stackword_t addr;

			POP_LDOUBLE(d0, sp);
			addr = POP(sp);
			if (IS_CI_ADDR(addr)) {
				PUT_LDOUBLE(d0, (stackword_t *)addr);
			}
			else {
				if (WRITEDATA(addr, d0.d_longdouble) != 0) {
					res = CI_ER_WRITEDATA_FAILED;
					goto quit;
				}
			}
			PUSH_LDOUBLE(d0, sp);
			BREAK; }
#endif

#if WANT_LL
		CASE OC_ASSIGN_AND_PUSH_LONG_LONG: {
			stackword_t addr;

			POP_LONGLONG(d0, sp);
			addr = POP(sp);
			PUT_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
#endif
		CASE OC_ASSIGN_AND_PUSH_BYTE:
			*(char *)sp[1] = *sp;
			sp[1] = *sp;
			++sp;
			BREAK;
		CASE OC_ASSIGN_AND_PUSH_WORD:
			*(short *)sp[1] = *sp;
			sp[1] = *sp;
			++sp;
			BREAK;
		CASE OC_ASSIGN_AND_PUSH_LONG:
			*(long *)sp[1] = *sp;
			sp[1] = *sp;
			++sp;
			BREAK;
		CASE OC_ASSIGN_AND_PUSH_FLOAT: {
			stackword_t addr;

			POP_FLOAT(d0, sp);
			addr = POP(sp);
			PUT_FLOAT(d0, (stackword_t *)addr);
			PUSH_FLOAT(d0, sp);
			BREAK; }
		CASE OC_ASSIGN_AND_PUSH_DOUBLE: {
			stackword_t addr;

			POP_DOUBLE(d0, sp);
			addr = POP(sp);
			PUT_DOUBLE(d0, (stackword_t *)addr);
			PUSH_DOUBLE(d0, sp);
			BREAK; }
#if WANT_LDBL
		CASE OC_ASSIGN_AND_PUSH_LONG_DOUBLE: {
			stackword_t addr;

			POP_LDOUBLE(d0, sp);
			addr = POP(sp);
			PUT_LDOUBLE(d0, (stackword_t *)addr);
			PUSH_LDOUBLE(d0, sp);
			BREAK; }
#endif

#if WANT_LL
		CASE OC_PROC_DEREF_SIGNED_LONG_LONG: {
			stackword_t addr;

			addr = POP(sp);
			if (IS_CI_ADDR(*sp)) {
				GET_LONGLONG(d0, (stackword_t *)addr);
			}
			else {
				if (READDATA(addr, d0.d_longlong) != 0) {
					res = CI_ER_READDATA_FAILED;
					goto quit;
				}
			}
			PUSH_LONGLONG(d0, sp);
			BREAK; }
#endif
		CASE OC_PROC_DEREF_SIGNED_BYTE:
			if (IS_CI_ADDR(*sp))
				d0.d_char = *(char *)*sp;
			else {
				if (READDATA(*sp, d0.d_char) != 0) {
					res = CI_ER_READDATA_FAILED;
					goto quit;
				}
			}
			*sp = d0.d_char;
			BREAK;
		CASE OC_PROC_DEREF_SIGNED_WORD:
			if (IS_CI_ADDR(*sp))
				d0.d_short = *(short *)*sp;
			else {
				if (READDATA(*sp, d0.d_short) != 0) {
					res = CI_ER_READDATA_FAILED;
					goto quit;
				}
			}
			*sp = d0.d_short;
			BREAK;
		CASE OC_PROC_DEREF_SIGNED_LONG:
			if (IS_CI_ADDR(*sp))
				d0.d_long = *(long *)*sp;
			else {
				if (READDATA(*sp, d0.d_long) != 0) {
					res = CI_ER_READDATA_FAILED;
					goto quit;
				}
			}
			*sp = d0.d_long;
			BREAK;
		CASE OC_PROC_DEREF_FLOAT: {
			stackword_t addr;

			addr = POP(sp);
			if (IS_CI_ADDR(*sp)) {
				GET_FLOAT(d0, (stackword_t *)addr);
			}
			else {
				if (READDATA(addr, d0.d_float) != 0) {
					res = CI_ER_READDATA_FAILED;
					goto quit;
				}
			}
			PUSH_FLOAT(d0, sp);
			BREAK; }
		CASE OC_PROC_DEREF_DOUBLE: {
			stackword_t addr;

			addr = POP(sp);
			if (IS_CI_ADDR(*sp)) {
				GET_DOUBLE(d0, (stackword_t *)addr);
			}
			else {
				if (READDATA(addr, d0.d_double) != 0) {
					res = CI_ER_READDATA_FAILED;
					goto quit;
				}
			}
			PUSH_DOUBLE(d0, sp);
			BREAK; }
#if WANT_LDBL
		CASE OC_PROC_DEREF_LONG_DOUBLE: {
			stackword_t addr;

			addr = POP(sp);
			if (IS_CI_ADDR(*sp)) {
				GET_LDOUBLE(d0, (stackword_t *)addr);
			}
			else {
				if (READDATA(addr, d0.d_longdouble) != 0) {
					res = CI_ER_READDATA_FAILED;
					goto quit;
				}
			}
			PUSH_LDOUBLE(d0, sp);
			BREAK; }
#endif

#if WANT_LL
		CASE OC_PROC_DEREF_UNSIGNED_LONG_LONG: {
			stackword_t addr;

			addr = POP(sp);
			if (IS_CI_ADDR(*sp)) {
				GET_LONGLONG(d0, (stackword_t *)addr);
			}
			else {
				if (READDATA(addr, d0.d_ulonglong) != 0) {
					res = CI_ER_READDATA_FAILED;
					goto quit;
				}
			}
			PUSH_LONGLONG(d0, sp);
			BREAK; }
#endif
		CASE OC_PROC_DEREF_UNSIGNED_BYTE:
			if (IS_CI_ADDR(*sp))
				d0.d_uchar = *(unsigned char *)*sp;
			else {
				if (READDATA(*sp, d0.d_uchar) != 0) {
					res = CI_ER_READDATA_FAILED;
					goto quit;
				}
			}
			*sp = d0.d_uchar;
			BREAK;
		CASE OC_PROC_DEREF_UNSIGNED_WORD:
			if (IS_CI_ADDR(*sp))
				d0.d_ushort = *(unsigned short *)*sp;
			else {
				if (READDATA(*sp, d0.d_ushort) != 0) {
					res = CI_ER_READDATA_FAILED;
					goto quit;
				}
			}
			*sp = d0.d_ushort;
			BREAK;
		CASE OC_PROC_DEREF_UNSIGNED_LONG:
			if (IS_CI_ADDR(*sp))
				d0.d_ulong = *(unsigned long *)*sp;
			else {
				if (READDATA(*sp, d0.d_ulong) != 0) {
					res = CI_ER_READDATA_FAILED;
					goto quit;
				}
			}
			*sp = d0.d_ulong;
			BREAK;

#if WANT_LL
		CASE OC_DEREF_SIGNED_LONG_LONG: {
			stackword_t addr;

			addr = POP(sp);
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
#endif
		CASE OC_DEREF_SIGNED_BYTE:
			*sp = *(char *)*sp;
			BREAK;
		CASE OC_DEREF_SIGNED_WORD:
			*sp = *(short *)*sp;
			BREAK;
		CASE OC_DEREF_SIGNED_LONG:
			*sp = *(long *)*sp;
			BREAK;

#if WANT_LL
		CASE OC_DEREF_UNSIGNED_LONG_LONG: {
			stackword_t addr;

			addr = POP(sp);
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
#endif
		CASE OC_DEREF_UNSIGNED_BYTE:
			*sp = *(unsigned char *)*sp;
			BREAK;
		CASE OC_DEREF_UNSIGNED_WORD:
			*sp = *(unsigned short *)*sp;
			BREAK;
		CASE OC_DEREF_UNSIGNED_LONG:
			*sp = *(unsigned long *)*sp;
			BREAK;
		CASE OC_DEREF_FLOAT: {
			stackword_t addr;

			addr = POP(sp);
			GET_FLOAT(d0, (stackword_t *)addr);
			PUSH_FLOAT(d0, sp);
			BREAK; }
		CASE OC_DEREF_DOUBLE: {
			stackword_t addr;

			addr = POP(sp);
			GET_DOUBLE(d0, (stackword_t *)addr);
			PUSH_DOUBLE(d0, sp);
			BREAK; }
#if WANT_LDBL
		CASE OC_DEREF_LONG_DOUBLE: {
			stackword_t addr;

			addr = POP(sp);
			GET_LDOUBLE(d0, (stackword_t *)addr);
			PUSH_LDOUBLE(d0, sp);
			BREAK; }
#endif
		
		CASE OC_MULTI_ARROW: {
			stackword_t addr;
			int offset, count;

			offset = sp[0];
			addr = sp[1];
			count = sp[2];

			if (count < 0) {
				res = CI_ER_BAD_MA_COUNT;
				goto quit;
			}

			while (--count >= 0) {
				if (IS_CI_ADDR(addr)) {
					addr = *(unsigned long *)(addr + offset);
				}
				else {
					if (READDATA(addr + offset,
							     d0.d_ulong) != 0) {
						res = CI_ER_READDATA_FAILED;
						goto quit;
					}
					addr = d0.d_ulong;
				}
			}

			sp += 2;
			*sp = addr;

			BREAK; }

		CASE OC_NEG_CONSTPUSH_B:
			PUSH(-*pc++, sp);
			BREAK;
		CASE OC_NEG_CONSTPUSH_W:
			PUSH(-GETWORD(pc), sp);
			pc += 2;
			BREAK;
		CASE OC_NEG_CONSTPUSH_L:
			PUSH(-GETLONG(pc), sp);
			pc += 4;
			BREAK;
		CASE OC_NEG_CONSTPUSH_Q:
			PUSH(-GETQUAD(pc), sp);
			pc += 8;
			BREAK;

		CASE OC_CONSTPUSH_B:
			PUSH(*pc++, sp);
			BREAK;
		CASE OC_CONSTPUSH_W:
			PUSH(GETWORD(pc), sp);
			pc += 2;
			BREAK;
		CASE OC_CONSTPUSH_L:
			PUSH(GETLONG(pc), sp);
			pc += 4;
			BREAK;
		CASE OC_CONSTPUSH_Q:
			PUSH(GETQUAD(pc), sp);
			pc += 8;
			BREAK;
		
		CASE OC_PUSH_FLOAT_CONST:
			memcpy((char *)d0.d_words, pc, FLOAT_NBYTES);
			PUSH_FLOAT(d0, sp);
			pc += FLOAT_NBYTES;
			BREAK;
		CASE OC_PUSH_DOUBLE_CONST:
			memcpy((char *)d0.d_words, pc, DOUBLE_NBYTES);
			PUSH_DOUBLE(d0, sp);
			pc += DOUBLE_NBYTES;
			BREAK;

		CASE OC_CHECK_SP_B: {
			int checkval;

			checkval = *pc++;
			if ((char *)fp - (char *)sp != checkval)
				spfailed(checkval, (char *)fp - (char *)sp,
						(pc - cf->cf_text_and_data) - 2);
			BREAK; }
		CASE OC_CHECK_SP_W: {
			int checkval;

			checkval = GETWORD(pc);
			if ((char *)fp - (char *)sp != checkval)
				spfailed(checkval, (char *)fp - (char *)sp,
					       (pc - cf->cf_text_and_data) - 1);
			pc += 2;
			BREAK; }
		CASE OC_CHECK_SP_L: {
			int checkval;

			checkval = GETLONG(pc);
			if ((char *)fp - (char *)sp != checkval)
				spfailed(checkval, (char *)fp - (char *)sp,
					       (pc - cf->cf_text_and_data) - 1);
			pc += 4;
			BREAK; }
		CASE OC_CHECK_SP_Q: {
			int checkval;

			checkval = GETQUAD(pc);
			if ((char *)fp - (char *)sp != checkval)
				spfailed(checkval, (char *)fp - (char *)sp,
					       (pc - cf->cf_text_and_data) - 1);
			pc += 8;
			BREAK; }

		CASE OC_PROC_PUSH_FP_ADDR_B:
			PUSH((stackword_t)(procfp + *pc++), sp);
			BREAK;
		CASE OC_PROC_PUSH_FP_ADDR_W:
			PUSH((stackword_t)(procfp + GETWORD(pc)), sp);
			pc += 2;
			BREAK;
		CASE OC_PROC_PUSH_FP_ADDR_L:
			PUSH((stackword_t)(procfp + GETLONG(pc)), sp);
			pc += 4;
			BREAK;
		CASE OC_PROC_PUSH_FP_ADDR_Q:
			PUSH((stackword_t)(procfp + GETQUAD(pc)), sp);
			pc += 8;
			BREAK;

		CASE OC_PROC_PUSH_AP_ADDR_B:
			PUSH((stackword_t)(procap + *pc++), sp);
			BREAK;
		CASE OC_PROC_PUSH_AP_ADDR_W:
			PUSH((stackword_t)(procap + GETWORD(pc)), sp);
			pc += 2;
			BREAK;
		CASE OC_PROC_PUSH_AP_ADDR_L:
			PUSH((stackword_t)(procap + GETLONG(pc)), sp);
			pc += 4;
			BREAK;
		CASE OC_PROC_PUSH_AP_ADDR_Q:
			PUSH((stackword_t)(procap + GETQUAD(pc)), sp);
			pc += 8;
			BREAK;

		CASE OC_PROC_PUSH_CFA_ADDR_B:
			PUSH((stackword_t)(proccfa + *pc++), sp);
			BREAK;
		CASE OC_PROC_PUSH_CFA_ADDR_W:
			PUSH((stackword_t)(proccfa + GETWORD(pc)), sp);
			pc += 2;
			BREAK;
		CASE OC_PROC_PUSH_CFA_ADDR_L:
			PUSH((stackword_t)(proccfa + GETLONG(pc)), sp);
			pc += 4;
			BREAK;
		CASE OC_PROC_PUSH_CFA_ADDR_Q:
			PUSH((stackword_t)(proccfa + GETQUAD(pc)), sp);
			pc += 8;
			BREAK;

		CASE OC_PUSH_STACKADDR_B:
			PUSH((stackword_t)((char *)fp + *pc++), sp);
			BREAK;
		CASE OC_PUSH_STACKADDR_W:
			PUSH((stackword_t)((char *)fp + GETWORD(pc)), sp);
			pc += 2;
			BREAK;
		CASE OC_PUSH_STACKADDR_L:
			PUSH((stackword_t)((char *)fp + GETLONG(pc)), sp);
			pc += 4;
			BREAK;
		CASE OC_PUSH_STACKADDR_Q:
			PUSH((stackword_t)((char *)fp + GETQUAD(pc)), sp);
			pc += 8;
			BREAK;

#if WANT_LL
		CASE OC_PUSH_UNSIGNED_LONG_LONG_AT_ADDR_B: {
			stackword_t addr;

			addr = *pc++;
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
		CASE OC_PUSH_UNSIGNED_LONG_LONG_AT_ADDR_W: {
			stackword_t addr;

			addr = GETWORD(pc);
			pc += 2;
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
		CASE OC_PUSH_UNSIGNED_LONG_LONG_AT_ADDR_L: {
			stackword_t addr;

			addr = GETLONG(pc);
			pc += 4;
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
		CASE OC_PUSH_UNSIGNED_LONG_LONG_AT_ADDR_Q: {
			stackword_t addr;

			addr = GETQUAD(pc);
			pc += 8;
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
#endif
		
		CASE OC_PUSH_UNSIGNED_BYTE_AT_ADDR_B: {
			stackword_t addr;

			addr = *pc++;
			PUSH(*(unsigned char *)addr, sp);
			BREAK; }
		CASE OC_PUSH_UNSIGNED_BYTE_AT_ADDR_W:
			PUSH(*(unsigned char *)GETWORD(pc), sp);
			pc += 2;
			BREAK;
		CASE OC_PUSH_UNSIGNED_BYTE_AT_ADDR_L:
			PUSH(*(unsigned char *)GETLONG(pc), sp);
			pc += 4;
			BREAK;
		CASE OC_PUSH_UNSIGNED_BYTE_AT_ADDR_Q:
			PUSH(*(unsigned char *)GETQUAD(pc), sp);
			pc += 8;
			BREAK;

		CASE OC_PUSH_UNSIGNED_SHORT_AT_ADDR_B: {
			stackword_t addr;

			addr = *pc++;
			PUSH(*(unsigned short *)addr, sp);
			BREAK; }
		CASE OC_PUSH_UNSIGNED_SHORT_AT_ADDR_W:
			PUSH(*(unsigned short *)GETWORD(pc), sp);
			pc += 2;
			BREAK;
		CASE OC_PUSH_UNSIGNED_SHORT_AT_ADDR_L:
			PUSH(*(unsigned short *)GETLONG(pc), sp);
			pc += 4;
			BREAK;
		CASE OC_PUSH_UNSIGNED_SHORT_AT_ADDR_Q:
			PUSH(*(unsigned short *)GETQUAD(pc), sp);
			pc += 8;
			BREAK;

		CASE OC_PUSH_UNSIGNED_LONG_AT_ADDR_B: {
			stackword_t addr;

			addr = *pc++;
			PUSH(*(unsigned long *)addr, sp);
			BREAK; }
		CASE OC_PUSH_UNSIGNED_LONG_AT_ADDR_W:
			PUSH(*(unsigned long *)GETWORD(pc), sp);
			pc += 2;
			BREAK;
		CASE OC_PUSH_UNSIGNED_LONG_AT_ADDR_L:
			PUSH(*(unsigned long *)GETLONG(pc), sp);
			pc += 4;
			BREAK;
		CASE OC_PUSH_UNSIGNED_LONG_AT_ADDR_Q:
			PUSH(*(unsigned long *)GETQUAD(pc), sp);
			pc += 8;
			BREAK;

#if WANT_LL
		CASE OC_PUSH_UNSIGNED_LONG_LONG_AT_STACKADDR_B: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + *pc++);
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
		CASE OC_PUSH_UNSIGNED_LONG_LONG_AT_STACKADDR_W: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + GETWORD(pc));
			pc += 2;
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
		CASE OC_PUSH_UNSIGNED_LONG_LONG_AT_STACKADDR_L: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + GETLONG(pc));
			pc += 4;
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
		CASE OC_PUSH_UNSIGNED_LONG_LONG_AT_STACKADDR_Q: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + GETQUAD(pc));
			pc += 8;
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
#endif
		
		CASE OC_PUSH_UNSIGNED_BYTE_AT_STACKADDR_B:
			PUSH(*(unsigned char *)((char *)fp + *pc++), sp);
			BREAK;
		CASE OC_PUSH_UNSIGNED_BYTE_AT_STACKADDR_W:
			PUSH(*(unsigned char *)((char *)fp + GETWORD(pc)), sp);
			pc += 2;
			BREAK;
		CASE OC_PUSH_UNSIGNED_BYTE_AT_STACKADDR_L:
			PUSH(*(unsigned char *)((char *)fp + GETLONG(pc)), sp);
			pc += 4;
			BREAK;
		CASE OC_PUSH_UNSIGNED_BYTE_AT_STACKADDR_Q:
			PUSH(*(unsigned char *)((char *)fp + GETQUAD(pc)), sp);
			pc += 8;
			BREAK;

		CASE OC_PUSH_UNSIGNED_SHORT_AT_STACKADDR_B:
			PUSH(*(unsigned short *)((char *)fp + *pc++), sp);
			BREAK;
		CASE OC_PUSH_UNSIGNED_SHORT_AT_STACKADDR_W:
			PUSH(*(unsigned short *)((char *)fp + GETWORD(pc)), sp);
			pc += 2;
			BREAK;
		CASE OC_PUSH_UNSIGNED_SHORT_AT_STACKADDR_L:
			PUSH(*(unsigned short *)((char *)fp + GETLONG(pc)), sp);
			pc += 4;
			BREAK;
		CASE OC_PUSH_UNSIGNED_SHORT_AT_STACKADDR_Q:
			PUSH(*(unsigned short *)((char *)fp + GETQUAD(pc)), sp);
			pc += 8;
			BREAK;

		CASE OC_PUSH_UNSIGNED_LONG_AT_STACKADDR_B:
			PUSH(*(unsigned long *)((char *)fp + *pc++), sp);
			BREAK;
		CASE OC_PUSH_UNSIGNED_LONG_AT_STACKADDR_W:
			PUSH(*(unsigned long *)((char *)fp + GETWORD(pc)), sp);
			pc += 2;
			BREAK;
		CASE OC_PUSH_UNSIGNED_LONG_AT_STACKADDR_L:
			PUSH(*(unsigned long *)((char *)fp + GETLONG(pc)), sp);
			pc += 4;
			BREAK;
		CASE OC_PUSH_UNSIGNED_LONG_AT_STACKADDR_Q:
			PUSH(*(unsigned long *)((char *)fp + GETQUAD(pc)), sp);
			pc += 8;
			BREAK;

#if WANT_LL
		CASE OC_PUSH_SIGNED_LONG_LONG_AT_ADDR_B: {
			stackword_t addr;

			addr = *pc++;
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
		CASE OC_PUSH_SIGNED_LONG_LONG_AT_ADDR_W: {
			stackword_t addr;

			addr = GETWORD(pc);
			pc += 2;
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
		CASE OC_PUSH_SIGNED_LONG_LONG_AT_ADDR_L: {
			stackword_t addr;

			addr = GETLONG(pc);
			pc += 4;
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
		CASE OC_PUSH_SIGNED_LONG_LONG_AT_ADDR_Q: {
			stackword_t addr;

			addr = GETQUAD(pc);
			pc += 8;
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
#endif
		
		CASE OC_PUSH_SIGNED_BYTE_AT_ADDR_B: {
			stackword_t addr;

			addr = *pc++;
			PUSH(*(char *)addr, sp);
			BREAK; }
		CASE OC_PUSH_SIGNED_BYTE_AT_ADDR_W:
			PUSH(*(char *)GETWORD(pc), sp);
			pc += 2;
			BREAK;
		CASE OC_PUSH_SIGNED_BYTE_AT_ADDR_L:
			PUSH(*(char *)GETLONG(pc), sp);
			pc += 4;
			BREAK;
		CASE OC_PUSH_SIGNED_BYTE_AT_ADDR_Q:
			PUSH(*(char *)GETQUAD(pc), sp);
			pc += 8;
			BREAK;

		CASE OC_PUSH_SIGNED_SHORT_AT_ADDR_B: {
			stackword_t addr;

			addr = *pc++;
			PUSH(*(short *)addr, sp);
			BREAK; }
		CASE OC_PUSH_SIGNED_SHORT_AT_ADDR_W:
			PUSH(*(short *)GETWORD(pc), sp);
			pc += 2;
			BREAK;
		CASE OC_PUSH_SIGNED_SHORT_AT_ADDR_L:
			PUSH(*(short *)GETLONG(pc), sp);
			pc += 4;
			BREAK;
		CASE OC_PUSH_SIGNED_SHORT_AT_ADDR_Q:
			PUSH(*(short *)GETQUAD(pc), sp);
			pc += 8;
			BREAK;

		CASE OC_PUSH_SIGNED_LONG_AT_ADDR_B: {
			stackword_t addr;

			addr = *pc++;
			PUSH(*(long *)addr, sp);
			BREAK; }
		CASE OC_PUSH_SIGNED_LONG_AT_ADDR_W:
			PUSH(*(long *)GETWORD(pc), sp);
			pc += 2;
			BREAK;
		CASE OC_PUSH_SIGNED_LONG_AT_ADDR_L:
			PUSH(*(long *)GETLONG(pc), sp);
			pc += 4;
			BREAK;
		CASE OC_PUSH_SIGNED_LONG_AT_ADDR_Q:
			PUSH(*(long *)GETQUAD(pc), sp);
			pc += 8;
			BREAK;
		
		CASE OC_PUSH_FLOAT_AT_ADDR_B: {
			stackword_t addr;

			addr = *pc++;
			GET_FLOAT(d0, (stackword_t *)addr);
			PUSH_FLOAT(d0, sp);
			BREAK; }
		CASE OC_PUSH_FLOAT_AT_ADDR_W: {
			stackword_t addr;

			addr = GETWORD(pc);
			pc += 2;
			GET_FLOAT(d0, (stackword_t *)addr);
			PUSH_FLOAT(d0, sp);
			BREAK; }
		CASE OC_PUSH_FLOAT_AT_ADDR_L: {
			stackword_t addr;

			addr = GETLONG(pc);
			pc += 4;
			GET_FLOAT(d0, (stackword_t *)addr);
			PUSH_FLOAT(d0, sp);
			BREAK; }
		CASE OC_PUSH_FLOAT_AT_ADDR_Q: {
			stackword_t addr;

			addr = GETQUAD(pc);
			pc += 8;
			GET_FLOAT(d0, (stackword_t *)addr);
			PUSH_FLOAT(d0, sp);
			BREAK; }

		CASE OC_PUSH_DOUBLE_AT_ADDR_B: {
			stackword_t addr;

			addr = *pc++;
			GET_DOUBLE(d0, (stackword_t *)addr);
			PUSH_DOUBLE(d0, sp);
			BREAK; }
		CASE OC_PUSH_DOUBLE_AT_ADDR_W: {
			stackword_t addr;

			addr = GETWORD(pc);
			pc += 2;
			GET_DOUBLE(d0, (stackword_t *)addr);
			PUSH_DOUBLE(d0, sp);
			BREAK; }
		CASE OC_PUSH_DOUBLE_AT_ADDR_L: {
			stackword_t addr;

			addr = GETLONG(pc);
			pc += 4;
			GET_DOUBLE(d0, (stackword_t *)addr);
			PUSH_DOUBLE(d0, sp);
			BREAK; }
		CASE OC_PUSH_DOUBLE_AT_ADDR_Q: {
			stackword_t addr;

			addr = GETQUAD(pc);
			pc += 8;
			GET_DOUBLE(d0, (stackword_t *)addr);
			PUSH_DOUBLE(d0, sp);
			BREAK; }

#if WANT_LDBL
		CASE OC_PUSH_LONG_DOUBLE_AT_ADDR_B: {
			stackword_t addr;

			addr = *pc++;
			GET_LDOUBLE(d0, (stackword_t *)addr);
			PUSH_LDOUBLE(d0, sp);
			BREAK; }
		CASE OC_PUSH_LONG_DOUBLE_AT_ADDR_W: {
			stackword_t addr;

			addr = GETWORD(pc);
			pc += 2;
			GET_LDOUBLE(d0, (stackword_t *)addr);
			PUSH_LDOUBLE(d0, sp);
			BREAK; }
		CASE OC_PUSH_LONG_DOUBLE_AT_ADDR_L: {
			stackword_t addr;

			addr = GETLONG(pc);
			pc += 4;
			GET_LDOUBLE(d0, (stackword_t *)addr);
			PUSH_LDOUBLE(d0, sp);
			BREAK; }
		CASE OC_PUSH_LONG_DOUBLE_AT_ADDR_Q: {
			stackword_t addr;

			addr = GETQUAD(pc);
			pc += 8;
			GET_LDOUBLE(d0, (stackword_t *)addr);
			PUSH_LDOUBLE(d0, sp);
			BREAK; }
#endif

#if WANT_LL
		CASE OC_PUSH_SIGNED_LONG_LONG_AT_STACKADDR_B: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + *pc++);
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
		CASE OC_PUSH_SIGNED_LONG_LONG_AT_STACKADDR_W: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + GETWORD(pc));
			pc += 2;
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
		CASE OC_PUSH_SIGNED_LONG_LONG_AT_STACKADDR_L: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + GETLONG(pc));
			pc += 4;
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
		CASE OC_PUSH_SIGNED_LONG_LONG_AT_STACKADDR_Q: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + GETQUAD(pc));
			pc += 8;
			GET_LONGLONG(d0, (stackword_t *)addr);
			PUSH_LONGLONG(d0, sp);
			BREAK; }
#endif
		
		CASE OC_PUSH_SIGNED_BYTE_AT_STACKADDR_B:
			PUSH(*(char *)((char *)fp + *pc++), sp);
			BREAK;
		CASE OC_PUSH_SIGNED_BYTE_AT_STACKADDR_W:
			PUSH(*(char *)((char *)fp + GETWORD(pc)), sp);
			pc += 2;
			BREAK;
		CASE OC_PUSH_SIGNED_BYTE_AT_STACKADDR_L:
			PUSH(*(char *)((char *)fp + GETLONG(pc)), sp);
			pc += 4;
			BREAK;
		CASE OC_PUSH_SIGNED_BYTE_AT_STACKADDR_Q:
			PUSH(*(char *)((char *)fp + GETQUAD(pc)), sp);
			pc += 8;
			BREAK;

		CASE OC_PUSH_SIGNED_SHORT_AT_STACKADDR_B:
			PUSH(*(short *)((char *)fp + *pc++), sp);
			BREAK;
		CASE OC_PUSH_SIGNED_SHORT_AT_STACKADDR_W:
			PUSH(*(short *)((char *)fp + GETWORD(pc)), sp);
			pc += 2;
			BREAK;
		CASE OC_PUSH_SIGNED_SHORT_AT_STACKADDR_L:
			PUSH(*(short *)((char *)fp + GETLONG(pc)), sp);
			pc += 4;
			BREAK;
		CASE OC_PUSH_SIGNED_SHORT_AT_STACKADDR_Q:
			PUSH(*(short *)((char *)fp + GETQUAD(pc)), sp);
			pc += 8;
			BREAK;

		CASE OC_PUSH_SIGNED_LONG_AT_STACKADDR_B:
			PUSH(*(long *)((char *)fp + *pc++), sp);
			BREAK;
		CASE OC_PUSH_SIGNED_LONG_AT_STACKADDR_W:
			PUSH(*(long *)((char *)fp + GETWORD(pc)), sp);
			pc += 2;
			BREAK;
		CASE OC_PUSH_SIGNED_LONG_AT_STACKADDR_L:
			PUSH(*(long *)((char *)fp + GETLONG(pc)), sp);
			pc += 4;
			BREAK;
		CASE OC_PUSH_SIGNED_LONG_AT_STACKADDR_Q:
			PUSH(*(long *)((char *)fp + GETQUAD(pc)), sp);
			pc += 8;
			BREAK;

		CASE OC_PUSH_FLOAT_AT_STACKADDR_B: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + *pc++);
			GET_FLOAT(d0, (stackword_t *)addr);
			PUSH_FLOAT(d0, sp);
			BREAK; }
		CASE OC_PUSH_FLOAT_AT_STACKADDR_W: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + GETWORD(pc));
			pc += 2;
			GET_FLOAT(d0, (stackword_t *)addr);
			PUSH_FLOAT(d0, sp);
			BREAK; }
		CASE OC_PUSH_FLOAT_AT_STACKADDR_L: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + GETLONG(pc));
			pc += 4;
			GET_FLOAT(d0, (stackword_t *)addr);
			PUSH_FLOAT(d0, sp);
			BREAK; }
		CASE OC_PUSH_FLOAT_AT_STACKADDR_Q: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + GETQUAD(pc));
			pc += 8;
			GET_FLOAT(d0, (stackword_t *)addr);
			PUSH_FLOAT(d0, sp);
			BREAK; }

		CASE OC_PUSH_DOUBLE_AT_STACKADDR_B: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + *pc++);
			GET_DOUBLE(d0, (stackword_t *)addr);
			PUSH_DOUBLE(d0, sp);
			BREAK; }
		CASE OC_PUSH_DOUBLE_AT_STACKADDR_W: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + GETWORD(pc));
			pc += 2;
			GET_DOUBLE(d0, (stackword_t *)addr);
			PUSH_DOUBLE(d0, sp);
			BREAK; }
		CASE OC_PUSH_DOUBLE_AT_STACKADDR_L: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + GETLONG(pc));
			pc += 4;
			GET_DOUBLE(d0, (stackword_t *)addr);
			PUSH_DOUBLE(d0, sp);
			BREAK; }
		CASE OC_PUSH_DOUBLE_AT_STACKADDR_Q: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + GETQUAD(pc));
			pc += 8;
			GET_DOUBLE(d0, (stackword_t *)addr);
			PUSH_DOUBLE(d0, sp);
			BREAK; }

#if WANT_LDBL
		CASE OC_PUSH_LONG_DOUBLE_AT_STACKADDR_B: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + *pc++);
			GET_LDOUBLE(d0, (stackword_t *)addr);
			PUSH_LDOUBLE(d0, sp);
			BREAK; }
		CASE OC_PUSH_LONG_DOUBLE_AT_STACKADDR_W: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + GETWORD(pc));
			pc += 2;
			GET_LDOUBLE(d0, (stackword_t *)addr);
			PUSH_LDOUBLE(d0, sp);
			BREAK; }
		CASE OC_PUSH_LONG_DOUBLE_AT_STACKADDR_L: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + GETLONG(pc));
			pc += 4;
			GET_LDOUBLE(d0, (stackword_t *)addr);
			PUSH_LDOUBLE(d0, sp);
			BREAK; }
		CASE OC_PUSH_LONG_DOUBLE_AT_STACKADDR_Q: {
			stackword_t addr;

			addr = (stackword_t)((char *)fp + GETQUAD(pc));
			pc += 8;
			GET_LDOUBLE(d0, (stackword_t *)addr);
			PUSH_LDOUBLE(d0, sp);
			BREAK; }
#endif
		
		CASE OC_SWITCH_ON_TABLE: {
			int ncase, minval, switchval;

			ncase = GETWORD(pc);
			pc += 2;
			minval = GETLONG(pc);
			switchval = POP(sp) - minval;
			if ((unsigned)switchval >= ncase)
				switchval = ncase;
			pc += 4 + switchval * 2;
			pc += (short)GETWORD(pc);
			BREAK; }

		CASE OC_SWITCH_ON_CHAIN_B: {
			int ncase, minval, switchval, i;

			ncase = GETWORD(pc);
			pc += 2;
			minval = GETLONG(pc);
			pc += 4;
			switchval = POP(sp) - minval;
			for (i = 0; i < ncase; ++i) {
				int caseval;

				caseval = *pc++;
				if (caseval == switchval) {
					pc += (short)GETWORD(pc);
					break;
				}
				pc += 2;
			}
			if (i == ncase)
				pc += (short)GETWORD(pc);
			BREAK; }

		CASE OC_SWITCH_ON_CHAIN_W: {
			int ncase, minval, switchval, i;

			ncase = GETWORD(pc);
			pc += 2;
			minval = GETLONG(pc);
			pc += 4;
			switchval = POP(sp) - minval;
			for (i = 0; i < ncase; ++i) {
				int caseval;

				caseval = GETWORD(pc);
				pc += 2;
				if (caseval == switchval) {
					pc += (short)GETWORD(pc);
					break;
				}
				pc += 2;
			}
			if (i == ncase)
				pc += (short)GETWORD(pc);
			BREAK; }

		CASE OC_SWITCH_ON_CHAIN_L: {
			int ncase, minval, switchval, i;

			ncase = GETWORD(pc);
			pc += 2;
			minval = GETLONG(pc);
			pc += 4;
			switchval = POP(sp) - minval;
			for (i = 0; i < ncase; ++i) {
				int caseval;

				caseval = GETLONG(pc);
				pc += 4;
				if (caseval == switchval) {
					pc += (short)GETWORD(pc);
					break;
				}
				pc += 2;
			}
			if (i == ncase)
				pc += (short)GETWORD(pc);
			BREAK; }

		CASE OC_SWITCH_ON_CHAIN_Q: {
			int ncase, minval, switchval, i;

			ncase = GETWORD(pc);
			pc += 2;
			minval = GETLONG(pc);
			pc += 4;
			switchval = POP(sp) - minval;
			for (i = 0; i < ncase; ++i) {
				int caseval;

				caseval = GETQUAD(pc);
				pc += 8;
				if (caseval == switchval) {
					pc += (short)GETWORD(pc);
					break;
				}
				pc += 2;
			}
			if (i == ncase)
				pc += (short)GETWORD(pc);
			BREAK; }

		CASE OC_UNRESOLVED_JUMP:
			panic("bad opcode: unresolved jump");
			BREAK;
		CASE OC_JUMP:
			pc += (short)GETWORD(pc);
			BREAK;
		CASE OC_JUMP_IF_NON_ZERO:
			pc += (POP(sp) != 0) ? (short)GETWORD(pc) : 2;
			BREAK;
		CASE OC_JUMP_IF_ZERO:
			pc += (POP(sp) == 0) ? (short)GETWORD(pc) : 2;
			BREAK;

		CASE OC_DUP:
			--sp;
			*sp = sp[1];
			BREAK;
		CASE OC_DUP_BACK_ONE:
			sp[2] = *sp;
			BREAK;
		CASE OC_RESERVE_SLOT:
			--sp;
			BREAK;
		CASE OC_POP:
			++sp;
			BREAK;

		CASE OC_CVT_TO_BOOL:
			*sp = *sp != 0;
			BREAK;

#if WANT_LL
		CASE OC_BITWISE_NOT_LONG_LONG:
			POP_LONGLONG(d0, sp);
			d0.d_ulonglong = ~d0.d_ulonglong;
			PUSH_LONGLONG(d0, sp);
			BREAK;
		CASE OC_LONG_LONG_UNARY_MINUS:
			POP_LONGLONG(d0, sp);
			d0.d_longlong = -d0.d_longlong;
			PUSH_LONGLONG(d0, sp);
			BREAK;
#endif
			
		CASE OC_BITWISE_NOT:
			*sp = ~*sp;
			BREAK;
		CASE OC_LOGICAL_NOT:
			*sp = !*sp;
			BREAK;
		CASE OC_UNARY_MINUS:
			*sp = -*sp;
			BREAK;

		CASE OC_FLOAT_UNARY_MINUS:
			POP_FLOAT(d0, sp);
			d0.d_float = -d0.d_float;
			PUSH_FLOAT(d0, sp);
			BREAK;
		CASE OC_ADD_FLOATS:
			POP_FLOAT(d1, sp);
			POP_FLOAT(d0, sp);
			d0.d_float += d1.d_float;
			PUSH_FLOAT(d0, sp);
			BREAK;
		CASE OC_SUB_FLOATS:
			POP_FLOAT(d1, sp);
			POP_FLOAT(d0, sp);
			d0.d_float -= d1.d_float;
			PUSH_FLOAT(d0, sp);
			BREAK;
		CASE OC_MUL_FLOATS:
			POP_FLOAT(d1, sp);
			POP_FLOAT(d0, sp);
			d0.d_float *= d1.d_float;
			PUSH_FLOAT(d0, sp);
			BREAK;
		CASE OC_DIV_FLOATS:
			POP_FLOAT(d1, sp);
			POP_FLOAT(d0, sp);
			d0.d_float /= d1.d_float;
			PUSH_FLOAT(d0, sp);
			BREAK;
		CASE OC_CHKDIV_FLOATS:
			POP_FLOAT(d1, sp);
			POP_FLOAT(d0, sp);
			if (d1.d_float == 0.0) {	/* BUG: exact cmp */
				res = CI_ER_DIVISION_BY_ZERO;
				goto quit;
			}
			d0.d_float /= d1.d_float;
			PUSH_FLOAT(d0, sp);
			BREAK;

		CASE OC_DOUBLE_UNARY_MINUS:
			POP_DOUBLE(d0, sp);
			d0.d_double = -d0.d_double;
			PUSH_DOUBLE(d0, sp);
			BREAK;
		CASE OC_ADD_DOUBLES:
			POP_DOUBLE(d1, sp);
			POP_DOUBLE(d0, sp);
			d0.d_double += d1.d_double;
			PUSH_DOUBLE(d0, sp);
			BREAK;
		CASE OC_SUB_DOUBLES:
			POP_DOUBLE(d1, sp);
			POP_DOUBLE(d0, sp);
			d0.d_double -= d1.d_double;
			PUSH_DOUBLE(d0, sp);
			BREAK;
		CASE OC_MUL_DOUBLES:
			POP_DOUBLE(d1, sp);
			POP_DOUBLE(d0, sp);
			d0.d_double *= d1.d_double;
			PUSH_DOUBLE(d0, sp);
			BREAK;
		CASE OC_DIV_DOUBLES:
			POP_DOUBLE(d1, sp);
			POP_DOUBLE(d0, sp);
			d0.d_double /= d1.d_double;
			PUSH_DOUBLE(d0, sp);
			BREAK;
		CASE OC_CHKDIV_DOUBLES:
			POP_DOUBLE(d1, sp);
			POP_DOUBLE(d0, sp);
			if (d1.d_double == 0.0) {	/* BUG: exact cmp */
				res = CI_ER_DIVISION_BY_ZERO;
				goto quit;
			}
			d0.d_double /= d1.d_double;
			PUSH_DOUBLE(d0, sp);
			BREAK;

#if WANT_LDBL
		CASE OC_LONG_DOUBLE_UNARY_MINUS:
			POP_LDOUBLE(d0, sp);
			d0.d_longdouble = -d0.d_longdouble;
			PUSH_LDOUBLE(d0, sp);
			BREAK;
		CASE OC_ADD_LONG_DOUBLES:
			POP_LDOUBLE(d1, sp);
			POP_LDOUBLE(d0, sp);
			d0.d_longdouble += d1.d_longdouble;
			PUSH_LDOUBLE(d0, sp);
			BREAK;
		CASE OC_SUB_LONG_DOUBLES:
			POP_LDOUBLE(d1, sp);
			POP_LDOUBLE(d0, sp);
			d0.d_longdouble -= d1.d_longdouble;
			PUSH_LDOUBLE(d0, sp);
			BREAK;
		CASE OC_MUL_LONG_DOUBLES:
			POP_LDOUBLE(d1, sp);
			POP_LDOUBLE(d0, sp);
			d0.d_longdouble *= d1.d_longdouble;
			PUSH_LDOUBLE(d0, sp);
			BREAK;
		CASE OC_DIV_LONG_DOUBLES:
			POP_LDOUBLE(d1, sp);
			POP_LDOUBLE(d0, sp);
			d0.d_longdouble /= d1.d_longdouble;
			PUSH_LDOUBLE(d0, sp);
			BREAK;
		CASE OC_CHKDIV_LONG_DOUBLES:
			POP_LDOUBLE(d1, sp);
			POP_LDOUBLE(d0, sp);
			if (d1.d_longdouble == 0.0) {	/* BUG: exact cmp */
				res = CI_ER_DIVISION_BY_ZERO;
				goto quit;
			}
			d0.d_longdouble /= d1.d_longdouble;
			PUSH_LDOUBLE(d0, sp);
			BREAK;
#endif

		CASE OC_BITWISE_AND:
			sp[1] &= *sp;
			++sp;
			BREAK;
		CASE OC_BITWISE_XOR:
			sp[1] ^= *sp;
			++sp;
			BREAK;
		CASE OC_BITWISE_OR:
			sp[1] |= *sp;
			++sp;
			BREAK;
		CASE OC_MUL_UNSIGNED:
			sp[1] *= *sp;
			++sp;
			BREAK;
		CASE OC_MUL_SIGNED:
			sp[1] = (long)sp[1] * (long)*sp;
			++sp;
			BREAK;
		CASE OC_CHKDIV_UNSIGNED:
			if (*sp == 0) {
				res = CI_ER_DIVISION_BY_ZERO;
				goto quit;
			}
			/* fall through */
		CASE OC_DIV_UNSIGNED:
			sp[1] /= *sp;
			++sp;
			BREAK;
		CASE OC_CHKDIV_SIGNED:
			if (*sp == 0) {
				res = CI_ER_DIVISION_BY_ZERO;
				goto quit;
			}
			/* fall through */
		CASE OC_DIV_SIGNED:
			sp[1] = (long)sp[1] / (long)*sp;
			++sp;
			BREAK;
		CASE OC_MOD:
			sp[1] %= *sp;
			++sp;
			BREAK;
		CASE OC_ADD:
			sp[1] += *sp;
			++sp;
			BREAK;
		CASE OC_SUB:
			sp[1] -= *sp;
			++sp;
			BREAK;
		CASE OC_LSHIFT:
			sp[1] <<= *sp;
			++sp;
			BREAK;
		CASE OC_RSHIFT:
			sp[1] >>= *sp;
			++sp;
			BREAK;

#if WANT_LL
		CASE OC_BITWISE_AND_LONG_LONG:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			d0.d_ulonglong &= d1.d_ulonglong;
			PUSH_LONGLONG(d0, sp);
			BREAK;
		CASE OC_BITWISE_XOR_LONG_LONG:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			d0.d_ulonglong ^= d1.d_ulonglong;
			PUSH_LONGLONG(d0, sp);
			BREAK;
		CASE OC_BITWISE_OR_LONG_LONG:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			d0.d_ulonglong |= d1.d_ulonglong;
			PUSH_LONGLONG(d0, sp);
			BREAK;
		CASE OC_MUL_UNSIGNED_LONG_LONG:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			d0.d_ulonglong *= d1.d_ulonglong;
			PUSH_LONGLONG(d0, sp);
			BREAK;
		CASE OC_MUL_SIGNED_LONG_LONG:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			d0.d_longlong *= d1.d_longlong;
			PUSH_LONGLONG(d0, sp);
			BREAK;
		CASE OC_CHKDIV_UNSIGNED_LONG_LONG:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			if (d1.d_ulonglong == 0) {
				res = CI_ER_DIVISION_BY_ZERO;
				goto quit;
			}
			d0.d_ulonglong /= d1.d_ulonglong;
			PUSH_LONGLONG(d0, sp);
			BREAK;
		CASE OC_DIV_UNSIGNED_LONG_LONG:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			d0.d_ulonglong /= d1.d_ulonglong;
			PUSH_LONGLONG(d0, sp);
			BREAK;
		CASE OC_CHKDIV_SIGNED_LONG_LONG:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			if (d1.d_longlong == 0) {
				res = CI_ER_DIVISION_BY_ZERO;
				goto quit;
			}
			d0.d_longlong /= d1.d_longlong;
			PUSH_LONGLONG(d0, sp);
			BREAK;
		CASE OC_DIV_SIGNED_LONG_LONG:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			d0.d_longlong /= d1.d_longlong;
			PUSH_LONGLONG(d0, sp);
			BREAK;
		CASE OC_MOD_LONG_LONG:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			d0.d_ulonglong %= d1.d_ulonglong;
			PUSH_LONGLONG(d0, sp);
			BREAK;
		CASE OC_ADD_LONG_LONG:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			d0.d_ulonglong += d1.d_ulonglong;
			PUSH_LONGLONG(d0, sp);
			BREAK;
		CASE OC_SUB_LONG_LONG:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			d0.d_ulonglong -= d1.d_ulonglong;
			PUSH_LONGLONG(d0, sp);
			BREAK;
		CASE OC_LSHIFT_LONG_LONG:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			d0.d_ulonglong <<= d1.d_ulonglong;
			PUSH_LONGLONG(d0, sp);
			BREAK;
		CASE OC_RSHIFT_LONG_LONG:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			d0.d_ulonglong >>= d1.d_ulonglong;
			PUSH_LONGLONG(d0, sp);
			BREAK;
#endif
			
		CASE OC_IS_EQUAL:
			sp[1] = sp[1] == *sp;
			++sp;
			BREAK;
		CASE OC_NOT_EQUAL:
			sp[1] = sp[1] != *sp;
			++sp;
			BREAK;

		CASE OC_LESS_SIGNED:
			sp[1] = (long)sp[1] < (long)*sp;
			++sp;
			BREAK;
		CASE OC_GREATER_SIGNED:
			sp[1] = (long)sp[1] > (long)*sp;
			++sp;
			BREAK;
		CASE OC_LESS_OR_EQUAL_SIGNED:
			sp[1] = (long)sp[1] <= (long)*sp;
			++sp;
			BREAK;
		CASE OC_GREATER_OR_EQUAL_SIGNED:
			sp[1] = (long)sp[1] >= (long)*sp;
			++sp;
			BREAK;

		CASE OC_LESS_UNSIGNED:
			sp[1] = sp[1] < *sp;
			++sp;
			BREAK;
		CASE OC_GREATER_UNSIGNED:
			sp[1] = sp[1] > *sp;
			++sp;
			BREAK;
		CASE OC_LESS_OR_EQUAL_UNSIGNED:
			sp[1] = sp[1] <= *sp;
			++sp;
			BREAK;
		CASE OC_GREATER_OR_EQUAL_UNSIGNED:
			sp[1] = sp[1] >= *sp;
			++sp;
			BREAK;

#if WANT_LL
		CASE OC_LONG_LONG_IS_EQUAL:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			PUSH(d0.d_ulonglong == d1.d_ulonglong, sp);
			BREAK;
		CASE OC_LONG_LONG_NOT_EQUAL:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			PUSH(d0.d_ulonglong != d1.d_ulonglong, sp);
			BREAK;

		CASE OC_LONG_LONG_LESS_SIGNED:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			PUSH(d0.d_longlong < d1.d_longlong, sp);
			BREAK;
		CASE OC_LONG_LONG_GREATER_SIGNED:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			PUSH(d0.d_longlong > d1.d_longlong, sp);
			BREAK;
		CASE OC_LONG_LONG_LESS_OR_EQUAL_SIGNED:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			PUSH(d0.d_longlong <= d1.d_longlong, sp);
			BREAK;
		CASE OC_LONG_LONG_GREATER_OR_EQUAL_SIGNED:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			PUSH(d0.d_longlong >= d1.d_longlong, sp);
			BREAK;

		CASE OC_LONG_LONG_LESS_UNSIGNED:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			PUSH(d0.d_ulonglong < d1.d_ulonglong, sp);
			BREAK;
		CASE OC_LONG_LONG_GREATER_UNSIGNED:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			PUSH(d0.d_ulonglong > d1.d_ulonglong, sp);
			BREAK;
		CASE OC_LONG_LONG_LESS_OR_EQUAL_UNSIGNED:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			PUSH(d0.d_ulonglong <= d1.d_ulonglong, sp);
			BREAK;
		CASE OC_LONG_LONG_GREATER_OR_EQUAL_UNSIGNED:
			POP_LONGLONG(d1, sp);
			POP_LONGLONG(d0, sp);
			PUSH(d0.d_ulonglong >= d1.d_ulonglong, sp);
			BREAK;
#endif
			
		CASE OC_FLOAT_IS_EQUAL:
			POP_FLOAT(d1, sp);
			POP_FLOAT(d0, sp);
			PUSH(d0.d_float == d1.d_float, sp);
			BREAK;
		CASE OC_FLOAT_NOT_EQUAL:
			POP_FLOAT(d1, sp);
			POP_FLOAT(d0, sp);
			PUSH(d0.d_float != d1.d_float, sp);
			BREAK;
		CASE OC_FLOAT_LESS:
			POP_FLOAT(d1, sp);
			POP_FLOAT(d0, sp);
			PUSH(d0.d_float < d1.d_float, sp);
			BREAK;
		CASE OC_FLOAT_GREATER:
			POP_FLOAT(d1, sp);
			POP_FLOAT(d0, sp);
			PUSH(d0.d_float > d1.d_float, sp);
			BREAK;
		CASE OC_FLOAT_LESS_OR_EQUAL:
			POP_FLOAT(d1, sp);
			POP_FLOAT(d0, sp);
			PUSH(d0.d_float <= d1.d_float, sp);
			BREAK;
		CASE OC_FLOAT_GREATER_OR_EQUAL:
			POP_FLOAT(d1, sp);
			POP_FLOAT(d0, sp);
			PUSH(d0.d_float >= d1.d_float, sp);
			BREAK;

		CASE OC_DOUBLE_IS_EQUAL:
			POP_DOUBLE(d1, sp);
			POP_DOUBLE(d0, sp);
			PUSH(d0.d_double == d1.d_double, sp);
			BREAK;
		CASE OC_DOUBLE_NOT_EQUAL:
			POP_DOUBLE(d1, sp);
			POP_DOUBLE(d0, sp);
			PUSH(d0.d_double != d1.d_double, sp);
			BREAK;
		CASE OC_DOUBLE_LESS:
			POP_DOUBLE(d1, sp);
			POP_DOUBLE(d0, sp);
			PUSH(d0.d_double < d1.d_double, sp);
			BREAK;
		CASE OC_DOUBLE_GREATER:
			POP_DOUBLE(d1, sp);
			POP_DOUBLE(d0, sp);
			PUSH(d0.d_double > d1.d_double, sp);
			BREAK;
		CASE OC_DOUBLE_LESS_OR_EQUAL:
			POP_DOUBLE(d1, sp);
			POP_DOUBLE(d0, sp);
			PUSH(d0.d_double <= d1.d_double, sp);
			BREAK;
		CASE OC_DOUBLE_GREATER_OR_EQUAL:
			POP_DOUBLE(d1, sp);
			POP_DOUBLE(d0, sp);
			PUSH(d0.d_double >= d1.d_double, sp);
			BREAK;
#if WANT_LDBL		

		CASE OC_LONG_DOUBLE_IS_EQUAL:
			POP_LDOUBLE(d1, sp);
			POP_LDOUBLE(d0, sp);
			PUSH(d0.d_longdouble == d1.d_longdouble, sp);
			BREAK;
		CASE OC_LONG_DOUBLE_NOT_EQUAL:
			POP_LDOUBLE(d1, sp);
			POP_LDOUBLE(d0, sp);
			PUSH(d0.d_longdouble != d1.d_longdouble, sp);
			BREAK;
		CASE OC_LONG_DOUBLE_LESS:
			POP_LDOUBLE(d1, sp);
			POP_LDOUBLE(d0, sp);
			PUSH(d0.d_longdouble < d1.d_longdouble, sp);
			BREAK;
		CASE OC_LONG_DOUBLE_GREATER:
			POP_LDOUBLE(d1, sp);
			POP_LDOUBLE(d0, sp);
			PUSH(d0.d_longdouble > d1.d_longdouble, sp);
			BREAK;
		CASE OC_LONG_DOUBLE_LESS_OR_EQUAL:
			POP_LDOUBLE(d1, sp);
			POP_LDOUBLE(d0, sp);
			PUSH(d0.d_longdouble <= d1.d_longdouble, sp);
			BREAK;
		CASE OC_LONG_DOUBLE_GREATER_OR_EQUAL:
			POP_LDOUBLE(d1, sp);
			POP_LDOUBLE(d0, sp);
			PUSH(d0.d_longdouble >= d1.d_longdouble, sp);
			BREAK;
			
		CASE OC_CVT_DOUBLE_TO_LONG_DOUBLE:
			POP_DOUBLE(d0, sp);
			d1.d_longdouble = d0.d_double;
			PUSH_LDOUBLE(d1, sp);
			BREAK;
		CASE OC_CVT_FLOAT_TO_LONG_DOUBLE:
			POP_FLOAT(d0, sp);
			d1.d_longdouble = d0.d_float;
			PUSH_LDOUBLE(d1, sp);
			BREAK;
#if WANT_LL
		CASE OC_CVT_UNSIGNED_LONG_LONG_TO_LONG_DOUBLE:
			POP_LONGLONG(d0, sp);
			d1.d_longdouble = d0.d_ulonglong;
			PUSH_LDOUBLE(d1, sp);
			BREAK;
		CASE OC_CVT_SIGNED_LONG_LONG_TO_LONG_DOUBLE:
			POP_LONGLONG(d0, sp);
			d1.d_longdouble = d0.d_longlong;
			PUSH_LDOUBLE(d1, sp);
			BREAK;
#endif
		CASE OC_CVT_UNSIGNED_LONG_TO_LONG_DOUBLE:
			d0.d_longdouble = (unsigned long)POP(sp);
			PUSH_LDOUBLE(d0, sp);
			BREAK;
		CASE OC_CVT_SIGNED_LONG_TO_LONG_DOUBLE:
			d0.d_longdouble = (long)POP(sp);
			PUSH_LDOUBLE(d0, sp);
			BREAK;

		CASE OC_CVT_LONG_DOUBLE_TO_DOUBLE:
			POP_LDOUBLE(d0, sp);
			d1.d_double = d0.d_longdouble;
			PUSH_DOUBLE(d1, sp);
			BREAK;
#endif

		CASE OC_CVT_FLOAT_TO_DOUBLE:
			POP_FLOAT(d0, sp);
			d1.d_double = d0.d_float;
			PUSH_DOUBLE(d1, sp);
			BREAK;
#if WANT_LL
		CASE OC_CVT_UNSIGNED_LONG_LONG_TO_DOUBLE:
			POP_LONGLONG(d0, sp);
			d1.d_double = d0.d_ulonglong;
			PUSH_DOUBLE(d1, sp);
			BREAK;
		CASE OC_CVT_SIGNED_LONG_LONG_TO_DOUBLE:
			POP_LONGLONG(d0, sp);
			d1.d_double = d0.d_longlong;
			PUSH_DOUBLE(d1, sp);
			BREAK;
#endif
		CASE OC_CVT_UNSIGNED_LONG_TO_DOUBLE:
			d0.d_double = (unsigned long)POP(sp);
			PUSH_DOUBLE(d0, sp);
			BREAK;
		CASE OC_CVT_SIGNED_LONG_TO_DOUBLE:
			d0.d_double = (long)POP(sp);
			PUSH_DOUBLE(d0, sp);
			BREAK;

		CASE OC_CVT_FLOAT_TO_BOOL:
			POP_FLOAT(d0, sp);
			PUSH(d0.d_float != 0.0, sp);
			BREAK;
		CASE OC_CVT_DOUBLE_TO_BOOL:
			POP_DOUBLE(d0, sp);
			PUSH(d0.d_double != 0.0, sp);
			BREAK;
#if WANT_LDBL
		CASE OC_CVT_LONG_DOUBLE_TO_BOOL:
			POP_LDOUBLE(d0, sp);
			PUSH(d0.d_longdouble != 0.0, sp);
			BREAK;
#endif

#if WANT_LL		
		CASE OC_CVT_LONG_LONG_TO_BOOL:
			POP_LONGLONG(d0, sp);
			PUSH(d0.d_longlong != 0.0, sp);
			BREAK;

#if WANT_LDBL
		CASE OC_CVT_LONG_DOUBLE_TO_LONG_LONG:
			POP_LDOUBLE(d0, sp);
			d1.d_longlong = d0.d_longdouble;
			PUSH_LONGLONG(d1, sp);
			BREAK;
#endif
		CASE OC_CVT_DOUBLE_TO_LONG_LONG:
			POP_DOUBLE(d0, sp);
			d1.d_longlong = d0.d_double;
			PUSH_LONGLONG(d1, sp);
			BREAK;
		CASE OC_CVT_FLOAT_TO_LONG_LONG:
			POP_FLOAT(d0, sp);
			d1.d_longlong = d0.d_float;
			PUSH_LONGLONG(d1, sp);
			BREAK;
		CASE OC_CVT_UNSIGNED_LONG_TO_LONG_LONG:
			d0.d_longlong = (unsigned long)POP(sp);
			PUSH_LONGLONG(d0, sp);
			BREAK;
		CASE OC_CVT_SIGNED_LONG_TO_LONG_LONG:
			d0.d_longlong = (long)POP(sp);
			PUSH_LONGLONG(d0, sp);
			BREAK;

		CASE OC_CVT_UNSIGNED_LONG_LONG_TO_LONG:
			POP_LONGLONG(d0, sp);
			PUSH(d0.d_ulonglong, sp);
			BREAK;
		CASE OC_CVT_SIGNED_LONG_LONG_TO_LONG:
			POP_LONGLONG(d0, sp);
			PUSH(d0.d_longlong, sp);
			BREAK;
#endif

		CASE OC_CVT_FLOAT_TO_LONG:
			POP_FLOAT(d0, sp);
			PUSH(d0.d_float, sp);
			BREAK;
		CASE OC_CVT_DOUBLE_TO_LONG:
			POP_DOUBLE(d0, sp);
			PUSH(d0.d_double, sp);
			BREAK;
#if WANT_LDBL
		CASE OC_CVT_LONG_DOUBLE_TO_LONG:
			POP_LDOUBLE(d0, sp);
			PUSH(d0.d_longdouble, sp);
			BREAK;
#endif

		CASE OC_CVT_UNSIGNED_LONG_TO_FLOAT:
			d0.d_float = (unsigned long)POP(sp);
			PUSH_FLOAT(d0, sp);
			BREAK;
		CASE OC_CVT_SIGNED_LONG_TO_FLOAT:
			d0.d_float = (long)POP(sp);
			PUSH_FLOAT(d0, sp);
			BREAK;
#if WANT_LL
		CASE OC_CVT_UNSIGNED_LONG_LONG_TO_FLOAT:
			POP_LONGLONG(d0, sp);
			d1.d_float = d0.d_ulonglong;
			PUSH_FLOAT(d1, sp);
			BREAK;
		CASE OC_CVT_SIGNED_LONG_LONG_TO_FLOAT:
			POP_LONGLONG(d0, sp);
			d1.d_float = d0.d_longlong;
			PUSH_FLOAT(d1, sp);
			BREAK;
#endif
		CASE OC_CVT_DOUBLE_TO_FLOAT:
			POP_DOUBLE(d0, sp);
			d1.d_float = d0.d_double;
			PUSH_FLOAT(d1, sp);
			BREAK;
#if WANT_LDBL
		CASE OC_CVT_LONG_DOUBLE_TO_FLOAT:
			POP_LDOUBLE(d0, sp);
			d1.d_float = d0.d_longdouble;
			PUSH_FLOAT(d1, sp);
			BREAK;
#endif
		
		CASE OC_NOP:
			BREAK;
		CASE OC_TRAP:
			res = CI_ER_TRAP;
			goto quit;
		
#if !WANT_JUMPTAB
		default:
			panic("illegal opcode");
			break;
#endif
		}
	}

quit:
	ma->ma_pc = pc - 1;
	ma->ma_sp = sp;
	ma->ma_fp = fp;
	ma->ma_codefile = cf;
	ma->ma_retval = retval;

	ci_set_expecting_arithmetic_signals(FALSE);

	return res;
}

static void
spfailed(expected, got, location)
long expected, got, location;
{
	errf("SP botch (expected %ld, got %ld) pc=%ld",
		    expected, got, location);
	panic("sp botch");
}

void
ci_cause_exit(ma)
machine_t *ma;
{
	if (ma == NULL)
		ma = Current_machine;

	ma->ma_stopres = CI_ER_EXITED;
}

int
ci_get_exit_status(ma)
machine_t *ma;
{
	return ma->ma_retval.d_word;
}

int
ci_dump_machine_stats(ma, fp, path)
machine_t *ma;
FILE *fp;
const char *path;
{
	int i;
	unsigned long *counts;

	if ((counts = ma->ma_opcounts) == NULL) {
		errf("Can't show machine stats - not collected");
		return -1;
	}

	for (i = 0; i < NUM_OPCODES; ++i) {
		if (counts[i] != 0)
			fprintf(fp, "%3d: %ld\n", i, counts[i]);
	}

	if (ferror(fp) || fflush(fp) == EOF) {
		failmesg("Error writing to", "", path);
		return -1;
	}

	return 0;
}
