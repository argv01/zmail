/*
 * $RCSfile: hashtab.c,v $
 * $Revision: 2.21 $
 * $Date: 1995/07/14 04:12:02 $
 * $Author: schaefer $
 */

#include <glist.h>
#include <dlist.h>
#include <hashtab.h>

#include "zclimits.h"
#include "bfuncs.h"

#ifndef CHAR_BIT
# define CHAR_BIT (8)
#endif /* CHAR_BIT */

#ifndef lint
static const char hashtab_rcsid[] =
    "$Id: hashtab.c,v 2.21 1995/07/14 04:12:02 schaefer Exp $";
#endif /* lint */

#ifdef WIN16
#define BCMP(a,b,c) (bcmp((a),(b),(short) (c)))
#else /* !WIN16 */
#define BCMP(a,b,c) (bcmp((a),(b),(c)))
#endif /* !WIN16 */

void
hashtab_Init(ht, hashfn, matchfn, eltsize, nbuckets)
    struct hashtab *ht;
    unsigned int (*hashfn) NP((const GENERIC_POINTER_TYPE *));
    int (*matchfn) NP((const GENERIC_POINTER_TYPE *,
		       const GENERIC_POINTER_TYPE *));
    int eltsize, nbuckets;
{
    int i;

    glist_Init(&(ht->buckets), (sizeof (struct dlist)), nbuckets);
    ht->hashfn = hashfn;
    ht->matchfn = matchfn;
    ht->eltsize = eltsize;
    ht->length = 0;
    for (i = 0; i < nbuckets; ++i) {
	glist_Add(&(ht->buckets), 0);
	dlist_Init((struct dlist *) glist_Nth(&(ht->buckets), i),
		   eltsize, 4);
    }
}

void
hashtab_Destroy(ht)
    struct hashtab *ht;
{
    glist_CleanDestroy(&(ht->buckets),
		       (void (*) NP((VPTR))) dlist_Destroy);
}

void
hashtab_CleanDestroy(ht, final)
    struct hashtab *ht;
    void (*final) NP((GENERIC_POINTER_TYPE *));
{
    struct dlist *dl;
    int i, j;
    GENERIC_POINTER_TYPE *p;

    glist_FOREACH(&(ht->buckets), struct dlist, dl, i) {
	dlist_FOREACH(dl, GENERIC_POINTER_TYPE, p, j) {
	    (*final)(p);
	}
    }
    hashtab_Destroy(ht);
}

void
hashtab_Add(ht, elt)
    struct hashtab *ht;
    const GENERIC_POINTER_TYPE * elt;
{
    dlist_Prepend(glist_Nth(&(ht->buckets),
			    ((*(ht->hashfn))(elt) %
			     glist_Length(&(ht->buckets)))), elt);
    ++(ht->length);
}

GENERIC_POINTER_TYPE *
hashtab_Find(ht, probe)
    struct hashtab *ht;
    const GENERIC_POINTER_TYPE * probe;
{
    struct dlist *dl;
    int i, bucket, eltsize = ht->eltsize;
    int (*compare)() = ht->matchfn;
    GENERIC_POINTER_TYPE * p;

    dl = (struct dlist *) glist_Nth(&(ht->buckets),
				    bucket = ((*(ht->hashfn))(probe) %
					      glist_Length(&(ht->buckets))));
    dlist_FOREACH(dl, GENERIC_POINTER_TYPE, p, i) {
	if (!(compare ?
	      ((*compare)(probe, p)) :
	      BCMP(probe, dlist_Nth(dl, i), eltsize))) {
	    ht->lastfind.bucket = bucket;
	    ht->lastfind.elt = i;
	    return dlist_Nth(dl, i);
	}
    }
    return (0);
}

void
hashtab_Remove(ht, probe)
    struct hashtab *ht;
    const GENERIC_POINTER_TYPE * probe;
{
    struct dlist *dl;
    int i;
    int (*compare)() = ht->matchfn;
    int eltsize = ht->eltsize;
    GENERIC_POINTER_TYPE * p;

    dl = (struct dlist *) glist_Nth(&(ht->buckets),
				    (probe ? 
				     ((*(ht->hashfn))(probe) %
				      glist_Length(&(ht->buckets))) :
				     ht->lastfind.bucket));
    if (!probe) {
	dlist_Remove(dl, ht->lastfind.elt);
	--(ht->length);
	return;
    }
    dlist_FOREACH(dl, GENERIC_POINTER_TYPE, p, i) {
	if (!(compare ?
	      ((*compare)(probe, p)) :
	      BCMP(probe, dlist_Nth(dl, i), eltsize))) {
	    dlist_Remove(dl, i);
	    --(ht->length);
	    return;
	}
    }
}

