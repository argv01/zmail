/* child.c	Copyright 1991 Z-Code Software Corp. */

/* see description and compiling notes in child.h */

#include "config.h"

#if defined(DARWIN)
#include <stdlib.h>
#endif

/* Bart: Tue Jan  5 11:47:14 PST 1993 -- SCO 3.2 doesn't need, 3.1 does? */
#if defined(INTERACTIVE_UNIX) /* || defined(M_UNIX) */
#define _POSIX_SOURCE		/* Bart: Wed Jul 22 11:25:51 PDT 1992 */
#undef HAVE_WAITPID    /* Don: Fri Jul 24 18:58:06 PDT 1992; waitpid is broken */
#endif /* INTERACTIVE_UNIX || M_UNIX */

#include "zmail.h"
#include "child.h"
#include "catalog.h"
#include "zctime.h"
#include "linklist.h"

#ifdef ZM_CHILD_MANAGER

#ifdef HAVE_RESOURCE_H
#include <sys/resource.h>
#endif /* HAVE_RESOURCE_H */

int zmChildVerbose = 0;

/*
 * Macros for blocking and unblocking SIGCHLD within this file.  sigchld_mask
 * is initialized in zmChildInitialize.
 */
#ifndef HAVE_SIGPROCMASK
# ifdef HAVE_SIGBLOCK
#  define block_sigchld()	sigblock(sigmask(SIGCHLD))
#  define unblock_sigchld()	sigsetmask(sigsetmask(~0) & ~sigmask(SIGCHLD))
# else /* HAVE_SIGBLOCK */
#  define block_sigchld()	(sighold(SIGCHLD))
#  define unblock_sigchld()	(sigrelse(SIGCHLD))
# endif /* HAVE_SIGBLOCK */
#else /* sigprocmask exists; it's great; use it */
sigset_t sigchld_mask;
# define block_sigchld()	_block_sigchld(__LINE__)
# define unblock_sigchld()	_unblock_sigchld(__LINE__)

static int sigchld_blocked;

static void
_block_sigchld(__line__)
    int __line__;
{
    sigchld_blocked = 1;
    if (zmChildVerbose >= 2)
	print(catgets( catalog, CAT_CHILD, 1, "blocking, line %d\n" ), __line__);
    sigprocmask(SIG_BLOCK, &sigchld_mask, (sigset_t *)0);
}

static void
_unblock_sigchld(__line__)
    int __line__;
{
    sigchld_blocked = 0;
    if (zmChildVerbose >= 2)
	print(catgets( catalog, CAT_CHILD, 2, "unblocking, line %d\n" ), __line__);
    sigprocmask(SIG_UNBLOCK, &sigchld_mask, (sigset_t *)0);
}
#endif /* HAVE_SIGPROCMASK */

static void sigchld_handler();
static int sigchld_handler_longjmp;
static jmp_buf foil_race_condition;

/*
 * We keep track of total number of SIGCHLDs ever caught
 */
static int sigchld_count = 0;

/*
 * Data structure for child list.
 */
struct child {
    struct link link;
    int pid;
    unsigned int is_zombie:1, is_blocked:1, is_reaped:1;
    WAITSTATUS waitstatus;		/* only set if it's a zombie */
    void (*exited_callback)();
    void *exited_data;
    void (*stopped_callback)();
    void *stopped_data;
};

static struct child *children = (struct child *)NULL;

/*
 * Macros and functions for dealing with these child lists
 */
#define enqueue_child(child, list) \
			insert_link((list), &(child)->link)
#define extract_child(child, list) \
			remove_link((list), &(child)->link)
#define first_child(list) (list)
#define last_child(list) ((list) ? link_prev(struct child, link,list) : (list))
#define next_child(it, list) (!(it) ? first_child(list)		\
	 : (it) == last_child(list) ? (struct child *)NULL	\
				    : link_next(struct child, link, it))
#define prev_child(it, list) (!(it) ? last_child(list)		\
	: (it) == first_child(list) ? (struct child *)NULL	\
				    : link_prev(struct child, link, it))

static struct child *
find_child_in_list(pid, list)
    int pid;
    struct child *list;
{
    struct child *it;
    for (it = first_child(list); it; it = next_child(it, list))
	if (it->pid == pid || pid == -1)
	    break;
    return it;
}

