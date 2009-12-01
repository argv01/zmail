/* execute.c     Copyright 1990, 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "child.h"
#include "catalog.h"
#include "execute.h"

#ifdef HAVE_RESOURCE_H
#include <sys/resource.h>
#endif /* HAVE_RESOURCE_H */

#if defined( MSDOS )  /* **RJL 11.06.92 - needed for _P_WAIT */
#include <process.h>
#endif  /* MSDOS */

#ifndef ZM_CHILD_MANAGER
static jmp_buf execjbuf;
WAITSTATUS status_from_catcher;

#if defined(HAVE_WAITPID) && defined(GUI)

/* Replace the wait() system call with one that handles our
 * asynchronous composition windows (e.g. external editors).
 */

wait(stword)
int *stword;
{
    int pid = waitpid(-1, stword, 0);

    if (pid > 0 && istool > 1)
        gui_restore_compose(pid);

    return pid;
}

#endif /* HAVE_WAITPID && GUI */
#endif /* ZM_CHILD_MANAGER */

#ifndef MAC_OS

/* Global variables -- Sky Schulz, 1991.09.05 01:46 */
int exec_pid;

/* If argv is DUBL_NULL, we are continuing a suspended editor. */
int
execute(argv)
    char * const *argv;
{
#ifndef WIN16
    WAITSTATUS status;
    int pid;
#endif
#ifdef SIGCONT
    RETSIGTYPE (*oldstop)(), (*oldcont)();
#endif /* SIGCONT */
#ifdef WIN16
    RETSIGTYPE (*oldint)(int), (*oldquit)(int);
#else    
    RETSIGTYPE (*oldint)(), (*oldquit)();
#endif    

    oldint = signal(SIGINT, SIG_IGN);
    oldquit = signal(SIGQUIT, SIG_IGN);

#ifdef SIGCONT
#ifdef _SC_JOB_CONTROL
    if (sysconf(_SC_JOB_CONTROL) >= 0)
#endif /* _SC_JOB_CONTROL */
    {
    oldstop = signal(SIGTSTP, SIG_DFL);
    oldcont = signal(SIGCONT, SIG_DFL);
    }
#endif /* SIGCONT */
    turnon(glob_flags, IGN_SIGS);

#ifndef VUI
    echo_on();
#endif /* VUI */

#ifndef MSDOS
#ifndef ZM_CHILD_MANAGER
    if (!setjmp(execjbuf)) {
#endif /* ZM_CHILD_MANAGER */

#ifdef SIGCONT
	if (!argv)
	    (void) kill(exec_pid, SIGCONT);
	else
#endif /* SIGCONT */
	if ((exec_pid = zmChildVFork()) == 0) {
	    (void) signal(SIGINT, SIG_DFL);
	    (void) signal(SIGQUIT, SIG_DFL);
	    (void) signal(SIGPIPE, SIG_DFL);
	    (void) closefileds_above(2);

#if defined(GUI) && !defined(VUI)
	    /* Some programs (notably Unipress Emacs) kill their pgrp
	     * with a SIGHUP on exit.  This may kill Z-Mail.  In line
	     * mode, we can't change the pgrp of the child unless we
	     * also change the pgrp of the terminal, because the child
	     * gets a SIGTTOU.  However, we can protect ourself in GUI
	     * mode, where no tty exists unless created by the child.
	     */
	    if (istool > 1)
#ifdef SYSV_SETPGRP
		if (setpgrp() == -1)
#else /* SYSV_SETPGRP */
		if (setpgrp(0, getpid()) == -1)
#endif /* SYSV_SETPGRP */
		    error(SysErrWarning, "setpgrp");
#endif /* GUI && !VUI */

#ifdef apollo
	    setgid(getgid());
#endif /* apollo */
	    execvp(*argv, argv);
#else /* MSDOS */
#ifndef WIN16
	if (spawnvp(_P_WAIT, *argv, argv) != 0)
#endif /* WIN16 */
#endif /* !MSDOS */
	    if (errno == ENOENT) {
		print(catgets( catalog, CAT_SHELL, 97, "%s: command not found." ), *argv);
		print("\n");
	    } else
		error(SysErrWarning, *argv);
#ifndef MSDOS
	    _exit(-1);
	}
#ifndef ZM_CHILD_MANAGER
	/* Parent's got to do something; sigchldcatcher may also be waiting.
	 * This loop will usually get broken by the longjmp() (except tool),
	 * but in certain circumstances sigchldcatcher isn't yet active.
	 */
	while ((pid = wait(&status)) != -1) {
	/*
	 * Note from Don:  My impression is that the following
	 * never ever ever got executed, since the signal handler
	 * always reaped the child and (if it caught exec_pid) did a longjmp.
	 */
	    Debug("The exec loop caught a signal? (pid = %d)\n", pid);
	    if (pid == exec_pid) {
		status_from_catcher = status;
		exec_pid = 0;
		break;
	    }
	}
#else /* ZM_CHILD_MANAGER */
	while ((pid = zmChildWaitPid(exec_pid, &status, 0)) == -1 &&
		errno == EINTR)
	    ;
	if (pid == -1)
	    error(SysErrWarning, "zmChildWaitPid(%d) failed", exec_pid);
	exec_pid = 0;
#endif /* ZM_CHILD_MANAGER */

#ifndef ZM_CHILD_MANAGER
    }
#endif /* ZM_CHILD_MANAGER */
#endif /* !MSDOS */

/* reset our ttymodes */
#ifndef VUI
    echo_off();
#endif /* VUI */
#ifndef WIN16
    (void) signal(SIGINT, oldint);
    (void) signal(SIGQUIT, oldquit);
#endif /* !WIN16 */
#ifdef SIGCONT
#ifdef _SC_JOB_CONTROL
    if (sysconf(_SC_JOB_CONTROL) >= 0)
#endif /* _SC_JOB_CONTROL */
    {
    (void) signal(SIGTSTP, oldstop);
    (void) signal(SIGCONT, oldcont);
    }
#endif /* SIGCONT */
    turnoff(glob_flags, IGN_SIGS);

#ifdef ZM_CHILD_MANAGER
    return WEXITSTATUS(status);
#else /* !ZM_CHILD_MANAGER */
    return WEXITSTATUS(status_from_catcher);
#endif /* !ZM_CHILD_MANAGER */
}

