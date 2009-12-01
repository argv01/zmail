#ifndef _UICHOOSE_H_
#define _UICHOOSE_H_

/*
 * $RCSfile: uichoose.h,v $
 * $Revision: 1.5 $
 * $Date: 1994/06/06 21:24:17 $
 * $Author: pf $
 */

#include <general.h>
#include <uisupp.h>
#include <dynstr.h>

struct uichoose {
    const char *query, *dflt;
    struct dynstr result;
};
typedef struct uichoose uichoose_t;

#define uichoose_SetResult(X, Y) (dynstr_Set(&(X)->result, (Y)))
#define uichoose_GetResult(X) (dynstr_Str(&(X)->result))
#define uichoose_SetQuery(X, Y) ((X)->query = (Y))
#define uichoose_GetQuery(X) ((X)->query)
#define uichoose_SetDefault(X, Y) ((X)->dflt = (Y))
#define uichoose_GetDefault(X) ((X)->dflt)

void uichoose_Init P((uichoose_t *));
void uichoose_Destroy P((uichoose_t *));
zmBool uichoose_Ask P((uichoose_t *));

#endif /* _UICHOOSE_H_ */
