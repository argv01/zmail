/*
 * $RCSfile: splitv.h,v $
 * $Revision: 2.5 $
 * $Date: 1994/03/23 01:44:24 $
 * $Author: bobg $
 *
 * $Log: splitv.h,v $
 * Revision 2.5  1994/03/23 01:44:24  bobg
 * Banish the spUnpack() function to portability hell.  Don't
 * RAISE(strerror(EINVAL), ...) just because the caller tried to install
 * a menu item in an impossible slot.  Trim CVS logs.
 *
 * Revision 2.4  1993/12/16  01:44:54  bobg
 * Change several P(())'s to NP(())'s.  Add several typecasts to satisfy
 * yet another flavor of prototype-sensitivity.
 *
 * Revision 2.3  1993/12/01  00:08:35  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Changed some stuff.
 */

#ifndef SPOOR_SPLITVIEW_H
#define SPOOR_SPLITVIEW_H

#include <spoor.h>
#include "view.h"

enum spSplitview_splitType {
    spSplitview_leftRight,
    spSplitview_topBottom,
    spSplitview_SPLITTYPES
};

enum spSplitview_style {
    spSplitview_plain,
    spSplitview_boxed,
    spSplitview_STYLES
};

struct spSplitview {
    SUPERCLASS(spView);
    struct spView          *child[2];
    enum spSplitview_splitType splitType;
    enum spSplitview_style  style;
    int                     childSize, whichChildSize, percentP;
    int                     borders;
};

#define spSplitview_child(s,i) (((struct spSplitview *) (s))->child[(i)])
#define spSplitview_boxstyle(s) \
    (((struct spSplitview *) (s))->style)
#define spSplitview_borders(s) \
    (((struct spSplitview *) (s))->borders)

#define spSplitview_LEFT	(1<<0)
#define spSplitview_TOP		(1<<1)
#define spSplitview_RIGHT	(1<<2)
#define spSplitview_BOTTOM	(1<<3)
#define spSplitview_SEPARATE	(1<<4)

#define spSplitview_ALLBORDERS	(spSplitview_LEFT	| \
				 spSplitview_TOP	| \
				 spSplitview_RIGHT	| \
				 spSplitview_BOTTOM	| \
				 spSplitview_SEPARATE)

extern struct spClass    *spSplitview_class;

#define spSplitview_NEW() \
    ((struct spSplitview *) spoor_NewInstance(spSplitview_class))

extern int              m_spSplitview_setChild;
extern int              m_spSplitview_setup;

extern void             spSplitview_InitializeClass();

extern struct spSplitview *spSplitview_Create P((GENERIC_POINTER_TYPE *,
						 GENERIC_POINTER_TYPE *,
						 int, int, int,
						 enum spSplitview_splitType,
						 enum spSplitview_style,
						 int));
#endif /* SPOOR_SPLITVIEW_H */
