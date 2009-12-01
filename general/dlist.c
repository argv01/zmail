/*
 * $RCSfile: dlist.c,v $
 * $Revision: 2.13 $
 * $Date: 1995/06/27 19:37:52 $
 * $Author: spencer $
 */

#include <glist.h>
#include <dlist.h>
#include <excfns.h>
#include "bfuncs.h"

#ifndef lint
static const char dlist_rcsid[] =
    "$Id: dlist.c,v 2.13 1995/06/27 19:37:52 spencer Exp $";
#endif /* lint */

#define nthEntry(d,n) ((struct dlistEntry *) glist_Nth(&((d)->entries),(n)))

static void
init_from_elt(elt, ptr, size)
    const GENERIC_POINTER_TYPE *elt;
    GENERIC_POINTER_TYPE *ptr;
    int size;
{
    if (elt)
	bcopy(elt, ptr, size);
    else
	bzero(ptr, size);
}

static struct dlistEntry *
newentry(dl, indx)
    struct dlist           *dl;
    int                    *indx;
{
    struct dlistEntry      *result;

    if (dl->freeHead >= 0) {
	result = nthEntry(dl, dl->freeHead);
	*indx = dl->freeHead;
	if (dl->freeHead == dl->freeTail) {
	    dl->freeHead = dl->freeTail = -1;
	} else {
	    dl->freeHead = result->next;
	    (nthEntry(dl, result->next))->prev = -1;
	}
    } else {
	*indx = glist_Add(&(dl->entries), 0);
	result = nthEntry(dl, *indx);
    }
    return (result);
}

void
dlist_Init(dl, eltSize, growSize)
    struct dlist *dl;
    int eltSize, growSize;
{
    dl->eltSize = eltSize;
    glist_Init(&(dl->entries), eltSize + ALIGN(sizeof (struct dlistEntry)),
	       growSize);
    dl->head = dl->tail = dl->freeHead = dl->freeTail = -1;
    dl->length = 0;
}

void
dlist_Destroy(dl)
    struct dlist           *dl;
{
    glist_Destroy(&(dl->entries));
}

void
dlist_CleanDestroy(dl, final)
    struct dlist *dl;
    void (*final) NP((GENERIC_POINTER_TYPE *));
{
    int i = dlist_Head(dl);

    while (i >= 0) {
	(*final)(dlist_Nth(dl, i));
	i = dlist_Next(dl, i);
    }
    dlist_Destroy(dl);
}

int
dlist_Append(dl, elt)
    struct dlist *dl;
    const GENERIC_POINTER_TYPE *elt;
{
    int indx;
    struct dlistEntry *new = newentry(dl, &indx);

    new->next = -1;
    new->prev = dl->tail;
    dl->tail = indx;
    if (dl->head == -1)
	dl->head = indx;
    else
	(nthEntry(dl, new->prev))->next = indx;
    ++(dl->length);
    init_from_elt(elt, new + 1, dl->eltSize);
    return (indx);
}

int
dlist_InsertAfter(dl, afterIndex, elt)
    struct dlist *dl;
    int afterIndex;
    const GENERIC_POINTER_TYPE *elt;
{
    int indx;
    struct dlistEntry *new, *after = nthEntry(dl, afterIndex);

    if (after->next == -1) {
	return (dlist_Append(dl, elt));
    }
    new = newentry(dl, &indx);
    after = nthEntry(dl, afterIndex); /* in case a realloc happened */
    new->next = after->next;
    new->prev = afterIndex;
    after->next = indx;
    (nthEntry(dl, new->next))->prev = indx;
    ++(dl->length);
    init_from_elt(elt, new + 1, dl->eltSize);
    return (indx);
}

