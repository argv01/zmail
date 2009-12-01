/* os-mips.h	Copyright 1991 Z-Code Sortware Corp. */

/*
 * This file assumes the MIPS bsd43 environment.  It is also possible to
 * compile using the System V environment, but file locking can deadlock.
 *
 * Using HAVE_INT_SPRINTF may be unnecessary but is the safest assumption.
 */

#ifdef SYSTYPE_BSD43	/* Defined by "cc -systype bsd43" */

/* os-bsd.h	Copyright 1991 Z-Code Software Corp. */

/**************************************************************************
 *
 * Source files should include OSCONFIG as follows:
 *
 * #ifndef OSCONFIG
 * #include "osconfig.h"
 * #endif
 *
 **************************************************************************/

#define OSCONFIG	/* Prevent this file from being included again */

/* Some of these names are lifted from deliver -- check copyrights */

/**************************************************************************
 *
 * Operating System Basics
 *
 **************************************************************************/

#undef  HAVE_UNISTD_H	/* Has <unistd.h> */
#define HAVE_RESOURCE_H	/* Has <resource.h> */

#define HAVE_GETHOSTID	/* Has gethostid() _and_ it works right */
#define HAVE_HOSTENT	/* Has struct hostent in <netdb.h> */
#undef  HAVE_UTSNAME	/* Has struct utsname in <sys/utsname.h> */
#undef  DECLARE_GETPW	/* Need to declare getpwuid() and getpwnam() */
#define HAVE_GETHOSTNAME	/* Has gethostname() */
#undef  HAVE_UNAME	/* Has uname() */
#undef  HAVE_UUNAME	/* Has "uuname" program */
/*#define HOSTFILE	"/etc/systemid"	/* Get hostname from file */
/*#define HOSTNAME	"bogus"		/* Hardwire the hostname */

#define HAVE_SETREUID	/* Has setreuid() and can swap ids */
#define HAVE_SAVEDUID	/* Will handle saved-set-uid setuid() */
#define HAVE_SETREGID	/* Has setregid() and can swap ids */
#undef  HAVE_SAVEDGID	/* Will handle saved-set-gid setgid() */
#undef  SYSV_SETPGRP	/* No arguments to setpgrp() */

#undef  HAVE_SIGPROCMASK	/* Has sigprocmask() */
#undef HAVE_SIGSETJMP
#undef HAVE_SIGLONGJMP	/* Has sigsetjmp() and siglongjmp() */
#define RETSIGTYPE void	/* Signal handlers have void return type */
#undef  DECLARE_SIGNAL	/* Need to declare extern RETSIGTYPE (*signal())() */
#define HAVE_RESTARTABLE_SIGNALS	/* Has SysV-style signals that interrupt read(2) */
#define HAVE_SIGLIST	/* Has extern char *sys_siglist[] */

#undef  HAVE_SYS_BSDTYPES_H	/* Has <sys/bsdtypes.h> for typedef u_long etc. */
#define DECLARE_ERRNO	/* Need to declare extern int errno */

#define HAVE_FCNTL_H	/* Has <fcntl.h> */
#undef  HAVE_RDCHK	/* Has rdchk() and should use it */
#undef  HAVE_SYS_HAVE_SELECT_H	/* Has <sys/select.h> for select() */
#define HAVE_HAVE_SELECT	/* Has select() */
#define HAVE_FD_SET_TYPE	/* Has typedef fd_set */

#define HAVE_LONG_FILE_NAMES	/* Has variable-length filenames (not 14 char) */
#define HAVE_FTRUNCATE	/* Has ftruncate() to truncate files */
#undef  HAVE_CHSIZE	/* Has chsize() to truncate files */
#define HAVE_MKDIR	/* Has mkdir() system call */
#define HAVE_READDIR	/* Has BSD-style directory routines */
#undef  DIRENT	/* Has <dirent.h> */
#define SYSDIR	/* Has <sys/dir.h> */
#define DEFINE_DIRENT	/* Needs #define dirent direct */
#define HAVE_GETWD	/* Has getwd() instead of getcwd() */