/* The following hashing algorithm is derived from Karp & Rabin,
 * Harvard Center for Research in Computing Technology Tech Report
 * TR-31-81.  HASHPRIME should be defined to be the next-to-largest
 * prime that will fit in an int.
 */

#ifdef WIN16
# define HASHPRIME (65519)
#else /* WIN16 */
#if defined(__osf__) && defined(__alpha)  /* 64-bit ints */
#define HASHPRIME (9223372036854775643)
#else  /* 32-bit ints */
#define HASHPRIME (2147483629)
#endif /* 64-bit ints */
#endif /* WIN16 */

unsigned int
hashtab_StringHash(str)
    const char *str;
{
    const unsigned char *ptr = (const unsigned char *) str;
    
    if (ptr) {
	register unsigned int sum = 0;
	
	while (*ptr) {
        register unsigned int bit = CHAR_BIT;
	    while (bit--)
		if ((sum <<= 1) >= HASHPRIME)
		    sum -= HASHPRIME;
	    if ((sum += *ptr++) >= HASHPRIME)
		sum -= HASHPRIME;
	}
	return sum + 1;
    }
    else
	return 1;
}

void
hashtab_InitIterator(it)
    struct hashtab_iterator *it;
{
    it->bucket = 0;
    it->elt = -1;
}

GENERIC_POINTER_TYPE *
hashtab_Iterate(ht, it)
    struct hashtab *ht;
    struct hashtab_iterator *it;
{
    do {
	if (it->bucket >= glist_Length(&(ht->buckets)))
	    return (0);
	if (it->elt < 0) {
	    it->elt = dlist_Head((struct dlist *) glist_Nth(&(ht->buckets),
							    it->bucket));
	} else {
	    it->elt = dlist_Next((struct dlist *) glist_Nth(&(ht->buckets),
							    it->bucket),
				 it->elt);
	}
	if (it->elt < 0)
	    ++(it->bucket);
    } while (it->elt < 0);
    ht->lastfind.bucket = it->bucket;
    ht->lastfind.elt = it->elt;
    return (dlist_Nth((struct dlist *) glist_Nth(&(ht->buckets),
						 it->bucket),
		      it->elt));
}

void
hashtab_Map(ht, fn, data)
    struct hashtab *ht;
    void (*fn) NP((VPTR, VPTR));
    VPTR data;
{
    struct hashtab_iterator hti;
    VPTR elt;

    hashtab_InitIterator(&hti);
    while (elt = hashtab_Iterate(ht, &hti))
	(*fn)(elt, data);
}

void
hashtab_Rehash(ht, newlen, newfn)
    struct hashtab *ht;
    int newlen;
    unsigned int (*newfn) NP((const GENERIC_POINTER_TYPE *));
{
    int oldlen = glist_Length(&(ht->buckets));
    int i, this, next, bucket;
    struct dlist *dl;

    if (newlen) {
	if (newlen > oldlen) {
	    for (i = oldlen; i < newlen; ++i) {
		glist_Add(&(ht->buckets), 0);
		dlist_Init((struct dlist *) glist_Nth(&(ht->buckets), i),
			   ht->eltsize, 4);
	    }
	}
    } else {
	newlen = oldlen;
    }
    if (newfn)
	ht->hashfn = newfn;
    else
	newfn = ht->hashfn;
    for (i = 0; i < oldlen; ++i) {
	dl = (struct dlist *) glist_Nth(&(ht->buckets), i);
	this = dlist_Head(dl);
	while (this >= 0) {
	    next = dlist_Next(dl, this);
	    if ((bucket = ((*newfn)(dlist_Nth(dl, this))) % newlen) != i) {
		dlist_Prepend((struct dlist *) glist_Nth(&(ht->buckets),
							 bucket),
			      dlist_Nth(dl, this));
		dlist_Remove(dl, this);
	    }
	    this = next;
	}
    }
    if (newlen < oldlen) {
	for (i = oldlen - 1; i >= newlen; --i) {
	    dlist_Destroy((struct dlist *) glist_Nth(&(ht->buckets), i));
	    glist_Pop(&(ht->buckets));
	}
    }
}
