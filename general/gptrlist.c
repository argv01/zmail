#include <gptrlist.h>
#include <excfns.h>

#ifndef lint
static const char gptrlist_rcsid[] =
    "$Id: gptrlist.c,v 2.4 1995/02/09 04:36:26 bobg Exp $";
#endif

int
gptrlist_Add(gpl, elt)
struct gptrlist *gpl;
VPTR elt;
{
    return glist_Add(&gpl->gl, &elt);
}

int
gintlist_Add(gil, elt)
struct gintlist *gil;
long elt;
{
    return glist_Add(&gil->gl, &elt);
}

void
gptrlist_Set(gpl, n, elt)
struct gptrlist *gpl;
int n;
VPTR elt;
{
    glist_Set(&gpl->gl, n, &elt);
}

void
gintlist_Set(gil, n, elt)
struct gintlist *gil;
int n;
long elt;
{
    glist_Set(&gil->gl, n, &elt);
}

int
gptrlist_FindFrom(gpl, elt, start)
struct gptrlist *gpl;
VPTR elt;
int start;
{
    int l = gptrlist_Length(gpl);
    
    for (; start < l; start++)
	if (gptrlist_Nth(gpl, start) == elt)
	    return start;
    return -1;
}

int
gptrlist_RemoveElt(gpl, elt)
struct gptrlist *gpl;
VPTR elt;
{
    int pos = gptrlist_Find(gpl, elt);

    if (pos < 0) return 0;
    gptrlist_Remove(gpl, pos);
    return 1;
}
