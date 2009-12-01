/*
 * $RCSfile: hdrsf.h,v $
 * $Revision: 2.6 $
 * $Date: 1995/09/20 06:44:11 $
 * $Author: liblit $
 *
 * $Log: hdrsf.h,v $
 * Revision 2.6  1995/09/20 06:44:11  liblit
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
 * Revision 2.5  1994/03/03 07:36:05  bobg
 * Add new COT custom stuff.  Add a new widget class, Screen, which is a
 * subclass of Dialog; it distinguishes full-screen dialogs from popups.
 * Change MenuDialog to MenuScreen.  Some of Dialog's interactions have
 * now moved into Screen.  Shorten CVS logs.  Add the following new
 * subclasses of Screen:  Aliases; Envelope; Headers; Variables.  Don't
 * ever allow focus to land in the main screen's Folder: field.  Don't
 * allow pinups!
 *
 * Revision 2.4  1993/12/01  00:10:16  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Permit Z-Scriptable buttons and menus.
 */

#ifndef ZMLHDRSFRAME_H
#define ZMLHDRSFRAME_H

#include <spoor.h>
#include <dialog.h>
#include <spoor/cmdline.h>
#include <spoor/listv.h>
#include <spoor/buttonv.h>

struct zmlhdrsframe {
    SUPERCLASS(dialog);
    struct spCmdline *name;
    struct spButtonv *control;
    struct spListv *current, *available;
};

extern struct spWclass *zmlhdrsframe_class;

extern struct spWidgetInfo *spwc_Headers;

#define zmlhdrsframe_NEW() \
    ((struct zmlhdrsframe *) spoor_NewInstance(zmlhdrsframe_class))

extern void zmlhdrsframe_InitializeClass P((void));

#endif /* ZMLHDRSFRAME_H */
