/*
 * $RCSfile: buttonv.h,v $
 * $Revision: 2.11 $
 * $Date: 1994/02/24 19:15:46 $
 * $Author: bobg $
 *
 * $Log: buttonv.h,v $
 * Revision 2.11  1994/02/24 19:15:46  bobg
 * Change everything.
 *
 * But seriously, change the way "widget classes" are handled.  Rather
 * than requiring a bazillion hashtable lookups (because of everything
 * being string-keyword based), use actual data structures to associate
 * e.g. keymaps and interaction lists with widget classes.  A spoor
 * class's "widget class" is now stored in the spoor class descriptor
 * itself, rather than requiring yet two more hashtable lookups.  Of
 * course, spoor class descriptors don't have room for a "widget class"
 * pointer field, but fortunately, class descriptors are instances of the
 * class class!  So, subclass the class class to get a subclass of
 * "class" called spWclass (re-read that sentence until your eyes don't
 * cross).  Any class that wants to point to a widget class now creates
 * its class descriptor as an instance of spWclass, not spClass.
 *
 * Can you believe all this worked perfectly on the very first try?
 *
 * Revision 2.10  1993/12/27  23:18:31  bobg
 * Add the clickButton method tp buttonv.  Correct egregious error in
 * returning from the main interaction loop; among other things, it
 * prevented "ask" dialogs from returning successfully!!  Correct
 * list-widget bug that caused the cursor to end up on the right margin
 * instead of the left.  Get rid of needless hack that used to try to get
 * around that bug.  Create spListv_Create.  Replace a large chunk of
 * menu.c that was doing an inefficient version of glist_Insert() with a
 * call to glist_Insert().
 *
 * Revision 2.9  1993/12/22  04:42:12  bobg
 * Use a SPOOR_PROTECT when invoking a button's callback.  I wish I had
 * garbage collection.  Switch from using P((VA_PROTO())) to just
 * VA_PROTO().  Handle returning from the interaction loop correctly.
 * Finalize the interaction manager more correctly.  Make views know
 * whether they have the input focus.  Zero a view's parent pointer
 * immediately when unembedding to prevent reentrance problems; keep
 * track of the old parent during the unembed operation.
 *
 * Revision 2.8  1993/12/07  23:35:17  bobg
 * Add "replace" method to buttonv.  Keep menu highlighting correct by
 * not losing the selection when a submenu pops up.  Automatically append
 * " ->" to menu items with submenus.  Don't needlessly propagate embed
 * into popupview's child.  Don't redundantly initialize twelve variables
 * in spSplitview_desiredSize.  Make spSplitview_desiredSize a little
 * smarter by sometimes calculating max width/height.  Fill in a
 * long-standing blank in spWrapview_desiredSize:  min width is now
 * correctly computed.  Correctly determine max width/height in
 * spWrapview_desiredSize.
 *
 * Revision 2.7  1993/12/01  00:08:02  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Changed some stuff.
 *
 * Revision 2.6  1993/10/28  05:57:45  bobg
 * All the changes from the att-custom-06-26 branch, and then some.
 *
 * Revision 2.5.2.2  1993/09/02  04:24:15  bobg
 * Allow callers to turn the scroll-percent indicator off for all
 * objects, by adding a hook called spTextview_enableScrollpct.
 */

#ifndef SPOOR_BUTTONV_H
#define SPOOR_BUTTONV_H

#include <spoor.h>
#include "view.h"

enum spButtonv_bstyle {
    spButtonv_horizontal,
    spButtonv_vertical,
    spButtonv_multirow,
    spButtonv_grid,
    spButtonv_bstyleS
};

enum spButtonv_toggleStyle {
    spButtonv_brackets,
    spButtonv_inverse,
    spButtonv_checkbox,
    spButtonv_TOGGLESTYLES
};

struct spButtonv {
    SUPERCLASS(spView);
    struct glist buttons;
    enum spButtonv_bstyle style;
    enum spButtonv_toggleStyle toggleStyle;
    void (*callback) P((struct spButtonv *, int));
    int selection, hoffset, voffset, widest, anticipatedWidth;
    unsigned int haveFocus:1, scrunch:1;
    unsigned int highlightWithoutFocus:1, suppressBrackets:1;
    unsigned int blink:1, blinking:1, wrap:1;
};

#define spButtonv_wrap(s) (((struct spButtonv *) (s))->wrap)
#define spButtonv_blink(s) (((struct spButtonv *) (s))->blink)
#define spButtonv_suppressBrackets(s) \
    (((struct spButtonv *) (s))->suppressBrackets)
#define spButtonv_anticipatedWidth(s) \
    (((struct spButtonv *) (s))->anticipatedWidth)
#define spButtonv_button(s,i) \
    (*((struct spButton **) glist_Nth(&spButtonv_buttons(s),(i))))
#define spButtonv_buttons(s)     (((struct spButtonv *) (s))->buttons)
#define spButtonv_callback(s)    (((struct spButtonv *) (s))->callback)
#define spButtonv_length(s)      (glist_Length(&spButtonv_buttons(s)))
#define spButtonv_selection(s)   (((struct spButtonv *) (s))->selection)
#define spButtonv_style(s)       (((struct spButtonv *) (s))->style)
#define spButtonv_toggleStyle(s) (((struct spButtonv *) (s))->toggleStyle)
#define spButtonv_scrunch(s)     (((struct spButtonv *) (s))->scrunch)
#define spButtonv_highlightWithoutFocus(s) \
    (((struct spButtonv *) (s))->highlightWithoutFocus)

extern struct spWclass *spButtonv_class;
extern struct spWidgetInfo *spwc_Buttonpanel;

#define spButtonv_NEW() \
    ((struct spButtonv *) spoor_NewInstance(spButtonv_class))

/* Method selectors */
extern int m_spButtonv_insert;
extern int m_spButtonv_remove;
extern int m_spButtonv_replace;
extern int m_spButtonv_clickButton;

extern void spButtonv_InitializeClass();

extern void spButtonv_radioButtonHack();
extern int spButtonv_radioSelection();

extern struct spButtonv *spButtonv_Create(VA_PROTO(enum spButtonv_bstyle));

#endif /* SPOOR_BUTTONV_H */
