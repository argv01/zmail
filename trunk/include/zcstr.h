/* zcstr.h	Copyright 1992 Z-Code Software Corp. */

#ifndef _ZCSTR_H_
#define _ZCSTR_H_

#include "osconfig.h"
#include <general.h>

/* we don't need/want these macros if the actual functions are there. */
#if defined(HAVE_INDEX) && defined(index)
#undef index
#undef rindex
#endif /* HAVE_INDEX && index */

#ifdef HAVE_STRINGS_H
#ifndef ZC_INCLUDED_STRINGS_H
# define ZC_INCLUDED_STRINGS_H
# include <strings.h>
#endif /* ZC_INCLUDED_STRINGS_H */
#endif /* HAVE_STRINGS_H */

#ifdef HAVE_STRING_H

#ifndef ZC_INCLUDED_STRING_H
# define ZC_INCLUDED_STRING_H
# include <string.h>
#endif /* ZC_INCLUDED_STRING_H */

#endif /* HAVE_STRING_H */

#include <zctype.h>

#if defined(HAVE_STRCHR) && !defined(HAVE_INDEX) && !defined(index)
# define index  strchr
# define rindex strrchr
#endif /* HAVE_STRCHR && !HAVE_INDEX && !index */

#ifndef HAVE_PROTOTYPES

/* Bart: Fri Dec 11 20:12:46 PST 1992
 * THIS STUFF IS ALL EXTREMELY MESSY AND NEEDS CLEANING UP!	XXX
 */
/* hopefully better now...  pjf 05-22-93 */

#ifndef HAVE_STRCAT_STRCPY_DECLARED
/* External function definitions for routines described in string(3).  */
extern char *strcat  P((char *, const char *));
extern char *strncat P((char *, const char *, size_t));
extern char *strcpy  P((char *, const char *));
extern char *strncpy P((char *, const char *, size_t));
extern char *strstr  P((const char *, const char *));

# ifdef DECLARE_INDEX
extern char *index  P((const char *, int));
extern char *rindex P((const char *, int));
# endif /* DECLARE_INDEX */
#endif /* HAVE_STRCAT_STRCPY_DECLARED */

/* Bart: Thu Oct  8 15:04:22 PDT 1992
 * Potential problem here for VUI version on AIX: strcmp() is a macro!
 */
#ifndef HAVE_STRCMP_DECLARED
extern int strcmp  P((const char *, const char *));
extern int strncmp P((const char *, const char *, size_t));
#endif /* HAVE_STRCMP_DECLARED */

#ifndef HAVE_STRLEN_DECLARED
extern size_t strlen P((const char *));
#endif /* HAVE_STRLEN_DECLARED */

#endif /* !HAVE_PROTOTYPES */


#define ZSTRDUP(dst, src) (xfree (dst), dst = savestr(src))


#endif /* _ZCSTR_H_ */
