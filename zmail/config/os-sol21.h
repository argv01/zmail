/* osconfig.h.  Generated automatically by configure.  */
/* Do not make any changes to this file before the END OF CONFIGURE INFO
 * line.  OS-specific make information should go after that line.
 */

#ifndef OSCONFIG
#define OSCONFIG sol21

/*
 * functions we may need
 */
/* #undef HAVE_BCMP */
/* #undef HAVE_BCOPY */
/* #undef HAVE_BZERO */
#define HAVE_CBREAK 1
/* #undef HAVE_CHSIZE */
#define HAVE_FTRUNCATE 1
/* #undef HAVE_GETDTABLESIZE */
#define HAVE_GETENV 1
/* #undef HAVE_GETHOSTNAME */
/* #undef HAVE_GETWD */
#define HAVE_IDLOK 1
/* #undef HAVE_LOCKING */
#define HAVE_LSTAT 1
#define HAVE_MEMCPY 1
#define HAVE_MEMSET 1
#define HAVE_MKDIR 1
#define HAVE_PUTENV 1
#define HAVE_PUTP 1
/* #undef HAVE_RANDOM */
#define HAVE_READDIR 1
/* #undef HAVE_REGCMP */
#define HAVE_RENAME 1
#define HAVE_RESET_SHELL_MODE 1
/* #undef HAVE_RE_COMP */
#define HAVE_SELECT 1
/* #undef HAVE_SIGBLOCK */
#define HAVE_SIGLONGJMP 1
#define HAVE_SIGPROCMASK 1
#define HAVE_SIGSETJMP 1
#define HAVE_SLK_ATTRON 1
#define HAVE_SLK_INIT 1
#define HAVE_SRAND 1
#define HAVE_STRCHR 1
#define HAVE_STRERROR 1
#define HAVE_STRSIGNAL 1
#define HAVE_TCSETATTR 1
#define HAVE_TIGETSTR 1
#define HAVE_TIMEZONE 1
#define HAVE_TOUCHLINE 1
#define HAVE_TPARM 1
#define HAVE_TYPEAHEAD 1
#define HAVE_UNAME 1
#define HAVE_VPRINTF 1
/* #undef HAVE_WAIT2 */
/* #undef HAVE_WAIT3 */
#define HAVE_WAITPID 1
#define HAVE_WATTRON 1
#define HAVE_WATTRSET 1
/* #undef HAVE_XSTANDOUT */
/*
 * header files we may need
 */
/* #undef HAVE_RESOURCE_H */
#define HAVE_FCNTL_H 1
#define HAVE_SYS_SELECT_H 1
/* #undef HAVE_SYS_BSDTYPES_H */
#define HAVE_STDLIB_H 1
#define HAVE_MALLOC_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STRING_H 1
/* #undef HAVE_STRINGS_H */
#define HAVE_STRING_H 1
#define HAVE_SYS_IOCTL_H 1
#define HAVE_MAILLOCK_H 1
/* #undef HAVE_VFORK_H */
#define HAVE_UNISTD_H 1
#define HAVE_LOCALE_H 1
#define HAVE_NL_TYPES_H 1
#define HAVE_SYS_SYSTEMINFO_H 1

#define HAVE_UUNAME 1
/* #undef HAVE_AIX_CURSES */
/* #undef HAVE_CURSESX */

#define HAVE_HOSTENT 1	/* struct hostent in <netdb.h> */
#define HAVE_UTSNAME 1	/* struct utsname in <sys/utsname.h> */
/* #undef DECLARE_GETPW */	/* need to declare getpwuid() and getpwnam() */

#define HAVE_VOID_STAR 1	/* generic pointer type (void *) is accepted */
/* #undef SAFE_BCOPY */	/* bcopy works with overlapping areas */
#define ALIGNMENT 8
#define RETSIGTYPE void /* return type for a signal handler */
/*
 * this is true on BSD systems where read(2) is restarted when interrupted,
 * rather than returning EINTR...
 */ 
/* #undef HAVE_RESTARTABLE_SYSCALLS */

/*
 * this is true on SysV systems where we need to reinstall signal
 * handlers when a signal happens...
 */
#define HANDLERS_NEED_RESETTING 1

