/* ao_procfs.c - process manipulation for a.out files using procfs */

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


/* @(#)ao_procfs.c	1.3 20 Jun 1995 (UKC) */
char ups_ao_procfs_c_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include "ao_ifdefs.h"

#if AO_USE_PROCFS

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/fault.h>
#include <sys/procfs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stropts.h>
#include <poll.h>

#include <local/ukcprog.h>
#include <mtrprog/utils.h>
#include <local/arg.h>
#include <local/wn.h>		/* for wn_get_wn_fds() */

#include "ups.h"
#include "symtab.h"
#include "target.h"
#include "ao_regs.h"
#include "ao_core.h"
#include "ao_target.h"
#include "ao_procfs.h"
#include "ao_elflib.h"
#include "ao_elfcore.h"
#include "breakpoint.h"
#include "obj_bpt.h"
#include "debug.h"
#include "ui.h"			/* for user_wants_stop() */
#include "tdr.h"
#include "state.h"

pid_t vfork(void);

#define PROCFS_PATH_SIZE	sizeof("/proc/nnnnn")

/* Used for reattaching to same target */
static int Last_attach_pid; 

struct Procfs_info {
	int procfd;
	char path[PROCFS_PATH_SIZE];
	bool clear_fault_on_resume;

	prgregset_t regtab;

	prfpregset_t fpregs;
	bool fpregs_updated;
	
	int maxsig;
	struct sigaction *handlers;
	bool handlers_updated;
	bool at_pause;
};

#ifdef ARCH_SUN4
#define IS_INTEGER_REG(regno)	((regno) < 32)
#define FLOAT_REGNO(regno)	((regno) - 32)
#endif
#ifdef ARCH_386
#define IS_INTEGER_REG(regno)	((regno) < _NGREG)
#define FLOAT_REGNO(regno)	((regno) - _NGREG)
#endif

int 
procfs_get_last_attach_pid()
{
  return Last_attach_pid;
}

static bool
open_procfs_file(pid_t pid, char *path, bool exclusive, int *p_procfd)
{
	int mode;
	
	sprintf(path, "/proc/%05d", (int)pid);

	mode = O_RDWR;
	
	if (exclusive)
		mode |= O_EXCL;
	
	if ((*p_procfd = open(path, mode)) == -1) {
		if (errno == EBUSY)
			errf("Someone else is already tracing process %ld",
			     (long)pid);
		else
			failmesg("Can't open", "procfs file", path);
		return FALSE;
	}

	return TRUE;
}

static bool
setup_procfs_in_child(const char *path, int procfd)
{
	sysset_t sysset;
	long flags;
	
	/*  Arrange to stop after execve of target process.
	 */
	premptyset(&sysset);
	praddset(&sysset, SYS_execve);

	if (ioctl(procfd, PIOCSEXIT, &sysset) != 0) {
		failmesg("Can't do PIOCSEXIT SYS_execve on", "", path);
		return FALSE;
	}

	/*  Clear tracing flags on fork.
	 */
	flags = PR_FORK;
	if (ioctl(procfd, PIOCRESET, &flags) != 0) {
		failmesg("Can't do PIOCRESET PR_FORK on", "", path);
		return FALSE;
	}

	return TRUE;
}

static void
close_procfs(Procfs_info *pi)
{
	if (pi->procfd != -1)
		close(pi->procfd);

	if (pi->handlers)
	  free(pi->handlers);
	free(pi);
}

static bool
procfn(Procfs_info *pi, int op, void *p, const char *action)
{
	if (ioctl(pi->procfd, op, p) != 0) {
		errf("Can't %s on %s: %s", action, pi->path, action);
		return FALSE;
	}

	return TRUE;
}

