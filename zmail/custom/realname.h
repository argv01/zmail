/* realname.h	Copyright 1994 Z-Code Software Corp. */

/*
 * $Revision: 2.1 $
 * $Date: 1994/12/31 03:43:44 $
 * $Author: wilbur $
 */

#ifndef _REALNAME_H_
#define _REALNAME_H_

#include <general.h>

//
// function prototypes
//

/*
 * Get the server's opinion of the user's real name, and set the
 * Z-Script $realname variable to the same.  Returns zero on success,
 * nonzero on failure.  May report SysErrWarnings itself if the
 * connection runs into difficulties.  This function initiates and
 * terminates its own server connection.
 */
void zync_realname P((const char *host, const char *username, char *password, int flags));

#endif /* _REALNAME_H_ */
