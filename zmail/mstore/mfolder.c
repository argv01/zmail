/* 
 * $RCSfile: mfolder.c,v $
 * $Revision: 1.38 $
 * $Date: 1996/08/09 16:44:57 $
 * $Author: schaefer $
 */

#include <spoor.h>
#include <mfolder.h>
#include <message.h>
#include <ghosts.h>
#include <glist.h>
#include <dpipe.h>
#include <dputil.h>
#include <intset.h>
#include <zstrings.h>
#include <bfuncs.h>

static const char mfldr_rcsid[] =
    "$Id: mfolder.c,v 1.38 1996/08/09 16:44:57 schaefer Exp $";

#define LXOR(a,b) ((!(a))!=(!(b)))

#ifndef CHAR_BIT
#define CHAR_BIT 8	/* a reasonable assumption */
#endif /* !CHAR_BIT */

/* The class descriptor */
struct spClass *mfldr_class = (struct spClass *) 0;

int m_mfldr_DiffStati;
int m_mfldr_SuperHash;
int m_mfldr_HashBucket;
int m_mfldr_HashBucketMembers;
int m_mfldr_DeleteMsg;
int m_mfldr_Import;
int m_mfldr_Update;
int m_mfldr_Diff;
int m_mfldr_RefineDiff;
int m_mfldr_Sync;
int m_mfldr_BuryGhosts;

DEFINE_EXCEPTION(mfldr_err_FromSyntax, "mfldr_err_FromSyntax");

static void mfldr_initialize P((struct mfldr *));
static void mfldr_finalize P((struct mfldr *));
static int hashset_cmp P((const struct hashset *, const struct hashset *));

static void
mfldr_initialize(self)
    struct mfldr *self;
{
    glist_Init(&(self->mmsgs), sizeof(struct mmsg *), 32);
}

static void
mfldr_finalize(self)
    struct mfldr *self;
{
    int i;
    struct mmsg **m;

    glist_FOREACH(&(self->mmsgs), struct mmsg *, m, i) {
	mmsg_Destroy(*m);
    }
    glist_Destroy(&(self->mmsgs));
}

/* Returns the first BITS bits of MHASH as a bucket number
 */
int
mailhash_bucket(mhash, bits)
    struct mailhash *mhash;	
    int bits;
{
    int result = 0;
    int i;

    for (i = 0; i < bits; ++i) {
	/* Test bits LSB to MSB, set bits MSB to LSB, because
	 * we must make each additional bit the least
	 * significant.  That way, the child bucket
	 * numbers for bucket N are (N * 2) and (N * 2 + 1).
	 */
	if (mhash->x[i / CHAR_BIT] & (1 << (i % CHAR_BIT)))
	    result += (1 << ((bits - 1) - i));
    }
    return (result);
}

static void
mfldr_HashBucket_fn(self, arg)
    struct mfldr *self;
    spArgList_t arg;
{
    int bits = spArg(arg, int);
    int part = spArg(arg, int);
    int keyp = spArg(arg, int);
    struct glist *hashes = spArg(arg, struct glist *);
    struct hashset hs;
    int i;

    for (i = 0; i < mfldr_NumMessages(self); ++i) {
	mmsg_KeyHash(mfldr_NthMessage(self, i), &hs.key);
	if (mailhash_in_bucket(&hs.key, bits, part)) {
	    hs.num = i;
	    if (!keyp)
		mmsg_HeaderHash(mfldr_NthMessage(self, i), &hs.header);
	    glist_Add(hashes, &hs);
	}
    }
}

static void
mfldr_HashBucketMembers_fn(self, arg)
    struct mfldr *self;
    spArgList_t arg;
{
    int bits = spArg(arg, int);
    int part = spArg(arg, int);
    struct intset *iset = spArg(arg, struct intset *);
    int keyp = spArg(arg, int);
    struct mailhash mhash;
    int i;

    for (i = 0; i < mfldr_NumMessages(self); ++i) {
	mmsg_KeyHash(mfldr_NthMessage(self, i), &mhash);
	if (mailhash_in_bucket(&mhash, bits, part))
	    intset_Add(iset, i);
    }
}

