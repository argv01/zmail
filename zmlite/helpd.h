/*
 * $RCSfile: helpd.h,v $
 * $Revision: 2.5 $
 * $Date: 1995/09/20 06:44:12 $
 * $Author: liblit $
 *
 * $Log: helpd.h,v $
 * Revision 2.5  1995/09/20 06:44:12  liblit
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
 * Revision 2.4  1994/02/24 19:20:01  bobg
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
 * Revision 2.3  1993/12/01  00:10:19  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Permit Z-Scriptable buttons and menus.
 */

#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <spoor.h>
#include <pagerd.h>

struct helpDialog {
    SUPERCLASS(pagerDialog);
};

extern struct spWclass *helpDialog_class;

extern struct spWidgetInfo *spwc_Help;

#define helpDialog_NEW() \
    ((struct helpDialog *) spoor_NewInstance(helpDialog_class))

extern void helpDialog_InitializeClass P((void));

#endif /* HELPDIALOG_H */
