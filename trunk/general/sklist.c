/*
 * $RCSfile: sklist.c,v $
 * $Revision: 2.13 $
 * $Date: 1995/08/11 01:07:42 $
 * $Author: liblit $
 *
 * Skiplist implementation.
 *
 * As described in Communications of the ACM, Vol. 33, Number 6,
 * June 1990, in "Skip Lists:  A Probabilistic Alternative to
 * Balanced Trees" by William Pugh.
 */

#include <sklist.h>
#include <excfns.h>
#include "bfuncs.h"

#ifndef lint
static const char sklist_rcsid[] =
    "$Id";
#endif /* lint */

#define sklnode_nthptr(skl,p,n) \
    (((GENERIC_POINTER_TYPE (**)) \
      (((char *) (p)) + ALIGN((skl)->eltsize)))[(n)])

static int current_eltsize;
static int default_cmp P((const GENERIC_POINTER_TYPE *a,
			  const GENERIC_POINTER_TYPE *b));

static int
default_cmp(a, b)
    const GENERIC_POINTER_TYPE *a, *b;
{
#ifndef WIN16
    return (bcmp(a, b, current_eltsize));
#else
    return (bcmp(a, b, (size_t) current_eltsize));
#endif /* !WIN16 */
}

void
sklist_Init(skl, eltsize, cmp, num, denom)
    struct sklist *skl;
    int eltsize;
    int (*cmp) NP((const GENERIC_POINTER_TYPE *,
		   const GENERIC_POINTER_TYPE *));
    int num, denom;
{
    glist_Init(&(skl->levels), (sizeof (GENERIC_POINTER_TYPE *)), 4);
    skl->lastfind = (GENERIC_POINTER_TYPE *) 0;
    skl->eltsize = eltsize;
    skl->cmp = cmp ? cmp : default_cmp;
    skl->p.numerator = num;
    skl->p.denominator = denom;
    skl->length = 0;
}

static void
destroy(skl, final)
    struct sklist *skl;
    void (*final) NP((GENERIC_POINTER_TYPE *));
{
    GENERIC_POINTER_TYPE *node, *next;

    if (!glist_EmptyP(&(skl->levels))) {
	node = *((GENERIC_POINTER_TYPE **) glist_Nth(&(skl->levels), 0));
	while (node) {
	    next = sklnode_nthptr(skl, node, 0);
	    if (final)
		(*final)(node);
	    free(node);
	    node = next;
	}
    }
    glist_Destroy(&(skl->levels));
}

void
sklist_CleanDestroy(skl, final)
    struct sklist *skl;
    void (*final) NP((GENERIC_POINTER_TYPE *));
{
    destroy(skl, final);
}

void
sklist_Destroy(skl)
    struct sklist *skl;
{
    destroy(skl, 0);
}

#ifdef HAVE_RANDOM
# define Random() (random())
#else /* HAVE_RANDOM */
# define Random() (rand())
#endif /* HAVE_RANDOM */

static int
newlevel(num, denom, max)
    int num, denom, max;
{
    int result = 0;

    while ((result <= max) && ((Random() % denom) < num))
	++result;
    return (result);
}

static struct glist searchvector;
static int initialized = 0, foundlevel, searchvectorvalid;

static GENERIC_POINTER_TYPE *
search(skl, probe, findonly)
    struct sklist *skl;
    const GENERIC_POINTER_TYPE *probe;
    int findonly;
{
    int level = glist_Length(&(skl->levels)) - 1, cmp;
    GENERIC_POINTER_TYPE *node, *newnode, *found = (GENERIC_POINTER_TYPE *) 0;

    if (!findonly && !initialized) {
	initialized = 1;
	glist_Init(&searchvector, (sizeof (GENERIC_POINTER_TYPE *)), 4);
    }

    if (level < 0) {
	searchvectorvalid = 0;
	return (skl->lastfind = (GENERIC_POINTER_TYPE *) 0);
    }

    node = (GENERIC_POINTER_TYPE *) 0;
    do {
	if (newnode = (node ?
		       sklnode_nthptr(skl, node, level) :
		       *((GENERIC_POINTER_TYPE **) glist_Nth(&(skl->levels),
							     level)))) {
	    current_eltsize = skl->eltsize;
	    if ((cmp = ((newnode == found) ? 0 :
			(*(skl->cmp))(newnode, probe))) > 0) {
		/* We've passed it */
		if (!findonly)
		    glist_Set(&searchvector, level, &node);
		--level;
	    } else if (cmp < 0) {
		node = newnode;
	    } else {		/* found a match */
		if (!found) {	/* might have already found it at a
				 * higher level */
		    foundlevel = level;
		    found = newnode;
		    if (findonly)
			return (found);
		}
		glist_Set(&searchvector, level--, &node);
	    }
	} else {
	    if (!findonly)
		glist_Set(&searchvector, level, &node);
	    --level;
	}
    } while (level >= 0);
    searchvectorvalid = 1;
    return (skl->lastfind = found);
}

