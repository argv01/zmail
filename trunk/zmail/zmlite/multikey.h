/*
 * $RCSfile: multikey.h,v $
 * $Revision: 2.8 $
 * $Date: 1995/09/20 06:44:20 $
 * $Author: liblit $
 *
 * $Log: multikey.h,v $
 * Revision 2.8  1995/09/20 06:44:20  liblit
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
 * Revision 2.7  1994/02/24 19:20:20  bobg
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
 * Revision 2.6  1994/02/22  20:47:34  bobg
 * Use the new keysequence API.  Include dirserv.h in addrbook.c.  Make
 * only single clicks work in the variables list.  Make $newline work in
 * Commandfield objects.
 *
 * Revision 2.5  1993/12/07  23:43:39  bobg
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
 * Revision 2.4  1993/12/01  00:10:34  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Permit Z-Scriptable buttons and menus.
 */

#ifndef MULTIKEY_H
#define MULTIKEY_H

#include <spoor.h>
#include <dialog.h>

#include <dynstr.h>
#include <spoor/wrapview.h>
#include <spoor/textview.h>
#include <spoor/cmdline.h>
#include <spoor/buttonv.h>
#include <spoor/listv.h>
#include <spoor/list.h>

struct multikey {
    SUPERCLASS(dialog);
    struct spKeysequence ks;
    struct spWrapview *seqwrap;
    struct spTextview *instructions;
    struct spCmdline *keyname, *sequence;
    struct spListv *keynames;
};

extern struct spWclass *multikey_class;

extern struct spWidgetInfo *spwc_Multikey;

#define multikey_NEW() \
    ((struct multikey *) spoor_NewInstance(multikey_class))

extern void multikey_InitializeClass P((void));

#endif /* MULTIKEY_H */
