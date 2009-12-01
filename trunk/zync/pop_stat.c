/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "popper.h"
#include <stdio.h>
#include <sys/types.h>

static const char pop_stat_rcsid[] =
    "$Id: pop_stat.c,v 1.9 1995/07/17 21:27:13 bobg Exp $";

int
pop_stat(p)
    POP *p;
{
    do_drop(p);

    pop_msg(p, POP_SUCCESS, "%u %u",
	    NUMMSGS(p) - p->msgs_deleted,
	    p->drop_size - p->bytes_deleted);
    return POP_SUCCESS;
}
