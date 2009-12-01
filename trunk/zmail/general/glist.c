/*
 * $RCSfile: glist.c,v $
 * $Revision: 2.24 $
 * $Date: 1995/07/14 04:12:01 $
 * $Author: schaefer $
 */

#include <glist.h>
#include <excfns.h>
#include "bfuncs.h"

#ifndef lint
static const char glist_rcsid[] =
    "$Id: glist.c,v 2.24 1995/07/14 04:12:01 schaefer Exp $";
#endif /* lint */

#undef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

void
glist_Destroy(gl)
    struct glist *gl;
{
    if (gl->elts)
	free(gl->elts);
}

void
glist_CleanDestroy(gl, final)
    struct glist *gl;
    void (*final) NP((GENERIC_POINTER_TYPE *));
{
    int i;

    for (i = 0; i < glist_Length(gl); ++i)
	(*final)(glist_Nth(gl, i));
    glist_Destroy(gl);
}

GENERIC_POINTER_TYPE *
glist_GiveUpList(gl)
    struct glist *gl;
{
    return (gl->elts);
}

void
glist_Init(gl, eltSize, growSize)
    struct glist           *gl;
    int                     eltSize, growSize;
{
    gl->used = gl->allocated = 0;
    gl->eltSize = eltSize;
    gl->growSize = growSize;
    gl->elts = NULL;
}

#define Elts ((char *) gl->elts)

GENERIC_POINTER_TYPE *
glist_Nth(gl, n)
    struct glist           *gl;
    int                     n;
{
    return (Elts + (n * MALIGN(gl->eltSize)));
}

GENERIC_POINTER_TYPE *
glist_Last(gl)
    struct glist *gl;
{
    return (glist_Nth(gl, gl->used - 1));
}

static void
glist_Grow(gl, min)
    struct glist *gl;
    int min;
{
    int growsize = MAX(min, gl->allocated + gl->growSize);

    if (gl->allocated)
	gl->elts = ((GENERIC_POINTER_TYPE *)
		    erealloc(gl->elts, growsize * MALIGN(gl->eltSize),
			     "glist_Grow"));
    else
	gl->elts = ((GENERIC_POINTER_TYPE *)
		    emalloc(growsize * MALIGN(gl->eltSize),
			    "glist_Grow"));
    gl->allocated = growsize;
}

int
glist_Add(gl, elt)
    struct glist *gl;
    const GENERIC_POINTER_TYPE *elt;
{
    if (gl->used == gl->allocated)
	glist_Grow(gl, 0);
    if (elt)
	bcopy(elt,
	      Elts + (gl->used * MALIGN(gl->eltSize)),
	      gl->eltSize);
    else
	bzero(Elts + (gl->used * MALIGN(gl->eltSize)),
	      gl->eltSize);
    return ((gl->used)++);
}

void
glist_Set(gl, n, elt)
    struct glist *gl;
    int n;
    const GENERIC_POINTER_TYPE *elt;
{
    if (n >= gl->used) {
	if (n >= gl->allocated)
	    glist_Grow(gl, n + 1);
	gl->used = n + 1;
    }
    if (elt)
	bcopy(elt,
	      Elts + (n * MALIGN(gl->eltSize)),
	      gl->eltSize);
    else
	bzero(Elts + (n * MALIGN(gl->eltSize)),
	      gl->eltSize);
}

#define SWAPBUF 64

void
glist_Swap(gl, m, n)
    struct glist *gl;
    int m, n;
{
    char buf[SWAPBUF];
    int swapped = 0, toswap;

    while (swapped < gl->eltSize) {
	if ((toswap = (gl->eltSize - swapped)) > SWAPBUF)
	    toswap = SWAPBUF;
#ifndef WIN16
	bcopy(Elts + (m * MALIGN(gl->eltSize)) + swapped, buf, toswap);
	bcopy(Elts + (n * MALIGN(gl->eltSize)) + swapped,
	      Elts + (m * MALIGN(gl->eltSize)) + swapped,
	      toswap);
	bcopy(buf, Elts + (n * MALIGN(gl->eltSize)) + swapped, toswap);
#else
	bcopy(Elts + (m * MALIGN(gl->eltSize)) + swapped, buf, (short) toswap);
	bcopy(Elts + (n * MALIGN(gl->eltSize)) + swapped,
	      Elts + (m * MALIGN(gl->eltSize)) + swapped,
	      (short) toswap);
	bcopy(buf, Elts + (n * MALIGN(gl->eltSize)) + swapped, (short) toswap);
#endif /* !WIN16 */
	swapped += toswap;
    }
}

void
glist_Sort(g, compare)
    struct glist *g;
    int (*compare) NP((const GENERIC_POINTER_TYPE *,
		       const GENERIC_POINTER_TYPE *));
{
    qsort(g->elts, g->used, MALIGN(g->eltSize),
	  (int (*) NP((CVPTR, CVPTR))) compare);
}

void
glist_Insert(gl, elt, pos)
    struct glist *gl;
    const GENERIC_POINTER_TYPE *elt;
    int pos;
{
    if (pos < 0)
	pos = glist_Length(gl);
    if (pos == glist_Length(gl)) {
	glist_Add(gl, elt);
    } else {
	if (gl->used == gl->allocated)
	    glist_Grow(gl, 0);	/* This must happen first.  If it doesn't,
				 * then between the time glist_Nth is called
				 * in the following line, and the time
				 * glist_Add adds it, a realloc could
				 * happen, rendering the return value
				 * from glist_Nth invalid.
				 */
	/* Shift the last element one slot to the "right" */
	glist_Add(gl, glist_Nth(gl, glist_Length(gl) - 1));
	/* Shift everything from the target position on up one
	 * slot to the "right" (but do not redundantly shift the
	 * last element over). */
	if ((glist_Length(gl) - pos) >= 2)
	    safe_bcopy(Elts + (pos * MALIGN(gl->eltSize)),
		       Elts + ((pos + 1) * MALIGN(gl->eltSize)),
		       (glist_Length(gl) - pos - 2) * MALIGN(gl->eltSize));
	/* Now put the new element in the hole */
	glist_Set(gl, pos, elt);
    }
}

void
glist_Remove(gl, n)
    struct glist *gl;
    int n;
{
    int i;

    for (i = n; i < glist_Length(gl) - 1; ++i) {
	glist_Set(gl, i, glist_Nth(gl, i + 1));
    }
    glist_Pop(gl);
}

int
glist_Bsearch(gl, probe, cmp)
    struct glist *gl;
    const GENERIC_POINTER_TYPE * probe;
    int (*cmp) P((const GENERIC_POINTER_TYPE *,
		  const GENERIC_POINTER_TYPE *));
{
    int l = 0, u = glist_Length(gl) - 1, m;
    int c;

    while (l <= u) {
	m = (l + u) / 2;
	c = (*cmp)(probe, glist_Nth(gl, m));
	if (c < 0) {
	    u = m - 1;
	} else if (c > 0) {
	    l = m + 1;
	} else {
	    return (m);
	}
    }
    return (-1);
}

void
glist_Map(gl, fn, data)
    struct glist *gl;
    void (*fn) NP((VPTR, VPTR));
    VPTR data;
{
    int i;

    for (i = 0; i < glist_Length(gl); ++i) {
	(*fn)(glist_Nth(gl, i), data);
    }
}
