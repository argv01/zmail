/* osconfig.h.  Generated automatically by configure.  */
/* Do not make any changes to this file before the END OF CONFIGURE INFO
 * line.  OS-specific make information should go after that line.
 */

#ifndef OSCONFIG
#define OSCONFIG darwin

/*
 * functions we may need
 */
#define HAVE_CBREAK 1
/* #undef HAVE_CHSIZE */
#define HAVE_FTRUNCATE 1
#define HAVE_GETDTABLESIZE 1
#define HAVE_GETENV 1
#define HAVE_GETHOSTNAME 1
#define HAVE_GETWD 1
#define HAVE_IDLOK 1
#define HAVE_HERROR 1
/* #undef HAVE_LOCKING */
#define HAVE_LSTAT 1
/* #undef HAVE_MEMMOVE */
#define HAVE_MKDIR 1
#define HAVE_POLL 1
#define HAVE_POPEN 1
#define HAVE_PUTENV 1
#define HAVE_PUTP 1
#define HAVE_RANDOM 1
#define HAVE_READDIR 1
#define HAVE_RENAME 1
#define HAVE_RESET_SHELL_MODE 1
/* #undef HAVE_RE_COMP */
#define HAVE_SELECT 1
#define HAVE_SIGBLOCK 1
#define HAVE_SIGLONGJMP 1
#define HAVE_SIGPROCMASK 1
#define HAVE_SIGSETJMP 1
#define HAVE_SLK_ATTRON 1
#define HAVE_SLK_INIT 1
#define HAVE_SRAND 1
#define HAVE_STRCASECMP 1
#define HAVE_STRCHR 1
#define HAVE_STRERROR 1
/* #undef HAVE__STRICMP */	/* Windows only, so far */
#define HAVE_STRNCASECMP 1
/* #undef HAVE__STRNICMP */	/* Windows only, so far */
#define HAVE_STRPBRK 1
#define HAVE_STRSIGNAL 1
#define HAVE_STRSPN 1
#define HAVE_STRSTR 1
#define HAVE_STRTOUL 1
#define HAVE_TCGETATTR 1
#define HAVE_TCSETATTR 1
#define HAVE_TIGETSTR 1
#define HAVE_TIMEZONE 1
#define HAVE_TOUCHLINE 1
#define HAVE_TPARM 1
#define HAVE_TYPEAHEAD 1
#define HAVE_UNAME 1
#define HAVE_U_SHORT_AND_LONG 1
#define HAVE_VPRINTF 1
/* #undef HAVE_WAIT2 */
#define HAVE_WAIT3 1
#define HAVE_WAITPID 1
#define HAVE_WATTRON 1
#define HAVE_WATTRSET 1
/* #undef HAVE_XSTANDOUT */
/*
 * header files we may need
 */
/* #undef HAVE_BSTRING_H */
/* #undef HAVE_RESOURCE_H */
#define HAVE_FCNTL_H 1
#define HAVE_SYS_SELECT_H 1
/* #undef HAVE_SYS_BSDTYPES_H */
#define HAVE_STDLIB_H 1
#define HAVE_LIMITS_H 1
/* #undef HAVE_MALLOC_H */
#define HAVE_MEMORY_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_SYS_IOCTL_H 1
/* #undef HAVE_MAILLOCK_H */
/* #undef HAVE_VFORK_H */
#define HAVE_UNISTD_H 1
#define HAVE_LOCALE_H 1
#define HAVE_NL_TYPES_H 1
/* #undef HAVE_SYS_SYSTEMINFO_H */
#define HAVE_NETDB_H 1
/* #undef HAVE_NET_ERRNO_H */
/* #undef HAVE_SSDEFAULTSCHEME_H */

/* #undef HAVE_UUNAME */
/* #undef HAVE_AIX_CURSES */
/* #undef HAVE_CURSESX */
/* #undef HAVE_NCURSES */

#define HAVE_HOSTENT 1	/* struct hostent in <netdb.h> */
#define HAVE_UTSNAME 1	/* struct utsname in <sys/utsname.h> */
/* #undef DECLARE_GETPW */	/* need to declare getpwuid() and getpwnam() */

#define INTIS32BITS 1	/* is an int 32 bits wide? */
#define LONGIS32BITS 1	/* is a long 32 bits wide? */
#define HAVE_VOID_STAR 1	/* generic pointer type (void *) is accepted */
#define SAFE_BCOPY 1	/* bcopy works with overlapping areas */
#define HAVE_B_MEMFUNCS 1	/* bcopy and friends */
#define HAVE_MEM_MEMFUNCS 1 /* memcpy and friends */
#define ALIGNMENT 4
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

#define NO_FOPEN_A_PLUS 1  /* fopen(..., "a+") doesn't work like we expect */
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
#define HAVE_INT_SPRINTF 1		/* sprintf() returns int */
/* #undef HAVE_TCHARS */		/* struct tchars present */
/* #undef HAVE_LTCHARS */		/* struct ltchars present */
/* #undef HAVE_SGTTYB */		/* struct sgttyb present */
/* #undef HAVE_TERMIO */		/* <termio.h> and struct termio present */
#define HAVE_TERMIOS 1
#define HAVE_TTYCHARS 1		/* <sys/ttychars.h> and struct ttychars */
/* #undef HAVE_SGTTY_DECLARED */	/* SGTTY is declared in curses.h */
/* #undef USE_UNION_WAIT */		/* need to use union wait brain-damage */

/* #undef USE_USR_MAIL */	/* mail is in /usr/mail */
#define USE_SPOOL_MAIL 1	/* mail is in /usr/spool/mail */