#define NO_FOPEN_A_PLUS 1  /* fopen(..., "a+") doesn't work like we expect */
/* #undef HAVE_SIGLIST */	/* extern char *sys_siglist[] there */
/* #undef DECLARE_ERRNO */	/* need to declare errno */
#define DECLARE_SYS_ERRLIST 1 /* need to declare sys_errlist[] */

#define HAVE_FD_SET_TYPE 1		/* fd_set defined */
#define HAVE_LONG_FILE_NAMES 1	/* > 14 character filenames allowed */
#define DIRENT 1
/* #undef SYSNDIR */
/* #undef SYSDIR */
#undef HAVE_VFORK
#define TIME_WITH_SYS_TIME 1	/* should include sys/time.h and time.h */
#define HAVE_SYS_TIME_H 1		/* has <sys/time.h> */
/* #undef HAVE_BSD_SYS_TIME */	/* on MIPS, include <bsd/sys/time.h> */
#define HAVE_SYS_UTIME_H 1		/* has <sys/utime.h> for struct utimbuf */
#define HAVE_UTIME_H 1		/* has <utime.h> for struct utimbuf */
#define HAVE_UTIMBUF 1		/* has struct utimbuf */
/* #undef HAVE_TM_ZONE */		/* struct tm has char *tm_zone field */
#define HAVE_TZNAME 1		/* we have global char *tzname[] */
/* #undef DECLARE_TZNAME */		/* need to declare extern char *tzname[] */ 
#define HAVE_INT_SPRINTF 1		/* sprintf() returns int */
/* #undef HAVE_TCHARS */		/* struct tchars present */
/* #undef HAVE_LTCHARS */		/* struct ltchars present */
/* #undef HAVE_SGTTYB */		/* struct sgttyb present */
#define HAVE_TERMIO 1		/* <termio.h> and struct termio present */
#define HAVE_TERMIOS 1
#define HAVE_TTYCHARS 1		/* <sys/ttychars.h> and struct ttychars */
#define HAVE_SGTTY_DECLARED 1	/* SGTTY is declared in curses.h */
/* #undef USE_UNION_WAIT */		/* need to use union wait brain-damage */

#define USE_USR_MAIL 1	/* mail is in /usr/mail */
/* #undef USE_SPOOL_MAIL */	/* mail is in /usr/spool/mail */

/* #undef HAVE_GETHOSTID */	/* we have gethostid() _and_ it works right */
/* #undef HAVE_SETREUID */	/* we have setreuid() that can swap ids */
/* #undef HAVE_SETREGID */	/* we have setregid() that can swap ids */
#define HAVE_SAVEDUID 1	/* will handle saved-set-uid setuid() */
#define HAVE_SAVEDGID 1	/* will handle saved-set-gid setgid() */
#define SYSV_SETPGRP 1	/* no arguments to setpgrp() */

#define GETGROUPS_T gid_t	/* will be defined to "int" or "gid_t" */

/* #undef DECLARE_SIGNAL */	/* Need to declare extern SIGRET (*signal())() */
/* #undef HAVE_RDCHK */	/* Has rdchk() and should use it */
#define HAVE_PROTOTYPES 1	/* Has ANSI C style prototypes _and_ we use them */
/* #undef const */		/* Does not fully support the "const" qualifier */
#define SAFE_REALLOC 1	/* realloc() is non-destructive on failure */
#define HAVE_QSORT 1	/* Has working qsort() */
/* #undef HAVE_VARARGS_H */	/* Has <varargs.h> and we should use it */
#define HAVE_STDARG_H 1	/* Has <stdarg.h> and it's better than <varargs.h> */
#define HAVE_VOID_FREE 1	/* Declares void free() */
/* #undef HAVE_CHAR_FREE */	/* Declares char *free() */
/* #undef BROKEN_ENVIRON */	/* Modifying environ corrupts children (Ultrix 4) */
/* #undef USES_TIOCGLTC */	/* Has TIOCGLTC and actually makes good use of it */
#define DECLARE_TTY 1	/* Need to declare _tty even for curses */
/* #undef DEFINE_UNCTRL */	/* Need to define and declare char *_unctrl[] */
#define MVINCH_WORKS 1	/* Can read from curses window with mvinch() */
#define PRINTW_WORKS 1	/* Can output to curses window with printw() */
#define UPLINE_CHECK 1	/* Can check UP variable for motion capability */
#define SYSV_CURSES_BUG 1	/* Has SysV curses bug with drawing top few lines */
			/* SYSV_CURSES_BUG should be defined to 1 or 2 */
