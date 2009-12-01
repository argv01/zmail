/*
 * $RCSfile: popupv.h,v $
 * $Revision: 2.8 $
 * $Date: 1995/08/11 21:10:23 $
 * $Author: bobg $
 */

#ifndef SPOOR_POPUPVIEW_H
#define SPOOR_POPUPVIEW_H

#include <spoor.h>
#include "view.h"
#include "event.h"

enum spPopupView_style {
    spPopupView_plain,
    spPopupView_boxed,
    spPopupView_STYLES
};

struct spPopupView {
    SUPERCLASS(spView);
    struct spView *view;
    enum spPopupView_style style;
    int extra, minh, minw;
};

#define spPopupView_view(p) (((struct spPopupView *) (p))->view)
#define spPopupView_extra(p) (((struct spPopupView *) (p))->extra)

extern struct spClass *spPopupView_class;

#define spPopupView_NEW() \
    ((struct spPopupView *) spoor_NewInstance(spPopupView_class))

extern int m_spPopupView_setView;

extern void spPopupView_InitializeClass();

extern struct spPopupView *spPopupView_Create P((struct spView *,
						 enum spPopupView_style));
#endif /* SPOOR_POPUPVIEW_H */
