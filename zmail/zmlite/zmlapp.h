/* 
 * $RCSfile: zmlapp.h,v $
 * $Revision: 2.3 $
 * $Date: 1995/09/20 06:44:39 $
 * $Author: liblit $
 *
 * $Log: zmlapp.h,v $
 * Revision 2.3  1995/09/20 06:44:39  liblit
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
 * Revision 2.2  1994/02/24 19:20:56  bobg
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
 * Revision 2.1  1994/01/14  05:33:32  bobg
 * Use the restored Popup widget-class.  Show the "simplest" keysequence
 * for jumping to the menubar.  Don't deselect all messages after
 * executing a Z-Script command with no output.  Create the "Notifier"
 * widget class.  Create the zmlapp class, which inherits from
 * spCursesIm; ZmlIm is now a zmlapp instance (so it can override the
 * refocus method, which it does to update the menubar help label).
 * Refocus after binding/unbinding keys.  Make "multikey -l" verbose
 * about what it's accomplishing.
 *
 */

#ifndef ZMLAPP_H
# define ZMLAPP_H

#include <spoor.h>
#include <spoor/cursim.h>

struct zmlapp {
    SUPERCLASS(spCursesIm);
};

extern struct spWclass *zmlapp_class;

extern struct spWidgetInfo *spwc_Zmliteapp;

extern void zmlapp_InitializeClass P((void));

#define zmlapp_NEW() \
    ((struct zmlapp *) spoor_NewInstance(zmlapp_class))

#endif /* ZMLAPP_H */
