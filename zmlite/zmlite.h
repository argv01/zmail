/* 
 * $RCSfile: zmlite.h,v $
 * $Revision: 2.12 $
 * $Date: 1995/09/20 06:44:44 $
 * $Author: liblit $
 *
 * $Log: zmlite.h,v $
 * Revision 2.12  1995/09/20 06:44:44  liblit
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
 * Revision 2.11  1995/07/08 00:39:03  spencer
 * Cast ZmlIm to struct spCursesIm * in the LITE_BUSY and LITE_ENDBUSY
 * macros, for one fewer implicit pointer cast warning from the compiler
 * per instance :-)
 *
 * Revision 2.10  1994/07/05  23:07:39  bobg
 * Brr!  It was starting to get pretty chilly out there on the
 * lite-fcs-freeze branch.  I sure am glad I'm merging back onto the
 * trunk now.
 *
 * If this causes any problems, see Carlyn, who has asked to deal
 * directly with difficulties arising from this merge.
 *
 * Revision 2.9.2.1  1994/05/16  23:13:59  bobg
 * Do the screencmd/lockscreen thing when calling edit_file.  Understand
 * DIALOG_IF_HIDDEN and WPRINT_ALWAYS flags in gui_watch_filed.  Lock the
 * screen during the crucial parts of gui_execute.  Don't turn the
 * absence of a doc string in a deferred "bindkey" command into the
 * presence of an empty doc string.
 *
 * Revision 2.9  1994/05/11  07:29:29  bobg
 * Include zmlutil.h.
 *
 * Revision 2.8  1994/03/03  07:36:27  bobg
 * Add new COT custom stuff.  Add a new widget class, Screen, which is a
 * subclass of Dialog; it distinguishes full-screen dialogs from popups.
 * Change MenuDialog to MenuScreen.  Some of Dialog's interactions have
 * now moved into Screen.  Shorten CVS logs.  Add the following new
 * subclasses of Screen:  Aliases; Envelope; Headers; Variables.  Don't
 * ever allow focus to land in the main screen's Folder: field.  Don't
 * allow pinups!
 *
 * Revision 2.7  1994/02/24  19:21:02  bobg
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
 * Revision 2.6  1994/01/17  19:28:19  bobg
 * Memorize the menu help keysequence globally, not per-dialog.  Don't
 * attempt to compute the menu help while in a menu, nor while a
 * taskmeter is going.
 *
 * Revision 2.5  1994/01/14  05:33:36  bobg
 * Use the restored Popup widget-class.  Show the "simplest" keysequence
 * for jumping to the menubar.  Don't deselect all messages after
 * executing a Z-Script command with no output.  Create the "Notifier"
 * widget class.  Create the zmlapp class, which inherits from
 * spCursesIm; ZmlIm is now a zmlapp instance (so it can override the
 * refocus method, which it does to update the menubar help label).
 * Refocus after binding/unbinding keys.  Make "multikey -l" verbose
 * about what it's accomplishing.
 */

#ifndef ZMLITE_H
# define ZMLITE_H

# include <zmcot.h>

# include <zmail.h>
# include <zmcomp.h>
# include <zmlapp.h>

# include <zmlutil.h>

extern struct zmlapp *ZmlIm;

enum zcmd_mode {                /* controls whether ZCommand changes */
    zcmd_ignore,                /* the frame's message list according */
    zcmd_use,                   /* to a command's output */
    zcmd_useIfNonempty,
    zcmd_commandline,           /* mode used by type-in command line */
    zcmd_MODES
};

/* The dialogs that stick around */
extern struct zmlmainframe *MainDialog;
extern struct zmlmsgframe *MessageDialog;
extern struct tsearch *TextsearchDialog;

#ifdef ZMCOT
extern struct zmcot *ZmcotDialog;
#endif /* ZMCOT */

extern struct zmlcomposeframe *ComposeDialog;

extern struct spWidgetInfo *spwc_FileList;
extern struct spWidgetInfo *spwc_FolderStatusList;
extern struct spWidgetInfo *spwc_MessageSummaries;
extern struct spWidgetInfo *spwc_PullrightMenu;
extern struct spWidgetInfo *spwc_Radiogroup;
extern struct spWidgetInfo *spwc_Togglegroup;
extern struct spWidgetInfo *spwc_Commandfield;

extern int taskmeter_up;
extern u_long RefreshReason;

extern struct dialog *Dialog P((GENERIC_POINTER_TYPE *));
extern void ZCommand P((char *, enum zcmd_mode));
extern void lite_busy P((void)), lite_unbusy P((void));

extern void EnterScreencmd P((int)), ExitScreencmd P((int));

#define LITE_BUSY \
    do { \
	spCursesIm_busy((struct spCursesIm *)ZmlIm); \
        TRY {

#define LITE_ENDBUSY \
        } FINALLY { \
	    spCursesIm_unbusy((struct spCursesIm *)ZmlIm); \
	} ENDTRY; \
    } while (0)

#endif /* ZMLITE_H */
