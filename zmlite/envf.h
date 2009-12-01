/*
 * $RCSfile: envf.h,v $
 * $Revision: 2.6 $
 * $Date: 1995/09/20 06:44:07 $
 * $Author: liblit $
 *
 * $Log: envf.h,v $
 * Revision 2.6  1995/09/20 06:44:07  liblit
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
 * Revision 2.5  1994/03/03 07:36:03  bobg
 * Add new COT custom stuff.  Add a new widget class, Screen, which is a
 * subclass of Dialog; it distinguishes full-screen dialogs from popups.
 * Change MenuDialog to MenuScreen.  Some of Dialog's interactions have
 * now moved into Screen.  Shorten CVS logs.  Add the following new
 * subclasses of Screen:  Aliases; Envelope; Headers; Variables.  Don't
 * ever allow focus to land in the main screen's Folder: field.  Don't
 * allow pinups!
 *
 * Revision 2.4  1993/12/01  00:10:06  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Permit Z-Scriptable buttons and menus.
 */

#ifndef ZMLENVFRAME_H
#define ZMLENVFRAME_H

#include <spoor.h>
#include <dialog.h>
#include <spoor/cmdline.h>
#include <spoor/listv.h>

struct zmlenvframe {
    SUPERCLASS(dialog);
    struct spCmdline *name, *text;
    struct spListv *list;
};

extern struct spWclass *zmlenvframe_class;

extern struct spWidgetInfo *spwc_Envelope;

#define zmlenvframe_NEW() \
    ((struct zmlenvframe *) spoor_NewInstance(zmlenvframe_class))

extern void zmlenvframe_InitializeClass P((void));

#endif /* ZMLENVFRAME_H */
