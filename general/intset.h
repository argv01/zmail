/*
 * $RCSfile: intset.h,v $
 * $Revision: 2.10 $
 * $Date: 1995/02/17 02:45:53 $
 * $Author: bobg $
 */

#ifndef INTSET_H
#define INTSET_H

#include <dlist.h>

struct intset {
    int Min, Max, count;
    struct dlist parts;
};

struct intset_iterator {
    int partnum;
    int val;			/* only meaningful when partnum >= 0 */
};

#define intset_EmptyP(is) (dlist_EmptyP(&((is)->parts)))
#define intset_Count(is) ((is)->count)

extern int intset_Contains P((struct intset *, int));
extern int intset_Equal P((struct intset *, struct intset *));
extern int intset_Min P((struct intset *));
extern int intset_Max P((struct intset *));
extern void intset_Add P((struct intset *, int));
extern void intset_AddRange P((struct intset *, int, int));
extern void intset_Clear P((struct intset *));
extern void intset_Destroy P((struct intset *));
extern void intset_Init P((struct intset *));
extern void intset_Remove P((struct intset *, int));

extern void intset_InitIterator P((struct intset_iterator *));
extern int *intset_Iterate P((struct intset *, struct intset_iterator *));

#endif /* INTSET_H */
