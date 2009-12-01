#ifndef INCLUDE_STRCASE_H
#define INCLUDE_STRCASE_H

/*
 * String and character case manipulators
 *
 * Copyright 1993, Z-Code Software Corporation.
 *
 */

#include "osconfig.h"
#include "zcstr.h"
#include "zctype.h"
#ifndef MAC_OS
# include <general/general.h>
#else
# include <general.h>
#endif /* MAC_OS */

#include <ctype.h>

extern int   ci_strcmp P((const char *, const char *));
extern int  ci_strncmp P((const char *, const char *, size_t));
extern int  ci_istrcmp P((const char *, const char *));
extern int ci_istrncmp P((const char *, const char *, size_t));

extern int ci_identcmp P((const char *, const char *));

extern char *  ci_strcpy P((register char *, register const char *));
extern char * ci_istrcpy P((register char *, register const char *));

extern int ci_stringHash P((const char *));


#define lower(byte)  ('A' <= (byte) && (byte) <= 'Z' ? (byte) |  0x20 : (byte))
#define upper(byte)  ('a' <= (byte) && (byte) <= 'z' ? (byte) & ~0x20 : (byte))
#define Lower(byte)  ((byte) = lower(byte))
#define Upper(byte)  ((byte) = upper(byte))

#ifdef SAFE_CASE_CHANGE
#define ilower(byte)  tolower(byte)
#define iupper(byte)  toupper(byte)
#else
#define ilower(byte)  (isupper(byte) ? tolower(byte) : (byte))
#define iupper(byte)  (islower(byte) ? toupper(byte) : (byte))
#endif /* SAFE_CASE_CHANGE */
#define iLower(byte)  ((byte) = ilower(byte))
#define iUower(byte)  ((byte) = iupper(byte))


#endif /* !INCLUDE_STRCASE_H */
