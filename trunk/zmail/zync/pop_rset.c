/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "popper.h"
#include <stdio.h>
#include <sys/types.h>
#include <mstore/message.h>

static const char pop_rset_rcsid[] =
    "$Id: pop_rset.c,v 1.12 1995/07/17 18:34:27 bobg Exp $";

int
pop_rset(p)
    POP *p;
{
    struct msg_info **mpp;
    struct msg_info *mp;
    register int i;

    do_drop(p);

    /*  Unmark all the messages */
    glist_FOREACH(&(p->minfo), struct msg_info, mp, i) {
	mp->status &= ~mmsg_status_DELETED;
	if ((i + 1) > p->orig_last_msg)
	    mp->status |= mmsg_status_UNREAD;
    }
    
    /*  Reset the messages-deleted and bytes-deleted counters */
    p->msgs_deleted = 0;
    p->bytes_deleted = 0;
   
    /*  Reset the last-message-access flag */
    p->last_msg = 0;

    pop_msg(p, POP_SUCCESS, "Maildrop has %u messages (%u octets)",
	    NUMMSGS(p), p->drop_size);
    return POP_SUCCESS;
}