#undef  HAVE_VFORK_H	/* Has <vfork.h> (SunOS?) */
#define HAVE_VFORK	/* Has vfork() and it works right */
#undef  HAVE_WAITPID	/* Has waitpid() */
#define HAVE_WAIT3	/* Has wait3() (BSD) */
#undef  HAVE_WAIT2	/* Has wait2() (Version 7, not fully supported) */

#define HAVE_SYS_TIME_H	/* Has <sys/time.h> which includes <time.h> */
#undef  HAVE_SYS_UTIME	/* Has <sys/utime.h> for struct utimbuf */
#undef  HAVE_UTIMBUF	/* Has struct utimbuf in <utime.h> or <sys/utime.h> */
#define HAVE_TM_ZONE	/* Struct tm has char *tm_zone field */
#define HAVE_TIMEZONE	/* Has timezone() [and gettimeofday() ?] */
#undef  HAVE_TZNAME	/* Has global char *tzname[] */
#undef  DECLARE_TZNAME	/* Need to declare extern char *tzname[] */

/* If none of the HAS_T definitions are available, define constants */
/*#define TIMEZONE	"PST"	/**/
/*#define DAYLITETZ	"PDT"	/**/

/**************************************************************************
 *
 * C Compiler and Library Characteristics
 *
 **************************************************************************/

#undef  HAVE_PROTOTYPES	/* Has ANSI C style prototypes _and_ we use them */

#undef  HAVE_STDLIB_H	/* Has <stdlib.h> */
#undef  HAVE_MALLOC_H	/* Has <malloc.h> */
#define SAFE_REALLOC	/* realloc() is non-destructive on failure */
#undef  HAVE_MEMORY_H	/* Has <memory.h> */
#undef HAVE_MEMCPY
#undef HAVE_MEMSET	/* Has memcpy() and memset() */
#define HAVE_BCOPY
#define HAVE_BZERO	/* Has bcopy() and bzero() */
#undef  USG	/* Has string.h */
#define HAVE_STRINGS_H	/* Has strings.h */
#define HAVE_QSORT	/* Has working qsort() */
#undef  HAVE_INT_SPRINTF	/* sprintf() returns int like printf() */

#define HAVE_VARARGS	/* Has <varargs.h> and we should use it */
#undef  HAVE_STDARG	/* Has <stdarg.h> and it's better than <varargs.h> */
#undef  HAVE_HAVE_VPRINTF	/* Has vprintf() and vsprintf() */

#undef  HAVE_VOID_FREE	/* Declares void free() */
#undef  HAVE_CHAR_FREE	/* Declares char *free() */

#undef  HAVE_REGCMP	/* Has regcmp() and regex() */
#define HAVE_RE_COMP	/* Has re_comp() and re_exec() */
#define HAVE_RANDOM	/* Has random() and srandom() */
#undef  HAS_RAND	/* Has rand() and srand() */

#undef  HAVE_PUTENV	/* Has putenv() to modify environment */
#undef  BROKEN_ENVIRON	/* Modifying environ corrupts children (Ultrix 4) */

/**************************************************************************
 *
 * TTY and Curses Characteristics
 *
 **************************************************************************/

#define HAVE_SYS_IOCTL	/* Has <sys/ioctl.h> */
#define HAVE_TCHARS	/* Has struct tchars (from <sys/ioctl.h>) */
#define HAVE_LTCHARS	/* Has struct ltchars (from <sys/ioctl.h>) */
#define HAVE_SGTTYB	/* Has struct sgttyb (from <sys/ioctl.h> ?) */
#undef  HAVE_TERMIO	/* Has <termio.h> and struct termio */
#define HAVE_TTYCHARS	/* Has <sys/ttychars.h> and struct ttychars */
#define USES_TIOCGLTC	/* Has TIOCGLTC and actually makes good use of it */
#define *HAVE_SGTTY_DECLARED	/* Need to declare SGTTY (not decl'd in <curses.h>) */
#undef  DECLARE_TTY	/* Need to declare _tty even for curses */
#define DEFINE_UNCTRL	/* Need to define and declare char *_unctrl[] */