static bool
setup_procfs_in_parent(iproc_t *ip, int pid, bool attached)
{
	sigset_t sigset;
	fltset_t fltset;
	long flags;
	Procfs_info *pi;
	
	pi = e_malloc(sizeof(Procfs_info));
	
	/* RGA for purify */
	memset((char *)pi, '\0', sizeof(Procfs_info));

	if (!open_procfs_file(pid, pi->path, attached, &pi->procfd)) {
	  if (pi->handlers)
	    free(pi->handlers);
	  free(pi);
	  return FALSE;
	}

	if (!procfn(pi, PIOCMAXSIG, &pi->maxsig, "do PIOCMAXSIG") != 0)
		panic("ioctl PIOCMAXSIG failed");
	pi->handlers = e_malloc(pi->maxsig * sizeof(struct sigaction));
	pi->handlers_updated = FALSE;

	pi->fpregs_updated = FALSE;
	    
	/*  Trace all signals except SIGKILL.
	 */
	prfillset(&sigset);
	prdelset(&sigset, SIGKILL);

	if (!procfn(pi, PIOCSTRACE, &sigset, "do PIOCSTRACE")) {
		close_procfs(pi);
		return FALSE;
	}

	/*  Trace breakpoint and single step faults.
	 *
	 *  TODO: catch other faults and present useful information
	 *        about them in the stack trace.
	 */
	premptyset(&fltset);
	praddset(&fltset, FLTBPT);
	praddset(&fltset, FLTTRACE);

	if (!procfn(pi, PIOCSFAULT, &fltset, "do PIOCSFAULT")) {
		close_procfs(pi);
		return FALSE;
	}

	flags = attached ? PR_RLC : PR_KLC;
	
	if (!procfn(pi, PIOCSET, &flags, "do PIOCSET") != 0) {
		close_procfs(pi);
		return FALSE;
	}

	/*  Keep things deterministic.
	 */
	memset(&pi->regtab, '\0', sizeof(pi->regtab));
	memset(&pi->fpregs, '\0', sizeof(pi->fpregs));
	pi->clear_fault_on_resume = FALSE;

	if (ip->ip_procfs_info != NULL)
		close_procfs(ip->ip_procfs_info);
	
	ip->ip_pid = pid;
	ip->ip_core = NULL;
	ip->ip_attached = attached;
	ip->ip_procfs_info = pi;

	return TRUE;
}

void
procfs_init(iproc_t *ip)
{
	Procfs_info *pi;
	
	if (ip->ip_core == NULL) {
		pi = NULL;
	}
	else {
		Elf_core_regs *cr;

		pi = e_malloc(sizeof(Procfs_info));
		*pi->path = '\0';
		pi->procfd = -1;
		pi->maxsig = 0;
		pi->handlers = NULL;
		pi->handlers_updated = TRUE;
		pi->at_pause = FALSE;

		cr = (Elf_core_regs *)core_getregs(ip->ip_core);
	
		if (cr->p_fpregs != NULL)
			memcpy(&pi->fpregs, cr->p_fpregs, sizeof(pi->fpregs));
		else
			memset(&pi->fpregs, '\0', sizeof(pi->fpregs));
		pi->fpregs_updated = TRUE;
	
		memcpy(pi->regtab, cr->regtab, sizeof(pi->regtab));
		pi->clear_fault_on_resume = FALSE;
	}

	ip->ip_procfs_info = pi;
}
	       
static void
note_target_dead(target_t *xp)
{
	iproc_t *ip;
	
	ip = GET_IPROC(xp);

	close_procfs(ip->ip_procfs_info);
	ip->ip_procfs_info = NULL;
	
	ip->ip_pid = 0;
	ip->ip_stopres = SR_DIED;
	ip->ip_attached = FALSE;

	mark_breakpoints_as_uninstalled(xp);

	elf_zap_solib_addrs(xp);
}

static bool
wait_for_stop(Procfs_info *pi)
{
	enum { WN, PROC };
	struct pollfd fds[2];
	unsigned long fdmask;
	int wn_fd;
	bool user_stopped;
	int alarm_time;

	user_stopped = FALSE;
	fdmask = wn_get_wn_fds();
	if ((alarm_time = get_run_alarm_time()) > 0)
	    alarm_time /= 1000;	/* convert from micro-secs to milli-secs */
	else
	    alarm_time = INFTIM;

	if (fdmask == 0)
		return user_stopped;

	/*  We don't handle multiple wn fds.  All the world is X these days.
	 */
	for (wn_fd = 0; (fdmask & 1) == 0; ++wn_fd)
		fdmask >>= 1;
	
	fds[WN].fd = wn_fd;
	fds[WN].events = POLLRDNORM;

	fds[PROC].fd = pi->procfd;
	fds[PROC].events = POLLPRI;

	for (;;) {
		int res;

		res = poll(fds, 2, alarm_time);

		if (res < 0) {
			errf("Warning: poll returned %d (%s)",
			     res, get_errno_str());
			return user_stopped;
		}

		if (res == 0) {
			run_alarm_time_expired();
			alarm_time = INFTIM;
			continue;
		}

		if (fds[WN].revents != 0) {
			if (user_wants_stop(FALSE)) {
			  if (pi->at_pause == TRUE) 
			    /* RGA is case target has no SIGCONT handler */
			  {
			    int signo;

			    signo = SIGSTOP;
			    if (ioctl(pi->procfd, PIOCKILL, &signo) != 0) {
			      failmesg
				("Warning: ioctl PIOCKILL SIGSTOP failed on",
				 "", pi->path);
			    }
			  }
				procfn(pi, PIOCSTOP, NULL,
				       "stop target with PIOCSTOP");
				user_stopped = TRUE;
				break;
			}
		}
		else if (fds[PROC].revents != 0) {
			break;
		}
		else {
			errf("Warning: poll() returned with no events set");
		}
	}
	return user_stopped;
}
		