GENERIC_POINTER_TYPE *
sklist_Insert(skl, elt)
    struct sklist *skl;
    const GENERIC_POINTER_TYPE *elt;
{
    int level = newlevel(skl->p.numerator, skl->p.denominator,
			 glist_Length(&(skl->levels)) - 1);
    int i;
    GENERIC_POINTER_TYPE *node, *tmp;

    node = ((GENERIC_POINTER_TYPE *)
	    emalloc((ALIGN(skl->eltsize) +
		     ((level + 1) *
		      MALIGN(sizeof (GENERIC_POINTER_TYPE *)))),
		    "sklist_Insert"));
#ifndef WIN16
    bcopy(elt, node, skl->eltsize);
#else
    bcopy(elt, node, (size_t) skl->eltsize);
#endif /* !WIN16 */
    (void) search(skl, elt, 0);
    if (level == glist_Length(&(skl->levels))) {
	glist_Add(&(skl->levels), &node);
	sklnode_nthptr(skl, node, level) = (GENERIC_POINTER_TYPE *) 0;
	--level;
    }
    for (i = 0; i <= level; ++i) {
	if (tmp = *((GENERIC_POINTER_TYPE **) glist_Nth(&searchvector, i))) {
	    sklnode_nthptr(skl, node, i) = sklnode_nthptr(skl, tmp, i);
	    sklnode_nthptr(skl, tmp, i) = node;
	} else {
	    sklnode_nthptr(skl, node, i) = *((GENERIC_POINTER_TYPE **)
					     glist_Nth(&(skl->levels), i));
	    glist_Set(&(skl->levels), i, &node);
	}
    }
    ++(skl->length);
    return (node);
}

GENERIC_POINTER_TYPE *
sklist_Find(skl, probe, record)
    struct sklist *skl;
    const GENERIC_POINTER_TYPE *probe;
    int record;
{
    return (search(skl, probe, !record));
}

static void
Remove(skl, probe, final)
    struct sklist *skl;
    const GENERIC_POINTER_TYPE *probe;
    void (*final) NP((GENERIC_POINTER_TYPE *));
{
    GENERIC_POINTER_TYPE *node = search(skl,
					probe ? probe : skl->lastfind,
					0), *tmp;
    int i;

    if (!node)
	return;
    for (i = 0; i <= foundlevel; ++i) {
	if (tmp = *((GENERIC_POINTER_TYPE **) glist_Nth(&searchvector, i))) {
	    sklnode_nthptr(skl, tmp, i) = sklnode_nthptr(skl, node, i);
	} else {
	    glist_Set(&(skl->levels), i, &sklnode_nthptr(skl, node, i));
	}
    }
    while ((!glist_EmptyP(&(skl->levels)))
	   && (!*((GENERIC_POINTER_TYPE **) glist_Last(&(skl->levels)))))
	glist_Pop(&(skl->levels));
    if (final)
	(*final)(node);
    free(node);
    --(skl->length);
}

void
sklist_CleanRemove(skl, probe, final)
    struct sklist *skl;
    const GENERIC_POINTER_TYPE *probe;
    void (*final) NP((GENERIC_POINTER_TYPE *));
{
    Remove(skl, probe, final);
}

void
sklist_Remove(skl, probe)
    struct sklist *skl;
    const GENERIC_POINTER_TYPE *probe;
{
    Remove(skl, probe, 0);
}

GENERIC_POINTER_TYPE *
sklist_LastMiss(skl)
    struct sklist *skl;
{
    return (searchvectorvalid ?
	    *((GENERIC_POINTER_TYPE **) glist_Nth(&searchvector, 0)) :
	    0);
}

GENERIC_POINTER_TYPE *
sklist_Next(skl, elt)
    struct sklist *skl;
    const GENERIC_POINTER_TYPE *elt;
{
    return (sklnode_nthptr(skl, elt, 0));
}

GENERIC_POINTER_TYPE *
sklist_First(skl)
    struct sklist *skl;
{
    if (glist_EmptyP(&(skl->levels)))
	return (0);
    return (*((GENERIC_POINTER_TYPE **) glist_Nth(&(skl->levels), 0)));
}

void
sklist_Map(skl, fn, data)
    struct sklist *skl;
    void (*fn) NP((VPTR, VPTR));
    VPTR data;
{
    VPTR elt;
    
    for (elt = sklist_First(skl); elt; elt = sklist_Next(skl, elt))
	(*fn)(elt, data);
}
