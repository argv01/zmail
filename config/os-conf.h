/* os-conf.h	Copyright 1991 Z-Code Software Corp. */

/**************************************************************************
 *
 * This is the complete template file used to generate an operating system
 * configuration file.  It's often easier to create new files by including
 * one of "os-bsd.h" or "os-sysv.h" and then adding #defines or #undefs.
 *
 * Source files should include OSCONFIG as follows:
 *
 * #ifdef OSCONFIG
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

#define HAVE_UNISTD_H	/* Has <unistd.h> */
#define HAVE_RESOURCE_H	/* Has <resource.h> */

#define HAVE_GETHOSTID	/* Has gethostid() _and_ it works right */
#define HAVE_HOSTENT	/* Has struct hostent in <netdb.h> */
#define HAVE_UTSNAME	/* Has struct utsname in <sys/utsname.h> */
#define DECLARE_GETPW	/* Need to declare getpwuid() and getpwnam() */
#define HAVE_GETHOSTNAME	/* Has gethostname() */
#define HAVE_UNAME	/* Has uname() */
#define HAVE_UUNAME	/* Has "uuname" program */
/*#define HOSTFILE	"/etc/systemid"	/* Get hostname from file */
/*#define HOSTNAME	"bogus"		/* Hardwire the hostname */

#define HAVE_SETREUID	/* Has setreuid() and can swap ids */
#define HAVE_SAVEDUID	/* Will handle saved-set-uid setuid() */
#define HAVE_SETREGID	/* Has setregid() and can swap ids */
#define HAVE_SAVEDGID	/* Will handle saved-set-gid setgid() */
#define SYSV_SETPGRP	/* No arguments to setpgrp() */

#define HAVE_SIGPROCMASK	/* Has sigprocmask() */
#define HAVE_SIGSETJMP
#define HAVE_SIGLONGJMP	/* Has sigsetjmp() and siglongjmp() */
#define RETSIGTYPE void	/* Signal handlers have void return type */
#define DECLARE_SIGNAL	/* Need to declare extern RETSIGTYPE (*signal())() */
#undef HAVE_RESTARTABLE_SIGNALS	/* Has SysV-style signals that interrupt read(2) */
#define HAVE_SIGLIST	/* Has extern char *sys_siglist[] */

#define HAVE_SYS_BSDTYPES_H	/* Has <sys/bsdtypes.h> for typedef u_long etc. */
#define DECLARE_ERRNO	/* Need to declare extern int errno */

#define HAVE_FCNTL_H	/* Has <fcntl.h> */
#define HAVE_RDCHK	/* Has rdchk() and should use it */
#define HAVE_SYS_HAVE_SELECT_H	/* Has <sys/select.h> for select() */
#define HAVE_HAVE_SELECT	/* Has select() */
#define HAVE_FD_SET_TYPE	/* Has typedef fd_set */

#define HAVE_LONG_FILE_NAMES	/* Has variable-length filenames (not 14 char) */
#define HAVE_FTRUNCATE	/* Has ftruncate() to truncate files */
#define HAVE_CHSIZE	/* Has chsize() to truncate files */
#define HAVE_MKDIR	/* Has mkdir() system call */
#define HAVE_RENAME	/* Has rename() system call */
#define HAVE_READDIR	/* Has BSD-style directory routines */
#define DIRENT	/* Has <dirent.h> */
#define SYSDIR	/* Has <sys/dir.h> */
#define DEFINE_DIRENT	/* Needs #define dirent direct */
#define HAVE_GETWD	/* Has getwd() instead of getcwd() */

#define HAVE_VFORK_H	/* Has <vfork.h> (SunOS?) */
#define HAVE_VFORK	/* Has vfork() and it works right */
#define HAVE_WAITPID	/* Has waitpid() */
#define HAVE_WAIT3	/* Has wait3() (BSD) */
#define HAVE_WAIT2	/* Has wait2() (Version 7, not fully supported) */

#define HAVE_SYS_TIME_H	/* Has <sys/time.h> which includes <time.h> */
#define HAVE_SYS_UTIME	/* Has <sys/utime.h> for struct utimbuf */
#define HAVE_UTIMBUF	/* Has struct utimbuf in <utime.h> or <sys/utime.h> */
#define HAVE_TM_ZONE	/* Struct tm has char *tm_zone field */
#define HAVE_TIMEZONE	/* Has timezone() [and gettimeofday() ?] */
#define HAVE_TZNAME	/* Has global char *tzname[] */
#define DECLARE_TZNAME	/* Need to declare extern char *tzname[] */

/* If none of the HAS_T definitions are available, define constants */
/*#define TIMEZONE	"PST"	/**/
/*#define DAYLITETZ	"PDT"	/**/

/**************************************************************************
 *
 * C Compiler and Library Characteristics
 *
 **************************************************************************/

#define HAVE_PROTOTYPES	/* Has ANSI C style prototypes _and_ we use them */

#define HAVE_STDLIB_H	/* Has <stdlib.h> */
#define HAVE_MALLOC_H	/* Has <malloc.h> */
#define SAFE_REALLOC	/* realloc() is non-destructive on failure */
#define HAVE_MEMORY_H	/* Has <memory.h> */
#define HAVE_MEMCPY
#define HAVE_MEMSET	/* Has memcpy() and memset() */
#define HAVE_BCOPY
#define HAVE_BZERO	/* Has bcopy() and bzero() */
#define USG	/* Has string.h */
#define HAVE_STRINGS_H	/* Has strings.h */
#define HAVE_QSORT	/* Has working qsort() */
#define HAVE_INT_SPRINTF	/* sprintf() returns int like printf() */

