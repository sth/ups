/* arg_redir.c - handle Bourne shell and csh type i/o redirection */

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


/* @(#)arg_redir.c	1.13 21 Apr 1994 (UKC) */
char arg_redir_rcsid[] = "$Id$";

#include <mtrprog/ifdefs.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifdef __STDC__
#include <unistd.h>
#endif

#ifndef FD_SETSIZE
#ifdef __FD_SETSIZE
#define FD_SETSIZE __FD_SETSIZE
#else
#error Unable to determine the size of an fd_set
#endif
#endif

#ifndef SEEK_END
#define SEEK_END	2
#endif

#include <local/ukcprog.h>

#include "arg.h"

typedef enum actionen {
	RDA_CLOSE,		/* close(fd1)		*/
	RDA_DUP,		/* dup2(fd1, fd2)	*/
	RDA_DUP_AND_CLOSE	/* dup2(fd1, fd2); close(fd1) */
} action_t;

typedef struct rdlistst {
	action_t rdl_action;
	int rdl_fd1;
	int rdl_fd2;
	struct rdlistst *rdl_next;
} rdlist_t;

#ifdef DEBUG
#define rda_close(fd)		errf("\tclose(%d)", fd)
#define rda_dup2(fd1, fd2)	errf("\tdup2(%d, %d)", fd1, fd2)
#else
#define rda_close(fd)		close(fd)
#define rda_dup2(fd1, fd2)	dup2(fd1, fd2)
#endif /* !DEBUG */

static int dtablesize PROTO((void));
static int getfd PROTO((const char **p_s, int *p_fd));
static rdlist_t *make_rdl PROTO((action_t action, int fd1, int fd2, rdlist_t **p_first, rdlist_t *last));

static fd_set saved_fds;

static int
dtablesize()
{
	static int dts = 0;
	
#if defined(_SC_OPEN_MAX)
	if (dts == 0)
		dts = sysconf(_SC_OPEN_MAX);
#elif defined(OPEN_MAX)
	if (dts == 0)
		dts = OPEN_MAX;
#endif

	if (dts <= 0)
		dts = 64;

	return dts;
}

static int
getfd(p_s, p_fd)
const char **p_s;
int *p_fd;
{
	int fd;
	const char *s;

	s = *p_s;
	fd = 0;
	while (*s != '\0' && isdigit(*s))
		fd = fd * 10 + *s++ - '0';
	if (fd < 0 || fd > dtablesize()) {
		errf("file descriptor %d is out of range", fd);
		return -1;
	}
	*p_fd = fd;
	*p_s = s;
	return 0;
}

static rdlist_t *
make_rdl(action, fd1, fd2, p_first, last)
action_t action;
int fd1, fd2;
rdlist_t **p_first, *last;
{
	rdlist_t *rdl;

	rdl = (rdlist_t *) e_malloc(sizeof(rdlist_t));
	rdl->rdl_action = action;
	rdl->rdl_fd1 = fd1;
	rdl->rdl_fd2 = fd2;
	
	if (last != NULL)
		last->rdl_next = rdl;
	else
		*p_first = rdl;
	
	rdl->rdl_next = NULL;
	
	return rdl;
}

typedef struct {
	const char *rd_pat;
	bool rd_takes_arg;
} redir_desc_t;

redir_desc_t Rdtab[] = {
	{ "d>&d",	FALSE },
	{ "d>&-",	FALSE },
	{ "d<&d",	FALSE },
	{ "d<&-",	FALSE },
	{ ">&-",	FALSE },
	{ "<&-",	FALSE },
	{ "<&d",	FALSE },
	{ ">&d",	FALSE },
	{ ">>",		TRUE  },
	{ ">",		TRUE  },
	{ "d>",		TRUE  },
	{ "<",		TRUE  },
	{ "d<",		TRUE  },
	{ ">&",		TRUE  },
};

#define RDTAB_SIZE (sizeof Rdtab / sizeof *Rdtab)

