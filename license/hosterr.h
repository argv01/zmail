/* hosterr.h	Copyright 1994, Z-Code Software Corp. */

#ifndef INCLUDE_LICENSE_HOSTERR_H
#define INCLUDE_LICENSE_HOSTERR_H


#include "osconfig.h"


#ifdef HAVE_H_ERRNO
/* nothing */
#else /* !HAVE_H_ERRNO */
#define h_errno (1)
#endif /* HAVE_H_ERRNO */

#ifdef HAVE_HERROR
/* nothing */
#else /* !HAVE_HERROR */
#include <stdio.h>
#define herror(prefix)  fprintf(stderr, "%s\n", (prefix) ? (prefix) : "")
#endif /* HAVE_HERROR */


#endif /* INCLUDE_LICENSE_HOSTERR_H */
