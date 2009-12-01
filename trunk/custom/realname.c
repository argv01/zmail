#define _POSIX_SOURCE
#ifdef __sgi
#define _BSD_TYPES	/* for uint typedef */
#endif

#include "zmail.h"	/* for set_var() */
#include <string.h>
#include "catalog.h"
#include "epop.h"
#include "except.h"
#include "pop.h"
#include "zm_ask.h"
#include "zync.h"
#include "mailserv.h"

#define WHO_COMMAND "ZWHO"
#define OK_PREFIX  "+OK "
#define OK_LENGTH  4
#define WHITESPACE " \t"


/*
 * Get the server's opinion of the user's real name, and set the
 * Z-Script $realname variable to the same.
 *
 * Errors are reported by throwing or propagating the following
 * exceptions:
 *
 *  "pop"      -- exception data is a localized string containing
 *		  poplib's description of the failure
 *
 *  "z-script" -- exception data is a localized string that identifies
 *		  the point of failure as a failed attempt to set the
 *		  $realname variable
 */

void
zync_realname(host, username, password, flags)
     const char *host;
     const char *username;
     const char *password;
     int flags;
{
  PopServer server = zync_open(host, username, password, flags);
  char *response;
  int bad = FALSE;

  if (!server)
      return;
  if (sendline(server, WHO_COMMAND) || !(response = getline(server)))
      bad = TRUE;
  else if (bad = strncmp(response, OK_PREFIX, OK_LENGTH))
  	pop_close(server);
  ASSERT(!bad, "pop", pop_error);

  /* GF 1/14/94  why this?  if zync returns "+OK Greg Fox", it'll become "Fox" */
#if NOT_NOW
  for (response += OK_LENGTH; strchr(WHITESPACE, *response); response++)
    ;
#endif

  errno = 0;
  response += OK_LENGTH;
  if (*response) {
      bad = set_var(VarRealname, "=", response);
      if (!bad)
	  var_mark_readonly(VarRealname);
  }
  pop_close(server);
  ASSERT(!bad, "z-script", "unable to set $realname");
}
