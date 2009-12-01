/*
 * Exception-based wrappers around poplib routines
 */

#ifndef MAC_OS
# include <general/except.h>
#else 
# include <except.h>
#endif /* !MAC_OS */
#include "epop.h"
#include "zync.h"	/* for zync_open() */

#ifndef lint
static char	epoplib_rcsid[] =
    "$Id: epoplib.c,v 2.4 1995/03/06 01:12:56 fox Exp $";
#endif

PopServer
epop_open(host, username, password, flags)
     const char *host;
     const char *username;
     char *password;
     int flags;
{
  PopServer server = pop_open(host, username, password, flags);
  ASSERT(server, "pop", pop_error);
  return server;
}


PopServer
ezync_open(host, username, password, flags)
     const char *host;
     const char *username;
     char *password;
     int flags;
{
  PopServer server = zync_open(host, username, password, flags);
  ASSERT(server, "zync", pop_error);
  return server;
}


void
esendline(server, line)
     PopServer server;
     const char *line;
{
  ASSERT(!sendline(server, line), "pop", pop_error);
}


char *
egetline(server)
     PopServer server;
{
    char *ret = getline(server);
    ASSERT(ret, "pop", pop_error);
    return ret;
}
