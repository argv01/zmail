/*
 * $RCSfile: printd.h,v $
 * $Revision: 2.8 $
 * $Date: 1995/09/20 06:44:27 $
 * $Author: liblit $
 *
 * $Log: printd.h,v $
 * Revision 2.8  1995/09/20 06:44:27  liblit
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
 * Revision 2.7  1995/07/08 01:23:17  spencer
 * Replacing references to folder.h with zfolder.h.  Whee.
 *
 * Revision 2.6  1994/02/24  19:20:30  bobg
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
 * Revision 2.5  1993/12/31  06:56:55  bobg
 * Get rid of spurious pullright-arrows on attach dialog pullrights.
 * Correctly track each dialog's current folder and message group.
 * Delete a lot more dead code from composef.c.  Get rid of the
 * "dialog_composepanes" and "dialog_messagepanes" observations, and add
 * a "zmlmainframe_mainselection" observation.  Add a missing argument to
 * a call in composef's updateZbutton method, fixing the core dump upon
 * starting any composition.  Revert the contents of a Messages: item if
 * a user edits it, then tabs out (without pressing ENTER).  Fill in the
 * Folder: field of dialogs even when they're created *after* the
 * dialog's folder is set.  Fix a divide-by-zero in using the task meter
 * during summary-redrawing.  Correct the logic that permits a message
 * screen to remain active even when the main screen switches folders.
 * Don't wrap lines in a "showkeys" pager.  Add the AttachDialog to the
 * set of dialogs that can be found by Dialog().  Save "bindkey" and
 * "unbindkey" commands with "saveopts -g"!
 *
 * Revision 2.4  1993/12/01  00:10:42  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Permit Z-Scriptable buttons and menus.
 */

#ifndef PRINTDIALOG_H
#define PRINTDIALOG_H

#include <spoor.h>
#include <dialog.h>

#include <spoor/textview.h>
#include <spoor/cmdline.h>
#include <spoor/buttonv.h>
#include <spoor/listv.h>
#include <spoor/wrapview.h>

#include <zfolder.h>

struct printdialog {
    SUPERCLASS(dialog);
    int namep;
    struct spButtonv *options;
    struct spListv *printers;
    struct spCmdline *command;
    struct spWrapview *cmdwrap;
    msg_group mg;
};

extern struct spWclass *printdialog_class;

extern struct spWidgetInfo *spwc_Print;

#define printdialog_NEW() \
    ((struct printdialog *) spoor_NewInstance(printdialog_class))

extern void printdialog_InitializeClass P((void));

#endif /* PRINTDIALOG_H */
