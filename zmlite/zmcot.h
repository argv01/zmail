/* 
 * $RCSfile: zmcot.h,v $
 * $Revision: 2.6 $
 * $Date: 1995/09/20 06:44:38 $
 * $Author: liblit $
 *
 * $Log: zmcot.h,v $
 * Revision 2.6  1995/09/20 06:44:38  liblit
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
 * Revision 2.5  1994/04/05 06:08:18  bobg
 * Some Chevron stuff.  Fix switching-to-backup-folder bug.  Supply
 * gui_print_status().
 *
 * Revision 2.4  1994/03/15  22:32:24  bobg
 * Some changes.
 *
 * Revision 2.3  1994/03/09  21:45:52  bobg
 * Do NOT define ZMCOT by default.
 *
 * Revision 2.2  1994/03/03  21:38:49  bobg
 * Fix bugs, some serious.  Make several functions match their
 * prototypes.
 *
 * Revision 2.1  1994/03/03  07:36:23  bobg
 * Add new COT custom stuff.  Add a new widget class, Screen, which is a
 * subclass of Dialog; it distinguishes full-screen dialogs from popups.
 * Change MenuDialog to MenuScreen.  Some of Dialog's interactions have
 * now moved into Screen.  Shorten CVS logs.  Add the following new
 * subclasses of Screen:  Aliases; Envelope; Headers; Variables.  Don't
 * ever allow focus to land in the main screen's Folder: field.  Don't
 * allow pinups!
 *
 */

#ifndef ZMCOT_H
#define ZMCOT_H

#ifdef ZMCOT

#include <spoor.h>
#include <dialog.h>

#include <spoor/cmdline.h>
#include <spoor/textview.h>
#include <spoor/event.h>

struct zmcot {
    SUPERCLASS(dialog);
    struct spCmdline *to, *subject;
    struct spTextview *composeBody, *receiveBody;
    struct {
	struct spTextview *to, *from, *subject, *date;
    } rechdrs;
    struct {
	struct spTextview *top;
	struct spTextview *fldr;
    } status;
    struct spEvent *timeEvent;
    struct {
	msg_folder *fldr;
	int num;
    } lastDisplayed, currentDisplayed;
    int msgoffset, msgflags, is_from_trader;
};

extern struct spWclass *zmcot_class;

extern int m_zmcot_clear;
extern int m_zmcot_append;
extern int m_zmcot_setmsg;

#define zmcot_NEW() \
    ((struct zmcot *) spoor_NewInstance(zmcot_class))

extern void zmcot_InitializeClass P((void));

extern void zmcot_active P((void));

#endif /* ZMCOT */

#endif /* ZMCOT_H */
