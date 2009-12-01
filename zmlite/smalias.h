/*
 * $RCSfile: smalias.h,v $
 * $Revision: 2.6 $
 * $Date: 1995/09/20 06:44:30 $
 * $Author: liblit $
 *
 * $Log: smalias.h,v $
 * Revision 2.6  1995/09/20 06:44:30  liblit
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
 * Revision 2.5  1994/02/24 19:20:36  bobg
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
 * Revision 2.4  1993/12/01  00:10:47  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Permit Z-Scriptable buttons and menus.
 */

#ifndef SMALLALIASES_H
#define SMALLALIASES_H

#include <spoor.h>
#include <dialog.h>
#include <spoor/listv.h>

struct smallaliases {
    SUPERCLASS(dialog);
    struct spListv *list;
};

extern struct spWclass *smallaliases_class;

extern struct spWidgetInfo *spwc_Choosealias;

#define smallaliases_NEW() \
    ((struct smallaliases *) spoor_NewInstance(smallaliases_class))

extern void smallaliases_InitializeClass P((void));

#endif /* SMALLALIASES_H */