/* #undef LOCK_FLOCK */	/* Use flock() for file locks */
/* #undef LOCK_FCNTL */	/* Use fcntl() for locking */
/* #undef LOCK_LK_LOCK */	/* Use MMDF's lk_fopen() and lk_fclose() */
/* #undef LOCK_LOCKING */	/* Use locking() for locking, if present */
/* #undef ML_KERNEL */	/* Use kernel locking as defined above */
/* #undef ML_DOTLOCK */	/* Use file locking with .lock suffix */
/* #undef ML_DOTMLK */	/* Use file locking with .mlk suffix */
/* #undef BLOCK_AGAIN */	/* Deadlocked blocking locks return EAGAIN */
/* #undef HAVE_BINMAIL */	/* Uses /bin/mail for MTA */
#define HAVE_SENDMAIL 1	/* Uses /usr/lib/sendmail for MTA */
/* #undef HAVE_EXECMAIL */	/* Uses /usr/lib/mail/execmail for MTA */
/* #undef HAVE_SUBMIT */	/* Uses MMDF's /usr/mmdf/bin/submit for MTA */
/* #undef PICKY_MAILER */	/* MTA won't accept From: or Date: headers */
/* #undef USE_HOMEMAIL */	/* MTA puts mail in user's home directory */
/* #undef MMDF */		/* MTA writes folders in MMDF format */
/* #undef BSD */		/* Needed for X11 header files */
#define SYSV 1		/* Needed for X11 header files */
#define USG 1		/* Needed for X11 header files */
/* #undef AUX */		/* Special set42sig() call needed */
/* #undef HPUX */		/* Needed for X11 header files */
/* #undef MIPS */		/* Needed for X11 header files */
/* #undef MOTOROLA */		/* Needed for X11 header files */
/* #undef TOSHIBA */		/* Permit stacktracing on Toshiba 386 (SCO UNIX) */
/* #undef WYSE */		/* Needed for X11 header files */
/* #undef pyrSVR4 */		/* Needed for X11 header files */
/* #undef PYRAMID */		/* Needed for license/zserver.c */
#define HAVE_STRCAT_STRCPY_DECLARED 1
#define HAVE_STRCMP_DECLARED 1
#define HAVE_STRLEN_DECLARED 1

/* to allocate an array of objects, must do
 *   array = (foo *) malloc(n * ALIGN(sizeof (foo)));
 * the MALIGN(n) (yuk yuk, it means "maybe align") macro in
 * general/general.h expands to ALIGN(n) or (n) when appropriate.
 */
/* #undef MUST_ALIGN */

/* can't include <sys/stat.h> without first #including <sys/types.h> */
/* #undef STAT_H_NEEDS_TYPES_H */

#define TERM_USE_TERMIO 1
/* #define TERM_USE_TERMIOS 1 */
/* #undef TERM_USE_SGTTYB */
/* #undef TERM_USE_NONE */

#define DECLARE_ENVIRON 1

/* #undef XLIB_ILLEGAL_ACCESS */  /* special access to Xlib internals */

/* Motif 1.2 defines */
#define MOTIF_1_2 1
#define SELECT_POS_LIST 1

/* this is true if curses.h does not include any terminal #includes */
/* #undef CURSES_NEEDS_INCLUDE_TERM */

/* #undef _ALL_SOURCE */	/* needed for AIX */
/* #undef _POSIX_SOURCE */	/* needed for ISC */
/* #undef _XOPEN_SOURCE */	/* needed for ??? */
/* #undef _HPUX_SOURCE */	/* needed for HPUX */

#define MSG_SEPARATOR "\001\001\001\001\n"

/* corrections to autoconf output follow */

/* END OF CONFIGURE INFO - add extra config info after this line */

/* Avoid duplicate defs. */
#define __TIMEVAL__

#define SUN_4_1		/* Needed to find TTY stuff */
#endif /* OSCONFIG -- do not add stuff after this line */
