/* zcsig.h	Copyright 1992 Z-Code Software Corp. */

#ifndef _ZCSIG_H_
#define _ZCSIG_H_

#ifndef OSCONFIG
#include <osconfig.h>
#endif /* OSCONFIG */

#ifndef ZC_INCLUDED_SIGNAL_H
# define ZC_INCLUDED_SIGNAL_H
# ifdef PYR
#  define _POSIX_IMPLEMENTATION
#  undef _POSIX_SOURCE
# endif /* PYR */
# include <signal.h>
# ifdef PYR
#  undef _POSIX_IMPLEMENTATION
#  define _POSIX_SOURCE
# endif /* PYR */
#endif /* ZC_INCLUDED_SIGNAL_H */

#ifndef RETSIGTYPE
#define RETSIGTYPE int
#endif /* RETSIGTYPE */

#ifdef DECLARE_SIGNAL
extern RETSIGTYPE (*signal())();
#endif /* DECLARE_SIGNAL */

#ifndef SIGCHLD
#define SIGCHLD SIGCLD
#endif /* SIGCHLD */

#ifdef HAVE_SIGLIST

# ifndef SYS_SIGLIST_DECLARED
extern char *sys_siglist[];
# endif /* SYS_SIGLIST_DECLARED */

#else /* !HAVE_SIGLIST */

#if 0	/* This belongs in a library .c file of some kind */

/* sys-v doesn't have normal sys_siglist -- hope this is right */
static char *sys_siglist[NSIG+1] = {
    /* no error */  	"no error",
    /* SIGHUP */	"hangup",
    /* SIGINT */	"interrupt (rubout)",
    /* SIGQUIT */	"quit (ASCII FS)",
    /* SIGILL */	"illegal instruction (not reset when caught)",
    /* SIGTRAP */	"trace trap (not reset when caught)",
    /* SIGIOT */	"IOT instruction",
    /* SIGEMT */	"EMT instruction",
    /* SIGFPE */	"floating point exception",
    /* SIGKILL */	"kill (cannot be caught or ignored)",
    /* SIGBUS */	"bus error",
    /* SIGSEGV */	"segmentation violation",
    /* SIGSYS */	"bad argument to system call",
    /* SIGPIPE */	"write on a pipe with no one to read it",
    /* SIGALRM */	"alarm clock",
    /* SIGTERM */	"software termination signal from kill",
    /* SIGUSR1 */	"user defined signal 1",
    /* SIGUSR2 */	"user defined signal 2",
    /* SIGCLD */	"death of a child",
    /* SIGPWR */	"power-fail restart"
};

#else	/* For zmail, it's in signals.c */

extern char *sys_siglist[];

#endif /* 0 */

#endif /* !HAVE_SIGLIST */

#endif /* _ZCSIG_H_ */
