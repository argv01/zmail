/* SPAMM -- the Simple, Poor-man's Allocator/Memory-Manager
 * (with garbage collection)
 *
 * $RCSfile: spamm.c,v $
 * $Revision: 2.10 $
 * $Date: 1995/02/17 02:46:04 $
 * $Author: bobg $
 */

#include <spamm.h>

#include <except.h>
#include <excfns.h>
#include <dlist.h>

#ifndef lint
static const char spamm_rcsid[] =
    "$Id: spamm.c,v 2.10 1995/02/17 02:46:04 bobg Exp $";
#endif /* lint */

#define pageisempty(p) (((p)->inUse)==((unsigned long)0))

#define pageofobject(obj) \
    (*((struct spamm_ObjectPage **) \
       (((char *) (obj)) - ALIGN(sizeof (struct spamm_ObjectPage *)))))

struct dlist spamm_ObjectRoot;
static struct glist spamm_ObjectPools;

static int gcsuspend = 0;
static int gcpending = 0;

void (*spamm_GcStart)(), (*spamm_GcEnd)();

static int
pageisfull(p)
    struct spamm_ObjectPage *p;
{
    return ((p->elts == ULBITS) ?
	    (p->inUse == ~((unsigned long) 0)) :
	    (p->inUse == ((1 << p->elts) - 1)));
}

void
spamm_Initialize()
{
    dlist_Init(&spamm_ObjectRoot, (sizeof (GENERIC_POINTER_TYPE * *)), 32);
    glist_Init(&spamm_ObjectPools, (sizeof (struct spamm_ObjectPool *)), 4);
    spamm_GcStart = 0;
    spamm_GcEnd = 0;
}

int
spamm_Root(ptr)
    GENERIC_POINTER_TYPE **ptr;
{
    dlist_Prepend(&spamm_ObjectRoot, &ptr);
    return (dlist_Head(&spamm_ObjectRoot));
}

static int
ffb(ul, v, bits)
    unsigned long ul;
    int v, bits;
{
    int i;

    if (v) {
	for (i = 0; i < bits; ++i)
	    if (ul & (1 << i))
		return (i);
    } else {
	for (i = 0; i < bits; ++i)
	    if (!(ul & (1 << i)))
		return (i);
    }
    return (-1);
}

static void
spamm_ObjectPage_Init(op, size, elts, trace, reclaim)
    struct spamm_ObjectPage *op;
    int size, elts;
    void (*trace) P((GENERIC_POINTER_TYPE *));
    void (*reclaim) P((GENERIC_POINTER_TYPE *));
{
    int i;
    char *p;

    op->gcMarks = (unsigned long) 0;
    op->inUse = (unsigned long) 0;
    op->size = MALIGN(size + ALIGN(sizeof (struct spamm_ObjectPage *)));
    op->elts = elts;
    op->data = (GENERIC_POINTER_TYPE *) emalloc(elts * op->size,
						"spamm_ObjectPage_Init");
    op->trace = trace;
    op->reclaim = reclaim;
    for (i = 0, p = op->data; i < elts; ++i, p += op->size)
	*((struct spamm_ObjectPage **) p) = op;
}

static GENERIC_POINTER_TYPE *
spamm_ObjectPage_Nth(op, n)
    struct spamm_ObjectPage *op;
    int n;
{
    return (op->data + (n * op->size) +
	    ALIGN(sizeof (struct spamm_ObjectPage *)));
}

static GENERIC_POINTER_TYPE *
spamm_ObjectPage_Allocate(op)
    struct spamm_ObjectPage *op;
{
    int which = ffb(op->inUse, 0, op->elts);

    if (which >= 0) {
	op->inUse |= (1 << which);
	op->gcMarks &= ~(1 << which);
	return (spamm_ObjectPage_Nth(op, which));
    }
    return ((GENERIC_POINTER_TYPE *) 0);
}

static void
spamm_ObjectPage_DeallocateIndex(op, indx)
    struct spamm_ObjectPage *op;
    int indx;
{
    if (op->reclaim)
	(*(op->reclaim))(spamm_ObjectPage_Nth(op, indx));
    op->inUse &= ~(1 << indx);
}

static void
spamm_ObjectPage_Deallocate(op, ptr)
    struct spamm_ObjectPage *op;
    char *ptr;
{
    spamm_ObjectPage_DeallocateIndex(op, (ptr - op->data) / op->size);
}

void
spamm_Trace(obj)
    GENERIC_POINTER_TYPE *obj;
{
    struct spamm_ObjectPage *page;
    int which;

    if (!obj)
	return;
    page = pageofobject(obj);
    which = (((char *) obj) - page->data) / page->size;
    if (page->gcMarks & (1 << which))
	return;
    page->gcMarks |= (1 << which);
    if (page->trace)
	(*(page->trace))(obj);
}

static void
spamm_ObjectPage_Destroy(page)
    struct spamm_ObjectPage *page;
{
    free(page->data);
}

