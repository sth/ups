#ifndef __UPS_SIGNAL_H
#define __UPS_SIGNAL_H

#ifndef __UPS_SIG_ATOMIC_T_DEFINED
	typedef int __sig_atomic_t;
#	define __UPS_SIG_ATOMIC_T_DEFINED
#endif

extern void (*signal(int, void (*)(int)))(int) ;
extern int raise  (int)  ;

#define SIG_ERR ((void (*)(int)) -1) /* Error return.  */
#define SIG_DFL ((void (*)(int)) 0)  /* Default action.  */
#define SIG_IGN ((void (*)(int)) 1)  /* Ignore signal.  */

/* Signals.  */
#define	SIGINT		2	/* Interrupt (ANSI).  */
#define	SIGILL		4	/* Illegal instruction (ANSI).  */
#define	SIGABRT		6	/* Abort (ANSI).  */
#define	SIGFPE		8	/* Floating-point exception (ANSI).  */
#define	SIGSEGV		11	/* Segmentation violation (ANSI).  */
#define	SIGTERM		15	/* Termination (ANSI).  */

#endif
