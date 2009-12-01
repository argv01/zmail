/* popenv.c	Copyright 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "child.h"
#include "fsfix.h"
#include "osconfig.h"
#include "popenv.h"

#ifdef HAVE_STDARG_H
# include <stdarg.h>
#else /* HAVE_STDARG_H */
# ifndef va_dcl
#  include <varargs.h>
# endif
#endif

#include <general.h>

#ifdef ZM_CHILD_MANAGER

/*
 * popenv and friends: open a process for reading, writing, and rithmatic.
 * exported procedures:
 *	popenv, popenl, popenve, popenle, popenvp, popenlp
 *	pclosev
 *	popensh, popencsh, popencsh_f
 */

/*LINTLIBRARY*/
#if 0	/* (the following are needed, but are in zmail.h) */
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif /* 0 */
#include "maxfiles.h"

#ifndef POPENV_MAXARGS
#define POPENV_MAXARGS 1024
#endif

#define DEF_PATH	".:/bin:/usr/bin:/usr/ucb:/etc:/usr/etc"

#if !defined(__HIGHC__) || !defined(MSDOS) || !defined(MAC_OS)
extern char **environ;
#endif /* !HIGHC || !MSDOS || !MAC_OS */
extern FILE *fdopen P((int, const char *));
/* && !MSDOS */
#if !defined(SVR4) \
    && !defined(pyrSVR4) \
    && !defined(AIX) \
    && !defined(MSDOS) \
    && !defined(__hpux)
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
extern int fcntl VP((int, int, ...));
#endif /* HAVE_FCNTL_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
extern int close P((int))
extern int execl VP((const char *, const char *, ...));
extern int fcntl VP((int, int, ...));
extern pid_t fork P((void));
extern int pipe P((int[2]));
#endif /* HAVE_UNISTD_H */
#endif /* SVR4 */

#define parents_end(i)  ((i) == 0)
#define childs_end(i)   ((i) != 0)
#define parents_type(i) ((i) == 0 ? "w" : "r")

int 
popenve(in, out, err, name, argv, envp)
    FILE **in, **out, **err;
    const char *name;
    char **argv;
    char **envp;
{
    FILE **fps[3];
    int pipes[3][2];
    int fds_to_close_on_error[6], n_fds_to_close_on_error = 0;
    int childs_errno = 0;
    register int i, pid;

    fps[0] = in;
    fps[1] = out;
    fps[2] = err;

    /*
     * check executability of the file BEFORE the fork, since on systems
     * without vfork(), it is difficult to communicate from child to parent
     * the fact that the exec failed.
     */
    if (Access(name, 1) != 0)
	return -1;

    for (i = 0; i < 3; ++i)
	if (fps[i] && !*fps[i]) {
	    if (i == 2 && out == err)
		break;	/* Special case -- stdout and stderr the same */
	    if (pipe(pipes[i]) < 0)
		goto error;

	    fds_to_close_on_error[n_fds_to_close_on_error++] = pipes[i][0];
	    fds_to_close_on_error[n_fds_to_close_on_error++] = pipes[i][1];
	}
    if ((pid = zmChildVFork()) == 0) {

	for (i = 0; i < 3; ++i) {
	    if (fps[i]) {
		if (*fps[i]) {	/* redirect from/to existing stream */
		    (void) dup2(fileno(*fps[i]), i);
		    if (i != 1 || out != err)
			(void) close(fileno(*fps[i]));
		} else {	/* set up a pipe */
		    if (i == 2 && out == err) {
			/* Special case -- stdout and stderr the same */
			(void) dup2(1, 2);
		    } else {
			(void) close(pipes[i][parents_end(i)]);
			(void) dup2(pipes[i][childs_end(i)], i);
			(void) close(pipes[i][childs_end(i)]);
		    }
		}
	    }
	}

	/* close every descriptor >= 3 */

	for (i = maxfiles(); i >= 3; --i)
	    (void) close(i);

#ifdef apollo
	setgid(getgid());
#endif /* apollo */
	(void) execve(name, argv, envp);
	childs_errno = errno;
	_exit(1);
    }
    if (pid == -1)
	goto error;

    /*
     * If fork() is used instead of vfork(), the following can never happen,
     * so if the exec fails for some reason, the parent won't know it.
     * 
     * The most common type of error, ENOACCESS, should not happen anyway, since
     * we check for accessibility above.
     */
    if (childs_errno) {
	(void) pclosev(pid);	/* reap the child that couldn't exec */
	errno = childs_errno;
	goto error;
    }
    for (i = 0; i < 3; ++i)
	if (fps[i] && !*fps[i]) {
	    (void) close(pipes[i][childs_end(i)]);
	    *fps[i] = fdopen(pipes[i][parents_end(i)], parents_type(i));
	}
    return pid;

error:
    while (n_fds_to_close_on_error--)
	close(fds_to_close_on_error[n_fds_to_close_on_error]);
    return -1;
}

/* popenle(in, out, err, name, arg0, ... argn, 0, envp) */
int 
#ifdef HAVE_STDARG_H
popenle(FILE **in, FILE **out, FILE **err, const char *name, ...)
#else /* HAVE_STDARG_H */
popenle(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    va_list ap;
    char *args[POPENV_MAXARGS];
    char **envp;
    int argno = 0;
#ifndef HAVE_STDARG_H
    FILE **in, **out, **err;
    char *name;

    va_start(ap);
    in = va_arg(ap, FILE **);
    out = va_arg(ap, FILE **);
    err = va_arg(ap, FILE **);
    name = va_arg(ap, char *);
#else /* HAVE_STDARG_H */
    va_start(ap, name);
#endif /* HAVE_STDARG_H */

    while (args[argno++] = va_arg(ap, char *));
    envp = va_arg(ap, char **);
    va_end(ap);

    return popenve(in, out, err, name, args, envp);
}

