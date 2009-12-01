/* 
 * $RCSfile: notifier.h,v $
 * $Revision: 2.6 $
 * $Date: 1995/09/20 06:44:22 $
 * $Author: liblit $
 */

#ifndef NOTIFIER_H
# define NOTIFIER_H

#include <spoor.h>
#include <dialog.h>

struct notifier {
    SUPERCLASS(dialog);
    struct spTextview *textview;
};

/* Add field accessors */
#define notifier_textview(x) \
    (((struct notifier *) (x))->textview)

/* Declare method selectors */

extern struct spWclass *notifier_class;

extern struct spWidgetInfo *spwc_Notifier;

extern void notifier_InitializeClass P((void));

#define notifier_NEW() \
    ((struct notifier *) spoor_NewInstance(notifier_class))

enum notifier_DeactivateReason {
    notifier_Done = dialog_DEACTIVATEREASONS,
    notifier_Ok,
    notifier_Yes,
    notifier_Bye,
    notifier_No,
    notifier_Cancel,
#ifdef PARTIAL_SEND
    notifier_SendSplit,
    notifier_SendWhole,
#endif /* PARTIAL_SEND */
    notifier_DEACTIVATEREASONS
};

extern struct notifier *notifier_Create P((const char *,
					   const char *,
					   unsigned long,
					   enum notifier_DeactivateReason));

#endif /* NOTIFIER_H */