int
mailhash_cmp(a, b)
    const struct mailhash *a, *b;
{
    int result, i;

    for (i = 0; i < MAILHASH_BYTES; ++i) {
	if (result = (a->x[i] - b->x[i]))
	    return (result);
    }
    return (0);
}

static void
mfldr_Sync_fn(self, arg)
    struct mfldr *self;
    spArgList_t arg;
{
    struct mfldr *fldr2 = spArg(arg, struct mfldr *);
    struct dlist *a = spArg(arg, struct dlist *);
    struct dlist *b = spArg(arg, struct dlist *);
    struct dlist *d = spArg(arg, struct dlist *);
    mfldr_SyncProgress_t progress_fn = spArg(arg, mfldr_SyncProgress_t);
    VPTR progress_data = spArg(arg, VPTR);
    struct mfldr_diff *diff;
    struct mfldr_refinedDiff *rdiff;
    int i, total = 0, n = 0;

    if (a)
	total += dlist_Length(a);
    if (b)
	total += dlist_Length(b);
    if (d)
	total += dlist_Length(d);
    if (a) {
	dlist_FOREACH(a, struct mfldr_diff, diff, i) {
	    if (progress_fn)
		(*progress_fn)(n, total, progress_data);
	    ++n;
	    if (diff->apply) {
		if (diff->remove) {
		    mfldr_DeleteMsg(self, diff->num);
		} else {
		    mfldr_Import(fldr2,
				   mfldr_NthMessage(self, diff->num),
				   -1);
		}
	    }
	}
    }
    if (b) {
	dlist_FOREACH(b, struct mfldr_diff, diff, i) {
	    if (progress_fn)
		(*progress_fn)(n, total, progress_data);
	    ++n;
	    if (diff->apply) {
		if (diff->remove) {
		    mfldr_DeleteMsg(fldr2, diff->num);
		} else {
		    mfldr_Import(self,
				   mfldr_NthMessage(fldr2, diff->num),
				   -1);
		}
	    }
	}
    }
    if (d) {
	dlist_FOREACH(d, struct mfldr_refinedDiff, rdiff, i) {
	    if (progress_fn)
		(*progress_fn)(n, total, progress_data);
	    ++n;
	    if (rdiff->apply1) {
		mmsg_SetStatus(mfldr_NthMessage(self, rdiff->num1),
			       mmsg_status_ALL,
			       mmsg_Status(mfldr_NthMessage(fldr2,
							    rdiff->num2)));
	    } else if (rdiff->apply2) {
		mmsg_SetStatus(mfldr_NthMessage(fldr2, rdiff->num2),
			       mmsg_status_ALL,
			       mmsg_Status(mfldr_NthMessage(self,
							    rdiff->num1)));
	    }
	}
    }
}

#define LEAFBUCKETSIZE (8)

static int
hashset_cmp(a, b)
    const struct hashset *a, *b;
{
    return (mailhash_cmp(&(a->key), &(b->key)));
}

struct twodlists {
    struct dlist *a, *b;
};

static void
diff_aux(fldr1, fldr2, mhashes1, mhashes2, x)
    struct mfldr *fldr1, *fldr2;
    struct glist *mhashes1, *mhashes2;
    struct twodlists *x;
{
    struct mfldr_diff diff;
    int cmp, j = 0, k = 0;	/* j walks mhashes1, k walks mhashes2 */
    struct dlist *a = x->a, *b = x->b;

    diff.apply = 0;
    while ((j < glist_Length(mhashes1)) || (k < glist_Length(mhashes2))) {
	if ((j < glist_Length(mhashes1)) && (k < glist_Length(mhashes2))) {
	    cmp = mailhash_cmp(&(((struct hashset *)
				  glist_Nth(mhashes1, j))->key),
			       &(((struct hashset *)
				  glist_Nth(mhashes2, k))->key));
	} else if (j < glist_Length(mhashes1)) {
	    cmp = -1;
	} else {
	    cmp = 1;
	}
	if (cmp < 0) {		/* hashes1[j] is a singleton */
	    if (a) {
		diff.num = ((struct hashset *) glist_Nth(mhashes1, j))->num;
		diff.remove = ghostp(&(((struct hashset *)
					glist_Nth(mhashes1, j))->key));
		dlist_Append(a, &diff);
	    }
	    ++j;
	} else if (cmp > 0) {	/* hashes2[k] is a singleton */
	    if (b) {
		diff.num = ((struct hashset *) glist_Nth(mhashes2, k))->num;
		diff.remove = ghostp(&(((struct hashset *)
					glist_Nth(mhashes2, k))->key));
		dlist_Append(b, &diff);
	    }
	    ++k;
	} else {		/* cmp == 0 */
	    ++j;
	    ++k;
	}
    }
}

