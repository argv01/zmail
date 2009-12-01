#include "popper.h"
#include <stdio.h>
#include <mstore/mfolder.h>
#include <mstore/mime-api.h>

static int
do_hash(p, keyp)
    POP *p;
    int keyp;
{
    struct number_list temp_list;
    int count, *numbers, *num_ptr;
    char buf[4 * MAILHASH_BYTES];

    if (parse_message_list(p, p->pop_parm[1], &temp_list))
	return (POP_FAILURE);
    count = temp_list.list_count;
    numbers = temp_list.list_numbers;
    for (num_ptr = numbers; num_ptr < (numbers + count); ++num_ptr) {
	if (keyp ?
	    NTHMSG(p, *num_ptr)->have_key_hash :
	    NTHMSG(p, *num_ptr)->have_header_hash)
	    continue;
	if (compute_extras(p, 1, num_ptr) != 0) {
	    free(numbers);
	    return (POP_FAILURE);
	}
    }

    /* Send the success message. */
    pop_msg(p, POP_SUCCESS, "%d %s", count,
	    count == 1 ? "message" : "messages");
    allow(p, count * 25);	/* roughly */
    for (num_ptr = numbers; num_ptr < (numbers + count); ++num_ptr) {
	fputs(mailhash_to_string(buf,
				 (keyp ?
				  &(NTHMSG(p, *num_ptr)->key_hash) :
				  &(NTHMSG(p, *num_ptr)->header_hash))),
	      p->output);
	fputs(mime_CRLF, p->output);
    }
    free(numbers);
    putc('.', p->output);
    fputs(mime_CRLF, p->output);
    fflush(p->output);
    return (POP_SUCCESS);
}

/* Send key hash codes for a given list of messages. */

int
zync_zmhk(p)
    POP *p;
{
    do_drop(p);

    return (do_hash(p, 1));
}


/* Send header hash codes for a given list of messages. */

int
zync_zmhq(p)
    POP *p;
{
    do_drop(p);

    return (do_hash(p, 0));
}

