/*
 * $RCSfile: prqueue.c,v $
 * $Revision: 2.13 $
 * $Date: 1995/07/14 04:12:04 $
 * $Author: schaefer $
 */

#include <glist.h>
#include <prqueue.h>
#include <excfns.h>
#include "bfuncs.h"

#ifndef lint
static const char prqueue_rcsid[] =
    "$Id: prqueue.c,v 2.13 1995/07/14 04:12:04 schaefer Exp $";
#endif /* lint */

#define nth(p,n) (glist_Nth(&((p)->entries),(n)))

#define parent(n) (((n)-1)/2)
#define lchild(n) (((n)<<1)+1)
#define rchild(n) (((n)+1)<<1)

void
prqueue_Init(pq, compare, size, growsize)
    struct prqueue *pq;
    int (*compare) NP((const GENERIC_POINTER_TYPE *,
		       const GENERIC_POINTER_TYPE *));
    int size, growsize;
{
    glist_Init(&(pq->entries), size, growsize);
    pq->compare = compare;
}

void
prqueue_Destroy(pq)
    struct prqueue *pq;
{
    glist_Destroy(&(pq->entries));
}

void
prqueue_CleanDestroy(pq, f)
    struct prqueue *pq;
    void (*f) NP((GENERIC_POINTER_TYPE *));
{
    GENERIC_POINTER_TYPE *p;
    int i;

    glist_FOREACH(&(pq->entries), GENERIC_POINTER_TYPE, p, i) {
	(*f)(p);
    }
    glist_Destroy(&(pq->entries));
}

static void
swap(pq, a, b)
    struct prqueue         *pq;
    int                     a, b;
{
    static GENERIC_POINTER_TYPE *buf;
    static int allocated = 0;

    if (pq->entries.eltSize > allocated) {
	if (allocated)
	    buf = (GENERIC_POINTER_TYPE *) erealloc(buf,
						    pq->entries.eltSize,
						    "prqueue:swap");
	else
	    buf = (GENERIC_POINTER_TYPE *) emalloc(pq->entries.eltSize,
						   "prqueue:swap");
	allocated = pq->entries.eltSize;
    }
#ifndef WIN16
    bcopy(nth(pq, a), buf, pq->entries.eltSize);
#else
    bcopy(nth(pq, a), buf, (short) pq->entries.eltSize);
#endif /* !WIN16 */
    glist_Set(&(pq->entries), a, nth(pq, b));
    glist_Set(&(pq->entries), b, buf);
}

void
prqueue_Add(pq, elt)
    struct prqueue *pq;
    GENERIC_POINTER_TYPE *elt;
{
    int                     indx, dad;

    glist_Add(&(pq->entries), elt);
    indx = glist_Length(&(pq->entries)) - 1;
    while (indx > 0) {
	dad = parent(indx);
	if ((*(pq->compare)) (nth(pq, indx), nth(pq, dad)) > 0) {
	    swap(pq, indx, dad);
	    indx = dad;
	} else {
	    return;
	}
    }
}

void
prqueue_Remove(pq)
    struct prqueue         *pq;
{
    if (glist_Length(&(pq->entries)) > 1) {
	int                     lcmp, rcmp, indx, lkid, rkid, len;

	glist_Set(&(pq->entries), 0,
		      nth(pq, glist_Length(&(pq->entries)) - 1));
	glist_Pop(&(pq->entries));
	len = glist_Length(&(pq->entries));
	indx = 0;
	lkid = 1;
	rkid = 2;
	while (lkid < len) {
	    lcmp = (*(pq->compare)) (nth(pq, lkid), nth(pq, indx));
	    if (rkid < len) {
		rcmp = (*(pq->compare)) (nth(pq, rkid), nth(pq, indx));
		if (lcmp > 0) {
		    if (rcmp > 0) {
			int                     lrcmp;

			lrcmp = (*(pq->compare)) (nth(pq, lkid),
						  nth(pq, rkid));
			if (lrcmp < 0) {
			    swap(pq, indx, rkid);
			    indx = rkid;
			} else {
			    swap(pq, indx, lkid);
			    indx = lkid;
			}
		    } else {
			swap(pq, indx, lkid);
			indx = lkid;
		    }
		} else if (rcmp > 0) {
		    swap(pq, indx, rkid);
		    indx = rkid;
		} else {
		    return;
		}
	    } else if (lcmp > 0) {
		swap(pq, indx, lkid);
		indx = lkid;
	    } else {
		return;
	    }
	    lkid = lchild(indx);
	    rkid = rchild(indx);
	}
    } else {
	glist_Pop(&(pq->entries));
    }
}

