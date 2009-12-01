/* 
 * $RCSfile: dialog.h,v $
 * $Revision: 2.19 $
 * $Date: 1995/09/20 06:44:02 $
 * $Author: liblit $
 *
 * $Log: dialog.h,v $
 * Revision 2.19  1995/09/20 06:44:02  liblit
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
 * Revision 2.18  1995/07/08 01:23:12  spencer
 * Replacing references to folder.h with zfolder.h.  Whee.
 *
 * Revision 2.17  1994/05/11  02:04:44  bobg
 * Don't allow switching screens when modal dialogs are up.  Implement
 * gui_clean_compose.  Don't wedge when changing main_panes outside of
 * the Main context.  Get rid of Help button on pager dialog (per
 * Carlyn).  Add ability to set pager title.  Change ^X-w to report info
 * on enclosing Dialog as well as current widget.  Implement
 * DIALOG_NEEDED flag for gui_watch_filed.  Recognize LITETERM as
 * superseding TERM.  Add -q (quiet) flag to "multikey -l".  Actually
 * *use* the new attachment Comment field when sending attachments.
 *
 * Revision 2.16  1994/04/17  06:29:24  bobg
 * Rewrite the attachments dialog to be more correct.  It now uses the
 * Z-Script attachments API wherever possible, and deals more correctly
 * with its two list widgets.
 *
 * Revision 2.15  1994/03/03  07:36:00  bobg
 * Add new COT custom stuff.  Add a new widget class, Screen, which is a
 * subclass of Dialog; it distinguishes full-screen dialogs from popups.
 * Change MenuDialog to MenuScreen.  Some of Dialog's interactions have
 * now moved into Screen.  Shorten CVS logs.  Add the following new
 * subclasses of Screen:  Aliases; Envelope; Headers; Variables.  Don't
 * ever allow focus to land in the main screen's Folder: field.  Don't
 * allow pinups!
 *
 * Revision 2.14  1994/02/24  19:19:47  bobg
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
 * Revision 2.13  1994/02/18  19:58:40  bobg
 * The changes from the avoid-timer-api branch, plus:  don't allow
 * wprint("\n") to blank the status line.
 *
 * Revision 2.12  1994/02/02  03:50:40  bobg
 * Speed up dialog creation and entry/exit using new
 * dialog_MUNGE/dialog_ENDMUNGE pair of macros.  In a dialog_MUNGE
 * region, changes to a dialog's widget hierarchy are deferred.  At the
 * end of the region, they're all performed at once, for a significant
 * speedup.  Identified a number of cmdline's that should not revert
 * their contents by default.  Don't be redundant in a number of places
 * during dialog creation, for even more speedup.  Make "compcmd
 * insert-header" work.  Fix several problems relating to compositions,
 * toggling edit_hdrs, and getting and setting headers to and from the
 * header prompts.  Take out some dead code.  Add a couple of missing
 * arguments (I love C... not).  Don't explicitly reset the main screen's
 * Messages: field when focus leaves it any more; use cmdline's new
 * behavior instead.  (This fixes the bug causing summaries to be
 * needlessly redrawn when focus leaves that field.)
 *
 * Revision 2.11  1994/01/17  19:28:14  bobg
 * Memorize the menu help keysequence globally, not per-dialog.  Don't
 * attempt to compute the menu help while in a menu, nor while a
 * taskmeter is going.
 *
 * Revision 2.10  1994/01/14  05:33:05  bobg
 * Use the restored Popup widget-class.  Show the "simplest" keysequence
 * for jumping to the menubar.  Don't deselect all messages after
 * executing a Z-Script command with no output.  Create the "Notifier"
 * widget class.  Create the zmlapp class, which inherits from
 * spCursesIm; ZmlIm is now a zmlapp instance (so it can override the
 * refocus method, which it does to update the menubar help label).
 * Refocus after binding/unbinding keys.  Make "multikey -l" verbose
 * about what it's accomplishing.
 *
 * Revision 2.9  1994/01/13  21:28:49  bobg
 * Change the parameters of the insertActionAreaItem,
 * removeActionAreaItem, installZbutton, uninstallZbutton, and
 * updateZbutton methods so that an action area to operate on can be
 * passed in.  This is used in the case where an action area exists but
 * is hidden.  Previously, operations on hidden action areas worked by
 * temporarily unhiding the action area, which was amazingly time
 * consuming.
 * 	Don't display negative percentages in task meters.
 * 	Dialogs don't need to remember their folders' names if they're
 * remembering their folders.  Further, don't free the foldername string
 * since it's owned by the core!  (This fixes numerous stack-trashing and
 * core-dumping bugs).
 * 	Oops, don't declare a local variable with the same name as a
 * parameter.
 *
 * Revision 2.8  1994/01/01  21:38:52  bobg
 * Add callbacks for folder_title, hidden, summary_fmt, and templates.
 * Finalize (generic) dialogs correctly.
 */

