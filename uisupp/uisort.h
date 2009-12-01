#ifndef _UISORT_H_
#define _UISORT_H_

/*
 * $RCSfile: uisort.h,v $
 * $Revision: 1.3 $
 * $Date: 1995/10/05 05:28:02 $
 * $Author: liblit $
 */

#include "uisupp.h"
#include "zmflag.h"

typedef enum uisort_index_enum {
    uisort_IndexDate = 0,
    uisort_IndexSubject,
    uisort_IndexAuthor,
    uisort_IndexLength,
    uisort_IndexPriority,
    uisort_IndexStatus,
    uisort_IndexMax
} uisort_index_t ;

struct _uisort {
    uisort_index_t indices[uisort_IndexMax+1];
    zmFlags flags, inuse, reverse;
    int count;
};
typedef struct _uisort uisort_t;

#define uisort_FlagIgnoreCase   ULBIT(0)
#define uisort_FlagUseRe        ULBIT(1)
#define uisort_FlagDateReceived ULBIT(2)

#define uisort_IndBit(X) (1 << ((int)(X)))

#define uisort_HasIndex(X, Y) (ison((X)->inuse, uisort_IndBit((Y))))

extern void uisort_AddIndex P ((uisort_t *, uisort_index_t));
extern void uisort_RemoveIndex P ((uisort_t *, uisort_index_t));
extern void uisort_ReverseIndex P ((uisort_t *, uisort_index_t, zmBool));
extern char **uisort_DescribeSort P ((uisort_t *));
extern zmBool uisort_DoSort P ((uisort_t *));
extern void uisort_UseOptions P ((uisort_t *, zmFlags));
extern void uisort_Init P ((uisort_t *));
extern void uisort_Destroy P ((uisort_t *));

#endif /* _UISORT_H_ */
