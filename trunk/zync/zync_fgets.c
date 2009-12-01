/*
 * Copyright (c) 1993-1998 NetManage, Inc. 
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1993-1998 NetManage, Inc.\n";
#endif /* not lint */

#include <stdio.h>
#include <errno.h>

#include "popper.h"

void
allow(p, nbytes)
    POP *p;
    size_t nbytes;
{
    if (p->speed < 0) {
	char *value;

	if (!(p->client_props))
	    return;
	value = plist_Get(p->client_props, "speed");

	if (value)
	    if ((p->speed = atoi(value)) <= 0)
		p->speed = -1;
    }
    if (p->speed > 0) {
	nbytes *= 10;		/* bytes * (~bits/byte) = bits */
	nbytes /= p->speed;	/* bits / (bits/second) = seconds */
	p->additional_timeout += nbytes;
    }
}

/* Like fgets, but has timeout and spits out error messages */
/* in a Zync-ish fashion. */

char *
zync_fgets(s, n, p)
    char *s;
    int n;
    POP *p;
{
    struct timeval idle_timeval;
    int total_timeout;
    int select_rslt;
    int indx = 0;
    int done = FALSE;
    int old_errno;
    int c;

    if (feof(p->input)) {
	pop_msg(p, POP_FAILURE, "EOF on input stream.");
	return NULL;
    }

    while (indx<MAXLINELEN-1) {

	/* Wait for chars available or timeout. */
	FD_ZERO(&p->input_fdset);
	FD_SET(fileno(p->input), &p->input_fdset);
        if (p->idle_timeout) {
	    total_timeout = p->idle_timeout + p->additional_timeout;
	    idle_timeval.tv_sec = total_timeout;
	    idle_timeval.tv_usec = 0;
        }
	p->additional_timeout = 0;
	select_rslt = select(FD_SETSIZE, &p->input_fdset, NULL, NULL,
			     p->idle_timeout ? &idle_timeval : (struct timeval *)NULL);
	if (!select_rslt) {
	    pop_msg(p, POP_FAILURE,
		    "Server has timed out after %d seconds of inactivity.",
		    total_timeout);
	    return NULL;

	    /* Shouldn't usually happen, but may if, say, the client gets
	       backgrounded and we get a signal, etc. */
	} else if (select_rslt == -1) {
	    if (p->debug & DEBUG_WARNINGS)
		pop_log(p, POP_PRIORITY, 
			"Warning: error on select() on input stream: %s", 
			strerror(errno));
    
	    /* Chars are available, so get one... */
	} else if ((c = getc(p->input)) == EOF) {
	    old_errno = errno;
	    if (!feof(p->input))
		break;
	    else {
		pop_msg(p, POP_FAILURE, "Error reading input stream: %s", 
			strerror(old_errno));
		return NULL;
	    }

	    /* Stow it. */
	} else {
	    s[indx++] = c;

	    /* If it was a newline, terminate. */
	    if (c == '\n')
		break;
	}
    }

    /* Tie it off. */
    s[indx]='\0';
    return s;
}