static void
wait_for_target_to_stop(target_t *xp)
{
	Procfs_info *pi;
	prstatus_t status;
	stopres_t stopres;
	int lastsig;
	iproc_t *ip;
	bool user_stopped;

	ip = GET_IPROC(xp);
	pi = ip->ip_procfs_info;

	user_stopped = wait_for_stop(pi);

	/* RGA for purify */
	memset((char *)&status, '\0', sizeof(prstatus_t));
	if (ioctl(pi->procfd, PIOCWSTOP, &status) != 0) {
		if (errno == ENOENT) {
			note_target_dead(xp);
		}
		else {
			failmesg("ioctl PIOCWSTOP failed on", "", pi->path);
			ip->ip_stopres = SR_FAILED;
		}
		
		return;
	}

	lastsig = 0;
	pi->clear_fault_on_resume = FALSE;
	
	switch (status.pr_why) {
	case PR_REQUESTED:
		stopres = SR_USER;
		break;
	case PR_SIGNALLED:
	case PR_JOBCONTROL:
		stopres = SR_SIG;
		lastsig = status.pr_what;
		break;
	case PR_FAULTED:
		switch (status.pr_what) {
		case FLTBPT:
			if (user_stopped)    /* RGA so Stop button breaks */
			  stopres = SR_USER; /* out of a bad jump to here */
			else
			  stopres = SR_BPT;
			status.pr_reg[R_PC] = status.pr_reg[R_PC] - BPT_PC_OFFSET;
			pi->clear_fault_on_resume = TRUE;
			break;
		case FLTTRACE:
			if (user_stopped)    /* RGA so Stop button breaks */
			  stopres = SR_USER; /* out of a Step */
			else
			  stopres = SR_SSTEP;
			pi->clear_fault_on_resume = TRUE;
			break;
		default:
			errf("Process stopped on unknown fault %d",
			     status.pr_what);
			stopres = SR_FAILED;
			break;
		}
		break;
	case PR_SYSENTRY:
		errf("Process stopped with unexpected PR_SYSENTRY %d",
		     status.pr_what);
		stopres = SR_FAILED;
		break;
	case PR_SYSEXIT:
		if (status.pr_what == SYS_execve) {
			stopres = SR_BPT;
		}
		else {
			errf("Process stopped with unexpected PR_SYSEXIT %d",
			     status.pr_what);
			stopres = SR_FAILED;
		}
		break;
	case PR_SUSPENDED:
		errf("Process stopped with unexpected PR_SUSPENDED");
		stopres = SR_FAILED;
		break;
	default:
		errf("Process stopped with unknown pr_why value %d",
		     status.pr_why);
		stopres = SR_FAILED;
		break;
	}

	if (stopres != SR_FAILED && stopres != SR_DIED) {
		memcpy(pi->regtab, status.pr_reg, sizeof(pi->regtab));
		ip->ip_restart_pc = pi->regtab[R_PC];

		pi->handlers_updated = FALSE;
		pi->fpregs_updated = FALSE;
	}
	if (status.pr_syscall == SYS_pause) /* RGA added to break out */
					    /* of a pause()  */
	  pi->at_pause = TRUE;
	else
	  pi->at_pause = FALSE;

	ip->ip_stopres = stopres;
	ip->ip_lastsig = lastsig;
}

static void
write_to_stderr(const char *mesg)
{
	write(2, mesg, strlen(mesg));
	write(2, "\n", 1);
}

static const char *
mflags_to_string(unsigned long flags)
{
	static char buf[7];

	buf[0] = (flags & MA_STACK) ? 's' : '-';
	buf[1] = (flags & MA_BREAK) ? 'b' : '-';
	buf[2] = (flags & MA_READ)  ? 'r' : '-';
	buf[3] = (flags & MA_WRITE) ? 'w' : '-';
	buf[4] = (flags & MA_EXEC)  ? 'x' : '-';
	buf[5] = (flags & MA_SHARED) ? 'S' : '-';
	buf[6] = '\0';

	return buf;
}

