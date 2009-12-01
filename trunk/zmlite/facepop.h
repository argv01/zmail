/*
 * $RCSfile: facepop.h,v $
 * $Revision: 2.6 $
 * $Date: 1995/09/20 06:44:08 $
 * $Author: liblit $
 *
 * $Log: facepop.h,v $
 * Revision 2.6  1995/09/20 06:44:08  liblit
 * Get rather carried away and prototype a large number of zero-argument
 * functions.  Unlike C++, ANSI C has two extremely different meanings
 * for "()" and "(void)" in function declarations.
 *
 * Also prototype some parameter-taking functions, but not too many,
 * because there are only so many compiler warnings I can take.  :-)
 *
 * In printdialog_activate(), found in printd.c, change the order of some
 * operations so that a "namep" field gets initialized before it is first
 * referenced.  The UMR that this fixes corresponds to PR #6441.
 *
 * Revision 2.5  1994/02/24 19:19:55  bobg
 * Switch over to the new way of doing widget classes.  Previously,
 * hashtable lookups due to widget-class searches accounted for more than
 * 33% of Lite's running time.  The new widget class scheme eliminates
 * nearly all those lookups, replacing them with simple pointer
 * traversals and structure dereferences.  Profiling this version of Lite
 * reveals that we're back to the state where the main thing slowing down
 * Lite is the core itself.  Yay!
 *
 * Can you believe all this worked perfectly on the very first try?
 *
 * Revision 2.4  1993/12/07  23:43:25  bobg
 * Restore several widget names and widgetclass names that got clobbered
 * in the recent unpleasantness.  Fix several minor bugs in chooseone
 * dialogs.  Correctly initialize all Z-Script button areas and menu
 * areas.  Add remaining code required for correct Z-Script menu
 * handling.  Correctly keep track of the current message group.  Fix the
 * X-Face dialog (hooray!).  Fix finalizing bugs in the filelist object.
 * Eliminate various chunks of dead code.  Hardwire the desired size of
 * the multikey dialog (despite all the new smarts in
 * spWrapview_desiredSize and spSplitview_desiredSize).  Don't
 * redundantly define some widget classes.  Restore do-sequence and
 * widget-info interactions, and Zmliteapp widget class, all of which got
 * inadvertently clobbered in the aforementioned recent unpleasantness.
 *
 * Revision 2.3  1993/12/01  00:10:10  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Permit Z-Scriptable buttons and menus.
 *
 * Revision 2.2  1993/05/29  00:54:35  bobg
 * Change spoorClass_t to struct spClass, the SPOOR metaclass.  Use short
 * names in "To" field in chevron screen.  Don't show the end of a new
 * message in the Chevron screen; show its beginning.  Remove spurious
 * functions from chevron.c.  Add chevron-delete interaction, which will
 * back up to the previous non-deleted, non-saved message if there is no
 * such one in the forward direction.
 */

#ifndef FACEPOPUP_H
#define FACEPOPUP_H

#include <spoor.h>
#include <dialog.h>

struct facepopup {
    SUPERCLASS(dialog);
    struct spTextview *face;
};

extern struct spWclass *facepopup_class;

extern struct spWidgetInfo *spwc_Xface;

#define facepopup_NEW() \
    ((struct facepopup *) spoor_NewInstance(facepopup_class))

extern void facepopup_InitializeClass P((void));

extern struct facepopup *facepopup_Create P((char *));

#endif /* FACEPOPUP_H */
