#ifndef INCLUDE_ZYNC_H
#define INCLUDE_ZYNC_H


#include <general.h>	/* for P & NP macros */
#include "pop.h"

#ifdef __STDC__
struct dpipe;
#endif /* __STDC__ */


/*
 * ZHAV & ZGET utilities.  Note that the caller is responsible for
 * initiating and closing the server connection, as well as issuing
 * the actual ZHAV & ZGET commands.  These two functions only handle
 * generating and receiving the data portion of the commands,
 * respectively.
 */

void zync_describe_files P((PopServer, const char *));
void zync_update_files P((PopServer, const char *, void (*)NP((const char *))));

/*
 * Get the server's opinion of the user's real name, and set the
 * Z-Script $realname variable to the same.  Returns zero on success,
 * nonzero on failure.  May report SysErrWarnings itself if the
 * connection runs into difficulties.  This function initiates and
 * terminates its own server connection.
 */

void zync_realname P((const char *, const char *, const char *, int));
PopServer zync_open P((const char *host, const char *uname, const char *pword, unsigned long flags));
int zync_get_prefs P((PopServer ps));
int zync_set_prefs();

#endif /* !INCLUDE_ZYNC_H */