static void
dump_mappings(int procfd, prmap_t *prtab, int nmap)
{
	errf_ofunc_t old_ofunc;
	int i;
	
	old_ofunc = errf_set_ofunc(write_to_stderr);

	printf("    %8s  %7s  %6s  %5s  %8s  %6s  %8s\n",
	       "vaddr", "msize", "off", "flags", "dev", "ino", "fsize");
	
	for (i = 0; i < nmap; ++i) {
		int fd;
		struct stat stbuf;
		
		printf("%2d: %8p  %7ld  %6ld  %s  ",
		       i, prtab[i].pr_vaddr, prtab[i].pr_size,
		       prtab[i].pr_off, mflags_to_string(prtab[i].pr_mflags));
		
		fd = ioctl(procfd, PIOCOPENM, &prtab[i].pr_vaddr);

		if (fd == -1) {
			if (errno != EINVAL)
				printf("PIOCOPENM failed: %s", get_errno_str());
		}
		else if (fstat(fd, &stbuf) != 0) {
			printf("Can't fstat fd %d: %s", fd, get_errno_str());
		}
		else {
			printf("%8lx  %6ld  %8ld",
			       (long)stbuf.st_dev, (long)stbuf.st_ino,
			       (long)stbuf.st_size);
		}
		fputc('\n', stdout);

		close(fd);
	}

	errf_set_ofunc(old_ofunc);
}

bool
procfs_load_mmap_info(target_t *xp, bool attached)
{
	typedef struct Seen {
		dev_t dev;
		ino_t ino;
		struct Seen *next;
	} Seen;
	Procfs_info *pi;
	alloc_pool_t *tmpap;
	prmap_t *prtab;
	int i, nmap;
	Seen *seenlist;

	pi = GET_IPROC(xp)->ip_procfs_info;
	
	/* RGA for purify */
	nmap = 0;
	if (!procfn(pi, PIOCNMAP, &nmap, "do PIOCNMAP"))
		return FALSE;

	tmpap = alloc_create_pool();
	prtab = alloc(tmpap, (nmap + 1) * sizeof(struct prmap));

	if (!procfn(pi, PIOCMAP, prtab, "do PIOCMAP")) {
		alloc_free_pool(tmpap);
		return FALSE;
	}

	if ((Debug_flags & DBFLAG_DUMP_MMAP_INFO) != 0)
		dump_mappings(pi->procfd, prtab, nmap);

	seenlist = NULL;
	
	for (i = 0; i < nmap; ++i) {
		int fd;
		struct stat stbuf;
		prmap_t *pr;

		pr = &prtab[i];
		fd = ioctl(pi->procfd, PIOCOPENM, &pr->pr_vaddr);

		if (fd == -1) {
			if (errno != EINVAL) {
				errf("Can't open fd on file mapped at "
				     "virtual address %p: %s",
				     pr->pr_vaddr, get_errno_str());
			}
		}
		else if (fstat(fd, &stbuf) != 0) {
			printf("Can't fstat file mapped at "
			       "virtual address %p: %s",
			       pr->pr_vaddr, get_errno_str());
		}
		else {
			Seen *seen;

			/*  We get several entries for each shared library,
			 *  so skip ones we've seen before.
			 *
			 *  This is vital on Solaris 2.3, where we get a
			 *  bogus second entry for libc in the list of
			 *  mappings (this doesn't happen on Solaris 2.4).
			 *
			 *  TODO: replace all this stuff with a breakpoint
			 *        at the point where shared libraries are
			 *        mapped (see <link.h>).
			 */
			for (seen = seenlist; seen != NULL; seen = seen->next) {
				if (seen->dev == stbuf.st_dev &&
				    seen->ino == stbuf.st_ino) {
					break;
				}
			}

			if (seen == NULL) {
				seen = alloc(tmpap, sizeof(Seen));
				seen->dev = stbuf.st_dev;
				seen->ino = stbuf.st_ino;
				seen->next = seenlist;
				seenlist = seen;

				elf_note_shlib_addr(xp->xp_apool, GET_IPROC(xp),
						    stbuf.st_dev, stbuf.st_ino,
						    stbuf.st_size,
#ifndef OS_SUNOS
						    0,0,
#else
						    stbuf.st_mtim.tv_sec,
						    stbuf.st_mtim.tv_nsec,
#endif
						    pr->pr_off,
						    (taddr_t)pr->pr_vaddr,
						    (size_t)pr->pr_pagesize);
			}
		}
		
		if (fd != -1)
		  close(fd);
	}

	alloc_free_pool(tmpap);

/*	if (!attached)*/ 
	recalculate_bpt_addrs(xp);

	return TRUE;
}