static void
refine_aux(fldr1, fldr2, mhashes1, mhashes2, d)
    struct mfldr *fldr1, *fldr2;
    struct glist *mhashes1, *mhashes2;
    struct dlist *d;
{
    int cmp, j = 0, k = 0;

    while ((j < glist_Length(mhashes1)) || (k < glist_Length(mhashes2))) {
	if ((j < glist_Length(mhashes1)) && (k < glist_Length(mhashes2))) {
	    cmp = mailhash_cmp(&(((struct hashset *)
				  glist_Nth(mhashes1, j))->key),
			  &(((struct hashset *)
			     glist_Nth(mhashes2, k))->key));
	} else if (j < glist_Length(mhashes1)) {
	    cmp = -1;
	} else {
	    cmp = 1;
	}
	if (cmp < 0) {
	    /* This could happen if one bucket contains multiple
	     * copies of the same hash and the other doesn't.
	     */
	    ++j;
	} else if (cmp > 0) {
	    /* This could happen if one bucket contains multiple
	     * copies of the same hash and the other doesn't.
	     */
	    ++k;
	} else {
	    if (mailhash_cmp(&(((struct hashset *)
				glist_Nth(mhashes1, j))->header),
			     &(((struct hashset *)
				glist_Nth(mhashes2, k))->header))) {
		struct mfldr_refinedDiff r;

		r.apply1 = 0;
		r.apply2 = 0;
		r.num1 = ((struct hashset *) glist_Nth(mhashes1, j))->num;
		r.num2 = ((struct hashset *) glist_Nth(mhashes2, k))->num;
		dlist_Append(d, &r);
	    }
	    ++j;
	    ++k;
	}
    }
}

static void
mfldr_SuperHash_fn(self, arg)
    struct mfldr *self;
    spArgList_t arg;
{
    int bits = spArg(arg, int);
    struct intset *which = spArg(arg, struct intset *);
    struct glist *hashbuf = spArg(arg, struct glist *);
    int keyp = spArg(arg, int);
    struct intset *subset = spArg(arg, struct intset *);
    struct mailhash hash;
    struct glist buckets, *listp;
    int i, j, nbuckets = (which ? intset_Count(which) : (1 << bits));

    /* buckets is a Glist of Glists.  Each bucket holds a Glist of all
     * the hashes in that bucket.  Once all the buckets are
     * full, the list of hashes for each bucket will be sorted,
     * then hashed into a superhash part.
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
	    for (i = 0; i < mfldr_NumMessages(self); ++i) {
		if (subset && !intset_Contains(subset, i))
		    continue;
		mmsg_KeyHash(mfldr_NthMessage(self, i), &hash);
		if (which) {
		    struct intset_iterator ii;
		    int *ip;

		    j = 0;
		    intset_InitIterator(&ii);
		    while (ip = intset_Iterate(which, &ii)) {
			if (mailhash_in_bucket(&hash, bits, *ip)) {
			    if (!keyp)
				mmsg_HeaderHash(mfldr_NthMessage(self, i),
						&hash);
			    glist_Add(glist_Nth(&buckets, j), &hash);
			    break;
			}
			++j;
		    }
		} else {
		    j = mailhash_bucket(&hash, bits);
		    if (!keyp)
			mmsg_HeaderHash(mfldr_NthMessage(self, i), &hash);
		    glist_Add(glist_Nth(&buckets, j), &hash);
		}
	    }
	    glist_FOREACH(&buckets, struct glist, listp, i) {
		struct dpipe dp;
		struct dputil_MD5buf md5buf;

		glist_Add(hashbuf, (VPTR) 0); /* extend hashbuf */
		dputil_MD5buf_init(&md5buf,
				   ((struct mailhash *)
				    glist_Nth(hashbuf, i))->x);
		dpipe_Init(&dp,
			   (dpipe_Callback_t) dputil_MD5, &md5buf,
			   (dpipe_Callback_t) 0,
			   (GENERIC_POINTER_TYPE *) 0, 0);
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
	    }
	} FINALLY {
	    for (i = 0; i < glist_Length(&buckets); ++i) {
		glist_Destroy(glist_Nth(&buckets, i));
	    }
	} ENDTRY;
    } FINALLY {
	glist_Destroy(&buckets);
    } ENDTRY;
}