#define HAVE_VARARGS	/* Has <varargs.h> and we should use it */
#define HAVE_STDARG	/* Has <stdarg.h> and it's better than <varargs.h> */
#define HAVE_HAVE_VPRINTF	/* Has vprintf() and vsprintf() */

#define HAVE_VOID_FREE	/* Declares void free() */
#define HAVE_CHAR_FREE	/* Declares char *free() */

#define HAVE_REGCMP	/* Has regcmp() and regex() */
#define HAVE_RE_COMP	/* Has re_comp() and re_exec() */
#define HAVE_RANDOM	/* Has random() and srandom() */
#define HAVE_SRAND	/* Has rand() and srand() */

#define HAVE_PUTENV	/* Has putenv() to modify environment */
#define BROKEN_ENVIRON	/* Modifying environ corrupts children (Ultrix 4) */

/**************************************************************************
 *
 * TTY and Curses Characteristics
 *
 **************************************************************************/

#define HAVE_SYS_IOCTL	/* Has <sys/ioctl.h> */
#define HAVE_TCHARS	/* Has struct tchars (from <sys/ioctl.h>) */
#define HAVE_LTCHARS	/* Has struct ltchars (from <sys/ioctl.h>) */
#define HAVE_SGTTYB	/* Has struct sgttyb (from <sys/ioctl.h> ?) */
#define HAVE_TERMIO	/* Has <termio.h> and struct termio */
#define HAVE_TTYCHARS	/* Has <sys/ttychars.h> and struct ttychars */
#define USES_TIOCGLTC	/* Has TIOCGLTC and actually makes good use of it */
#undef *HAVE_SGTTY_DECLARED	/* Need to declare SGTTY (not decl'd in <curses.h>) */
#define DECLARE_TTY	/* Need to declare _tty even for curses */
#define DEFINE_UNCTRL	/* Need to define and declare char *_unctrl[] */

#define MVINCH_WORKS	/* Can read from curses window with mvinch() */
#define PRINTW_WORKS	/* Can output to curses window with printw() */
#define UPLINE_CHECK	/* Can check UP variable for motion capability */
#define SYSV_CURSES_BUG	/* Has SysV curses bug with drawing top few lines */
			/* SYSV_CURSES_BUG should be defined to 1 or 2 */

/**************************************************************************
 *
 * Mail Transport Characteristics
 *
 **************************************************************************/

#define HAVE_MAILLOCK_H	/* Use SysVr4 maillock() call */
#define HAVE_LOCKING	/* Use locking() for file locks */
#define LOCK_FLOCK	/* Use flock() for file locks */
#define LOCK_FCNTL	/* Use fcntl() for locking */
#define LOCK_LK_LOCK	/* Use MMDF's lk_fopen() and lk_fclose() */
/*#define LCKDFLDIR	"/usr/spool/mmdf/lockfiles" /* for LOCK_LK_LOCK */

#define ML_KERNEL	/* Use kernel locking as defined above */
#define ML_DOTLOCK	/* Use file locking with .lock suffix */
#define ML_DOTMLK	/* Use file locking with .mlk suffix */
#define BLOCK_AGAIN	/* Deadlocked blocking locks return EAGAIN */

#define HAVE_HAVE_BINMAIL	/* Uses /bin/mail for MTA */
#define HAVE_SENDMAIL	/* Uses /usr/lib/sendmail for MTA */
#define HAVE_EXECMAIL	/* Uses /usr/lib/mail/execmail for MTA */
#define HAVE_SUBMIT	/* Uses MMDF's /usr/mmdf/bin/submit for MTA */
#define PICKY_MAILER	/* MTA won't accept From: or Date: headers */

#define USE_USR_MAIL	/* MTA puts mail in /usr/mail */
#define USE_SPOOL_MAIL	/* MTA puts mail in /usr/spool/mail */
#define USE_HOMEMAIL	/* MTA puts mail in user's home directory */
/*#define MAILFILE	"Mailbox"	/* File where HOMEMAIL is delivered */

#define MMDF		/* MTA writes folders in MMDF format */
#define MSG_SEPARATOR	"\001\001\001\001\n"

/*#define NO_COMMAS	/* Don't use commas in MTA envelope (address args) */
/*#define UUCP		/* Use strictly UUCP-style conventions */

/**************************************************************************
 *
 * Miscellaneous Stuff
 *
 **************************************************************************/

#define BSD		/* Needed for X11 header files */
#define SYSV		/* Needed for X11 header files */
#define USG		/* Needed for X11 header files */

#define AUX		/* Special set42sig() call needed */
#define HPUX		/* Needed for X11 header files */
#define MIPS		/* Needed for X11 header files */
#define MOTOROLA	/* Needed for X11 header files */
#define TOSHIBA		/* Permit stacktracing on Toshiba 386 (SCO UNIX) */
#define WYSE		/* Needed for X11 header files */
#define pyrSVR4		/* Needed for X11 header files */