static struct child *
new_child()
{
    struct child *child = (struct child *)malloc(sizeof(struct child));
    if (!child)
	error(SysErrFatal, catgets( catalog, CAT_CHILD, 3, "new_child: malloc(%d) failed" ),
				    sizeof(struct child));
    return child;
}

static void
free_child(child)
struct child *child;
{
    (void)free((char *)child);
}

static void
discard_child(child)
struct child *child;
{
    if (!child || !child->is_reaped)
	return;

    extract_child(child, &children);
    free_child(child);
}

static char discard_ready;

static void
discard_reaped_children()
{
    struct child *child, *tmp;

    if (!(child = children) || !discard_ready)
	return;

    block_sigchld();

    do {
	tmp = next_child(child, children);
	if (tmp == child)
	    tmp = 0;
	if (child->is_reaped)
	    discard_child(child);
    } while (children && (child = tmp) && tmp != children);

    discard_ready = 0;

    unblock_sigchld();
}

static void
defer_discard_child(child)
struct child *child;
{
    child->is_reaped = 1;

    if (discard_ready)
	return;

    func_deferred(discard_reaped_children, NULL);
    discard_ready = 1;
}

void
zmChildReset(initialized)
int initialized;
{
    RETSIGTYPE (*oldchld)() = signal(SIGCHLD, sigchld_handler);

    if (oldchld == SIG_ERR) {
	error(SysErrFatal, catgets( catalog, CAT_CHILD, 4, "zmChildInitialize: signal() failed: %s" ),
							strerror(errno));
    }
    if (initialized && oldchld != sigchld_handler)
	error(ForcedMessage, catgets(catalog, CAT_CHILD, 36, "Signal handlers unexpectedly reset!"));
}

/*
 * static initialization routine.
 */
void
zmChildInitialize()
{
    static int initialized = 0;

    if (!initialized) {
	zmChildReset(0);
#ifdef HAVE_SIGPROCMASK
	sigemptyset(&sigchld_mask);
	sigaddset(&sigchld_mask, SIGCHLD);
#endif /* HAVE_SIGPROCMASK */
	initialized = 1;
    }
}

static int
_zm_child_process_fork(pid, forkname)
int pid;
char *forkname;
{
    struct child *child;

    /*
     * block_sigchld() has been done by the calling function
     */

    switch(pid) {
	case -1:	/* (v)fork failed */
	    if (zmChildVerbose)
		print(catgets( catalog, CAT_CHILD, 5, "(_zm_child_process_fork: %s failed)\n" ), forkname);
	    break;
	case 0:		/* this is the child */
	    if (zmChildVerbose)
		print(catgets( catalog, CAT_CHILD, 6, "(I am %d)\n" ), getpid());
	    break;
	default:	/* this is the parent */
	    child = new_child();
	    child->pid = pid;
	    child->is_zombie = 0;
	    child->is_blocked = 0;
	    child->is_reaped = 0;
	    child->exited_callback = (void (*)())0;
	    child->exited_data = (void *)0;
	    child->stopped_callback = (void (*)())0;
	    child->stopped_data = (void *)0;
	    enqueue_child(child, &children);
	    if (zmChildVerbose)
		print(catgets( catalog, CAT_CHILD, 7, "(child %d forked)\n" ), child->pid);
	    break;
    }
    unblock_sigchld();
    return pid;
}

int
_zmChildPreVFork()
{
    zmChildInitialize();
    fflush(stdout);
    fflush(stderr); 
    block_sigchld();
    return (0);			/* not used */
}

int
_zmChildProcessVFork(pid)
    int pid;
{
    return _zm_child_process_fork(pid, "vfork");
}

int
zmChildFork()
{
    _zmChildPreVFork();
    return _zm_child_process_fork(fork(), "fork");
}

#if defined(SYSV_SETPGRP) || defined (FREEBSD4) || defined(DARWIN)
	/* We can't learn the process group of other processes, so we   XXX
	 * must wait for specific processes in other groups.  Problem?  XXX
	 */
