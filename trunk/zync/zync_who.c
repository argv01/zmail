/*
 * Copyright (c) 1993-1998 NetManage, Inc.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1993-1998 NetManage, Inc.\n";
#endif /* not lint */

#include "popper.h"
#include <pwd.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define GECOS_DELIMITER (',')

int
zync_who(p)
    POP *p;
{
    struct passwd *entry;

    entry = getpwuid(geteuid());
    if (entry) {
	char *terminus = index(entry->pw_gecos, GECOS_DELIMITER);
	if (terminus) 
	    *terminus = 0;
	pop_msg(p, POP_SUCCESS, "%s", entry->pw_gecos);
	return POP_SUCCESS;
    } else {
	pop_msg(p, POP_FAILURE, "Unable to locate real name for %s (%d)", 
		p->user, geteuid());
	return POP_FAILURE;
    } 
}