int 
procfs_create_child(xp, argv, envp, rdlist, p_stopres)
target_t *xp;
const char **argv, **envp;
long rdlist;
stopres_t *p_stopres;
{
	iproc_t *ip;
	int pid;
	taddr_t main_addr, main_min_bpt_addr;
	errf_ofunc_t old_ofunc;

	ip = GET_IPROC(xp);
	
	fflush(stdout);
	fflush(stderr);

	/*  On Solaris 2, we hang if the target is not executable.
	 *  So check that the file is executable.
	 *
	 *  This is a little dubious, because the check might report
	 *  the wrong answer, but at least we don't wedge on the obvious
	 *  case of no execute bits being set.
	 */
	if (access(xp->xp_textpath, X_OK) != 0) {
		errf("%s does not have execute permission", xp->xp_textpath);
		return -1;
	}
	
	old_ofunc = errf_set_ofunc(write_to_stderr);

	if ((pid = vfork()) == 0) {
		char path[PROCFS_PATH_SIZE];
		int procfd;
		
		arg_do_redirs_in_child(rdlist);

		if (!open_procfs_file(getpid(), path, FALSE, &procfd))
			_exit(1);

		if (!setup_procfs_in_child(path, procfd))
			_exit(1);

		close(procfd);
		
		execve(xp->xp_textpath, (char **)argv, (char **)envp);
		perror(xp->xp_textpath);
		_exit(1);
	}

	errf_set_ofunc(old_ofunc);

	arg_tidy_redirs_in_parent(rdlist);
	if (pid == -1) {
		failmesg("Can't fork to run", "", xp->xp_textpath);
		*p_stopres = SR_DIED;
		return -1;
	}

	if (!setup_procfs_in_parent(ip, pid, FALSE)) {
		/*  Clear away remains.  No error checking, as we have
		 *  already given an error message.
		 */
		kill(pid, SIGKILL);
		waitpid(pid, (int *)NULL, 0);
		
		*p_stopres = SR_FAILED;
		return -1;
	}
	
	wait_for_target_to_stop(xp);

	if (ip->ip_stopres != SR_BPT) {
		if (ip->ip_stopres == SR_DIED) {
			errf("Can't start %s", xp->xp_textpath);
		}
		else {
			errf("Process not stopped properly on startup");
			xp_kill(xp);
		}
	
		return -1;
	}

	*p_stopres = xp_execto(xp, xp_get_entryaddr(xp));
	if (*p_stopres != SR_BPT) {
		errf("Can't run process to start of main function");
		if (*p_stopres != SR_DIED)
			xp_kill(xp);
		return -1;
	}
	if (!procfs_load_mmap_info(xp, FALSE)) {
		xp_kill(xp);
		return -1;
	}
#ifdef AO_ELF
	elf_update_dynamic_solibs(xp->xp_apool, xp->xp_textpath,
				  &ip->ip_solibs, TRUE); 
#endif
	if (get_startup_stop_addrs(xp, &main_addr, &main_min_bpt_addr) != 0) {
		xp_kill(xp);
		return -1;
	}
	*p_stopres = xp_execto(xp, main_addr);

	return 0;
}

int
procfs_attach_to_process(xp, pid)
target_t *xp;
int pid;
{
	iproc_t *ip;
	int reattach, initial_last_attach_pid;

	initial_last_attach_pid =  Last_attach_pid;
	if (pid == 0)
	  pid = Last_attach_pid;
	else
	  Last_attach_pid = pid;

	ip = GET_IPROC(xp);

	if (!setup_procfs_in_parent(ip, pid, TRUE))
		return -1;

	if (!procfn(ip->ip_procfs_info, PIOCSTOP, NULL,
		    "stop target with PIOCSTOP")) {
		procfs_kill(xp);
		return -1;
	}
	
	wait_for_target_to_stop(xp);
	
	reattach = Last_attach_pid != 0;

	if (!procfs_load_mmap_info(xp, TRUE)) {
	  close_procfs(ip->ip_procfs_info);
	  ip->ip_procfs_info = NULL;
	  return -1;
	}
	if (reattach)
	  rescan_init_file();

	set_current_target(xp);
	/* RGA this clause needs to skipped when using the
	   incomplete elf_make_main_symtab() hack */
#ifdef AO_ELF
	xp->xp_new_dynamic_libs = 
	  elf_get_dynamic_solibs(xp->xp_apool, xp->xp_textpath,
				 &ip->ip_solibs, TRUE, FALSE);
	elf_update_dynamic_solibs(xp->xp_apool, xp->xp_textpath, &ip->ip_solibs, TRUE);
#endif
	return 0;
}

