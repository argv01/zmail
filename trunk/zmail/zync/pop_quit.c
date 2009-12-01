/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "popper.h"
#include <stdio.h>
#include <sys/types.h>

static const char pop_quit_rcsid[] =
    "$Id: pop_quit.c,v 1.8 1995/08/26 21:46:55 spencer Exp $";

int
pop_quit(p)
    POP *p;
{
    char *t, tmp[255];

    /* 10/2/93  GF -- blind try to delete files.  OK if it fails */
    t = (char *)getenv("TMPDIR");
    if (!t) t=DEFTMPDIR;

    /* make sure we don't delete /tmp */
    if (p->user[0]) {
	/* note that this should have been consumed by z-mail */
	sprintf(tmp, "%s/%s", t, p->user);
	unlink(tmp);
	sprintf(tmp, "%s/%s.initrc", t, p->user);
	unlink(tmp);
	sprintf(tmp, "%s/%s.zmxxx", t, p->user);
	unlink(tmp);
	sprintf(tmp, "%s/%s.messages", t, p->user);
	unlink(tmp);
    }

    /*  Release the message information list */
    return POP_SUCCESS;
}

