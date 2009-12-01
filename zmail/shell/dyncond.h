#ifndef DYNCOND_H
# define DYNCOND_H

#include "callback.h"

typedef struct DynConditionRec {
    char *exp;
    ZmCallback callback_chain;
} DynConditionRec;
# define DynCondExpression(X) ((X)->exp)

typedef DynConditionRec *DynCondition;

extern DynCondition CreateDynCondition P((const char *, void_proc, VPTR));
extern void DestroyDynCondition P((DynCondition));
extern void SetDynConditionValue P((DynCondition, int));
extern int eval_bexp P((const char *, int *));

# define GetDynConditionValue(X) (eval_bexp((X)->exp, (int *) 0))

#endif /* DYNCOND_H */
