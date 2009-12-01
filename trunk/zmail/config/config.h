/* config.h	Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * $Revision: 2.17 $
 * $Date: 1998/12/07 22:40:25 $
 * $Author: schaefer $
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

/*
 * Angle brackets in the #includes below cause the directories in the search
 * path to be checked before the directory in which the current file resides.
 * This allows config/config.h to include ./features.h (for example)(.
 */

#ifndef OSCONFIG
#include <osconfig.h>	/* Should be a link to config/os-something.h */
#endif /* OSCONFIG */

#ifdef OSLOCAL
#include <oslocal.h>	/* May be defined for other local configuration */
#undef OSLOCAL
#endif /* OSLOCAL */

#include "config/features.h"  /* Uses Motif defaults from config/ if none in ./ */

#if !defined(MAC_OS) && !defined(MSDOS) && !defined(UNIX) && !defined(_WINDOWS)
#define UNIX
#endif /* !MAC_OS && !MSDOS && !UNIX && !_WINDOWS */

#ifdef MAC_OS
#include <defaults.h>
#else /* In case there is another defaults.h, make sure we get config/ ... */
#ifdef _WINDOWS             
#include "defaults.h"
#else
#include "config/defaults.h"
#endif
#endif /* MAC_OS */

/*
 * Define TIMEZONE if your system has neither the SysV external variable
 * tzname nor the BSD timezone() function.  The example below is for
 * Gould BSD4.3 systems; others should define it as a string, e.g. "PST"
 * If TIMEZONE is defined, DAYLITETZ can also be defined, e.g. "PDT"
 *
 * Define USA if you are using US/NorthAmerican time zone abbreviations.
 * If USA is not defined, dates in outgoing mail will include timezones
 * specified as offsets from GMT, e.g. Pacific Standard Time is -0800.
 */
/* #define TIMEZONE T->tm_zone	/**/
/* #define USA /**/

/*
 * Some systems have regcmp/regex as their regular expression matching
 * routines while others have re_comp/re_exec -- If you have regcmp,
 * you should define REGCMP so that you will use the routines regcmp()
 * and regex() as the regular expression composer/parser.  REGCMP should
 * normally be defined for xenix and System-V Unix.  If you don't have
 * REGCMP defined, then the routines re_comp() and re_exec() are used.
 *
 * Note that some systems do not have either set of routines in the default
 * libraries.  You must find the library to use and add it to the list of
 * local libraries in the makefile.  Read your manual for regex(3).
 */
#if defined(HAVE_REGCMP) && !defined(REGEXPR)
#define REGCMP
#endif /* HAVE_REGCMP && !REGEXPR */

/*
 * Z-Mail works best when it is able to use select() to implement macros,
 * mappings and bindings.  If your system is a BSD system, this is used
 * automatically.  However, with the advent of hybrid bsd/sys-v systems, you
 * may not be able to set BSD, but still have select() --for such systems,
 * define SELECT here.  Note that this may require additional libraries,
 * check the list of local libraries in the makefile.
 */
/* #define SELECT	/**/
#ifdef HAVE_SELECT
#define SELECT
#endif /* HAVE_SELECT */

/*
 * If your system supports the vprintf() functions, True for sys-v and
 * later sun versions (3.0+ ?).  Typically not true for BSD systems, but
 * that will probably change in the future.
 */
#if defined(SYSV) || defined(sun) || defined(apollo) || defined(ultrix) || defined(MSDOS) || defined(HAVE_VPRINTF)
#define VPRINTF
#endif /* HAVE_VPRINTF */

/* Mailer specification sanity */

#ifdef HAVE_BINMAIL
#define BINMAIL /* This should eventually be _replaced_ by HAVE_BINMAIL */
#endif /* HAVE_BINMAIL */

#if defined(M_UNIX) && !defined(HAVE_SENDMAIL)
#define MMDF
#endif /* M_UNIX && !HAVE_SENDMAIL */

/* If your mailer does not understand commas between addresses, you should
 * define NO_COMMAS.  This includes pre-3.0 smail and default MTAs used on
 * xenix, and sys-v systems.
 * This does NOT apply to MMDF or sendmail, in most cases, but SunOS 4.1
 * has a warped sendmail.cf.
 */
#if defined(BINMAIL) || defined(SUN_4_1)
#define NO_COMMAS	/**/
#endif /* BINMAIL || SUN_4_1 */

/* mail delivery system macros and defines... */

#ifdef MMDF
/*
 * This value should be identical to the contents of delim1 and delim2
 * in MMDFSRC/conf/yoursite/conf.c (sans newline).
 * If delim1 and delim2 are different, reinstall MMDF. :-(
 */
#undef  MSG_SEPARATOR
#define MSG_SEPARATOR	"\001\001\001\001\n"
/*
 * If MMDF delivers mail the user's home directory, define HOMEMAIL.
 * Also check the definition of the delivery file name MAILFILE, below.
 */
