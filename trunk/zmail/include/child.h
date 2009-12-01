/*
 *  child.h -- header file for child.c.

    Implementation of primitive job control.

    No functions in Z-Mail except the ones in child.c should catch SIGCHLD,
    and the functions on the left should never be used; instead,
    use the equivalent function on the right.
	fork()		zmChildFork()
	vfork()		zmChildVFork()	     (same as zmChildFork if no vfork)
	wait()		zmChildWait()
	waitpid()	zmChildWaitPid()

    In addition, the following functions are implemented:

    zmChildSetCallback(int pid, flags, void (*fun)(), void *data)
	Sets a callback to be called when the child with the given pid
	exits or stops.
	If fun is NULL, unsets the callback.

	flags is formed by or-ing together zero or more of the following:
	    ZM_CHILD_EXITED -- action is to be performed when the child dies.
	    ZM_CHILD_STOPPED -- action is to be performed when the child is
								      suspended.

	fun is the callback function, which is called as follows:
		fun(pid, WAITSTATUS *statusp, data);
	where "data" is the argument that was passed to zmChildSetCallback.
	In most cases the callback function will want to reap the child
	using zmChildWait() or zmChildWaitPid().
	fun can be set to zmChildWaitPid if this is the only action desired.

    (unimplemented)
    int zmChildGetCallback(int pid, int flags, void (**fun)(), void **data)
	Returns the current callback for a given set of children.
	flags must be one (but not both) of ZM_CHILD_EXIT or ZM_CHILD_STOPPED.
	pid can be any of the values specified above for zmChildSetCallback.
	If there is no action for the given process or processes
	(where an action includes throwing it away, i.e. *fun=NULL)
	then the function return value is -1 (in which case *fun and *data are
	not altered), otherwise 0.
	FIX THIS-- or at least get clear on what the desired action is--
		What if the following sequence gets called:
		    zmChildSetCallback(-1, ZM_CHILD_EXITED, bigfun, NULL);
		    zmChildSetCallback(1000, ZM_CHILD_EXITED, littlefun, NULL);
		    zmChildGetCallback(-1, ZM_CHILD_EXITED, &whichfun, NULL);
		What does whichfun get set to?  I think it should get set to
		bigfun, even though this is a little misleading (i.e.
		it leads one to believe that bigfun will be called for all
		children, which is not the case).

    zmChildBlock(pid)
    zmChildUnblock(pid)
	Blocks (i.e. postpones) or unblocks execution of the callback for
	the given child (-1 for all children).
	Returns 1 if the child was already being blocked, 0 otherwise.


    An external int zmChildVerbose is provided.  If it's set,
    the routines in this module will describe what they are doing.
    Also, there is a routine zmChildInitialize which never need
    be called (it is called automatically by zmChild(V)Fork()),
    but can be used to turn on SIGCHLD handling right away for debugging
    if desired.
 *
 */

#ifndef _CHILD_H_
#define _CHILD_H_

#include "osconfig.h"
#include "config.h"
#include "zcsig.h"

#ifdef HAVE_VFORK_H
#ifndef ZC_INCLUDED_VFORK_H
# include <vfork.h>
# define ZC_INCLUDED_VFORK_H
#endif /* ZC_INCLUDED_VFORK_H */
#endif /* HAVE_VFORK_H */

/* This is redundant with zmail.h but included for portability */
#ifdef USE_UNION_WAIT
#define WAITSTATUS union wait
#else
#define WAITSTATUS int
#endif /* !USE_UNION_WAIT */

#ifndef RETSIGTYPE
#define RETSIGTYPE void
#endif /* !RETSIGTYPE */

#ifdef ZM_CHILD_MANAGER

#include "general.h"

#define ZM_CHILD_EXITED		(1<<0)
#define ZM_CHILD_STOPPED	(1<<1)

extern int zmChildFork P((void));
extern int zmChildWait P((WAITSTATUS *));
extern int zmChildWaitPid P((int, WAITSTATUS *, int));
extern void zmChildSetCallback P((int, int, void(*)(), void *));
extern void zmChildGetCallback();	/* unimplemented */
extern int zmChildBlock P((int));
extern int zmChildUnblock P((int));
extern int zmChildVerbose;
/*
 * Can never return in the child from a function that calls vfork(),
 * so we use the following.
 */
extern int _zmChildPreVFork P((void)), _zmChildProcessVFork P((int));

#ifdef HAVE_VFORK
#define zmChildVFork()	(_zmChildPreVFork(), _zmChildProcessVFork(vfork()))
#else /* !HAVE_VFORK */
#define zmChildVFork() 	zmChildFork()
#endif /* !HAVE_VFORK */

#else /* !ZM_CHILD_MANAGER */

#define zmChildFork() fork()

#ifdef HAVE_VFORK
#define zmChildVFork() vfork()
#else /* !HAVE_VFORK */
#define zmChildVFork() fork()
#endif /* !HAVE_VFORK */

#endif /* ZM_CHILD_MANAGER */

/*
 * Notes on compiling child.c:
 *      1) If HAVE_VFORK is not #defined, then fork will be used instead
 *	   of vfork. (Since zmChildVFork is a macro, HAVE_VFORK must be
 *	   #defined prior to #including child.h in every file that uses it).
 *	2) WAITSTATUS should be #defined to be the type of the "status"
 *	   returned by wait().  Sun says it's int, and POSIX says it's int,
 *	   but some systems will probably say it's union wait.
 *	   If it's not #defined prior to #including this file, then
 *	   it defaults to int.
 *	3) RETSIGTYPE should be #defined to be the type returned by signal().
 *	4) If the compiler says waitpid() is undefined, #undef HAVE_WAITPID.
 *	   If the compiler says wait3() is undefined, #undef HAVE_WAIT3.
 *	   If the compiler says wait2() is undefined, #undef HAVE_WAIT2.
 *	   If the compiler says wait() is undefined, there is nothing
 *	   I can do for you.
 */

/* This depends on <signal.h> being included before child.h */
#ifndef SIG_ERR
#define SIG_ERR ((RETSIGTYPE (*)()) -1)
#endif /* !SIG_ERR */

#if !defined(MSDOS) && !defined(MAC_OS)
#ifndef ZC_INCLUDED_SYS_WAIT_H
# define ZC_INCLUDED_SYS_WAIT_H
# include <sys/wait.h>	/* Note: Old Version 7 uses just <wait.h> */
#endif /* ZC_INCLUDED_SYS_WAIT_H */
#endif /* !MSDOS && !MAC_OS */

#ifndef WEXITSTATUS
# ifndef USE_UNION_WAIT
#  define WEXITSTATUS(x) (((x) >> 8) & 0xff)
# else /* USE_UNION_WAIT */
#  define WEXITSTATUS(x) (x).w_retcode
# endif /* USE_UNION_WAIT */
#endif /* !WEXITSTATUS */

#ifndef WNOHANG
#define WNOHANG 1
#endif

#ifndef WUNTRACED
#define WUNTRACED 0
#endif /* WUNTRACED */

#ifndef WIFSTOPPED
# ifdef SIGSTOP
#  if defined(SYSV) || defined(MSDOS) || defined(AIX) || defined(MAC_OS)
#   define WIFSTOPPED(x) (((int)(x)&0377) == 0177)
#  else /* !SYSV && !MSDOS */
#   define WIFSTOPPED(x) (((union wait *)&(x))->w_stopval == 0177)
#  endif /* SYSV || MSDOS */
# else /* !SIGSTOP */
#  define WIFSTOPPED(x) 0
# endif /* SIGSTOP */
#endif /* !WIFSTOPPED */

#endif /* _CHILD_H_ */
