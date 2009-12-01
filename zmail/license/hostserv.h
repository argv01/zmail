/*
 * hostserv.h
 *
 * Header file for hostserv.c.
 */

/* Copyright 1992 Z-Code Software Corp. */

#ifndef _HOSTSERV_H_
#define _HOSTSERV_H_

#include "osconfig.h"

/* some people just won't learn */
#if !defined(HOST_NOT_FOUND) && defined(HAVE_NETDB_H)
#include <netdb.h>
#endif /* !HOST_NOT_FOUND && HAVE_NETDB_H */


struct hostserv {
    int allocated;	/* boolean */
    struct hostent *host;
    struct servent *serv;
};
extern struct hostserv *hostserv_getbyname(/* char *name,
					      int host_required,
					      int serv_required,
					      int allocate */);
extern void hostserv_destroy(/* hostserv */);
extern struct hostserv *hostserv_build(/* hostent, servent */);
extern struct hostserv *hostserv_dup(/* hostserv */);
extern char *hostserv_errstr;	/* valid if any of the above routines fail */

/*
 * Possible values for the arguments to hostserv_getbyname.
 * The arguments are really just boolean, but this
 * makes it more obvious what the caller is doing...
 */
#define HS_HOST_REQUIRED	1
#define HS_HOST_NOT_REQUIRED	0

#define HS_SERV_REQUIRED	1
#define HS_SERV_NOT_REQUIRED	0

#define HS_ALLOCATE		1
#define HS_STATIC		0

#endif /* !_HOSTSERV_H_ */