static const char empty_md5_hash[] =
    "d41d 8cd9 8f00 b204 e980 0998 ecf8 427e";

static int
empty_hash_p(hash)
    const struct mailhash *hash;
{
    static int initialized = 0;
    static struct mailhash TheEmptyHash;

    if (!initialized) {
	string_to_mailhash(&TheEmptyHash, empty_md5_hash);
	initialized = 1;
    }
    return (!mailhash_cmp(hash, &TheEmptyHash));
}

static void
call_callback(fldr1, fldr2, bits, bucket, keyp, callback, cbdata)
    struct mfldr *fldr1, *fldr2;
    int bits, bucket, keyp;
    void (*callback) NP((struct mfldr *, struct mfldr *,
			 struct glist *, struct glist *,
			 GENERIC_POINTER_TYPE *));
    GENERIC_POINTER_TYPE *cbdata;
{
    struct glist hashes1, hashes2;

    glist_Init(&hashes1, (sizeof (struct hashset)), LEAFBUCKETSIZE);
    glist_Init(&hashes2, (sizeof (struct hashset)), LEAFBUCKETSIZE);
    TRY {
	mfldr_HashBucket(fldr1, bits, bucket, keyp, &hashes1);
	glist_Sort(&hashes1, (int (*) NP((CVPTR, CVPTR))) hashset_cmp);

	mfldr_HashBucket(fldr2, bits, bucket, keyp, &hashes2);
	glist_Sort(&hashes2, (int (*) NP((CVPTR, CVPTR))) hashset_cmp);

	(*callback)(fldr1, fldr2, &hashes1, &hashes2, cbdata);
    } FINALLY {
	glist_Destroy(&hashes1);
	glist_Destroy(&hashes2);
    } ENDTRY;
}

/* Used by both mfldr_Diff and mfldr_RefineDiff */
static void
do_diff(fldr1, fldr2, keyp, subset1, subset2, callback, cbdata)
    struct mfldr *fldr1, *fldr2;
    int keyp;
    struct intset *subset1, *subset2;
    void (*callback) NP((struct mfldr *, struct mfldr *,
			 struct glist *, struct glist *,
			 GENERIC_POINTER_TYPE *));
    GENERIC_POINTER_TYPE *cbdata;
{
    struct glist hash1, hash2, selected;
    struct intset iset;
    struct intset_iterator ii;
    int bits = 0, i, *ip, maxbits = 0, l, l2;
    
    glist_Init(&hash1, sizeof (struct mailhash), 8);
    glist_Init(&hash2, sizeof (struct mailhash), 8);
    glist_Init(&selected, sizeof (int), 8);
    intset_Init(&iset);
    TRY {
	/* Compute maxbits, the maximum number of bits by which we
	 * intend to partition the superhash.  That number is chosen
	 * to be the one such that each superhash partition represents
	 * (on average) LEAFBUCKETSIZE messages.
	 */
	l = mfldr_NumMessages(fldr1);
	l2 = mfldr_NumMessages(fldr2);

	if (l2 > l)
	    l = l2;
	while (l > LEAFBUCKETSIZE) {
	    ++maxbits;
	    l /= 2;
	}

	intset_Add(&iset, 0);
	for (bits = 0; !intset_EmptyP(&iset) && (bits <= maxbits); ++bits) {
	    glist_Truncate(&hash1, 0);
	    glist_Truncate(&hash2, 0);
	    mfldr_SuperHash(fldr1, bits, &iset, &hash1, keyp, subset1);
	    mfldr_SuperHash(fldr2, bits, &iset, &hash2, keyp, subset2);

	    /* Construct `selected', the list of buckets that were
	     * selected in the superhash calls above.  It's the
	     * same info as in iset, but we're going to need
	     * to start munging iset in a moment.
	     */
	    glist_Truncate(&selected, 0);
	    intset_InitIterator(&ii);
	    while (ip = intset_Iterate(&iset, &ii))
		glist_Add(&selected, ip);

	    intset_Clear(&iset);
	    glist_FOREACH(&selected, int, ip, i) {
		if (mailhash_cmp(glist_Nth(&hash1, i),
				 glist_Nth(&hash2, i))) {
		    if ((bits == maxbits)
			|| empty_hash_p(glist_Nth(&hash1, i))
			|| empty_hash_p(glist_Nth(&hash2, i))) {
			/* Don't need to refine this bucket any farther.
			 * Call the callback on the bucket members.
			 */
			call_callback(fldr1, fldr2, bits, *ip, keyp,
				      callback, cbdata);
		    } else {
			/* This bucket differs (and in a non-trivial way);
			 * select both children for next iter.
			 */
			intset_Add(&iset, *ip << 1);
			intset_Add(&iset, (*ip << 1) + 1);
		    }
		}
	    }
	}
    } FINALLY {
	glist_Destroy(&hash1);
	glist_Destroy(&hash2);
	glist_Destroy(&selected);
	intset_Destroy(&iset);
    } ENDTRY;
}