void
spamm_CollectGarbage()
{
    int rootindex, poolindex, pageindex, i, j, used, allocated;
    GENERIC_POINTER_TYPE **ptr, ***ptrp;
    struct spamm_ObjectPage *page, **opp;
    struct spamm_ObjectPool *pool, **poolp;

    if (gcpending)
	return;
    gcpending = 1;
    TRY {
	if (spamm_GcStart)
	    (*(spamm_GcStart))();

	/* Trace and mark all rooted objects */
	dlist_FOREACH(&spamm_ObjectRoot, GENERIC_POINTER_TYPE **,
		      ptrp, rootindex) {
	    ptr = *ptrp;
	    if (ptr)
		spamm_Trace(*ptr);
	}

	/* Sweep all pools */
	glist_FOREACH(&spamm_ObjectPools, struct spamm_ObjectPool *,
		      poolp, poolindex) {
	    pool = *poolp;
	    allocated = 0;
	    used = 0;

	    /* Sweep all neitherPages in this pool */
	    dlist_FOREACH2(&(pool->neitherPages), struct spamm_ObjectPage *,
			  opp, pageindex, j) {
		page = *opp;
		for (i = 0; i < page->elts; ++i) {
		    if (page->inUse & (1 << i)) {
			if (page->gcMarks & (1 << i))
			    ++used;
			else
			    spamm_ObjectPage_DeallocateIndex(page, i);
		    }
		}
		page->gcMarks = (unsigned long) 0;
		if (pageisempty(page)) {
		    dlist_Prepend(&(pool->emptyPages),
				  &page);
		    dlist_Remove(&(pool->neitherPages), pageindex);
		}
	    }

	    /* Sweep all full pages in this pool */
	    dlist_FOREACH2(&(pool->fullPages), struct spamm_ObjectPage *,
			   opp, pageindex, j) {
		page = *opp;
		for (i = 0; i < page->elts; ++i) {
		    if (page->inUse & (1 << i)) {
			if (page->gcMarks & (1 << i))
			    ++used;
			else
			    spamm_ObjectPage_DeallocateIndex(page, i);
		    }
		}
		page->gcMarks = (unsigned long) 0;
		if (!pageisfull(page)) {
		    dlist_Prepend((pageisempty(page) ?
				   &(pool->emptyPages) :
				   &(pool->neitherPages)),
				  &page);
		    dlist_Remove(&(pool->fullPages), pageindex);
		}
	    }

	    /* Recompute total allocation, free some empty pages
	       if appropriate */
	    allocated = (pool->pageelts *
			 (dlist_Length(&(pool->emptyPages))
			  + dlist_Length(&(pool->neitherPages))
			  + dlist_Length(&(pool->fullPages))));
	    while (!dlist_EmptyP(&(pool->emptyPages))
		   && (used < (allocated / 4))) {
		pageindex = dlist_Head(&(pool->emptyPages));
		page = *((struct spamm_ObjectPage **)
			 dlist_Nth(&(pool->emptyPages), pageindex));
		dlist_Remove(&(pool->emptyPages), pageindex);
		spamm_ObjectPage_Destroy(page);
		allocated -= pool->pageelts;
	    }
	}
	if (spamm_GcEnd)
	    (*(spamm_GcEnd))();
    } FINALLY {
	gcpending = 0;
    } ENDTRY;
}

GENERIC_POINTER_TYPE *
spamm_Allocate(op)
    struct spamm_ObjectPool *op;
