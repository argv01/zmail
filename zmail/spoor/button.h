/*
 * $RCSfile: button.h,v $
 * $Revision: 2.6 $
 * $Date: 1995/07/28 18:59:06 $
 * $Author: bobg $
 */

#ifndef SPOOR_BUTTON_H
#define SPOOR_BUTTON_H

#include <spoor.h>
#include "obsrvbl.h"

struct spButton {
    SUPERCLASS(spObservable);
    char *label;
    void (*callback) NP((struct spButton *, GENERIC_POINTER_TYPE *));
    GENERIC_POINTER_TYPE *callbackData;
};

#define spButton_label(b) (((struct spButton *) (b))->label)
#define spButton_callbackData(x) \
    (((struct spButton *) (x))->callbackData)
#define spButton_callback(x) \
    (((struct spButton *) (x))->callback)

extern struct spClass    *spButton_class;

#define spButton_NEW() \
    ((struct spButton *) spoor_NewInstance(spButton_class))

/* Method selectors */
extern int m_spButton_push;
extern int m_spButton_setLabel;
extern int m_spButton_setCallback;

extern void             spButton_InitializeClass();

enum spButton_observation {
    spButton_pushed = spObservable_OBSERVATIONS,
    spButton_OBSERVATIONS
};

extern struct spButton *
    spButton_Create P((const char *,
		       void (*) NP((struct spButton *,
				    GENERIC_POINTER_TYPE *)),
		       GENERIC_POINTER_TYPE *));

#endif /* SPOOR_BUTTON_H */