int
arg_convert_redir_to_shell_syntax(p_s, p_redir_str, p_postarg_str)
const char **p_s, **p_redir_str, **p_postarg_str;
{
	char patbuf[10];
	const char *s, *patlim;
	char *pat;
	redir_desc_t *rd;

	s = *p_s;
	pat = patbuf;
	patlim = pat + sizeof(patbuf) - 1;

	if (isdigit(*s)) {
		++s;
		*pat++ = 'd';
	}

	if (*s != '<' && *s != '>') {
		*p_redir_str = NULL;	/* Not a redirection */
		return 0;
	}
	*pat++ = *s++;

	if (*s == '&' || *s == '>')
		*pat++ = *s++;

	while (*s != '\0' && strchr("<>&-", *s) != NULL) {
		if (pat < patlim)
			*pat++ = *s++;
	}

	if (isdigit(*s)) {
		++s;
		if (pat < patlim)
			*pat++ = 'd';
	}

	*pat = '\0';

	for (rd = Rdtab; rd < Rdtab + RDTAB_SIZE; ++rd) {
		if (strcmp(patbuf, rd->rd_pat) == 0) {
			static char buf[10];
			size_t len;

			len = s - *p_s;
			memcpy(buf, *p_s, len);
			buf[len] = '\0';

			if (strcmp(patbuf, ">&") == 0) {
				*p_redir_str = ">";
				*p_postarg_str = " 2>&1";
			}
			else {
				*p_redir_str = buf;
				*p_postarg_str = rd->rd_takes_arg ? "" : NULL;
			}
			
			*p_s = s;
			return 0;
		}
	}

	errf("Unrecognised redirection syntax `%.*s'", s - *p_s, *p_s);
	return -1;
}

/*  Parse a redirection (but not the filename if any being redirected to).
 *  We recognise the sh forms ">", "<", ">>", "dig>" and "dig>&dig"
 *  We also recognise the csh form ">&".
 *  We complain about "<<" as we aren't in the business of parsing scripts.
 */
redirtype_t
arg_get_redir(p_s, p_fdaddr, p_lrdl)
const char **p_s;
int **p_fdaddr;
long *p_lrdl;
{
	redirtype_t redirtype;
	int rfd, outfd, redirch;
	const char *s, *save_s;
	rdlist_t *rdlhead, *rdl, *last;

	*p_fdaddr = NULL;
	
	last = NULL;
	rdlhead = *(rdlist_t **)p_lrdl;
	for (rdl = rdlhead; rdl != NULL; rdl = rdl->rdl_next)
		last = rdl;
	
	s = save_s = *p_s;
	for (; *s != '\0' && isdigit(*s); s++)
		;
	
	redirch = *s++;
	
	if (redirch != '>' && redirch != '<')
		return RD_NOT_REDIR;

	if (isdigit(*save_s)) {
		if (getfd(&save_s, &rfd) != 0)
			return RD_ERROR;
	}
	else {
		rfd = (redirch == '<') ? 0 : 1;
	}

	rdl = last = make_rdl(RDA_DUP_AND_CLOSE, -1, rfd, &rdlhead, last);
	
	if (redirch == '>') {
		redirtype = RD_CREATE;
		
		if (*s == '>') {
			redirtype = RD_APPEND;
			s++;
		}
		
		if (*s == '&' && s[1] != '-' && !isdigit(s[1])) {
			last = make_rdl(RDA_DUP, rfd, 2, &rdlhead, last);
			s++;
		}
	}
	else  {
		if (*s == '<') {
			errf("Can't handle << redirection");
			return RD_ERROR;
		}
		
		redirtype = RD_READ;
	}

	if (*s != '&') {
		/*  Need a following filename
		 */
		*p_fdaddr = &rdl->rdl_fd1;
	}
	else {
		/*  No filename needed.
		 */
		*p_fdaddr = NULL;
		
		/*  dig>&dig or dig>&-
		 */
		if (redirtype == RD_APPEND) {
			errf("illegal redirection >>&");
			return RD_ERROR;
		}
		
		s++;
		if (*s == '-') {
			s++;
			rdl->rdl_fd1 = rfd;
			rdl->rdl_action = RDA_CLOSE;
		}
		else {
			if (!isdigit(*s)) {
				errf("Missing digit after %c&", redirch);
				return RD_ERROR;
			}
			
			if (getfd(&s, &outfd) == -1)
				return RD_ERROR;
			
			rdl->rdl_action = RDA_DUP;
			rdl->rdl_fd1 = outfd;
		}
	}
	
	*p_lrdl = (long) rdlhead;
	*p_s = s;
	return redirtype;
}