#endif /* !MAC_OS */

#ifndef ZM_CHILD_MANAGER
RETSIGTYPE
#ifdef _WINDOWS 
/* to avoid compiler warnings, pass the correct type to signal()'s fnctn. */
sigchldcatcher(int unused)
#else 
sigchldcatcher()
#endif
{
    int	   pid;

#ifdef HAVE_WAIT3
    while ((pid = wait3(&status_from_catcher, WNOHANG, (struct rusage *)0)) > 0)
#else
#ifdef HAVE_WAITPID
    while ((pid = waitpid(-1, &status_from_catcher, WNOHANG)) > 0)
#else
#if !defined(SYSV) && !defined(MAC_OS) && !defined(MSDOS) || defined(HAVE_WAIT2)
    /* Old Version 7 -- not supported, but for completeness ... */
    while ((pid = wait2(&status_from_catcher, WNOHANG)) > 0)
#else /* SYSV || MAC_OS || MSDOS */
    while ((pid = wait((int *)0)) > 0)
#endif /* !SYSV && !MSDOS */
#endif /* HAVE_WAITPID */
#endif /* HAVE_WAIT3 */
    {
	Debug("%d died...\n", pid);
/* RJL ** 12.31.92 - added !MSDOS to prevent call to gui_restore_compose */
#if defined( GUI  ) && !defined( MSDOS )
	if (istool > 1)
	    gui_restore_compose(pid);
	else
#endif /* GUI */
	if (pid == exec_pid)
	    break;
    }
#ifdef SYSV
    (void) signal(SIGCHLD, sigchldcatcher);
#endif /* SYSV */
    if (istool)
	return;
    if (pid == exec_pid && pid > 0) {
	exec_pid = 0;
	/* Note from Don:
	 * Goodness, why this?
	 */
	longjmp(execjbuf, 1);
    }
}
#endif /* !ZM_CHILD_MANAGER */
