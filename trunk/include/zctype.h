/* zctype.h	Copyright 1992 Z-Code Software Corp. */

#ifndef _ZCTYPE_H_
#define _ZCTYPE_H_

#if !defined(MSDOS)
# ifndef ZC_INCLUDED_SYS_TYPES_H
#  define ZC_INCLUDED_SYS_TYPES_H
#  ifdef PYR
#   define _POSIX_IMPLEMENTATION
#   undef _POSIX_SOURCE
#  endif /* PYR */
#  include <sys/types.h>
#  ifdef PYR
#   undef _POSIX_IMPLEMENTATION
#   define _POSIX_SOURCE
#  endif /* PYR */
#  include "zcsyssel.h" /* for sys/select.h where appropriate */
# endif /* ZC_INCLUDED_SYS_TYPES_H */
#endif /* !MSDOS */

#ifdef PYR
#include <ctype.h>
#endif /* PYR */

#ifndef MAC_OS

#if !defined(SPTX21) || defined(NEED_U_LONG)
#ifdef u_long
#undef u_long
#endif /* u_long */
#define u_long unsigned long
#endif /* !defined(SPTX21) || defined(NEED_U_LONG) */

#if !defined(SPTX21) || defined(NEED_U_SHORT)
#ifdef u_short
#undef u_short
#endif /* u_short */
#define u_short unsigned short
#endif /* !defined(SPTX21) || defined(NEED_U_SHORT) */

#endif /* !MAC_OS */

#ifdef QUAD_LONGS
#define Int32 unsigned int
#else /* !QUAD_LONGS */
#define Int32 unsigned long
#endif /* !QUAD_LONGS */

typedef int (*int_proc)();	/* Find better names for these! */
typedef void (*void_proc)();

#endif /* _ZCTYPE_H_ */
