/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "popper.h"
#include <stdio.h>
#include <sys/types.h>

static const char pop_last_rcsid[] =
    "$Id: pop_last.c,v 1.7 1995/07/10 22:59:21 bobg Exp $";

int
pop_last(p)
    POP *p;
{
    do_drop(p);

    pop_msg(p, POP_SUCCESS, "%u is the last message seen.",
	    p->last_msg);
    return POP_SUCCESS;
}

