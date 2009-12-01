/*
 * $RCSfile: sklist.h,v $
 * $Revision: 2.12 $
 * $Date: 1995/09/20 06:20:51 $
 * $Author: liblit $
 */

#ifndef SKLIST_H
#define SKLIST_H

#include <glist.h>

struct sklist {
    struct glist levels;
    int eltsize, (*cmp) P((const GENERIC_POINTER_TYPE *,
			   const GENERIC_POINTER_TYPE *));
    struct {
	int numerator, denominator;
    } p;			/* probability of going up 1 level */
    GENERIC_POINTER_TYPE *lastfind;
    int length;
};

#define sklist_Length(s) ((s)->length)
#define sklist_EmptyP(s) (sklist_Length(s)==0)

extern GENERIC_POINTER_TYPE *sklist_Find P((struct sklist *,
					    const GENERIC_POINTER_TYPE *,
					    int));
extern void sklist_CleanDestroy P((struct sklist *,
				   void (*) NP((GENERIC_POINTER_TYPE *))));
extern void sklist_Destroy P((struct sklist *));
extern void sklist_Init P((struct sklist *,
			   int, int (*) (const GENERIC_POINTER_TYPE *,
					 const GENERIC_POINTER_TYPE *),
			   int, int));
extern GENERIC_POINTER_TYPE *sklist_Insert P((struct sklist *,
					      const GENERIC_POINTER_TYPE *));
extern void sklist_CleanRemove P((struct sklist *,
				  const GENERIC_POINTER_TYPE *,
				  void (*) NP((GENERIC_POINTER_TYPE *))));
extern void sklist_Remove P((struct sklist *,
			     const GENERIC_POINTER_TYPE *));
extern GENERIC_POINTER_TYPE *sklist_LastMiss P((struct sklist *));
extern GENERIC_POINTER_TYPE *sklist_Next P((struct sklist *,
					    const GENERIC_POINTER_TYPE *));
extern GENERIC_POINTER_TYPE *sklist_First P((struct sklist *));

extern void sklist_Map P((struct sklist *, void (*) NP((VPTR, VPTR)), VPTR));

#ifdef HAVE_RANDOM
# include <math.h>
# define Srandom(s) (srandom(s))
#else /* HAVE_RANDOM */
# include <stdlib.h>
# define Srandom(s) (srand(s))
#endif /* HAVE_RANDOM */

#define sklist_FOREACH(s,t,v) \
    for (v = (t *) sklist_First(s); v; v = (t *) sklist_Next((s), v))

#define sklist_FOREACH2(s,t,v,w) \
    for (v = (t *) sklist_First(s); \
	 v && ((w = (t *) sklist_Next((s), v)), 1); \
	 v = w)

#endif /* SKLIST_H */
