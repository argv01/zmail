/*
 * $RCSfile: mainf.h,v $
 * $Revision: 2.14 $
 * $Date: 1995/09/20 06:44:15 $
 * $Author: liblit $
 *
 * $Log: mainf.h,v $
 * Revision 2.14  1995/09/20 06:44:15  liblit
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
 * Revision 2.13  1995/07/08 01:23:15  spencer
 * Replacing references to folder.h with zfolder.h.  Whee.
 *
 * Revision 2.12  1994/02/24  19:20:12  bobg
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
 * Revision 2.11  1994/02/18  19:58:45  bobg
 * The changes from the avoid-timer-api branch, plus:  don't allow
 * wprint("\n") to blank the status line.
 *
 * Revision 2.10  1994/02/02  22:45:28  bobg
 * Shorten CVS log.  Make selected messages be preserved when switching
 * folders.  Don't revert contents in the multikey dialog's Sequence
 * field.
 *
 * Revision 2.9  1993/12/31  06:56:46  bobg
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
 * Revision 2.8  1993/12/14  21:29:07  bobg
 * Use new strcase stuff.  Initialize zmlcomposeframes better; their
 * compositions are now assigned immediately.  Add more Z-Script variable
 * callbacks.  Fix bug in list of active dialogs.  Fix bug in
 * ZmButtonList container lists.  Add "goto-main" interaction, which
 * somehow got lost recently.  Fix bug in creating focus list for main
 * screen.  Correct changed_folder_refresh() to do a full_refresh() when
 * the changed-to folder is new.  Correctly permit turning off main
 * screen's action area.  Do the GSTATE_ACTIVE_COMP case of
 * gui_get_state().  Don't do gui_cleanup if istool != 2 (grr).
 * Understand goofy dialog names like AddFolder (should be a Z-Script
 * function).
 *
 * Revision 2.7  1993/12/01  00:10:27  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Permit Z-Scriptable buttons and menus.
 *
 * Revision 2.6  1993/10/28  06:04:18  bobg
 * All the changes from the att-custom-06-26 branch, and then some.
 */

#ifndef ZMLMAINFRAME_H
#define ZMLMAINFRAME_H

#include <spoor.h>
#include <dialog.h>
#include <spoor/toggle.h>
#include <spoor/cmdline.h>
#include <spoor/textview.h>
#include <spoor/listv.h>
#include <zfolder.h>

struct zmlmainframe {
    SUPERCLASS(dialog);
    struct {
	struct spListv *status;
	struct spListv *messages;
	struct spTextview *output;
	struct spCmdline *command;
    } panes;
    struct spButtonv *aa;	/* save it when it's not visible */
    int cachedNewlines, paneschanged;
    msg_folder *fldr;
};

extern struct spWclass *zmlmainframe_class;

extern struct spWidgetInfo *spwc_Main;

#define zmlmainframe_NEW() \
    ((struct zmlmainframe *) spoor_NewInstance(zmlmainframe_class))

extern int m_zmlmainframe_clearHdrs;
extern int m_zmlmainframe_redrawHdrs;
extern int m_zmlmainframe_setMainPanes;
extern int m_zmlmainframe_newHdrs;

extern void zmlmainframe_InitializeClass P((void));

enum zmlmainframe_observation {
    zmlmainframe_mainselection = dialog_OBSERVATIONS,
    zmlmainframe_OBSERVATIONS
};

#endif /* ZMLMAINFRAME_H */
