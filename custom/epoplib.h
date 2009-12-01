/* epoplib.h	Copyright 1994 Z-Code Software Corp. */

/*
 * $Revision: 2.1 $
 * $Date: 1994/12/31 03:32:04 $
 * $Author: wilbur $
 */

#ifndef _EPOPLIB_H_
#define _EPOPLIB_H_

#include <general.h>	/* for P() macro */
#include "poplib.h"

//
// function prototypes
//

PopServer epop_open P((const char *host, const char *username, char *password, int flags));
PopServer ezync_open P((const char *host, const char *username, char *password, int flags));
void esendline P((PopServer server, const char *line));
char *egetline P((PopServer server));

#endif /* _EPOPLIB_H_ */
