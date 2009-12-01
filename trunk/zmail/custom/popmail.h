/* popmail.h	Copyright 1994 Z-Code Software Corp. */

/*
 * $Revision: 2.2 $
 * $Date: 1995/01/10 12:57:27 $
 * $Author: spencer $
 */

#ifndef _POPMAIL_H_
#define _POPMAIL_H_

/*
 * Header file for the "popmail.c" client POP3 
 * protocol implementation.
 */
#ifdef POP3_SUPPORT

#include "poplib.h"

/*
 * Function prototypes
 */
char *date P((char *message));
char *from_line P((char *message));

#ifndef HAVE_STRSTR
char *strstr P((const char *s, const char *wanted));
#endif /* !HAVE_STRSTR */

#endif /* POP3_SUPPORT */

#endif /* _POPMAIL_H_ */