void
procfs_kill(xp)
target_t *xp;
{
	int signo;
	iproc_t *ip;
	Procfs_info *pi;

	ip = GET_IPROC(xp);
	pi = ip->ip_procfs_info;
	
	signo = SIGKILL;
	if (ioctl(pi->procfd, PIOCKILL, &signo) != 0) {
		failmesg("Warning: ioctl PIOCKILL SIGKILL failed on",
			 "", pi->path);
	}
	else if (!ip->ip_attached &&
		 waitpid(ip->ip_pid, (int *)NULL, 0) == -1) {
		errf("Warning: waitpid for process %d failed: %s",
		     ip->ip_pid, get_errno_str());
	}

	note_target_dead(xp);
}

void
procfs_stop(xp)
target_t *xp;
{
	iproc_t *ip;

	ip = GET_IPROC(xp);
	if (ip != 0) {
	  kill(ip->ip_pid, SIGSTOP);
	}
}

static bool
resume_execution(target_t *xp, taddr_t restart_pc, int sig, long flags)
{
	iproc_t *ip;
	Procfs_info *pi;
	prrun_t prrun;
	int signo;

	ip = GET_IPROC(xp);
	pi = ip->ip_procfs_info;
	
	memset(&prrun, '\0', sizeof(prrun));

	/*  We used to always set PRSVADDR here and using ip_restart_pc,
	 *  but it breaks (steps incorrectly) on some lines of code.
	 *  Maybe we should use nPC?  For now, only set PRSVADDR when
	 *  we are passed an explicit starting address or when we are
	 *  restarting after a breakpoint was hit, as we may have rolled
	 *  back the PC in that case.
	 */
	if (restart_pc != 0) {
		prrun.pr_vaddr = (caddr_t)restart_pc;
		flags |= PRSVADDR;
	}
	else if (ip->ip_stopres == SR_BPT) {
		prrun.pr_vaddr = (caddr_t)ip->ip_restart_pc;
		flags |= PRSVADDR;
	}
	
	prrun.pr_flags = flags;

	if (sig == 0)
		prrun.pr_flags |= PRCSIG;

	if (pi->clear_fault_on_resume) {
		prrun.pr_flags |= PRCFAULT;
		pi->clear_fault_on_resume = FALSE;
	}
	
	if (!procfn(pi, PIOCRUN, &prrun, "do PIOCRUN") != 0) {
		ip->ip_stopres = SR_FAILED;
		return FALSE;
	}

	if (pi->at_pause == TRUE) /* RGA added to break out of a pause()  */
	{
	  signo = SIGCONT;
	  if (ioctl(pi->procfd, PIOCKILL, &signo) != 0) {
	    failmesg("Warning: ioctl PIOCKILL SIGCONT failed on",
		     "", pi->path);
	  }
	}

	return TRUE;
}

void
procfs_detach_from_process(xp)
target_t *xp;
{
	Procfs_info *pi;
	pi = GET_IPROC(xp)->ip_procfs_info;

	/* RGA clear pause flag on detach */
	pi->at_pause = FALSE;
	resume_execution(xp, 0, 0, 0);
	
	/*  The PR_RLC flag takes care of setting the target running,
	 *  so all we have to do is close the procfs file descriptor.
	 */
	note_target_dead(xp);
}

static stopres_t
run_target(target_t *xp, taddr_t restart_pc, int sig, long flags)
{
	if (resume_execution(xp, restart_pc, sig, flags))
		wait_for_target_to_stop(xp);

	return GET_IPROC(xp)->ip_stopres;
}

stopres_t
procfs_continue(xp, restart_pc, sig)
target_t *xp;
int sig;
taddr_t restart_pc;
{
	return run_target(xp, restart_pc, sig, 0L);
}