#define pid_matches(pid, pids)						\
	    ((pids) == -1 || ((pids) > 0 && (pid) == (pids)))
#else /* SYSV_SETPGRP */
#define pid_matches(pid, pids)						\
	    ((pids) == -1 || (pids) > 0 && (pid) == (pids)		\
			 || (pids) < -1 && getpgrp(pid) == -(pids)	\
			 || (pids) == 0 && getpgrp(pid) == getpgrp(0))
#endif /* SYSV_SETPGRP */

extern int
zmChildWaitPid(pid, statusp, options)
int pid;
WAITSTATUS *statusp;
int options;
{
    struct child *child = (struct child *)NULL;
    int found_potential_match, first_time_around, prev_sigchld_count;

    /*
     * pid == 0 means anything in the same process group as the parent.
     * pid < 0 means anything whose process group is -pid.
     * So we don't have to call getpgrp(getpid()) a zillion times...
     */
    if (pid == 0) {
#if defined( SYSV_SETPGRP ) || defined(FREEBSD4) || defined(DARWIN)
	error(ZmErrWarning, catgets( catalog, CAT_CHILD, 8, "Cannot wait for children by process group" ));
	pid = -getpgrp();
#else /* SYSV_SETPGRP */
	pid = -getpgrp(getpid());
#endif /* SYSV_SETPGRP */
    }

    /*
     * Block SIGCHLD, so the list doesn't change while we are looking at it
     * or altering it.
     */
    block_sigchld();

    prev_sigchld_count = sigchld_count;
    first_time_around = 1;
    while (1) {
	/*
	 * Yes, we really need to recheck the whole list after each pause.
	 * The reason is that a callback might get called which removes
	 * the last possible candidate from the list, in which case
	 * we should return -1.
	 */
	found_potential_match = 0;
	for (child = first_child(children); child != (struct child *)NULL;
					child = next_child(child, children)) {
	    if (pid_matches(child->pid, pid)) {

#ifdef NOT_YET
	XXX
	This introduces problems of its own, namely,
	on some systems (sgi) when a child becomes a zombie,
	it becomes undetectable by kill -0.
	The proper thing to do is probably some combination
	of unblocking and longjmping, but now
	I am worrying about zmChildWaitPid not being reentrant.
	This will be dealt with in a future release.

		/*
		 * New test to make sure we never pause
		 * for a non-existent child. (This can happen
		 * if a child gets accidentally reaped and
		 * thrown away, as it does in some
		 * bogus implementations of waitpid).
		 * If this error ever happens, we need to
		 * find out why.
		 */
		if (kill(child->pid, 0) == -1 && errno == ESRCH
					      && !child->is_zombie) {
		    error(SysErrWarning,
	            catgets( catalog, CAT_CHILD, 9, "zmChildWaitPid: cannot get exit status of child process %d" ),
								    child->pid);

		    defer_discard_child(child);
		} else
#endif /* NOT_YET */

		    found_potential_match = 1;
		if (child->is_zombie &&
			((options&WUNTRACED) || !WIFSTOPPED(child->waitstatus)))
		    break;
	    }
	}
	if (!found_potential_match) {
	    if (first_time_around)
		errno = ECHILD;
	    else {
		error(ZmErrWarning,
		     catgets( catalog, CAT_CHILD, 10, "zmChildWaitPid: waited-for child %d disappeared!\n" ), pid);
		errno = EINTR;
	    }
	    unblock_sigchld();
	    return -1;
	}
	if (child)
	    break;	/* found it! */

	/*
	 * No zombie matches the desired conditions, but some child
	 * has potential.
	 * Pause until another child dies or is added
	 * to the end of the list (unless WNOHANG is specified,
	 * in which case we return 0 immediately).
	 */
	if (options & WNOHANG) {
	    unblock_sigchld();
	    return 0;
	}

	if (setjmp(foil_race_condition) == 0) {
	    sigchld_handler_longjmp = 1;
	    unblock_sigchld();	/* the handler might get called immediately...*/
	    if (sigchld_count == prev_sigchld_count)
		pause();
	}
	sigchld_handler_longjmp = 0;
	block_sigchld();
	if (sigchld_count == prev_sigchld_count) {
	    /*
	     * Nothing was added to the list, so what interrupted
	     * the pause was a signal other than SIGCHLD.
	     */
	    if (zmChildVerbose)
		print(catgets( catalog, CAT_CHILD, 11, "zmChildWaitPid interrupted: %s\n" ), strerror(errno));
	    unblock_sigchld();
	    return -1;
	}
	prev_sigchld_count = sigchld_count;

	first_time_around = 0;
    }

    /*
     * Retrieve the information
     */
    pid = child->pid;
    if (statusp)
	*statusp = child->waitstatus;

    /*
     * If the wait showed that the child stopped, then the
     * child still exists and is no longer a zombie.
     * Otherwise the child is gone, so delete it from the list forever.
     */
    if (WIFSTOPPED(child->waitstatus)) {
	child->is_zombie = 0;
	if (zmChildVerbose)
	    print(catgets( catalog, CAT_CHILD, 12, "(child %d reaped, still a zombie)\n" ), child->pid);
    } else {
	if (zmChildVerbose)
	    print(catgets( catalog, CAT_CHILD, 13, "(child %d reaped, status %d)\n" ),
				    child->pid, WEXITSTATUS(child->waitstatus));
	defer_discard_child(child);
    }

    unblock_sigchld();
    return pid;
}