int
dlist_InsertBefore(dl, beforeIndex, elt)
    struct dlist *dl;
    int beforeIndex;
    const GENERIC_POINTER_TYPE *elt;
{
    int indx;
    struct dlistEntry *new, *before = nthEntry(dl, beforeIndex);

    if (before->prev == -1) {
	return (dlist_Prepend(dl, elt));
    }
    new = newentry(dl, &indx);
    before = nthEntry(dl, beforeIndex);	/* in case a realloc happened */
    new->next = beforeIndex;
    new->prev = before->prev;
    before->prev = indx;
    (nthEntry(dl, new->prev))->next = indx;
    ++(dl->length);
    init_from_elt(elt, new + 1, dl->eltSize);
    return (indx);
}

int
dlist_Prepend(dl, elt)
    struct dlist *dl;
    const GENERIC_POINTER_TYPE *elt;
{
    int indx;
    struct dlistEntry *new = newentry(dl, &indx);

    new->next = dl->head;
    new->prev = -1;
    dl->head = indx;
    if (dl->tail == -1)
	dl->tail = indx;
    else
	(nthEntry(dl, new->next))->prev = indx;
    ++(dl->length);
    init_from_elt(elt, new + 1, dl->eltSize);
    return (indx);
}

void
dlist_Replace(dl, indx, elt)
    struct dlist *dl;
    int indx;
    const GENERIC_POINTER_TYPE *elt;
{
    init_from_elt(elt, nthEntry(dl, indx) + 1, dl->eltSize);
}

void
dlist_Remove(dl, indx)
    struct dlist           *dl;
    int                     indx;
{
    struct dlistEntry      *entry = nthEntry(dl, indx);

    if (entry->prev == -1)
	dl->head = entry->next;
    else
	(nthEntry(dl, entry->prev))->next = entry->next;
    if (entry->next == -1)
	dl->tail = entry->prev;
    else
	(nthEntry(dl, entry->next))->prev = entry->prev;
    if (dl->freeHead == -1)
	dl->freeHead = dl->freeTail = indx;
    else {
	entry->next = dl->freeHead;
	entry->prev = -1;
	dl->freeHead = indx;
    }
    --(dl->length);
}

VPTR
dlist_HeadElt(dl)
    struct dlist *dl;
{
    return (dlist_Nth(dl, dlist_Head(dl)));
}

VPTR
dlist_TailElt(dl)
    struct dlist *dl;
{
    return (dlist_Nth(dl, dlist_Tail(dl)));
}

void
dlist_Map(dl, fn, data)
    struct dlist *dl;
    void (*fn) NP((VPTR, VPTR));
    VPTR data;
{
    int i;

    for (i = dlist_Head(dl); i >= 0; i = dlist_Next(dl, i))
	(*fn)(dlist_Nth(dl, i), data);
}

#ifdef NOT_YET
static void
swap(dl, a, b)			/* swap a's and b's positions in the list */
    struct dlist           *dl;	/* will come in handy when qsort is written */
    int                     a, b;
{
    struct dlistEntry      *ae = nthEntry(dl, a);
    struct dlistEntry      *be = nthEntry(dl, b);
    struct dlistEntry      *ape, *ane, *bpe, *bne;
    int                     an = ae->next, ap = ae->prev;
    int                     bn = be->next, bp = be->prev;

    ape = ane = bpe = bne = (struct dlistEntry *) 0;
    if (an >= 0)
	ane = nthEntry(dl, an);
    if (ap >= 0)
	ape = nthEntry(dl, ap);
    if (bn >= 0)
	bne = nthEntry(dl, bn);
    if (bp >= 0)
	bpe = nthEntry(dl, bp);
    ae->next = bn;
    ae->prev = bp;
    be->next = an;
    be->prev = ap;
    if (ape)
	ape->next = b;
    else
	dl->head = b;
    if (ane)
	ane->prev = b;
    else
	dl->tail = b;
    if (bpe)
	bpe->next = a;
    else
	dl->head = a;
    if (bne)
	bne->prev = a;
    else
	dl->tail = a;
}
#endif /* NOT_YET */
