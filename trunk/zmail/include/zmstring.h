/* zmstring.h	Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * $Revision: 2.45 $
 * $Date: 1998/12/07 22:49:53 $
 * $Author: schaefer $
 */

#ifndef _ZMSTRING_H_
#define _ZMSTRING_H_

#ifndef OSCONFIG
#include <osconfig.h>
#endif /* OSCONFIG */

#include <stdio.h>
#include "zcstr.h"

#include <general.h>

/* Prototypes that used to be here; now moved down into other headers. */
#include "shell/zstrings.h"

extern int    stripq P(( char ** ));

/* return the last component of a file path */
#if !defined(__STDC__) && !defined(__cplusplus) && !defined(_OSF_SOURCE)
extern const char * basename P(( const char * ));
#endif  /* !__STDC__ && !__cplusplus && !_OSF_SOURCE */
extern int catv P(( int, char ***, int, char ** ));
/* string copy converting control chars to ascii */
extern char * ctrl_strcpy P(( char *, char *, int ));
extern void fmt_string P(( char *, char *, int, int, int, int ));
#if !defined (_WINDOWS) && !defined(MAC_OS)
/* return a string representation of a number */
extern char * itoa P(( int ));
#endif /* !_WINDOWS && !MAC_OS */
/* join a vector of strings into one string */
extern char * joinv P(( char *, char **, const char * ));
/* translate string from ascii to ctrl-char format */
extern char * m_xlate P(( char * ));
/* prints an argv as one string */
extern void print_argv P(( char ** ));
/* put a string */
extern void putstring P(( char *, FILE * ));
extern int re_glob P(( char *, char *, int ));
#ifndef savestr
/* strcpy arg into malloc-ed memory; return address */
extern char * savestr P(( const char * ));
#endif /* !savestr */
/* strcpy arg into malloc-ed memory; return address */
extern char * savestrn P(( const char * , int ));

/* append a string to a malloc-ed string */
extern char * strapp P(( char **, const char * ));
#ifndef OLI24
extern char * strcat P(( char *, const char * ));
#endif
#if !defined(GUI) && !defined(HAVE_STRING_H)
extern char * strncat P(( char *, const char *, int ));
extern char * strchr P(( const char *, const char ));
extern char * strrchr P(( const char *, const char ));
#endif
#if !defined(WIN16) && !defined(OLI24)
extern int strcmp P(( const char *, const char * ));
extern char * strcpy P(( char *, const char * ));
#endif
extern int Strcpy P(( char *, const char * ));
extern int strnumcmp P(( char **, char ** ));
#ifndef WIN16
extern int strptrcmp P(( const char **, const char ** ));
#else
extern short strptrcmp P(( char **, char ** ));
#endif /* !WIN16 */
/* convert a string into a vector of strings */
extern char ** strvec P(( const char *, const char *, int ));
/* convert a pointer into a one-element vector */
extern char ** unitp P(( const char * ));
/* convert a string into a one-element vector */
extern char ** unitv P(( const char * ));
extern int vcat P(( char ***, char ** ));
extern int vcatstr P(( char ***, const char * ));
extern int vcpy P(( char ***, char ** ));
extern int vins P(( char ***, char **, int ));
extern char ** vavec VP(( char *, ... ));
extern char ** vaptr VP(( char *, ... ));
extern char *unhidestr VP(( int a, ... ));
/* create an allocated copy of a vector */
extern char ** vdup P(( char ** ));
/* return position of a string within a vector */
extern char ** vindex P(( char **, char * ));
extern int vlen P(( char ** ));
/* convert a va_list of strings into a vector */
extern char ** vvavec P(( va_list ));
/* convert a va_list of pointers into a vector */
extern char ** vvaptr P(( va_list ));
extern char * str_replace P(( char **, const char * ));
extern int  vcmp P((char **, char **));

#endif /* _ZMSTRING_H_ */