stopres_t
procfs_single_step(xp, sig)
target_t *xp;
int sig;
{
#ifdef ARCH_SUN4
	Procfs_info *pi;
	stopres_t stopres;

	pi = GET_IPROC(xp)->ip_procfs_info;

	for (;;) {
		stopres = run_target(xp, 0, sig, PRSTEP);

		if (stopres != SR_SSTEP)
			break;
		
		if (pi->regtab[R_nPC] == pi->regtab[R_PC] + 4)
			break;

		/*  We are in the delay slot of a control transfer
		 *  instruction - do another single step.
		 */
	}

	return stopres;
#else
	return run_target(xp, 0, sig, PRSTEP);
#endif
}

static int
get_machine_regno(int upsreg)
{
	switch(upsreg) {
	case UPSREG_PC:
		return R_PC;
	case UPSREG_SP:
		return R_SP;
	case UPSREG_FP:
	case UPSREG_AP:
		return R_FP;
#if (defined R_PSR)
	case UPSREG_CONDITION_CODES:
		return R_PSR;
#elif (defined R_PS)
	case UPSREG_CONDITION_CODES:
		return R_PS;
#endif
	default:
		if (upsreg < 0 || upsreg >= NPRGREG)
			panic("bad reg in pr");
#ifdef ARCH_386
                return x86_gcc_register(upsreg);
#else
                return upsreg;
#endif
	}
}

#ifdef ARCH_386

static bool
get_fpreg_addr(Procfs_info *pi, int regno, int num_bytes, void **p_addr)
{
	enum { N_FP_REGS = (sizeof(pi->fpregs.fp_reg_set.f_fpregs) / 
			    sizeof(*pi->fpregs.fp_reg_set.f_fpregs)) };

	panic ("get_fpreg_addr: NYI");
}

#else

static bool
get_fpreg_addr(Procfs_info *pi, int regno, int num_bytes, void **p_addr)
{
	enum { N_FP_REGS = (sizeof(pi->fpregs.pr_fr.pr_regs) / 
			    sizeof(*pi->fpregs.pr_fr.pr_regs)) };

	if (num_bytes == sizeof(double) && regno % 2 != 0) {
		errf("Illegal odd double fp register %d", regno);
		return FALSE;
	}
	
	if (regno < 32) {
		*p_addr = &pi->regtab[regno];
		return TRUE;
	}

	regno = FLOAT_REGNO(regno);
	
	if (regno < 0 || regno >= N_FP_REGS) {
		errf("Fp register number %d not in range 0..%d",
		     regno, N_FP_REGS - 1);
		return FALSE;
	}
			 
	if (!pi->fpregs_updated) {
		if (!procfn(pi, PIOCGFPREG, &pi->fpregs,
			    "fetch fp registers with PIOCGFPREG")) {
			return FALSE;
		}
		pi->fpregs_updated = TRUE;
	}

	*p_addr = &pi->fpregs.pr_fr.pr_regs[regno];
	return TRUE;
}

#endif

int
procfs_readreg(ip, upsreg, p_val)
iproc_t *ip;
int upsreg;
taddr_t *p_val;
{
	Procfs_info *pi;

	pi = ip->ip_procfs_info;

	if (IS_INTEGER_REG(upsreg)) {
		*p_val = pi->regtab[get_machine_regno(upsreg)];
	}
	else {
		void *addr;

		if (!get_fpreg_addr(pi, upsreg, sizeof(float), &addr))
			return -1;

		memcpy(p_val, addr, sizeof(*p_val));
	}
	
	return 0;
}

int
procfs_read_fpreg(ip, regno, num_bytes, p_val)
iproc_t *ip;
int regno;
int num_bytes;
fpval_t *p_val;
{
	void *addr;
	
	if (!get_fpreg_addr(ip->ip_procfs_info, regno, num_bytes, &addr))
		return -1;

 	switch (num_bytes) {
 	    case sizeof(float):
 		memcpy(&p_val->f, addr, num_bytes);
 		break;
 	    case sizeof(double):
 		memcpy(&p_val->d, addr, num_bytes);
 		break;
#if HAVE_LONG_DOUBLE
 	    case sizeof(long double):
 		memcpy(&p_val->ld, addr, num_bytes);
 		break;
#endif
 	}

	return 0;
}

