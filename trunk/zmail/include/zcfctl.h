/* zcfctl.h	Copyright 1992 Z-Code Software Corp. */

/*
 * IOCTL and FCNTL stuff
 */

#ifndef _ZCFCTL_H_
#define _ZCFCTL_H_

#if !defined(MSDOS)

#include "zctype.h"
#include "zctime.h"

#ifndef SYSV
# ifndef MSDOS
#   ifndef ZC_INCLUDED_SYS_IOCTL_H
#    define ZC_INCLUDED_SYS_IOCTL_H
#    include <sys/ioctl.h>   /* for ltchars */
#   endif /* ZC_INCLUDED_SYS_IOCTL_H */
# endif /* MSDOS */
#endif /* !SYSV */

#include "zcsyssel.h" /* sys/select.h */

#ifdef HAVE_FCNTL_H
# ifndef ZC_INCLUDED_FCNTL_H
#  define ZC_INCLUDED_FCNTL_H
#  ifndef MAC_OS
#   include <fcntl.h>
#  else /* MAC_OS */
#   include <sys/fcntl.h>
#  endif /* !MAC_OS */
# endif /* ZC_INCLUDED_FCNTL_H */
#endif /* HAVE_FCNTL_H */

#endif /* MSDOS */

#endif /* _ZCFCTL_H_ */
