/*
 * Copyright (c) 1993-1998 NetManage, Inc.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1993-1998 NetManage, Inc.\n";
#endif /* not lint */

#include "popper.h"
#include <stdio.h>
#include <sys/types.h>

/* 
 *  init:  get z-script init file for z-mail to eval before updating
 */

int
zync_init(p)
    POP *p;
{
    char *initdir;
    char initname[MAXPATHLEN];

    if (!(initdir = (char *)getenv("TMPDIR")))
	initdir = DEFTMPDIR;
    sprintf(initname, "%s.initrc", p->user);
    switch (zync_get_remote_file(p, initdir, initname,
				 "Send init script.")) {
      case POP_SUCCESS:
	pop_msg(p, POP_SUCCESS, "Wrote init script file for %s.", p->user);
	return POP_SUCCESS;
      case POP_FAILURE:
	return POP_FAILURE;
      case POP_ABORT:
	return POP_ABORT;
    }
}