int
procfs_setreg(ip, upsreg, value)
iproc_t *ip;
int upsreg;
taddr_t value;
{
	Procfs_info *pi;

	pi = ip->ip_procfs_info;
	
	if (IS_INTEGER_REG(upsreg)) {
		pi->regtab[get_machine_regno(upsreg)] = value;

		if (!procfn(pi, PIOCSREG, pi->regtab,
			    "set registers with PIOCSREG")) {
			return -1;
		}
	}
	else {
		void *addr;
		
		/*  Floating point register.
		 */
		if (!get_fpreg_addr(pi, upsreg, sizeof(float), &addr))
			return -1;
	
		memcpy(addr, &value, sizeof(value));

		/*  Now pi->fpregs is no longer up to date.  Even if the
		 *  PIOCSFPREG succeeds, we want to read the values back
		 *  again.
		 */
		pi->fpregs_updated = FALSE;

		if (!procfn(pi, PIOCSFPREG, &pi->fpregs,
			    "set fp registers with PIOCSFPREG")) {
			return -1;
		}
	}
	
	return 0;
}

void
procfs_get_regtab(iproc_t *ip, int *regtab)
{
	memcpy(regtab, ip->ip_procfs_info->regtab, sizeof(prgregset_t));
}

int
procfs_get_reg_index(Procfs_regname regname)
{
	switch (regname) {
	case PROCFS_REG_SP:
		return R_SP;
#ifdef R_O0
	case PROCFS_REG_O0:
		return R_O0;
#endif
	case PROCFS_REG_NREGS:
		return NPRGREG;
	default:
		panic("unknown regname in pgri");
		return 0;	/* to satisfy gcc */
	}
}

bool
procfs_set_all_regs(iproc_t *ip, int *regtab)
{
	Procfs_info *pi;

	pi = ip->ip_procfs_info;
	
	if (!procfn(pi, PIOCSREG, regtab, "set registers with PIOCSREG"))
		return FALSE;

	memcpy(pi->regtab, regtab, sizeof(prgregset_t));
	return TRUE;
}

sigstate_t
procfs_get_sigstate(ip, signo)
iproc_t *ip;
int signo;
{
	typedef void (*Sigfunc)(int); /* Actually any func ptr type will do */
	Procfs_info *pi;
	sigstate_t res;
	Sigfunc sigfunc;

	pi = ip->ip_procfs_info;
	
	if (signo < 1 || signo > pi->maxsig) {
		errf("Signal %d not in range 1..%d - assumed not caught",
		     signo, pi->maxsig);
		return SGH_DEFAULT;
	}

	if (!pi->handlers_updated) {
		if (!procfn(pi, PIOCACTION, pi->handlers, "do PIOCACTION"))
			return SGH_DEFAULT;

		pi->handlers_updated = TRUE;
	}

	sigfunc = (Sigfunc)pi->handlers[signo - 1].sa_handler;

	if (sigfunc == (Sigfunc)SIG_DFL) {
		res = SGH_DEFAULT;
	}
	else if (sigfunc == (Sigfunc)SIG_IGN) {
		res = SGH_IGNORED;
	}
	else {
		res = SGH_CAUGHT;
	}

	return res;
}

int
procfs_read_data(ip, addr, buf, nbytes)
iproc_t *ip;
taddr_t addr;
char *buf;
size_t nbytes;
{
	int fd;

	if (ip->ip_core != NULL)
		return core_dread(ip->ip_core, addr, buf, nbytes) ? 0 : -1;
	
	fd = ip->ip_procfs_info->procfd;

	if (lseek(fd, (off_t)addr, SEEK_SET) == -1 ||
	    read(fd, buf, nbytes) != nbytes)
		return -1;

	return 0;
}

int
procfs_write_data(ip, addr, buf, nbytes)
iproc_t *ip;
taddr_t addr;
const char *buf;
size_t nbytes;
{
	int fd;
	
	fd = ip->ip_procfs_info->procfd;

	if (lseek(fd, (off_t)addr, SEEK_SET) == -1 ||
	    write(fd, buf, nbytes) != nbytes)
		return -1;

	return 0;
}

int
procfs_install_watchpoint(ip, addr, size)
iproc_t *ip;
taddr_t addr;
size_t size;
{
	return -1;
}

int
procfs_uninstall_watchpoint(ip, addr, size, watchnum)
iproc_t *ip;
taddr_t addr;
size_t size;
int watchnum;
{
	return -1;
}

int
procfs_load_library(xp, arg)
target_t *xp;
char *arg;
{
  load_library_input((void *)xp, arg);
  return -1;
}

#endif /* AO_USE_PROCFS */

