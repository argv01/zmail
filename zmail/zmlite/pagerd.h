/*
 * $RCSfile: pagerd.h,v $
 * $Revision: 2.8 $
 * $Date: 1995/09/20 06:44:23 $
 * $Author: liblit $
 *
 * $Log: pagerd.h,v $
 * Revision 2.8  1995/09/20 06:44:23  liblit
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
 * Revision 2.7  1994/05/11 02:04:50  bobg
 * Don't allow switching screens when modal dialogs are up.  Implement
 * gui_clean_compose.  Don't wedge when changing main_panes outside of
 * the Main context.  Get rid of Help button on pager dialog (per
 * Carlyn).  Add ability to set pager title.  Change ^X-w to report info
 * on enclosing Dialog as well as current widget.  Implement
 * DIALOG_NEEDED flag for gui_watch_filed.  Recognize LITETERM as
 * superseding TERM.  Add -q (quiet) flag to "multikey -l".  Actually
 * *use* the new attachment Comment field when sending attachments.
 *
 * Revision 2.6  1994/05/04  07:32:12  bobg
 * Improve dead-letter test.  Fix extreme brokennesses in dynhdr dialog.
 * Modernize pager dialog; make Pager and Help classes subclasses of
 * MenuPopup.  Restore old summary-selection behavior which occasionally
 * moves the cursor unexpectedly, but which is right 99% of the time.
 *
 * Revision 2.5  1994/02/24  19:20:27  bobg
 * Switch over to the new way of doing widget classes.  Previously,
 * hashtable lookups due to widget-class searches accounted for more than
 * 33% of Lite's running time.  The new widget class scheme eliminates
 * nearly all those lookups, replacing them with simple pointer
 * traversals and structure dereferences.  Profiling this version of Lite
 * reveals that we're back to the state where the main thing slowing down
 * Lite is the core itself.  Yay!
 *
 * Can you believe all this worked perfectly on the very first try?
 */

#ifndef PAGERDIALOG_H
#define PAGERDIALOG_H

#include <spoor.h>
#include <dialog.h>

#include <dynstr.h>

#include <spoor/splitv.h>
#include <spoor/textview.h>
#include <spoor/buttonv.h>
#include <spoor/toggle.h>

struct pagerDialog {
    SUPERCLASS(dialog);
    struct spTextview *text;
    struct dynstr filename;
    struct spToggle *editable;
    int modified;
};

extern struct spWclass *pagerDialog_class;

extern struct spWidgetInfo *spwc_Pager;

#define pagerDialog_textview(x) \
    (((struct pagerDialog *) (x))->text)

#define pagerDialog_NEW() \
    ((struct pagerDialog *) spoor_NewInstance(pagerDialog_class))

extern int m_pagerDialog_append;
extern int m_pagerDialog_clear;
extern int m_pagerDialog_setFile;
extern int m_pagerDialog_setTitle;

extern void pagerDialog_InitializeClass P((void));

#endif /* PAGERDIALOG_H */
