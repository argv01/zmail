/* 
 * $RCSfile: zync_zhbm.c,v $
 * $Revision: 1.9 $
 * $Date: 1995/10/26 20:21:27 $
 * $Author: bobg $
 */

#include "popper.h"
#include <except.h>
#include <intset.h>
#include <mstore/mfolder.h>
#include <mstore/mime-api.h>
#include <dynstr.h>
#include <bfuncs.h>

static const char zync_zhbm_rcsid[] =
    "$Id: zync_zhbm.c,v 1.9 1995/10/26 20:21:27 bobg Exp $";

/* Copied from mstore/zpopfldr.c */
static void
intset_to_dynstr(iset, d, incr)
    struct intset *iset;
    struct dynstr *d;
    int incr;			/* a number to add to each intset elt *
				 * before stringifying */
{
    char buf[32];
    struct intset_iterator ii;
    int *iip;
    int start = -1, last = -1, any = 0;

    intset_InitIterator(&ii);
    while (iip = intset_Iterate(iset, &ii)) {
	if (*iip < -incr)
	    continue;
	if (start >= 0) {
	    if (*iip > (last + 1)) {
		if (last == start) {
		    sprintf(buf, "%d", start + incr);
		} else {
		    sprintf(buf, "%d-%d", start + incr, last + incr);
		}
		if (any)
		    dynstr_AppendChar(d, ',');
		dynstr_Append(d, buf);
		any = 1;
		start = *iip;
	    }
	} else {
	    start = *iip;
	}
	last = *iip;
    }
    if (start >= 0) {
	if (last == start) {
	    sprintf(buf, "%d", start + incr);
	} else {
	    sprintf(buf, "%d-%d", start + incr, last + incr);
	}
	if (any)
	    dynstr_AppendChar(d, ',');
	dynstr_Append(d, buf);
	any = 1;
    }
}

/* hashbucketmembers
 * for a given hash bucket (bits/part number),
 * send back the message numbers participating
 */

int
zync_zhbm(p)
    POP *p;
{
    struct dynstr result;
    int bits, part, keyp, i;
    struct intset iset;
    struct mailhash mhash;

    do_drop(p);

    dynstr_Init(&result);
    intset_Init(&iset);
    TRY {
	bits = atoi(p->pop_parm[1]);
	part = atoi(p->pop_parm[2]);
	keyp = atoi(p->pop_parm[3]);
	for (i = 1; i <= NUMMSGS(p); ++i) {
	    if (!(NTHMSG(p, i)->have_key_hash))
		compute_extras(p, 1, &i);
	    bcopy(&(NTHMSG(p, i)->key_hash), &mhash,
		  (sizeof (struct mailhash)));
	    if (mailhash_in_bucket(&mhash, bits, part))
		intset_Add(&iset, i);
	}
	intset_to_dynstr(&iset, &result, 0);
	if (dynstr_Length(&result))
	    pop_msg(p, POP_SUCCESS, dynstr_Str(&result));
	else
	    pop_msg(p, POP_SUCCESS, "0");
    } FINALLY {
	intset_Destroy(&iset);
	dynstr_Destroy(&result);
    } ENDTRY;
    return (POP_SUCCESS);
}

int
zync_zhb2(p)
    POP *p;
{
    size_t sent = 0;
    int bits, part, keyp, i;

    do_drop(p);

    bits = atoi(p->pop_parm[1]);
    part = atoi(p->pop_parm[2]);
    keyp = atoi(p->pop_parm[3]);

    pop_msg(p, POP_SUCCESS, 0);
    for (i = 1; i <= NUMMSGS(p); ++i) {
	if (!(NTHMSG(p, i)->have_key_hash))
	    compute_extras(p, 1, &i);
	if (mailhash_in_bucket(&(NTHMSG(p, i)->key_hash), bits, part)) {
	    fprintf(p->output, "%d:%s:", i,
		    mailhash_to_string(0, &(NTHMSG(p, i)->key_hash)));
	    if (!(NTHMSG(p, i)->have_header_hash))
		compute_extras(p, 1, &i);
	    fputs(mailhash_to_string(0, &(NTHMSG(p, i)->header_hash)),
		  p->output);
	    fputs(mime_CRLF, p->output);
	    sent += 120;	/* roughly */
	}
    }
    fputc('.', p->output);
    fputs(mime_CRLF, p->output);
    fflush(p->output);
    allow(p, sent);
    return (POP_SUCCESS);
}
