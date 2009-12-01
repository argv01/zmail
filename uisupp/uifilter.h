#ifndef _UIFILTER_H_
#define _UIFILTER_H_

/*
 * $RCSfile: uifilter.h,v $
 * $Revision: 1.4 $
 * $Date: 1994/04/01 04:03:45 $
 * $Author: pf $
 */

#include "uiact.h"
#include "uipick.h"

#define uifilter_NewMail     ULBIT(0)

struct uifilter {
    zmFlags flags;
    char *name;
    uiact_t action;
    uipick_t pick;
};
typedef struct uifilter uifilter_t;

#define uifilter_SetName(F, N) (str_replace(&(F)->name, (N)))
#define uifilter_GetName(F) ((F)->name)
#define uifilter_SetFlags(F, FL) (turnon((F)->flags, (FL)))
#define uifilter_GetFlags(F, FL) ((F)->flags & (FL))
#define uifilter_GetPick(F)  (&(F)->pick)
#define uifilter_GetAction(F)  (&(F)->action)

extern uifilter_t *uifilter_Create P ((void));
extern void uifilter_Delete P ((uifilter_t *));
extern uifilter_t *uifilter_Get P ((int));
extern zmBool uifilter_Install P ((uifilter_t *));
extern char **uifilter_List P ((void));
extern zmBool uifilter_SupplyName P ((uifilter_t *));
extern zmBool uifilter_Remove P ((int));

#endif /* _UIFILTER_H_ */