#define MVINCH_WORKS	/* Can read from curses window with mvinch() */
#define PRINTW_WORKS	/* Can output to curses window with printw() */
#define UPLINE_CHECK	/* Can check UP variable for motion capability */

/**************************************************************************
 *
 * Mail Transport Characteristics
 *
 **************************************************************************/

#undef  HAVE_MAILLOCK_H	/* Use SysVr4 maillock() call */
#undef  HAVE_LOCKING	/* Use locking() for file locks */
#define LOCK_FLOCK	/* Use flock() for file locks */
#undef  LOCK_FCNTL	/* Use fcntl() for locking */
#undef  LOCK_LK_LOCK	/* Use MMDF's lk_fopen() and lk_fclose() */
/*#define LCKDFLDIR	"/usr/spool/mmdf/lockfiles" /* for LOCK_LK_LOCK */

#define ML_KERNEL	/* Use kernel locking as defined above */
#undef  ML_DOTLOCK	/* Use file locking with .lock suffix */
#undef  ML_DOTMLK	/* Use file locking with .mlk suffix */
#undef  BLOCK_AGAIN	/* Deadlocked blocking locks return EAGAIN */

#undef  HAVE_HAVE_BINMAIL	/* Uses /bin/mail for MTA */
#define HAVE_SENDMAIL	/* Uses /usr/lib/sendmail for MTA */
#undef  HAVE_EXECMAIL	/* Uses /usr/lib/mail/execmail for MTA */
#undef  HAVE_SUBMIT	/* Uses MMDF's /usr/mmdf/bin/submit for MTA */
#undef  PICKY_MAILER	/* MTA won't accept From: or Date: headers */

#undef  USE_USR_MAIL	/* MTA puts mail in /usr/mail */
#define USE_SPOOL_MAIL	/* MTA puts mail in /usr/spool/mail */
#undef  USE_HOMEMAIL	/* MTA puts mail in user's home directory */
/*#define MAILFILE	"Mailbox"	/* File where HOMEMAIL is delivered */

#undef  MMDF		/* MTA writes folders in MMDF format */
#define MSG_SEPARATOR	"\001\001\001\001\n"

/*#define NO_COMMAS	/* Don't use commas in MTA envelope (address args) */
/*#define UUCP		/* Use strictly UUCP-style conventions */

/**************************************************************************
 *
 * Miscellaneous Stuff
 *
 **************************************************************************/

#define BSD		/* Needed for X11 header files */
#undef  SYSV		/* Needed for X11 header files */
#undef  USG		/* Needed for X11 header files */

#undef  AUX		/* Special set42sig() call needed */
#undef  HPUX		/* Special set42sig() call needed */
#undef  MIPS		/* Needed for X11 header files */
#undef  MOTOROLA	/* Needed for X11 header files */
#undef  TOSHIBA		/* Permit stacktracing on Toshiba 386 (SCO UNIX) */
#undef  WYSE		/* Needed for X11 header files */
#undef  pyrSVR4		/* Needed for X11 header files */


#undef  HAVE_RESOURCE_H	/* Has <resource.h> */
#define HAVE_INT_SPRINTF	/* sprintf() returns int like printf() [maybe] */
#undef  PRINTW_WORKS	/* Can output to curses window with printw() */

#define MIPS		/* Needed for X11 header files */

/* END OF CONFIGURE INFO - add extra stuff after this line */

#define setvbuf(VB_fp, VB_buf, VB_flag, VB_bufsiz)	\
	(VB_flag == _IONBF? setbuf(VB_fp, (char *)0) : \
	(VB_flag == _IOLBF? setlinebuf(VB_fp) : \
	setbuffer(VB_fp, VB_buf, VB_bufsiz)))

#define LARGE_FONTS
