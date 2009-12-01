/*
 * $RCSfile: wrapview.h,v $
 * $Revision: 2.4 $
 * $Date: 1994/03/23 01:44:49 $
 * $Author: bobg $
 *
 * $Log: wrapview.h,v $
 * Revision 2.4  1994/03/23 01:44:49  bobg
 * Banish the spUnpack() function to portability hell.  Don't
 * RAISE(strerror(EINVAL), ...) just because the caller tried to install
 * a menu item in an impossible slot.  Trim CVS logs.
 *
 * Revision 2.3  1993/12/16  01:45:06  bobg
 * Change several P(())'s to NP(())'s.  Add several typecasts to satisfy
 * yet another flavor of prototype-sensitivity.
 *
 * Revision 2.2  1993/12/01  00:08:50  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Changed some stuff.
 */

#ifndef SPOOR_WRAPVIEW_H
#define SPOOR_WRAPVIEW_H

#include <spoor.h>
#include "view.h"

struct spWrapview {
    SUPERCLASS(spView);
    int highlightp, besth, bestw, boxed, bigChild;
    char *label[4];
    struct spView *view;
    unsigned long align;
};

#define spWrapview_bigChild(w)   (((struct spWrapview *) (w))->bigChild)
#define spWrapview_boxed(w)      (((struct spWrapview *) (w))->boxed)
#define spWrapview_besth(w)      (((struct spWrapview *) (w))->besth)
#define spWrapview_bestw(w)      (((struct spWrapview *) (w))->bestw)
#define spWrapview_highlightp(w) (((struct spWrapview *) (w))->highlightp)
#define spWrapview_view(w)	 (((struct spWrapview *) (w))->view)
#define spWrapview_label(w,n)	 (((struct spWrapview *) (w))->label[(n)])
#define spWrapview_align(w)	 (((struct spWrapview *) (w))->align)

enum spWrapview_updateFlag {
    spWrapview_labelOnly = spView_UPDATEFLAGS,
    spWrapview_UPDATEFLAGS
};

enum spWrapview_side {
    spWrapview_top,
    spWrapview_left,
    spWrapview_right,
    spWrapview_bottom,
    spWrapview_SIDES
};

#define spWrapview_TBCENTER (1<<0)
#define spWrapview_LRCENTER (1<<1)
#define spWrapview_LFLUSH   (1<<2)
#define spWrapview_RFLUSH   (1<<3)
#define spWrapview_TFLUSH   (1<<4)
#define spWrapview_BFLUSH   (1<<5)

extern struct spClass *spWrapview_class;

#define spWrapview_NEW() \
    ((struct spWrapview *) spoor_NewInstance(spWrapview_class))

/* Method selectors */
extern int m_spWrapview_setView;
extern int m_spWrapview_setLabel;

extern void spWrapview_InitializeClass();

extern struct spWrapview *spWrapview_Create P((GENERIC_POINTER_TYPE *,
					       char *, char *, char *, char *,
					       int, int, unsigned long));

enum spWrapview_observation {
    spWrapview_OBSERVATIONS = spView_OBSERVATIONS
};

#endif /* SPOOR_WRAPVIEW_H */
