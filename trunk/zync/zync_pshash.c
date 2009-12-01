/* 
 * $RCSfile: zync_pshash.c,v $
 * $Revision: 1.11 $
 * $Date: 1995/10/26 20:21:23 $
 * $Author: bobg $
 */

#include <except.h>
#include <dputil.h>
#include "popper.h"
#include <stdio.h>
#include <mstore/mfolder.h>
#include <glist.h>
#include <intset.h>
#include <dpipe.h>
#include <bfuncs.h>

static const char zync_pshash_rcsid[] =
    "$Id: zync_pshash.c,v 1.11 1995/10/26 20:21:23 bobg Exp $";

/* Send partitioned super-hash codes for a given list of messages. */

#define LEAFBUCKETSIZE (8)	/* as in mstore/mfolder.c */

int
zync_zpsh(p)
    POP *p;
{
    size_t sent = 0;
    int bits, keyp;
    struct intset whichbuf, *which = 0;
    struct intset subsetbuf, *subset = 0;
    struct mailhash hash;
    struct glist buckets, *listp;
    int i, j, nbuckets;
    struct number_list temp_list;

    bits = atoi(p->pop_parm[1]);

    nbuckets = (which ? intset_Count(which) : (1 << bits));

    if (parse_bucket_list(p, p->pop_parm[2], (1 << bits), &temp_list))
	return (POP_FAILURE);
    if (temp_list.list_count > 0) {
	which = &whichbuf;
	intset_Init(which);
	for (i = 0; i < temp_list.list_count; ++i)
	    intset_Add(which, temp_list.list_numbers[i]);
    }
    free(temp_list.list_numbers);
    keyp = atoi(p->pop_parm[3]);
    if (parse_message_list(p, p->pop_parm[4], &temp_list)) {
	if (which)
	    intset_Destroy(which);
	return (POP_FAILURE);	/* xxx cleanup */
    }
    if (temp_list.list_count > 0) {
	subset = &subsetbuf;
	intset_Init(subset);
	for (i = 0; i < temp_list.list_count; ++i)
	    intset_Add(subset, temp_list.list_numbers[i]);
    }
    free(temp_list.list_numbers);

    /* buckets is a Glist of Glists.  Each bucket holds a Glist of all
     * the hashes in that bucket.  Once all the buckets are
     * full, the list of hashes for each bucket will be sorted,
     * then hashes into a superhash part.
     */
    glist_Init(&buckets, sizeof (struct glist), nbuckets);
    TRY {
	for (i = 0; i < nbuckets; ++i) {
	    glist_Add(&buckets, 0);
	    glist_Init(glist_Nth(&buckets, i),
		       sizeof (struct mailhash),
		       LEAFBUCKETSIZE);
	}
	TRY {
	    for (i = 1; i <= NUMMSGS(p); ++i) {
		if (subset && !intset_Contains(subset, i))
		    continue;
		if (!(NTHMSG(p, i)->have_key_hash)
		    || (!keyp && !(NTHMSG(p, i)->have_header_hash)))
		    compute_extras(p, 1, &i);
		bcopy(&(NTHMSG(p, i)->key_hash), &hash,
		      (sizeof (struct mailhash)));
		if (which) {
		    struct intset_iterator ii;
		    int *ip;

		    j = 0;
		    intset_InitIterator(&ii);
		    while (ip = intset_Iterate(which, &ii)) {
			if (mailhash_in_bucket(&hash, bits, *ip)) {
			    if (!keyp)
				bcopy(&(NTHMSG(p, i)->header_hash), &hash,
				      (sizeof (struct mailhash)));
			    glist_Add(glist_Nth(&buckets, j), &hash);
			    break;
			}
			++j;
		    }
		} else {
		    int k = mailhash_bucket(&hash, bits);

		    if (!keyp)
			bcopy(&(NTHMSG(p, i)->header_hash), &hash,
			      (sizeof (struct mailhash)));
		    glist_Add(glist_Nth(&buckets, k), &hash);
		}
	    }
	    pop_msg(p, POP_SUCCESS, 0);
	    glist_FOREACH(&buckets, struct glist, listp, i) {
		struct dpipe dp;
		struct dputil_MD5buf md5buf;
		struct mailhash result;

		dputil_MD5buf_init(&md5buf, result.x);
		dpipe_Init(&dp, dputil_MD5, &md5buf, 0, 0, 0);
		TRY {
		    struct mailhash *hashp, lasthash;

		    glist_Sort(listp,
			       (int (*) NP((CVPTR, CVPTR))) mailhash_cmp);
		    glist_FOREACH(listp, struct mailhash, hashp, j) {
			if ((j > 0) && !mailhash_cmp(hashp, &lasthash))
			    continue; /* duplicate elimination */
			dpipe_Write(&dp, (char *) (hashp->x), MAILHASH_BYTES);
			bcopy(hashp, &lasthash, sizeof (struct mailhash));
		    }
		    dpipe_Close(&dp);
		    dpipe_Flush(&dp);
		    dputil_MD5buf_final(&md5buf);
		} FINALLY {
		    dpipe_Destroy(&dp);
		} ENDTRY;
		fputs(mailhash_to_string(0, &result), p->output);
		putc('\r', p->output);
		putc('\n', p->output);
		sent += 25;	/* roughly */
	    }
	} FINALLY {
	    for (i = 0; i < nbuckets; ++i) {
		glist_Destroy(glist_Nth(&buckets, i));
	    }
	} ENDTRY;
    } FINALLY {
	glist_Destroy(&buckets);
	intset_Destroy(which);
	intset_Destroy(subset);
    } ENDTRY;
    fputs(".\r\n", p->output);
    fflush(p->output);
    allow(p, sent + 20);	/* roughly */
    return (POP_SUCCESS);
}
