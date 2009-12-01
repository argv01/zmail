#ifndef _GPTRLIST_H_
#define _GPTRLIST_H_

/*
 * $RCSfile: gptrlist.h,v $
 * $Revision: 2.3 $
 * $Date: 1994/03/15 03:40:41 $
 * $Author: pf $
 */

#include <glist.h>

struct gptrlist {
    struct glist gl;
};
struct gintlist {
    struct glist gl;
};

extern int gptrlist_Add P ((struct gptrlist *, VPTR));
extern void gptrlist_Set P ((struct gptrlist *, int, VPTR));
extern int gptrlist_FindFrom P ((struct gptrlist *, VPTR, int));
extern int gptrlist_RemoveElt P ((struct gptrlist *, VPTR));

#define gptrlist_Find(GP, ELT) gptrlist_FindFrom(GP, ELT, 0)

extern int gintlist_Add P ((struct gintlist *, long));
extern void gintlist_Set P ((struct gintlist *, int, long));

#define gintlist_Find(GP, ELT) gptrlist_Find(GP, (VPTR) ELT)
#define gintlist_FindFrom(GP, ELT, FM) gptrlist_Find(GP, (VPTR) ELT, FM)
#define gintlist_RemoveElt(GP, ELT) gintlist_RemoveElt(GP, (VPTR) ELT)

#define gptrlist_Init(GL, GS) (glist_Init(&(GL)->gl, sizeof(VPTR), GS))
#define gptrlist_Destroy(GL) (glist_Destroy(&(GL)->gl))
#define gptrlist_EmptyP(GL) (glist_EmptyP(&(GL)->gl))
#define gptrlist_Length(GL) (glist_Length(&(GL)->gl))
#define gptrlist_Pop(GL) (glist_Pop(&(GL)->gl))
#define gptrlist_GiveUpList(GL) (glist_GiveUpList(&(GL)->gl))
#define gptrlist_Last(GL) (*(VPTR *)glist_Last(&(GL)->gl))
#define gptrlist_Nth(GL, N) (*(VPTR *)glist_Nth(&(GL)->gl, (N)))
#define gptrlist_Remove(GL, N) (glist_Remove(&(GL)->gl, (N)))

#define gptrlist_FOREACH(g,t,v,i) \
    for (i = 0; \
	 (i < gptrlist_Length(g)) && ((v = (t *) gptrlist_Nth((g), i)), 1); \
	 ++i)

#define gintlist_Init(GL, GS) (glist_Init(&(GL)->gl, sizeof(VPTR), GS))
#define gintlist_Destroy(GL) (glist_Destroy(&(GL)->gl))
#define gintlist_EmptyP(GL) (glist_EmptyP(&(GL)->gl))
#define gintlist_Length(GL) (glist_Length(&(GL)->gl))
#define gintlist_Pop(GL) (glist_Pop(&(GL)->gl))
#define gintlist_GiveUpList(GL) (glist_GiveUpList(&(GL)->gl))
#define gintlist_Last(GL) (*(long *)glist_Last(&(GL)->gl))
#define gintlist_Nth(GL, N) (*(long *)glist_Nth(&(GL)->gl, (N)))
#define gintlist_Remove(GL, N) (glist_Remove(&(GL)->gl, (N)))

#define gintlist_FOREACH(g,v,i) \
    for (i = 0; \
	 (i < gptrlist_Length(g)) && ((v = gptrlist_Nth((g), i)), 1); \
	 ++i)

#endif /* _GPTRLIST_H_ */
