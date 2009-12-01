/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <general/dputil.h>
#include "popper.h"
#include <stdio.h>
#include <sys/types.h>
#include <general/dpipe.h>
#include <general/glist.h>
#include <general/dynstr.h>
#include <mstore/mime-api.h>
#include <mstore/message.h>
#include <general/strcase.h>

static const char pop_send_rcsid[] =
    "$Id: pop_send.c,v 1.29 1996/04/22 23:43:53 spencer Exp $";

extern int is_from_line P((char *)); /* in pop_dropinfo.c */

int
pop_send(p)
    POP *p;
{
    int mnum = atoi(p->pop_parm[1]);
    int saw_xkd = 0;
    int mlines, is_zrtr;
    struct msg_info *minfo;
    struct glist hlist;
    struct dpipe dp;
    int retval = POP_SUCCESS;
    const char *newline;

    do_drop(p);

    if ((mnum <= 0) || (mnum > NUMMSGS(p))) {
	pop_msg(p, POP_FAILURE, "Message %d does not exist", mnum);
	return (POP_FAILURE);
    }
    minfo = NTHMSG(p, mnum);

    if (minfo->status & mmsg_status_DELETED) {
	pop_msg(p, POP_FAILURE, "Message %d has been deleted", mnum);
	return (POP_FAILURE);
    }
    if (!ci_strcmp(p->pop_command, "top")) {
	mlines = atoi(p->pop_parm[2]);
	is_zrtr = 0;
    } else {
	mlines = -1;
	is_zrtr = !ci_strcmp(p->pop_command, "zrtr");
	if (!is_zrtr) {
	    minfo->status &= ~mmsg_status_NEW;
	    minfo->status &= ~mmsg_status_UNREAD;
	}
    }

    efseek(p->drop, minfo->header_offset, SEEK_SET, "pop_send");

    pop_msg(p, POP_SUCCESS, "%d octets",
	    minfo->header_length + minfo->body_length - minfo->header_adjust);
    allow(p, minfo->header_length + minfo->body_length);

    dpipe_Init(&dp, 0, 0, dputil_FILEtoDpipe, p->drop, 0);
    glist_Init(&hlist, (sizeof (struct mime_pair)), 8);

    TRY {
	if (less_than_mime_Headers(&dp, &hlist, mime_CRLF)) {
	    off_t content_length = 0;
	    struct mime_pair *mp;
	    struct dynstr d;
	    int i;

	    dynstr_Init(&d);
	    TRY {
		glist_FOREACH(&hlist, struct mime_pair, mp, i) {
		    if ((p->spool_format & CONTENT_LENGTH)
			&& !ci_strcmp("content-length",
				      dynstr_Str(&(mp->name)))) {
			if (content_length <= 0)
			    content_length = atol(dynstr_Str(&(mp->value)));
			fputs(dynstr_Str(&(mp->name)), p->output);
			fputc(':', p->output);
			fputs(dynstr_Str(&(mp->value)), p->output);
		    } else if (!ci_strcmp("x-key-digest",
					  dynstr_Str(&(mp->name)))) {
			char *paren;

			if (!saw_xkd
			    && (!(paren = index(dynstr_Str(&(mp->name)), '('))
				|| (atoi(paren + 1) == glist_Length(&hlist))
				|| minfo->have_key_hash)) {
			    fputs("X-Key-Digest: ", p->output);
			    if (minfo->have_key_hash) {
				fputs(mailhash_to_string(0,
							 &(minfo->key_hash)),
				      p->output);
				fprintf(p->output, " (%d)%s",
					glist_Length(&hlist), mime_CRLF);
			    } else {
				fputs(dynstr_Str(&(mp->value)), p->output);
			    }
			    saw_xkd = 1;
			}
		    } else {
			fputs(dynstr_Str(&(mp->name)), p->output);
			fputc(':', p->output);
			fputs(dynstr_Str(&(mp->value)), p->output);
		    }
		}
		if (!saw_xkd && minfo->have_key_hash) {
		    fputs("X-Key-Digest: ", p->output);
		    fputs(mailhash_to_string(0, &(minfo->key_hash)),
			  p->output);
		    fprintf(p->output, " (%d)%s",
			    glist_Length(&hlist) + 1, mime_CRLF);
		}

		/* Skip the newline following the headers */
		mime_Readline(&dp, 0);
		fputs(mime_CRLF, p->output);

		/* XXX why are we bothering with content_length?  ought to
		   just use minfo->body_length */
		while (mlines && (content_length > 0)) {
		    if (dpipe_Eof(&dp))
			break;
		    dynstr_Set(&d, 0);
		    newline = mime_Readline(&dp, &d);
		    content_length -= dynstr_Length(&d);
		    if (newline)
			content_length -= strlen(newline);
		    if (dynstr_Str(&d)[0] == '.') {
			fputc('.', p->output);
		    }
		    fputs(dynstr_Str(&d), p->output);
		    fputs(mime_CRLF, p->output);
		    if (mlines > 0)
			--mlines;
		}

		while (mlines) {
		    if (dpipe_Eof(&dp))
			break;
		    dynstr_Set(&d, 0);
		    mime_Readline(&dp, &d);
		    if ((p->spool_format & MMDF_SEPARATORS)
			? !strcmp(dynstr_Str(&p->msg_separator),
				  dynstr_Str(&d))
			: (!strncmp("From ", dynstr_Str(&d), 5)
			   && is_from_line(dynstr_Str(&d))))
			break;
		    if (dynstr_Str(&d)[0] == '.')
			fputc('.', p->output);
		    fputs(dynstr_Str(&d), p->output);
		    fputs(mime_CRLF, p->output);
		    if (mlines > 0)
			--mlines;
		}
		fputc('.', p->output);
		fputs(mime_CRLF, p->output);

		if (!is_zrtr && (p->last_msg < mnum))
		    p->last_msg = mnum;
	    } FINALLY {
		dynstr_Destroy(&d);
	    } ENDTRY;
	    fflush(p->output);
	} else {
	    /* XXX A "From " line followed by no headers?? */
	}
    } EXCEPT(ANY) {
	retval = POP_FAILURE;
    } FINALLY {
	glist_CleanDestroy(&hlist,
			   (void (*) NP((VPTR))) mime_pair_destroy);
	dpipe_Destroy(&dp);
    } ENDTRY;
    return (retval);
}

/* This function is used by other files. */

pop_sendline(p,buffer)
    POP *p;
    char *buffer;
{
    char *bp;
    int stuffed = FALSE;

    /* Byte stuff lines that begin with the temination octet */
    if (*buffer == '.') {
	fputc('.', p->output);
	stuffed = TRUE;
    }

    /* Look for a <NL> in the buffer */
    if (bp = index(buffer, '\n'))
	*bp = 0;

    /* Send the line to the client */
    fputs(buffer, p->output);

    if (p->debug & DEBUG_VERBOSE)
	pop_log(p, POP_DEBUG, stuffed ? "Sent: \".%s\"" : "Sent: \"%s\"", buffer);

    /* Put a <CR><NL> if a newline was removed from the buffer */
    if (bp)
	fputs("\r\n", p->output);
}