extern int
zmChildWait(statusp)
WAITSTATUS *statusp;
{
    return zmChildWaitPid(-1, statusp, WUNTRACED);
}


static void
call_callback(child, flags)
struct child *child;
int flags;
{
    if (!child->is_zombie)
	return;
    if (child->is_blocked)
	return;

    if (WIFSTOPPED(child->waitstatus)) {
	if ((flags & ZM_CHILD_STOPPED) && child->stopped_callback) {
	    if (zmChildVerbose)
		print(catgets( catalog, CAT_CHILD, 14, "Calling stopped_callback %#x for child %d\n" ),
					child->stopped_callback, child->pid);
	    child->stopped_callback(child->pid, &child->waitstatus,
				    child->stopped_data);
	    if (zmChildVerbose)
		print(catgets( catalog, CAT_CHILD, 15, "Returned from stopped_callback %#x for child %d\n" ),
					child->stopped_callback, child->pid);
	}
    } else {
	if ((flags & ZM_CHILD_EXITED) && child->exited_callback) {
	    if (zmChildVerbose)
		print(catgets( catalog, CAT_CHILD, 16, "Calling exited_callback %#x for child %d\n" ),
					    child->exited_callback, child->pid);
	    child->exited_callback(child->pid, &child->waitstatus,
				    child->exited_data);
	    if (zmChildVerbose)
		print(catgets( catalog, CAT_CHILD, 17, "Returned from exited_callback %#x for child %d\n" ),
					    child->exited_callback, child->pid);
	}
    }
}

