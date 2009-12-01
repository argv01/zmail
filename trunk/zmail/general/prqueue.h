/*
 * $RCSfile: prqueue.h,v $
 * $Revision: 2.8 $
 * $Date: 1995/02/17 02:45:57 $
 * $Author: bobg $
 */

#ifndef PRQUEUE_H
# define PRQUEUE_H

# include <general.h>
# include <glist.h>

# define prqueue_EmptyP(p) (glist_EmptyP(&((p)->entries)))
# define prqueue_Head(p) (glist_Nth(&((p)->entries),0))

struct prqueue {
    struct glist            entries;
    int (*compare) NP((const GENERIC_POINTER_TYPE *,
		       const GENERIC_POINTER_TYPE *));
};

extern void prqueue_CleanDestroy P((struct prqueue *,
				    void (*) NP((GENERIC_POINTER_TYPE *))));
extern void prqueue_Destroy P((struct prqueue *));
extern void prqueue_Add P((struct prqueue *,
			   GENERIC_POINTER_TYPE *));
extern void prqueue_Init P((struct prqueue *,
			    int (*) NP((const GENERIC_POINTER_TYPE *,
					const GENERIC_POINTER_TYPE *)),
			    int,
			    int));
extern void prqueue_Remove P((struct prqueue *));

#endif /* PRQUEUE_H */
