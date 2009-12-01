/* osconfig.h.  Generated automatically by configure.  */
/* Do not make any changes to this file before the END OF CONFIGURE INFO
 * line.  OS-specific make information should go after that line.
 */

#ifndef OSCONFIG
#define OSCONFIG sun411ow

/*
 * functions we may need
 */
#define HAVE_BCMP 1
#define HAVE_BCOPY 1
#define HAVE_BZERO 1
/* #undef HAVE_CBREAK */
/* #undef HAVE_CHSIZE */
#define HAVE_FTRUNCATE 1
#define HAVE_GETDTABLESIZE 1
#define HAVE_GETENV 1
#define HAVE_GETHOSTNAME 1
#define HAVE_GETWD 1
#define HAVE_IDLOK 1
/* #undef HAVE_HERROR */
/* #undef HAVE_LOCKING */
#define HAVE_LSTAT 1
#define HAVE_MEMCPY 1
#define HAVE_MEMSET 1
#define HAVE_MKDIR 1
#define HAVE_PUTENV 1
/* #undef HAVE_PUTP */
#define HAVE_RANDOM 1
#define HAVE_READDIR 1
#define HAVE_RENAME 1
/* #undef HAVE_RESET_SHELL_MODE */
#define HAVE_RE_COMP 1
#define HAVE_SELECT 1
#define HAVE_SIGBLOCK 1
#define HAVE_SIGLONGJMP 1
#define HAVE_SIGPROCMASK 1
#define HAVE_SIGSETJMP 1
/* #undef HAVE_SLK_ATTRON */
/* #undef HAVE_SLK_INIT */
#define HAVE_SRAND 1
#define HAVE_STRCASECMP 1
#define HAVE_STRCHR 1
/* #undef HAVE_STRERROR */
/* #undef HAVE__STRICMP */	/* Windows only, so far */
#define HAVE_STRNCASECMP 1
/* #undef HAVE__STRNICMP */	/* Windows only, so far */
/* #undef HAVE_STRSIGNAL */
#define HAVE_STRSTR 1
#define HAVE_TCGETATTR 1
#define HAVE_TCSETATTR 1
/* #undef HAVE_TIGETSTR */
#define HAVE_TIMEZONE 1
#define HAVE_TOUCHLINE 1
/* #undef HAVE_TPARM */
/* #undef HAVE_TYPEAHEAD */
#define HAVE_UNAME 1
#define HAVE_U_SHORT_AND_LONG 1
#define HAVE_VPRINTF 1
/* #undef HAVE_WAIT2 */
#define HAVE_WAIT3 1
#define HAVE_WAITPID 1
/* #undef HAVE_WATTRON */
/* #undef HAVE_WATTRSET */
/* #undef HAVE_XSTANDOUT */
/*
 * header files we may need
 */
/* #undef HAVE_RESOURCE_H */
#define HAVE_FCNTL_H 1
/* #undef HAVE_SYS_SELECT_H */
/* #undef HAVE_SYS_BSDTYPES_H */
#define HAVE_STDLIB_H 1
#define HAVE_LIMITS_H 1
#define HAVE_MALLOC_H 1
#define HAVE_MEMORY_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_IOCTL_H 1
/* #undef HAVE_MAILLOCK_H */
#define HAVE_VFORK_H 1
#define HAVE_UNISTD_H 1
#define HAVE_LOCALE_H 1
/* #undef HAVE_NL_TYPES_H */
/* #undef HAVE_SYS_SYSTEMINFO_H */
#define HAVE_NETDB_H 1
/* #undef HAVE_NET_ERRNO_H */
/* #undef HAVE_SSDEFAULTSCHEME_H */

/* #undef HAVE_UUNAME */
/* #undef HAVE_AIX_CURSES */
/* #undef HAVE_CURSESX */

#define HAVE_HOSTENT 1	/* struct hostent in <netdb.h> */
#define HAVE_UTSNAME 1	/* struct utsname in <sys/utsname.h> */
/* #undef DECLARE_GETPW */	/* need to declare getpwuid() and getpwnam() */

