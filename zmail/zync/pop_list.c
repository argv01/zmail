/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char copyright[] = 
"Copyright (c) 1990 Regents of the University of California.\n\
All rights reserved.\n";
static char SccsId[] = "@(#)@(#)pop_list.c	2.1  2.1 3/18/91";
#endif /* not lint */

#include "popper.h"
#include <stdio.h>
#include <sys/types.h>
#include <mstore/message.h>

int
pop_list(p)
    POP *p;
{
    struct msg_info *mp;
    register int i;
    register int msg_num;
    FILE *output_fp;

    do_drop(p);

    /* Was a message number provided? */
    if (p->parm_count > 0) {
	msg_num = atoi(p->pop_parm[1]);

	/* Is requested message out of range? */
	if ((msg_num < 1) || (msg_num > NUMMSGS(p))) {
	    pop_msg(p, POP_FAILURE, "Message %d does not exist.", msg_num);
	    return POP_FAILURE;
	}

	/* Get a pointer to the message in the message list */
	mp = NTHMSG(p, msg_num);

	/* Is the message already flagged for deletion? */
	if (mp->status & mmsg_status_DELETED) {
	    pop_msg(p, POP_FAILURE, "Message %d has been deleted.", msg_num);
	    return POP_FAILURE;
	}

	/* Display message information */
	pop_msg(p, POP_SUCCESS, "%u %u", msg_num,
		mp->header_length + mp->body_length);
	return POP_SUCCESS;
    }
    
    /* Display the entire list of messages */
    pop_msg(p, POP_SUCCESS, "%u messages (%u octets)",
	    NUMMSGS(p) - p->msgs_deleted,
	    p->drop_size - p->bytes_deleted);
    allow(p, (NUMMSGS(p) - p->msgs_deleted) * 15);

    /* Loop through the message information list.  Skip deleted messages */
    glist_FOREACH(&(p->minfo), struct msg_info, mp, i) {
	if (!(mp->status & mmsg_status_DELETED)) {
	    fprintf(p->output, "%u %u\r\n", mp->number,
		    mp->header_length + mp->body_length - mp->header_adjust);
	    if (p->debug & DEBUG_VERBOSE)
		pop_log(p, POP_DEBUG, "Sent: \"%u %u\"", mp->number,
			mp->header_length + mp->body_length - mp->header_adjust);
	}
    }

    /* "." signals the end of a multi-line transmission */
    output_fp = p->output;
    fputs(".\r\n", output_fp);
    if (p->debug & DEBUG_VERBOSE)
	pop_log(p, POP_DEBUG, "Sent: \".\"");
    fflush(output_fp);

    return POP_SUCCESS;
}