#ifdef USE_HOMEMAIL
#define HOMEMAIL
#endif /* USE_HOMEMAIL */
#if defined(M_UNIX) || defined(HAVE_EXECMAIL)
/* SCO uses a mix of MMDF and older stuff */
#define MAIL_DELIVERY	"/usr/lib/mail/execmail"
#define VERBOSE_ARG	"-v"    /* undef if none exists */
#define METOO_ARG	"-m"    /* man sendmail for more info. */
#define MTA_EXIT	0	/* exit status for successful mail delivery */
#else /* M_UNIX || !HAVE_EXECMAIL */
#ifdef HAVE_SUBMIT
#define MAIL_DELIVERY	"exec /usr/mmdf/bin/submit -mlnr"
#define VERBOSE_ARG	"Ww"
#define MTA_EXIT	9	/* exit status for successful submit */
#else /* !HAVE_SUBMIT */
#define BINMAIL
#define MAIL_DELIVERY	"/bin/mail"
#endif /* HAVE_SUBMIT */
#endif /* M_UNIX */
#else /* MMDF */
/*
 * If you are not using MMDF, check these definitions.
 */
#ifdef BINMAIL
#define MAIL_DELIVERY	"/bin/mail"
#else /* !BINMAIL */
#if defined(DVX)
#define MAIL_DELIVERY   "/usr/lib/sendmail.exe"
#define VERBOSE_ARG     "-v"    /* undef if none exists */
#else /* !DVX */
#ifndef MAIL_DELIVERY
# define MAIL_DELIVERY	"/usr/lib/sendmail -i" /* "-i" works like "-oi" */
#endif /* MAIL_DELIVERY */
#define VERBOSE_ARG	"-v"    /* undef if none exists */
#define METOO_ARG	"-m"    /* man sendmail for more info. */
#endif /* DVX */
#endif /* BINMAIL */
#define MTA_EXIT	0	/* exit status for successful mail delivery */
#endif /* MMDF */

#ifdef HOMEMAIL
#ifndef MAILFILE
#define MAILFILE	"Mailbox"	/* or whatever */
#endif /* MAILFILE */
#else /* HOMEMAIL */
# ifndef _WINDOWS
#  ifndef MAILDIR
#   ifdef USE_USR_MAIL
#    define MAILDIR		"/usr/mail"
#   else /* assume USE_SPOOL_MAIL */
#    define MAILDIR		"/usr/spool/mail"
#   endif /* USE_USR_MAIL */
#  endif /* MAILDIR */
# else
#  define MAILDIR       "\\ZMAIL"
# endif /* _WINDOWS */
#endif /* HOMEMAIL */

/*
 * The remainder of this file attempts to intuit default definitions in
 * case certain important ones were missed.  It should not be necessary
 * to edit anything below this point.
 */

#if defined(SUNTOOL) || defined(SUN_3_5) || defined(SUN_4_0) || defined(SUN_4_1)
#if !defined(BSD) && !defined(SYSV)
#    define BSD /* default to BSD */
#endif /* !BSD && !SYSV */
#if !defined(SUN_3_5) && !defined(SUN_4_0)
#    define SUN_4_0 /* default to Sun 4.0, needed for Sun 4.1 as well */
#endif /* !SUN_3_5 && !SUN_4_0 */
#ifdef SUN_4_0
#    undef SUN_3_5
#    undef SIGRET
#    define SIGRET void
#endif /* SUN_4_0 */
#endif /* SUNTOOL || SUN_3_5 || SUN_4_0 */

#if defined(SYSV) && !defined(NO_VFORK)
#define NO_VFORK
#endif /* SYSV && !NO_VFORK */

#ifdef NO_VFORK
#undef HAVE_VFORK		/* Backwards compat */
#endif /* NO_VFORK */

#ifdef NEW_CHILD_STUFF
#define ZM_CHILD_MANAGER	/* Backwards compat */
#endif /* NEW_CHILD_STUFF */

/* Apple uses AUX_SOURCE as a standard definition */
#if defined(AUX_SOURCE) && !defined(AUX)
#define AUX
#endif /* AUX_SOURCE && !AUX */
#if defined(AUX) && !defined(DIRECTORY)
#define DIRECTORY
#endif /* AUX && !DIRECTORY */

/* What release of X11 are we using, if any? */
#if defined(MOTIF)
# include <X11/Intrinsic.h>
# ifndef XtSpecificationRelease
#  define XtSpecificationRelease 0
# endif /* !XtSpecificationRelease */
# include <X11/Xlib.h>
# ifndef XlibSpecificationRelease
#  define XlibSpecificationRelease 0
# endif /* !XlibSpecificationRelease */
#endif /* MOTIF */

#endif /* _CONFIG_H_ */
