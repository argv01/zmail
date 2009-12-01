/* SPAMM -- the Simple, Poor-man's Allocator/Memory-Manager
 * (with garbage collection)
 *
 * $RCSfile: spamm.h,v $
 * $Revision: 2.8 $
 * $Date: 1995/10/05 05:00:01 $
 * $Author: liblit $
 *
 */

#ifndef SPAMM_H
#define SPAMM_H

#include <excfns.h>
#include <dlist.h>

#define ULBITS (8*(sizeof(unsigned long)))

struct spamm_ObjectPage {
    unsigned long gcMarks, inUse;
    int size;			/* after rounding up to align */
    int elts;
    void (*trace) P((GENERIC_POINTER_TYPE *));
    void (*reclaim) P((GENERIC_POINTER_TYPE *));
    char *data;
};

struct spamm_ObjectPool {
    int size;			/* before rounding up to align */
    int pageelts;		/* between 1 and ULBITS */
    void (*trace) P((GENERIC_POINTER_TYPE *));
    void (*reclaim) P((GENERIC_POINTER_TYPE *));
    struct dlist emptyPages, fullPages, neitherPages;
};

extern struct dlist spamm_ObjectRoot;

extern void (*spamm_GcStart)(), (*spamm_GcEnd)();

extern void spamm_Initialize();
extern void spamm_InitPool P((struct spamm_ObjectPool *,
			      int,
			      int,
			      void (*) (GENERIC_POINTER_TYPE *),
			      void (*) (GENERIC_POINTER_TYPE *)));
extern int spamm_PoolStats P((struct spamm_ObjectPool *,
			      int *, int *, int *));

extern GENERIC_POINTER_TYPE *spamm_Allocate P((struct spamm_ObjectPool *));

extern int spamm_Root P((GENERIC_POINTER_TYPE **));
extern int spamm_RootList VP((GENERIC_POINTER_TYPE **, ...));

extern void spamm_Trace P((GENERIC_POINTER_TYPE *));
extern void spamm_CollectGarbage();
extern void spamm_GcSuspend();
extern void spamm_GcUnsuspend();


#define spamm_Unroot(n) (dlist_Remove(&spamm_ObjectRoot,(n)))


#define SPAMM_ROOT(list) \
    do { \
        int _Spamm_Root_Count_ = spamm_RootList list; \
	int _Spamm_Root_First_Index_ = dlist_Head(&spamm_ObjectRoot); \
        TRY

#define SPAMM_ENDROOT \
        FINALLY { \
            int i, j, index = _Spamm_Root_First_Index_; \
            for (i = 0; i < _Spamm_Root_Count_; ++i) { \
                j = index; \
                index = dlist_Next(&spamm_ObjectRoot, j); \
                spamm_Unroot(j); \
            } \
        } ENDTRY; \
    } while (0)


#define SPAMM_GCSUSPEND \
    do { \
        spamm_GcSuspend(); \
        TRY

#define SPAMM_ENDGCSUSPEND \
        FINALLY { \
            spamm_GcUnsuspend(); \
        } ENDTRY; \
    } while (0)

#endif /* SPAMM_H */