#ifndef DIALOG_H
# define DIALOG_H

# include <spoor.h>
# include <spoor/wrapview.h>

# include <spoor/menu.h>
# include <spoor/buttonv.h>

# include <zmail.h>
# include <zfolder.h>
# include <buttons.h>

struct dialog {
    SUPERCLASS(spWrapview);
    char *mgroupstr;
    struct spView *lastFocus;
    struct dlist focuslist;
    struct spMenu *menu;
    struct spTextview *menuhelp;
    struct spButtonv *actionArea;
    struct spView *view, *savedView;
    struct spCmdline *folder, *messages;
    int activeIndex, interactingModally, resetChildren, activated;
    int mungelevel, needaafocus;
    unsigned long options;
    struct spSplitview *s1, *s2, *s3, *s4, *s5;
    struct spWrapview *w1, *w2;
    ZmCallback foldertitle_cb;
};

/* Add field accessors */
# define dialog_activeIndex(x) \
    (((struct dialog *) (x))->activeIndex)
# define dialog_lastFocus(x) \
    (((struct dialog *) (x))->lastFocus)
# define dialog_options(x) \
    (((struct dialog *) (x))->options)
# define dialog_menu(x) \
    (((struct dialog *) (x))->menu)
# define dialog_actionArea(x) \
    (((struct dialog *) (x))->actionArea)
# define dialog_view(x) \
    (((struct dialog *) (x))->view)
# define dialog_folder(x) \
    (((struct dialog *) (x))->folder)
# define dialog_messages(x) \
    (((struct dialog *) (x))->messages)

/* Declare method selectors */
extern int m_dialog_menuhelp;
extern int m_dialog_removeActionAreaItem;
extern int m_dialog_setopts;
extern int m_dialog_addFocusView;
extern int m_dialog_clearFocusViews;
extern int m_dialog_setActionArea;
extern int m_dialog_setMenu;
extern int m_dialog_setView;
extern int m_dialog_interactModally;
extern int m_dialog_bury;
extern int m_dialog_enter;
extern int m_dialog_leave;
extern int m_dialog_deactivate;
extern int m_dialog_folder;
extern int m_dialog_foldername;
extern int m_dialog_mgroup;
extern int m_dialog_mgroupstr;
extern int m_dialog_activate;
extern int m_dialog_insertActionAreaItem;
extern int m_dialog_setmgroup;
extern int m_dialog_setfolder;
extern int m_dialog_installZbuttonList;
extern int m_dialog_uninstallZbuttonList;
extern int m_dialog_installZbutton;
extern int m_dialog_uninstallZbutton;
extern int m_dialog_updateZbutton;
extern int m_dialog_setChildren;

extern struct spWclass *dialog_class;

extern struct spWidgetInfo *spwc_ActionArea;
extern struct spWidgetInfo *spwc_Popup;
extern struct spWidgetInfo *spwc_Dialog;
extern struct spWidgetInfo *spwc_MenuScreen;
extern struct spWidgetInfo *spwc_Screen;
extern struct spWidgetInfo *spwc_MenuPopup;

extern void dialog_InitializeClass P((void));

# define dialog_NEW() \
    ((struct dialog *) spoor_NewInstance(dialog_class))

enum dialog_DeactivateReason {
    dialog_Close,
    dialog_Cancel,
    dialog_DEACTIVATEREASONS
};

enum dialog_observation {
    dialog_updateList = spWrapview_OBSERVATIONS,
    dialog_refresh,
    dialog_OBSERVATIONS
};

enum dialog_updateFlag {
    dialog_listUpdate = spWrapview_UPDATEFLAGS,
    dialog_UPDATEFLAGS
};

enum dialog_option {
    dialog_ShowFolder = (1 << 0),
    dialog_ShowMessages = (1 << 1),
    dialog_OPTIONS = (1 << 2)
};

extern struct options **ZmlUpdatedList;
extern struct dialog *CurrentDialog;
extern msg_group *current_mgroup;

extern int AdjustedButtonPosition P((ZmButton, ZmButtonList));

# define dialog_MUNGE(self) \
    do { \
        struct dialog *_t = (struct dialog *) (self); \
	\
	++(_t)->mungelevel; \
	SPOOR_PROTECT { \
            TRY

# define dialog_ENDMUNGE \
            FINALLY { \
                if ((--((_t)->mungelevel)) == 0) { \
                    spSend(_t, m_dialog_setChildren, \
		           _t->menu, _t->view, _t->actionArea, \
		           0, 0, 0); \
		} \
            } ENDTRY; \
	} SPOOR_ENDPROTECT; \
    } while (0)

#endif /* DIALOG_H */
