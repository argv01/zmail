/*
 * $RCSfile: dlist.h,v $
 * $Revision: 2.16 $
 * $Date: 1995/11/10 20:20:06 $
 * $Author: spencer $
 */

#ifndef DLIST_H
# define DLIST_H

# include <general.h>
# include <glist.h>

struct dlistEntry {
    int next, prev;
};

struct dlist {
    struct glist entries;
    int head, tail, freeHead, freeTail, eltSize, length;
};

# define dlist_EmptyP(d) (((d)->head)<0)

# define dlist_Head(d) ((d)->head)
# define dlist_Tail(d) ((d)->tail)

# define dlist_Length(d) ((d)->length)

/* XXX casting away const */
# define dlist_Next(d,i) \
    (((struct dlistEntry *) glist_Nth((struct glist *) \
				      &(((d))->entries), (i)))->next)
# define dlist_Prev(d,i) \
    (((struct dlistEntry *) glist_Nth((struct glist *) \
				      &((d)->entries),(i)))->prev)

# define dlist_Nth(d,n) \
    ((GENERIC_POINTER_TYPE *) \
     (((char *) \
       glist_Nth(&((d)->entries), (n))) + ALIGN(sizeof(struct dlistEntry))))

# define dlist_FOREACH(d,t,v,i) \
    for (i = dlist_Head(d); \
	 (i >= 0) ? ((v = (t *) dlist_Nth((d), i)), 1) : 0; \
	 i = dlist_Next((d), i))

/* dlist_FOREACH2 is for when the loop body might remove
 * the i'th element of d (before dlist_Next can be computed).
 */
# define dlist_FOREACH2(d,t,v,i,j) \
    for (i = dlist_Head(d); \
         (i >= 0) ? ((v = (t *)dlist_Nth((d), i)), j = dlist_Next((d), i), 1) \
                  : 0; \
	 i = j)

extern void dlist_CleanDestroy P((struct dlist *,
				  void (*) NP((GENERIC_POINTER_TYPE *))));
extern void dlist_Destroy P((struct dlist *));

extern int dlist_Append       P((struct dlist *,
				 const GENERIC_POINTER_TYPE *));
extern int dlist_InsertAfter  P((struct dlist *, int,
				 const GENERIC_POINTER_TYPE *));
extern int dlist_InsertBefore P((struct dlist *, int,
				 const GENERIC_POINTER_TYPE *));
extern int dlist_Prepend      P((struct dlist *,
				 const GENERIC_POINTER_TYPE *));
extern void dlist_Init        P((struct dlist *, int, int));
extern void dlist_Remove      P((struct dlist *, int));
extern void dlist_Replace     P((struct dlist *, int,
				 const GENERIC_POINTER_TYPE *));

extern VPTR dlist_HeadElt P((struct dlist *));
extern VPTR dlist_TailElt P((struct dlist *));

extern void dlist_Map P((struct dlist *, void (*) NP((VPTR, VPTR)), VPTR));

#endif /* DLIST_H */