void
arg_tidy_redirs_in_parent(lrdl)
long lrdl;
{
	rdlist_t *rdl, *next;

	for (rdl = (rdlist_t *)lrdl; rdl != NULL; rdl = next) {
		next = rdl->rdl_next;
		if (rdl->rdl_action == RDA_DUP_AND_CLOSE)
			rda_close(rdl->rdl_fd1);
		free((char *)rdl);
	}
}

void
arg_do_redirs_in_child(lrdl)
long lrdl;
{
	int i;
	fd_set dont_close;
	rdlist_t *rdl;

	memcpy(&dont_close, &saved_fds, sizeof(fd_set));

	FD_SET(0, &dont_close);
	FD_SET(1, &dont_close);
	FD_SET(2, &dont_close);

	for (rdl = (rdlist_t *)lrdl; rdl != NULL; rdl = rdl->rdl_next) {
		switch(rdl->rdl_action) {
		case RDA_CLOSE:
			rda_close(rdl->rdl_fd1);
			FD_CLR(rdl->rdl_fd1, &dont_close);
			break;
		case RDA_DUP:
			if (rdl->rdl_fd1 != rdl->rdl_fd2)
				rda_dup2(rdl->rdl_fd1, rdl->rdl_fd2);
			FD_SET(rdl->rdl_fd2, &dont_close);
			break;
		case RDA_DUP_AND_CLOSE:
			if (rdl->rdl_fd1 != rdl->rdl_fd2) {
				rda_dup2(rdl->rdl_fd1, rdl->rdl_fd2);
				rda_close(rdl->rdl_fd1);
			}
			FD_SET(rdl->rdl_fd2, &dont_close);
			break;
		default:
			panic("bad action %d in dric");
		}
		FD_SET(rdl->rdl_fd1, &dont_close);
	}
	for (i = dtablesize() - 1; i >= 0; --i)
		if (!FD_ISSET(i, &dont_close))
			rda_close(i);
}

int
arg_open_redir_file(name, redirtype)
const char *name;
redirtype_t redirtype;
{
	int fd;
	const char *mesg;

	mesg = NULL;
	switch(redirtype) {
	case RD_READ:
		if ((fd = open(name, O_RDONLY)) == -1)
			mesg = "Can't open %s for reading (%m)";
		break;
		
	case RD_APPEND:
		if ((fd = open(name, O_WRONLY)) == -1) {
			if (errno != ENOENT)
				mesg = "can't open %s for writing (%m)";
			else if ((fd = creat(name, 0666)) == -1)
				mesg = "Can't create %s (%m)";
		}
		if (fd != -1 && lseek(fd, 0L, SEEK_END) == -1) {
			int save_errno;

			mesg = "Can't lseek to end of %s (%m)";
			save_errno = errno;
			(void) close(fd);
			errno = save_errno;
		}
		break;
		
	case RD_CREATE:
		if ((fd = creat(name, 0666)) == -1)
			mesg = "Can't create %s (%m)";
		break;
		
	default:
		panic("unknown redirection type in ord");
		fd = -1; /* to satisfy gcc */
	}
	
	if (mesg != NULL)
		errf(mesg, name);
	
	return fd;
}

void
arg_save_open_fds()
{
	int i;

	FD_ZERO(&saved_fds);
   
	for (i = dtablesize() - 1; i > 2; --i)
		if (i < FD_SETSIZE && fcntl(i, F_GETFL, 0) >= 0)
			FD_SET(i, &saved_fds);

	return;
}
