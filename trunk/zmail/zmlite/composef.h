/*
 * $RCSfile: composef.h,v $
 * $Revision: 2.21 $
 * $Date: 1998/12/07 23:56:32 $
 * $Author: schaefer $
 *
 * $Log: composef.h,v $
 * Revision 2.21  1998/12/07 23:56:32  schaefer
 * Merge NetManage's changes from Z-Mail 5.0 on the Motif-4-1-branch:
 *
 * Add IMAP support.  No more editorial comments, you know what I think.
 * Add LDAP support.  (Bite tongue.)
 * Add phone-tag and tag-it support for ECCO/ZM-Pro integration.
 *
 * Revision 2.20.4.2  1997/07/07 18:50:30  bentley
 * Removed a variable that is no longer used.
 *
 * Revision 2.20.4.1  1997/04/15 15:25:28  bentley
 * Added variables for Phone-Tag and Tag-It.
 *
 * Revision 2.20  1995/09/20 06:44:00  liblit
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
 * Revision 2.19  1995/09/07 23:55:48  liblit
 * Factor some common code for filling in the four header fields at the
 * top of a compose window.  This change opens up the potential for
 * refilling individual fields without touching the other three.  Using
 * this new flexibility....
 *
 * Use ZCBTYPE_ADDRESS ZmCallbacks to keep current when to, subject, cc,
 * or bcc changes.  Previously, we were refilling all four at the end of
 * many compcmd's.  This should be a bit more efficient.  More
 * importantly, it keeps us current even when the change was not enacted
 * by a compcmd.  This can happen, for example, when directory services
 * are in use.
 *
 * PR #6187.
 *
 * Revision 2.18  1994/04/17 06:29:20  bobg
 * Rewrite the attachments dialog to be more correct.  It now uses the
 * Z-Script attachments API wherever possible, and deals more correctly
 * with its two list widgets.
 *
 * Revision 2.17  1994/02/24  19:19:41  bobg
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
 * Revision 2.16  1994/02/18  19:58:35  bobg
 * The changes from the avoid-timer-api branch, plus:  don't allow
 * wprint("\n") to blank the status line.
 *
 * Revision 2.15  1994/02/02  18:33:43  bobg
 * Correct new bugs introduced by yesterday's fixes.  Compositions should
 * no longer start with 0 or 2 sets of headers.  (Also eliminated yet
 * more redundancy in compose-dialog creation.)  When programmatically
 * setting a dialog's Messages: string, protect it from "reverting" when
 * focus is lost.
 *
 * Revision 2.14  1994/01/25  06:49:12  bobg
 * Turn off the ACTIVE bit of a composition during an external edit.
 * Correctly detect whether or not the action area should be visible in
 * the compose screen.  Correctly handle the case where main_panes equals
 * "folder,messages".  Select the first new or unread message in a
 * newly-opened folder.
 *
 * Revision 2.13  1994/01/19  03:53:09  bobg
 * Fix some problems in the text search dialog.  Wire it up correctly.
 *
 * Revision 2.12  1994/01/17  03:55:46  bobg
 * No longer keep pointers to attach dialogs in compose and message
 * dialogs.
 *
 * Revision 2.11  1994/01/01  19:08:23  bobg
 * Install the callback for compose_panes.  Correctly draw the Messages:
 * field, even when the widget is created after the dialog gets its
 * message-group.  Correctly compute the list positions of message
 * summaries that follow hidden summaries.  Call frameHighlighted after
 * redrawing summaries.  Don't dismiss the message screen just because
 * the message it's viewing has changed position in the folder.
 */

#ifndef ZMLCOMPOSEFRAME_H
#define ZMLCOMPOSEFRAME_H

#include <spoor.h>
#include <dialog.h>

#include <spoor/textview.h>
#include <spoor/buttonv.h>
#include <dynhdrs.h>
#include <zmail.h>
#include <zmcomp.h>

#define ABNORMAL_STATE  -1
#define NORMAL_STATE    0
#define TAG_IT_STATE    1 
#define PHONE_TAG_STATE 2

struct zmlcomposeframe {
    SUPERCLASS(dialog);
    struct Compose *comp;
    struct {
	struct spCmdline *to, *subject, *cc, *bcc , *date_time, *signed_by;
	struct spCmdline *from, *of, *msg_date, *msg_time;
        struct spCmdline *area_code, *phone_number;
	struct spCmdline *extension, *fax_number;
	struct spCmdline *label1, *label2;
	struct spSplitview *tree;
	ZmCallback recipients_cb, subject_cb;
    } headers;
    struct spButtonv *aa;
    struct spTextview *body;
    struct dynhdrs *dh;
    int paneschanged, new, edit_hdrs, the_state;
    ZmCallback state_cb, panes_cb;
    struct spButtonv *phone_tag_options;
    struct {
        struct spToggle *phoned, *call_back, *returned_call;
        struct spToggle *see_you, *was_in, *will_call;
        struct spToggle *urgent;
    } toggles;
};

#define zmlcomposeframe_dh(x) \
    (((struct zmlcomposeframe *) (x))->dh)
#define zmlcomposeframe_comp(z) (((struct zmlcomposeframe *) (z))->comp)
#define zmlcomposeframe_pending(z) (((struct zmlcomposeframe *) (z))->pending)
#define zmlcomposeframe_body(z) (((struct zmlcomposeframe *) (z))->body)

extern struct spWclass *zmlcomposeframe_class;

extern struct spWidgetInfo *spwc_Compose;

extern int m_zmlcomposeframe_readEdFile;
extern int m_zmlcomposeframe_writeEdFile;

#define zmlcomposeframe_NEW() \
    ((struct zmlcomposeframe *) spoor_NewInstance(zmlcomposeframe_class))

enum zmlcomposeframe_observation {
    zmlcomposeframe_changeAttachments = dialog_OBSERVATIONS,
    zmlcomposeframe_OBSERVATIONS
};

extern void zmlcomposeframe_InitializeClass P((void));

#endif /* ZMLCOMPOSEFRAME_H */
