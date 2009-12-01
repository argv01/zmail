/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "popper.h"

#include <stdio.h>
#include <sys/types.h>
#include <ctype.h>
#include <mstore/message.h>

static const char pop_dele_rcsid[] =
    "$Id: pop_dele.c,v 1.12 1995/07/17 18:34:22 bobg Exp $";

int
pop_dele (p)
    POP *p;
{
    struct msg_info *mp;
    int msg_num;

    do_drop(p);

    /*  Convert the message number parameter to an integer */
    msg_num = atoi(p->pop_parm[1]);

    /*  Is requested message out of range? */
    if ((msg_num < 1) || (msg_num > NUMMSGS(p))) {
	pop_msg(p, POP_FAILURE, "Message %d does not exist.", msg_num);
	return POP_FAILURE;
    }

    /*  Get a pointer to the message in the message list */
    mp = NTHMSG(p, msg_num);

    /*  Is the message already flagged for deletion? */
    if (mp->status & mmsg_status_DELETED) {
	pop_msg(p, POP_FAILURE, "Message %d has already been deleted.",
		msg_num);
	return POP_FAILURE;
    }
  
    /*  Flag the message for deletion */
    mp->status |= mmsg_status_DELETED;

    /*  Update the messages_deleted and bytes_deleted counters */
    (p->msgs_deleted)++;
    p->bytes_deleted += mp->header_length + mp->body_length;

    /*  Update the last-message-accessed number if it is lower than 
	the deleted message */
    if (p->last_msg < msg_num)
	p->last_msg = msg_num;

    pop_msg(p, POP_SUCCESS, "Message %d has been deleted.", msg_num);
    return POP_SUCCESS;
}
