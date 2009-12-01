/*
 * $RCSfile: excfns.h,v $
 * $Revision: 2.32 $
 * $Date: 1998/12/07 22:44:47 $
 * $Author: schaefer $
 */

#ifndef EXCFNS_H
#define EXCFNS_H

/*************************************************************\
 * Exception-raising interfaces to common libc routines      *
\*************************************************************/

#include <osconfig.h>

#include <except.h>
#include <stdio.h>

#include <general.h>

#include <zcerr.h>
#include <zctype.h>
#include <zctime.h>
#include <zcunix.h>
#include <zcfctl.h>
#include <zcsig.h>

#ifdef HAVE_READDIR
# if defined(DIRENT) || defined(_POSIX_VERSION)
#  include <dirent.h>
#  define NLENGTH(dirent) (strlen((dirent)->d_name))
# else				/* not (DIRENT or _POSIX_VERSION) */
#  define dirent direct
#  define NLENGTH(dirent) ((dirent)->d_namlen)
#  ifdef USG
#   ifdef SYSNDIR
#    include <sys/ndir.h>
#   else			/* not SYSNDIR */
#    include <ndir.h>
#   endif			/* not SYSNDIR */
#  else				/* not USG */
#   include <sys/dir.h>		/* Assume SYSDIR */
#  endif			/* not USG */
# endif				/* not (DIRENT or _POSIX_VERSION) */
#endif				/* HAVE_READDIR */

struct timeval;
struct timezone;
struct stat;

/* Each of these can raise "no mem" */
extern GENERIC_POINTER_TYPE *emalloc P((int, const char *));
extern GENERIC_POINTER_TYPE *erealloc P((GENERIC_POINTER_TYPE *,
					 int, const char *));
extern GENERIC_POINTER_TYPE *ecalloc P((int, int, const char *));
extern int egettimeofday P((struct timeval *, struct timezone *, const char *));
extern int eread P((int, char *, int, const char *));
extern int ewrite P((int, char *, int, const char *));
extern int efwrite P((char *, int, int, FILE *, const char *));
extern int efread P((char *, int, int, FILE *, const char *));
extern int efprintf (VA_PROTO(FILE *));
extern void efseek P((FILE *, long, int, const char *));
extern void elink P((const char *, const char *, const char *));
extern void efclose P((FILE *, const char *));
extern void ekill P((int, int, const char *));
extern void estat P((const char *, struct stat *, const char *));
extern void elstat P((const char *, struct stat *, const char *));
extern void emkdir P((const char *, int, const char *));
extern FILE *efopen P((const char *, const char *, const char *));

#if defined(HAVE_SELECT) || defined(HAVE_POLL)
extern int eselect P((int, fd_set *, fd_set *, fd_set *,
		      struct timeval *, const char *));
#endif /* HAVE_SELECT || HAVE_POLL */

#ifdef HAVE_READDIR
#ifdef SCO
#define eopendir zeopendir
#endif /* SCO */
extern DIR *eopendir P((const char *, const char *));
#endif /* HAVE_READDIR */

#ifdef HAVE_POPEN
extern FILE *epopen P((const char *, const char *, const char *));
#endif /* HAVE_POPEN */

#ifdef HAVE_PUTENV
extern void eputenv P((char *, const char *));
#endif /* HAVE_PUTENV */

extern int edup P((int, const char *));

#endif /* EXCFNS_H */
