/* zctime.h	Copyright 1992 Z-Code Software Corp. */

#ifndef _ZCTIME_H_
#define _ZCTIME_H_

#if !defined(MSDOS)

#ifndef OSCONFIG
#include <osconfig.h>
#endif /* OSCONFIG */

#include "zctype.h"

#ifdef TIME_WITH_SYS_TIME
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else /* HAVE_SYS_TIME_H */
#  ifdef HAVE_BSD_SYS_TIME
#   include "/usr/include/bsd/sys/time.h"
#  endif /* HAVE_BSD_SYS_TIME */
# endif /* HAVE_SYS_TIME_H */
# include <time.h>
#else /* TIME_WITH_SYS_TIME */
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else /* HAVE_SYS_TIME_H */
#  ifdef HAVE_BSD_SYS_TIME
#   include "/usr/include/bsd/sys/time.h"
#  endif /* HAVE_BSD_SYS_TIME */
#  include <time.h>
# endif /* HAVE_SYS_TIME_H */
#endif /* TIME_WITH_SYS_TIME */

/* Bart: Wed Jul 22 11:32:01 PDT 1992
 * This stuff may require <sys/types.h> which is included by "gui_def.h"
 * when MOTIF is defined -- so I had to move it here.
 */

#if defined(HAVE_SYS_UTIME_H) && !defined(HAVE_UTIMBUF)
# define HAVE_UTIMBUF
#endif /* HAVE_SYS_UTIME_H && !HAVE_UTIMBUF */

#ifdef HAVE_UTIMBUF
# ifdef HAVE_SYS_UTIME_H
#  ifndef ZC_INCLUDED_SYS_UTIME_H
#   define ZC_INCLUDED_SYS_UTIME_H
#   include <sys/utime.h>
#  endif /* ZC_INCLUDED_SYS_UTIME_H */
# else /* !HAVE_SYS_UTIME_H */
#  ifndef ZC_INCLUDED_UTIME_H
#   define ZC_INCLUDED_UTIME_H
#   ifdef HAVE_UTIME_H
#    include <utime.h>
#   endif /* HAVE_UTIME_H */
#  endif /* ZC_INCLUDED_UTIME_H */
# endif /* HAVE_SYS_UTIME_H */
#endif /* HAVE_UTIMBUF */

#endif /* MSDOS */

/* Bart: Mon Dec 14 12:03:54 PST 1992
 * This "lint" thing is silly, but ...
 */
#if defined(lint) || defined(DECLARE_TIME)
time_t time P((time_t *));	/* satisfy lint */
#endif /* lint || DECLARE_TIME */

#endif /* _ZCTIME_H_ */