EXC_BEGIN
{
    GENERIC_POINTER_TYPE * result;
    struct spamm_ObjectPage *page;
    struct dlist *dl;
    int indx;

    /* Get an element from a neither page if possible */
    if (!dlist_EmptyP(dl = &(op->neitherPages))) {
	indx = dlist_Head(dl);
	page = *((struct spamm_ObjectPage **) dlist_Nth(dl, indx));
	result = spamm_ObjectPage_Allocate(page);
	if (pageisfull(page)) {
	    dlist_Remove(dl, indx);
	    dlist_Prepend(&(op->fullPages), &page);
	}
	return(result);
    }

    /* No neither pages?  Get an element from an empty page */
    if (!dlist_EmptyP(dl = &(op->emptyPages))) {
	indx = dlist_Head(dl);
	page = *((struct spamm_ObjectPage **) dlist_Nth(dl, indx));
	result = spamm_ObjectPage_Allocate(page);
	dlist_Remove(dl, indx);
	dlist_Prepend((pageisfull(page) ?
		       &(op->fullPages) : &(op->neitherPages)),
		      &page);
	return(result);
    }

    /* No empty pages either?  Collect garbage (unless we're suspending) */
    if (!gcsuspend) {
	spamm_CollectGarbage();

	/* Try neither page again */
	if (!dlist_EmptyP(dl = &(op->neitherPages))) {
	    indx = dlist_Head(dl);
	    page = *((struct spamm_ObjectPage **) dlist_Nth(dl, indx));
	    result = spamm_ObjectPage_Allocate(page);
	    if (pageisfull(page)) {
		dlist_Remove(dl, indx);
		dlist_Prepend(&(op->fullPages), &page);
	    }
	    return(result);
	}

	/* Try empty page again */
	if (!dlist_EmptyP(dl = &(op->emptyPages))) {
	    indx = dlist_Head(dl);
	    page = *((struct spamm_ObjectPage **) dlist_Nth(dl, indx));
	    result = spamm_ObjectPage_Allocate(page);
	    dlist_Remove(dl, indx);
	    dlist_Prepend((pageisfull(page) ?
			   &(op->fullPages) : &(op->neitherPages)),
			  &page);
	    return(result);
	}
    }

    /* Looks like we'll have to malloc */
    TRY {
	page = (struct spamm_ObjectPage *)
	    emalloc((sizeof (struct spamm_ObjectPage)),
		    "spamm_ObjectPool_Allocate");
	spamm_ObjectPage_Init(page, op->size, op->pageelts,
			      op->trace, op->reclaim);
	result = spamm_ObjectPage_Allocate(page);
	dlist_Prepend(&(op->neitherPages), &page);
	EXC_RETURNVAL(GENERIC_POINTER_TYPE *, result);
    } EXCEPT(strerror(ENOMEM)) {
	/* Malloc failed; maybe a GC will help (if it wasn't already tried) */
	if (gcsuspend) {
	    spamm_CollectGarbage();

	    /* Try the neither pages once more */
	    if (!dlist_EmptyP(dl = &(op->neitherPages))) {
		indx = dlist_Head(dl);
		page = *((struct spamm_ObjectPage **) dlist_Nth(dl, indx));
		result = spamm_ObjectPage_Allocate(page);
		if (pageisfull(page)) {
		    dlist_Remove(dl, indx);
		    dlist_Prepend(&(op->fullPages), &page);
		}
		EXC_RETURNVAL(GENERIC_POINTER_TYPE *, result);
	    }

	    /* Try the empty pages once more */
	    if (!dlist_EmptyP(dl = &(op->emptyPages))) {
		indx = dlist_Head(dl);
		page = *((struct spamm_ObjectPage **) dlist_Nth(dl, indx));
		result = spamm_ObjectPage_Allocate(page);
		dlist_Remove(dl, indx);
		dlist_Prepend((pageisfull(page) ?
			       &(op->fullPages) : &(op->neitherPages)),
			      &page);
		EXC_RETURNVAL(GENERIC_POINTER_TYPE *, result);
	    }
	}

	/* We're genuinely out of memory */
	PROPAGATE();
    } ENDTRY;
} EXC_END

void
spamm_GcSuspend()
{
    ++gcsuspend;
}

void
spamm_GcUnsuspend()
{
    if (gcsuspend > 0)
	--gcsuspend;
}

int
spamm_RootList(VA_ALIST(GENERIC_POINTER_TYPE **ptr))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(GENERIC_POINTER_TYPE **ptr);
    int i = 0;

    VA_START(ap, GENERIC_POINTER_TYPE **, ptr);
    do {
	++i;
	(void) spamm_Root(ptr);
    } while (ptr = VA_ARG(ap, GENERIC_POINTER_TYPE **));
    VA_END(ap);
    return (i);
}

int
spamm_PoolStats(pool, empty, neither, full)
    struct spamm_ObjectPool *pool;
    int *empty, *neither, *full;
{
    int result = dlist_Length(&(pool->emptyPages));

    result += dlist_Length(&(pool->neitherPages));
    result += dlist_Length(&(pool->fullPages));
    if (empty)
	*empty = dlist_Length(&(pool->emptyPages));
    if (neither)
	*neither = dlist_Length(&(pool->neitherPages));
    if (full)
	*full = dlist_Length(&(pool->fullPages));
    return (result);
}

void
spamm_InitPool(pool, size, elts, trace, reclaim)
    struct spamm_ObjectPool *pool;
    int size, elts;
    void (*trace) P((GENERIC_POINTER_TYPE *));
    void (*reclaim) P((GENERIC_POINTER_TYPE *));
{
    if ((elts < 1) || (elts > ULBITS))
	RAISE(strerror(EINVAL), "spamm_NewPool");
    pool->size = size;
    pool->pageelts = elts;
    pool->trace = trace;
    pool->reclaim = reclaim;
    dlist_Init(&(pool->emptyPages),
	       (sizeof (struct spamm_ObjectPage *)), 8);
    dlist_Init(&(pool->fullPages),
	       (sizeof (struct spamm_ObjectPage *)), 8);
    dlist_Init(&(pool->neitherPages),
	       (sizeof (struct spamm_ObjectPage *)), 8);
    glist_Add(&spamm_ObjectPools, &pool);
}
