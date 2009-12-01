/* zcerr.h	Copyright 1992 Z-Code Software Corp. */

#ifndef _ZCERR_H_
#define _ZCERR_H_

#ifndef ZC_INCLUDED_ERRNO_H
# define ZC_INCLUDED_ERRNO_H
# ifndef MAC_OS
#  include <errno.h>
# else /* MAC_OS */
#  include <sys/errno.h>
# endif /* !MAC_OS */
#endif /* ZC_INCLUDED_ERRNO_H */

#ifdef DECLARE_SYS_ERRLIST
extern char *sys_errlist[];
#endif /* DECLARE_SYS_ERRLIST */

#if !defined(HAVE_STRERROR)

/* Bart: Fri Dec 11 20:02:40 PST 1992
 * Not sure whether/why this next test is valid ...
 */
#if !defined(HAVE_STDLIB_H) || defined(pyrSVR4) || defined(SVR4)

extern char *sys_errlist[];    /* system's list of global error messages */

#endif /* !HAVE_STDLIB_H || SVR4 */

#if !defined( MSDOS ) && !defined(MAC_OS) && !defined(HAVE_STRERROR)
# ifndef strerror
#   define strerror(eno)	sys_errlist[(int)(eno)]
# endif /* strerror */
#endif /* !MSDOS && !MAC_OS && !HAVE_STRERROR */

#endif /* !HAVE_STRERROR */

#ifdef DECLARE_ERRNO
extern int errno;	/* global system error number */
#endif /* DECLARE_ERRNO */

#endif /* _ZCERR_H_ */