/* #undef HAVE_GETHOSTID */	/* we have gethostid() _and_ it works right */
#define HAVE_SETREUID 1	/* we have setreuid() that can swap ids */
#define HAVE_SETREGID 1	/* we have setregid() that can swap ids */
#define HAVE_SAVEDUID 1	/* will handle saved-set-uid setuid() */
#define HAVE_SAVEDGID 1	/* will handle saved-set-gid setgid() */

#define SYS_SIGLIST_DECLARED 1	/* is sys_siglist declared in a header? */

/* #undef SYSV_SETPGRP */	/* no arguments to setpgrp() */

/* #undef pid_t */		/* will be defined to "int" if not in <sys/types.h> */
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
#define DEFINE_UNCTRL 1	/* Need to define and declare char *_unctrl[] */
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
#define BSD 199506		/* Needed for X11 header files */
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

#define XLIB_ILLEGAL_ACCESS 1  /* special access to Xlib internals */
/* #undef XT_R4_INCLUDES */	    /* Xt R4 internal headers available */
#define HAVE_XT_BASE_TRANSLATIONS 1	/* XmNbaseTranslations resources */
/* #undef XEDITRES_HANDLER */		/* name of editres event handler */

/* motif/xm/list.c gets compiled if either of these next two are defined */
#define SELECT_POS_LIST 1	/* use XmListSelectPositions from motif/xm/list.c */
#define USE_XM_LIST_C 1	/* use list widget from motif/xm/list.c */

/* this is true if curses.h does not include any terminal #includes */
#define CURSES_NEEDS_INCLUDE_TERM 1

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
#undef OSCONFIG
#define OSCONFIG 
#undef HAVE_FTRUNCATE
#define HAVE_FTRUNCATE 
#undef HAVE_GETHOSTNAME
#define HAVE_GETHOSTNAME 
#undef HAVE_GETWD
#define HAVE_GETWD 
#undef HAVE_MKDIR
#define HAVE_MKDIR 
#undef HAVE_PUTENV
#undef HAVE_RANDOM
#define HAVE_RANDOM 
#undef HAVE_READDIR
#define HAVE_READDIR 
#define HAVE_RE_COMP 
#undef HAVE_SIGLONGJMP
#define HAVE_SIGLONGJMP 
#undef HAVE_SIGPROCMASK
#define HAVE_SIGPROCMASK 
#undef HAVE_SIGSETJMP
#define HAVE_SIGSETJMP 
#undef HAVE_TIMEZONE
#define HAVE_TIMEZONE 
#undef HAVE_UNAME
#undef HAVE_WAIT3
#define HAVE_WAIT3 
#undef HAVE_WAITPID
#define HAVE_RESOURCE_H 
#undef HAVE_FCNTL_H
#define HAVE_FCNTL_H 
#undef HAVE_STDLIB_H
#undef HAVE_MEMORY_H
#undef HAVE_STRINGS_H
#define HAVE_STRINGS_H 
#undef HAVE_HOSTENT
#define HAVE_HOSTENT 
#undef HAVE_UTSNAME
#undef HAVE_SIGLIST
#define HAVE_SIGLIST 
#define DECLARE_ERRNO 
#undef DECLARE_SYS_ERRLIST
#undef HAVE_FD_SET_TYPE
#define HAVE_FD_SET_TYPE 
#undef HAVE_LONG_FILE_NAMES
#define HAVE_LONG_FILE_NAMES 
#undef DIRENT
#define SYSDIR 
#undef HAVE_VFORK
#define HAVE_VFORK 
#undef HAVE_SYS_TIME_H
#define HAVE_SYS_TIME_H 
#undef HAVE_UTIMBUF
#undef HAVE_TM_ZONE
#define HAVE_TM_ZONE 
#undef HAVE_INT_SPRINTF
#define HAVE_INT_SPRINTF 
#define HAVE_TCHARS 
#define HAVE_LTCHARS 
#define HAVE_SGTTYB 
#undef HAVE_TTYCHARS
#define HAVE_TTYCHARS 
#undef USE_SPOOL_MAIL
#define USE_SPOOL_MAIL 
#define HAVE_GETHOSTID 
#undef HAVE_SETREUID
#define HAVE_SETREUID 
#undef HAVE_SETREGID
#define HAVE_SETREGID 
#undef HAVE_SAVEDUID
#define HAVE_SAVEDUID 
#undef HAVE_SAVEDGID
#undef SAFE_REALLOC
#define SAFE_REALLOC 
#undef HAVE_QSORT
#define HAVE_QSORT 
#undef HAVE_VOID_FREE
#define USES_TIOCGLTC 
#undef DECLARE_TTY
#undef DEFINE_UNCTRL
#define DEFINE_UNCTRL 
#undef MVINCH_WORKS
#define MVINCH_WORKS 
#undef PRINTW_WORKS
#define PRINTW_WORKS 
#undef UPLINE_CHECK
#define UPLINE_CHECK 
#define LOCK_FLOCK 
#define ML_KERNEL 
#undef HAVE_SENDMAIL
#define HAVE_SENDMAIL 
#undef BSD
#define BSD 

/* END OF CONFIGURE INFO - add extra config info after this line */

#undef DECLARE_SYS_ERRLIST
#define HAVE_UNISTD_H 1
#define DARWIN 1
/*#define SYSV_SETPGRP */
#undef SYSV_SETPGRP
#define HAVE_STDARG_H 1
#define HAVE_PROTOTYPES 1
#undef USE_XM_LIST_C 
#define HAVE_NCURSES 1
#define HAVE_VOID_FREE 1

#endif /* OSCONFIG -- do not add stuff after this line */
