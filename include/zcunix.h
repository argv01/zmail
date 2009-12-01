/* zcunix.h	Copyright 1992, 1994 Z-Code Software Corp. */

#ifndef _ZCUNIX_H_
#define _ZCUNIX_H_

/*
 * $RCSfile: zcunix.h,v $
 * $Revision: 2.32 $
 * $Date: 1995/10/26 20:54:43 $
 * $Author: liblit $
 */

#ifndef OSCONFIG
#include <osconfig.h>
#endif /* OSCONFIG */
#include <general.h>

#ifdef HAVE_UNISTD_H
#ifndef ZC_INCLUDED_UNISTD_H
# define ZC_INCLUDED_UNISTD_H
# include <unistd.h>		/* Is this POSIX? */
#endif /* ZC_INCLUDED_UNISTD_H */
#endif /* HAVE_UNISTD_H */

#ifndef ZC_INCLUDED_STDIO_H
# define ZC_INCLUDED_STDIO_H
# include <stdio.h>
#endif /* ZC_INCLUDED_STDIO_H */

#if defined(_IOFBF) && !defined(PYR)
# define FSETBUF(F__, B__, S__) setvbuf(F__, B__, _IOFBF, S__)
#else /* !(_IOFBF && !PYR) */
#ifdef BSD
# define FSETBUF(F__, B__, S__) setbuffer(F__, B__, S__)
#else /* !BSD */
# undef BETTER_BUFSIZ				/* Can't change it anyway */
# define FSETBUF(F__, B__, S__) setbuf(F__, B__)	/* Just in case */
#endif /* !BSD */
#endif /* !(_IOFBF && !PYR) */

#ifndef MAC_OS
# ifdef HAVE_STDLIB_H
#  ifndef ZC_INCLUDED_STDLIB_H
#   define ZC_INCLUDED_STDLIB_H
#   include <stdlib.h>
#  endif /* ZC_INCLUDED_STDLIB_H */
# endif /* HAVE_STDLIB_H */
#endif /* !MAC_OS */

/*
 * If not using new child stuff, we deal with systems without vfork
 * by simply #defining vfork to be fork.
 *
 * If using ZM_CHILD_MANAGER, the child module would like to
 * know which it's using, so we let it handle this issue itself.
 *
 * ZM_CHILD_MANAGER should become ZC_CHILD_MANAGER		XXX
 */
#if !defined(HAVE_VFORK) && !defined(ZM_CHILD_MANAGER)
#define vfork fork
#endif /* !HAVE_VFORK && !ZM_CHILD_MANAGER */

#include "zctype.h"

#ifndef ZC_INCLUDED_SYS_STAT_H
# define ZC_INCLUDED_SYS_STAT_H
# ifdef PYR
#  define _POSIX_IMPLEMENTATION
#  undef _POSIX_SOURCE
# endif /* PYR */
# include <sys/stat.h>
# ifdef PYR
#  undef _POSIX_IMPLEMENTATION
#  define _POSIX_SOURCE
# endif /* PYR */
#endif /* ZC_INCLUDED_SYS_STAT_H */

#if !(defined(MSDOS) || defined(MAC_OS)) && (!defined(GUI) || !defined(SYSV))
#ifndef ZC_INCLUDED_SYS_FILE_H
# define ZC_INCLUDED_SYS_FILE_H
# include <sys/file.h>
#endif /* ZC_INCLUDED_SYS_FILE_H */
#endif /* !(MSDOS || MAC_OS) && (!GUI || !SYSV) */

#if defined(__hpux) || defined(__sgi)
# ifdef MIN
#  undef MIN
#  undef MAX
# endif
#ifndef ZC_INCLUDED_SYS_PARAM_H
# define ZC_INCLUDED_SYS_PARAM_H
#  include <sys/param.h>
#endif /* ZC_INCLUDED_SYS_PARAM_H */
#endif /* __hpux || __sgi */

#ifdef _OSF_SOURCE
#include <sys/time.h>
#endif /* _OSF_SOURCE */

#ifndef L_SET
#define L_SET	0
#define L_INCR	1
#define L_XTND	2
#endif /* L_SET */

#ifndef F_OK
#define F_OK	000
#define R_OK	004
#define W_OK	002
#define E_OK	001
#endif /* F_OK */

/* There is probably a sysconf() for this, too */
#if !defined(MAXPATHLEN) && !defined(__hpux)
#define MAXPATHLEN BUFSIZ
#endif /* MAXPATHLEN && !__hpux */

#ifndef Ctrl
# define Ctrl(c) ((c) & 037)
#endif /* Ctrl */

#define ESC 		'\033'

#if defined(NULL) && !defined(__STDC__) && !defined(_WINDOWS)
#undef  NULL
/* Bart: Fri Dec 11 18:38:49 PST 1992 -- This is disgusting and must change. */
#define NULL		(VPTR)0
#endif /* NULL that we don't trust */

#define SNGL_NULL	(char *)0
#define DUBL_NULL	(char **)0
#define TRPL_NULL	(char ***)0

#ifdef WIN16
#define NULL_FILE	(FILE *)0L
#else
#define NULL_FILE	(FILE *)0
#endif

#undef putchar
#define putchar(c)	(fputc(c, stdout), fflush(stdout))

#ifdef BSD
# ifndef NOT_ZMAIL
#define fputs Fputs	/* See comments in print.c */
# endif /* NOT_ZMAIL */
int Fputs P((const char *, FILE *));
#endif /* BSD */

/* This should associate with some kind of library		XXX */
#ifdef apollo		/* See custom/apollo_file.c */
extern FILE *apollo_lkfopen P((const char *, const char *));
#define fopen(f,m)	apollo_lkfopen(f,m)
#define open(p,f,m)	apollo_lkopen(p,f,m)
#define flock(d,o)	apollo_flock(d,o)
#endif /* apollo */

#if defined(HAVE_GETWD)
extern char *getwd P((char *));
#define GetCwd(buf,len)	(errno = 0, getwd(buf))
#else
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
# if !defined(_WINDOWS) && !defined(MAC_OS)
extern char *getcwd P((char *, size_t));
# endif /* !_WINDOWS && !MAC_OS */
#endif /* HAVE_UNISTD_H */
#ifdef MSDOS
extern char *dos_copy_path (char *, const char *);
extern char *GetCwd (char *, int);
#else /* !MSDOS */
#define GetCwd(buf,len)	(errno = 0, getcwd(buf,len))
#endif /* MSDOS */
#endif /* HAVE_GETWD */

/* pjf 05-24-93 */
#ifdef DECLARE_ENVIRON
extern char **environ;		/* user's environment variables */
#endif /* DECLARE_ENVIRON */
#ifdef HAVE_GETENV
extern char *getenv P((const char *));
#endif

/*
 * API to check to see if there is enough to room to write a bunch of
 * data out to various files on the disk.
 */
struct os_check_free_space_struct {
    const char *filename;
    unsigned long size;
    unsigned long flags; /* overwrite, etc. */
};

typedef struct os_check_free_space_struct os_check_free_space_t;
#define OS_CFS_OVERWRITE    ULBIT(0)

#ifdef _WINDOWS
extern int os_check_free_space P((os_check_free_space_t *files, int count));
#else /* !_WINDOWS */
#define os_check_free_space(X, Y) 1     /* sure, PLENTY of room! */
#endif /* !_WINDOWS */

#endif /* _ZCUNIX_H_ */
