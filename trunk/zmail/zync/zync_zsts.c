/* 
 * $RCSfile: zync_zsts.c,v $
 * $Revision: 1.8 $
 * $Date: 1995/08/14 17:52:07 $
 * $Author: bobg $
 */

#include "popper.h"
#include <except.h>
#include <mstore/mime-api.h>

static const char zync_zsts_rcsid[] =
    "$Id: zync_zsts.c,v 1.8 1995/08/14 17:52:07 bobg Exp $";

/* ZSTS
 * gives the status of a message
 */
int
zync_zsts(p)
    POP *p;
{
    int num = atoi(p->pop_parm[1]);

    do_drop(p);

    pop_msg(p, POP_SUCCESS, "%lu", NTHMSG(p, num)->status);
    return (POP_SUCCESS);
}

int
zync_zst2(p)
    POP *p;
{
    struct number_list temp_list;
    int *num_ptr;
    size_t sent = 0;

    do_drop(p);

    if (parse_message_list(p, p->pop_parm[1], &temp_list))
	return (POP_FAILURE);
    pop_msg(p, POP_SUCCESS, "%d %s", temp_list.list_count,
	    (temp_list.list_count == 1) ? "message" : "messages");
    for (num_ptr = temp_list.list_numbers;
	 num_ptr < (temp_list.list_numbers + temp_list.list_count);
	 ++num_ptr) {
	fprintf(p->output, "%d %lu", *num_ptr, NTHMSG(p, *num_ptr)->status);
	fputs(mime_CRLF, p->output);
	sent += 15;		/* roughly */
    }
    fputc('.', p->output);
    fputs(mime_CRLF, p->output);
    fflush(p->output);
    allow(p, sent + 20);	/* roughly */
    return (POP_SUCCESS);
}