static void
sigchld_handler(sig)
int sig;
{
    char *wait_name;
    WAITSTATUS status;
    int pid, do_the_longjmp = sigchld_handler_longjmp;	/* See note below */
    struct child *it = 0, *it2;

    if (zmChildVerbose) {
	if (sig == SIGCHLD)
	    print(catgets( catalog, CAT_CHILD, 18, "sigchld_handler: caught SIGCHLD\n" ));
	else
	    error(ZmErrWarning, catgets( catalog, CAT_CHILD, 19, "sigchld_handler caught %d" ), sig);
    }

#if defined(SYSV) || !defined(HAVE_RESTARTABLE_SIGNALS)
    block_sigchld();
#endif /* SYSV || !HAVE_RESTARTABLE_SIGNALS */

    /*
     * Keep track of total number of SIGCHLDs that have ever been handled
     */
    sigchld_count++;

#if defined(HAVE_WAITPID) && 0
    /* more than one system has had bogus waitpids, so NEVER use it */
    wait_name = "waitpid";
    pid = waitpid(-1, &status, WNOHANG | WUNTRACED);
#else /* !HAVE_WAITPID */
#ifdef HAVE_WAIT3
    wait_name = "wait3";
    pid = wait3(&status, WNOHANG | WUNTRACED, (struct rusage *)NULL);
#else /* !HAVE_WAIT3 */
#ifdef HAVE_WAIT2
    wait_name = "wait2";
    pid = wait2(&status, WNOHANG | WUNTRACED);
#else /* !HAVE_WAIT2 */
    wait_name = "wait";
    pid = wait(&status);
#endif /* HAVE_WAIT2 */
#endif /* HAVE_WAIT3 */
#endif /* HAVE_WAITPID */
    /*
     * The manual entry says that waitpid returns 0 if WNOHANG is
     * specified and there are no stopped or exited children (but
     * there is a running child).
     * This should never happen.
     *
     * Well, I THOUGHT it should never happen, but it does happen in the
     * following situation, and it's scary, because it means if
     * only wait() exists, wait will hang, and we NEVER want to
     * hang inside a signal handler.  I only pray that the following
     * cannot happen on such a system:
     *	1) Mail somebody, say ~v to get into the editor,
     *     hit ^Z to stop it.  Both zmail and the editor are stopped.
     *	   when they are resumed, zmail gets a SIGCHILD, but
     *     since the child has come back to life, the wait returns 0.
     */
    if (pid == 0) {
	/*
	 * This is a frequent occurrence, so only print it if verbose is set.
	 */
	if (zmChildVerbose) {
	    print(catgets( catalog, CAT_CHILD, 20, "WARNING: sigchld_handler: got SIGCHLD but %s returned 0--\n" ),
								    wait_name);
	    print(catgets( catalog, CAT_CHILD, 21, "\tIf it weren't for WNOHANG, you would have been HUNG!!\n" ));
	}
	goto ChildHandlerDone;
    }

    if (pid == -1) {
	/*
	 * This would be nice to know, if one of us is running zmail,
	 * but it sometimes happens, so only print if verbose is set.
	 */
	if (zmChildVerbose)
	    error(SysErrWarning,
		catgets( catalog, CAT_CHILD, 22, "sigchld_handler: got SIGCHLD but %s failed" ), wait_name);
	goto ChildHandlerDone;
    }

    if (zmChildVerbose)
	print(WIFSTOPPED(status) ? catgets( catalog, CAT_CHILD, 24, "(child %d stopped, status %d, now a zombie)\n" )
	                         : catgets( catalog, CAT_CHILD, 25, "(child %d exited, status %d, now a zombie)\n" ),
	      pid, WEXITSTATUS(status));

    /*
     * The wait succeeded.
     * Move the child to the zombie list.
     * The child might already be on the zombie list (for example,
     * if the process was stopped and then continued and then stopped
     * again).
     * FIX THIS someday-- this is probably not the correct procedure,
     * but if the child is already on the zombie list, we remove it and
     * put it back with the new info.  (The correct thing to do is probably
     * to have multiple occurrances of it on the zombie list).
     */
    it = find_child_in_list(pid, children);
    if (!it) {
#if defined( IMAP )
      if ( !using_imap && !imap_initialized )
#endif
      error(ZmErrWarning, WIFSTOPPED(status) ?
	      catgets( catalog, CAT_CHILD, 27, "sigchld_handler: child %d stopped, and I didn't even know I had it!\n" )
	    : catgets( catalog, CAT_CHILD, 28, "sigchld_handler: child exited, and I didn't even know I had it!\n" ), pid);
	goto ChildHandlerDone;
    }

    it->waitstatus = status;
    it->is_zombie = 1;

    /*
     * Move the child to the end of the list, so that zombie
     * children will occur in the list in the order in which the
     * SIGCHLDs occurred
     */
    extract_child(it, &children);
    enqueue_child(it, &children);

    /*
     * See if there are any actual zombies; if so, send ourselves another
     * SIGCHLD.
     * (We can't depend on automatically getting a SIGCHLD for each child's
     * death; for example, if two children die during a single
     * execution of sigchld_handler, they will only generate one SIGCHLD).
     *
     * XXX FIX THIS someday -- The "kill -0" test doesn't work for stopped
     *	   processes.
     *     I don't know how to tell whether a child has stopped or not
     *     if we don't have a version of wait with a WNOHANG option.
     *     Probably UNIXes with job control always have such a wait.
     */
    for (it2 = first_child(children); it2; it2 = next_child(it2, children))
	if (!it2->is_zombie && kill(it2->pid, 0) == -1) {
	    if (zmChildVerbose)
		print(catgets( catalog, CAT_CHILD, 29, "(someone else needs reaping; sending myself SIGCHLD)\n" ));
	    kill(getpid(), SIGCHLD);
	    break;
	}

ChildHandlerDone:
#ifdef HANDLERS_NEED_RESETTING
    /*
     * NOTE:  On the toshiba, the following line generates
     * a SIGCHLD immediately if there is a child waiting to be reaped.
     * This is great, and makes the previous test unnecessary,
     * but I don't know if we can assume this behavior on all
     * SYSV systems.
     */
    (void) signal(SIGCHLD, sigchld_handler);
#endif /* HANDLERS_NEED_RESETTING */

    /*
     * Calling the callback should be the last thing this function
     * does, in case the callback does a longjmp.  Don't unblock
     * SIGCHLD yet, since a longjmp will do this automatically.
     */
    if (it) call_callback(it, ZM_CHILD_EXITED|ZM_CHILD_STOPPED);

    sigchld_handler_longjmp = 0;	/* See note below */

#if defined(SYSV) || !defined(HAVE_RESTARTABLE_SIGNALS)
    unblock_sigchld();
#endif /* SYSV || !HAVE_RESTARTABLE_SIGNALS */

    /*
     * This is really nasty.  There can be a race condition with the pause()
     * in zmChildWaitPid() such that we make the test for a signal and decide
     * it's safe to pause, but then the signal comes in before the pause()
     * gets started, so we end up waiting forever for another signal.  So,
     * if sigchld_handler_longjmp is set, we jump around the pause() call to
     * prevent the hang.  Yeeesh.
     */
    if (do_the_longjmp)
	longjmp(foil_race_condition, 1);
}

