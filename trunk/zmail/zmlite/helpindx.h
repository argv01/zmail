/*
 * $RCSfile: helpindx.h,v $
 * $Revision: 2.8 $
 * $Date: 1995/09/20 06:44:13 $
 * $Author: liblit $
 *
 * $Log: helpindx.h,v $
 * Revision 2.8  1995/09/20 06:44:13  liblit
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
 * Revision 2.7  1994/05/02 08:51:14  bobg
 * Add next-page and previous-page interactions and keybindings to
 * Filebox.  Clean up the implementation of the Helpindex; it works a lot
 * better, but the core needs work before it's perfect.  Fix
 * missing-parameter bug in pattern-search dialog.  Don't dump core in
 * small-alias dialog when edit_hdrs is on.  Create folder directory when
 * opening various dialogs.  Don't dump core in a handful of textedit
 * subcommands.
 *
 * Revision 2.6  1994/02/24  19:20:05  bobg
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
 * Revision 2.5  1994/02/11  00:29:46  bobg
 * Make the Help Index work again.  Do it by not directly calling help()
 * any more, because help() can't handle a modal Help Index (i.e., one
 * for which ZmPagerStop doesn't exit until the user dismisses the
 * dialog).  This is so gross; it required deferring (via the event
 * queue) the popup up the Help Index dialog, and all kinds of terrible
 * things.  Also don't use the "revert" behavior in the Alias dialog's
 * fields; make "set autodismiss += sort" work; and fix saveopts -g so
 * that bindkeys that get written are correct w.r.t. bindkey syntax and
 * widget names.
 */

#ifndef HELPINDEX_H
#define HELPINDEX_H

#include <spoor.h>
#include <dialog.h>

#include <spoor/buttonv.h>
#include <spoor/textview.h>
#include <spoor/splitv.h>
#include <spoor/listv.h>

struct helpIndex {
    SUPERCLASS(dialog);
    struct spButtonv *category;
    struct spListv *list;
    struct spTextview *text;
    struct spSplitview *split;
};

extern struct spWclass *helpIndex_class;

extern struct spWidgetInfo *spwc_Helpindex;

extern int m_helpIndex_clear;
extern int m_helpIndex_append;
extern int m_helpIndex_setTopic;

#define helpIndex_NEW() \
    ((struct helpIndex *) spoor_NewInstance(helpIndex_class))

extern void helpIndex_InitializeClass P((void));

#endif /* HELPINDEX_H */