#define HAVE_VOID_STAR 1	/* generic pointer type (void *) is accepted */
#define SAFE_BCOPY 1	/* bcopy works with overlapping areas */
#define ALIGNMENT 8
#define RETSIGTYPE void /* return type for a signal handler */
/*
 * this is true on BSD systems where read(2) is restarted when interrupted,
 * rather than returning EINTR...
 */ 
#define HAVE_RESTARTABLE_SYSCALLS 1

/*
 * this is true on SysV systems where we need to reinstall signal
 * handlers when a signal happens...
 */
/* #undef HANDLERS_NEED_RESETTING */

/* #undef NO_FOPEN_A_PLUS */  /* fopen(..., "a+") doesn't work like we expect */
#define HAVE_SIGLIST 1	/* extern char *sys_siglist[] there */
/* #undef DECLARE_ERRNO */	/* need to declare errno */
#define DECLARE_SYS_ERRLIST 1 /* need to declare sys_errlist[] */
#define HAVE_H_ERRNO 1	/* errno used by gethostbyname() and such */

#define HAVE_FD_SET_TYPE 1		/* fd_set defined */
#define HAVE_LONG_FILE_NAMES 1	/* > 14 character filenames allowed */
#define DIRENT 1
/* #undef SYSNDIR */
/* #undef SYSDIR */
#define HAVE_VFORK 1
#define TIME_WITH_SYS_TIME 1	/* should include sys/time.h and time.h */
#define HAVE_SYS_TIME_H 1		/* has <sys/time.h> */
/* #undef HAVE_BSD_SYS_TIME */	/* on MIPS, include <bsd/sys/time.h> */
/* #undef HAVE_SYS_UTIME_H */		/* has <sys/utime.h> for struct utimbuf */
#define HAVE_UTIME_H 1		/* has <utime.h> for struct utimbuf */
#define HAVE_UTIMBUF 1		/* has struct utimbuf */
#define HAVE_TM_ZONE 1		/* struct tm has char *tm_zone field */
/* #undef HAVE_TZNAME */		/* we have global char *tzname[] */
/* #undef DECLARE_TZNAME */		/* need to declare extern char *tzname[] */ 
/* #undef HAVE_INT_SPRINTF */		/* sprintf() returns int */
#define HAVE_TCHARS 1		/* struct tchars present */
#define HAVE_LTCHARS 1		/* struct ltchars present */
#define HAVE_SGTTYB 1		/* struct sgttyb present */
#define HAVE_TERMIO 1		/* <termio.h> and struct termio present */
#define HAVE_TERMIOS 1
#define HAVE_TTYCHARS 1		/* <sys/ttychars.h> and struct ttychars */
#define HAVE_SGTTY_DECLARED 1	/* SGTTY is declared in curses.h */
#define USE_UNION_WAIT 1		/* need to use union wait brain-damage */

/* #undef USE_USR_MAIL */	/* mail is in /usr/mail */
#define USE_SPOOL_MAIL 1	/* mail is in /usr/spool/mail */

#define HAVE_GETHOSTID 1	/* we have gethostid() _and_ it works right */
#define HAVE_SETREUID 1	/* we have setreuid() that can swap ids */
#define HAVE_SETREGID 1	/* we have setregid() that can swap ids */
#define HAVE_SAVEDUID 1	/* will handle saved-set-uid setuid() */
#define HAVE_SAVEDGID 1	/* will handle saved-set-gid setgid() */
/* #undef SYSV_SETPGRP */	/* no arguments to setpgrp() */

/* #undef pid_t */		/* will be defined to "int" if not in <sys/types.h> */
#define GETGROUPS_T int	/* will be defined to "int" or "gid_t" */

