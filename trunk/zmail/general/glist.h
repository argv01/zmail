/*
 * $RCSfile: glist.h,v $
 * $Revision: 2.17 $
 * $Date: 1995/04/23 01:45:03 $
 * $Author: bobg $
 */

#ifndef GLIST_H
# define GLIST_H

# include <general.h>

# define glist_EmptyP(gl) (((gl)->used)==0)
# define glist_Length(gl) ((gl)->used)

# define glist_Pop(gl) (--((gl)->used))
# define glist_Truncate(gl,num) (((gl)->used)=(num))

struct glist {
    int used, allocated, eltSize, growSize;
    GENERIC_POINTER_TYPE *elts;
};

extern int glist_Add P((struct glist *, const GENERIC_POINTER_TYPE *));
extern void glist_CleanDestroy P((struct glist *,
				  void (*) NP((GENERIC_POINTER_TYPE *))));
extern void glist_Destroy P((struct glist *));
extern GENERIC_POINTER_TYPE *glist_GiveUpList P((struct glist *));
extern void glist_Init P((struct glist *, int, int));
extern void glist_Insert P((struct glist *,
			    const GENERIC_POINTER_TYPE *,
			    int));
extern GENERIC_POINTER_TYPE *glist_Last P((struct glist *));
extern GENERIC_POINTER_TYPE *glist_Nth P((struct glist *, int));
extern void glist_Remove P((struct glist *, int));
extern void glist_Set P((struct glist *, int, const GENERIC_POINTER_TYPE *));
extern void glist_Sort P((struct glist *,
			  int (*) NP((const GENERIC_POINTER_TYPE *,
				      const GENERIC_POINTER_TYPE *))));
extern void glist_Swap P((struct glist *, int, int));

extern int glist_Bsearch P((struct glist *, const GENERIC_POINTER_TYPE *,
			    int (*) NP((const GENERIC_POINTER_TYPE *,
					const GENERIC_POINTER_TYPE *))));

extern void glist_Map P((struct glist *, void (*) NP((VPTR, VPTR)), VPTR));

# define glist_FOREACH(g,t,v,i) \
    for (i = 0; \
	 (i < glist_Length(g)) && ((v = (t *) glist_Nth((g), i)), 1); \
	 ++i)

#endif /* GLIST_H */