static void
mfldr_Diff_fn(self, arg)
    struct mfldr *self;
    spArgList_t arg;
{
    struct mfldr *fldr2 = spArg(arg, struct mfldr *);
    struct dlist *a = spArg(arg, struct dlist *);
    struct dlist *b = spArg(arg, struct dlist *);
    struct twodlists x;

    x.a = a;
    x.b = b;
    do_diff(self, fldr2, 1,
            (GENERIC_POINTER_TYPE *) 0,
            (GENERIC_POINTER_TYPE *) 0,
            diff_aux,
            (GENERIC_POINTER_TYPE *) &x);
}

static void
mfldr_RefineDiff_fn(self, arg)
    struct mfldr *self;
    spArgList_t arg;
{
    struct mfldr *fldr2 = spArg(arg, struct mfldr *);
    struct dlist *a = spArg(arg, struct dlist *);
    struct dlist *b = spArg(arg, struct dlist *);
    struct dlist *d = spArg(arg, struct dlist *);
    struct intset c1, c2;
    struct mfldr_diff *diff;
    int n1 = mfldr_NumMessages(self);
    int n2 = mfldr_NumMessages(fldr2);
    int i;

    if (!n1 || !n2)
	return;
    intset_Init(&c1);
    intset_Init(&c2);
    TRY {
	intset_AddRange(&c1, 0, n1 - 1);
	intset_AddRange(&c2, 0, n2 - 1);
	dlist_FOREACH(a, struct mfldr_diff, diff, i) {
	    intset_Remove(&c1, diff->num);
	}
	dlist_FOREACH(b, struct mfldr_diff, diff, i) {
	    intset_Remove(&c2, diff->num);
	}
	do_diff(self, fldr2, 0, &c1, &c2, refine_aux,
		(GENERIC_POINTER_TYPE *) d);
    } FINALLY {
	intset_Destroy(&c1);
	intset_Destroy(&c2);
    } ENDTRY;
}

char *
mailhash_to_string(str, hash)
    char *str;
    const struct mailhash *hash;
{
    static char buf[1 + 3 * MAILHASH_BYTES];
    char b[8];
    int i;

    if (!str)
	str = buf;
    *str = 0;
    for (i = 0; i < MAILHASH_BYTES; i += 2) {
	sprintf(b, "%02x%02x", hash->x[i], hash->x[i + 1]);
	if (i > 0)
	    strcat(str, " ");
	strcat(str, b);
    }
    return (str);
}