#define DECLARE_SIGNAL 1	/* Need to declare extern SIGRET (*signal())() */
/* #undef HAVE_RDCHK */	/* Has rdchk() and should use it */
/* #undef HAVE_PROTOTYPES */	/* Has ANSI C style prototypes _and_ we use them */
#define const 		/* Does not fully support the "const" qualifier */
#define SAFE_REALLOC 1	/* realloc() is non-destructive on failure */
#define HAVE_QSORT 1	/* Has working qsort() */
#define HAVE_VARARGS_H 1	/* Has <varargs.h> and we should use it */
/* #undef HAVE_STDARG_H */	/* Has <stdarg.h> and it's better than <varargs.h> */
/* #undef HAVE_VOID_FREE */	/* Declares void free() */
#define HAVE_CHAR_FREE 1	/* Declares char *free() */
/* #undef BROKEN_ENVIRON */	/* Modifying environ corrupts children (Ultrix 4) */
/* #undef USES_TIOCGLTC */	/* Has TIOCGLTC and actually makes good use of it */
#define DECLARE_TTY 1	/* Need to declare _tty even for curses */
/* #undef DEFINE_UNCTRL */	/* Need to define and declare char *_unctrl[] */
#define MVINCH_WORKS 1	/* Can read from curses window with mvinch() */
#define PRINTW_WORKS 1	/* Can output to curses window with printw() */
#define UPLINE_CHECK 1	/* Can check UP variable for motion capability */
/* #undef SYSV_CURSES_BUG */	/* Has SysV curses bug with drawing top few lines */
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
#define BSD 1		/* Needed for X11 header files */
/* #undef SYSV */		/* Needed for X11 header files */
/* #undef USG */		/* Needed for X11 header files */
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
#define HAVE_INDEX 1
/* #undef DECLARE_INDEX */
#define SAFE_CASE_CHANGE 1	/* May use tolower/toupper without range checks */

/* to allocate an array of objects, must do
 *   array = (foo *) malloc(n * ALIGN(sizeof (foo)));
 * the MALIGN(n) (yuk yuk, it means "maybe align") macro in
 * general/general.h expands to ALIGN(n) or (n) when appropriate.
 */
/* #undef MUST_ALIGN */

/* can't include <sys/stat.h> without first #including <sys/types.h> */
/* #undef STAT_H_NEEDS_TYPES_H */

/* #undef TERM_USE_TERMIO */
#define TERM_USE_TERMIOS 1
/* #undef TERM_USE_SGTTYB */
/* #undef TERM_USE_NONE */

#define DECLARE_ENVIRON 1

/* #undef XLIB_ILLEGAL_ACCESS */  /* special access to Xlib internals */
#define XT_R4_INCLUDES -I/zyrcon/usr1/src/X11R4/mit/lib/Xt	    /* Xt R4 internal headers available */
/* #undef HAVE_XT_BASE_TRANSLATIONS */	/* XmNbaseTranslations resources */

/* Motif 1.2 defines */
/* #undef MOTIF_1_2 */
#define SELECT_POS_LIST 1

/* this is true if curses.h does not include any terminal #includes */
/* #undef CURSES_NEEDS_INCLUDE_TERM */

/* #undef _ALL_SOURCE */		/* needed for AIX */
/* #undef _POSIX_SOURCE */		/* needed for ISC, OSF/1, HP/UX */
/* #undef _XOPEN_SOURCE */		/* needed for OSF/1, HP/UX */
/* #undef _HPUX_SOURCE */		/* needed for HPUX */
/* #undef _OSF_SOURCE */		/* needed for OSF/1 */

#define MSG_SEPARATOR "\001\001\001\001\n"

/* special SGI features */
/* #undef FAM_OPEN */		/* which FAMOpen() to use, if any */
/* #undef HAVE_HELP_BROKER */	/* Insight-based help system */
/* #undef OZ_DATABASE */	/* drag & drop icons */
/* #undef HAVE_IMPRESSARIO */	/* Impressario print widget */

/* corrections to autoconf output follow */

/* END OF CONFIGURE INFO - add extra config info after this line */

#define SUN_4_1		/* Needed to find TTY stuff */
#undef HAVE_STRSTR /* it's beastly slow */

#endif /* OSCONFIG -- do not add stuff after this line */