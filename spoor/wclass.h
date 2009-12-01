/* 
 * $RCSfile: wclass.h,v $
 * $Revision: 2.1 $
 * $Date: 1994/02/24 19:23:59 $
 * $Author: bobg $
 *
 * $Log: wclass.h,v $
 * Revision 2.1  1994/02/24 19:23:59  bobg
 * Oops, don't forget to add these new files, which make all the new
 * stuff possible.
 *
 */

#ifndef SPWCLASS_H
# define SPWCLASS_H

#include <spoor.h>

struct spWclass {
    SUPERCLASS(spClass);
    struct spWidgetInfo *wclass;
};

/* Add field accessors */
#define spWclass_wclass(x) \
    (((struct spWclass *) (x))->wclass)

/* Declare method selectors */

extern struct spClass *spWclass_class;
extern void spWclass_InitializeClass();

extern struct spWclass *
    spWclass_Create P((char *, char *,
		       struct spClass *, int,
		       void (*) NP((GENERIC_POINTER_TYPE *)),
		       void (*) NP((GENERIC_POINTER_TYPE *)),
		       struct spWidgetInfo *));

#define spWclass_NEW() \
    ((struct spWclass *) spoor_NewInstance(spWclass_class))

#endif /* SPWCLASS_H */