extern void
zmChildSetCallback(pid, flags, fun, data)
int pid;
int flags;
void (*fun)();
void *data;
{
    struct child *child;

    block_sigchld();

    if (!(child = find_child_in_list(pid, children))) {
	error(ZmErrWarning,
	    catgets( catalog, CAT_CHILD, 30, "zmChildSetCallback(%d,...): no such process zmChildFork()ed" ), pid);
	unblock_sigchld();
	return;
    }

    if (flags & ZM_CHILD_EXITED) {
	child->exited_callback = fun;
	child->exited_data = data;
    }
    if (flags & ZM_CHILD_STOPPED) {
	child->stopped_callback = fun;
	child->stopped_data = data;
    }

    /*
     * If the process is already a zombie, call the callback immediately.
     * It's okay for the callback to do a longjmp while still blocking
     * SIGCHLD, since longjmp automatically resets the signal mask.
     * It's NOT okay for the callback to just sit and not return;
     * then SIGCHLD will stay blocked.
     */
    call_callback(child, flags);

    unblock_sigchld();
}

static int
_zm_child_set_block(pid, val)
    int pid, val;
{
    struct child *it;
    int was_blocked = 0;

    block_sigchld();

    for (it = first_child(children); it; it = next_child(it, children)) {
	if (pid_matches(it->pid, pid)) {
	    was_blocked |= it->is_blocked;
	    it->is_blocked = val;
	    if (!val)
		/*
		 * FIX THIS--
		 * if the callback does a longjmp,
		 * we will never get back here to finish.
		 * This is only a problem if pid specifies more than one
		 * process.  I don't know how to deal with this;
		 * therefore the warning message in zmChildUnBlock.
		 */
		call_callback(it, ZM_CHILD_EXITED|ZM_CHILD_STOPPED);
	    if (pid > 0)
		break;	/* there can be only one match */
	}
    }

    unblock_sigchld();

    return was_blocked;
}

extern int
zmChildBlock(pid)
    int pid;
{
    return _zm_child_set_block(pid, 1);
}

extern int
zmChildUnblock(pid)
    int pid;
{
    if (pid <= 0)
	error(ZmErrWarning,
	  catgets( catalog, CAT_CHILD, 31, "zmChildUnBlock(%d): unblocking more than one child is unpredictable" ),
									pid);
    return _zm_child_set_block(pid, 0);
}

#endif /* ZM_CHILD_MANAGER */