int 
popenv(in, out, err, name, argv)
    FILE **in, **out, **err;
    const char *name;
    char **argv;
{
    return popenve(in, out, err, name, argv, environ);
}

/* popenl(in, out, err, name, arg0, ... argn, 0) */
int 
#ifdef HAVE_STDARG_H
popenl(FILE **in, FILE **out, FILE **err, const char *name, ...)
#else /* HAVE_STDARG_H */
popenl(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    va_list ap;
    char *args[POPENV_MAXARGS];
    int argno = 0;
#ifndef HAVE_STDARG_H
    FILE **in, **out, **err;
    char *name;

    va_start(ap);
    in = va_arg(ap, FILE **);
    out = va_arg(ap, FILE **);
    err = va_arg(ap, FILE **);
    name = va_arg(ap, char *);
#else /* HAVE_STDARG_H */
    va_start(ap, name);
#endif /* HAVE_STDARG_H */

    while (args[argno++] = va_arg(ap, char *));
    va_end(ap);

    return popenv(in, out, err, name, args);
}

/*
 * get the path environment (or a default path if none is found) and
 * search thru the path-list until the command is found.  Of course,
 * if the command starts with a '/' don't bother searching.  Return -1
 * if command not found. else, call rwpopenv and returnw whatever that
 * returns.  Considering the case of commands which start with "." or
 * "..", by design, "." should be in your path. If it's not, it shouldn't
 * work anyway.
 *
 * This could have been implemented by having the child call execvp()
 * instead of execve() (or equivalently, have the child execv() each
 * possible pathname) but I think it's better to check for access
 * before the fork-- if vfork() is being used, we don't lose any time
 * (since the parent would sit and wait for the child to do all this stuff
 * anyway) and if vfork() doesn't exist, it avoids the major
 * headache of trying to communicate from the child to the parent
 * the fact that the exec failed.
 */
int 
popenvp(in, out, err, name, argv)
    FILE **in, **out, **err;
    const char *name;
    char **argv;
{
    register char *p, *dir;
    char *getenv(), buf[MAXPATHLEN*2];
    register int found = 0;

    if (!(p = getenv("PATH")))
	p = DEF_PATH;
    if (has_psep(name))
	found = !Access(name, 1);
    else
	for (dir = p; *dir; dir = p) {
#if defined(MSDOS) || defined(MAC_OS)
	    while (*p && *p != ';')
		p++;
#else /* MSDOS || MAC_OS */
	    while (*p && *p != ':')
		p++;
#endif /* MSDOS || MAC_OS */
	    (void) sprintf(buf, "%.*s/%s", p - dir, dir, name);
	    if (found = !Access(buf, 1)) {
		name = buf;
		break;
	    }
	    if (*p)
		p++;
	}
    if (!found)
	return -1;
    /* errno is set to whatever the last call to "access" set it to. */

    return popenv(in, out, err, name, argv);
}

/* popenlp(in, out, err, name, arg0, ... argn, 0) */
int 
#ifdef HAVE_STDARG_H
popenlp(FILE **in, FILE **out, FILE **err, const char *name, ...)
#else /* HAVE_STDARG_H */
popenlp(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    va_list ap;
    char *args[POPENV_MAXARGS];
    int argno = 0;
#ifndef HAVE_STDARG_H
    FILE **in, **out, **err;
    char *name;

    va_start(ap);
    in = va_arg(ap, FILE **);
    out = va_arg(ap, FILE **);
    err = va_arg(ap, FILE **);
    name = va_arg(ap, char *);
#else /* HAVE_STDARG_H */
    va_start(ap, name);
#endif /* HAVE_STDARG_H */

    while (args[argno++] = va_arg(ap, char *));
    va_end(ap);

    return popenvp(in, out, err, name, args);
}

int
popensh(in, out, err, command)
FILE **in, **out, **err;
const char *command;
{
    return popenl(in, out, err, "/bin/sh", "sh", "-c", command, (char *) NULL);
}

int
popencsh(in, out, err, command)
FILE **in, **out, **err;
const char *command;
{
    return popenl(in, out, err, "/bin/csh", "csh", "-c", command, (char *) NULL);
}

int
popencsh_f(in, out, err, command)
FILE **in, **out, **err;
const char *command;
{
    return popenl(in, out, err, "/bin/csh", "csh", "-f", "-c", command,
	    (char *) NULL);
}

/*
 *	In order to wrap up a process begun by rwpopen,
 *	the program must explicitly close all the pipes into/out of it,
 *	and then call pclosev(pid).
 *
 *      return the status of the wait: note that the status value must
 *      right shifted by a byte (>>8) to see the actual exit value from
 *      exec call.
 */
int 
pclosev(pid)
int pid;
{
    int status;
    RETSIGTYPE (*hstat) (), (*istat) (), (*qstat) ();
/* **RJL 12.1.92 - if MSDOS just get out */
#if defined( MSDOS )
	return -1;
#endif /* MSDOS */

    if (pid < 0)
	return -1;

    istat = signal(SIGINT, SIG_IGN);
    qstat = signal(SIGQUIT, SIG_IGN);
    hstat = signal(SIGHUP, SIG_IGN);
    if (zmChildWaitPid(pid, (WAITSTATUS *) & status, 0) == -1)
	status = -1;
    (void) signal(SIGINT, istat);
    (void) signal(SIGQUIT, qstat);
    (void) signal(SIGHUP, hstat);
    return status;
}

#endif	/* ZM_CHILD_MANAGER */