void
string_to_mailhash(hash, str)
    struct mailhash *hash;
    const char *str;
{
    int hashbyte = 0, nybble = 1, ok, val;
    const char *p;
    char c;

    bzero(hash->x, MAILHASH_BYTES);
    for (p = str; (c = *p) && (hashbyte < MAILHASH_BYTES); ++p) {
	ok = 0;
	if (('0' <= c) && (c <= '9')) {
	    ok = 1;
	    val = c - '0';
	} else if (('a' <= c) && (c <= 'f')) {
	    ok = 1;
	    val = c - 'a' + 10;
	} else if (('A' <= c) && (c <= 'F')) {
	    ok = 1;
	    val = c - 'A' + 10;
	}
	if (ok) {
	    hash->x[hashbyte] |= (val << (4 * nybble));
	    if (nybble) {
		nybble = 0;
	    } else {
		++hashbyte;
		nybble = 1;
	    }
	}
    }
}

static void
mfldr_BuryGhosts_fn(self, arg)
    struct mfldr *self;
    spArgList_t arg;
{
    char *tombfile = spArg(arg, char *);
    time_t interrment = time((time_t *)0);
    struct mmsg **mesg;
    int i;

    glist_FOREACH(&(self->mmsgs), struct mmsg *, mesg, i) {
	TRY {
	    if (mmsg_Status(*mesg) & mmsg_status_DELETED) {
		struct mailhash keyhash;

		mmsg_KeyHash(*mesg, &keyhash);
		ghost_Bury(&keyhash, interrment);
	    }
	} EXCEPT(ANY) {
	    ;	/* It's only a ghost record.  Ignore it. */
	} ENDTRY;
    }

    ghost_SealTomb();
}

static void
mfldr_DiffStati_fn(self, arg)
    struct mfldr *self;
    spArgList_t arg;
{
    struct dlist *dl = spArg(arg, struct dlist *);
    struct mfldr_diff *d;
    int i;

    dlist_FOREACH(dl, struct mfldr_diff, d, i) {
	d->status = mmsg_Status(mfldr_NthMessage(self, d->num));
    }
}

void
mfldr_InitializeClass()
{
    if (mfldr_class)
	return;
    mfldr_class = spoor_CreateClass("mfldr",
				    "Abstract folders",
				    spoor_class,
				    (sizeof (struct mfldr)),
				    (void (*) NP((VPTR))) mfldr_initialize,
				    (void (*) NP((VPTR))) mfldr_finalize);

    m_mfldr_DiffStati = spoor_AddMethod(mfldr_class,
					"DiffStati",
					"DiffStati",
					mfldr_DiffStati_fn);
    m_mfldr_SuperHash = spoor_AddMethod(mfldr_class,
					"SuperHash",
					"Compute the superhash for a folder",
					mfldr_SuperHash_fn);
    m_mfldr_HashBucket = spoor_AddMethod(mfldr_class,
					 "HashBucket",
					 "HashBucket",
					 mfldr_HashBucket_fn);
    m_mfldr_HashBucketMembers = spoor_AddMethod(mfldr_class,
						"HashBucketMembers",
						"HashBucketMembers",
						mfldr_HashBucketMembers_fn);
    m_mfldr_DeleteMsg = spoor_AddMethod(mfldr_class,
					"DeleteMsg",
					"DeleteMsg",
					(spoor_method_t) 0);
    m_mfldr_Import = spoor_AddMethod(mfldr_class,
				     "Import",
				     "Import",
				     (spoor_method_t) 0);
    m_mfldr_Update = spoor_AddMethod(mfldr_class,
				     "Update",
				     "Update",
				     (spoor_method_t) 0);
    m_mfldr_Diff = spoor_AddMethod(mfldr_class,
				   "Diff",
				   "Compare two folders",
				   mfldr_Diff_fn);
    m_mfldr_RefineDiff = spoor_AddMethod(mfldr_class,
					 "RefineDiff",
					 "RefineDiff",
					 mfldr_RefineDiff_fn);
    m_mfldr_Sync = spoor_AddMethod(mfldr_class,
				   "Sync",
				   "Sync",
				   mfldr_Sync_fn);
    m_mfldr_BuryGhosts = spoor_AddMethod(mfldr_class,
				         "BuryGhosts",
				         "Bury ghost messages",
				         mfldr_BuryGhosts_fn);

    mmsg_InitializeClass();
}
